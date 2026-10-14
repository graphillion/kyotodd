#ifndef KYOTODD_UNREDUCED_DD_TDZDD_H
#define KYOTODD_UNREDUCED_DD_TDZDD_H

/**
 * @file unreduced_dd_tdzdd.h
 * @brief UnreducedDD::construct(): build an UnreducedDD from a TdZdd spec.
 *
 * This header depends on TdZdd (third_party/TdZdd, header-only). It is
 * NOT included by the umbrella header bdd.h; include it explicitly.
 *
 * The algorithm mirrors tdzdd::DdBuilder (breadth-first, one level at a
 * time, top-down), but writes nodes directly into KyotoDD's node array:
 *
 *   1. get_root() gives the root level n.  Missing variables are created
 *      so that KyotoDD level (n + level_offset) exists.
 *   2. For i = n down to 1, the pending states at level i are
 *      deduplicated with hash_code / equal_to (and merge_states), each
 *      unique state becomes one unreduced node with bddnull placeholder
 *      children, and the parent's edge is patched to point to it.
 *      States are visited in the same order as tdzdd::DdBuilder (newest
 *      first), so order-dependent merge_states give identical results.
 *   3. Each live node is expanded with get_child(state, i, 0/1); the
 *      resulting child states are queued at their target level.
 *
 * Everything runs inside bdd_gc_guard, so GC cannot run while raw
 * placeholder nodes are unreachable from any protected root. If the node
 * table overflows, bdd_gc_guard runs GC and re-executes the whole build
 * from scratch; all resources are therefore created inside the guarded
 * lambda and released by RAII (spec states are destruct()'ed).
 *
 * The implementation is purely iterative (a loop over levels), so the
 * _rec / _iter split and the BddExecMode argument do not apply.
 */

#include "unreduced_dd.h"
#include "bdd_base.h"
#include "bdd_internal.h"

#include <tdzdd/DdSpec.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace kyotodd {
namespace tdzdd_detail {

/**
 * @brief Chunked, address-stable storage for spec states.
 *
 * TdZdd never relocates a state after get_copy(), and state types may
 * hold self-referential pointers (e.g. std::string with SSO, std::map).
 * Therefore states are stored in fixed-size chunks that are never moved.
 *
 * `count` is the number of CONSTRUCTED states. push_uninit() hands out a
 * raw slot; the caller must get_copy() into it and then call commit().
 * If get_copy() throws, the slot is simply reused and never destructed.
 */
template<typename SPEC>
class StateArena {
public:
    StateArena() : spec_(nullptr), words_(1), per_chunk_(1), count_(0) {}

    void init(SPEC* spec, size_t words) {
        spec_ = spec;
        words_ = words;
        // Roughly 64 KiB per chunk, at least 16 states per chunk.
        per_chunk_ = (64 * 1024) / (words * sizeof(size_t));
        if (per_chunk_ < 16) per_chunk_ = 16;
    }

    ~StateArena() { clear(); }

    size_t size() const { return count_; }

    void* at(size_t idx) const {
        return chunks_[idx / per_chunk_].get() + (idx % per_chunk_) * words_;
    }

    /** @brief Reserve a raw (unconstructed) slot at index size(). */
    void* push_uninit() {
        size_t chunk = count_ / per_chunk_;
        if (chunk == chunks_.size()) {
            chunks_.push_back(std::unique_ptr<size_t[]>(
                new size_t[per_chunk_ * words_]));
        }
        return at(count_);
    }

    /** @brief Mark the slot returned by push_uninit() as constructed. */
    void commit() { ++count_; }

    /** @brief Destruct and drop the last constructed state. */
    void pop_back() {
        --count_;
        spec_->destruct(at(count_));
    }

    /** @brief Destruct all constructed states (storage is kept). */
    void clear() {
        while (count_ > 0) pop_back();
    }

private:
    StateArena(const StateArena&);
    StateArena& operator=(const StateArena&);

