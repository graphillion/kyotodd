#include <gtest/gtest.h>
#include "bdd.h"

class UnreducedBDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

class UnreducedZDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// =============================================================
// UnreducedBDD Tests
// =============================================================

TEST_F(UnreducedBDDTest, FactoryZero) {
    UnreducedBDD z = UnreducedBDD::zero();
    EXPECT_TRUE(z.is_terminal());
    EXPECT_TRUE(z.is_zero());
    EXPECT_TRUE(z.is_reduced());
    EXPECT_EQ(z.get_id(), bddfalse);
}

TEST_F(UnreducedBDDTest, FactoryOne) {
    UnreducedBDD o = UnreducedBDD::one();
    EXPECT_TRUE(o.is_terminal());
    EXPECT_TRUE(o.is_one());
    EXPECT_TRUE(o.is_reduced());
    EXPECT_EQ(o.get_id(), bddtrue);
}

TEST_F(UnreducedBDDTest, DefaultConstructor) {
    UnreducedBDD u;
    EXPECT_EQ(u.get_id(), bddfalse);
    EXPECT_TRUE(u.is_reduced());
}

TEST_F(UnreducedBDDTest, IntConstructor) {
    UnreducedBDD u0(0);
    EXPECT_EQ(u0.get_id(), bddfalse);
    UnreducedBDD u1(1);
    EXPECT_EQ(u1.get_id(), bddtrue);
    UnreducedBDD un(-1);
    EXPECT_EQ(un.get_id(), bddnull);
}

TEST_F(UnreducedBDDTest, CopyConstructor) {
    bddvar v = bddnewvar();
    UnreducedBDD a = UnreducedBDD::node(v, UnreducedBDD::zero(), UnreducedBDD::one());
    UnreducedBDD b(a);
    EXPECT_EQ(a.get_id(), b.get_id());
}

TEST_F(UnreducedBDDTest, MoveConstructor) {
    bddvar v = bddnewvar();
    UnreducedBDD a = UnreducedBDD::node(v, UnreducedBDD::zero(), UnreducedBDD::one());
    bddp id = a.get_id();
    UnreducedBDD b(std::move(a));
    EXPECT_EQ(b.get_id(), id);
    EXPECT_EQ(a.get_id(), bddnull);
}

TEST_F(UnreducedBDDTest, ConversionFromBDD) {
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    UnreducedBDD u(x);
    EXPECT_EQ(u.get_id(), x.get_id());
    EXPECT_TRUE(u.is_reduced());
}

TEST_F(UnreducedBDDTest, NodeWithReducedChildren_DelegatesToGetnode) {
    bddvar v = bddnewvar();
    // Build unreduced node with reduced children and lo != hi
    UnreducedBDD lo = UnreducedBDD::zero();
    UnreducedBDD hi = UnreducedBDD::one();
    UnreducedBDD n = UnreducedBDD::node(v, lo, hi);
    // Should delegate to getnode -> reduced
    EXPECT_TRUE(n.is_reduced());
    // Should match BDDvar
    BDD x = BDDvar(v);
    EXPECT_EQ(n.get_id(), x.get_id());
}

TEST_F(UnreducedBDDTest, NodeLoEqualsHi_CreatesUnreducedNode) {
    bddvar v = bddnewvar();
    UnreducedBDD t = UnreducedBDD::one();
    UnreducedBDD n = UnreducedBDD::node(v, t, t);
    // lo == hi: unreduced (BDD reduction rule would eliminate this)
    EXPECT_FALSE(n.is_reduced());
    EXPECT_FALSE(n.is_terminal());
    EXPECT_EQ(n.top(), v);
}

TEST_F(UnreducedBDDTest, NodeLoEqualsHi_ReducesToChild) {
    bddvar v = bddnewvar();
    UnreducedBDD t = UnreducedBDD::one();
    UnreducedBDD n = UnreducedBDD::node(v, t, t);
    BDD reduced = n.reduce();
    // Reducing a node with lo == hi should eliminate it
    EXPECT_EQ(reduced.get_id(), bddtrue);
}

