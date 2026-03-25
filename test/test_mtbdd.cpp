#include <gtest/gtest.h>
#include "bdd.h"
#include "mtbdd.h"

class MTBDDTerminalTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        bddinit(256, UINT64_MAX);
    }
};

// --- MTBDDTerminalTable<int> tests ---

TEST_F(MTBDDTerminalTableTest, ZeroIndexIsAlwaysZero) {
    auto& table = MTBDDTerminalTable<int>::instance();
    EXPECT_EQ(table.zero_index(), 0u);
}

TEST_F(MTBDDTerminalTableTest, InitialSizeIsOne) {
    auto& table = MTBDDTerminalTable<int>::instance();
    EXPECT_EQ(table.size(), 1u);
}

TEST_F(MTBDDTerminalTableTest, ZeroValueAtIndexZero) {
    auto& table = MTBDDTerminalTable<int>::instance();
    EXPECT_EQ(table.get_value(0), 0);
}

TEST_F(MTBDDTerminalTableTest, InsertAndRetrieve) {
    auto& table = MTBDDTerminalTable<int>::instance();
    uint64_t idx = table.get_or_insert(42);
    EXPECT_GE(idx, 1u);
    EXPECT_EQ(table.get_value(idx), 42);
}

TEST_F(MTBDDTerminalTableTest, DuplicateInsertReturnsSameIndex) {
    auto& table = MTBDDTerminalTable<int>::instance();
    uint64_t idx1 = table.get_or_insert(100);
    uint64_t idx2 = table.get_or_insert(100);
    EXPECT_EQ(idx1, idx2);
}

TEST_F(MTBDDTerminalTableTest, ZeroInsertReturnsIndexZero) {
    auto& table = MTBDDTerminalTable<int>::instance();
    uint64_t idx = table.get_or_insert(0);
    EXPECT_EQ(idx, 0u);
}

TEST_F(MTBDDTerminalTableTest, MultipleValues) {
    auto& table = MTBDDTerminalTable<int>::instance();
    uint64_t idx1 = table.get_or_insert(10);
    uint64_t idx2 = table.get_or_insert(20);
    uint64_t idx3 = table.get_or_insert(30);
    EXPECT_NE(idx1, idx2);
    EXPECT_NE(idx2, idx3);
    EXPECT_EQ(table.get_value(idx1), 10);
    EXPECT_EQ(table.get_value(idx2), 20);
    EXPECT_EQ(table.get_value(idx3), 30);
}

TEST_F(MTBDDTerminalTableTest, ContainsWorks) {
    auto& table = MTBDDTerminalTable<int>::instance();
    EXPECT_TRUE(table.contains(0));
    EXPECT_FALSE(table.contains(999));
    table.get_or_insert(999);
    EXPECT_TRUE(table.contains(999));
}

TEST_F(MTBDDTerminalTableTest, GetValueOutOfRange) {
    auto& table = MTBDDTerminalTable<int>::instance();
    EXPECT_THROW(table.get_value(99999), std::out_of_range);
}

TEST_F(MTBDDTerminalTableTest, MakeTerminal) {
    bddp t0 = MTBDDTerminalTable<int>::make_terminal(0);
    bddp t1 = MTBDDTerminalTable<int>::make_terminal(1);
    bddp t2 = MTBDDTerminalTable<int>::make_terminal(2);
    EXPECT_EQ(t0, bddfalse);
    EXPECT_EQ(t1, bddtrue);
    EXPECT_EQ(t2, BDD_CONST_FLAG | 2);
    // All are terminals
    EXPECT_TRUE((t0 & BDD_CONST_FLAG) != 0);
    EXPECT_TRUE((t1 & BDD_CONST_FLAG) != 0);
    EXPECT_TRUE((t2 & BDD_CONST_FLAG) != 0);
}

