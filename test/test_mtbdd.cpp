#include <gtest/gtest.h>
#include <sstream>
#include "bdd.h"
#include "mtbdd.h"

using namespace kyotodd;

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
    bddp original_id = f.id();
    MTBDD<int> g = std::move(f);
    EXPECT_EQ(g.id(), original_id);
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
    EXPECT_EQ(f.id() & BDD_COMP_FLAG, 0u);
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
    EXPECT_EQ(f.id() & BDD_COMP_FLAG, 0u);
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
// MTBDD apply / binary operator tests
// ==========================================================================

TEST_F(MTBDDClassTest, AddTerminals) {
    auto a = MTBDD<int>::terminal(3);
    auto b = MTBDD<int>::terminal(5);
    auto c = a + b;
    EXPECT_TRUE(c.is_terminal());
    EXPECT_EQ(c.terminal_value(), 8);
}

TEST_F(MTBDDClassTest, SubtractTerminals) {
    auto a = MTBDD<int>::terminal(10);
    auto b = MTBDD<int>::terminal(3);
    auto c = a - b;
    EXPECT_TRUE(c.is_terminal());
    EXPECT_EQ(c.terminal_value(), 7);
}

TEST_F(MTBDDClassTest, MultiplyTerminals) {
    auto a = MTBDD<int>::terminal(4);
    auto b = MTBDD<int>::terminal(7);
    auto c = a * b;
    EXPECT_TRUE(c.is_terminal());
    EXPECT_EQ(c.terminal_value(), 28);
}

TEST_F(MTBDDClassTest, MinMaxTerminals) {
    auto a = MTBDD<int>::terminal(3);
    auto b = MTBDD<int>::terminal(7);
    EXPECT_EQ(MTBDD<int>::min(a, b).terminal_value(), 3);
    EXPECT_EQ(MTBDD<int>::max(a, b).terminal_value(), 7);
}

TEST_F(MTBDDClassTest, AddBruteForce) {
    // f(v1,v2) = v2 ? (v1 ? 3 : 2) : (v1 ? 1 : 0)
    // g(v1,v2) = v2 ? 10 : 20
    auto f = MTBDD<int>::ite(2,
        MTBDD<int>::ite(1, MTBDD<int>::terminal(3), MTBDD<int>::terminal(2)),
        MTBDD<int>::ite(1, MTBDD<int>::terminal(1), MTBDD<int>::terminal(0)));
    auto g = MTBDD<int>::ite(2,
        MTBDD<int>::terminal(10),
        MTBDD<int>::terminal(20));

    auto h = f + g;

    // Brute force verification
    for (int v1 = 0; v1 <= 1; v1++) {
        for (int v2 = 0; v2 <= 1; v2++) {
            std::vector<int> a = {0, v1, v2, 0};
            int expected = f.evaluate(a) + g.evaluate(a);
            EXPECT_EQ(h.evaluate(a), expected)
                << "v1=" << v1 << " v2=" << v2;
        }
    }
}

TEST_F(MTBDDClassTest, MultiplyBruteForce) {
    auto f = MTBDD<int>::ite(1,
        MTBDD<int>::terminal(3), MTBDD<int>::terminal(2));
    auto g = MTBDD<int>::ite(2,
        MTBDD<int>::terminal(5), MTBDD<int>::terminal(1));

    auto h = f * g;

    for (int v1 = 0; v1 <= 1; v1++) {
        for (int v2 = 0; v2 <= 1; v2++) {
            std::vector<int> a = {0, v1, v2, 0};
            int expected = f.evaluate(a) * g.evaluate(a);
            EXPECT_EQ(h.evaluate(a), expected)
                << "v1=" << v1 << " v2=" << v2;
        }
    }
}

TEST_F(MTBDDClassTest, CompoundAssignment) {
    auto f = MTBDD<int>::terminal(3);
    auto g = MTBDD<int>::terminal(5);
    f += g;
    EXPECT_EQ(f.terminal_value(), 8);
}

TEST_F(MTBDDClassTest, ApplyDouble) {
    auto f = ADD<double>::ite(1,
        ADD<double>::terminal(1.5), ADD<double>::terminal(2.5));
    auto g = ADD<double>::ite(1,
        ADD<double>::terminal(3.0), ADD<double>::terminal(0.5));

    auto h = f + g;
    EXPECT_DOUBLE_EQ(h.evaluate({0, 0, 0}), 3.0);  // 2.5 + 0.5
    EXPECT_DOUBLE_EQ(h.evaluate({0, 1, 0}), 4.5);  // 1.5 + 3.0
}

// ==========================================================================
// MTBDD ITE (3-operand) tests
// ==========================================================================

TEST_F(MTBDDClassTest, IteTerminalConditionZero) {
    auto cond = MTBDD<int>::terminal(0);  // zero → else
    auto then_val = MTBDD<int>::terminal(10);
    auto else_val = MTBDD<int>::terminal(20);
    auto result = cond.ite(then_val, else_val);
    EXPECT_EQ(result.terminal_value(), 20);
}

TEST_F(MTBDDClassTest, IteTerminalConditionNonZero) {
    auto cond = MTBDD<int>::terminal(5);  // non-zero → then
    auto then_val = MTBDD<int>::terminal(10);
    auto else_val = MTBDD<int>::terminal(20);
    auto result = cond.ite(then_val, else_val);
    EXPECT_EQ(result.terminal_value(), 10);
}

TEST_F(MTBDDClassTest, IteBruteForce) {
    // cond = (v1 ? 1 : 0), then = terminal(100), else = terminal(200)
    auto cond = MTBDD<int>::ite(1,
        MTBDD<int>::terminal(1), MTBDD<int>::terminal(0));
    auto then_val = MTBDD<int>::terminal(100);
    auto else_val = MTBDD<int>::terminal(200);

    auto result = cond.ite(then_val, else_val);

    // v1=0 → cond=0 → else=200
    EXPECT_EQ(result.evaluate({0, 0, 0}), 200);
    // v1=1 → cond=1 → then=100
    EXPECT_EQ(result.evaluate({0, 1, 0}), 100);
}

TEST_F(MTBDDClassTest, IteWithNonTrivialBranches) {
    // cond = (v2 ? 3 : 0)
    // then = (v1 ? 10 : 20)
    // else = (v1 ? 30 : 40)
    auto cond = MTBDD<int>::ite(2,
        MTBDD<int>::terminal(3), MTBDD<int>::terminal(0));
    auto then_br = MTBDD<int>::ite(1,
        MTBDD<int>::terminal(10), MTBDD<int>::terminal(20));
    auto else_br = MTBDD<int>::ite(1,
        MTBDD<int>::terminal(30), MTBDD<int>::terminal(40));

    auto result = cond.ite(then_br, else_br);

    for (int v1 = 0; v1 <= 1; v1++) {
        for (int v2 = 0; v2 <= 1; v2++) {
            std::vector<int> a = {0, v1, v2, 0};
            int c = cond.evaluate(a);
            int expected = (c == 0) ? else_br.evaluate(a) : then_br.evaluate(a);
            EXPECT_EQ(result.evaluate(a), expected)
                << "v1=" << v1 << " v2=" << v2;
        }
    }
}

// ==========================================================================
// MTZDD apply tests
// ==========================================================================

TEST_F(MTBDDClassTest, MTZDDAddTerminals) {
    auto a = MTZDD<int>::terminal(3);
    auto b = MTZDD<int>::terminal(5);
    auto c = a + b;
    EXPECT_TRUE(c.is_terminal());
    EXPECT_EQ(c.terminal_value(), 8);
}

TEST_F(MTBDDClassTest, MTZDDAddBruteForce) {
    auto f = MTZDD<int>::ite(1,
        MTZDD<int>::terminal(3), MTZDD<int>::terminal(2));
    auto g = MTZDD<int>::ite(1,
        MTZDD<int>::terminal(5), MTZDD<int>::terminal(1));

    auto h = f + g;

    EXPECT_EQ(h.evaluate({0, 0}), 3);  // 2+1
    EXPECT_EQ(h.evaluate({0, 1}), 8);  // 3+5
}

// ==========================================================================
// from_bdd / from_zdd tests
// ==========================================================================

TEST_F(MTBDDClassTest, FromBddFalse) {
    BDD f(0);
    auto mt = MTBDD<int>::from_bdd(f);
    EXPECT_TRUE(mt.is_terminal());
    EXPECT_EQ(mt.terminal_value(), 0);
}

TEST_F(MTBDDClassTest, FromBddTrue) {
    BDD f(1);
    auto mt = MTBDD<int>::from_bdd(f);
    EXPECT_TRUE(mt.is_terminal());
    EXPECT_EQ(mt.terminal_value(), 1);
}

TEST_F(MTBDDClassTest, FromBddVariable) {
    // BDD for variable 1: v1 ? true : false
    BDD f = BDD::prime(1);
    auto mt = MTBDD<int>::from_bdd(f);

    EXPECT_EQ(mt.evaluate({0, 0, 0}), 0);
    EXPECT_EQ(mt.evaluate({0, 1, 0}), 1);
}

TEST_F(MTBDDClassTest, FromBddCustomTerminals) {
    BDD f = BDD::prime(1);
    auto mt = MTBDD<double>::from_bdd(f, 0.0, 10.0);

    EXPECT_DOUBLE_EQ(mt.evaluate({0, 0, 0}), 0.0);
    EXPECT_DOUBLE_EQ(mt.evaluate({0, 1, 0}), 10.0);
}

TEST_F(MTBDDClassTest, FromBddAnd) {
    BDD f = BDD::prime(1) & BDD::prime(2);
    auto mt = MTBDD<int>::from_bdd(f, 0, 100);

    // AND: only (1,1) → 100, rest → 0
    EXPECT_EQ(mt.evaluate({0, 0, 0, 0}), 0);
    EXPECT_EQ(mt.evaluate({0, 1, 0, 0}), 0);
    EXPECT_EQ(mt.evaluate({0, 0, 1, 0}), 0);
    EXPECT_EQ(mt.evaluate({0, 1, 1, 0}), 100);
}

TEST_F(MTBDDClassTest, FromBddNot) {
    BDD f = ~BDD::prime(1);
    auto mt = MTBDD<int>::from_bdd(f, 0, 1);

    // NOT v1: v1=0 → 1, v1=1 → 0
    EXPECT_EQ(mt.evaluate({0, 0, 0}), 1);
    EXPECT_EQ(mt.evaluate({0, 1, 0}), 0);
}