TEST_F(UnreducedBDDTest, NodeWithBddnullChildren) {
    bddvar v = bddnewvar();
    UnreducedBDD null(-1);
    UnreducedBDD n = UnreducedBDD::node(v, null, null);
    EXPECT_FALSE(n.is_reduced());
    EXPECT_EQ(n.top(), v);
}

TEST_F(UnreducedBDDTest, SetChild_Basic) {
    bddvar v = bddnewvar();
    UnreducedBDD null(-1);
    UnreducedBDD n = UnreducedBDD::node(v, null, null);

    // Set children
    n.set_child0(UnreducedBDD::zero());
    n.set_child1(UnreducedBDD::one());

    // Verify children were set
    EXPECT_EQ(UnreducedBDD::raw_child0(n.get_id()), bddfalse);
    EXPECT_EQ(UnreducedBDD::raw_child1(n.get_id()), bddtrue);
}

TEST_F(UnreducedBDDTest, SetChild_ThenReduce) {
    bddvar v = bddnewvar();
    UnreducedBDD null(-1);
    UnreducedBDD n = UnreducedBDD::node(v, null, null);

    n.set_child0(UnreducedBDD::zero());
    n.set_child1(UnreducedBDD::one());

    BDD reduced = n.reduce();
    BDD expected = BDDvar(v);
    EXPECT_EQ(reduced, expected);
}

TEST_F(UnreducedBDDTest, SetChild_ThrowsOnReduced) {
    bddvar v = bddnewvar();
    // This creates a reduced node (both children reduced, lo != hi)
    UnreducedBDD n = UnreducedBDD::node(v, UnreducedBDD::zero(), UnreducedBDD::one());
    EXPECT_TRUE(n.is_reduced());
    EXPECT_THROW(n.set_child0(UnreducedBDD::one()), std::invalid_argument);
}

TEST_F(UnreducedBDDTest, SetChild_ThrowsOnTerminal) {
    UnreducedBDD t = UnreducedBDD::one();
    EXPECT_THROW(t.set_child0(UnreducedBDD::zero()), std::invalid_argument);
}

TEST_F(UnreducedBDDTest, SetChild_ThrowsOnComplement) {
    bddvar v = bddnewvar();
    UnreducedBDD null(-1);
    UnreducedBDD n = UnreducedBDD::node(v, null, null);
    UnreducedBDD neg = ~n;
    EXPECT_THROW(neg.set_child0(UnreducedBDD::zero()), std::invalid_argument);
}

TEST_F(UnreducedBDDTest, ChildAccessors_BDDSemantics) {
    bddvar v = bddnewvar();
    UnreducedBDD lo = UnreducedBDD::zero();
    UnreducedBDD hi = UnreducedBDD::one();
    UnreducedBDD n = UnreducedBDD::node(v, lo, hi);

    // Non-complemented reference
    EXPECT_EQ(n.child0().get_id(), bddfalse);
    EXPECT_EQ(n.child1().get_id(), bddtrue);
}

TEST_F(UnreducedBDDTest, ChildAccessors_ComplementPropagation) {
    bddvar v = bddnewvar();
    UnreducedBDD lo = UnreducedBDD::zero();
    UnreducedBDD hi = UnreducedBDD::one();
    UnreducedBDD n = UnreducedBDD::node(v, lo, hi);
    bddp neg_id = bddnot(n.get_id());

    // BDD complement: both children are negated
    EXPECT_EQ(UnreducedBDD::child0(neg_id), bddtrue);
    EXPECT_EQ(UnreducedBDD::child1(neg_id), bddfalse);
}

