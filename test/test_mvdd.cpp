#include <gtest/gtest.h>
#include "bdd.h"
#include "mvdd.h"
#include <sstream>
#include <string>
#include <random>
#include <set>

class MVDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD::init(1024, 1024 * 1024);
    }
};

// ============================================================
//  MVDDVarTable tests
// ============================================================

TEST_F(MVDDTest, VarTableConstructor) {
    MVDDVarTable table(3);
    EXPECT_EQ(table.k(), 3);
    EXPECT_EQ(table.mvdd_var_count(), 0u);
}

TEST_F(MVDDTest, VarTableInvalidK) {
    EXPECT_THROW(MVDDVarTable(1), std::invalid_argument);
    EXPECT_THROW(MVDDVarTable(0), std::invalid_argument);
    EXPECT_THROW(MVDDVarTable(-1), std::invalid_argument);
    EXPECT_THROW(MVDDVarTable(65537), std::invalid_argument);
    EXPECT_NO_THROW(MVDDVarTable(2));
    EXPECT_NO_THROW(MVDDVarTable(65536));
}

TEST_F(MVDDTest, VarTableRegisterAndLookup) {
    MVDDVarTable table(3);
    bddvar d1 = bddnewvar();
    bddvar d2 = bddnewvar();
    bddvar d3 = bddnewvar();
    bddvar d4 = bddnewvar();

    bddvar mv1 = table.register_var({d1, d2});
    bddvar mv2 = table.register_var({d3, d4});

    EXPECT_EQ(mv1, 1u);
    EXPECT_EQ(mv2, 2u);
    EXPECT_EQ(table.mvdd_var_count(), 2u);

    EXPECT_EQ(table.dd_vars_of(1).size(), 2u);
    EXPECT_EQ(table.dd_vars_of(1)[0], d1);
    EXPECT_EQ(table.dd_vars_of(1)[1], d2);

    EXPECT_EQ(table.mvdd_var_of(d1), 1u);
    EXPECT_EQ(table.mvdd_var_of(d2), 1u);
    EXPECT_EQ(table.mvdd_var_of(d3), 2u);
    EXPECT_EQ(table.mvdd_var_of(d4), 2u);

    EXPECT_EQ(table.dd_var_index(d1), 0);
    EXPECT_EQ(table.dd_var_index(d2), 1);
    EXPECT_EQ(table.dd_var_index(d3), 0);
    EXPECT_EQ(table.dd_var_index(d4), 1);

    EXPECT_EQ(table.get_top_dd_var(1), d1);
    EXPECT_EQ(table.get_top_dd_var(2), d3);
}

TEST_F(MVDDTest, VarTableRegisterWrongSize) {
    MVDDVarTable table(3);
    EXPECT_THROW(table.register_var({1}), std::invalid_argument);
    EXPECT_THROW(table.register_var({1, 2, 3}), std::invalid_argument);
}

TEST_F(MVDDTest, VarTableRegisterDuplicateDDVar) {
    MVDDVarTable table(3);
    bddvar d1 = bddnewvar();
    // Same DD var twice in one call
    EXPECT_THROW(table.register_var({d1, d1}), std::invalid_argument);
}

TEST_F(MVDDTest, VarTableRegisterReusedDDVar) {
    MVDDVarTable table(3);
    bddvar d1 = bddnewvar();
    bddvar d2 = bddnewvar();
    bddvar d3 = bddnewvar();
    table.register_var({d1, d2});
    // d1 is already registered to MV var 1
    EXPECT_THROW(table.register_var({d1, d3}), std::invalid_argument);
}

TEST_F(MVDDTest, VarTableRegisterWrongLevelOrder) {
    MVDDVarTable table(3);
    bddvar d1 = bddnewvar();
    bddvar d2 = bddnewvar();
    // d2 has higher level than d1, so {d2, d1} is decreasing order → should throw
    EXPECT_THROW(table.register_var({d2, d1}), std::invalid_argument);
    // Correct order should work
    EXPECT_NO_THROW(table.register_var({d1, d2}));
}

TEST_F(MVDDTest, VarTableOutOfRange) {
    MVDDVarTable table(2);
    EXPECT_THROW(table.dd_vars_of(0), std::out_of_range);
    EXPECT_THROW(table.dd_vars_of(1), std::out_of_range);
    EXPECT_THROW(table.var_info(0), std::out_of_range);
    EXPECT_THROW(table.get_top_dd_var(0), std::out_of_range);
    EXPECT_EQ(table.mvdd_var_of(999), 0u);
    EXPECT_EQ(table.dd_var_index(999), -1);
}

// ============================================================
//  MVBDD basic tests
// ============================================================

TEST_F(MVDDTest, MVBDDDefaultConstructor) {
    MVBDD m;
    EXPECT_EQ(m.id(), bddnull);
    EXPECT_EQ(m.k(), 0);
    EXPECT_EQ(m.var_table(), nullptr);
}

TEST_F(MVDDTest, MVBDDKConstructor) {
    MVBDD m0(3, false);
    EXPECT_TRUE(m0.is_zero());
    EXPECT_EQ(m0.k(), 3);

    MVBDD m1(3, true);
    EXPECT_TRUE(m1.is_one());
    EXPECT_EQ(m1.k(), 3);
}

TEST_F(MVDDTest, MVBDDFactoryMethods) {
    MVBDD base(3);
    auto table = base.var_table();

    MVBDD z = MVBDD::zero(table);
    MVBDD o = MVBDD::one(table);

    EXPECT_TRUE(z.is_zero());
    EXPECT_TRUE(o.is_one());
    EXPECT_EQ(z.var_table().get(), table.get());
    EXPECT_EQ(o.var_table().get(), table.get());
}

TEST_F(MVDDTest, MVBDDNewVar) {
    MVBDD m(3);
    bddvar v1 = m.new_var();
    bddvar v2 = m.new_var();

    EXPECT_EQ(v1, 1u);
    EXPECT_EQ(v2, 2u);
    EXPECT_EQ(m.var_table()->mvdd_var_count(), 2u);
}

TEST_F(MVDDTest, MVBDDCopyMoveAssign) {
    MVBDD a(3, true);
    a.new_var();

    MVBDD b(a);
    EXPECT_EQ(a, b);

    MVBDD c;
    c = a;
    EXPECT_EQ(a, c);

    MVBDD d(std::move(b));
    EXPECT_EQ(a, d);
}

// ============================================================
//  MVBDD singleton and evaluation
// ============================================================

TEST_F(MVDDTest, MVBDDSingletonK3) {
    MVBDD base(3);
    bddvar v1 = base.new_var();

    MVBDD lit0 = MVBDD::singleton(base, v1, 0);
    MVBDD lit1 = MVBDD::singleton(base, v1, 1);
    MVBDD lit2 = MVBDD::singleton(base, v1, 2);

    EXPECT_TRUE(lit0.evaluate({0}));
    EXPECT_FALSE(lit0.evaluate({1}));
    EXPECT_FALSE(lit0.evaluate({2}));

    EXPECT_FALSE(lit1.evaluate({0}));
    EXPECT_TRUE(lit1.evaluate({1}));
    EXPECT_FALSE(lit1.evaluate({2}));

    EXPECT_FALSE(lit2.evaluate({0}));
    EXPECT_FALSE(lit2.evaluate({1}));
    EXPECT_TRUE(lit2.evaluate({2}));
}

TEST_F(MVDDTest, MVBDDSingletonK2) {
    MVBDD base(2);
    bddvar v1 = base.new_var();

    MVBDD lit0 = MVBDD::singleton(base, v1, 0);
    MVBDD lit1 = MVBDD::singleton(base, v1, 1);

    EXPECT_TRUE(lit0.evaluate({0}));
    EXPECT_FALSE(lit0.evaluate({1}));

    EXPECT_FALSE(lit1.evaluate({0}));
    EXPECT_TRUE(lit1.evaluate({1}));
}

TEST_F(MVDDTest, MVBDDSingletonTwoVars) {
    MVBDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    MVBDD lit = MVBDD::singleton(base, v1, 2);

    // v1 == 2, don't care about v2
    EXPECT_TRUE(lit.evaluate({2, 0}));
    EXPECT_TRUE(lit.evaluate({2, 1}));
    EXPECT_TRUE(lit.evaluate({2, 2}));
    EXPECT_FALSE(lit.evaluate({0, 0}));
    EXPECT_FALSE(lit.evaluate({1, 2}));
}

TEST_F(MVDDTest, MVBDDSingletonInvalidValue) {
    MVBDD base(3);
    base.new_var();
    EXPECT_THROW(MVBDD::singleton(base, 1, -1), std::invalid_argument);
    EXPECT_THROW(MVBDD::singleton(base, 1, 3), std::invalid_argument);
}

// ============================================================
//  MVBDD ITE
// ============================================================

TEST_F(MVDDTest, MVBDDIteK3) {
    MVBDD base(3);
    bddvar v1 = base.new_var();
    auto table = base.var_table();

    MVBDD c0 = MVBDD::zero(table);
    MVBDD c1 = MVBDD::one(table);
    MVBDD c2 = MVBDD::zero(table);

    // f(v1) = (v1==1) ? true : false
    MVBDD f = MVBDD::ite(base, v1, {c0, c1, c2});

    EXPECT_FALSE(f.evaluate({0}));
    EXPECT_TRUE(f.evaluate({1}));
    EXPECT_FALSE(f.evaluate({2}));
}

TEST_F(MVDDTest, MVBDDIteAllOne) {
    MVBDD base(3);
    bddvar v1 = base.new_var();
    auto table = base.var_table();

    MVBDD o = MVBDD::one(table);
    MVBDD f = MVBDD::ite(base, v1, {o, o, o});

    // All children are one → should reduce to one
    EXPECT_TRUE(f.is_one());
}