TEST_F(MTBDDClassTest, FromZddSingleton) {
    ZDD f = ZDD::singleton(1);  // {{1}}
    auto mt = MTZDD<int>::from_zdd(f, 0, 1);

    // {1} is in the family → v1=1 gives 1
    EXPECT_EQ(mt.evaluate({0, 1}), 1);
    // {} is not in the family → v1=0 gives 0
    EXPECT_EQ(mt.evaluate({0, 0}), 0);
}

TEST_F(MTBDDClassTest, FromZddCustomTerminals) {
    ZDD f = ZDD::singleton(1);
    auto mt = MTZDD<double>::from_zdd(f, 0.0, 5.0);

    EXPECT_DOUBLE_EQ(mt.evaluate({0, 1}), 5.0);
    EXPECT_DOUBLE_EQ(mt.evaluate({0, 0}), 0.0);
}

// ==========================================================================
// GC tests
// ==========================================================================

TEST_F(MTBDDClassTest, GCProtection) {
    // MTBDD nodes should survive GC
    auto lo = MTBDD<int>::terminal(10);
    auto hi = MTBDD<int>::terminal(20);
    auto f = MTBDD<int>::ite(1, hi, lo);
    bddp before = f.id();

    bddgc();

    EXPECT_EQ(f.id(), before);
    EXPECT_EQ(f.top(), 1u);
}

// ==========================================================================
// Fix verification tests
// ==========================================================================

// --- Fix 1: Non-commutative apply (subtraction) ---

TEST_F(MTBDDClassTest, SubtractNonCommutative) {
    // x = (v1 ? 7 : 0), y = (v1 ? 100 : 0)
    auto x = MTBDD<int>::ite(1,
        MTBDD<int>::terminal(7), MTBDD<int>::terminal(0));
    auto y = MTBDD<int>::ite(1,
        MTBDD<int>::terminal(100), MTBDD<int>::terminal(0));

    auto x_minus_y = x - y;
    auto y_minus_x = y - x;

    // v1=1: x-y = 7-100 = -93, y-x = 100-7 = 93
    EXPECT_EQ(x_minus_y.evaluate({0, 1, 0}), -93);
    EXPECT_EQ(y_minus_x.evaluate({0, 1, 0}), 93);

    // v1=0: x-y = 0, y-x = 0
    EXPECT_EQ(x_minus_y.evaluate({0, 0, 0}), 0);
    EXPECT_EQ(y_minus_x.evaluate({0, 0, 0}), 0);
}

TEST_F(MTBDDClassTest, MTZDDSubtractNonCommutative) {
    auto x = MTZDD<int>::ite(1,
        MTZDD<int>::terminal(7), MTZDD<int>::terminal(3));
    auto y = MTZDD<int>::ite(1,
        MTZDD<int>::terminal(100), MTZDD<int>::terminal(50));

    auto x_minus_y = x - y;
    auto y_minus_x = y - x;

    // v1=1: x-y = 7-100 = -93, y-x = 100-7 = 93
    EXPECT_EQ(x_minus_y.evaluate({0, 1}), -93);
    EXPECT_EQ(y_minus_x.evaluate({0, 1}), 93);

    // v1=0: x-y = 3-50 = -47, y-x = 50-3 = 47
    EXPECT_EQ(x_minus_y.evaluate({0, 0}), -47);
    EXPECT_EQ(y_minus_x.evaluate({0, 0}), 47);
}

// --- Fix 2: mtzdd_from_zdd_rec complement handling ---

TEST_F(MTBDDClassTest, FromZddComplement) {
    // ZDD complement toggles ∅ membership: ~F = F ⊕ {∅}
    // F = {{2}}, ∅ ∉ F, so ~F = {{}, {2}}
    ZDD f = ~ZDD::singleton(2);
    auto mt = MTZDD<int>::from_zdd(f, 0, 1);

    for (int v1 = 0; v1 <= 1; v1++) {
        for (int v2 = 0; v2 <= 1; v2++) {
            // ~{{2}} = {{}, {2}} (∅ toggled)
            int expected;
            if (v1 == 0 && v2 == 0) expected = 1;  // {} ∈ ~F
            else if (v1 == 0 && v2 == 1) expected = 1;  // {2} ∈ ~F
            else expected = 0;  // {1}, {1,2} ∉ ~F

            std::vector<int> a = {0, v1, v2, 0};
            EXPECT_EQ(mt.evaluate(a), expected)
                << "v1=" << v1 << " v2=" << v2;
        }
    }
}

TEST_F(MTBDDClassTest, FromZddComplementUnion) {
    // ~(ZDD::singleton(1) + ZDD::singleton(2)) = complement of {{1},{2}}
    ZDD f = ~(ZDD::singleton(1) + ZDD::singleton(2));
    auto mt = MTZDD<int>::from_zdd(f, 0, 1);

    // Check all 2-variable assignments
    // Original: {{1},{2}}. Complement toggles {}: F' = {{},{1},{2}} \ {{1},{2}} ∪ ({} if {} not in F)
    // Actually ZDD complement toggles ∅ membership: F' = F ⊕ {∅}
    // F = {{1},{2}} (does not contain ∅), so F' = {{1},{2},{}} = {{},{1},{2}}
    // Wait, that's F ∪ {∅}. Complement toggles: F' = F △ {∅}.
    // If ∅ ∉ F: F' = F ∪ {∅}  => {{},{1},{2}}
    // If ∅ ∈ F: F' = F \ {∅}
    EXPECT_EQ(mt.evaluate({0, 0, 0, 0}), 1);  // {} in F' → 1
    EXPECT_EQ(mt.evaluate({0, 1, 0, 0}), 1);  // {1} in F' → 1
    EXPECT_EQ(mt.evaluate({0, 0, 1, 0}), 1);  // {2} in F' → 1
    EXPECT_EQ(mt.evaluate({0, 1, 1, 0}), 0);  // {1,2} not in F' → 0
}

// --- Fix 3: mtzdd_ite_rec condition cofactoring (ZDD semantics) ---

TEST_F(MTBDDClassTest, MTZDDIteConditionZeroSuppressed) {
    // Condition at v1 only; v2 is zero-suppressed above v1 in cond.
    // cond = (v1 ? 5 : 3) — non-zero for both v1 branches
    // then = (v2 ? 100 : 200), else = (v2 ? 300 : 400)
    auto cond = MTZDD<int>::ite(1,
        MTZDD<int>::terminal(5), MTZDD<int>::terminal(3));
    auto then_val = MTZDD<int>::ite(2,
        MTZDD<int>::terminal(100), MTZDD<int>::terminal(200));
    auto else_val = MTZDD<int>::ite(2,
        MTZDD<int>::terminal(300), MTZDD<int>::terminal(400));

    auto result = cond.ite(then_val, else_val);

    // With ZDD cofactoring for condition:
    // At v2 (top): cond missing v2 → cond_hi=0 (ZDD), so hi branch uses else
    //   lo = ITE(cond, then_lo=200, else_lo=400) → cond non-zero → 200
    //     (but v1 is then zero-suppressed in result for lo, becoming terminal 200)
    //   hi = ITE(zero, then_hi=100, else_hi=300) → zero cond → 300
    // Result: node(v2, 200, 300)
    // Evaluations consider ALL zero-suppressed variables, including v1:
    EXPECT_EQ(result.evaluate({0, 0, 0, 0}), 200);  // v1=0,v2=0: lo=200, v1 suppressed OK
    EXPECT_EQ(result.evaluate({0, 1, 0, 0}), 0);    // v1=1,v2=0: v1 zero-suppressed → 0
    EXPECT_EQ(result.evaluate({0, 0, 1, 0}), 300);  // v1=0,v2=1: hi=300, v1 suppressed OK
    EXPECT_EQ(result.evaluate({0, 1, 1, 0}), 0);    // v1=1,v2=1: v1 zero-suppressed → 0
}

// --- Fix 4: MTZDD evaluate with variables above root ---

TEST_F(MTBDDClassTest, MTZDDEvaluateAboveRoot) {
    // Root at v1 (level 1), but v2 and v3 exist at higher levels
    auto f = MTZDD<int>::ite(1,
        MTZDD<int>::terminal(5), MTZDD<int>::terminal(7));

    // v1=0, v2=0, v3=0 → 7 (lo branch)
    EXPECT_EQ(f.evaluate({0, 0, 0, 0}), 7);
    // v1=1, v2=0, v3=0 → 5 (hi branch)
    EXPECT_EQ(f.evaluate({0, 1, 0, 0}), 5);
    // v2=1 → 0 (v2 is zero-suppressed above root, assignment=1 → zero)
    EXPECT_EQ(f.evaluate({0, 0, 1, 0}), 0);
    // v3=1 → 0 (v3 is zero-suppressed above root, assignment=1 → zero)
    EXPECT_EQ(f.evaluate({0, 0, 0, 1}), 0);
    // v2=1, v3=1 → 0
    EXPECT_EQ(f.evaluate({0, 0, 1, 1}), 0);
}

TEST_F(MTBDDClassTest, MTZDDEvaluateTerminalWithVars) {
    // Terminal MTZDD with allocated variables
    auto f = MTZDD<int>::terminal(42);

    // Empty set (all vars 0) → 42
    EXPECT_EQ(f.evaluate({0, 0, 0, 0}), 42);
    // Any variable set to 1 → 0 (zero-suppressed)
    EXPECT_EQ(f.evaluate({0, 1, 0, 0}), 0);
    EXPECT_EQ(f.evaluate({0, 0, 1, 0}), 0);
    EXPECT_EQ(f.evaluate({0, 0, 0, 1}), 0);
}

// --- Fix 5: op code reset / bddfinal ---

TEST_F(MTBDDClassTest, BddFinalRejectsNonStandardTerminal) {
    // Terminal index 1 collides with bddtrue (BDD::True), so create two
    // non-zero values to ensure one gets index >= 2.
    {
        auto f = MTBDD<int>::terminal(1);   // index 1 (= bddtrue, can't detect)
        auto g = MTBDD<int>::terminal(42);  // index >= 2 (non-standard)
        EXPECT_THROW(bddfinal(), std::runtime_error);
    }  // f, g destroyed → GC roots removed
    // Clean up: bddfinal should now succeed
    bddfinal();
    bddinit(256, UINT64_MAX);
    for (int i = 0; i < 3; i++) bddnewvar();
}

// --- Fix 6: is_one ---

