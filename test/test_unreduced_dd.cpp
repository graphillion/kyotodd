#include <gtest/gtest.h>
#include "bdd.h"

class UnreducedDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// =============================================================
// Basic construction
// =============================================================

TEST_F(UnreducedDDTest, FactoryZero) {
    UnreducedDD z = UnreducedDD::zero();
    EXPECT_TRUE(z.is_terminal());
    EXPECT_TRUE(z.is_zero());
    EXPECT_TRUE(z.is_reduced());
    EXPECT_EQ(z.id(), bddfalse);
}

TEST_F(UnreducedDDTest, FactoryOne) {
    UnreducedDD o = UnreducedDD::one();
    EXPECT_TRUE(o.is_terminal());
    EXPECT_TRUE(o.is_one());
    EXPECT_TRUE(o.is_reduced());
    EXPECT_EQ(o.id(), bddtrue);
}

TEST_F(UnreducedDDTest, DefaultConstructor) {
    UnreducedDD u;
    EXPECT_EQ(u.id(), bddfalse);
    EXPECT_TRUE(u.is_reduced());
}

TEST_F(UnreducedDDTest, IntConstructor) {
    UnreducedDD u0(0);
    EXPECT_EQ(u0.id(), bddfalse);
    UnreducedDD u1(1);
    EXPECT_EQ(u1.id(), bddtrue);
    UnreducedDD un(-1);
    EXPECT_EQ(un.id(), bddnull);
}

TEST_F(UnreducedDDTest, CopyConstructor) {
    bddvar v = bddnewvar();
    UnreducedDD a = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    UnreducedDD b(a);
    EXPECT_EQ(a.id(), b.id());
}

TEST_F(UnreducedDDTest, MoveConstructor) {
    bddvar v = bddnewvar();
    UnreducedDD a = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    bddp id = a.id();
    UnreducedDD b(std::move(a));
    EXPECT_EQ(b.id(), id);
    EXPECT_EQ(a.id(), bddnull);
}

// =============================================================
// getnode (always creates unreduced nodes)
// =============================================================

TEST_F(UnreducedDDTest, Getnode_AlwaysUnreduced) {
    bddvar v = bddnewvar();
    // Even with reduced children and lo != hi, getnode creates unreduced node
    UnreducedDD lo = UnreducedDD::zero();
    UnreducedDD hi = UnreducedDD::one();
    UnreducedDD n = UnreducedDD::getnode(v, lo, hi);
    EXPECT_FALSE(n.is_reduced());
    EXPECT_FALSE(n.is_terminal());
    EXPECT_EQ(n.top(), v);
}

TEST_F(UnreducedDDTest, Getnode_LoEqualsHi) {
    bddvar v = bddnewvar();
    UnreducedDD t = UnreducedDD::one();
    UnreducedDD n = UnreducedDD::getnode(v, t, t);
    // lo == hi: no BDD reduction rule applied
    EXPECT_FALSE(n.is_reduced());
    EXPECT_FALSE(n.is_terminal());
    EXPECT_EQ(n.top(), v);
}

TEST_F(UnreducedDDTest, Getnode_HiEqualsEmpty) {
    bddvar v = bddnewvar();
    UnreducedDD lo = UnreducedDD::one();
    UnreducedDD hi = UnreducedDD::zero();  // bddempty == bddfalse
    UnreducedDD n = UnreducedDD::getnode(v, lo, hi);
    // hi == bddempty: no ZDD zero-suppression applied
    EXPECT_FALSE(n.is_reduced());
    EXPECT_EQ(n.top(), v);
}

TEST_F(UnreducedDDTest, Getnode_WithBddnullChildren) {
    bddvar v = bddnewvar();
    UnreducedDD null(-1);
    UnreducedDD n = UnreducedDD::getnode(v, null, null);
    EXPECT_FALSE(n.is_reduced());
    EXPECT_EQ(n.top(), v);
}

// =============================================================
// Raw child accessors (no complement interpretation)
// =============================================================

TEST_F(UnreducedDDTest, RawChildAccessors) {
    bddvar v = bddnewvar();
    UnreducedDD lo = UnreducedDD::zero();
    UnreducedDD hi = UnreducedDD::one();
    UnreducedDD n = UnreducedDD::getnode(v, lo, hi);

    EXPECT_EQ(n.raw_child0().id(), bddfalse);
    EXPECT_EQ(n.raw_child1().id(), bddtrue);
    EXPECT_EQ(n.raw_child(0).id(), bddfalse);
    EXPECT_EQ(n.raw_child(1).id(), bddtrue);
}