TEST_F(MVDDTest, MVBDDIteTwoVars) {
    MVBDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    auto table = base.var_table();

    // Inner: v2 decides
    MVBDD z = MVBDD::zero(table);
    MVBDD o = MVBDD::one(table);
    MVBDD inner_v2 = MVBDD::ite(base, v2, {z, o, z});  // v2==1

    // Outer: v1==0 → false, v1==1 → (v2==1), v1==2 → true
    MVBDD f = MVBDD::ite(base, v1, {z, inner_v2, o});

    EXPECT_FALSE(f.evaluate({0, 0}));
    EXPECT_FALSE(f.evaluate({0, 1}));
    EXPECT_FALSE(f.evaluate({1, 0}));
    EXPECT_TRUE(f.evaluate({1, 1}));
    EXPECT_FALSE(f.evaluate({1, 2}));
    EXPECT_TRUE(f.evaluate({2, 0}));
    EXPECT_TRUE(f.evaluate({2, 1}));
    EXPECT_TRUE(f.evaluate({2, 2}));
}

// ============================================================
//  MVBDD Boolean operations
// ============================================================

TEST_F(MVDDTest, MVBDDBoolOps) {
    MVBDD base(3);
    bddvar v1 = base.new_var();

    MVBDD a = MVBDD::singleton(base, v1, 0);  // v1 == 0
    MVBDD b = MVBDD::singleton(base, v1, 1);  // v1 == 1

    MVBDD a_and_b = a & b;
    EXPECT_TRUE(a_and_b.is_zero());  // can't be both 0 and 1

    MVBDD a_or_b = a | b;
    EXPECT_TRUE(a_or_b.evaluate({0}));
    EXPECT_TRUE(a_or_b.evaluate({1}));
    EXPECT_FALSE(a_or_b.evaluate({2}));

    MVBDD not_a = ~a;
    EXPECT_FALSE(not_a.evaluate({0}));
    EXPECT_TRUE(not_a.evaluate({1}));
    EXPECT_TRUE(not_a.evaluate({2}));

    MVBDD a_xor_b = a ^ b;
    EXPECT_TRUE(a_xor_b.evaluate({0}));
    EXPECT_TRUE(a_xor_b.evaluate({1}));
    EXPECT_FALSE(a_xor_b.evaluate({2}));
}

TEST_F(MVDDTest, MVBDDCompoundAssign) {
    MVBDD base(3);
    bddvar v1 = base.new_var();

    MVBDD a = MVBDD::singleton(base, v1, 0);
    MVBDD b = MVBDD::singleton(base, v1, 1);

    MVBDD c = a;
    c |= b;
    EXPECT_TRUE(c.evaluate({0}));
    EXPECT_TRUE(c.evaluate({1}));
    EXPECT_FALSE(c.evaluate({2}));
}

// ============================================================
//  MVBDD child / top_var
// ============================================================

TEST_F(MVDDTest, MVBDDChildAndTopVar) {
    MVBDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    auto table = base.var_table();

    MVBDD z = MVBDD::zero(table);
    MVBDD o = MVBDD::one(table);

    // f: v1==0 → false, v1==1 → true, v1==2 → true
    MVBDD f = MVBDD::ite(base, v1, {z, o, o});

    // v1 should be the top (it's at a lower level than v2 in BDD,
    // but at a higher MVDD variable? No - v2's DD vars have higher levels)
    // Actually: v2 has higher DD var numbers → higher levels → closer to root
    // So for a function that only depends on v1, v1 should be the top MVDD var
    EXPECT_EQ(f.top_var(), v1);

    MVBDD c0 = f.child(0);
    MVBDD c1 = f.child(1);
    MVBDD c2 = f.child(2);

    EXPECT_TRUE(c0.is_zero());
    EXPECT_TRUE(c1.is_one());
    EXPECT_TRUE(c2.is_one());
}

TEST_F(MVDDTest, MVBDDChildValue2ReturnsConstant) {
    // Regression: child(2) for k=3 singleton must return constant one,
    // not a function that depends on lower DD variables.
    MVBDD base(3);
    bddvar v1 = base.new_var();

    MVBDD s2 = MVBDD::singleton(base, v1, 2);
    MVBDD c2 = s2.child(2);
    EXPECT_TRUE(c2.is_one());

    // Also verify child(0) and child(1) return false
    MVBDD c0 = s2.child(0);
    MVBDD c1 = s2.child(1);
    EXPECT_TRUE(c0.is_zero());
    EXPECT_TRUE(c1.is_zero());
}

TEST_F(MVDDTest, MVBDDChildK4AllValues) {
    // Test child() for all values with k=4
    MVBDD base(4);
    bddvar v1 = base.new_var();

    for (int val = 0; val < 4; ++val) {
        MVBDD s = MVBDD::singleton(base, v1, val);
        for (int test = 0; test < 4; ++test) {
            MVBDD c = s.child(test);
            if (test == val) {
                EXPECT_TRUE(c.is_one())
                    << "singleton(v1," << val << ").child(" << test << ") should be one";
            } else {
                EXPECT_TRUE(c.is_zero())
                    << "singleton(v1," << val << ").child(" << test << ") should be zero";
            }
        }
    }
}

TEST_F(MVDDTest, MVBDDChildTerminalThrows) {
    MVBDD base(3, true);
    EXPECT_THROW(base.child(0), std::invalid_argument);
}

// ============================================================
//  MVBDD/MVZDD unregistered DD variable validation
// ============================================================

TEST_F(MVDDTest, MVBDDConstructorRejectsUnregisteredDDVar) {
    auto table = std::make_shared<MVDDVarTable>(3);
    bddvar d1 = bddnewvar();
    bddvar d2 = bddnewvar();
    table->register_var({d1, d2});

    // BDD using an unregistered DD variable
    bddvar d3 = bddnewvar();
    BDD unregistered = BDD::prime(d3);
    EXPECT_THROW(MVBDD(table, unregistered), std::invalid_argument);

    // BDD using only registered DD variables should work
    BDD registered = BDD::prime(d1);
    EXPECT_NO_THROW(MVBDD(table, registered));
}

TEST_F(MVDDTest, MVBDDFromBddRejectsUnregisteredDDVar) {
    MVBDD base(3);
    bddvar v1 = base.new_var();
    bddvar d_unreg = bddnewvar();
    BDD unreg_bdd = BDD::prime(d_unreg);
    EXPECT_THROW(MVBDD::from_bdd(base, unreg_bdd), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDConstructorRejectsUnregisteredDDVar) {
    auto table = std::make_shared<MVDDVarTable>(3);
    bddvar d1 = bddnewvar();
    bddvar d2 = bddnewvar();
    table->register_var({d1, d2});

    bddvar d3 = bddnewvar();
    ZDD unregistered = ZDD::singleton(d3);
    EXPECT_THROW(MVZDD(table, unregistered), std::invalid_argument);

    ZDD registered = ZDD::singleton(d1);
    EXPECT_NO_THROW(MVZDD(table, registered));
}

TEST_F(MVDDTest, MVZDDFromZddRejectsUnregisteredDDVar) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar d_unreg = bddnewvar();
    ZDD unreg_zdd = ZDD::singleton(d_unreg);
    EXPECT_THROW(MVZDD::from_zdd(base, unreg_zdd), std::invalid_argument);
}

// ============================================================
//  MVBDD conversion
// ============================================================

TEST_F(MVDDTest, MVBDDToBddAndBack) {
    MVBDD base(3);
    bddvar v1 = base.new_var();

    MVBDD lit = MVBDD::singleton(base, v1, 1);

    BDD bdd = lit.to_bdd();
    MVBDD restored = MVBDD::from_bdd(base, bdd);

    EXPECT_EQ(lit, restored);
}

// ============================================================
//  MVBDD node count
// ============================================================

TEST_F(MVDDTest, MVBDDNodeCount) {
    MVBDD base(3);
    bddvar v1 = base.new_var();
    auto table = base.var_table();

    MVBDD z = MVBDD::zero(table);
    MVBDD o = MVBDD::one(table);

    MVBDD f = MVBDD::ite(base, v1, {z, o, z});
    EXPECT_EQ(f.mvbdd_node_count(), 1u);  // one MVDD node for v1

    EXPECT_TRUE(z.mvbdd_node_count() == 0u);  // terminal
}

// ============================================================
//  MVBDD comparison
// ============================================================