TEST_F(MTBDDClassTest, IsOneCorrectForMTBDD) {
    auto f42 = MTBDD<int>::terminal(42);
    auto f1  = MTBDD<int>::terminal(1);
    auto f0  = MTBDD<int>::terminal(0);

    EXPECT_FALSE(f42.is_one());  // 42 is not 1
    EXPECT_TRUE(f1.is_one());    // value is 1
    EXPECT_FALSE(f0.is_one());   // 0 is not 1
    EXPECT_TRUE(f0.is_zero());   // 0 is zero
}

TEST_F(MTBDDClassTest, IsOneCorrectForMTZDD) {
    auto f99 = MTZDD<int>::terminal(99);
    auto f1  = MTZDD<int>::terminal(1);
    auto f0  = MTZDD<int>::terminal(0);

    EXPECT_FALSE(f99.is_one());
    EXPECT_TRUE(f1.is_one());
    EXPECT_FALSE(f0.is_one());
    EXPECT_TRUE(f0.is_zero());
}

// ========================================================================
//  MTBDD binary export/import tests
// ========================================================================

TEST_F(MTBDDClassTest, MTBDDExportImportTerminalDouble) {
    auto t42 = MTBDD<double>::terminal(42.5);
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    t42.export_binary(ss);
    ss.seekg(0);
    auto imported = MTBDD<double>::import_binary(ss);
    EXPECT_TRUE(imported.is_terminal());
    EXPECT_DOUBLE_EQ(imported.terminal_value(), 42.5);
}

TEST_F(MTBDDClassTest, MTBDDExportImportTerminalInt) {
    auto t100 = MTBDD<int64_t>::terminal(100);
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    t100.export_binary(ss);
    ss.seekg(0);
    auto imported = MTBDD<int64_t>::import_binary(ss);
    EXPECT_TRUE(imported.is_terminal());
    EXPECT_EQ(imported.terminal_value(), 100);
}

TEST_F(MTBDDClassTest, MTBDDExportImportZeroTerminal) {
    MTBDD<double> zero;
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    zero.export_binary(ss);
    ss.seekg(0);
    auto imported = MTBDD<double>::import_binary(ss);
    EXPECT_TRUE(imported.is_terminal());
    EXPECT_TRUE(imported.is_zero());
    EXPECT_DOUBLE_EQ(imported.terminal_value(), 0.0);
}

TEST_F(MTBDDClassTest, MTBDDExportImportSingleVar) {
    BDD::new_var();
    BDD::new_var();
    // f(x1) = x1 ? 3.0 : 7.0
    auto hi = MTBDD<double>::terminal(3.0);
    auto lo = MTBDD<double>::terminal(7.0);
    auto f = MTBDD<double>::ite(1, hi, lo);

    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    f.export_binary(ss);
    ss.seekg(0);
    auto imported = MTBDD<double>::import_binary(ss);

    EXPECT_DOUBLE_EQ(imported.evaluate({0, 0}), 7.0);
    EXPECT_DOUBLE_EQ(imported.evaluate({0, 1}), 3.0);
}

TEST_F(MTBDDClassTest, MTBDDExportImportMultiVar) {
    BDD::new_var();
    BDD::new_var();
    BDD::new_var();
    // f(x2, x1) = x2 ? (x1 ? 10 : 20) : 30
    // var 2 (level 2) is root, var 1 (level 1) is inner
    auto t10 = MTBDD<int64_t>::terminal(10);
    auto t20 = MTBDD<int64_t>::terminal(20);
    auto t30 = MTBDD<int64_t>::terminal(30);
    auto inner = MTBDD<int64_t>::ite(1, t10, t20);
    auto f = MTBDD<int64_t>::ite(2, inner, t30);

    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    f.export_binary(ss);
    ss.seekg(0);
    auto imported = MTBDD<int64_t>::import_binary(ss);

    EXPECT_EQ(imported.evaluate({0, 0, 0}), 30);
    EXPECT_EQ(imported.evaluate({0, 0, 1}), 20);
    EXPECT_EQ(imported.evaluate({0, 1, 1}), 10);
}

TEST_F(MTBDDClassTest, MTBDDExportImportApplyResult) {
    BDD::new_var();
    BDD::new_var();
    auto a = MTBDD<double>::ite(1, MTBDD<double>::terminal(2.0),
                                   MTBDD<double>::terminal(3.0));
    auto b = MTBDD<double>::ite(1, MTBDD<double>::terminal(10.0),
                                   MTBDD<double>::terminal(20.0));
    auto sum = a + b;

    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    sum.export_binary(ss);
    ss.seekg(0);
    auto imported = MTBDD<double>::import_binary(ss);

    EXPECT_DOUBLE_EQ(imported.evaluate({0, 0}), 23.0);
    EXPECT_DOUBLE_EQ(imported.evaluate({0, 1}), 12.0);
}

// ========================================================================
//  MTZDD binary export/import tests
// ========================================================================

TEST_F(MTBDDClassTest, MTZDDExportImportTerminalDouble) {
    auto t = MTZDD<double>::terminal(3.14);
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    t.export_binary(ss);
    ss.seekg(0);
    auto imported = MTZDD<double>::import_binary(ss);
    EXPECT_TRUE(imported.is_terminal());
    EXPECT_DOUBLE_EQ(imported.terminal_value(), 3.14);
}

TEST_F(MTBDDClassTest, MTZDDExportImportTerminalInt) {
    auto t = MTZDD<int64_t>::terminal(-42);
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    t.export_binary(ss);
    ss.seekg(0);
    auto imported = MTZDD<int64_t>::import_binary(ss);
    EXPECT_TRUE(imported.is_terminal());
    EXPECT_EQ(imported.terminal_value(), -42);
}

TEST_F(MTBDDClassTest, MTZDDExportImportZeroTerminal) {
    MTZDD<double> zero;
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    zero.export_binary(ss);
    ss.seekg(0);
    auto imported = MTZDD<double>::import_binary(ss);
    EXPECT_TRUE(imported.is_terminal());
    EXPECT_TRUE(imported.is_zero());
}

TEST_F(MTBDDClassTest, MTZDDExportImportSingleVar) {
    BDD::new_var();
    BDD::new_var();
    // ZDD-style: variable 1 present → 5.0, absent → 2.0
    auto hi = MTZDD<double>::terminal(5.0);
    auto lo = MTZDD<double>::terminal(2.0);
    auto f = MTZDD<double>::ite(1, hi, lo);

    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    f.export_binary(ss);
    ss.seekg(0);
    auto imported = MTZDD<double>::import_binary(ss);

    EXPECT_DOUBLE_EQ(imported.evaluate({0, 0}), 2.0);
    EXPECT_DOUBLE_EQ(imported.evaluate({0, 1}), 5.0);
}

TEST_F(MTBDDClassTest, MTZDDExportImportMultiVar) {
    BDD::new_var();
    BDD::new_var();
    BDD::new_var();
    // var 2 (level 2) is root, var 1 (level 1) is inner
    auto t1 = MTZDD<int64_t>::terminal(1);
    auto t2 = MTZDD<int64_t>::terminal(2);
    auto t3 = MTZDD<int64_t>::terminal(3);
    auto inner = MTZDD<int64_t>::ite(1, t1, t2);
    auto f = MTZDD<int64_t>::ite(2, inner, t3);

    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    f.export_binary(ss);
    ss.seekg(0);
    auto imported = MTZDD<int64_t>::import_binary(ss);

    EXPECT_EQ(imported.evaluate({0, 0, 0}), 3);
    EXPECT_EQ(imported.evaluate({0, 0, 1}), 2);
    EXPECT_EQ(imported.evaluate({0, 1, 1}), 1);
}

TEST_F(MTBDDClassTest, MTZDDExportImportFromZDD) {
    BDD::new_var();
    BDD::new_var();
    BDD::new_var();
    // Create a ZDD and convert to MTZDD
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    ZDD f = x1 + x2;  // {{1}, {2}}
    auto mf = MTZDD<double>::from_zdd(f, 0.0, 1.0);

    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    mf.export_binary(ss);
    ss.seekg(0);
    auto imported = MTZDD<double>::import_binary(ss);

    // {1} → 1.0, {2} → 1.0, {} → 0.0, {1,2} → 0.0 (ZDD semantics)
    EXPECT_DOUBLE_EQ(imported.evaluate({0, 0, 0}), 0.0);
    EXPECT_DOUBLE_EQ(imported.evaluate({0, 1, 0}), 1.0);
    EXPECT_DOUBLE_EQ(imported.evaluate({0, 0, 1}), 1.0);
    EXPECT_DOUBLE_EQ(imported.evaluate({0, 1, 1}), 0.0);
}

TEST_F(MTBDDClassTest, MTZDDExportImportApplyResult) {
    BDD::new_var();
    BDD::new_var();
    auto a = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(10),
                                    MTZDD<int64_t>::terminal(20));
    auto b = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(3),
                                    MTZDD<int64_t>::terminal(7));
    auto sum = a + b;

    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    sum.export_binary(ss);
    ss.seekg(0);
    auto imported = MTZDD<int64_t>::import_binary(ss);

    EXPECT_EQ(imported.evaluate({0, 0}), 27);
    EXPECT_EQ(imported.evaluate({0, 1}), 13);
}

TEST_F(MTBDDClassTest, MTBDDExportImportNegativeDouble) {
    auto tn = MTBDD<double>::terminal(-99.5);
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    tn.export_binary(ss);
    ss.seekg(0);
    auto imported = MTBDD<double>::import_binary(ss);
    EXPECT_DOUBLE_EQ(imported.terminal_value(), -99.5);
}

TEST_F(MTBDDClassTest, MTZDDExportImportNegativeInt) {
    auto tn = MTZDD<int64_t>::terminal(-1000000);
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    tn.export_binary(ss);
    ss.seekg(0);
    auto imported = MTZDD<int64_t>::import_binary(ss);
    EXPECT_EQ(imported.terminal_value(), -1000000);
}

TEST_F(MTBDDClassTest, MTBDDImportBadMagicThrows) {
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    ss.write("XYZ", 3);
    ss.seekg(0);
    EXPECT_THROW(MTBDD<double>::import_binary(ss), std::runtime_error);
}

TEST_F(MTBDDClassTest, MTZDDImportTruncatedThrows) {
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    ss.write("BD", 2);  // incomplete magic
    ss.seekg(0);
    EXPECT_THROW(MTZDD<double>::import_binary(ss), std::runtime_error);
}