TEST_F(MTBDDTerminalTableTest, TerminalIndex) {
    EXPECT_EQ(MTBDDTerminalTable<int>::terminal_index(bddfalse), 0u);
    EXPECT_EQ(MTBDDTerminalTable<int>::terminal_index(bddtrue), 1u);
    EXPECT_EQ(MTBDDTerminalTable<int>::terminal_index(BDD_CONST_FLAG | 42), 42u);
}

TEST_F(MTBDDTerminalTableTest, ClearResetsToZeroOnly) {
    auto& table = MTBDDTerminalTable<int>::instance();
    table.get_or_insert(10);
    table.get_or_insert(20);
    EXPECT_GE(table.size(), 3u);

    table.clear();
    EXPECT_EQ(table.size(), 1u);
    EXPECT_EQ(table.get_value(0), 0);
    EXPECT_FALSE(table.contains(10));
    EXPECT_FALSE(table.contains(20));
}

// --- Separate singleton per type ---

TEST_F(MTBDDTerminalTableTest, SeparateSingletonsPerType) {
    auto& int_table = MTBDDTerminalTable<int>::instance();
    auto& dbl_table = MTBDDTerminalTable<double>::instance();

    uint64_t int_idx = int_table.get_or_insert(42);
    uint64_t dbl_idx = dbl_table.get_or_insert(3.14);

    EXPECT_EQ(int_table.get_value(int_idx), 42);
    EXPECT_EQ(dbl_table.get_value(dbl_idx), 3.14);

    // They are separate singletons
    EXPECT_NE(static_cast<void*>(&int_table), static_cast<void*>(&dbl_table));
}

// --- bddfinal clears terminal tables ---

TEST_F(MTBDDTerminalTableTest, BddFinalClearsTerminalTables) {
    auto& table = MTBDDTerminalTable<int>::instance();
    table.get_or_insert(777);
    EXPECT_TRUE(table.contains(777));

    bddfinal();

    // After bddfinal, table should be cleared
    EXPECT_EQ(table.size(), 1u);
    EXPECT_FALSE(table.contains(777));
    EXPECT_EQ(table.get_value(0), 0);

    // Re-initialize for subsequent tests
    bddinit(256, UINT64_MAX);
}

// --- MTBDDTerminalTable<double> tests ---

TEST_F(MTBDDTerminalTableTest, DoubleTable) {
    auto& table = MTBDDTerminalTable<double>::instance();
    EXPECT_EQ(table.get_value(0), 0.0);

    uint64_t idx = table.get_or_insert(2.718);
    EXPECT_EQ(table.get_value(idx), 2.718);
    EXPECT_EQ(table.get_or_insert(2.718), idx);
}

// --- Singleton identity ---

TEST_F(MTBDDTerminalTableTest, SingletonIdentity) {
    auto& t1 = MTBDDTerminalTable<int>::instance();
    auto& t2 = MTBDDTerminalTable<int>::instance();
    EXPECT_EQ(&t1, &t2);
}

// ==========================================================================
// MTBDD<T> class tests
// ==========================================================================

class MTBDDClassTest : public ::testing::Test {
protected:
    void SetUp() override {
        bddinit(256, UINT64_MAX);
        for (int i = 0; i < 3; i++) bddnewvar();
    }
};

TEST_F(MTBDDClassTest, DefaultConstructorIsZeroTerminal) {
    MTBDD<int> f;
    EXPECT_TRUE(f.is_terminal());
    EXPECT_TRUE(f.is_zero());
    EXPECT_EQ(f.terminal_value(), 0);
}

TEST_F(MTBDDClassTest, TerminalFactory) {
    auto f = MTBDD<int>::terminal(42);
    EXPECT_TRUE(f.is_terminal());
    EXPECT_EQ(f.terminal_value(), 42);
}

TEST_F(MTBDDClassTest, TerminalZero) {
    auto f = MTBDD<int>::terminal(0);
    EXPECT_TRUE(f.is_zero());
    EXPECT_EQ(f.terminal_value(), 0);
}