TEST_F(MVDDTest, MVBDDComparison) {
    MVBDD base(3);
    bddvar v1 = base.new_var();

    MVBDD a = MVBDD::singleton(base, v1, 1);
    MVBDD b = MVBDD::singleton(base, v1, 1);
    MVBDD c = MVBDD::singleton(base, v1, 2);

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST_F(MVDDTest, MVBDDComparisonDifferentTables) {
    MVBDD a(3, true);
    MVBDD b(3, true);

    // Different tables → not equal even if same value
    EXPECT_NE(a, b);
}

// ============================================================
//  MVBDD evaluate errors
// ============================================================

TEST_F(MVDDTest, MVBDDEvaluateWrongSize) {
    MVBDD base(3);
    base.new_var();
    base.new_var();

    MVBDD f = MVBDD::one(base.var_table());
    EXPECT_THROW(f.evaluate({0}), std::invalid_argument);
    EXPECT_THROW(f.evaluate({0, 0, 0}), std::invalid_argument);
}

TEST_F(MVDDTest, MVBDDEvaluateValueOutOfRange) {
    MVBDD base(3);
    base.new_var();

    MVBDD f = MVBDD::one(base.var_table());
    EXPECT_THROW(f.evaluate({3}), std::invalid_argument);
    EXPECT_THROW(f.evaluate({-1}), std::invalid_argument);
}

// ============================================================
//  MVBDD incompatible operations
// ============================================================

TEST_F(MVDDTest, MVBDDIncompatibleOps) {
    MVBDD a(3, true);
    MVBDD b(3, true);

    EXPECT_THROW(a & b, std::invalid_argument);
    EXPECT_THROW(a | b, std::invalid_argument);
    EXPECT_THROW(a ^ b, std::invalid_argument);
}

// ============================================================
//  MVZDD basic tests
// ============================================================

TEST_F(MVDDTest, MVZDDDefaultConstructor) {
    MVZDD m;
    EXPECT_EQ(m.id(), bddnull);
    EXPECT_EQ(m.k(), 0);
}

TEST_F(MVDDTest, MVZDDKConstructor) {
    MVZDD m0(3, false);
    EXPECT_TRUE(m0.is_zero());

    MVZDD m1(3, true);
    EXPECT_TRUE(m1.is_one());
}

TEST_F(MVDDTest, MVZDDFactoryMethods) {
    MVZDD base(3);
    auto table = base.var_table();

    MVZDD z = MVZDD::zero(table);
    MVZDD o = MVZDD::one(table);

    EXPECT_TRUE(z.is_zero());
    EXPECT_TRUE(o.is_one());
}

TEST_F(MVDDTest, MVZDDNewVar) {
    MVZDD m(3);
    bddvar v1 = m.new_var();
    bddvar v2 = m.new_var();

    EXPECT_EQ(v1, 1u);
    EXPECT_EQ(v2, 2u);
}

// ============================================================
//  MVZDD singleton and evaluation
// ============================================================

TEST_F(MVDDTest, MVZDDSingletonK3) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s0 = MVZDD::singleton(base, v1, 0);
    MVZDD s1 = MVZDD::singleton(base, v1, 1);
    MVZDD s2 = MVZDD::singleton(base, v1, 2);

    // s0 = family containing {(0)}, i.e., the all-zero assignment
    EXPECT_TRUE(s0.evaluate({0}));
    EXPECT_FALSE(s0.evaluate({1}));
    EXPECT_FALSE(s0.evaluate({2}));

    EXPECT_FALSE(s1.evaluate({0}));
    EXPECT_TRUE(s1.evaluate({1}));
    EXPECT_FALSE(s1.evaluate({2}));

    EXPECT_FALSE(s2.evaluate({0}));
    EXPECT_FALSE(s2.evaluate({1}));
    EXPECT_TRUE(s2.evaluate({2}));
}

TEST_F(MVDDTest, MVZDDSingletonTwoVars) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    // Singleton: v1=2, v2=0 → family containing one assignment {(2,0)}
    MVZDD s = MVZDD::singleton(base, v1, 2);

    EXPECT_TRUE(s.evaluate({2, 0}));
    EXPECT_FALSE(s.evaluate({2, 1}));  // v2 must be 0 in singleton
    EXPECT_FALSE(s.evaluate({0, 0}));
    EXPECT_FALSE(s.evaluate({1, 0}));
}

TEST_F(MVDDTest, MVZDDSingletonInvalidMvValue0) {
    // Regression: singleton(base, invalid_mv, 0) must throw, not return bddsingle
    MVZDD base(3);
    base.new_var();
    EXPECT_THROW(MVZDD::singleton(base, 999, 0), std::out_of_range);
    EXPECT_THROW(MVZDD::singleton(base, 0, 0), std::out_of_range);
}

// ============================================================
//  MVZDD set operations
// ============================================================

TEST_F(MVDDTest, MVZDDUnion) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s0 = MVZDD::singleton(base, v1, 0);
    MVZDD s1 = MVZDD::singleton(base, v1, 1);
    MVZDD s2 = MVZDD::singleton(base, v1, 2);

    MVZDD all = s0 + s1 + s2;

    EXPECT_TRUE(all.evaluate({0}));
    EXPECT_TRUE(all.evaluate({1}));
    EXPECT_TRUE(all.evaluate({2}));

    EXPECT_DOUBLE_EQ(all.count(), 3.0);
}

TEST_F(MVDDTest, MVZDDIntersect) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s0 = MVZDD::singleton(base, v1, 0);
    MVZDD s1 = MVZDD::singleton(base, v1, 1);

    MVZDD empty = s0 & s1;
    EXPECT_TRUE(empty.is_zero());
}

TEST_F(MVDDTest, MVZDDDifference) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s0 = MVZDD::singleton(base, v1, 0);
    MVZDD s1 = MVZDD::singleton(base, v1, 1);

    MVZDD all = s0 + s1;
    MVZDD diff = all - s0;

    EXPECT_FALSE(diff.evaluate({0}));
    EXPECT_TRUE(diff.evaluate({1}));
}

TEST_F(MVDDTest, MVZDDCompoundAssign) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD a = MVZDD::singleton(base, v1, 0);
    MVZDD b = MVZDD::singleton(base, v1, 1);

    a += b;
    EXPECT_TRUE(a.evaluate({0}));
    EXPECT_TRUE(a.evaluate({1}));
}

// ============================================================
//  MVZDD ITE
// ============================================================

TEST_F(MVDDTest, MVZDDIteK3) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    auto table = base.var_table();

    MVZDD e = MVZDD::zero(table);   // empty family
    MVZDD u = MVZDD::one(table);    // family {(all-zero)}

    // f: v1==0 → empty, v1==1 → {(0-assignment)}, v1==2 → empty
    MVZDD f = MVZDD::ite(base, v1, {e, u, e});

    EXPECT_FALSE(f.evaluate({0}));
    EXPECT_TRUE(f.evaluate({1}));
    EXPECT_FALSE(f.evaluate({2}));
}

TEST_F(MVDDTest, MVZDDIteTwoVars) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    auto table = base.var_table();

    MVZDD e = MVZDD::zero(table);
    MVZDD u = MVZDD::one(table);

    // Inner: v2 decides
    MVZDD inner = MVZDD::ite(base, v2, {u, e, u});  // v2==0 or v2==2

    // Outer: v1==1 → inner
    MVZDD f = MVZDD::ite(base, v1, {e, inner, e});

    EXPECT_FALSE(f.evaluate({0, 0}));
    EXPECT_TRUE(f.evaluate({1, 0}));
    EXPECT_FALSE(f.evaluate({1, 1}));
    EXPECT_TRUE(f.evaluate({1, 2}));
    EXPECT_FALSE(f.evaluate({2, 0}));
}

TEST_F(MVDDTest, MVZDDIteChildDependsOnSameVar) {
    // Regression: ite must strip dd_vars of mv from children.
    // children[1] = singleton(base, v1, 2) depends on v1 itself.
    // The result should still be consistent.
    MVZDD base(3);
    bddvar v1 = base.new_var();
    auto table = base.var_table();

    MVZDD e = MVZDD::zero(table);
    MVZDD u = MVZDD::one(table);
    MVZDD s2 = MVZDD::singleton(base, v1, 2);

    // ite(v1, {empty, s2, empty}):
    // children[1] = s2 = family { (2) } — but s2 encodes v1=2.
    // After stripping v1's dd_vars from s2, it becomes { (0) } = unit family.
    // Then value 1 encoding adds dd_vars[0], producing family { (1) }.
    MVZDD f = MVZDD::ite(base, v1, {e, s2, e});

    EXPECT_FALSE(f.evaluate({0}));
    EXPECT_TRUE(f.evaluate({1}));
    EXPECT_FALSE(f.evaluate({2}));

    // Verify count matches evaluate results
    EXPECT_DOUBLE_EQ(f.count(), 1.0);

    // Verify enumerate is consistent with evaluate
    auto sets = f.enumerate();
    EXPECT_EQ(sets.size(), 1u);
    if (!sets.empty()) {
        EXPECT_EQ(sets[0].size(), 1u);
        EXPECT_EQ(sets[0][0], 1);
    }
}

// ============================================================
//  MVZDD child / top_var
// ============================================================

TEST_F(MVDDTest, MVZDDChildAndTopVar) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    auto table = base.var_table();

    MVZDD e = MVZDD::zero(table);
    MVZDD u = MVZDD::one(table);

    MVZDD f = MVZDD::ite(base, v1, {e, u, e});
    EXPECT_EQ(f.top_var(), v1);

    MVZDD c0 = f.child(0);
    MVZDD c1 = f.child(1);
    MVZDD c2 = f.child(2);

    EXPECT_TRUE(c0.is_zero());
    EXPECT_TRUE(c1.is_one());
    EXPECT_TRUE(c2.is_zero());
}

TEST_F(MVDDTest, MVZDDChildTerminalThrows) {
    MVZDD base(3, true);
    EXPECT_THROW(base.child(0), std::invalid_argument);
}

// ============================================================
//  MVZDD counting
// ============================================================

TEST_F(MVDDTest, MVZDDCount) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s0 = MVZDD::singleton(base, v1, 0);
    MVZDD s1 = MVZDD::singleton(base, v1, 1);
    MVZDD s2 = MVZDD::singleton(base, v1, 2);

    MVZDD all = s0 + s1 + s2;
    EXPECT_DOUBLE_EQ(all.count(), 3.0);
    EXPECT_EQ(all.exact_count(), bigint::BigInt(3));

    MVZDD empty = MVZDD::zero(base.var_table());
    EXPECT_DOUBLE_EQ(empty.count(), 0.0);
}

// ============================================================
//  MVZDD enumerate
// ============================================================

TEST_F(MVDDTest, MVZDDEnumerate) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s0 = MVZDD::singleton(base, v1, 0);
    MVZDD s1 = MVZDD::singleton(base, v1, 1);
    MVZDD s2 = MVZDD::singleton(base, v1, 2);

    MVZDD all = s0 + s1 + s2;
    auto sets = all.enumerate();

    EXPECT_EQ(sets.size(), 3u);

    // Check that we have all three values
    bool found[3] = {false, false, false};
    for (const auto& s : sets) {
        ASSERT_EQ(s.size(), 1u);
        ASSERT_GE(s[0], 0);
        ASSERT_LT(s[0], 3);
        found[s[0]] = true;
    }
    EXPECT_TRUE(found[0]);
    EXPECT_TRUE(found[1]);
    EXPECT_TRUE(found[2]);
}