    SPEC* spec_;
    size_t words_;
    size_t per_chunk_;
    size_t count_;
    std::vector<std::unique_ptr<size_t[]> > chunks_;
};

/** @brief Where the edge to a pending state must be written. */
struct PendingEdge {
    bddp parent;        ///< Parent node id, or bddnull for the root.
    int parent_level;   ///< TdZdd level of the parent (0 for the root).
    int branch;         ///< 0 (lo) or 1 (hi).
};

/** @brief Pending states at one level, with their incoming edges. */
template<typename SPEC>
struct LevelBucket {
    StateArena<SPEC> states;
    std::vector<PendingEdge> edges;   // parallel to states
};

/** @brief Hash / equality functors over arena indices, for one level. */
template<typename SPEC>
struct IndexHasher {
    const SPEC* spec;
    const StateArena<SPEC>* arena;
    int level;
    size_t operator()(size_t i) const {
        return spec->hash_code(arena->at(i), level);
    }
};

template<typename SPEC>
struct IndexEqual {
    const SPEC* spec;
    const StateArena<SPEC>* arena;
    int level;
    bool operator()(size_t a, size_t b) const {
        return spec->equal_to(arena->at(a), arena->at(b), level);
    }
};

/** @brief Overwrite one child slot of an unreduced node. */
inline void patch_child(bddp parent, int branch, bddp child) {
    bddvar var = node_var(parent);
    if (branch == 0) node_write(parent, var, child, node_hi(parent));
    else             node_write(parent, var, node_lo(parent), child);
}

/** @brief RAII holder for a single temporary spec state. */
template<typename SPEC>
class TempState {
public:
    TempState(SPEC& spec, size_t words)
        : spec_(spec), buf_(words), live_(false) {}
    ~TempState() { if (live_) spec_.destruct(data()); }
    void* data() { return buf_.data(); }
    void set_live(bool live) { live_ = live; }
private:
    SPEC& spec_;
    std::vector<size_t> buf_;
    bool live_;
};

/**
 * @brief Chain of padding nodes above one child, shared by all parents.
 *
 * nodes[k] is the padding node at TdZdd level (base_level + 1 + k); its
 * lo child is nodes[k-1] (or the child itself for k == 0).
 */
struct PadChain {
    int base_level;             ///< TdZdd level of the child (0 for terminals).
    std::vector<bddp> nodes;
};

/** @brief Per-entry outcome of deduplication at one level. */
enum EntryKind {
    ENTRY_LIVE = 0,   ///< Fresh node; expand it.
    ENTRY_DEAD = 1,   ///< Fresh node forwarded to 0 by merge_states; do not expand.
    ENTRY_SKIP = 2    ///< Duplicate of another entry, or forwarded to 0.
};

/**
 * @brief The whole build for one spec copy. Lives inside bdd_gc_guard.
 *
 * Returns the root node id (or a terminal).
 */
template<typename SPEC>
class SpecBuilder {
public:
    SpecBuilder(const SPEC& spec_in, SkipPadding pad, int level_offset)
        : spec_(spec_in), pad_(pad), offset_(level_offset),
          words_(compute_words(spec_)), one_(spec_, words_) {}

    bddp run() {
        // --- root ---
        TempState<SPEC> tmp(spec_, words_);
        int n = spec_.get_root(tmp.data());
        tmp.set_live(true);
        if (n == 0) return bddfalse;
        if (n < 0)  return bddtrue;

        ensure_variables(n);

        buckets_.reserve(static_cast<size_t>(n) + 1);
        for (int i = 0; i <= n; ++i) {
            buckets_.push_back(std::unique_ptr<LevelBucket<SPEC> >(new LevelBucket<SPEC>()));
            buckets_.back()->states.init(&spec_, words_);
        }
        {
            LevelBucket<SPEC>& b = *buckets_[n];
            void* p = b.states.push_uninit();
            spec_.get_copy(p, tmp.data());
            b.states.commit();
            PendingEdge e = { bddnull, 0, 0 };
            b.edges.push_back(e);
        }

        bddp root = bddnull;
        for (int i = n; i >= 1; --i) {
            process_level(i, root);
        }
        return root;
    }

private:
    SpecBuilder(const SpecBuilder&);
    SpecBuilder& operator=(const SpecBuilder&);

    static size_t compute_words(SPEC& spec) {
        int ds = spec.datasize();
        if (ds <= 0) return 1;
        return (static_cast<size_t>(ds) + sizeof(size_t) - 1) / sizeof(size_t);
    }

    void ensure_variables(int n) {
        uint64_t top = static_cast<uint64_t>(n) + static_cast<uint64_t>(offset_);
        if (top > static_cast<uint64_t>(UINT32_MAX))
            throw std::invalid_argument("UnreducedDD::construct: level + level_offset too large");
        while (static_cast<uint64_t>(bddvarused()) < top) bddnewvar();
    }