TEST_F(UnreducedBDDTest, OperatorNot) {
    bddvar v = bddnewvar();
    UnreducedBDD n = UnreducedBDD::node(v, UnreducedBDD::zero(), UnreducedBDD::one());
    UnreducedBDD neg = ~n;
    EXPECT_EQ(neg.get_id(), bddnot(n.get_id()));
    // Double negation
    UnreducedBDD dneg = ~neg;
    EXPECT_EQ(dneg.get_id(), n.get_id());
}

TEST_F(UnreducedBDDTest, ComparisonOperators) {
    UnreducedBDD a = UnreducedBDD::zero();
    UnreducedBDD b = UnreducedBDD::one();
    EXPECT_TRUE(a == a);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != a);
    // operator< is based on bddp value
    EXPECT_TRUE((a < b) || (b < a));
}

TEST_F(UnreducedBDDTest, HashFunction) {
    std::unordered_map<UnreducedBDD, int> map;
    UnreducedBDD a = UnreducedBDD::zero();
    UnreducedBDD b = UnreducedBDD::one();
    map[a] = 1;
    map[b] = 2;
    EXPECT_EQ(map[a], 1);
    EXPECT_EQ(map[b], 2);
}

TEST_F(UnreducedBDDTest, IsReduced_BddnullReturnsFalse) {
    UnreducedBDD n(-1);
    EXPECT_EQ(n.get_id(), bddnull);
    EXPECT_FALSE(n.is_reduced());
}

TEST_F(UnreducedBDDTest, Reduce_Terminal) {
    BDD r0 = UnreducedBDD::zero().reduce();
    EXPECT_EQ(r0.get_id(), bddfalse);
    BDD r1 = UnreducedBDD::one().reduce();
    EXPECT_EQ(r1.get_id(), bddtrue);
}

TEST_F(UnreducedBDDTest, Reduce_AlreadyReduced) {
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    UnreducedBDD u(x);
    BDD reduced = u.reduce();
    EXPECT_EQ(reduced.get_id(), x.get_id());
}

TEST_F(UnreducedBDDTest, Reduce_ThrowsOnBddnull) {
    UnreducedBDD n(-1);
    EXPECT_THROW(n.reduce(), std::invalid_argument);
}

TEST_F(UnreducedBDDTest, Reduce_ChainedUnreducedNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Build: v2 node with lo == hi (unreduced)
    UnreducedBDD leaf = UnreducedBDD::one();
    UnreducedBDD mid = UnreducedBDD::node(v2, leaf, leaf);
    EXPECT_FALSE(mid.is_reduced());

    // Build: v1 node with unreduced child
    UnreducedBDD root = UnreducedBDD::node(v1, mid, UnreducedBDD::zero());
    EXPECT_FALSE(root.is_reduced());

    // Reduce: mid (lo == hi) should be eliminated to "one"
    // root becomes (v1, one, zero) = ~BDDvar(v1)
    BDD reduced = root.reduce();
    BDD expected = ~BDDvar(v1);
    EXPECT_EQ(reduced.get_id(), expected.get_id());
}

TEST_F(UnreducedBDDTest, Reduce_ComplementEdge) {
    bddvar v = bddnewvar();
    UnreducedBDD n = UnreducedBDD::node(v, UnreducedBDD::zero(), UnreducedBDD::one());
    UnreducedBDD neg = ~n;
    BDD reduced = neg.reduce();
    BDD expected = ~BDDvar(v);
    EXPECT_EQ(reduced.get_id(), expected.get_id());
}

