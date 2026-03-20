#include <gtest/gtest.h>
#include "bdd.h"

class QDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

TEST_F(QDDTest, FactoryZero) {
    QDD z = QDD::zero();
    EXPECT_TRUE(z.is_terminal());
    EXPECT_TRUE(z.is_zero());
    EXPECT_EQ(z.get_id(), bddfalse);
}

TEST_F(QDDTest, FactoryOne) {
    QDD o = QDD::one();
    EXPECT_TRUE(o.is_terminal());
    EXPECT_TRUE(o.is_one());
    EXPECT_EQ(o.get_id(), bddtrue);
}

TEST_F(QDDTest, NodePreservesLoEqualsHi) {
    // In BDD, getnode(var, a, a) returns a (jump rule).
    // In QDD, getqnode(var, a, a) creates a new node.
    bddvar v1 = bddnewvar();
    bddnewvar();  // v2 at level 2 (unused but needed to allocate levels)
    // v1 is level 1, v2 is level 2

    // Build QDD bottom-up: level 1 node, then level 2 node
    QDD lo_node = QDD::getnode(v1, QDD::zero(), QDD::zero());
    EXPECT_FALSE(lo_node.is_terminal());  // not collapsed to terminal

    // Compare with BDD which would collapse
    BDD bdd_collapsed = BDD_ID(BDD::getnode(v1, bddfalse, bddfalse));
    EXPECT_TRUE(bdd_collapsed.is_zero());  // BDD collapses lo==hi
}

TEST_F(QDDTest, LevelValidationRejectsWrongLevel) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // v1=level1, v2=level2, v3=level3

    // v3 (level 3) expects children at level 2, but terminals are level 0
    EXPECT_THROW(QDD::getnode(v3, QDD::zero(), QDD::zero()), std::invalid_argument);

    // v2 (level 2) expects children at level 1 — terminals are level 0
    EXPECT_THROW(QDD::getnode(v2, QDD::zero(), QDD::zero()), std::invalid_argument);

    // v1 (level 1) expects children at level 0 (terminals) — should work
    EXPECT_NO_THROW(QDD::getnode(v1, QDD::zero(), QDD::one()));
}

TEST_F(QDDTest, ComplementBDDSemantics) {
    bddvar v1 = bddnewvar();

    QDD n = QDD::getnode(v1, QDD::zero(), QDD::one());
    QDD nc = ~n;

    EXPECT_NE(n, nc);
    // BDD complement: ~(v1, 0, 1) = (v1, ~0, ~1) = (v1, 1, 0)
    // child0 resolves complement
    EXPECT_EQ(QDD::child0(nc.get_id()), bddtrue);
    EXPECT_EQ(QDD::child1(nc.get_id()), bddfalse);
}

TEST_F(QDDTest, RawChildAccessors) {
    bddvar v1 = bddnewvar();

    QDD n = QDD::getnode(v1, QDD::zero(), QDD::one());
    // raw: node_lo is bddfalse, node_hi is bddtrue (no complement on this node)
    EXPECT_EQ(QDD::raw_child0(n.get_id()), bddfalse);
    EXPECT_EQ(QDD::raw_child1(n.get_id()), bddtrue);

    // Member versions
    QDD rc0 = n.raw_child0();
    QDD rc1 = n.raw_child1();
    EXPECT_EQ(rc0.get_id(), bddfalse);
    EXPECT_EQ(rc1.get_id(), bddtrue);
}

TEST_F(QDDTest, UniqueTableSharing) {
    bddvar v1 = bddnewvar();

    QDD a = QDD::getnode(v1, QDD::zero(), QDD::one());
    QDD b = QDD::getnode(v1, QDD::zero(), QDD::one());
    // Same (var, lo, hi) should produce same node ID
    EXPECT_EQ(a.get_id(), b.get_id());
}

