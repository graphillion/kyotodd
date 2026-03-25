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