TEST_F(MTBDDClassTest, TerminalValueThrowsOnNonTerminal) {
    auto lo = MTBDD<int>::terminal(10);
    auto hi = MTBDD<int>::terminal(20);
    auto f = MTBDD<int>::ite(1, hi, lo);
    EXPECT_FALSE(f.is_terminal());
    EXPECT_THROW(f.terminal_value(), std::logic_error);
}

TEST_F(MTBDDClassTest, IteCreatesNode) {
    auto lo = MTBDD<int>::terminal(10);
    auto hi = MTBDD<int>::terminal(20);
    auto f = MTBDD<int>::ite(1, hi, lo);
    EXPECT_FALSE(f.is_terminal());
    EXPECT_EQ(f.top(), 1u);
}

TEST_F(MTBDDClassTest, IteBddReduction) {
    // BDD reduction: lo == hi → return lo
    auto t = MTBDD<int>::terminal(42);
    auto f = MTBDD<int>::ite(1, t, t);
    // Should reduce to the terminal
    EXPECT_TRUE(f.is_terminal());
    EXPECT_EQ(f.terminal_value(), 42);
}

TEST_F(MTBDDClassTest, IteMultiLevel) {
    auto t0 = MTBDD<int>::terminal(0);
    auto t1 = MTBDD<int>::terminal(1);
    auto t2 = MTBDD<int>::terminal(2);
    auto t3 = MTBDD<int>::terminal(3);

    // f = (v1 ? t1 : t0), g = (v1 ? t3 : t2)
    auto f = MTBDD<int>::ite(1, t1, t0);
    auto g = MTBDD<int>::ite(1, t3, t2);

    // h = (v2 ? g : f) = (v2 ? (v1 ? 3 : 2) : (v1 ? 1 : 0))
    auto h = MTBDD<int>::ite(2, g, f);
    EXPECT_FALSE(h.is_terminal());
    EXPECT_EQ(h.top(), 2u);
}

TEST_F(MTBDDClassTest, CopySemantics) {
    auto f = MTBDD<int>::terminal(42);
    MTBDD<int> g = f;
    EXPECT_EQ(f, g);
    EXPECT_EQ(g.terminal_value(), 42);
}

TEST_F(MTBDDClassTest, MoveSemantics) {
    auto f = MTBDD<int>::terminal(42);
    bddp original_id = f.get_id();
    MTBDD<int> g = std::move(f);
    EXPECT_EQ(g.get_id(), original_id);
    EXPECT_EQ(g.terminal_value(), 42);
}

TEST_F(MTBDDClassTest, Equality) {
    auto f = MTBDD<int>::terminal(10);
    auto g = MTBDD<int>::terminal(10);
    auto h = MTBDD<int>::terminal(20);
    EXPECT_EQ(f, g);
    EXPECT_NE(f, h);
}

TEST_F(MTBDDClassTest, NoComplementEdges) {
    auto lo = MTBDD<int>::terminal(10);
    auto hi = MTBDD<int>::terminal(20);
    auto f = MTBDD<int>::ite(1, hi, lo);
    // Node ID should not have complement flag
    EXPECT_EQ(f.get_id() & BDD_COMP_FLAG, 0u);
}

TEST_F(MTBDDClassTest, ADDAlias) {
    auto f = ADD<double>::terminal(3.14);
    EXPECT_TRUE(f.is_terminal());
    EXPECT_EQ(f.terminal_value(), 3.14);
}

// ==========================================================================
// MTZDD<T> class tests
// ==========================================================================

TEST_F(MTBDDClassTest, MTZDDDefaultConstructor) {
    MTZDD<int> f;
    EXPECT_TRUE(f.is_terminal());
    EXPECT_TRUE(f.is_zero());
    EXPECT_EQ(f.terminal_value(), 0);
}

TEST_F(MTBDDClassTest, MTZDDTerminalFactory) {
    auto f = MTZDD<int>::terminal(99);
    EXPECT_TRUE(f.is_terminal());
    EXPECT_EQ(f.terminal_value(), 99);
}