TEST_F(UnreducedDDTest, RawChildAccessors_ComplementedNodeReadsBaseNode) {
    bddvar v = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    UnreducedDD neg = ~n;

    // raw_child accesses the base node (strips complement bit)
    // but does NOT propagate complement to children
    EXPECT_EQ(neg.raw_child0().id(), bddfalse);
    EXPECT_EQ(neg.raw_child1().id(), bddtrue);
}

TEST_F(UnreducedDDTest, RawChildAccessors_WithComplementedChildren) {
    bddvar v = bddnewvar();
    // Store a complemented lo edge
    UnreducedDD neg_one = ~UnreducedDD::one();
    UnreducedDD hi = UnreducedDD::one();
    UnreducedDD n = UnreducedDD::getnode(v, neg_one, hi);

    // raw_child returns the stored value as-is (complement bit preserved)
    EXPECT_EQ(n.raw_child0().id(), bddnot(bddtrue));
    EXPECT_EQ(n.raw_child1().id(), bddtrue);
}

TEST_F(UnreducedDDTest, RawChildAccessors_ThrowOnTerminal) {
    UnreducedDD t = UnreducedDD::one();
    EXPECT_THROW(t.raw_child0(), std::invalid_argument);
    EXPECT_THROW(t.raw_child1(), std::invalid_argument);
}

TEST_F(UnreducedDDTest, RawChildAccessors_ThrowOnInvalidIndex) {
    bddvar v = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    EXPECT_THROW(n.raw_child(2), std::invalid_argument);
}

// =============================================================
// set_child0 / set_child1 (top-down construction)
// =============================================================

TEST_F(UnreducedDDTest, SetChild_Basic) {
    bddvar v = bddnewvar();
    UnreducedDD null(-1);
    UnreducedDD n = UnreducedDD::getnode(v, null, null);

    n.set_child0(UnreducedDD::zero());
    n.set_child1(UnreducedDD::one());

    EXPECT_EQ(UnreducedDD::raw_child0(n.id()), bddfalse);
    EXPECT_EQ(UnreducedDD::raw_child1(n.id()), bddtrue);
}

TEST_F(UnreducedDDTest, SetChild_ThrowsOnTerminal) {
    UnreducedDD t = UnreducedDD::one();
    EXPECT_THROW(t.set_child0(UnreducedDD::zero()), std::invalid_argument);
}

TEST_F(UnreducedDDTest, SetChild_ThrowsOnComplement) {
    bddvar v = bddnewvar();
    UnreducedDD null(-1);
    UnreducedDD n = UnreducedDD::getnode(v, null, null);
    UnreducedDD neg = ~n;
    EXPECT_THROW(neg.set_child0(UnreducedDD::zero()), std::invalid_argument);
}

// =============================================================
// operator~ (bit 0 toggle, no semantics)
// =============================================================

TEST_F(UnreducedDDTest, OperatorNot) {
    bddvar v = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    UnreducedDD neg = ~n;
    EXPECT_EQ(neg.id(), bddnot(n.id()));
    // Double negation
    UnreducedDD dneg = ~neg;
    EXPECT_EQ(dneg.id(), n.id());
}

TEST_F(UnreducedDDTest, OperatorNot_OnTerminals) {
    // ~zero == one, ~one == zero (terminal encoding)
    EXPECT_EQ((~UnreducedDD::zero()).id(), bddtrue);
    EXPECT_EQ((~UnreducedDD::one()).id(), bddfalse);
}

// =============================================================
// Comparison operators and hash
// =============================================================

TEST_F(UnreducedDDTest, ComparisonOperators) {
    UnreducedDD a = UnreducedDD::zero();
    UnreducedDD b = UnreducedDD::one();
    EXPECT_TRUE(a == a);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != a);
    EXPECT_TRUE((a < b) || (b < a));
}

TEST_F(UnreducedDDTest, HashFunction) {
    std::unordered_map<UnreducedDD, int> map;
    UnreducedDD a = UnreducedDD::zero();
    UnreducedDD b = UnreducedDD::one();
    map[a] = 1;
    map[b] = 2;
    EXPECT_EQ(map[a], 1);
    EXPECT_EQ(map[b], 2);
}

