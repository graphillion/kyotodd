#include "bdd.h"
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
        count_memo_ = std::make_shared<BddCountMemo>();
        return bddexactcount(root, *count_memo_);
    }
    return bddexactcount(root);
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
