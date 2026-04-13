#include "bdd.h"
#include "bdd_internal.h"
#include "zdd_rank_iter.h"
#include <algorithm>
#include <stdexcept>

// ---- descend ----

bool ZddRankIterator::descend(bddp f) {
    while (!(f & BDD_CONST_FLAG)) {
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);
        if (comp) lo = bddnot(lo);

        StackFrame frame;
        frame.var = var;
        frame.lo = lo;
        frame.phase = 0;
        state_->stack.push_back(frame);
        state_->current_set.push_back(var);
        f = hi;
    }

    if (f == bddsingle) {
        current_ = state_->current_set;
        std::sort(current_.begin(), current_.end());
        return true;
    }
    return false;
}

// ---- advance ----

void ZddRankIterator::advance() {
    // If empty set was the last yielded value, start DFS.
    if (state_->has_empty && !state_->empty_yielded) {
        state_->empty_yielded = true;
        if (state_->effective_root == bddempty) {
            exhausted_ = true;
            return;
        }
        if (descend(state_->effective_root)) {
            return;
        }
        // Should not happen for non-empty effective_root.
        exhausted_ = true;
        return;
    }

    // Backtrack and find the next set.
    while (!state_->stack.empty()) {
        StackFrame& frame = state_->stack.back();
        if (frame.phase == 0) {
            // Hi subtree done. Remove var from current_set and try lo.
            state_->current_set.pop_back();
            frame.phase = 1;
            if (descend(frame.lo)) {
                return;
            }
            // lo subtree empty, pop this frame.
            state_->stack.pop_back();
        } else {
            // Both subtrees done.
            state_->stack.pop_back();
        }
    }

    exhausted_ = true;
}

// ---- constructor ----

ZddRankIterator::ZddRankIterator(const ZDD& zdd)
    : state_(std::make_shared<State>()), exhausted_(false)
{
    state_->zdd = zdd;

    bddp root = zdd.id();

    if (root == bddnull) {
        throw std::invalid_argument(
            "ZddRankIterator: null ZDD");
    }

    if (root == bddempty) {
        exhausted_ = true;
        return;
    }

    state_->has_empty = bddhasempty(root);
    if (state_->has_empty) {
        state_->effective_root = bddnot(root);
        // Yield empty set first.
        current_ = value_type();
        return;
    }

    state_->effective_root = root;
    if (!descend(root)) {
        // Should not happen for non-empty ZDD.
        exhausted_ = true;
    }
}

ZddRankIterator::ZddRankIterator()
    : state_(), exhausted_(true)
{
}

// ---- iterator operators ----

ZddRankIterator::reference
ZddRankIterator::operator*() const {
    return current_;
}

ZddRankIterator::pointer
ZddRankIterator::operator->() const {
    return &current_;
}

ZddRankIterator&
ZddRankIterator::operator++() {
    advance();
    return *this;
}

ZddRankIterator
ZddRankIterator::operator++(int) {
    ZddRankIterator tmp = *this;
    advance();
    return tmp;
}

bool ZddRankIterator::operator==(
    const ZddRankIterator& other) const
{
    bool this_end = exhausted_;
    bool other_end = other.exhausted_;
    return this_end == other_end;
}

bool ZddRankIterator::operator!=(
    const ZddRankIterator& other) const
{
    return !(*this == other);
}

// ---- ZddRankRange ----

ZddRankRange::ZddRankRange(const ZDD& zdd)
    : zdd_(zdd)
{
}

ZddRankIterator ZddRankRange::begin() const {
    return ZddRankIterator(zdd_);
}

ZddRankIterator ZddRankRange::end() const {
    return ZddRankIterator();
}