// =============================================================
// is_reduced
// =============================================================

TEST_F(UnreducedDDTest, IsReduced_BddnullReturnsFalse) {
    UnreducedDD n(-1);
    EXPECT_EQ(n.id(), bddnull);
    EXPECT_FALSE(n.is_reduced());
}

TEST_F(UnreducedDDTest, IsReduced_TerminalReturnsTrue) {
    EXPECT_TRUE(UnreducedDD::zero().is_reduced());
    EXPECT_TRUE(UnreducedDD::one().is_reduced());
}

TEST_F(UnreducedDDTest, IsReduced_GetnodeReturnsFalse) {
    bddvar v = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    EXPECT_FALSE(n.is_reduced());
}

// =============================================================
// reduce_as_bdd
// =============================================================

TEST_F(UnreducedDDTest, ReduceAsBDD_Terminal) {
    BDD r0 = UnreducedDD::zero().reduce_as_bdd();
    EXPECT_EQ(r0.id(), bddfalse);
    BDD r1 = UnreducedDD::one().reduce_as_bdd();
    EXPECT_EQ(r1.id(), bddtrue);
}

TEST_F(UnreducedDDTest, ReduceAsBDD_SimpleNode) {
    bddvar v = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    BDD reduced = n.reduce_as_bdd();
    BDD expected = BDDvar(v);
    EXPECT_EQ(reduced.id(), expected.id());
}

TEST_F(UnreducedDDTest, ReduceAsBDD_LoEqualsHi_Eliminated) {
    bddvar v = bddnewvar();
    UnreducedDD t = UnreducedDD::one();
    UnreducedDD n = UnreducedDD::getnode(v, t, t);
    BDD reduced = n.reduce_as_bdd();
    // BDD jump rule: lo == hi => return lo
    EXPECT_EQ(reduced.id(), bddtrue);
}

TEST_F(UnreducedDDTest, ReduceAsBDD_ChainedUnreducedNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Build: v2 node with lo == hi (unreduced)
    UnreducedDD leaf = UnreducedDD::one();
    UnreducedDD mid = UnreducedDD::getnode(v2, leaf, leaf);

    // Build: v1 node with unreduced child
    UnreducedDD root = UnreducedDD::getnode(v1, mid, UnreducedDD::zero());

    // Reduce: mid (lo == hi) eliminated to "one"
    // root becomes (v1, one, zero) = ~BDDvar(v1)
    BDD reduced = root.reduce_as_bdd();
    BDD expected = ~BDDvar(v1);
    EXPECT_EQ(reduced.id(), expected.id());
}

TEST_F(UnreducedDDTest, ReduceAsBDD_WithComplementEdge) {
    bddvar v = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    UnreducedDD neg = ~n;
    BDD reduced = neg.reduce_as_bdd();
    BDD expected = ~BDDvar(v);
    EXPECT_EQ(reduced.id(), expected.id());
}

TEST_F(UnreducedDDTest, ReduceAsBDD_ThrowsOnBddnull) {
    UnreducedDD n(-1);
    EXPECT_THROW(n.reduce_as_bdd(), std::invalid_argument);
}

TEST_F(UnreducedDDTest, ReduceAsBDD_TopDownConstruction) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    UnreducedDD null(-1);
    UnreducedDD root = UnreducedDD::getnode(v2, null, null);

    UnreducedDD child_lo = UnreducedDD::getnode(v1, UnreducedDD::zero(),
                                                     UnreducedDD::one());
    root.set_child0(child_lo);
    root.set_child1(UnreducedDD::one());

    BDD reduced = root.reduce_as_bdd();
    BDD x1 = BDDvar(v1);
    BDD x2 = BDDvar(v2);
    BDD expected = x1 | x2;
    EXPECT_EQ(reduced.id(), expected.id());
}

// =============================================================
// reduce_as_zdd
// =============================================================

TEST_F(UnreducedDDTest, ReduceAsZDD_Terminal) {
    ZDD r0 = UnreducedDD::zero().reduce_as_zdd();
    EXPECT_EQ(r0.id(), bddempty);
    ZDD r1 = UnreducedDD::one().reduce_as_zdd();
    EXPECT_EQ(r1.id(), bddsingle);
}