// ========================================================================
//  MTZDD cofactor tests
// ========================================================================

TEST_F(MTBDDClassTest, MTZDDCofactor0Terminal) {
    auto t = MTZDD<double>::terminal(5.0);
    BDD::new_var();
    auto c = t.cofactor0(1);
    EXPECT_TRUE(c.is_terminal());
    EXPECT_DOUBLE_EQ(c.terminal_value(), 5.0);
}

TEST_F(MTBDDClassTest, MTZDDCofactor1Terminal) {
    auto t = MTZDD<double>::terminal(5.0);
    BDD::new_var();
    // Terminal cofactor1 → zero terminal (ZDD: missing var=1 → zero)
    auto c = t.cofactor1(1);
    EXPECT_TRUE(c.is_terminal());
    EXPECT_TRUE(c.is_zero());
}

TEST_F(MTBDDClassTest, MTZDDCofactor0AtTopVar) {
    BDD::new_var();
    auto hi = MTZDD<double>::terminal(10.0);
    auto lo = MTZDD<double>::terminal(20.0);
    auto f = MTZDD<double>::ite(1, hi, lo);

    // cofactor0(1): fix var 1 to 0 → lo branch = 20.0
    auto c0 = f.cofactor0(1);
    EXPECT_TRUE(c0.is_terminal());
    EXPECT_DOUBLE_EQ(c0.terminal_value(), 20.0);
}

TEST_F(MTBDDClassTest, MTZDDCofactor1AtTopVar) {
    BDD::new_var();
    auto hi = MTZDD<double>::terminal(10.0);
    auto lo = MTZDD<double>::terminal(20.0);
    auto f = MTZDD<double>::ite(1, hi, lo);

    // cofactor1(1): fix var 1 to 1 → hi branch = 10.0
    auto c1 = f.cofactor1(1);
    EXPECT_TRUE(c1.is_terminal());
    EXPECT_DOUBLE_EQ(c1.terminal_value(), 10.0);
}

TEST_F(MTBDDClassTest, MTZDDCofactor0ZeroSuppressed) {
    BDD::new_var();
    BDD::new_var();
    // f only uses var 1 (level 1), var 2 (level 2) is zero-suppressed
    auto hi = MTZDD<int64_t>::terminal(10);
    auto lo = MTZDD<int64_t>::terminal(20);
    auto f = MTZDD<int64_t>::ite(1, hi, lo);

    // cofactor0(2): var 2 is above f's top → v=0 is default → return f
    auto c0 = f.cofactor0(2);
    EXPECT_EQ(c0, f);
}

TEST_F(MTBDDClassTest, MTZDDCofactor1ZeroSuppressed) {
    BDD::new_var();
    BDD::new_var();
    auto hi = MTZDD<int64_t>::terminal(10);
    auto lo = MTZDD<int64_t>::terminal(20);
    auto f = MTZDD<int64_t>::ite(1, hi, lo);

    // cofactor1(2): var 2 is zero-suppressed → zero
    auto c1 = f.cofactor1(2);
    EXPECT_TRUE(c1.is_terminal());
    EXPECT_TRUE(c1.is_zero());
}

TEST_F(MTBDDClassTest, MTZDDCofactorMultiLevel) {
    BDD::new_var();
    BDD::new_var();
    BDD::new_var();
    // f: var 3 (top) → var 1 → terminals
    //   var3=1: (var1=1: 100, var1=0: 200)
    //   var3=0: 300
    auto t100 = MTZDD<int64_t>::terminal(100);
    auto t200 = MTZDD<int64_t>::terminal(200);
    auto t300 = MTZDD<int64_t>::terminal(300);
    auto inner = MTZDD<int64_t>::ite(1, t100, t200);
    auto f = MTZDD<int64_t>::ite(3, inner, t300);

    // cofactor0(3): fix var 3 to 0 → lo branch = 300
    auto c0_3 = f.cofactor0(3);
    EXPECT_TRUE(c0_3.is_terminal());
    EXPECT_EQ(c0_3.terminal_value(), 300);

    // cofactor1(3): fix var 3 to 1 → hi branch = inner
    auto c1_3 = f.cofactor1(3);
    EXPECT_EQ(c1_3.evaluate({0, 0, 0, 0}), 200);
    EXPECT_EQ(c1_3.evaluate({0, 1, 0, 0}), 100);

    // cofactor0(1): fix var 1 to 0 in f → recurse into var 3 node
    // var3=1: inner.cofactor0(1) = 200, var3=0: 300
    auto c0_1 = f.cofactor0(1);
    EXPECT_EQ(c0_1.evaluate({0, 0, 0, 0}), 300);
    EXPECT_EQ(c0_1.evaluate({0, 0, 0, 1}), 200);

    // cofactor1(1): fix var 1 to 1 in f
    // var3=1: inner.cofactor1(1) = 100, var3=0: zero (300.cofactor1(1)=0)
    auto c1_1 = f.cofactor1(1);
    EXPECT_EQ(c1_1.evaluate({0, 0, 0, 0}), 0);
    EXPECT_EQ(c1_1.evaluate({0, 0, 0, 1}), 100);

    // cofactor on var 2 (zero-suppressed in f)
    auto c0_2 = f.cofactor0(2);
    EXPECT_EQ(c0_2, f);  // v=0 is default for missing var
    auto c1_2 = f.cofactor1(2);
    EXPECT_TRUE(c1_2.is_zero());  // missing var=1 → zero
}

TEST_F(MTBDDClassTest, MTZDDCofactorFromZDD) {
    BDD::new_var();
    BDD::new_var();
    BDD::new_var();
    // ZDD: {{1}, {2}, {1,2}}
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    ZDD f = x1 + x2 + (x1 * x2);
    auto mf = MTZDD<double>::from_zdd(f, 0.0, 1.0);

    // cofactor1(1): sets containing var 1 → {{}, {2}} = 1.0 where x1 removed
    auto c1 = mf.cofactor1(1);
    EXPECT_DOUBLE_EQ(c1.evaluate({0, 0, 0}), 1.0);  // {} → 1
    EXPECT_DOUBLE_EQ(c1.evaluate({0, 0, 1}), 1.0);  // {2} → 1

    // cofactor0(1): sets not containing var 1 → {{2}}
    auto c0 = mf.cofactor0(1);
    EXPECT_DOUBLE_EQ(c0.evaluate({0, 0, 0}), 0.0);  // {} → 0
    EXPECT_DOUBLE_EQ(c0.evaluate({0, 0, 1}), 1.0);  // {2} → 1
}

TEST_F(MTBDDClassTest, MTZDDCofactorVarOutOfRange) {
    auto t = MTZDD<double>::terminal(1.0);
    EXPECT_THROW(t.cofactor0(0), std::invalid_argument);
    EXPECT_THROW(t.cofactor1(0), std::invalid_argument);
}

TEST_F(MTBDDClassTest, MTZDDCofactorZeroTerminal) {
    MTZDD<double> zero;
    BDD::new_var();
    auto c0 = zero.cofactor0(1);
    EXPECT_TRUE(c0.is_zero());
    auto c1 = zero.cofactor1(1);
    EXPECT_TRUE(c1.is_zero());
}

// ========================================================================
//  MTZDD count / exact_count tests
// ========================================================================

TEST_F(MTBDDClassTest, MTZDDCountZeroTerminal) {
    MTZDD<double> zero;
    EXPECT_DOUBLE_EQ(zero.count(), 0.0);
    EXPECT_EQ(zero.exact_count(), bigint::BigInt(0));
}

TEST_F(MTBDDClassTest, MTZDDCountNonZeroTerminal) {
    auto t = MTZDD<double>::terminal(5.0);
    EXPECT_DOUBLE_EQ(t.count(), 1.0);
    EXPECT_EQ(t.exact_count(), bigint::BigInt(1));
}

TEST_F(MTBDDClassTest, MTZDDCountSingleVar) {
    // var 1: hi=3.0, lo=zero → 1 non-zero path ({1} -> 3.0)
    BDD::new_var();
    auto t3 = MTZDD<double>::terminal(3.0);
    MTZDD<double> zero;
    auto f = MTZDD<double>::ite(1, t3, zero);
    EXPECT_DOUBLE_EQ(f.count(), 1.0);
    EXPECT_EQ(f.exact_count(), bigint::BigInt(1));
}

TEST_F(MTBDDClassTest, MTZDDCountTwoPaths) {
    // var 1: hi=3.0, lo=2.0 → 2 non-zero paths
    BDD::new_var();
    auto t3 = MTZDD<double>::terminal(3.0);
    auto t2 = MTZDD<double>::terminal(2.0);
    auto f = MTZDD<double>::ite(1, t3, t2);
    EXPECT_DOUBLE_EQ(f.count(), 2.0);
    EXPECT_EQ(f.exact_count(), bigint::BigInt(2));
}

TEST_F(MTBDDClassTest, MTZDDCountMultipleVars) {
    // Build MTZDD with 2 variables (var 2 = level 2 = top):
    //   var 2: hi=t5, lo=var1_node
    //   var 1: hi=t3, lo=zero
    // Paths: {2} -> 5.0, {1} -> 3.0 → 2 non-zero paths
    BDD::new_var();  // var 1, level 1
    BDD::new_var();  // var 2, level 2
    auto t5 = MTZDD<double>::terminal(5.0);
    auto t3 = MTZDD<double>::terminal(3.0);
    MTZDD<double> zero;
    auto v1_node = MTZDD<double>::ite(1, t3, zero);
    auto f = MTZDD<double>::ite(2, t5, v1_node);
    EXPECT_DOUBLE_EQ(f.count(), 2.0);
    EXPECT_EQ(f.exact_count(), bigint::BigInt(2));
}

TEST_F(MTBDDClassTest, MTZDDCountFullTree) {
    // Full binary tree with 2 variables (var 2 = top), all non-zero terminals:
    //   var 2: hi=var1_hi, lo=var1_lo
    //   var1_lo: hi=t2, lo=t1
    //   var1_hi: hi=t4, lo=t3
    // 4 non-zero paths: {1,2}->4, {2}->3, {1}->2, {}->1
    BDD::new_var();  // var 1, level 1
    BDD::new_var();  // var 2, level 2
    auto t1 = MTZDD<double>::terminal(1.0);
    auto t2 = MTZDD<double>::terminal(2.0);
    auto t3 = MTZDD<double>::terminal(3.0);
    auto t4 = MTZDD<double>::terminal(4.0);
    auto v1_lo = MTZDD<double>::ite(1, t2, t1);
    auto v1_hi = MTZDD<double>::ite(1, t4, t3);
    auto f = MTZDD<double>::ite(2, v1_hi, v1_lo);
    EXPECT_DOUBLE_EQ(f.count(), 4.0);
    EXPECT_EQ(f.exact_count(), bigint::BigInt(4));
}