    bddvar var_of(int tdzdd_level) const {
        // Add in 64 bits: tdzdd_level + offset_ could overflow int when
        // offset_ is near INT_MAX. ensure_variables() has already checked
        // that n + offset_ fits in bddvar, and tdzdd_level <= n.
        uint64_t lev = static_cast<uint64_t>(tdzdd_level) + static_cast<uint64_t>(offset_);
        return bddvaroflev(static_cast<bddvar>(lev));
    }

    /**
     * @brief Resolve the edge target for a child at `child_level` seen
     *        from a parent at `parent_level`, applying padding if needed.
     *
     * Padding nodes are created lazily, bottom-up, and memoized per child
     * so that every parent skipping to the same child shares them. Edges
     * to the 0-terminal are never padded (same as tdzdd::Unreduction).
     */
    bddp edge_target(bddp child, int child_level, int parent_level) {
        int target_level = parent_level - 1;
        if (pad_ == SkipPadding::None || target_level <= child_level)
            return child;
        if (child == bddfalse) return child;

        PadChain& chain = pad_memo_[child];
        if (chain.nodes.empty()) chain.base_level = child_level;
        size_t need = static_cast<size_t>(target_level - chain.base_level);
        while (chain.nodes.size() < need) {
            int level = chain.base_level + 1 + static_cast<int>(chain.nodes.size());
            bddp below = chain.nodes.empty() ? child : chain.nodes.back();
            bddp hi = (pad_ == SkipPadding::Bdd) ? below : bddfalse;
            chain.nodes.push_back(UnreducedDD::getnode_raw(var_of(level), below, hi));
        }
        return chain.nodes[need - 1];
    }

    /**
     * @brief Handle a child state that reached the 1-terminal.
     *
     * Like tdzdd::DdBuilder, the first such state is kept as the "one"
     * state and every later one is offered to merge_states(one, state)
     * (without an equal_to check). Return 1 rewrites every edge recorded
     * so far to the 0-terminal and makes `state` the new one-state;
     * return 2 sends this edge to the 0-terminal; otherwise the edge goes
     * to the 1-terminal.
     *
     * @return The (possibly padded) edge target for the parent's slot.
     */
    bddp one_terminal_edge(void* state, bddp parent, int branch, int parent_level) {
        if (!one_live_) {
            spec_.get_copy(one_.data(), state);
            one_live_ = true;
            one_.set_live(true);
        } else {
            switch (spec_.merge_states(one_.data(), state)) {
            case 1:
                for (size_t k = 0; k < one_src_.size(); ++k)
                    patch_child(one_src_[k].first, one_src_[k].second, bddfalse);
                one_src_.clear();
                spec_.destruct(one_.data());
                one_live_ = false;
                one_.set_live(false);
                spec_.get_copy(one_.data(), state);
                one_live_ = true;
                one_.set_live(true);
                break;
            case 2:
                return bddfalse;
            default:
                break;
            }
        }
        one_src_.push_back(std::make_pair(parent, branch));
        return edge_target(bddtrue, 0, parent_level);
    }

    void write_edge(const PendingEdge& e, bddp target, bddp& root) {
        if (e.parent == bddnull) root = target;
        else patch_child(e.parent, e.branch, target);
    }

