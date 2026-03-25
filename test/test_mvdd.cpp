#include <gtest/gtest.h>
#include "bdd.h"
#include "mvdd.h"
#include <sstream>
#include <string>

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
    EXPECT_EQ(m.get_id(), bddnull);
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
    EXPECT_EQ(m.get_id(), bddnull);
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
