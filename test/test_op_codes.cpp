#include <gtest/gtest.h>
#include "bdd.h"
#include "pidd.h"
#include "rotpidd.h"
#include "seqbdd.h"
#include "sop.h"
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace kyotodd;

// Every built-in operation-cache op code must be unique. Two operations that
// share a code can read each other's cache entries and return wrong results.

namespace {

std::vector<std::pair<std::string, uint8_t> > static_op_codes() {
    std::vector<std::pair<std::string, uint8_t> > v;
#define KYOTODD_OP(name) v.push_back(std::make_pair(std::string(#name), name))
    KYOTODD_OP(BDD_OP_ALWAYS);
    KYOTODD_OP(BDD_OP_AND);
    KYOTODD_OP(BDD_OP_AND_EXIST);
    KYOTODD_OP(BDD_OP_AT0);
    KYOTODD_OP(BDD_OP_AT1);
    KYOTODD_OP(BDD_OP_CARD);
    KYOTODD_OP(BDD_OP_CHANGE);
    KYOTODD_OP(BDD_OP_CHOOSE);
    KYOTODD_OP(BDD_OP_CLOSURE);
    KYOTODD_OP(BDD_OP_COFACTOR);
    KYOTODD_OP(BDD_OP_COIMPLYSET);
    KYOTODD_OP(BDD_OP_DELTA);
    KYOTODD_OP(BDD_OP_DISJOIN);
    KYOTODD_OP(BDD_OP_DIV);
    KYOTODD_OP(BDD_OP_EXIST);
    KYOTODD_OP(BDD_OP_IMPLY);
    KYOTODD_OP(BDD_OP_INTERSEC);
    KYOTODD_OP(BDD_OP_ISOP1);
    KYOTODD_OP(BDD_OP_ISOP2);
    KYOTODD_OP(BDD_OP_ITE);
    KYOTODD_OP(BDD_OP_JOIN);
    KYOTODD_OP(BDD_OP_JOINTJOIN);
    KYOTODD_OP(BDD_OP_LEN);
    KYOTODD_OP(BDD_OP_LIT);
    KYOTODD_OP(BDD_OP_LSHIFT);
    KYOTODD_OP(BDD_OP_LSHIFTB);
    KYOTODD_OP(BDD_OP_LSHIFTZ);
    KYOTODD_OP(BDD_OP_MAXIMAL);
    KYOTODD_OP(BDD_OP_MEET);
    KYOTODD_OP(BDD_OP_MINHIT);
    KYOTODD_OP(BDD_OP_MINIMAL);
    KYOTODD_OP(BDD_OP_MINSIZE);
    KYOTODD_OP(BDD_OP_OFFSET);
    KYOTODD_OP(BDD_OP_ONSET);
    KYOTODD_OP(BDD_OP_ONSET0);
    KYOTODD_OP(BDD_OP_PERMIT);
    KYOTODD_OP(BDD_OP_PERMITSYM);
    KYOTODD_OP(BDD_OP_PIDD_COFACT);
    KYOTODD_OP(BDD_OP_PIDD_DIV);
    KYOTODD_OP(BDD_OP_PIDD_MULT);
    KYOTODD_OP(BDD_OP_PIDD_ODD);
    KYOTODD_OP(BDD_OP_PIDD_SWAP);
    KYOTODD_OP(BDD_OP_PRODUCT);
    KYOTODD_OP(BDD_OP_REMAINDER);
    KYOTODD_OP(BDD_OP_REMOVE_SUBSETS);
    KYOTODD_OP(BDD_OP_REMOVE_SUPERSETS);
    KYOTODD_OP(BDD_OP_RESTRICT);
    KYOTODD_OP(BDD_OP_ROTPIDD_COFACT);
    KYOTODD_OP(BDD_OP_ROTPIDD_CONTMAX);
    KYOTODD_OP(BDD_OP_ROTPIDD_INSERT);
    KYOTODD_OP(BDD_OP_ROTPIDD_INVERSE);
    KYOTODD_OP(BDD_OP_ROTPIDD_LEFTROT);
    KYOTODD_OP(BDD_OP_ROTPIDD_MULT);
    KYOTODD_OP(BDD_OP_ROTPIDD_NORMALIZE);
    KYOTODD_OP(BDD_OP_ROTPIDD_ODD);
    KYOTODD_OP(BDD_OP_ROTPIDD_ORDER);
    KYOTODD_OP(BDD_OP_ROTPIDD_REMOVEMAX);
    KYOTODD_OP(BDD_OP_ROTPIDD_REVERSE);
    KYOTODD_OP(BDD_OP_ROTPIDD_SWAP);
    KYOTODD_OP(BDD_OP_RSHIFT);
    KYOTODD_OP(BDD_OP_RSHIFTB);
    KYOTODD_OP(BDD_OP_RSHIFTZ);
    KYOTODD_OP(BDD_OP_SEQBDD_CONCAT);
    KYOTODD_OP(BDD_OP_SEQBDD_LQUOT);
    KYOTODD_OP(BDD_OP_SMOOTH);
    KYOTODD_OP(BDD_OP_SOP_BDD);
    KYOTODD_OP(BDD_OP_SOP_DIV);
    KYOTODD_OP(BDD_OP_SOP_IMPL);
    KYOTODD_OP(BDD_OP_SOP_MULT);
    KYOTODD_OP(BDD_OP_SPREAD);
    KYOTODD_OP(BDD_OP_SUBTRACT);
    KYOTODD_OP(BDD_OP_SWAP);
    KYOTODD_OP(BDD_OP_SYMCHK);
    KYOTODD_OP(BDD_OP_SYMDIFF);
    KYOTODD_OP(BDD_OP_SYMSET);
    KYOTODD_OP(BDD_OP_UNION);
    KYOTODD_OP(BDD_OP_UNIV);
    KYOTODD_OP(BDD_OP_XOR);
    KYOTODD_OP(BDD_OP_ZSKIP);
#undef KYOTODD_OP
    return v;
}

}  // namespace

TEST(OpCodeTest, StaticOpCodesAreDistinct) {
    std::vector<std::pair<std::string, uint8_t> > ops = static_op_codes();
    std::set<uint8_t> seen;
    for (size_t i = 0; i < ops.size(); ++i) {
        EXPECT_TRUE(seen.insert(ops[i].second).second)
            << ops[i].first << " = " << static_cast<int>(ops[i].second)
            << " collides with another op code";
    }
}

TEST(OpCodeTest, StaticOpCodesBelowDynamicRange) {
    std::vector<std::pair<std::string, uint8_t> > ops = static_op_codes();
    for (size_t i = 0; i < ops.size(); ++i) {
        EXPECT_LT(ops[i].second, BDD_OP_DYNAMIC_MIN) << ops[i].first;
    }
    EXPECT_LT(BDD_OP_DYNAMIC_MIN, BDD_OP_USER_MIN);
}

TEST(OpCodeTest, MtbddAllocatesFromDynamicRange) {
    EXPECT_GE(mtbdd_alloc_op_code(), BDD_OP_DYNAMIC_MIN);
}