TEST_F(MVDDTest, MVZDDEnumerateTwoVars) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    auto table = base.var_table();

    MVZDD e = MVZDD::zero(table);
    MVZDD u = MVZDD::one(table);

    // v1==1, v2==0  or  v1==2, v2==1
    MVZDD inner1 = MVZDD::ite(base, v2, {u, e, e});
    MVZDD inner2 = MVZDD::ite(base, v2, {e, u, e});
    MVZDD f = MVZDD::ite(base, v1, {e, inner1, inner2});

    auto sets = f.enumerate();
    EXPECT_EQ(sets.size(), 2u);

    // Should contain {1, 0} and {2, 1}
    bool found_10 = false, found_21 = false;
    for (const auto& s : sets) {
        ASSERT_EQ(s.size(), 2u);
        if (s[0] == 1 && s[1] == 0) found_10 = true;
        if (s[0] == 2 && s[1] == 1) found_21 = true;
    }
    EXPECT_TRUE(found_10);
    EXPECT_TRUE(found_21);
}

// ============================================================
//  MVZDD print_sets / to_str
// ============================================================

TEST_F(MVDDTest, MVZDDToStr) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s = MVZDD::singleton(base, v1, 1);
    std::string str = s.to_str();
    EXPECT_EQ(str, "{1}\n");
}

TEST_F(MVDDTest, MVZDDPrintSetsWithNames) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s = MVZDD::singleton(base, v1, 2);
    std::ostringstream oss;
    s.print_sets(oss, {"color"});
    EXPECT_EQ(oss.str(), "{color=2}\n");
}

// ============================================================
//  MVZDD conversion
// ============================================================

TEST_F(MVDDTest, MVZDDToZddAndBack) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD s = MVZDD::singleton(base, v1, 1);
    ZDD zdd = s.to_zdd();
    MVZDD restored = MVZDD::from_zdd(base, zdd);

    EXPECT_EQ(s, restored);
}

// ============================================================
//  MVZDD node count
// ============================================================

TEST_F(MVDDTest, MVZDDNodeCount) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    auto table = base.var_table();

    MVZDD e = MVZDD::zero(table);
    MVZDD u = MVZDD::one(table);

    MVZDD f = MVZDD::ite(base, v1, {e, u, e});
    EXPECT_EQ(f.mvzdd_node_count(), 1u);

    EXPECT_EQ(e.mvzdd_node_count(), 0u);
}

// ============================================================
//  MVZDD comparison
// ============================================================

TEST_F(MVDDTest, MVZDDComparison) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    MVZDD a = MVZDD::singleton(base, v1, 1);
    MVZDD b = MVZDD::singleton(base, v1, 1);
    MVZDD c = MVZDD::singleton(base, v1, 2);

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST_F(MVDDTest, MVZDDIncompatibleOps) {
    MVZDD a(3, true);
    MVZDD b(3, true);

    EXPECT_THROW(a + b, std::invalid_argument);
    EXPECT_THROW(a - b, std::invalid_argument);
    EXPECT_THROW(a & b, std::invalid_argument);
}

// ============================================================
//  MVBDD/MVZDD shared variable table
// ============================================================

TEST_F(MVDDTest, SharedVarTable) {
    auto table = std::make_shared<MVDDVarTable>(3);

    MVBDD bdd_base(table, BDD(0));
    MVZDD zdd_base(table, ZDD(0));

    // Creating a var via MVBDD should be visible from MVZDD
    bddvar v1 = bdd_base.new_var();
    EXPECT_EQ(table->mvdd_var_count(), 1u);
    EXPECT_EQ(zdd_base.var_table()->mvdd_var_count(), 1u);
}

// ============================================================
//  MVBDD/MVZDD with k=4
// ============================================================

TEST_F(MVDDTest, MVBDDK4) {
    MVBDD base(4);
    bddvar v1 = base.new_var();

    for (int val = 0; val < 4; ++val) {
        MVBDD lit = MVBDD::singleton(base, v1, val);
        for (int test = 0; test < 4; ++test) {
            if (test == val) {
                EXPECT_TRUE(lit.evaluate({test}));
            } else {
                EXPECT_FALSE(lit.evaluate({test}));
            }
        }
    }
}

TEST_F(MVDDTest, MVZDDK4) {
    MVZDD base(4);
    bddvar v1 = base.new_var();

    for (int val = 0; val < 4; ++val) {
        MVZDD s = MVZDD::singleton(base, v1, val);
        for (int test = 0; test < 4; ++test) {
            if (test == val) {
                EXPECT_TRUE(s.evaluate({test}));
            } else {
                EXPECT_FALSE(s.evaluate({test}));
            }
        }
    }
}

// ============================================================
//  MVBDD/MVZDD SVG export tests
// ============================================================

static bool svg_contains(const std::string& svg, const std::string& sub) {
    return svg.find(sub) != std::string::npos;
}

TEST_F(MVDDTest, MVBDDSaveSvgExpanded) {
    MVBDD base(3);
    bddvar mv1 = base.new_var();
    auto lit = MVBDD::singleton(base, mv1, 1);
    std::string svg = lit.save_svg();
    EXPECT_TRUE(svg_contains(svg, "<svg xmlns="));
    EXPECT_TRUE(svg_contains(svg, "<circle"));
    EXPECT_TRUE(svg_contains(svg, "</svg>"));
}

TEST_F(MVDDTest, MVBDDSaveSvgRaw) {
    MVBDD base(3);
    bddvar mv1 = base.new_var();
    auto lit = MVBDD::singleton(base, mv1, 1);
    SvgParams params;
    params.mode = DrawMode::Raw;
    std::string svg = lit.save_svg(params);
    EXPECT_TRUE(svg_contains(svg, "<svg xmlns="));
    EXPECT_TRUE(svg_contains(svg, "<circle"));
}

TEST_F(MVDDTest, MVBDDSaveSvgVarNames) {
    MVBDD base(3);
    bddvar mv1 = base.new_var();
    auto lit = MVBDD::singleton(base, mv1, 1);
    SvgParams params;
    params.var_name_map[mv1] = "color";
    std::string svg = lit.save_svg(params);
    EXPECT_TRUE(svg_contains(svg, "color"));
}

TEST_F(MVDDTest, MVBDDSaveSvgEdgeLabels) {
    MVBDD base(3);
    bddvar mv1 = base.new_var();
    auto lit = MVBDD::singleton(base, mv1, 2);
    SvgParams params;
    params.draw_edge_labels = true;
    std::string svg = lit.save_svg(params);
    // Should contain edge label "2" for value 2
    EXPECT_TRUE(svg_contains(svg, ">2<"));
}

TEST_F(MVDDTest, MVBDDSaveSvgStream) {
    MVBDD base(3);
    bddvar mv1 = base.new_var();
    auto lit = MVBDD::singleton(base, mv1, 1);
    std::ostringstream oss;
    lit.save_svg(oss);
    EXPECT_EQ(oss.str(), lit.save_svg());
}

TEST_F(MVDDTest, MVBDDSaveSvgTerminal) {
    MVBDD t(3, true);
    std::string svg = t.save_svg();
    EXPECT_TRUE(svg_contains(svg, "<svg xmlns="));
    EXPECT_TRUE(svg_contains(svg, "<rect"));
    EXPECT_TRUE(svg_contains(svg, ">1<"));
}

TEST_F(MVDDTest, MVBDDSaveSvgTwoVars) {
    MVBDD base(3);
    bddvar mv1 = base.new_var();
    bddvar mv2 = base.new_var();
    auto lit1 = MVBDD::singleton(base, mv1, 1);
    auto lit2 = MVBDD::singleton(base, mv2, 2);
    auto f = lit1 | lit2;
    std::string svg = f.save_svg();
    EXPECT_TRUE(svg_contains(svg, "<circle"));
    // Should have at least 2 circles (2 MV nodes)
    size_t first = svg.find("<circle");
    ASSERT_NE(first, std::string::npos);
    size_t second = svg.find("<circle", first + 1);
    EXPECT_NE(second, std::string::npos);
}

TEST_F(MVDDTest, MVZDDSaveSvgExpanded) {
    MVZDD base(3);
    bddvar mv1 = base.new_var();
    auto singleton = MVZDD::singleton(base, mv1, 1);
    std::string svg = singleton.save_svg();
    EXPECT_TRUE(svg_contains(svg, "<svg xmlns="));
    EXPECT_TRUE(svg_contains(svg, "<circle"));
    EXPECT_TRUE(svg_contains(svg, "</svg>"));
}

TEST_F(MVDDTest, MVZDDSaveSvgDashedZeroEdge) {
    MVZDD base(3);
    bddvar mv1 = base.new_var();
    auto singleton = MVZDD::singleton(base, mv1, 1);
    std::string svg = singleton.save_svg();
    // 0-edge should be dashed
    EXPECT_TRUE(svg_contains(svg, "stroke-dasharray"));
}

TEST_F(MVDDTest, MVZDDSaveSvgRaw) {
    MVZDD base(3);
    bddvar mv1 = base.new_var();
    auto singleton = MVZDD::singleton(base, mv1, 1);
    SvgParams params;
    params.mode = DrawMode::Raw;
    std::string svg = singleton.save_svg(params);
    EXPECT_TRUE(svg_contains(svg, "<svg xmlns="));
    EXPECT_TRUE(svg_contains(svg, "<circle"));
}

TEST_F(MVDDTest, MVZDDSaveSvgStream) {
    MVZDD base(3);
    bddvar mv1 = base.new_var();
    auto singleton = MVZDD::singleton(base, mv1, 1);
    std::ostringstream oss;
    singleton.save_svg(oss);
    EXPECT_EQ(oss.str(), singleton.save_svg());
}