TEST_F(UnreducedDDTest, ReduceAsZDD_HiEqualsEmpty_Suppressed) {
    bddvar v = bddnewvar();
    UnreducedDD lo = UnreducedDD::one();
    UnreducedDD hi = UnreducedDD::zero();  // bddempty
    UnreducedDD n = UnreducedDD::getnode(v, lo, hi);
    ZDD reduced = n.reduce_as_zdd();
    // ZDD zero-suppression: hi == empty => return lo
    EXPECT_EQ(reduced.id(), bddsingle);
}

TEST_F(UnreducedDDTest, ReduceAsZDD_SimpleNode) {
    bddvar v = bddnewvar();
    UnreducedDD lo = UnreducedDD::zero();
    UnreducedDD hi = UnreducedDD::one();
    UnreducedDD n = UnreducedDD::getnode(v, lo, hi);
    ZDD reduced = n.reduce_as_zdd();
    // {{v}} = Change(v) on {{}}
    ZDD expected = ZDD(1).Change(v);
    EXPECT_EQ(reduced.id(), expected.id());
}

TEST_F(UnreducedDDTest, ReduceAsZDD_ChainedUnreducedNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // v1 node with hi == bddempty (would be suppressed)
    UnreducedDD mid = UnreducedDD::getnode(v1, UnreducedDD::one(),
                                             UnreducedDD::zero());

    // v2 node with unreduced child
    UnreducedDD root = UnreducedDD::getnode(v2, mid, UnreducedDD::one());

    // Reduce: mid suppressed (hi == empty => return lo = single)
    // root becomes (v2, single, single)
    ZDD reduced = root.reduce_as_zdd();

    ZDD z2 = ZDD(1).Change(v2);
    ZDD expected = z2 + ZDD(1);  // {{v2}, {}}
    EXPECT_EQ(reduced.id(), expected.id());
}

TEST_F(UnreducedDDTest, ReduceAsZDD_TopDownConstruction) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    UnreducedDD null(-1);
    UnreducedDD root = UnreducedDD::getnode(v2, null, null);

    UnreducedDD v1_node = UnreducedDD::getnode(v1, UnreducedDD::zero(),
                                                    UnreducedDD::one());
    root.set_child0(v1_node);
    root.set_child1(UnreducedDD::one());

    ZDD reduced = root.reduce_as_zdd();

    ZDD z1 = ZDD(1).Change(v1);
    ZDD z2 = ZDD(1).Change(v2);
    ZDD expected = z1 + z2;  // {{v1}, {v2}}
    EXPECT_EQ(reduced.id(), expected.id());
}

TEST_F(UnreducedDDTest, ReduceAsZDD_ThrowsOnBddnull) {
    UnreducedDD n(-1);
    EXPECT_THROW(n.reduce_as_zdd(), std::invalid_argument);
}

// =============================================================
// reduce_as_qdd
// =============================================================

TEST_F(UnreducedDDTest, ReduceAsQDD_Terminal) {
    QDD q0 = UnreducedDD::zero().reduce_as_qdd();
    EXPECT_EQ(q0.id(), bddfalse);
    QDD q1 = UnreducedDD::one().reduce_as_qdd();
    EXPECT_EQ(q1.id(), bddtrue);
}

TEST_F(UnreducedDDTest, ReduceAsQDD_SimpleNode) {
    bddvar v = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::one());
    QDD q = n.reduce_as_qdd();
    // QDD has same structure as BDD for single-variable case
    QDD expected = BDDvar(v).to_qdd();
    EXPECT_EQ(q.id(), expected.id());
}

TEST_F(UnreducedDDTest, ReduceAsQDD_MultiLevel) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Build: v2 node skipping v1 level
    UnreducedDD n = UnreducedDD::getnode(v2, UnreducedDD::zero(), UnreducedDD::one());
    QDD q = n.reduce_as_qdd();

    // QDD should have all levels filled
    QDD expected = BDDvar(v2).to_qdd();
    EXPECT_EQ(q.id(), expected.id());
}

// =============================================================
// Complement expansion constructors
// =============================================================

TEST_F(UnreducedDDTest, ConstructFromBDD_Terminal) {
    BDD b0 = BDD::False;
    BDD b1 = BDD::True;
    UnreducedDD u0(b0);
    UnreducedDD u1(b1);
    EXPECT_EQ(u0.id(), bddfalse);
    EXPECT_EQ(u1.id(), bddtrue);
}

