#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <stdexcept>

const BDD BDD::False(0);
const BDD BDD::True(1);
const BDD BDD::Null(-1);

const ZDD ZDD::Empty(0);
const ZDD ZDD::Single(1);
const ZDD ZDD::Null(-1);

bigint::BigInt ZDD::exact_count() const {
    if (count_memo_) {
        return bddexactcount(root, *count_memo_);
    }
    return bddexactcount(root);
}

bigint::BigInt ZDD::exact_count(bool save_memo) {
    if (count_memo_) {
        return bddexactcount(root, *count_memo_);
    }
    if (save_memo) {
        count_memo_ = std::make_shared<CountMemoMap>();
        return bddexactcount(root, *count_memo_);
    }
    return bddexactcount(root);
}

std::vector<bddvar> ZDD::uniform_sample_impl(
        std::function<bigint::BigInt(const bigint::BigInt&)> rand_func) {
    if (root == bddempty || root == bddnull) {
        throw std::invalid_argument(
            "uniform_sample: cannot sample from empty family");
    }

    // Ensure memo is populated
    if (!count_memo_) {
        exact_count(true);
    }

    std::vector<bddvar> result;
    bddp f = root;

    while (f != bddsingle && f != bddempty) {
        // ZDD complement semantics: complement toggles lo only
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        if (comp) {
            lo = bddnot(lo);
        }

        bigint::BigInt count_lo = bddexactcount(lo, *count_memo_);
        bigint::BigInt count_hi = bddexactcount(hi, *count_memo_);
        bigint::BigInt total = count_lo + count_hi;

        bigint::BigInt r = rand_func(total);

        if (r < count_lo) {
            f = lo;
        } else {
            result.push_back(var);
            f = hi;
        }
    }

    return result;
}

void ZDD::Print() const {
    bddvar v = Top();
    std::cout << "[ " << GetID()
              << " Var:" << v << "(" << bddlevofvar(v) << ")"
              << " Size:" << Size()
              << " Card:" << Card()
              << " Lit:" << Lit()
              << " Len:" << Len()
              << " ]" << std::endl;
    std::cout.flush();
}

void ZDD::PrintPla() const {
    throw std::logic_error("ZDD::PrintPla: not implemented");
}

ZDD ZDD::ZLev(int /*lev*/, int /*last*/) const {
    throw std::logic_error("ZDD::ZLev: not implemented");
}

void ZDD::SetZSkip() const {
    throw std::logic_error("ZDD::SetZSkip: not implemented");
}