TEST_F(MVDDTest, MVZDDSaveSvgVarNames) {
    MVZDD base(3);
    bddvar mv1 = base.new_var();
    auto singleton = MVZDD::singleton(base, mv1, 1);
    SvgParams params;
    params.var_name_map[mv1] = "val";
    std::string svg = singleton.save_svg(params);
    EXPECT_TRUE(svg_contains(svg, "val"));
}

TEST_F(MVDDTest, MVBDDSaveSvgAllSolid) {
    // MVBDD: all edges should be solid (no dashed for value 0)
    MVBDD base(3);
    bddvar mv1 = base.new_var();
    auto lit = MVBDD::singleton(base, mv1, 0);
    std::string svg = lit.save_svg();
    // MVBDD should NOT have dashed edges
    EXPECT_FALSE(svg_contains(svg, "stroke-dasharray"));
}

// ============================================================
//  MVZDD operator^  (symmetric difference)
// ============================================================

TEST_F(MVDDTest, MVZDDSymDiffBasic) {
    // k=3, 2 variables
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    // A = { (0,0), (1,0) }
    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    MVZDD A = a00 + a10;

    // B = { (0,0), (0,2) }
    auto b02 = MVZDD::singleton(base, v2, 2);
    MVZDD B = a00 + b02;

    // A ^ B = { (1,0), (0,2) }
    MVZDD result = A ^ B;
    EXPECT_EQ(result.count(), 2.0);
    EXPECT_TRUE(result.evaluate({1, 0}));
    EXPECT_TRUE(result.evaluate({0, 2}));
    EXPECT_FALSE(result.evaluate({0, 0}));  // in both, removed
}

TEST_F(MVDDTest, MVZDDSymDiffSelf) {
    MVZDD base(2);
    base.new_var();
    auto s = MVZDD::singleton(base, 1, 1);
    MVZDD result = s ^ s;
    EXPECT_TRUE(result.is_zero());
}

TEST_F(MVDDTest, MVZDDSymDiffEmpty) {
    MVZDD base(2);
    base.new_var();
    auto s = MVZDD::singleton(base, 1, 1);
    auto empty = MVZDD::zero(base.var_table());
    EXPECT_EQ(s ^ empty, s);
    EXPECT_EQ(empty ^ s, s);
}

TEST_F(MVDDTest, MVZDDSymDiffInPlace) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    auto a = MVZDD::singleton(base, v1, 1);
    auto b = MVZDD::singleton(base, v1, 2);
    MVZDD c = a + b;

    c ^= a;
    // c was {(1),(2)}, a was {(1)} => c should be {(2)}
    EXPECT_EQ(c.count(), 1.0);
    EXPECT_TRUE(c.evaluate({2}));
    EXPECT_FALSE(c.evaluate({1}));
}

// ============================================================
//  MVZDD has_empty
// ============================================================

TEST_F(MVDDTest, MVZDDHasEmptyTrue) {
    // one() = { all-zero assignment } = "empty" in ZDD terms
    MVZDD base(3);
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    EXPECT_TRUE(one.has_empty());
}

TEST_F(MVDDTest, MVZDDHasEmptyFalse) {
    MVZDD base(3);
    base.new_var();
    auto s = MVZDD::singleton(base, 1, 2);
    EXPECT_FALSE(s.has_empty());
}

TEST_F(MVDDTest, MVZDDHasEmptyUnion) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    auto s = MVZDD::singleton(base, v1, 1);
    auto one = MVZDD::one(base.var_table());
    MVZDD combined = s + one;
    EXPECT_TRUE(combined.has_empty());
}

TEST_F(MVDDTest, MVZDDHasEmptyZero) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());
    EXPECT_FALSE(empty.has_empty());
}

// ============================================================
//  MVZDD contains
// ============================================================

TEST_F(MVDDTest, MVZDDContainsBasic) {
    // k=3, 2 variables
    // Family = { (1,0), (0,2) }
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    auto s10 = MVZDD::singleton(base, v1, 1);
    auto s02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = s10 + s02;

    EXPECT_TRUE(F.contains({1, 0}));
    EXPECT_TRUE(F.contains({0, 2}));
    EXPECT_FALSE(F.contains({0, 0}));
    EXPECT_FALSE(F.contains({1, 2}));
    EXPECT_FALSE(F.contains({2, 0}));
    EXPECT_FALSE(F.contains({0, 1}));
}

TEST_F(MVDDTest, MVZDDContainsAllZero) {
    // Family = { (0,0) }
    MVZDD base(3);
    base.new_var();
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    EXPECT_TRUE(one.contains({0, 0}));
    EXPECT_FALSE(one.contains({1, 0}));
}

TEST_F(MVDDTest, MVZDDContainsEmpty) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());
    EXPECT_FALSE(empty.contains({0}));
    EXPECT_FALSE(empty.contains({1}));
}

TEST_F(MVDDTest, MVZDDContainsSizeMismatch) {
    MVZDD base(2);
    base.new_var();
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    EXPECT_THROW(one.contains({0}), std::invalid_argument);
    EXPECT_THROW(one.contains({0, 0, 0}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDContainsValueOutOfRange) {
    MVZDD base(3);
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    EXPECT_THROW(one.contains({3}), std::invalid_argument);
    EXPECT_THROW(one.contains({-1}), std::invalid_argument);
}

// ============================================================
//  MVZDD support_vars
// ============================================================

TEST_F(MVDDTest, MVZDDSupportVarsBasic) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    bddvar v3 = base.new_var();

    // Family only uses v1 and v3
    auto s1 = MVZDD::singleton(base, v1, 1);
    auto s3 = MVZDD::singleton(base, v3, 2);
    MVZDD F = s1 + s3;

    std::vector<bddvar> vars = F.support_vars();
    EXPECT_EQ(vars.size(), 2u);
    EXPECT_EQ(vars[0], v1);
    EXPECT_EQ(vars[1], v3);
    (void)v2;
}

TEST_F(MVDDTest, MVZDDSupportVarsEmpty) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());
    std::vector<bddvar> vars = empty.support_vars();
    EXPECT_TRUE(vars.empty());
}

TEST_F(MVDDTest, MVZDDSupportVarsOne) {
    // one() = { all-zero }, represented as terminal — no DD variables
    MVZDD base(2);
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    std::vector<bddvar> vars = one.support_vars();
    EXPECT_TRUE(vars.empty());
}

// ============================================================
//  MVZDD exact_count with memo
// ============================================================

TEST_F(MVDDTest, MVZDDExactCountWithMemo) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Build a family with known size
    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, 1, 1);
    auto a02 = MVZDD::singleton(base, 2, 2);
    MVZDD F = a00 + a10 + a02;

    ZddCountMemo memo(F.to_zdd());
    bigint::BigInt count1 = F.exact_count(memo);
    EXPECT_EQ(count1.to_string(), "3");

    // Calling again with same memo should give same result
    bigint::BigInt count2 = F.exact_count(memo);
    EXPECT_EQ(count2.to_string(), "3");
}

TEST_F(MVDDTest, MVZDDExactCountMemoConsistency) {
    // exact_count() and exact_count(memo) should agree
    MVZDD base(4);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto s11 = MVZDD::singleton(base, v1, 1);
    auto s12 = MVZDD::singleton(base, v1, 2);
    auto s13 = MVZDD::singleton(base, v1, 3);
    auto s21 = MVZDD::singleton(base, v2, 1);
    MVZDD F = s11 + s12 + s13 + s21;

    ZddCountMemo memo(F.to_zdd());
    bigint::BigInt with_memo = F.exact_count(memo);
    bigint::BigInt without_memo = F.exact_count();
    EXPECT_EQ(with_memo.to_string(), without_memo.to_string());
}

// ============================================================
//  MVZDD uniform_sample
// ============================================================

TEST_F(MVDDTest, MVZDDUniformSampleSingleElement) {
    // Family with exactly one assignment: { (2, 0) }
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    auto s = MVZDD::singleton(base, v1, 2);

    std::mt19937_64 rng(42);
    ZddCountMemo memo(s.to_zdd());
    std::vector<int> sample = s.uniform_sample(rng, memo);

    EXPECT_EQ(sample.size(), 2u);
    EXPECT_EQ(sample[0], 2);  // v1 = 2
    EXPECT_EQ(sample[1], 0);  // v2 = 0
    (void)v2;
}

TEST_F(MVDDTest, MVZDDUniformSampleAllZero) {
    // Family = { (0,0) }
    MVZDD base(3);
    base.new_var();
    base.new_var();
    auto one = MVZDD::one(base.var_table());

    std::mt19937_64 rng(123);
    ZddCountMemo memo(one.to_zdd());
    std::vector<int> sample = one.uniform_sample(rng, memo);

    EXPECT_EQ(sample.size(), 2u);
    EXPECT_EQ(sample[0], 0);
    EXPECT_EQ(sample[1], 0);
}

TEST_F(MVDDTest, MVZDDUniformSampleCoversAll) {
    // Family with 3 assignments — sample many times, check all are hit
    MVZDD base(3);
    bddvar v1 = base.new_var();

    auto s0 = MVZDD::one(base.var_table());       // (0)
    auto s1 = MVZDD::singleton(base, v1, 1);      // (1)
    auto s2 = MVZDD::singleton(base, v1, 2);      // (2)
    MVZDD F = s0 + s1 + s2;

    std::mt19937_64 rng(0);
    ZddCountMemo memo(F.to_zdd());

    std::set<int> seen;
    for (int i = 0; i < 100; ++i) {
        std::vector<int> sample = F.uniform_sample(rng, memo);
        ASSERT_EQ(sample.size(), 1u);
        int val = sample[0];
        EXPECT_GE(val, 0);
        EXPECT_LE(val, 2);
        EXPECT_TRUE(F.contains(sample));
        seen.insert(val);
    }
    // With 100 samples from 3 elements, all should appear
    EXPECT_EQ(seen.size(), 3u);
}