TEST_F(UnreducedDDTest, ConstructFromBDD_SimpleNonComplemented) {
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    UnreducedDD u(x);
    // No complement edges in BDDvar(v), so the expanded version
    // is an unreduced copy of the same structure
    EXPECT_FALSE(u.is_reduced());  // new unreduced node
    EXPECT_EQ(u.top(), v);
    EXPECT_EQ(u.raw_child0().id(), bddfalse);
    EXPECT_EQ(u.raw_child1().id(), bddtrue);
    // Round-trip reduce should give back the same BDD
    BDD back = u.reduce_as_bdd();
    EXPECT_EQ(back.id(), x.id());
}

TEST_F(UnreducedDDTest, ConstructFromBDD_ComplementExpanded) {
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    BDD neg_x = ~x;  // Has complement edge on root

    UnreducedDD u(neg_x);
    // After complement expansion, no complement bit should remain
    EXPECT_EQ(u.id() & BDD_COMP_FLAG, 0u);
    // The expanded DD should represent ~x: (v, 1, 0)
    EXPECT_EQ(u.raw_child0().id(), bddtrue);
    EXPECT_EQ(u.raw_child1().id(), bddfalse);
    // Reduce back to BDD
    BDD back = u.reduce_as_bdd();
    EXPECT_EQ(back.id(), neg_x.id());
}

TEST_F(UnreducedDDTest, ConstructFromBDD_ComplexComplementExpansion) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD x1 = BDDvar(v1);
    BDD x2 = BDDvar(v2);
    BDD f = x1 & x2;  // AND may use complement edges internally

    UnreducedDD u(f);
    BDD back = u.reduce_as_bdd();
    EXPECT_EQ(back.id(), f.id());
}

TEST_F(UnreducedDDTest, ConstructFromZDD_SimpleNonComplemented) {
    bddvar v = bddnewvar();
    ZDD z = ZDD(1).Change(v);  // {{v}}
    UnreducedDD u(z);
    ZDD back = u.reduce_as_zdd();
    EXPECT_EQ(back.id(), z.id());
}

TEST_F(UnreducedDDTest, ConstructFromZDD_ComplementExpanded) {
    bddvar v = bddnewvar();
    ZDD z = ZDD(1).Change(v);  // {{v}}
    ZDD neg_z = ~z;  // complement

    UnreducedDD u(neg_z);
    // After complement expansion, no complement bit on root
    EXPECT_EQ(u.id() & BDD_COMP_FLAG, 0u);
    // Reduce back
    ZDD back = u.reduce_as_zdd();
    EXPECT_EQ(back.id(), neg_z.id());
}

TEST_F(UnreducedDDTest, ConstructFromQDD_RoundTrip) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD x1 = BDDvar(v1);
    QDD q = x1.to_qdd();

    UnreducedDD u(q);
    QDD back = u.reduce_as_qdd();
    EXPECT_EQ(back.id(), q.id());
}

// =============================================================
// wrap_raw
// =============================================================

TEST_F(UnreducedDDTest, WrapRaw_BDD) {
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    UnreducedDD u = UnreducedDD::wrap_raw(x);
    // Wraps the same bddp
    EXPECT_EQ(u.id(), x.id());
    EXPECT_TRUE(u.is_reduced());
    // Same-type reduce gives back the same BDD
    BDD back = u.reduce_as_bdd();
    EXPECT_EQ(back.id(), x.id());
}

TEST_F(UnreducedDDTest, WrapRaw_ZDD) {
    bddvar v = bddnewvar();
    ZDD z = ZDD(1).Change(v);
    UnreducedDD u = UnreducedDD::wrap_raw(z);
    EXPECT_EQ(u.id(), z.id());
    EXPECT_TRUE(u.is_reduced());
    ZDD back = u.reduce_as_zdd();
    EXPECT_EQ(back.id(), z.id());
}

TEST_F(UnreducedDDTest, WrapRaw_QDD) {
    bddvar v = bddnewvar();
    QDD q = BDDvar(v).to_qdd();
    UnreducedDD u = UnreducedDD::wrap_raw(q);
    EXPECT_EQ(u.id(), q.id());
}

// =============================================================
// Complement edge interpretation during reduce
// =============================================================