TEST_F(QDDTest, ThreeVariableFullPath) {
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2
    bddvar v3 = bddnewvar();  // level 3

    // Build bottom-up: every path visits all 3 levels
    // Level 1 nodes (children are terminals at level 0)
    QDD n1_ff = QDD::getnode(v1, QDD::zero(), QDD::zero());
    QDD n1_ft = QDD::getnode(v1, QDD::zero(), QDD::one());
    QDD n1_tf = QDD::getnode(v1, QDD::one(), QDD::zero());
    QDD n1_tt = QDD::getnode(v1, QDD::one(), QDD::one());

    // Level 2 nodes (children are level 1 nodes)
    QDD n2_lo = QDD::getnode(v2, n1_ff, n1_ft);
    QDD n2_hi = QDD::getnode(v2, n1_tf, n1_tt);

    // Level 3 root (children are level 2 nodes)
    QDD root = QDD::getnode(v3, n2_lo, n2_hi);

    EXPECT_FALSE(root.is_terminal());
    EXPECT_EQ(root.top(), v3);

    // Verify child chain
    QDD c0 = root.child0();
    QDD c1 = root.child1();
    EXPECT_EQ(c0.top(), v2);
    EXPECT_EQ(c1.top(), v2);

    QDD c00 = c0.child0();
    QDD c01 = c0.child1();
    EXPECT_EQ(c00.top(), v1);
    EXPECT_EQ(c01.top(), v1);
}

TEST_F(QDDTest, QDD_IDValidation) {
    // Terminal should work
    EXPECT_NO_THROW(QDD_ID(bddfalse));
    EXPECT_NO_THROW(QDD_ID(bddtrue));
    EXPECT_NO_THROW(QDD_ID(bddnull));

    // Reduced node should work
    bddvar v1 = bddnewvar();
    QDD n = QDD::getnode(v1, QDD::zero(), QDD::one());
    EXPECT_NO_THROW(QDD_ID(n.get_id()));
}

TEST_F(QDDTest, Constants) {
    EXPECT_EQ(QDD::False.get_id(), bddfalse);
    EXPECT_EQ(QDD::True.get_id(), bddtrue);
    EXPECT_EQ(QDD::Null.get_id(), bddnull);
}

// =============================================================
// QDD::to_bdd() Tests
// =============================================================

TEST_F(QDDTest, ToBddTerminal) {
    EXPECT_EQ(QDD::zero().to_bdd().get_id(), bddfalse);
    EXPECT_EQ(QDD::one().to_bdd().get_id(), bddtrue);
}

TEST_F(QDDTest, ToBddCollapseLoEqualsHi) {
    bddvar v1 = bddnewvar();

    // QDD node where lo == hi == false → BDD should collapse to false
    QDD q = QDD::getnode(v1, QDD::zero(), QDD::zero());
    BDD b = q.to_bdd();
    EXPECT_TRUE(b.is_zero());

    // QDD node where lo == hi == true → BDD should collapse to true
    QDD q2 = QDD::getnode(v1, QDD::one(), QDD::one());
    BDD b2 = q2.to_bdd();
    EXPECT_TRUE(b2.is_one());
}

TEST_F(QDDTest, ToBddRoundTrip) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Build a BDD: v2 -> (v1 ? 1 : 0) for lo, (0) for hi
    // = v2' & v1
    BDD bv1 = BDDvar(v1);
    BDD bv2 = BDDvar(v2);
    BDD original = ~bv2 & bv1;

    // Build the equivalent QDD manually
    // Level 1 nodes
    QDD q_lo = QDD::getnode(v1, QDD::zero(), QDD::one());   // v1
    QDD q_hi = QDD::getnode(v1, QDD::zero(), QDD::zero());  // false (identity)
    // Level 2 root
    QDD q_root = QDD::getnode(v2, q_lo, q_hi);

    BDD converted = q_root.to_bdd();
    EXPECT_EQ(converted.get_id(), original.get_id());
}

TEST_F(QDDTest, ToBddWithComplement) {
    bddvar v1 = bddnewvar();

    QDD q = QDD::getnode(v1, QDD::zero(), QDD::one());
    QDD qc = ~q;
    BDD b = qc.to_bdd();

    // ~(v1, 0, 1) = (v1, 1, 0) = ~v1
    BDD expected = ~BDDvar(v1);
    EXPECT_EQ(b.get_id(), expected.get_id());
}

// =============================================================
// QDD::to_zdd() Tests
// =============================================================

TEST_F(QDDTest, ToZddTerminal) {
    EXPECT_EQ(QDD::zero().to_zdd().get_id(), bddempty);
    EXPECT_EQ(QDD::one().to_zdd().get_id(), bddsingle);
}