TEST_F(MTBDDClassTest, MTZDDCountMixedZeroNonZero) {
    // var 2 (top): hi=var1_node, lo=zero
    // var 1: hi=t3, lo=zero
    // Only 1 non-zero path: {1,2} -> 3
    BDD::new_var();  // var 1
    BDD::new_var();  // var 2
    auto t3 = MTZDD<double>::terminal(3.0);
    MTZDD<double> zero;
    auto v1_node = MTZDD<double>::ite(1, t3, zero);
    auto f = MTZDD<double>::ite(2, v1_node, zero);
    EXPECT_DOUBLE_EQ(f.count(), 1.0);
    EXPECT_EQ(f.exact_count(), bigint::BigInt(1));
}

TEST_F(MTBDDClassTest, MTZDDCountInt) {
    // Test with int64_t type
    BDD::new_var();
    auto t10 = MTZDD<int64_t>::terminal(10);
    auto t20 = MTZDD<int64_t>::terminal(20);
    MTZDD<int64_t> zero;
    auto f = MTZDD<int64_t>::ite(1, t10, t20);
    EXPECT_DOUBLE_EQ(f.count(), 2.0);
    EXPECT_EQ(f.exact_count(), bigint::BigInt(2));

    auto g = MTZDD<int64_t>::ite(1, t10, zero);
    EXPECT_DOUBLE_EQ(g.count(), 1.0);
    EXPECT_EQ(g.exact_count(), bigint::BigInt(1));
}

// ========================================================================
//  MTZDD count(terminal) / exact_count(terminal) tests
// ========================================================================

TEST_F(MTBDDClassTest, MTZDDCountForTerminalBasic) {
    // var 1: hi=3.0, lo=2.0 → 2 non-zero paths
    BDD::new_var();
    auto t3 = MTZDD<double>::terminal(3.0);
    auto t2 = MTZDD<double>::terminal(2.0);
    auto f = MTZDD<double>::ite(1, t3, t2);
    EXPECT_DOUBLE_EQ(f.count(3.0), 1.0);
    EXPECT_DOUBLE_EQ(f.count(2.0), 1.0);
    EXPECT_DOUBLE_EQ(f.count(0.0), 0.0);   // zero terminal not reachable
    EXPECT_DOUBLE_EQ(f.count(99.0), 0.0);  // non-existent value
    EXPECT_EQ(f.exact_count(3.0), bigint::BigInt(1));
    EXPECT_EQ(f.exact_count(2.0), bigint::BigInt(1));
    EXPECT_EQ(f.exact_count(99.0), bigint::BigInt(0));
}

TEST_F(MTBDDClassTest, MTZDDCountForTerminalShared) {
    // Two paths lead to the same terminal value 5.0:
    //   var 2 (top): hi=t5, lo=v1
    //   var 1:       hi=t5, lo=zero
    // Paths: {2}->5.0, {1}->5.0 → count(5.0) == 2
    BDD::new_var();  // var 1
    BDD::new_var();  // var 2
    auto t5 = MTZDD<double>::terminal(5.0);
    MTZDD<double> zero;
    auto v1 = MTZDD<double>::ite(1, t5, zero);
    auto f = MTZDD<double>::ite(2, t5, v1);
    EXPECT_DOUBLE_EQ(f.count(5.0), 2.0);
    EXPECT_DOUBLE_EQ(f.count(0.0), 1.0);  // path: {} → zero
    EXPECT_EQ(f.exact_count(5.0), bigint::BigInt(2));
}

TEST_F(MTBDDClassTest, MTZDDCountForTerminalZero) {
    // Zero terminal itself: count(0.0) on a zero terminal → 1
    MTZDD<double> zero;
    EXPECT_DOUBLE_EQ(zero.count(0.0), 1.0);
    EXPECT_DOUBLE_EQ(zero.count(3.0), 0.0);
    EXPECT_EQ(zero.exact_count(0.0), bigint::BigInt(1));

    // Non-zero terminal: count(5.0) → 1, count(0.0) → 0
    auto t5 = MTZDD<double>::terminal(5.0);
    EXPECT_DOUBLE_EQ(t5.count(5.0), 1.0);
    EXPECT_DOUBLE_EQ(t5.count(0.0), 0.0);
}

TEST_F(MTBDDClassTest, MTZDDCountForTerminalInt) {
    BDD::new_var();
    auto t10 = MTZDD<int64_t>::terminal(10);
    auto t20 = MTZDD<int64_t>::terminal(20);
    auto f = MTZDD<int64_t>::ite(1, t10, t20);
    EXPECT_DOUBLE_EQ(f.count(int64_t(10)), 1.0);
    EXPECT_DOUBLE_EQ(f.count(int64_t(20)), 1.0);
    EXPECT_DOUBLE_EQ(f.count(int64_t(99)), 0.0);
    EXPECT_EQ(f.exact_count(int64_t(10)), bigint::BigInt(1));
    EXPECT_EQ(f.exact_count(int64_t(20)), bigint::BigInt(1));
    EXPECT_EQ(f.exact_count(int64_t(99)), bigint::BigInt(0));
}

// ========================================================================
//  MTZDD has_empty tests
// ========================================================================

TEST_F(MTBDDClassTest, MTZDDHasEmptyZeroTerminal) {
    // Zero terminal: empty assignment maps to 0 → false
    MTZDD<double> f;
    EXPECT_FALSE(f.has_empty());
}

TEST_F(MTBDDClassTest, MTZDDHasEmptyNonZeroTerminal) {
    // Non-zero terminal: empty assignment maps to 5.0 → true
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_TRUE(f.has_empty());
}

TEST_F(MTBDDClassTest, MTZDDHasEmptySingleVar) {
    // ite(v1, hi=3.0, lo=0.0)
    // Empty assignment: all vars=0 → follow lo → 0.0 → false
    auto f = MTZDD<double>::ite(1, MTZDD<double>::terminal(3.0), MTZDD<double>());
    EXPECT_FALSE(f.has_empty());
}

TEST_F(MTBDDClassTest, MTZDDHasEmptySingleVarNonZeroLo) {
    // ite(v1, hi=3.0, lo=2.0)
    // Empty assignment: all vars=0 → follow lo → 2.0 → true
    auto f = MTZDD<double>::ite(1, MTZDD<double>::terminal(3.0), MTZDD<double>::terminal(2.0));
    EXPECT_TRUE(f.has_empty());
}

TEST_F(MTBDDClassTest, MTZDDHasEmptyMultiLevel) {
    // v3 top, v1 below (v2 zero-suppressed between)
    // ite(v3, hi=10, lo=ite(v1, hi=5, lo=0))
    // Empty: v3=0 → lo → v1=0 → lo → 0 → false
    auto inner = MTZDD<double>::ite(1, MTZDD<double>::terminal(5.0), MTZDD<double>());
    auto f = MTZDD<double>::ite(3, MTZDD<double>::terminal(10.0), inner);
    EXPECT_FALSE(f.has_empty());
}

TEST_F(MTBDDClassTest, MTZDDHasEmptyMultiLevelNonZero) {
    // v3 top, v1 below (v2 zero-suppressed between)
    // ite(v3, hi=10, lo=ite(v1, hi=5, lo=7))
    // Empty: v3=0 → lo → v1=0 → lo → 7 → true
    auto inner = MTZDD<double>::ite(1, MTZDD<double>::terminal(5.0), MTZDD<double>::terminal(7.0));
    auto f = MTZDD<double>::ite(3, MTZDD<double>::terminal(10.0), inner);
    EXPECT_TRUE(f.has_empty());
}

TEST_F(MTBDDClassTest, MTZDDHasEmptyInt) {
    // Test with int64_t type
    auto zero = MTZDD<int64_t>();
    auto t5 = MTZDD<int64_t>::terminal(5);
    auto t3 = MTZDD<int64_t>::terminal(3);

    // lo=0 → false
    auto f1 = MTZDD<int64_t>::ite(1, t5, zero);
    EXPECT_FALSE(f1.has_empty());

    // lo=3 → true
    auto f2 = MTZDD<int64_t>::ite(1, t5, t3);
    EXPECT_TRUE(f2.has_empty());
}

// ========================================================================
//  MTZDD support_vars tests
// ========================================================================

TEST_F(MTBDDClassTest, MTZDDSupportVarsZeroTerminal) {
    MTZDD<double> f;
    EXPECT_TRUE(f.support_vars().empty());
}

TEST_F(MTBDDClassTest, MTZDDSupportVarsNonZeroTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_TRUE(f.support_vars().empty());
}

TEST_F(MTBDDClassTest, MTZDDSupportVarsSingleVar) {
    auto f = MTZDD<double>::ite(1, MTZDD<double>::terminal(3.0), MTZDD<double>());
    auto vars = f.support_vars();
    ASSERT_EQ(vars.size(), 1u);
    EXPECT_EQ(vars[0], 1u);
}

TEST_F(MTBDDClassTest, MTZDDSupportVarsMultiVar) {
    // v3 top, v1 bottom (v2 zero-suppressed)
    auto inner = MTZDD<double>::ite(1, MTZDD<double>::terminal(5.0), MTZDD<double>());
    auto f = MTZDD<double>::ite(3, MTZDD<double>::terminal(10.0), inner);
    auto vars = f.support_vars();
    ASSERT_EQ(vars.size(), 2u);
    // Sorted by level (highest first): v3, v1
    EXPECT_EQ(vars[0], 3u);
    EXPECT_EQ(vars[1], 1u);
}

// ========================================================================
//  MTZDD enumerate tests
// ========================================================================

TEST_F(MTBDDClassTest, MTZDDEnumerateZeroTerminal) {
    MTZDD<double> f;
    auto paths = f.enumerate();
    EXPECT_TRUE(paths.empty());
}

TEST_F(MTBDDClassTest, MTZDDEnumerateNonZeroTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    auto paths = f.enumerate();
    ASSERT_EQ(paths.size(), 1u);
    EXPECT_TRUE(paths[0].first.empty());
    EXPECT_DOUBLE_EQ(paths[0].second, 5.0);
}