TEST_F(MVDDTest, MVZDDUniformSampleEmptyThrows) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());

    std::mt19937_64 rng(0);
    ZddCountMemo memo(empty.to_zdd());
    EXPECT_THROW(empty.uniform_sample(rng, memo), std::exception);
}

// ============================================================
//  MVZDD min_weight / max_weight
// ============================================================

TEST_F(MVDDTest, MVZDDMinMaxWeightBasic) {
    // k=3, 2 variables
    // Family = { (0,0), (1,0), (0,2) }
    // weights[0] = {0, 10, 20}  (var1: val0=0, val1=10, val2=20)
    // weights[1] = {0, 5, 15}   (var2: val0=0, val1=5, val2=15)
    // (0,0) weight = 0+0 = 0
    // (1,0) weight = 10+0 = 10
    // (0,2) weight = 0+15 = 15
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    EXPECT_EQ(F.min_weight(weights), 0);
    EXPECT_EQ(F.max_weight(weights), 15);
}

TEST_F(MVDDTest, MVZDDMinMaxWeightWithVal0Cost) {
    // val=0 also has non-zero weight
    // k=2, 2 variables
    // Family = { (0,0), (1,0), (0,1), (1,1) }
    // weights[0] = {3, 7}   (var1: val0=3, val1=7)
    // weights[1] = {2, 8}   (var2: val0=2, val1=8)
    // (0,0) = 3+2 = 5
    // (1,0) = 7+2 = 9
    // (0,1) = 3+8 = 11
    // (1,1) = 7+8 = 15
    MVZDD base(2);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a01 = MVZDD::singleton(base, v2, 1);
    // Build (1,1) via ITE
    auto a11 = MVZDD::ite(base, v1, {
        MVZDD::zero(base.var_table()),  // v1=0: empty
        MVZDD::singleton(base, v2, 1)   // v1=1: {(1,1)}
    });
    MVZDD F = a00 + a10 + a01 + a11;

    std::vector<std::vector<int>> weights = {{3, 7}, {2, 8}};

    EXPECT_EQ(F.min_weight(weights), 5);
    EXPECT_EQ(F.max_weight(weights), 15);
}

TEST_F(MVDDTest, MVZDDMinMaxWeightSingleAssignment) {
    // Family = { (2) }
    // weights[0] = {1, 5, 9}
    MVZDD base(3);
    bddvar v1 = base.new_var();
    auto s = MVZDD::singleton(base, v1, 2);

    std::vector<std::vector<int>> weights = {{1, 5, 9}};

    EXPECT_EQ(s.min_weight(weights), 9);
    EXPECT_EQ(s.max_weight(weights), 9);
}

TEST_F(MVDDTest, MVZDDMinMaxWeightNegativeWeights) {
    // Family = { (0), (1), (2) }
    // weights[0] = {-5, 3, -10}
    MVZDD base(3);
    bddvar v1 = base.new_var();

    auto s0 = MVZDD::one(base.var_table());
    auto s1 = MVZDD::singleton(base, v1, 1);
    auto s2 = MVZDD::singleton(base, v1, 2);
    MVZDD F = s0 + s1 + s2;

    std::vector<std::vector<int>> weights = {{-5, 3, -10}};

    EXPECT_EQ(F.min_weight(weights), -10);
    EXPECT_EQ(F.max_weight(weights), 3);
}

// ============================================================
//  MVZDD min_weight_set / max_weight_set
// ============================================================

TEST_F(MVDDTest, MVZDDMinWeightSetBasic) {
    // k=3, 2 variables
    // Family = { (0,0), (1,0), (0,2) }
    // weights: var1 = {0, 10, 20}, var2 = {0, 5, 15}
    // min = (0,0) with weight 0
    // max = (0,2) with weight 15
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    std::vector<int> min_set = F.min_weight_set(weights);
    EXPECT_EQ(min_set, (std::vector<int>{0, 0}));

    std::vector<int> max_set = F.max_weight_set(weights);
    EXPECT_EQ(max_set, (std::vector<int>{0, 2}));
}

TEST_F(MVDDTest, MVZDDMinWeightSetWithVal0Cost) {
    // k=2, 2 variables
    // Family = { (0,0), (1,0), (0,1), (1,1) }
    // weights: var1 = {3, 7}, var2 = {2, 8}
    // min = (0,0)=5, max = (1,1)=15
    MVZDD base(2);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a11 = MVZDD::ite(base, v1, {
        MVZDD::zero(base.var_table()),
        MVZDD::singleton(base, v2, 1)
    });
    MVZDD F = a00 + a10 + a01 + a11;

    std::vector<std::vector<int>> weights = {{3, 7}, {2, 8}};

    std::vector<int> min_set = F.min_weight_set(weights);
    EXPECT_EQ(min_set, (std::vector<int>{0, 0}));

    std::vector<int> max_set = F.max_weight_set(weights);
    EXPECT_EQ(max_set, (std::vector<int>{1, 1}));
}

TEST_F(MVDDTest, MVZDDMinWeightSetResultInFamily) {
    // Verify that the returned set is actually in the family
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto s12 = MVZDD::ite(base, v1, {
        MVZDD::zero(base.var_table()),
        MVZDD::singleton(base, v2, 2),
        MVZDD::zero(base.var_table())
    });
    auto s20 = MVZDD::singleton(base, v1, 2);
    MVZDD F = s12 + s20;

    std::vector<std::vector<int>> weights = {{0, 1, 100}, {0, 50, 2}};

    std::vector<int> min_set = F.min_weight_set(weights);
    EXPECT_TRUE(F.contains(min_set));

    std::vector<int> max_set = F.max_weight_set(weights);
    EXPECT_TRUE(F.contains(max_set));
}

// ============================================================
//  MVZDD weight validation errors
// ============================================================

TEST_F(MVDDTest, MVZDDWeightSizeMismatch) {
    MVZDD base(3);
    base.new_var();
    base.new_var();
    auto one = MVZDD::one(base.var_table());

    // Wrong outer size
    EXPECT_THROW(one.min_weight({{0, 1, 2}}), std::invalid_argument);
    // Wrong inner size
    EXPECT_THROW(one.min_weight({{0, 1}, {0, 1}}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDWeightSetSizeMismatch) {
    MVZDD base(2);
    base.new_var();
    auto one = MVZDD::one(base.var_table());

    // Wrong outer size (0 instead of 1)
    EXPECT_THROW(one.min_weight_set({}), std::invalid_argument);
    EXPECT_THROW(one.max_weight_set({}), std::invalid_argument);
}

// ============================================================
//  MVZDD cost_bound_le
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundLeBasic) {
    // k=3, 2 variables
    // Family = { (0,0), (1,0), (2,0), (0,1), (0,2) }
    // weights: var1 = {0, 10, 20}, var2 = {0, 5, 15}
    // (0,0)=0, (1,0)=10, (2,0)=20, (0,1)=5, (0,2)=15
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a20 = MVZDD::singleton(base, v1, 2);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a20 + a01 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    // cost <= 10: {(0,0)=0, (1,0)=10, (0,1)=5}
    MVZDD le10 = F.cost_bound_le(weights, 10);
    EXPECT_EQ(le10.count(), 3.0);
    EXPECT_TRUE(le10.contains({0, 0}));
    EXPECT_TRUE(le10.contains({1, 0}));
    EXPECT_TRUE(le10.contains({0, 1}));
    EXPECT_FALSE(le10.contains({2, 0}));
    EXPECT_FALSE(le10.contains({0, 2}));

    // cost <= 0: only (0,0)
    MVZDD le0 = F.cost_bound_le(weights, 0);
    EXPECT_EQ(le0.count(), 1.0);
    EXPECT_TRUE(le0.contains({0, 0}));

    // cost <= 100: all
    MVZDD le100 = F.cost_bound_le(weights, 100);
    EXPECT_EQ(le100, F);
}

TEST_F(MVDDTest, MVZDDCostBoundLeWithVal0Cost) {
    // val=0 has non-zero weight
    // k=2, 1 variable
    // Family = { (0), (1) }
    // weights: var1 = {3, 7}
    // (0)=3, (1)=7
    MVZDD base(2);
    bddvar v1 = base.new_var();

    auto a0 = MVZDD::one(base.var_table());
    auto a1 = MVZDD::singleton(base, v1, 1);
    MVZDD F = a0 + a1;

    std::vector<std::vector<int>> weights = {{3, 7}};

    // cost <= 5: only (0)=3
    MVZDD le5 = F.cost_bound_le(weights, 5);
    EXPECT_EQ(le5.count(), 1.0);
    EXPECT_TRUE(le5.contains({0}));

    // cost <= 7: both
    MVZDD le7 = F.cost_bound_le(weights, 7);
    EXPECT_EQ(le7.count(), 2.0);

    // cost <= 2: empty
    MVZDD le2 = F.cost_bound_le(weights, 2);
    EXPECT_TRUE(le2.is_zero());
}

TEST_F(MVDDTest, MVZDDCostBoundLeEmpty) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());
    std::vector<std::vector<int>> weights = {{0, 1}};
    MVZDD result = empty.cost_bound_le(weights, 100);
    EXPECT_TRUE(result.is_zero());
}

// ============================================================
//  MVZDD cost_bound_ge
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundGeBasic) {
    // Same family as above
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a20 = MVZDD::singleton(base, v1, 2);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a20 + a01 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    // cost >= 15: {(2,0)=20, (0,2)=15}
    MVZDD ge15 = F.cost_bound_ge(weights, 15);
    EXPECT_EQ(ge15.count(), 2.0);
    EXPECT_TRUE(ge15.contains({2, 0}));
    EXPECT_TRUE(ge15.contains({0, 2}));

    // cost >= 0: all
    MVZDD ge0 = F.cost_bound_ge(weights, 0);
    EXPECT_EQ(ge0, F);

    // cost >= 100: empty
    MVZDD ge100 = F.cost_bound_ge(weights, 100);
    EXPECT_TRUE(ge100.is_zero());
}