TEST_F(QDDTest, ToZddConstantFunction) {
    bddvar v1 = bddnewvar();

    // QDD: (v1, false, false) → ZDD: empty
    QDD q_zero = QDD::getnode(v1, QDD::zero(), QDD::zero());
    ZDD z = q_zero.to_zdd();
    EXPECT_EQ(z.get_id(), bddempty);

    // QDD: (v1, true, true) → ZDD: {∅, {v1}} = universe for 1 var
    QDD q_one = QDD::getnode(v1, QDD::one(), QDD::one());
    ZDD z2 = q_one.to_zdd();
    // This should be the ZDD for "all subsets of {v1}" = {{}, {v1}}
    EXPECT_FALSE(z2.is_terminal());
}

TEST_F(QDDTest, ToZddSingleVariable) {
    bddvar v1 = bddnewvar();

    // QDD for function v1: (v1, false, true)
    QDD q = QDD::getnode(v1, QDD::zero(), QDD::one());
    ZDD z = q.to_zdd();

    // BDD v1 as ZDD = {{v1}}: (v1, empty, single)
    ZDD expected = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(z.get_id(), expected.get_id());
}

TEST_F(QDDTest, ToZddWithComplement) {
    bddvar v1 = bddnewvar();

    // ~(v1, 0, 1) = (v1, 1, 0) = ~v1
    QDD q = ~QDD::getnode(v1, QDD::zero(), QDD::one());
    ZDD z = q.to_zdd();

    // ~v1 as ZDD: the function "not v1" = {∅}
    // ZDD::getnode(v1, single, empty) → ZDD zero-suppression: hi==empty → return lo=single
    // So ZDD for ~v1 is just bddsingle
    EXPECT_EQ(z.get_id(), bddsingle);
}

// =============================================================
// BDD::to_qdd() Tests
// =============================================================

TEST_F(QDDTest, BddToQddTerminals) {
    bddnewvar();  // need at least one var for to_qdd

    QDD qf = BDD::False.to_qdd();
    QDD qt = BDD::True.to_qdd();

    // With 1 variable, false → (v1, false, false), true → (v1, true, true)
    EXPECT_FALSE(qf.is_terminal());
    EXPECT_EQ(QDD::child0(qf.get_id()), bddfalse);
    EXPECT_EQ(QDD::child1(qf.get_id()), bddfalse);

    EXPECT_FALSE(qt.is_terminal());
    EXPECT_EQ(QDD::child0(qt.get_id()), bddtrue);
    EXPECT_EQ(QDD::child1(qt.get_id()), bddtrue);
}

TEST_F(QDDTest, BddToQddAllLevelsPresent) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // BDD for v3 (just the top variable): skips levels 1 and 2
    BDD b = BDDvar(v3);
    QDD q = b.to_qdd();

    // Root should be at level 3
    EXPECT_EQ(q.top(), v3);
    // Children at level 2
    QDD c0 = q.child0();
    QDD c1 = q.child1();
    EXPECT_EQ(c0.top(), v2);
    EXPECT_EQ(c1.top(), v2);
    // Grandchildren at level 1
    EXPECT_EQ(c0.child0().top(), v1);
    EXPECT_EQ(c0.child1().top(), v1);
    EXPECT_EQ(c1.child0().top(), v1);
    EXPECT_EQ(c1.child1().top(), v1);
    // Great-grandchildren are terminals
    EXPECT_TRUE(c0.child0().child0().is_terminal());
}

TEST_F(QDDTest, BddToQddRoundTrip) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    BDD bv1 = BDDvar(v1);
    BDD bv2 = BDDvar(v2);
    BDD bv3 = BDDvar(v3);

    // Test several BDDs
    BDD cases[] = {
        bv1,
        bv2 & bv3,
        bv1 | ~bv2,
        bv1 ^ bv2 ^ bv3,
        ~bv3,
    };

    for (const BDD& orig : cases) {
        QDD q = orig.to_qdd();
        BDD back = q.to_bdd();
        EXPECT_EQ(back.get_id(), orig.get_id())
            << "Round-trip failed for BDD " << orig.get_id();
    }
}

// =============================================================
// ZDD::to_qdd() Tests
// =============================================================