TEST_F(MTBDDClassTest, MTZDDEnumerateSingleVar) {
    // ite(v1, hi=3.0, lo=0.0): only hi path is non-zero
    auto f = MTZDD<double>::ite(1, MTZDD<double>::terminal(3.0), MTZDD<double>());
    auto paths = f.enumerate();
    ASSERT_EQ(paths.size(), 1u);
    ASSERT_EQ(paths[0].first.size(), 1u);
    EXPECT_EQ(paths[0].first[0], 1u);
    EXPECT_DOUBLE_EQ(paths[0].second, 3.0);
}

TEST_F(MTBDDClassTest, MTZDDEnumerateTwoPaths) {
    // ite(v1, hi=3.0, lo=2.0): both paths non-zero
    auto f = MTZDD<double>::ite(1, MTZDD<double>::terminal(3.0),
                                   MTZDD<double>::terminal(2.0));
    auto paths = f.enumerate();
    ASSERT_EQ(paths.size(), 2u);
    // lo path first (empty set -> 2.0), then hi path ({1} -> 3.0)
    EXPECT_TRUE(paths[0].first.empty());
    EXPECT_DOUBLE_EQ(paths[0].second, 2.0);
    ASSERT_EQ(paths[1].first.size(), 1u);
    EXPECT_EQ(paths[1].first[0], 1u);
    EXPECT_DOUBLE_EQ(paths[1].second, 3.0);
}

TEST_F(MTBDDClassTest, MTZDDEnumerateMultiVarSorted) {
    // v3 top, v1 bottom: path {v1, v3} should be sorted as {1, 3}
    auto inner = MTZDD<double>::ite(1, MTZDD<double>::terminal(5.0), MTZDD<double>());
    auto f = MTZDD<double>::ite(3, MTZDD<double>::terminal(10.0), inner);
    auto paths = f.enumerate();
    ASSERT_EQ(paths.size(), 2u);
    // lo of v3 -> inner -> hi of v1 -> {1} -> 5.0
    ASSERT_EQ(paths[0].first.size(), 1u);
    EXPECT_EQ(paths[0].first[0], 1u);
    EXPECT_DOUBLE_EQ(paths[0].second, 5.0);
    // hi of v3 -> {3} -> 10.0
    ASSERT_EQ(paths[1].first.size(), 1u);
    EXPECT_EQ(paths[1].first[0], 3u);
    EXPECT_DOUBLE_EQ(paths[1].second, 10.0);
}

TEST_F(MTBDDClassTest, MTZDDEnumerateInt) {
    auto f = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(7),
                                    MTZDD<int64_t>::terminal(3));
    auto paths = f.enumerate();
    ASSERT_EQ(paths.size(), 2u);
    EXPECT_EQ(paths[0].second, 3);
    EXPECT_EQ(paths[1].second, 7);
}

// ========================================================================
//  MTZDD to_str / print_sets tests
// ========================================================================

TEST_F(MTBDDClassTest, MTZDDToStrZeroTerminal) {
    MTZDD<double> f;
    EXPECT_EQ(f.to_str(), "");
}

TEST_F(MTBDDClassTest, MTZDDToStrNonZeroTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_EQ(f.to_str(), "{} -> 5");
}

TEST_F(MTBDDClassTest, MTZDDToStrSingleVar) {
    // ite(v1, hi=3, lo=0): only {1} -> 3
    auto f = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(3),
                                    MTZDD<int64_t>());
    EXPECT_EQ(f.to_str(), "{1} -> 3");
}

TEST_F(MTBDDClassTest, MTZDDToStrTwoPaths) {
    // ite(v1, hi=3, lo=2): {} -> 2, {1} -> 3
    auto f = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(3),
                                    MTZDD<int64_t>::terminal(2));
    EXPECT_EQ(f.to_str(), "{} -> 2, {1} -> 3");
}

TEST_F(MTBDDClassTest, MTZDDToStrMultiVar) {
    // v3 top, v1 bottom
    // lo of v3 -> ite(v1, 5, 0) -> {1} -> 5
    // hi of v3 -> 10 -> {3} -> 10
    auto inner = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(5),
                                        MTZDD<int64_t>());
    auto f = MTZDD<int64_t>::ite(3, MTZDD<int64_t>::terminal(10), inner);
    EXPECT_EQ(f.to_str(), "{1} -> 5, {3} -> 10");
}

TEST_F(MTBDDClassTest, MTZDDPrintSetsMatchesToStr) {
    auto f = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(7),
                                    MTZDD<int64_t>::terminal(3));
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), f.to_str());
}

TEST_F(MTBDDClassTest, MTZDDPrintSetsVarNameMap) {
    auto f = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(7),
                                    MTZDD<int64_t>::terminal(3));
    std::vector<std::string> names = {"unused", "x"};
    std::ostringstream oss;
    f.print_sets(oss, names);
    EXPECT_EQ(oss.str(), "{} -> 3, {x} -> 7");
}

TEST_F(MTBDDClassTest, MTZDDPrintSetsVarNameMapPartial) {
    // var_name_map only covers v1, v2 falls back to number
    auto inner = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(5),
                                        MTZDD<int64_t>());
    auto f = MTZDD<int64_t>::ite(3, MTZDD<int64_t>::terminal(10), inner);
    std::vector<std::string> names = {"", "a"};  // only v1 has a name
    std::ostringstream oss;
    f.print_sets(oss, names);
    EXPECT_EQ(oss.str(), "{a} -> 5, {3} -> 10");
}

// --- MTZDD threshold / to_zdd tests ---

TEST_F(MTBDDClassTest, MTZDDThresholdGtTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_EQ(f.threshold_gt(3.0), ZDD::Single);
    EXPECT_EQ(f.threshold_gt(5.0), ZDD::Empty);
    EXPECT_EQ(f.threshold_gt(6.0), ZDD::Empty);
}

TEST_F(MTBDDClassTest, MTZDDThresholdGeTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_EQ(f.threshold_ge(5.0), ZDD::Single);
    EXPECT_EQ(f.threshold_ge(5.1), ZDD::Empty);
}

TEST_F(MTBDDClassTest, MTZDDThresholdEqTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_EQ(f.threshold_eq(5.0), ZDD::Single);
    EXPECT_EQ(f.threshold_eq(4.0), ZDD::Empty);
}

TEST_F(MTBDDClassTest, MTZDDThresholdLtTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_EQ(f.threshold_lt(6.0), ZDD::Single);
    EXPECT_EQ(f.threshold_lt(5.0), ZDD::Empty);
}

TEST_F(MTBDDClassTest, MTZDDThresholdLeTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_EQ(f.threshold_le(5.0), ZDD::Single);
    EXPECT_EQ(f.threshold_le(4.9), ZDD::Empty);
}

TEST_F(MTBDDClassTest, MTZDDThresholdNeTerminal) {
    auto f = MTZDD<double>::terminal(5.0);
    EXPECT_EQ(f.threshold_ne(5.0), ZDD::Empty);
    EXPECT_EQ(f.threshold_ne(0.0), ZDD::Single);
}

TEST_F(MTBDDClassTest, MTZDDToZddTerminal) {
    auto zero = MTZDD<double>::terminal(0.0);
    EXPECT_EQ(zero.to_zdd(), ZDD::Empty);
    auto nonzero = MTZDD<double>::terminal(3.0);
    EXPECT_EQ(nonzero.to_zdd(), ZDD::Single);
}

TEST_F(MTBDDClassTest, MTZDDThresholdSingleVar) {
    // ite(1, hi=3.0, lo=0.0) → ZDD zero-suppression: lo=0 terminal removed
    // Result MTZDD: {1}->3.0 only (lo = zero terminal, suppressed)
    auto f = MTZDD<double>::ite(1, MTZDD<double>::terminal(3.0),
                                MTZDD<double>::terminal(0.0));
    // threshold_gt(2.0): {1}->3.0 passes → ZDD {{1}}
    auto z1 = f.threshold_gt(2.0);
    EXPECT_EQ(z1.count(), 1.0);
    auto sets1 = z1.enumerate();
    ASSERT_EQ(sets1.size(), 1u);
    EXPECT_EQ(sets1[0], std::vector<bddvar>({1}));

    // threshold_eq(0.0): {}->0.0 path exists (node not suppressed since hi!=0)
    auto z_eq0 = f.threshold_eq(0.0);
    EXPECT_EQ(z_eq0, ZDD::Single);  // {{}} = family containing empty set

    // to_zdd(): same as threshold_gt(0) here
    auto z2 = f.to_zdd();
    EXPECT_EQ(z2.count(), 1.0);
}

TEST_F(MTBDDClassTest, MTZDDThresholdSingleVarNonZeroLo) {
    // ite(1, hi=3.0, lo=1.0) → both branches non-zero
    // Paths: {}->1.0, {1}->3.0
    auto f = MTZDD<double>::ite(1, MTZDD<double>::terminal(3.0),
                                MTZDD<double>::terminal(1.0));
    // threshold_gt(2.0): only {1}->3.0 passes
    auto z1 = f.threshold_gt(2.0);
    EXPECT_EQ(z1.count(), 1.0);
    auto sets1 = z1.enumerate();
    ASSERT_EQ(sets1.size(), 1u);
    EXPECT_EQ(sets1[0], std::vector<bddvar>({1}));

    // threshold_ge(1.0): both paths pass
    auto z2 = f.threshold_ge(1.0);
    EXPECT_EQ(z2.count(), 2.0);

    // threshold_eq(1.0): only {}->1.0
    auto z3 = f.threshold_eq(1.0);
    EXPECT_EQ(z3.count(), 1.0);
    EXPECT_TRUE(z3.has_empty());
}

