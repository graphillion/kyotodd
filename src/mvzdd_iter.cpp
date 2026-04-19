#include "mvzdd_iter.h"
#include <stdexcept>
#include <string>

namespace kyotodd {

// ---- MVZDDRankIterator ----

MVZDDRankIterator::MVZDDRankIterator(const MVZDD& mv)
    : table_(mv.var_table()),
      inner_(mv.to_zdd()),
      end_(),
      exhausted_(false) {
    if (!table_) {
        throw std::logic_error("MVZDDRankIterator: no var table");
    }
    if (inner_ == end_) {
        exhausted_ = true;
        return;
    }
    refresh_current();
}

MVZDDRankIterator::MVZDDRankIterator()
    : table_(), inner_(), end_(), exhausted_(true) {}

void MVZDDRankIterator::refresh_current() {
    const std::vector<bddvar>& dd_vars = *inner_;
    bddvar n = table_->mvdd_var_count();
    current_.assign(n, 0);
    for (size_t j = 0; j < dd_vars.size(); ++j) {
        bddvar dv = dd_vars[j];
        bddvar mv = table_->mvdd_var_of(dv);
        if (mv == 0) {
            throw std::logic_error(
                "MVZDDRankIterator: DD variable " + std::to_string(dv) +
                " is not registered in the var table");
        }
        int idx = table_->dd_var_index(dv);
        current_[mv - 1] = idx + 1;
    }
}

MVZDDRankIterator::reference MVZDDRankIterator::operator*() const {
    return current_;
}

MVZDDRankIterator::pointer MVZDDRankIterator::operator->() const {
    return &current_;
}

MVZDDRankIterator& MVZDDRankIterator::operator++() {
    if (!exhausted_) {
        ++inner_;
        if (inner_ == end_) {
            exhausted_ = true;
        } else {
            refresh_current();
        }
    }
    return *this;
}

MVZDDRankIterator MVZDDRankIterator::operator++(int) {
    MVZDDRankIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool MVZDDRankIterator::operator==(const MVZDDRankIterator& other) const {
    return exhausted_ == other.exhausted_;
}

bool MVZDDRankIterator::operator!=(const MVZDDRankIterator& other) const {
    return !(*this == other);
}

// ---- MVZDDRankRange ----

MVZDDRankRange::MVZDDRankRange(const MVZDD& mv) : mv_(mv) {}

MVZDDRankIterator MVZDDRankRange::begin() const {
    return MVZDDRankIterator(mv_);
}

MVZDDRankIterator MVZDDRankRange::end() const {
    return MVZDDRankIterator();
}

} // namespace kyotodd