TEST_F(QDDTest, ZddToQddEmpty) {
    bddnewvar();

    QDD q = ZDD::Empty.to_qdd();
    // Should be all-false QDD
    BDD b = q.to_bdd();
    EXPECT_TRUE(b.is_zero());
}

TEST_F(QDDTest, ZddToQddRoundTrip) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // ZDD {{v1}} = (v1, empty, single)
    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    // ZDD {{v2}} = (v2, empty, single)
    ZDD z2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    // ZDD {{v1}, {v2}} = union
    ZDD z_union = ZDD_ID(bddunion(z1.get_id(), z2.get_id()));

    ZDD cases[] = { z1, z2, z_union, ZDD::Empty, ZDD::Single };

    for (const ZDD& orig : cases) {
        QDD q = orig.to_qdd();
        ZDD back = q.to_zdd();
        EXPECT_EQ(back.get_id(), orig.get_id())
            << "Round-trip failed for ZDD " << orig.get_id();
    }
}

// =============================================================
// UnreducedDD::reduce_as_qdd() Tests
// =============================================================

TEST_F(QDDTest, UnreducedDDReduceAsQddRoundTrip) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Build unreduced DD for v1 & v2
    UnreducedDD u_false = UnreducedDD::zero();
    UnreducedDD u_true = UnreducedDD::one();
    UnreducedDD u_v1 = UnreducedDD::getnode(v1, u_false, u_true);
    UnreducedDD u_root = UnreducedDD::getnode(v2, u_false, u_v1);

    QDD q = u_root.reduce_as_qdd();
    BDD b = q.to_bdd();

    // Should equal BDD for v1 & v2
    BDD expected = BDDvar(v1) & BDDvar(v2);
    EXPECT_EQ(b.get_id(), expected.get_id());
}

TEST_F(QDDTest, UnreducedDDReduceAsQddFromZDDStructure) {
    bddvar v1 = bddnewvar();
    bddnewvar();  // v2 at level 2

    // Build unreduced DD for {{v1}} (ZDD-like structure)
    UnreducedDD u_zero = UnreducedDD::zero();
    UnreducedDD u_one = UnreducedDD::one();
    UnreducedDD u_v1 = UnreducedDD::getnode(v1, u_zero, u_one);

    // reduce_as_zdd first, then convert to QDD via reduce_as_qdd
    ZDD z = u_v1.reduce_as_zdd();
    QDD q_from_zdd = z.to_qdd();

    // Also reduce as QDD directly (uses BDD semantics)
    QDD q_direct = u_v1.reduce_as_qdd();

    // Both should give valid QDDs (but may differ since BDD vs ZDD semantics)
    // Just verify round-trip through to_bdd
    BDD b = q_direct.to_bdd();
    BDD expected = BDDvar(v1);
    EXPECT_EQ(b.get_id(), expected.get_id());
}

TEST_F(QDDTest, ChildAccessorExceptions) {
    // Terminal
    EXPECT_THROW(QDD::raw_child0(bddfalse), std::invalid_argument);
    EXPECT_THROW(QDD::child0(bddfalse), std::invalid_argument);
    // Null
    EXPECT_THROW(QDD::raw_child0(bddnull), std::invalid_argument);
    EXPECT_THROW(QDD::child0(bddnull), std::invalid_argument);
    // Invalid child index
    bddvar v1 = bddnewvar();
    QDD n = QDD::getnode(v1, QDD::zero(), QDD::one());
    EXPECT_THROW(QDD::raw_child(n.get_id(), 2), std::invalid_argument);
    EXPECT_THROW(QDD::child(n.get_id(), 2), std::invalid_argument);
}

// --- Binary format export/import tests ---

TEST_F(QDDTest, Binary_TerminalZero) {
    QDD f = QDD::zero();
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    QDD g = QDD::import_binary(iss);
    EXPECT_EQ(g, QDD::zero());
}

TEST_F(QDDTest, Binary_TerminalOne) {
    QDD f = QDD::one();
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    QDD g = QDD::import_binary(iss);
    EXPECT_EQ(g, QDD::one());
}

TEST_F(QDDTest, Binary_SimpleRoundtrip) {
    bddvar v1 = bddnewvar();
    QDD f = QDD::getnode(v1, QDD::zero(), QDD::one());
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    QDD g = QDD::import_binary(iss);
    EXPECT_EQ(f, g);
}