TEST_F(MTBDDClassTest, MTZDDZeroSuppression) {
    // ZDD zero-suppression: hi == zero_terminal → return lo
    auto lo = MTZDD<int>::terminal(42);
    auto hi = MTZDD<int>::terminal(0);  // zero terminal
    auto f = MTZDD<int>::ite(1, hi, lo);
    // Should reduce to lo (42)
    EXPECT_TRUE(f.is_terminal());
    EXPECT_EQ(f.terminal_value(), 42);
}

TEST_F(MTBDDClassTest, MTZDDNoReductionWhenHiNonZero) {
    auto lo = MTZDD<int>::terminal(10);
    auto hi = MTZDD<int>::terminal(20);  // non-zero
    auto f = MTZDD<int>::ite(1, hi, lo);
    EXPECT_FALSE(f.is_terminal());
    EXPECT_EQ(f.top(), 1u);
}

TEST_F(MTBDDClassTest, MTZDDNoComplementEdges) {
    auto lo = MTZDD<int>::terminal(10);
    auto hi = MTZDD<int>::terminal(20);
    auto f = MTZDD<int>::ite(1, hi, lo);
    EXPECT_EQ(f.get_id() & BDD_COMP_FLAG, 0u);
}

TEST_F(MTBDDClassTest, MTBDDNoReductionWhenHiIsZero) {
    // MTBDD uses BDD reduction (lo == hi), NOT zero-suppression
    // So hi == zero_terminal but lo != hi should NOT reduce
    auto lo = MTBDD<int>::terminal(42);
    auto hi = MTBDD<int>::terminal(0);
    auto f = MTBDD<int>::ite(1, hi, lo);
    EXPECT_FALSE(f.is_terminal());
    EXPECT_EQ(f.top(), 1u);
}

// ==========================================================================
// MTBDD evaluate tests
// ==========================================================================

TEST_F(MTBDDClassTest, EvaluateTerminal) {
    auto f = MTBDD<int>::terminal(42);
    std::vector<int> a = {0, 0, 0, 0};  // index 0 unused
    EXPECT_EQ(f.evaluate(a), 42);
}

TEST_F(MTBDDClassTest, EvaluateSingleVariable) {
    auto t0 = MTBDD<int>::terminal(10);
    auto t1 = MTBDD<int>::terminal(20);
    auto f = MTBDD<int>::ite(1, t1, t0);  // v1 ? 20 : 10

    std::vector<int> a0 = {0, 0, 0, 0};
    std::vector<int> a1 = {0, 1, 0, 0};
    EXPECT_EQ(f.evaluate(a0), 10);
    EXPECT_EQ(f.evaluate(a1), 20);
}

TEST_F(MTBDDClassTest, EvaluateTwoVariables) {
    auto t0 = MTBDD<int>::terminal(0);
    auto t1 = MTBDD<int>::terminal(1);
    auto t2 = MTBDD<int>::terminal(2);
    auto t3 = MTBDD<int>::terminal(3);

    // f(v1, v2) = v2 ? (v1 ? 3 : 2) : (v1 ? 1 : 0)
    auto low  = MTBDD<int>::ite(1, t1, t0);
    auto high = MTBDD<int>::ite(1, t3, t2);
    auto f = MTBDD<int>::ite(2, high, low);

    // Exhaustive evaluation
    EXPECT_EQ(f.evaluate({0, 0, 0, 0}), 0);  // v1=0, v2=0
    EXPECT_EQ(f.evaluate({0, 1, 0, 0}), 1);  // v1=1, v2=0
    EXPECT_EQ(f.evaluate({0, 0, 1, 0}), 2);  // v1=0, v2=1
    EXPECT_EQ(f.evaluate({0, 1, 1, 0}), 3);  // v1=1, v2=1
}

// ==========================================================================
// MTZDD evaluate tests
// ==========================================================================

TEST_F(MTBDDClassTest, MTZDDEvaluateTerminal) {
    auto f = MTZDD<int>::terminal(42);
    std::vector<int> a = {0, 0, 0, 0};
    EXPECT_EQ(f.evaluate(a), 42);
}