TEST_F(UnreducedBDDTest, TopDownConstruction_FullWorkflow) {
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2 (higher)

    // Root must be at the highest level (v2) for proper BDD ordering
    UnreducedBDD null(-1);
    UnreducedBDD root = UnreducedBDD::node(v2, null, null);
    EXPECT_FALSE(root.is_reduced());

    // Create child at v1 (lower level)
    UnreducedBDD child_lo = UnreducedBDD::node(v1, UnreducedBDD::zero(),
                                                    UnreducedBDD::one());
    UnreducedBDD child_hi = UnreducedBDD::one();

    // Set children: (v2, v1_node, T)
    root.set_child0(child_lo);
    root.set_child1(child_hi);

    // Reduce: represents v1 | v2
    BDD reduced = root.reduce();

    BDD x1 = BDDvar(v1);
    BDD x2 = BDDvar(v2);
    BDD expected = x1 | x2;
    EXPECT_EQ(reduced.get_id(), expected.get_id());
}

TEST_F(UnreducedBDDTest, NodeWithUnreducedChild) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Create an unreduced child (lo == hi)
    UnreducedBDD unreduced_child = UnreducedBDD::node(v2,
        UnreducedBDD::zero(), UnreducedBDD::zero());
    EXPECT_FALSE(unreduced_child.is_reduced());

    // Create parent with one unreduced child
    UnreducedBDD parent = UnreducedBDD::node(v1, unreduced_child,
                                                  UnreducedBDD::one());
    EXPECT_FALSE(parent.is_reduced());
}

TEST_F(UnreducedBDDTest, RawChildAccessors) {
    bddvar v = bddnewvar();
    UnreducedBDD lo = UnreducedBDD::zero();
    UnreducedBDD hi = UnreducedBDD::one();
    UnreducedBDD n = UnreducedBDD::node(v, lo, hi);

    EXPECT_EQ(n.raw_child0().get_id(), bddfalse);
    EXPECT_EQ(n.raw_child1().get_id(), bddtrue);
    EXPECT_EQ(n.raw_child(0).get_id(), bddfalse);
    EXPECT_EQ(n.raw_child(1).get_id(), bddtrue);
}

TEST_F(UnreducedBDDTest, ChildAccessors_ThrowOnTerminal) {
    UnreducedBDD t = UnreducedBDD::one();
    EXPECT_THROW(t.child0(), std::invalid_argument);
    EXPECT_THROW(t.child1(), std::invalid_argument);
    EXPECT_THROW(t.raw_child0(), std::invalid_argument);
    EXPECT_THROW(t.raw_child1(), std::invalid_argument);
}

TEST_F(UnreducedBDDTest, ChildAccessors_ThrowOnNull) {
    EXPECT_THROW(UnreducedBDD::child0(bddnull), std::invalid_argument);
    EXPECT_THROW(UnreducedBDD::child1(bddnull), std::invalid_argument);
}

TEST_F(UnreducedBDDTest, ChildByIndex_ThrowOnInvalidIndex) {
    bddvar v = bddnewvar();
    UnreducedBDD n = UnreducedBDD::node(v, UnreducedBDD::zero(), UnreducedBDD::one());
    EXPECT_THROW(n.child(2), std::invalid_argument);
    EXPECT_THROW(n.raw_child(2), std::invalid_argument);
}

// =============================================================
// UnreducedZDD Tests
// =============================================================

TEST_F(UnreducedZDDTest, FactoryEmpty) {
    UnreducedZDD e = UnreducedZDD::empty();
    EXPECT_TRUE(e.is_terminal());
    EXPECT_TRUE(e.is_zero());
    EXPECT_TRUE(e.is_reduced());
    EXPECT_EQ(e.get_id(), bddempty);
}

TEST_F(UnreducedZDDTest, FactorySingle) {
    UnreducedZDD s = UnreducedZDD::single();
    EXPECT_TRUE(s.is_terminal());
    EXPECT_TRUE(s.is_one());
    EXPECT_TRUE(s.is_reduced());
    EXPECT_EQ(s.get_id(), bddsingle);
}

TEST_F(UnreducedZDDTest, ConversionFromZDD) {
    bddvar v = bddnewvar();
    ZDD z = ZDD(0).Change(v);
    UnreducedZDD u(z);
    EXPECT_EQ(u.get_id(), z.get_id());
    EXPECT_TRUE(u.is_reduced());
}