TEST_F(UnreducedDDTest, ComplementInterpretedAsB_BDD) {
    bddvar v = bddnewvar();
    // Build unreduced node with complemented lo: (v, ~0, 1)
    UnreducedDD n = UnreducedDD::getnode(v, ~UnreducedDD::zero(), UnreducedDD::one());

    // reduce_as_bdd: complement on lo is interpreted as BDD complement
    // BDD::getnode_raw(v, ~0, 1) = BDD::getnode_raw(v, 1, 1)
    //   with comp flag, then normalized => lo==hi => return 1, then negate => 0
    // Actually: ~0 = bddtrue. So (v, bddtrue, bddtrue) => jump rule => bddtrue, negate => bddfalse
    // Wait, let me think: reduce reads raw lo = ~zero = bddtrue, hi = bddtrue.
    // reduce(bddtrue) = bddtrue, reduce(bddtrue) = bddtrue.
    // BDD::getnode_raw(v, bddtrue, bddtrue) => lo==hi => return bddtrue.
    // No complement on f, so result = bddtrue.
    // Wait, ~zero = bddnot(bddfalse) = bddtrue. So the stored lo IS bddtrue (a terminal).
    // The reduce function reads node_lo(base) = bddtrue, node_hi(base) = bddtrue.
    BDD reduced = n.reduce_as_bdd();
    EXPECT_EQ(reduced.id(), bddtrue);
}

TEST_F(UnreducedDDTest, ComplementInterpretedAsZDD) {
    bddvar v = bddnewvar();
    // Build: (v, ~one, one) where ~one = bddfalse
    // Stored raw: lo = bddfalse, hi = bddtrue
    UnreducedDD n = UnreducedDD::getnode(v, ~UnreducedDD::one(), UnreducedDD::one());

    // reduce_as_zdd: lo = bddfalse, hi = bddtrue
    // ZDD::getnode_raw(v, bddfalse, bddtrue) => hi != bddempty, lo not complemented
    // Creates canonical node representing {{v}}
    ZDD reduced = n.reduce_as_zdd();
    ZDD expected = ZDD(1).Change(v);
    EXPECT_EQ(reduced.id(), expected.id());
}

TEST_F(UnreducedDDTest, ComplementOnNonTerminal_ReduceAsBDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Build inner node: (v1, 0, 1)
    UnreducedDD inner = UnreducedDD::getnode(v1, UnreducedDD::zero(), UnreducedDD::one());
    // Build outer node with complement: (v2, ~inner, 1)
    UnreducedDD outer = UnreducedDD::getnode(v2, ~inner, UnreducedDD::one());

    BDD reduced = outer.reduce_as_bdd();

    // ~inner through BDD reduce means: reduce(~inner)
    // = bddnot(reduce(inner)) = bddnot(BDDvar(v1)) = ~BDDvar(v1)
    // BDD::getnode_raw(v2, ~BDDvar(v1), bddtrue)
    // BDD comp normalization: lo is complemented, so flip both:
    // lo' = BDDvar(v1), hi' = bddfalse
    // Store (v2, BDDvar(v1), bddfalse), return complemented
    BDD x1 = BDDvar(v1);
    BDD x2 = BDDvar(v2);
    // The function is: if v2=0: ~x1, if v2=1: 1
    // = ~x1 & ~x2 | x2 = ~(x1 & ~x2) = ~x1 | x2
    BDD expected = ~x1 | x2;
    EXPECT_EQ(reduced.id(), expected.id());
}

TEST_F(UnreducedDDTest, SameStructure_DifferentReduceResults) {
    bddvar v = bddnewvar();
    // Build: (v, 0, 0) — lo == hi == bddzero == bddempty
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::zero(), UnreducedDD::zero());

    // reduce_as_bdd: lo == hi => jump rule => return bddfalse
    BDD bdd = n.reduce_as_bdd();
    EXPECT_EQ(bdd.id(), bddfalse);

    // reduce_as_zdd: hi == bddempty => zero-suppression => return lo = bddempty
    ZDD zdd = n.reduce_as_zdd();
    EXPECT_EQ(zdd.id(), bddempty);
}

TEST_F(UnreducedDDTest, SameStructure_DifferentResults_NonTrivial) {
    bddvar v = bddnewvar();
    // Build: (v, 1, 0) — hi == bddempty, lo == bddtrue
    UnreducedDD n = UnreducedDD::getnode(v, UnreducedDD::one(), UnreducedDD::zero());

    // reduce_as_bdd: lo != hi, creates BDD node (v, 1, 0) = ~BDDvar(v)
    BDD bdd = n.reduce_as_bdd();
    BDD expected_bdd = ~BDDvar(v);
    EXPECT_EQ(bdd.id(), expected_bdd.id());

    // reduce_as_zdd: hi == bddempty => zero-suppression => return lo = bddtrue
    ZDD zdd = n.reduce_as_zdd();
    EXPECT_EQ(zdd.id(), bddsingle);
}