TEST_F(MVDDTest, MVZDDCostBoundGeWithVal0Cost) {
    // k=2, 1 variable, val=0 has weight 3
    MVZDD base(2);
    bddvar v1 = base.new_var();

    auto a0 = MVZDD::one(base.var_table());
    auto a1 = MVZDD::singleton(base, v1, 1);
    MVZDD F = a0 + a1;

    std::vector<std::vector<int>> weights = {{3, 7}};

    // cost >= 5: only (1)=7
    MVZDD ge5 = F.cost_bound_ge(weights, 5);
    EXPECT_EQ(ge5.count(), 1.0);
    EXPECT_TRUE(ge5.contains({1}));

    // cost >= 3: both
    MVZDD ge3 = F.cost_bound_ge(weights, 3);
    EXPECT_EQ(ge3.count(), 2.0);
}

// ============================================================
//  MVZDD cost_bound_eq
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundEqBasic) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a20 = MVZDD::singleton(base, v1, 2);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a20 + a01 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    // cost == 10: {(1,0)=10}
    MVZDD eq10 = F.cost_bound_eq(weights, 10);
    EXPECT_EQ(eq10.count(), 1.0);
    EXPECT_TRUE(eq10.contains({1, 0}));

    // cost == 0: {(0,0)=0}
    MVZDD eq0 = F.cost_bound_eq(weights, 0);
    EXPECT_EQ(eq0.count(), 1.0);
    EXPECT_TRUE(eq0.contains({0, 0}));

    // cost == 7: empty (no assignment with cost 7)
    MVZDD eq7 = F.cost_bound_eq(weights, 7);
    EXPECT_TRUE(eq7.is_zero());
}

TEST_F(MVDDTest, MVZDDCostBoundEqWithVal0Cost) {
    // k=2, 1 variable, weights = {3, 7}
    MVZDD base(2);
    bddvar v1 = base.new_var();

    auto a0 = MVZDD::one(base.var_table());
    auto a1 = MVZDD::singleton(base, v1, 1);
    MVZDD F = a0 + a1;

    std::vector<std::vector<int>> weights = {{3, 7}};

    MVZDD eq3 = F.cost_bound_eq(weights, 3);
    EXPECT_EQ(eq3.count(), 1.0);
    EXPECT_TRUE(eq3.contains({0}));

    MVZDD eq7 = F.cost_bound_eq(weights, 7);
    EXPECT_EQ(eq7.count(), 1.0);
    EXPECT_TRUE(eq7.contains({1}));
}

// ============================================================
//  MVZDD cost_bound with memo
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundMemoReuse) {
    // Using a memo across multiple le calls should give correct results
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a20 = MVZDD::singleton(base, v1, 2);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a20 + a01 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    CostBoundMemo memo;

    MVZDD le5 = F.cost_bound_le(weights, 5, memo);
    EXPECT_EQ(le5.count(), 2.0);  // (0,0)=0, (0,1)=5

    MVZDD le10 = F.cost_bound_le(weights, 10, memo);
    EXPECT_EQ(le10.count(), 3.0);  // +  (1,0)=10

    MVZDD le20 = F.cost_bound_le(weights, 20, memo);
    EXPECT_EQ(le20.count(), 5.0);  // all

    // eq using the same memo
    MVZDD eq15 = F.cost_bound_eq(weights, 15, memo);
    EXPECT_EQ(eq15.count(), 1.0);
    EXPECT_TRUE(eq15.contains({0, 2}));
}

TEST_F(MVDDTest, MVZDDCostBoundConsistency) {
    // Verify: le(b) + ge(b+1) == F  for any b
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a02;  // costs: 0, 10, 15

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    for (long long b : {-1LL, 0LL, 5LL, 10LL, 15LL, 20LL}) {
        MVZDD le = F.cost_bound_le(weights, b);
        MVZDD ge = F.cost_bound_ge(weights, b + 1);
        EXPECT_EQ(le + ge, F) << "Failed for b=" << b;
    }
}

TEST_F(MVDDTest, MVZDDCostBoundNegativeWeights) {
    // Weights can be negative
    // k=3, 1 variable, weights = {-5, 3, -10}
    MVZDD base(3);
    bddvar v1 = base.new_var();

    auto s0 = MVZDD::one(base.var_table());
    auto s1 = MVZDD::singleton(base, v1, 1);
    auto s2 = MVZDD::singleton(base, v1, 2);
    MVZDD F = s0 + s1 + s2;

    std::vector<std::vector<int>> weights = {{-5, 3, -10}};

    // costs: (0)=-5, (1)=3, (2)=-10
    // le(-5): {(0)=-5, (2)=-10}
    MVZDD le_m5 = F.cost_bound_le(weights, -5);
    EXPECT_EQ(le_m5.count(), 2.0);
    EXPECT_TRUE(le_m5.contains({0}));
    EXPECT_TRUE(le_m5.contains({2}));

    // ge(0): {(1)=3}
    MVZDD ge0 = F.cost_bound_ge(weights, 0);
    EXPECT_EQ(ge0.count(), 1.0);
    EXPECT_TRUE(ge0.contains({1}));

    // eq(-10): {(2)}
    MVZDD eq_m10 = F.cost_bound_eq(weights, -10);
    EXPECT_EQ(eq_m10.count(), 1.0);
    EXPECT_TRUE(eq_m10.contains({2}));
}

// ============================================================
//  MVZDD::from_sets tests
// ============================================================

TEST_F(MVDDTest, MVZDDFromSetsEmpty) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Empty input → empty family
    MVZDD f = MVZDD::from_sets(base, {});
    EXPECT_TRUE(f.is_zero());
    EXPECT_EQ(f.count(), 0.0);
}

TEST_F(MVDDTest, MVZDDFromSetsSingleAllZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Single assignment: all zeros → family {(0,0)}
    MVZDD f = MVZDD::from_sets(base, {{0, 0}});
    EXPECT_EQ(f.count(), 1.0);
    EXPECT_TRUE(f.contains({0, 0}));
    EXPECT_TRUE(f.has_empty());
}

TEST_F(MVDDTest, MVZDDFromSetsSingleNonZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Single assignment: (1, 2)
    MVZDD f = MVZDD::from_sets(base, {{1, 2}});
    EXPECT_EQ(f.count(), 1.0);
    EXPECT_TRUE(f.contains({1, 2}));
    EXPECT_FALSE(f.contains({0, 0}));
}

TEST_F(MVDDTest, MVZDDFromSetsMultiple) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    std::vector<std::vector<int>> sets = {{0, 0}, {1, 2}, {2, 1}};
    MVZDD f = MVZDD::from_sets(base, sets);
    EXPECT_EQ(f.count(), 3.0);
    EXPECT_TRUE(f.contains({0, 0}));
    EXPECT_TRUE(f.contains({1, 2}));
    EXPECT_TRUE(f.contains({2, 1}));
    EXPECT_FALSE(f.contains({0, 1}));
}

TEST_F(MVDDTest, MVZDDFromSetsDuplicates) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Duplicate assignments should be unified
    std::vector<std::vector<int>> sets = {{1, 0}, {1, 0}, {0, 2}};
    MVZDD f = MVZDD::from_sets(base, sets);
    EXPECT_EQ(f.count(), 2.0);
    EXPECT_TRUE(f.contains({1, 0}));
    EXPECT_TRUE(f.contains({0, 2}));
}

TEST_F(MVDDTest, MVZDDFromSetsRoundTrip) {
    // from_sets(enumerate()) should produce the same MVZDD
    MVZDD base(4);
    base.new_var();
    base.new_var();
    base.new_var();

    std::vector<std::vector<int>> sets = {
        {0, 0, 0}, {1, 2, 3}, {3, 1, 0}, {0, 3, 2}
    };
    MVZDD f = MVZDD::from_sets(base, sets);

    std::vector<std::vector<int>> enumerated = f.enumerate();
    MVZDD g = MVZDD::from_sets(base, enumerated);
    EXPECT_EQ(f, g);
}

TEST_F(MVDDTest, MVZDDFromSetsInvalidNoVarTable) {
    MVZDD base;
    EXPECT_THROW(MVZDD::from_sets(base, {{0}}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDFromSetsInvalidWrongSize) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Assignment has wrong number of elements
    EXPECT_THROW(MVZDD::from_sets(base, {{1}}), std::invalid_argument);
    EXPECT_THROW(MVZDD::from_sets(base, {{1, 2, 0}}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDFromSetsInvalidValueOutOfRange) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // k=3, so values must be in [0, 2]
    EXPECT_THROW(MVZDD::from_sets(base, {{3, 0}}), std::invalid_argument);
    EXPECT_THROW(MVZDD::from_sets(base, {{0, -1}}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDFromSetsBinaryK2) {
    // k=2 case (binary variables)
    MVZDD base(2);
    base.new_var();
    base.new_var();
    base.new_var();

    std::vector<std::vector<int>> sets = {
        {0, 0, 0}, {1, 0, 1}, {0, 1, 0}, {1, 1, 1}
    };
    MVZDD f = MVZDD::from_sets(base, sets);
    EXPECT_EQ(f.count(), 4.0);
    for (const auto& s : sets) {
        EXPECT_TRUE(f.contains(s));
    }
}

// ============================================================
//  MVZDD::is_subset_family / is_disjoint tests
// ============================================================

TEST_F(MVDDTest, MVZDDIsSubsetFamily) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    MVZDD h = MVZDD::from_sets(base, {{0, 0}, {2, 1}});

    EXPECT_TRUE(f.is_subset_family(g));
    EXPECT_FALSE(g.is_subset_family(f));
    EXPECT_TRUE(f.is_subset_family(f));

    // Empty family is subset of anything
    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_TRUE(empty.is_subset_family(f));
    EXPECT_TRUE(empty.is_subset_family(empty));
}

TEST_F(MVDDTest, MVZDDIsDisjoint) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{2, 1}, {0, 1}});
    MVZDD h = MVZDD::from_sets(base, {{1, 2}, {2, 0}});

    EXPECT_TRUE(f.is_disjoint(g));
    EXPECT_FALSE(f.is_disjoint(h));  // {1,2} in common

    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_TRUE(f.is_disjoint(empty));
    EXPECT_TRUE(empty.is_disjoint(empty));
}