TEST_F(MTBDDClassTest, MTZDDThresholdMultiPath) {
    // Build MTZDD with: {}->1, {1}->3, {2}->5, {1,2}->7
    auto t1 = MTZDD<double>::terminal(1.0);
    auto t3 = MTZDD<double>::terminal(3.0);
    auto t5 = MTZDD<double>::terminal(5.0);
    auto t7 = MTZDD<double>::terminal(7.0);
    auto lo = MTZDD<double>::ite(1, t3, t1);    // v1: hi=3, lo=1
    auto hi = MTZDD<double>::ite(1, t7, t5);    // v1: hi=7, lo=5
    auto f = MTZDD<double>::ite(2, hi, lo);     // v2: hi=hi, lo=lo

    // Verify structure via enumerate
    auto paths = f.enumerate();
    ASSERT_EQ(paths.size(), 4u);

    // threshold_gt(4.0): {2}->5, {1,2}->7
    auto z1 = f.threshold_gt(4.0);
    EXPECT_EQ(z1.count(), 2.0);
    auto sets1 = z1.enumerate();
    ASSERT_EQ(sets1.size(), 2u);

    // threshold_ge(3.0): {1}->3, {2}->5, {1,2}->7
    auto z2 = f.threshold_ge(3.0);
    EXPECT_EQ(z2.count(), 3.0);

    // threshold_lt(5.0): {}->1, {1}->3
    auto z3 = f.threshold_lt(5.0);
    EXPECT_EQ(z3.count(), 2.0);
    EXPECT_TRUE(z3.has_empty());

    // threshold_le(3.0): {}->1, {1}->3
    auto z4 = f.threshold_le(3.0);
    EXPECT_EQ(z4.count(), 2.0);

    // threshold_eq(5.0): {2}->5 only
    auto z5 = f.threshold_eq(5.0);
    EXPECT_EQ(z5.count(), 1.0);

    // threshold_ne(1.0): {1}->3, {2}->5, {1,2}->7
    auto z6 = f.threshold_ne(1.0);
    EXPECT_EQ(z6.count(), 3.0);

    // to_zdd(): all 4 paths are non-zero
    auto z7 = f.to_zdd();
    EXPECT_EQ(z7.count(), 4.0);
}

TEST_F(MTBDDClassTest, MTZDDToZddRoundTrip) {
    // ZDD → MTZDD → to_zdd() → should equal original ZDD
    auto z1 = ZDD::singleton(1);
    auto z2 = ZDD::singleton(2);
    auto z = z1 + z2;  // {{1}, {2}}
    auto mtzdd = MTZDD<double>::from_zdd(z);
    auto z_back = mtzdd.to_zdd();
    EXPECT_EQ(z_back, z);
}

TEST_F(MTBDDClassTest, MTZDDToZddRoundTripWithEmpty) {
    // ZDD with empty set: {{}, {1}}
    auto z = ZDD::Single + ZDD::singleton(1);
    auto mtzdd = MTZDD<double>::from_zdd(z);
    auto z_back = mtzdd.to_zdd();
    EXPECT_EQ(z_back, z);
}

TEST_F(MTBDDClassTest, MTZDDToZddAllZero) {
    auto f = MTZDD<double>();  // zero terminal
    EXPECT_EQ(f.to_zdd(), ZDD::Empty);
}

TEST_F(MTBDDClassTest, MTZDDThresholdInt) {
    // Same pattern with int64_t
    auto f = MTZDD<int64_t>::ite(1, MTZDD<int64_t>::terminal(10),
                              MTZDD<int64_t>::terminal(3));
    auto z1 = f.threshold_gt(5);
    EXPECT_EQ(z1.count(), 1.0);
    auto sets1 = z1.enumerate();
    ASSERT_EQ(sets1.size(), 1u);
    EXPECT_EQ(sets1[0], std::vector<bddvar>({1}));

    auto z2 = f.threshold_ge(3);
    EXPECT_EQ(z2.count(), 2.0);

    auto z3 = f.to_zdd();
    EXPECT_EQ(z3.count(), 2.0);
}

TEST_F(MTBDDClassTest, MTZDDThresholdBruteForce) {
    // Build MTZDD, enumerate paths, verify threshold result matches
    auto t0 = MTZDD<double>::terminal(0.0);
    auto t2 = MTZDD<double>::terminal(2.0);
    auto t4 = MTZDD<double>::terminal(4.0);
    auto t6 = MTZDD<double>::terminal(6.0);
    auto inner = MTZDD<double>::ite(1, t4, t2);
    auto f = MTZDD<double>::ite(2, t6, inner);  // {2}->6, {1}->4, {}->2

    auto all_paths = f.enumerate();
    // Verify threshold_gt(3.0) by brute force
    auto z = f.threshold_gt(3.0);
    auto zdd_sets = z.enumerate();

    // Collect expected sets from enumerate
    std::vector<std::vector<bddvar>> expected;
    for (const auto& p : all_paths) {
        if (p.second > 3.0) {
            expected.push_back(p.first);
        }
    }
    std::sort(expected.begin(), expected.end());
    std::sort(zdd_sets.begin(), zdd_sets.end());
    EXPECT_EQ(zdd_sets, expected);
}

// ========================================================================
//  MTZDD cofactor iter (direct call) tests
//
// The public wrappers dispatch to _rec for shallow DDs, so these tests
// exercise the _iter branch directly and compare against the public API
// output on small examples.
// ========================================================================

class MtbddIterTest : public ::testing::Test {
protected:
    void SetUp() override {
        bddinit(256, UINT64_MAX);
        for (int i = 0; i < 4; i++) bddnewvar();
    }
};

TEST_F(MtbddIterTest, MtzddCofactor0IterMatchesPublic) {
    auto t100 = MTZDD<int64_t>::terminal(100);
    auto t200 = MTZDD<int64_t>::terminal(200);
    auto t300 = MTZDD<int64_t>::terminal(300);
    auto inner = MTZDD<int64_t>::ite(1, t100, t200);
    auto f = MTZDD<int64_t>::ite(3, inner, t300);

    uint8_t op_code = mtbdd_alloc_op_code();

    for (bddvar v : {static_cast<bddvar>(1), static_cast<bddvar>(2),
                     static_cast<bddvar>(3)}) {
        bddp expected = f.cofactor0(v).id();
        bddp iter_r = bdd_gc_guard([&]() -> bddp {
            return mtzdd_cofactor0_iter(f.id(), v, op_code);
        });
        EXPECT_EQ(iter_r, expected) << "v=" << v;
    }
}

TEST_F(MtbddIterTest, MtzddCofactor1IterMatchesPublic) {
    auto t100 = MTZDD<int64_t>::terminal(100);
    auto t200 = MTZDD<int64_t>::terminal(200);
    auto t300 = MTZDD<int64_t>::terminal(300);
    auto inner = MTZDD<int64_t>::ite(1, t100, t200);
    auto f = MTZDD<int64_t>::ite(3, inner, t300);

    uint8_t op_code = mtbdd_alloc_op_code();

    for (bddvar v : {static_cast<bddvar>(1), static_cast<bddvar>(2),
                     static_cast<bddvar>(3)}) {
        bddp expected = f.cofactor1(v).id();
        bddp iter_r = bdd_gc_guard([&]() -> bddp {
            return mtzdd_cofactor1_iter(f.id(), v, op_code);
        });
        EXPECT_EQ(iter_r, expected) << "v=" << v;
    }
}

TEST_F(MtbddIterTest, MtzddCofactorIterTerminals) {
    auto t = MTZDD<double>::terminal(7.5);
    uint8_t op0 = mtbdd_alloc_op_code();
    uint8_t op1 = mtbdd_alloc_op_code();

    bddp r0 = bdd_gc_guard([&]() -> bddp {
        return mtzdd_cofactor0_iter(t.id(), 1, op0);
    });
    EXPECT_EQ(r0, t.id());  // terminal is unchanged by cofactor0

    bddp r1 = bdd_gc_guard([&]() -> bddp {
        return mtzdd_cofactor1_iter(t.id(), 1, op1);
    });
    EXPECT_EQ(r1, BDD_CONST_FLAG);  // terminal → zero terminal for cofactor1
}

TEST_F(MtbddIterTest, MtzddCofactor0IterZeroSuppressedVar) {
    auto hi = MTZDD<int64_t>::terminal(10);
    auto lo = MTZDD<int64_t>::terminal(20);
    auto f = MTZDD<int64_t>::ite(1, hi, lo);

    uint8_t op_code = mtbdd_alloc_op_code();

    // v=2 above f's top level (f uses var 1, level 1; v=2 at level 2)
    bddp r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_cofactor0_iter(f.id(), 2, op_code);
    });
    EXPECT_EQ(r, f.id());
}

TEST_F(MtbddIterTest, MtzddCofactor1IterZeroSuppressedVar) {
    auto hi = MTZDD<int64_t>::terminal(10);
    auto lo = MTZDD<int64_t>::terminal(20);
    auto f = MTZDD<int64_t>::ite(1, hi, lo);

    uint8_t op_code = mtbdd_alloc_op_code();

    bddp r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_cofactor1_iter(f.id(), 2, op_code);
    });
    EXPECT_EQ(r, BDD_CONST_FLAG);
}

TEST_F(MtbddIterTest, MtzddCofactorIterFromZdd) {
    // Build a moderately complex MTZDD via from_zdd and verify iter vs public.
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    ZDD x3 = ZDD::singleton(3);
    ZDD f = x1 + x2 + (x1 * x3) + (x2 * x3);
    auto mf = MTZDD<double>::from_zdd(f, 0.0, 1.0);

    uint8_t op0 = mtbdd_alloc_op_code();
    uint8_t op1 = mtbdd_alloc_op_code();

    for (bddvar v : {static_cast<bddvar>(1), static_cast<bddvar>(2),
                     static_cast<bddvar>(3)}) {
        bddp e0 = mf.cofactor0(v).id();
        bddp r0 = bdd_gc_guard([&]() -> bddp {
            return mtzdd_cofactor0_iter(mf.id(), v, op0);
        });
        EXPECT_EQ(r0, e0) << "cofactor0 v=" << v;

        bddp e1 = mf.cofactor1(v).id();
        bddp r1 = bdd_gc_guard([&]() -> bddp {
            return mtzdd_cofactor1_iter(mf.id(), v, op1);
        });
        EXPECT_EQ(r1, e1) << "cofactor1 v=" << v;
    }
}

// ============================================================================
// MtbddTemplateIterTest: compares template-function iter vs rec.
// ============================================================================

class MtbddTemplateIterTest : public ::testing::Test {
protected:
    void SetUp() override {
        bddinit(1024, UINT64_MAX);
        for (int i = 0; i < 5; i++) bddnewvar();
    }
};