TEST_F(MTBDDClassTest, MTZDDEvaluateSingleVariable) {
    auto t0 = MTZDD<int>::terminal(0);
    auto t1 = MTZDD<int>::terminal(5);
    // v1=1 → 5, v1=0 → 0 (but hi=0 would be zero-suppressed)
    // So use a non-zero lo
    auto lo = MTZDD<int>::terminal(10);
    auto hi = MTZDD<int>::terminal(20);
    auto f = MTZDD<int>::ite(1, hi, lo);

    EXPECT_EQ(f.evaluate({0, 0, 0, 0}), 10);
    EXPECT_EQ(f.evaluate({0, 1, 0, 0}), 20);
}

TEST_F(MTBDDClassTest, MTZDDEvaluateZeroSuppressedVariable) {
    // Create MTZDD where variable 2 is zero-suppressed
    // Structure: node at v3 (lo→terminal(7), hi→terminal(5))
    // Variable 2 is not in the MTZDD (zero-suppressed)
    // Variable 1 is not in the MTZDD (zero-suppressed)
    auto lo = MTZDD<int>::terminal(7);
    auto hi = MTZDD<int>::terminal(5);
    auto f = MTZDD<int>::ite(3, hi, lo);

    // v3=0 → 7 (follow lo)
    EXPECT_EQ(f.evaluate({0, 0, 0, 0}), 7);
    // v3=1 → 5 (follow hi)
    EXPECT_EQ(f.evaluate({0, 0, 0, 1}), 5);
    // v3=0, v2=1 → 0 (v2 is zero-suppressed, assignment=1 → zero)
    EXPECT_EQ(f.evaluate({0, 0, 1, 0}), 0);
    // v3=0, v1=1 → 0 (v1 is zero-suppressed, assignment=1 → zero)
    EXPECT_EQ(f.evaluate({0, 1, 0, 0}), 0);
    // v3=1, v2=1 → 0 (v2 is zero-suppressed)
    EXPECT_EQ(f.evaluate({0, 0, 1, 1}), 0);
}

TEST_F(MTBDDClassTest, MTZDDEvaluateMultiLevel) {
    // Create: v3 → (lo: v1 → (lo:10, hi:20), hi: 30)
    auto t10 = MTZDD<int>::terminal(10);
    auto t20 = MTZDD<int>::terminal(20);
    auto t30 = MTZDD<int>::terminal(30);

    auto v1_node = MTZDD<int>::ite(1, t20, t10);  // v1 ? 20 : 10
    auto f = MTZDD<int>::ite(3, t30, v1_node);     // v3 ? 30 : (v1 ? 20 : 10)

    // Variable 2 is zero-suppressed between v3 and v1
    // v3=0, v2=0, v1=0 → 10
    EXPECT_EQ(f.evaluate({0, 0, 0, 0}), 10);
    // v3=0, v2=0, v1=1 → 20
    EXPECT_EQ(f.evaluate({0, 1, 0, 0}), 20);
    // v3=1, v2=0, v1=0 → 30
    EXPECT_EQ(f.evaluate({0, 0, 0, 1}), 30);
    // v3=0, v2=1, v1=0 → 0 (v2 zero-suppressed with assignment=1)
    EXPECT_EQ(f.evaluate({0, 0, 1, 0}), 0);
    // v3=0, v2=1, v1=1 → 0 (v2 zero-suppressed with assignment=1)
    EXPECT_EQ(f.evaluate({0, 1, 1, 0}), 0);
}

// ==========================================================================
// GC tests
// ==========================================================================

TEST_F(MTBDDClassTest, GCProtection) {
    // MTBDD nodes should survive GC
    auto lo = MTBDD<int>::terminal(10);
    auto hi = MTBDD<int>::terminal(20);
    auto f = MTBDD<int>::ite(1, hi, lo);
    bddp before = f.get_id();

    bddgc();

    EXPECT_EQ(f.get_id(), before);
    EXPECT_EQ(f.top(), 1u);
}