// ============================================================
//  MVZDD::count_intersec / jaccard_index / hamming_distance
// ============================================================

TEST_F(MVDDTest, MVZDDCountIntersec) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    MVZDD g = MVZDD::from_sets(base, {{1, 2}, {2, 1}, {0, 1}});

    bigint::BigInt ci = f.count_intersec(g);
    EXPECT_EQ(ci.to_string(), "2");  // {1,2} and {2,1}

    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_EQ(f.count_intersec(empty).to_string(), "0");
    EXPECT_EQ(f.count_intersec(f).to_string(), "3");
}

TEST_F(MVDDTest, MVZDDJaccardIndex) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{1, 2}, {2, 1}});

    // |F ∩ G| = 1 ({1,2}), |F ∪ G| = 3
    EXPECT_NEAR(f.jaccard_index(g), 1.0 / 3.0, 1e-10);
    EXPECT_NEAR(f.jaccard_index(f), 1.0, 1e-10);

    // Both empty → 1.0
    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_NEAR(empty.jaccard_index(empty), 1.0, 1e-10);
}

TEST_F(MVDDTest, MVZDDHammingDistance) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{1, 2}, {2, 1}});

    // F △ G = {{0,0}, {2,1}} → size 2
    bigint::BigInt hd = f.hamming_distance(g);
    EXPECT_EQ(hd.to_string(), "2");
    EXPECT_EQ(f.hamming_distance(f).to_string(), "0");
}

// ============================================================
//  MVZDD::cost_bound_range tests
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundRange) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // 3 assignments: (0,0) cost=0, (1,2) cost=5, (2,1) cost=3
    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    std::vector<std::vector<int>> weights = {{0, 1, 2}, {0, 1, 2}};
    // costs: (0,0)→0, (1,2)→1+2=3, (2,1)→2+1=3

    // range [0, 0]: only (0,0)
    MVZDD r1 = f.cost_bound_range(weights, 0, 0);
    EXPECT_EQ(r1.count(), 1.0);
    EXPECT_TRUE(r1.contains({0, 0}));

    // range [1, 3]: (1,2) and (2,1) with cost 3
    MVZDD r2 = f.cost_bound_range(weights, 1, 3);
    EXPECT_EQ(r2.count(), 2.0);
    EXPECT_TRUE(r2.contains({1, 2}));
    EXPECT_TRUE(r2.contains({2, 1}));

    // range [0, 10]: everything
    MVZDD r3 = f.cost_bound_range(weights, 0, 10);
    EXPECT_EQ(r3.count(), 3.0);

    // range [4, 10]: nothing
    MVZDD r4 = f.cost_bound_range(weights, 4, 10);
    EXPECT_EQ(r4.count(), 0.0);
}

TEST_F(MVDDTest, MVZDDCostBoundRangeWithMemo) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    std::vector<std::vector<int>> weights = {{0, 1, 2}, {0, 1, 2}};

    CostBoundMemo memo;
    MVZDD r1 = f.cost_bound_range(weights, 0, 0, memo);
    EXPECT_EQ(r1.count(), 1.0);

    MVZDD r2 = f.cost_bound_range(weights, 1, 3, memo);
    EXPECT_EQ(r2.count(), 2.0);
}

// ============================================================
//  MVZDD::sample_k / random_subset tests
// ============================================================

TEST_F(MVDDTest, MVZDDSampleK) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}, {0, 1}});
    ZDD z = f.to_zdd();
    ZddCountMemo memo(z);

    std::mt19937_64 rng(42);
    MVZDD sampled = f.sample_k(2, rng, memo);
    EXPECT_EQ(sampled.count(), 2.0);
    EXPECT_TRUE(sampled.is_subset_family(f));

    // k=0 → empty
    std::mt19937_64 rng2(42);
    MVZDD empty = f.sample_k(0, rng2, memo);
    EXPECT_TRUE(empty.is_zero());

    // k >= total → all
    std::mt19937_64 rng3(42);
    MVZDD all = f.sample_k(10, rng3, memo);
    EXPECT_EQ(all, f);
}

TEST_F(MVDDTest, MVZDDRandomSubset) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}, {0, 1}});

    // p=0 → empty
    std::mt19937_64 rng1(42);
    MVZDD empty = f.random_subset(0.0, rng1);
    EXPECT_TRUE(empty.is_zero());

    // p=1 → all
    std::mt19937_64 rng2(42);
    MVZDD all = f.random_subset(1.0, rng2);
    EXPECT_EQ(all, f);

    // p=0.5 → subset
    std::mt19937_64 rng3(42);
    MVZDD sub = f.random_subset(0.5, rng3);
    EXPECT_TRUE(sub.is_subset_family(f));
    EXPECT_LE(sub.count(), 4.0);
}

// ============================================================
//  MVZDD::profile / profile_double tests
// ============================================================

TEST_F(MVDDTest, MVZDDProfileBasic) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // {(0,0), (1,2), (0,1)}
    // (0,0) → 0 non-zero vars
    // (1,2) → 2 non-zero vars
    // (0,1) → 1 non-zero var
    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {0, 1}});
    auto prof = f.profile();

    ASSERT_EQ(prof.size(), 3u);
    EXPECT_EQ(prof[0].to_string(), "1");  // (0,0)
    EXPECT_EQ(prof[1].to_string(), "1");  // (0,1)
    EXPECT_EQ(prof[2].to_string(), "1");  // (1,2)
}

TEST_F(MVDDTest, MVZDDProfileAllZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Only the all-zero assignment
    MVZDD f = MVZDD::from_sets(base, {{0, 0}});
    auto prof = f.profile();

    ASSERT_EQ(prof.size(), 1u);
    EXPECT_EQ(prof[0].to_string(), "1");
}

TEST_F(MVDDTest, MVZDDProfileAllNonZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // All vars non-zero
    MVZDD f = MVZDD::from_sets(base, {{1, 1}, {2, 2}, {1, 2}});
    auto prof = f.profile();

    ASSERT_EQ(prof.size(), 3u);
    EXPECT_EQ(prof[0].to_string(), "0");
    EXPECT_EQ(prof[1].to_string(), "0");
    EXPECT_EQ(prof[2].to_string(), "3");
}

TEST_F(MVDDTest, MVZDDProfileEmpty) {
    MVZDD base(3);
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {});
    auto prof = f.profile();
    EXPECT_TRUE(prof.empty());
}

TEST_F(MVDDTest, MVZDDProfileDouble) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {0, 1}});
    auto prof = f.profile_double();

    ASSERT_EQ(prof.size(), 3u);
    EXPECT_DOUBLE_EQ(prof[0], 1.0);
    EXPECT_DOUBLE_EQ(prof[1], 1.0);
    EXPECT_DOUBLE_EQ(prof[2], 1.0);
}

// ============================================================
//  MVZDD::element_frequency tests
// ============================================================

TEST_F(MVDDTest, MVZDDElementFrequencyBasic) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // {(0,0), (1,2), (1,0)}
    // var1: val0→1, val1→2, val2→0
    // var2: val0→2, val1→0, val2→1
    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {1, 0}});
    auto freq = f.element_frequency();

    ASSERT_EQ(freq.size(), 2u);
    ASSERT_EQ(freq[0].size(), 3u);
    ASSERT_EQ(freq[1].size(), 3u);

    EXPECT_EQ(freq[0][0].to_string(), "1");
    EXPECT_EQ(freq[0][1].to_string(), "2");
    EXPECT_EQ(freq[0][2].to_string(), "0");

    EXPECT_EQ(freq[1][0].to_string(), "2");
    EXPECT_EQ(freq[1][1].to_string(), "0");
    EXPECT_EQ(freq[1][2].to_string(), "1");
}

TEST_F(MVDDTest, MVZDDElementFrequencyAllZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Only {(0,0)}: each variable has value 0 once
    MVZDD f = MVZDD::from_sets(base, {{0, 0}});
    auto freq = f.element_frequency();

    ASSERT_EQ(freq.size(), 2u);
    EXPECT_EQ(freq[0][0].to_string(), "1");
    EXPECT_EQ(freq[0][1].to_string(), "0");
    EXPECT_EQ(freq[0][2].to_string(), "0");
    EXPECT_EQ(freq[1][0].to_string(), "1");
    EXPECT_EQ(freq[1][1].to_string(), "0");
    EXPECT_EQ(freq[1][2].to_string(), "0");
}

TEST_F(MVDDTest, MVZDDElementFrequencyEmpty) {
    MVZDD base(3);
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {});
    auto freq = f.element_frequency();

    ASSERT_EQ(freq.size(), 1u);
    EXPECT_EQ(freq[0][0].to_string(), "0");
    EXPECT_EQ(freq[0][1].to_string(), "0");
    EXPECT_EQ(freq[0][2].to_string(), "0");
}

TEST_F(MVDDTest, MVZDDElementFrequencySumEqualsTotal) {
    // For each variable, sum of all value frequencies == |F|
    MVZDD base(4);
    base.new_var();
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {
        {0, 0, 0}, {1, 2, 3}, {3, 1, 0}, {0, 3, 2}, {2, 0, 1}
    });
    auto freq = f.element_frequency();
    bigint::BigInt total = f.exact_count();

    ASSERT_EQ(freq.size(), 3u);
    for (size_t mv = 0; mv < freq.size(); ++mv) {
        bigint::BigInt sum;
        for (int v = 0; v < 4; ++v) {
            sum += freq[mv][v];
        }
        EXPECT_EQ(sum, total);
    }
}