// =============================================================
// Binary format export/import
// =============================================================

TEST_F(UnreducedDDTest, Binary_TerminalZero) {
    UnreducedDD f = UnreducedDD::zero();
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    UnreducedDD g = UnreducedDD::import_binary(iss);
    EXPECT_EQ(g.id(), bddfalse);
}

TEST_F(UnreducedDDTest, Binary_TerminalOne) {
    UnreducedDD f = UnreducedDD::one();
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    UnreducedDD g = UnreducedDD::import_binary(iss);
    EXPECT_EQ(g.id(), bddtrue);
}

TEST_F(UnreducedDDTest, Binary_SimpleRoundtrip) {
    bddvar v1 = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v1, UnreducedDD::zero(),
                                          UnreducedDD::one());
    std::ostringstream oss;
    n.export_binary(oss);
    std::istringstream iss(oss.str());
    UnreducedDD g = UnreducedDD::import_binary(iss);
    EXPECT_EQ(g.top(), n.top());
    EXPECT_EQ(g.raw_child0().id(), bddfalse);
    EXPECT_EQ(g.raw_child1().id(), bddtrue);
}

TEST_F(UnreducedDDTest, Binary_MultiLevel) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    UnreducedDD l1 = UnreducedDD::getnode(v1, UnreducedDD::zero(),
                                           UnreducedDD::one());
    UnreducedDD l2 = UnreducedDD::getnode(v2, l1, UnreducedDD::one());
    std::ostringstream oss;
    l2.export_binary(oss);
    std::istringstream iss(oss.str());
    UnreducedDD g = UnreducedDD::import_binary(iss);
    EXPECT_EQ(g.top(), l2.top());
    // Verify child structure is preserved
    UnreducedDD gc0 = g.raw_child0();
    EXPECT_EQ(gc0.top(), v1);
    EXPECT_EQ(gc0.raw_child0().id(), bddfalse);
    EXPECT_EQ(gc0.raw_child1().id(), bddtrue);
    EXPECT_EQ(g.raw_child1().id(), bddtrue);
}

TEST_F(UnreducedDDTest, Binary_FileRoundtrip) {
    bddvar v1 = bddnewvar();
    UnreducedDD n = UnreducedDD::getnode(v1, UnreducedDD::zero(),
                                          UnreducedDD::one());
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    n.export_binary(fp);
    std::rewind(fp);
    UnreducedDD g = UnreducedDD::import_binary(fp);
    std::fclose(fp);
    EXPECT_EQ(g.top(), n.top());
    EXPECT_EQ(g.raw_child0().id(), bddfalse);
    EXPECT_EQ(g.raw_child1().id(), bddtrue);
}

TEST_F(UnreducedDDTest, Binary_ReduceAfterImport) {
    // Build an unreduced DD that has a lo==hi node (BDD would collapse)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    UnreducedDD l1 = UnreducedDD::getnode(v1, UnreducedDD::zero(),
                                           UnreducedDD::one());
    // lo == hi: both are l1
    UnreducedDD l2 = UnreducedDD::getnode(v2, l1, l1);

    std::ostringstream oss;
    l2.export_binary(oss);
    std::istringstream iss(oss.str());
    UnreducedDD g = UnreducedDD::import_binary(iss);

    // reduce_as_bdd: lo==hi node at v2 should be collapsed by jump rule
    BDD bdd = g.reduce_as_bdd();
    BDD expected = BDD_ID(BDD::getnode(v1, bddfalse, bddtrue));
    EXPECT_EQ(bdd.id(), expected.id());
}

TEST_F(UnreducedDDTest, GetnodeRejectsOutOfRangeVar) {
    EXPECT_THROW(UnreducedDD::getnode(0, UnreducedDD::zero(), UnreducedDD::one()),
                 std::invalid_argument);
    EXPECT_THROW(UnreducedDD::getnode(999, UnreducedDD::zero(), UnreducedDD::one()),
                 std::invalid_argument);
    // Valid var should succeed
    bddvar v1 = bddnewvar();
    EXPECT_NO_THROW(UnreducedDD::getnode(v1, UnreducedDD::zero(), UnreducedDD::one()));
}