TEST_F(UnreducedZDDTest, NodeWithReducedChildren_DelegatesToGetznode) {
    bddvar v = bddnewvar();
    UnreducedZDD lo = UnreducedZDD::empty();
    UnreducedZDD hi = UnreducedZDD::single();
    UnreducedZDD n = UnreducedZDD::node(v, lo, hi);
    // Should delegate to getznode -> reduced
    EXPECT_TRUE(n.is_reduced());
}

TEST_F(UnreducedZDDTest, NodeHiEqualsEmpty_CreatesUnreducedNode) {
    bddvar v = bddnewvar();
    UnreducedZDD lo = UnreducedZDD::single();
    UnreducedZDD hi = UnreducedZDD::empty();
    // hi == bddempty: ZDD would suppress this, but unreduced allows it
    UnreducedZDD n = UnreducedZDD::node(v, lo, hi);
    EXPECT_FALSE(n.is_reduced());
    EXPECT_EQ(n.top(), v);
}

TEST_F(UnreducedZDDTest, NodeHiEqualsEmpty_ReducesToLo) {
    bddvar v = bddnewvar();
    UnreducedZDD lo = UnreducedZDD::single();
    UnreducedZDD hi = UnreducedZDD::empty();
    UnreducedZDD n = UnreducedZDD::node(v, lo, hi);
    ZDD reduced = n.reduce();
    // Reducing should apply zero-suppression: hi==empty => return lo
    EXPECT_EQ(reduced.get_id(), bddsingle);
}

TEST_F(UnreducedZDDTest, ChildAccessors_ZDDSemantics) {
    bddvar v = bddnewvar();
    UnreducedZDD lo = UnreducedZDD::empty();
    UnreducedZDD hi = UnreducedZDD::single();
    UnreducedZDD n = UnreducedZDD::node(v, lo, hi);

    EXPECT_EQ(n.child0().get_id(), bddempty);
    EXPECT_EQ(n.child1().get_id(), bddsingle);
}

TEST_F(UnreducedZDDTest, ChildAccessors_ComplementOnlyAffectsLo) {
    bddvar v = bddnewvar();
    UnreducedZDD lo = UnreducedZDD::empty();
    UnreducedZDD hi = UnreducedZDD::single();
    UnreducedZDD n = UnreducedZDD::node(v, lo, hi);
    bddp neg_id = bddnot(n.get_id());

    // ZDD complement: only lo is affected
    bddp c0 = UnreducedZDD::child0(neg_id);
    bddp c1 = UnreducedZDD::child1(neg_id);
    EXPECT_EQ(c0, bddnot(bddempty));  // lo is negated
    EXPECT_EQ(c1, bddsingle);          // hi is NOT negated
}

TEST_F(UnreducedZDDTest, OperatorNot) {
    UnreducedZDD e = UnreducedZDD::empty();
    UnreducedZDD neg = ~e;
    EXPECT_EQ(neg.get_id(), bddnot(bddempty));
    EXPECT_EQ((~neg).get_id(), bddempty);
}

TEST_F(UnreducedZDDTest, SetChild_Basic) {
    bddvar v = bddnewvar();
    UnreducedZDD null(-1);
    UnreducedZDD n = UnreducedZDD::node(v, null, null);

    n.set_child0(UnreducedZDD::empty());
    n.set_child1(UnreducedZDD::single());

    EXPECT_EQ(UnreducedZDD::raw_child0(n.get_id()), bddempty);
    EXPECT_EQ(UnreducedZDD::raw_child1(n.get_id()), bddsingle);
}

TEST_F(UnreducedZDDTest, SetChild_ThenReduce) {
    bddvar v = bddnewvar();
    UnreducedZDD null(-1);
    UnreducedZDD n = UnreducedZDD::node(v, null, null);

    n.set_child0(UnreducedZDD::empty());
    n.set_child1(UnreducedZDD::single());

    ZDD reduced = n.reduce();
    // {{v}} represented as ZDD: Change(v) on {{}} = {{v}}
    ZDD expected = ZDD(1).Change(v);
    EXPECT_EQ(reduced.get_id(), expected.get_id());
}

