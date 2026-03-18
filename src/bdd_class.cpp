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

// --- ZddCountMemo constructors ---

ZddCountMemo::ZddCountMemo(bddp f) : f_(f), stored_(false), map_() {}
ZddCountMemo::ZddCountMemo(const ZDD& f) : f_(f.get_id()), stored_(false), map_() {}

// --- BddCountMemo constructors ---

BddCountMemo::BddCountMemo(bddp f, bddvar n) : f_(f), n_(n), stored_(false), map_() {}
BddCountMemo::BddCountMemo(const BDD& f, bddvar n) : f_(f.get_id()), n_(n), stored_(false), map_() {}

bigint::BigInt ZDD::exact_count() const {
    if (count_memo_) {
        return bddexactcount(root, *count_memo_);
    }
    return bddexactcount(root);
}

bigint::BigInt ZDD::exact_count(ZddCountMemo& memo) const {
    if (memo.f() != root) {
        throw std::invalid_argument(
            "exact_count: memo was created for a different ZDD");
    }
    if (memo.stored()) {
        return bddexactcount(root, memo.map());
    }
    bigint::BigInt result = bddexactcount(root, memo.map());
    memo.mark_stored();
    return result;
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
        std::function<bigint::BigInt(const bigint::BigInt&)> rand_func,
        ZddCountMemo& memo) {
    if (root == bddempty || root == bddnull) {
        throw std::invalid_argument(
            "uniform_sample: cannot sample from empty family");
    }
    if (memo.f() != root) {
        throw std::invalid_argument(
            "uniform_sample: memo was created for a different ZDD");
    }

    // Ensure memo is populated
    if (!memo.stored()) {
        exact_count(memo);
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

        bigint::BigInt count_lo = bddexactcount(lo, memo.map());
        bigint::BigInt count_hi = bddexactcount(hi, memo.map());
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