    void process_level(int i, bddp& root) {
        LevelBucket<SPEC>& cur = *buckets_[i];
        size_t m = cur.states.size();

        // --- (a) deduplicate, allocate nodes, patch incoming edges ---
        std::vector<bddp> node(m, bddnull);
        std::vector<unsigned char> kind(m, ENTRY_SKIP);
        IndexHasher<SPEC> hasher = { &spec_, &cur.states, i };
        IndexEqual<SPEC>  equal  = { &spec_, &cur.states, i };
        std::unordered_set<size_t, IndexHasher<SPEC>, IndexEqual<SPEC> >
            uniq(m * 2 + 1, hasher, equal);

        // Both loops below run in DESCENDING index order to reproduce the
        // exact processing order of tdzdd::DdBuilder: it pushes pending
        // states to the front of a list (MyList::alloc_front) and then
        // iterates from the front, i.e. newest first. Our buckets are
        // filled with push_back, so iterating from the back gives the
        // same order, including the order in which children are queued
        // at lower levels. This matters only for specs whose merge_states
        // (or 1-terminal handling) depends on which equal state comes
        // first, but it makes such specs behave exactly as in TdZdd.
        bddvar var = var_of(i);
        for (size_t j = m; j-- > 0; ) {
            auto ins = uniq.insert(j);
            if (ins.second) {
                node[j] = UnreducedDD::getnode_raw(var, bddnull, bddnull);
                kind[j] = ENTRY_LIVE;
            } else {
                // Equal state already seen at this level: let the spec
                // decide (see DdSpecBase: 0 = merge into the first,
                // 1 = forward the first to 0, 2 = forward the second to 0).
                size_t j0 = *ins.first;
                switch (spec_.merge_states(cur.states.at(j0), cur.states.at(j))) {
                case 1:
                    kind[j0] = ENTRY_DEAD;
                    uniq.erase(ins.first);
                    uniq.insert(j);
                    node[j] = UnreducedDD::getnode_raw(var, bddnull, bddnull);
                    kind[j] = ENTRY_LIVE;
                    break;
                case 2:
                    node[j] = bddfalse;
                    kind[j] = ENTRY_SKIP;
                    break;
                default:
                    node[j] = node[j0];
                    kind[j] = ENTRY_SKIP;
                    break;
                }
            }
            const PendingEdge& e = cur.edges[j];
            bddp target = (node[j] == bddfalse)
                ? bddfalse
                : edge_target(node[j], i, e.parent_level);
            write_edge(e, target, root);
        }

        // --- (b) expand live nodes ---
        LevelBucket<SPEC>& next = *buckets_[i - 1];
        for (size_t j = m; j-- > 0; ) {
            if (kind[j] == ENTRY_SKIP) continue;
            if (kind[j] == ENTRY_DEAD) {
                node_write(node[j], var, bddfalse, bddfalse);
                continue;
            }
            for (int b = 0; b < 2; ++b) {
                void* pp = next.states.push_uninit();
                spec_.get_copy(pp, cur.states.at(j));
                next.states.commit();
                int ii = spec_.get_child(pp, i, b);
                if (ii == 0) {
                    patch_child(node[j], b, bddfalse);
                    next.states.pop_back();
                } else if (ii < 0) {
                    patch_child(node[j], b, one_terminal_edge(pp, node[j], b, i));
                    next.states.pop_back();
                } else if (ii == i - 1) {
                    PendingEdge e = { node[j], i, b };
                    next.edges.push_back(e);
                } else if (ii < i - 1) {
                    LevelBucket<SPEC>& tgt = *buckets_[ii];
                    void* q = tgt.states.push_uninit();
                    spec_.get_copy(q, pp);
                    tgt.states.commit();
                    next.states.pop_back();
                    PendingEdge e = { node[j], i, b };
                    tgt.edges.push_back(e);
                } else {
                    throw std::logic_error(
                        "UnreducedDD::construct: get_child returned a level not lower than the current level");
                }
            }
        }

        // --- (c) release this level ---
        cur.states.clear();
        cur.edges.clear();
        spec_.destructLevel(i);
    }

    SPEC spec_;
    SkipPadding pad_;
    int offset_;
    size_t words_;
    std::vector<std::unique_ptr<LevelBucket<SPEC> > > buckets_;
    std::unordered_map<bddp, PadChain> pad_memo_;

    // The state kept for the 1-terminal, and the edges that point to it.
    TempState<SPEC> one_;
    bool one_live_ = false;
    std::vector<std::pair<bddp, int> > one_src_;
};

} // namespace tdzdd_detail

template<typename SPEC>
UnreducedDD UnreducedDD::construct(const tdzdd::DdSpecBase<SPEC, 2>& spec,
                                   SkipPadding pad, int level_offset) {
    static_assert(SPEC::ARITY == 2,
                  "UnreducedDD::construct supports binary specs only");
    if (level_offset < 0)
        throw std::invalid_argument("UnreducedDD::construct: level_offset must be non-negative");

    // The guard may re-run the lambda after a GC, so the builder (spec
    // copy, state buffers, memo) is created inside it every time.
    bddp result = bdd_gc_guard([&]() -> bddp {
        tdzdd_detail::SpecBuilder<SPEC> builder(spec.entity(), pad, level_offset);
        return builder.run();
    });
    UnreducedDD r(0);
    r.root = result;
    return r;
}

} // namespace kyotodd

#endif