TEST_F(UnreducedZDDTest, TopDownConstruction_FullWorkflow) {
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2 (higher)

    // Root at v2 (highest level) with placeholder children
    UnreducedZDD null(-1);
    UnreducedZDD root = UnreducedZDD::node(v2, null, null);

    // Create child at v1 (lower level)
    UnreducedZDD v1_node = UnreducedZDD::node(v1, UnreducedZDD::empty(),
                                                   UnreducedZDD::single());

    // Set children: root = (v2, v1_node, single)
    // This represents: {{v1}, {v2}}
    root.set_child0(v1_node);
    root.set_child1(UnreducedZDD::single());

    // Reduce
    ZDD reduced = root.reduce();

    // Expected: {{v1}, {v2}}
    ZDD z1 = ZDD(1).Change(v1);
    ZDD z2 = ZDD(1).Change(v2);
    ZDD expected = z1 + z2;
    EXPECT_EQ(reduced.get_id(), expected.get_id());
}

TEST_F(UnreducedZDDTest, Reduce_Terminal) {
    ZDD r0 = UnreducedZDD::empty().reduce();
    EXPECT_EQ(r0.get_id(), bddempty);
    ZDD r1 = UnreducedZDD::single().reduce();
    EXPECT_EQ(r1.get_id(), bddsingle);
}

TEST_F(UnreducedZDDTest, Reduce_ThrowsOnBddnull) {
    UnreducedZDD n(-1);
    EXPECT_THROW(n.reduce(), std::invalid_argument);
}

TEST_F(UnreducedZDDTest, IsReduced_BddnullReturnsFalse) {
    UnreducedZDD n(-1);
    EXPECT_FALSE(n.is_reduced());
}

TEST_F(UnreducedZDDTest, HashFunction) {
    std::unordered_map<UnreducedZDD, int> map;
    UnreducedZDD a = UnreducedZDD::empty();
    UnreducedZDD b = UnreducedZDD::single();
    map[a] = 1;
    map[b] = 2;
    EXPECT_EQ(map[a], 1);
    EXPECT_EQ(map[b], 2);
}

TEST_F(UnreducedZDDTest, SetChild_ThrowsOnReduced) {
    bddvar v = bddnewvar();
    UnreducedZDD n = UnreducedZDD::node(v, UnreducedZDD::empty(),
                                             UnreducedZDD::single());
    EXPECT_TRUE(n.is_reduced());
    EXPECT_THROW(n.set_child0(UnreducedZDD::single()), std::invalid_argument);
}

TEST_F(UnreducedZDDTest, Reduce_ChainedUnreducedNodes) {
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2 (higher)

    // Build: v1 node with hi == bddempty (would be suppressed by ZDD rule)
    UnreducedZDD mid = UnreducedZDD::node(v1, UnreducedZDD::single(),
                                               UnreducedZDD::empty());
    EXPECT_FALSE(mid.is_reduced());

    // Build: v2 node (higher level) with unreduced child
    UnreducedZDD root = UnreducedZDD::node(v2, mid, UnreducedZDD::single());
    EXPECT_FALSE(root.is_reduced());

    // Reduce: mid should be suppressed (hi == empty => return lo = single)
    // root becomes getznode(v2, single, single)
    ZDD reduced = root.reduce();

    // getznode(v2, single, single) = (v2, single, single)
    // This is {{v2}, {}} = {{v2}, {}}
    ZDD z2 = ZDD(1).Change(v2);
    ZDD expected = z2 + ZDD(1);  // {{v2}} union {{}}
    EXPECT_EQ(reduced.get_id(), expected.get_id());
}
