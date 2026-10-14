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
        : spec_(spec_in), pad_(pad), offset_(level_offset), words_(1) {
        int ds = spec_.datasize();
        if (ds > 0)
            words_ = (static_cast<size_t>(ds) + sizeof(size_t) - 1) / sizeof(size_t);
    }

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

    void ensure_variables(int n) {
        uint64_t top = static_cast<uint64_t>(n) + static_cast<uint64_t>(offset_);
        if (top > static_cast<uint64_t>(UINT32_MAX))
            throw std::invalid_argument("UnreducedDD::construct: level + level_offset too large");
        while (static_cast<uint64_t>(bddvarused()) < top) bddnewvar();
    }

    bddvar var_of(int tdzdd_level) const {
        return bddvaroflev(static_cast<bddvar>(tdzdd_level + offset_));
    }

    /** @brief Resolve an edge from (parent, parent_level) to `child` at `child_level`. */
    bddp edge_target(bddp child, int child_level, int parent_level) {
        (void)child_level; (void)parent_level;
        // Padding of level-skipping edges is added in a later step.
        return child;
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

        bddvar var = var_of(i);
        for (size_t j = 0; j < m; ++j) {
            auto ins = uniq.insert(j);
            if (ins.second) {
                node[j] = UnreducedDD::getnode_raw(var, bddnull, bddnull);
                kind[j] = ENTRY_LIVE;
            } else {
                size_t j0 = *ins.first;
                // merge_states handling (cases 1 / 2) is added in a later step.
                node[j] = node[j0];
                kind[j] = ENTRY_SKIP;
            }
            const PendingEdge& e = cur.edges[j];
            bddp target = (node[j] == bddfalse)
                ? bddfalse
                : edge_target(node[j], i, e.parent_level);
            write_edge(e, target, root);
        }

        // --- (b) expand live nodes ---
        LevelBucket<SPEC>& next = *buckets_[i - 1];
        for (size_t j = 0; j < m; ++j) {
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
                    patch_child(node[j], b, edge_target(bddtrue, 0, i));
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