TEST_F(MtbddTemplateIterTest, MtbddApplyIterMatchesRec) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    BDD c = BDD::prime(3);
    auto ma = MTBDD<int64_t>::from_bdd(a & b, 0, 3);
    auto mb = MTBDD<int64_t>::from_bdd(b | c, 0, 5);
    auto adder = [](const int64_t& x, const int64_t& y) { return x + y; };

    uint8_t op_rec = mtbdd_alloc_op_code();
    uint8_t op_iter = mtbdd_alloc_op_code();
    bddp rec_r = bdd_gc_guard([&]() -> bddp {
        return mtbdd_apply_rec<int64_t, decltype(adder)>(ma.id(), mb.id(), adder, op_rec);
    });
    bddp iter_r = bdd_gc_guard([&]() -> bddp {
        return mtbdd_apply_iter<int64_t, decltype(adder)>(ma.id(), mb.id(), adder, op_iter);
    });
    EXPECT_EQ(iter_r, rec_r);
}

TEST_F(MtbddTemplateIterTest, MtzddApplyIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    ZDD x3 = ZDD::singleton(3);
    ZDD fa = x1 + x2 + (x1 * x3);
    ZDD fb = x1 * x2 + x3;
    auto ma = MTZDD<int64_t>::from_zdd(fa, 0, 2);
    auto mb = MTZDD<int64_t>::from_zdd(fb, 0, 3);
    auto adder = [](const int64_t& x, const int64_t& y) { return x + y; };

    uint8_t op_rec = mtbdd_alloc_op_code();
    uint8_t op_iter = mtbdd_alloc_op_code();
    bddp rec_r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_apply_rec<int64_t, decltype(adder)>(ma.id(), mb.id(), adder, op_rec);
    });
    bddp iter_r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_apply_iter<int64_t, decltype(adder)>(ma.id(), mb.id(), adder, op_iter);
    });
    EXPECT_EQ(iter_r, rec_r);
}

TEST_F(MtbddTemplateIterTest, MtbddIteIterMatchesRec) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    auto cond = MTBDD<int64_t>::from_bdd(a & b, 0, 1);
    auto then_v = MTBDD<int64_t>::terminal(100);
    auto else_v = MTBDD<int64_t>::from_bdd(a | b, 7, 11);

    uint8_t op_rec = mtbdd_alloc_op_code();
    uint8_t op_iter = mtbdd_alloc_op_code();
    bddp rec_r = bdd_gc_guard([&]() -> bddp {
        return mtbdd_ite_rec<int64_t>(cond.id(), then_v.id(), else_v.id(), op_rec);
    });
    bddp iter_r = bdd_gc_guard([&]() -> bddp {
        return mtbdd_ite_iter<int64_t>(cond.id(), then_v.id(), else_v.id(), op_iter);
    });
    EXPECT_EQ(iter_r, rec_r);
}

TEST_F(MtbddTemplateIterTest, MtzddIteIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    auto cond = MTZDD<int64_t>::from_zdd(x1 + x2, 0, 1);
    auto then_v = MTZDD<int64_t>::from_zdd(x1 * x2, 0, 42);
    auto else_v = MTZDD<int64_t>::from_zdd(x1, 0, 7);

    uint8_t op_rec = mtbdd_alloc_op_code();
    uint8_t op_iter = mtbdd_alloc_op_code();
    bddp rec_r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_ite_rec<int64_t>(cond.id(), then_v.id(), else_v.id(), op_rec);
    });
    bddp iter_r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_ite_iter<int64_t>(cond.id(), then_v.id(), else_v.id(), op_iter);
    });
    EXPECT_EQ(iter_r, rec_r);
}

TEST_F(MtbddTemplateIterTest, MtbddFromBddIterMatchesRec) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    BDD c = BDD::prime(3);
    BDD f = (a & b) | (~c);  // exercises complement edges
    bddp zero_t = MTBDD<int64_t>::zero_terminal();
    auto& table = MTBDDTerminalTable<int64_t>::instance();
    bddp one_t = MTBDDTerminalTable<int64_t>::make_terminal(table.get_or_insert(1));

    uint8_t op_rec = mtbdd_alloc_op_code();
    uint8_t op_iter = mtbdd_alloc_op_code();
    bddp rec_r = bdd_gc_guard([&]() -> bddp {
        return mtbdd_from_bdd_rec<int64_t>(f.id(), zero_t, one_t, op_rec);
    });
    bddp iter_r = bdd_gc_guard([&]() -> bddp {
        return mtbdd_from_bdd_iter<int64_t>(f.id(), zero_t, one_t, op_iter);
    });
    EXPECT_EQ(iter_r, rec_r);
}

TEST_F(MtbddTemplateIterTest, MtzddFromZddIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    ZDD x3 = ZDD::singleton(3);
    ZDD f = x1 + (x2 * x3) + x3;  // has empty set via complement
    bddp zero_t = MTZDD<int64_t>::zero_terminal();
    auto& table = MTBDDTerminalTable<int64_t>::instance();
    bddp one_t = MTBDDTerminalTable<int64_t>::make_terminal(table.get_or_insert(1));

    uint8_t op_rec = mtbdd_alloc_op_code();
    uint8_t op_iter = mtbdd_alloc_op_code();
    bddp rec_r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_from_zdd_rec<int64_t>(f.id(), zero_t, one_t, op_rec);
    });
    bddp iter_r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_from_zdd_iter<int64_t>(f.id(), zero_t, one_t, op_iter);
    });
    EXPECT_EQ(iter_r, rec_r);
}

TEST_F(MtbddTemplateIterTest, MtzddCountIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    ZDD x3 = ZDD::singleton(3);
    ZDD f = x1 + x2 + (x1 * x3) + (x2 * x3);
    auto m = MTZDD<int64_t>::from_zdd(f, 0, 1);

    std::unordered_map<bddp, double> memo_rec, memo_iter;
    double rec_v = mtzdd_count_rec(m.id(), memo_rec);
    double iter_v = mtzdd_count_iter(m.id(), memo_iter);
    EXPECT_DOUBLE_EQ(iter_v, rec_v);
    EXPECT_GT(iter_v, 0.0);
}

TEST_F(MtbddTemplateIterTest, MtzddExactCountIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    ZDD x3 = ZDD::singleton(3);
    ZDD f = x1 + x2 + (x1 * x3) + (x2 * x3);
    auto m = MTZDD<int64_t>::from_zdd(f, 0, 1);

    std::unordered_map<bddp, bigint::BigInt> memo_rec, memo_iter;
    bigint::BigInt rec_v = mtzdd_exact_count_rec(m.id(), memo_rec);
    bigint::BigInt iter_v = mtzdd_exact_count_iter(m.id(), memo_iter);
    EXPECT_EQ(iter_v, rec_v);
}

TEST_F(MtbddTemplateIterTest, MtzddCountForTerminalIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    auto ma = MTZDD<int64_t>::from_zdd(x1 + x2, 0, 5);
    auto mb = MTZDD<int64_t>::from_zdd(x1 * x2, 0, 7);
    auto m = ma.apply(mb, [](const int64_t& x, const int64_t& y) { return x + y; });

    auto& table = MTBDDTerminalTable<int64_t>::instance();
    uint64_t idx;
    ASSERT_TRUE(table.find_index(5, idx));
    bddp target = MTBDDTerminalTable<int64_t>::make_terminal(idx);

    std::unordered_map<bddp, double> memo_rec, memo_iter;
    double rec_v = mtzdd_count_for_terminal_rec(m.id(), target, memo_rec);
    double iter_v = mtzdd_count_for_terminal_iter(m.id(), target, memo_iter);
    EXPECT_DOUBLE_EQ(iter_v, rec_v);
}

TEST_F(MtbddTemplateIterTest, MtzddExactCountForTerminalIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    auto ma = MTZDD<int64_t>::from_zdd(x1 + x2, 0, 5);
    auto mb = MTZDD<int64_t>::from_zdd(x1 * x2, 0, 7);
    auto m = ma.apply(mb, [](const int64_t& x, const int64_t& y) { return x + y; });

    auto& table = MTBDDTerminalTable<int64_t>::instance();
    uint64_t idx;
    ASSERT_TRUE(table.find_index(5, idx));
    bddp target = MTBDDTerminalTable<int64_t>::make_terminal(idx);

    std::unordered_map<bddp, bigint::BigInt> memo_rec, memo_iter;
    bigint::BigInt rec_v = mtzdd_exact_count_for_terminal_rec(m.id(), target, memo_rec);
    bigint::BigInt iter_v = mtzdd_exact_count_for_terminal_iter(m.id(), target, memo_iter);
    EXPECT_EQ(iter_v, rec_v);
}

TEST_F(MtbddTemplateIterTest, MtzddEnumerateIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    ZDD x3 = ZDD::singleton(3);
    ZDD f = x1 + x2 + (x1 * x3) + (x2 * x3);
    auto m = MTZDD<int64_t>::from_zdd(f, 0, 1);

    std::vector<std::pair<std::vector<bddvar>, int64_t> > rec_result, iter_result;
    std::vector<bddvar> cur_rec, cur_iter;
    mtzdd_enumerate_rec<int64_t>(m.id(), cur_rec, rec_result);
    mtzdd_enumerate_iter<int64_t>(m.id(), cur_iter, iter_result);
    ASSERT_EQ(iter_result.size(), rec_result.size());
    for (size_t i = 0; i < rec_result.size(); ++i) {
        EXPECT_EQ(iter_result[i].first, rec_result[i].first);
        EXPECT_EQ(iter_result[i].second, rec_result[i].second);
    }
}

TEST_F(MtbddTemplateIterTest, MtzddToZddIterMatchesRec) {
    ZDD x1 = ZDD::singleton(1);
    ZDD x2 = ZDD::singleton(2);
    auto ma = MTZDD<int64_t>::from_zdd(x1 + x2, 0, 5);
    auto mb = MTZDD<int64_t>::from_zdd(x1 * x2, 0, 7);
    auto m = ma.apply(mb, [](const int64_t& x, const int64_t& y) { return x + y; });

    auto& table = MTBDDTerminalTable<int64_t>::instance();
    std::unordered_set<bddp> pred_terminals;
    for (uint64_t i = 1; i < table.size(); ++i) {
        pred_terminals.insert(MTBDDTerminalTable<int64_t>::make_terminal(i));
    }

    uint8_t op_rec = mtbdd_alloc_op_code();
    uint8_t op_iter = mtbdd_alloc_op_code();
    bddp rec_r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_to_zdd_rec<int64_t>(m.id(), pred_terminals, op_rec);
    });
    bddp iter_r = bdd_gc_guard([&]() -> bddp {
        return mtzdd_to_zdd_iter<int64_t>(m.id(), pred_terminals, op_iter);
    });
    EXPECT_EQ(iter_r, rec_r);
}