TEST_F(QDDTest, Binary_MultiLevel) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // Build bottom-up: level 1, then level 2, then level 3
    QDD l1a = QDD::getnode(v1, QDD::zero(), QDD::one());
    QDD l1b = QDD::getnode(v1, QDD::one(), QDD::zero());
    QDD l2 = QDD::getnode(v2, l1a, l1b);
    QDD l3 = QDD::getnode(v3, l2, l2);  // lo == hi (preserved in QDD)
    std::ostringstream oss;
    l3.export_binary(oss);
    std::istringstream iss(oss.str());
    QDD g = QDD::import_binary(iss);
    EXPECT_EQ(l3, g);
}

TEST_F(QDDTest, Binary_ComplementRoot) {
    bddvar v1 = bddnewvar();
    QDD f = ~QDD::getnode(v1, QDD::zero(), QDD::one());
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    QDD g = QDD::import_binary(iss);
    EXPECT_EQ(f, g);
}

TEST_F(QDDTest, Binary_LoEqualsHiPreserved) {
    // Verify that QDD preserves lo == hi nodes (BDD would collapse them)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    QDD l1 = QDD::getnode(v1, QDD::zero(), QDD::one());
    QDD l2 = QDD::getnode(v2, l1, l1);  // lo == hi
    EXPECT_FALSE(l2.is_terminal());  // not collapsed

    std::ostringstream oss;
    l2.export_binary(oss);
    std::istringstream iss(oss.str());
    QDD g = QDD::import_binary(iss);
    EXPECT_EQ(l2, g);
    EXPECT_FALSE(g.is_terminal());
}

TEST_F(QDDTest, Binary_FileRoundtrip) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    QDD l1 = QDD::getnode(v1, QDD::zero(), QDD::one());
    QDD f = QDD::getnode(v2, l1, ~l1);
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    f.export_binary(fp);
    std::rewind(fp);
    QDD g = QDD::import_binary(fp);
    std::fclose(fp);
    EXPECT_EQ(f, g);
}

// --- Multi-root binary format tests ---

TEST_F(QDDTest, BinaryMulti_RoundtripStream) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    QDD f1 = QDD::getnode(v2, QDD::getnode(v1, QDD::zero(), QDD::one()),
                                QDD::getnode(v1, QDD::one(), QDD::zero()));
    QDD f2 = QDD::getnode(v2, QDD::getnode(v1, QDD::one(), QDD::one()),
                                QDD::getnode(v1, QDD::zero(), QDD::one()));
    std::vector<QDD> qdds = {f1, f2};
    std::ostringstream oss;
    QDD::export_binary_multi(oss, qdds);
    std::istringstream iss(oss.str());
    std::vector<QDD> result = QDD::import_binary_multi(iss);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], f1);
    EXPECT_EQ(result[1], f2);
}

TEST_F(QDDTest, BinaryMulti_EmptyVector) {
    std::vector<QDD> qdds;
    std::ostringstream oss;
    QDD::export_binary_multi(oss, qdds);
    std::istringstream iss(oss.str());
    std::vector<QDD> result = QDD::import_binary_multi(iss);
    EXPECT_TRUE(result.empty());
}

TEST_F(QDDTest, BinaryMulti_TerminalsOnly) {
    std::vector<QDD> qdds = {QDD::zero(), QDD::one()};
    std::ostringstream oss;
    QDD::export_binary_multi(oss, qdds);
    std::istringstream iss(oss.str());
    std::vector<QDD> result = QDD::import_binary_multi(iss);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], QDD::zero());
    EXPECT_EQ(result[1], QDD::one());
}

TEST_F(QDDTest, BinaryMulti_FILE) {
    bddvar v1 = bddnewvar();
    QDD f1 = QDD::getnode(v1, QDD::zero(), QDD::one());
    QDD f2 = QDD::getnode(v1, QDD::one(), QDD::zero());
    std::vector<QDD> qdds = {f1, f2};
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    QDD::export_binary_multi(fp, qdds);
    std::rewind(fp);
    std::vector<QDD> result = QDD::import_binary_multi(fp);
    std::fclose(fp);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], f1);
    EXPECT_EQ(result[1], f2);
}
