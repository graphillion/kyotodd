#include <gtest/gtest.h>
#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <cstring>
#include <random>
#include <sstream>
#include <unordered_set>
#include <climits>
#include <limits>

using namespace kyotodd;

class BDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// --- Graphillion format export/import tests ---

TEST_F(BDDTest, Graphillion_ExportTerminalEmpty) {
    std::ostringstream oss;
    ZDD e(0);  // empty family
    e.export_graphillion(oss);
    EXPECT_EQ(oss.str(), "B\n.\n");
}

TEST_F(BDDTest, Graphillion_ExportTerminalSingle) {
    std::ostringstream oss;
    ZDD s(1);  // unit family {empty set}
    s.export_graphillion(oss);
    EXPECT_EQ(oss.str(), "T\n.\n");
}

TEST_F(BDDTest, Graphillion_ImportTerminalEmpty) {
    std::istringstream iss("B\n.\n");
    ZDD z = ZDD::import_graphillion(iss);
    EXPECT_EQ(z, ZDD::Empty);
}

TEST_F(BDDTest, Graphillion_ImportTerminalSingle) {
    std::istringstream iss("T\n.\n");
    ZDD z = ZDD::import_graphillion(iss);
    EXPECT_EQ(z, ZDD::Single);
}

TEST_F(BDDTest, Graphillion_RoundtripSingleton) {
    // {{1}} - a family with one set containing variable 1
    ZDD f = ZDD::singleton(1);
    std::ostringstream oss;
    f.export_graphillion(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_graphillion(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Graphillion_RoundtripPowerSet3) {
    // Power set of {1,2,3}: {},{1},{2},{3},{1,2},{1,3},{2,3},{1,2,3}
    ZDD f = ZDD::power_set(3);
    std::ostringstream oss;
    f.export_graphillion(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_graphillion(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Graphillion_RoundtripCombination) {
    // C(4,2): all 2-element subsets of {1,2,3,4}
    ZDD f = ZDD::combination(4, 2);
    std::ostringstream oss;
    f.export_graphillion(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_graphillion(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Graphillion_RoundtripFromSets) {
    std::vector<std::vector<bddvar>> sets = {{1, 3}, {2}, {1, 2, 3}};
    ZDD f = ZDD::from_sets(sets);
    std::ostringstream oss;
    f.export_graphillion(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_graphillion(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Graphillion_ExportVariableReversal) {
    // {{3}} with 3 variables - root should be var 3 (level 3 in KyotoDD)
    // In Graphillion: root should have graphillion_var 1
    ZDD f = ZDD::singleton(3);
    std::ostringstream oss;
    f.export_graphillion(oss);
    std::string output = oss.str();
    // The last non-terminal line (before ".") should have the smallest
    // graphillion variable number (root side).
    // Since this is a single node with var 3 -> level 3, and N=3,
    // graphillion_var = 3 + 1 - 3 = 1
    // Output: "0 1 B T\n.\n"
    EXPECT_EQ(output, "0 1 B T\n.\n");
}

TEST_F(BDDTest, Graphillion_ImportKnownFormat) {
    // Manually constructed: 2 variables in Graphillion
    // Node 0: g_var=2, lo=B, hi=T  (near terminals, KyotoDD var 1)
    // Node 1: g_var=1, lo=0, hi=T  (root, KyotoDD var 2)
    // This should represent {{1},{2},{1,2}} - i.e. all non-empty subsets of {1,2}
    // Wait, let's trace: root=node1, var=2
    //   lo=node0 (var=1), hi=T
    //   node0: var=1, lo=B, hi=T
    // So: ZDD with var 2 at root:
    //   var2=0 -> node0: var1=0 -> B (empty), var1=1 -> T -> {{}}
    //     So lo of root gives: {{1}}
    //   var2=1 -> T -> {{}}
    //     Adding var2: {{2}}
    // Family = {{1}, {2}} - hmm, that's not right for what I want.
    // Actually: root node is var 2 (KyotoDD), lo=node0, hi=T
    //   Path var2=0: go to node0 (var1), lo=B (no), hi=T (yes, {1}) -> {{1}}
    //   Path var2=1: go to T -> {∅} -> add var2 -> {{2}}
    // Family = {{1}, {2}}
    std::istringstream iss("0 2 B T\n1 1 0 T\n.\n");
    ZDD z = ZDD::import_graphillion(iss);
    auto sets = z.enumerate();
    // Expected: {{1}, {2}}
    std::vector<std::vector<bddvar>> expected = {{1}, {2}};
    EXPECT_EQ(sets, expected);
}

TEST_F(BDDTest, Graphillion_RoundtripComplementEdge) {
    // ~(ZDD::Single) has complement on root
    // ZDD with complement edges: {{}} complement = family without empty set...
    // Actually ~ZDD::Single toggles empty set membership.
    // ZDD::Single = {∅}, ~ZDD::Single = {} (empty family) = ZDD::Empty
    // Let's use a more interesting case: power_set(2) includes ∅
    // Its complement toggles ∅ membership.
    ZDD f = ZDD::power_set(2);
    ZDD g = ~f;  // toggle empty set membership
    std::ostringstream oss;
    g.export_graphillion(oss);
    std::istringstream iss(oss.str());
    ZDD h = ZDD::import_graphillion(iss);
    EXPECT_EQ(g.enumerate(), h.enumerate());
}

TEST_F(BDDTest, Graphillion_OffsetImport) {
    // Import with offset=2: graphillion var 1 maps to level = 1+1-1+2 = 3
    // So a single-node ZDD with graphillion var 1 should become var at level 3
    std::istringstream iss("0 1 B T\n.\n");
    ZDD z = ZDD::import_graphillion(iss, 2);
    auto sets = z.enumerate();
    // level 3 -> var 3 (default ordering)
    std::vector<std::vector<bddvar>> expected = {{3}};
    EXPECT_EQ(sets, expected);
}

TEST_F(BDDTest, Graphillion_OffsetExport) {
    // Export singleton(1) (level 1, N=1) with offset=2:
    // graphillion_var = 1 + 1 - 1 + 2 = 3
    ZDD f = ZDD::singleton(1);
    std::ostringstream oss;
    f.export_graphillion(oss, 2);
    EXPECT_EQ(oss.str(), "0 3 B T\n.\n");
}

TEST_F(BDDTest, Graphillion_RoundtripMultipleVars) {
    // {{1,2}, {2,3}, {1,3}}
    std::vector<std::vector<bddvar>> sets = {{1, 2}, {2, 3}, {1, 3}};
    ZDD f = ZDD::from_sets(sets);
    std::ostringstream oss;
    f.export_graphillion(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_graphillion(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Graphillion_ImportErrorMissingDot) {
    std::istringstream iss("0 1 B T\n");
    EXPECT_THROW(ZDD::import_graphillion(iss), std::runtime_error);
}

TEST_F(BDDTest, Graphillion_ImportErrorUndefinedRef) {
    // Node references undefined child
    std::istringstream iss("0 1 B 99\n.\n");
    EXPECT_THROW(ZDD::import_graphillion(iss), std::runtime_error);
}

TEST_F(BDDTest, Graphillion_FileRoundtrip) {
    // Test FILE* variants
    ZDD f = ZDD::power_set(3);
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    f.export_graphillion(fp);
    std::rewind(fp);
    ZDD g = ZDD::import_graphillion(fp);
    std::fclose(fp);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Graphillion_NegativeOffsetUnderflow) {
    // offset too negative should throw, not cause unsigned underflow
    std::istringstream iss("1 2 3\n.\n");
    EXPECT_THROW(ZDD::import_graphillion(iss, -100), std::runtime_error);
}

TEST_F(BDDTest, Graphillion_ImportVarZeroThrows) {
    // g_var=0 is invalid (variables are 1-based)
    std::istringstream iss("2 1 B T\n4 0 B 2\n.\n");
    EXPECT_THROW(ZDD::import_graphillion(iss, 0), std::runtime_error);
}

// --- Sapporo format export/import tests ---

TEST_F(BDDTest, Sapporo_BDD_RoundtripStream) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    BDD f = a & b;  // AND of var 1 and var 2
    std::ostringstream oss;
    f.export_sapporo(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_sapporo(iss);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Sapporo_BDD_RoundtripFile) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    BDD f = a | b;  // OR of var 1 and var 2
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    f.export_sapporo(fp);
    std::rewind(fp);
    BDD g = BDD::import_sapporo(fp);
    std::fclose(fp);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Sapporo_BDD_TerminalFalse) {
    BDD f = BDD::False;
    std::ostringstream oss;
    f.export_sapporo(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_sapporo(iss);
    EXPECT_EQ(g, BDD::False);
}

TEST_F(BDDTest, Sapporo_BDD_TerminalTrue) {
    BDD f = BDD::True;
    std::ostringstream oss;
    f.export_sapporo(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_sapporo(iss);
    EXPECT_EQ(g, BDD::True);
}

TEST_F(BDDTest, Sapporo_ZDD_RoundtripStream) {
    ZDD f = ZDD::power_set(3);
    std::ostringstream oss;
    f.export_sapporo(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_sapporo(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Sapporo_ZDD_RoundtripFile) {
    ZDD f = ZDD::combination(4, 2);
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    f.export_sapporo(fp);
    std::rewind(fp);
    ZDD g = ZDD::import_sapporo(fp);
    std::fclose(fp);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Sapporo_ZDD_TerminalEmpty) {
    ZDD f = ZDD::Empty;
    std::ostringstream oss;
    f.export_sapporo(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_sapporo(iss);
    EXPECT_EQ(g, ZDD::Empty);
}

TEST_F(BDDTest, Sapporo_ZDD_TerminalSingle) {
    ZDD f = ZDD::Single;
    std::ostringstream oss;
    f.export_sapporo(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_sapporo(iss);
    EXPECT_EQ(g, ZDD::Single);
}

TEST_F(BDDTest, Sapporo_CLevel_BDD) {
    // Test C-level functions directly
    BDD f = BDD::prime(1) ^ BDD::prime(2);  // XOR
    std::ostringstream oss;
    bdd_export_sapporo(oss, f.GetID());
    std::istringstream iss(oss.str());
    bddp g = bdd_import_sapporo(iss);
    EXPECT_EQ(f.GetID(), g);
}

TEST_F(BDDTest, Sapporo_CLevel_ZDD) {
    // Test C-level functions directly
    std::vector<std::vector<bddvar>> sets = {{1, 2}, {3}};
    ZDD f = ZDD::from_sets(sets);
    std::ostringstream oss;
    zdd_export_sapporo(oss, f.GetID());
    std::istringstream iss(oss.str());
    bddp g = zdd_import_sapporo(iss);
    ZDD gz = ZDD_ID(g);
    EXPECT_EQ(f.enumerate(), gz.enumerate());
}

// --- Sapporo import error handling tests ---

TEST_F(BDDTest, Sapporo_BDD_InvalidInputThrows) {
    std::istringstream iss("garbage\n");
    EXPECT_THROW(BDD::import_sapporo(iss), std::runtime_error);
}

TEST_F(BDDTest, Sapporo_ZDD_InvalidInputThrows) {
    std::istringstream iss("garbage\n");
    EXPECT_THROW(ZDD::import_sapporo(iss), std::runtime_error);
}

TEST_F(BDDTest, Sapporo_BDD_EmptyInputThrows) {
    std::istringstream iss("");
    EXPECT_THROW(BDD::import_sapporo(iss), std::runtime_error);
}

// --- Binary format export/import tests ---

TEST_F(BDDTest, Binary_BDD_RoundtripStream) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    BDD f = a & b;
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_binary(iss);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Binary_BDD_TerminalFalse) {
    BDD f = BDD::False;
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_binary(iss);
    EXPECT_EQ(g, BDD::False);
}

TEST_F(BDDTest, Binary_BDD_TerminalTrue) {
    BDD f = BDD::True;
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_binary(iss);
    EXPECT_EQ(g, BDD::True);
}

TEST_F(BDDTest, Binary_BDD_ComplementRoot) {
    // BDD with complement on root
    BDD f = ~BDD::prime(1);  // NOT var 1
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_binary(iss);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Binary_BDD_Complex) {
    BDD f = BDD::Ite(BDD::prime(1), BDD::prime(2), BDD::prime(3));
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_binary(iss);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Binary_BDD_FileRoundtrip) {
    BDD f = BDD::prime(1) | BDD::prime(2);
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    f.export_binary(fp);
    std::rewind(fp);
    BDD g = BDD::import_binary(fp);
    std::fclose(fp);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Binary_ZDD_RoundtripStream) {
    ZDD f = ZDD::power_set(3);
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_binary(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Binary_ZDD_TerminalEmpty) {
    ZDD f = ZDD::Empty;
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_binary(iss);
    EXPECT_EQ(g, ZDD::Empty);
}

TEST_F(BDDTest, Binary_ZDD_TerminalSingle) {
    ZDD f = ZDD::Single;
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_binary(iss);
    EXPECT_EQ(g, ZDD::Single);
}

TEST_F(BDDTest, Binary_ZDD_Combination) {
    ZDD f = ZDD::combination(4, 2);
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    f.export_binary(fp);
    std::rewind(fp);
    ZDD g = ZDD::import_binary(fp);
    std::fclose(fp);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Binary_ZDD_ComplementEdge) {
    ZDD f = ~ZDD::power_set(2);
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_binary(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Binary_ZDD_FileRoundtrip) {
    ZDD f = ZDD::power_set(3);
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    f.export_binary(fp);
    std::rewind(fp);
    ZDD g = ZDD::import_binary(fp);
    std::fclose(fp);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Binary_ZDD_FromSets) {
    std::vector<std::vector<bddvar>> sets = {{1, 3}, {2}, {1, 2, 3}};
    ZDD f = ZDD::from_sets(sets);
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_binary(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Binary_MagicBytes) {
    ZDD f = ZDD::singleton(1);
    std::ostringstream oss;
    f.export_binary(oss);
    std::string data = oss.str();
    // First 3 bytes should be "BDD"
    EXPECT_EQ(data[0], 'B');
    EXPECT_EQ(data[1], 'D');
    EXPECT_EQ(data[2], 'D');
    // Version = 1
    EXPECT_EQ(static_cast<uint8_t>(data[3]), 1);
    // Type = 3 (ZDD)
    EXPECT_EQ(static_cast<uint8_t>(data[4]), 3);
}

TEST_F(BDDTest, Binary_ImportInvalidMagic) {
    std::istringstream iss("XXX");
    EXPECT_THROW(ZDD::import_binary(iss), std::runtime_error);
}

TEST_F(BDDTest, Binary_CLevel_BDD) {
    BDD f = BDD::prime(1) ^ BDD::prime(2);
    std::ostringstream oss;
    bdd_export_binary(oss, f.GetID());
    std::istringstream iss(oss.str());
    bddp g = bdd_import_binary(iss);
    EXPECT_EQ(f.GetID(), g);
}

TEST_F(BDDTest, Binary_CLevel_ZDD) {
    std::vector<std::vector<bddvar>> sets = {{1, 2}, {3}};
    ZDD f = ZDD::from_sets(sets);
    std::ostringstream oss;
    zdd_export_binary(oss, f.GetID());
    std::istringstream iss(oss.str());
    bddp g = zdd_import_binary(iss);
    ZDD gz = ZDD_ID(g);
    EXPECT_EQ(f.enumerate(), gz.enumerate());
}

// --- Multi-root binary format export/import tests ---

TEST_F(BDDTest, BinaryMulti_BDD_RoundtripStream) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    BDD f1 = a & b;
    BDD f2 = a | b;
    BDD f3 = ~a;
    std::vector<BDD> bdds = {f1, f2, f3};
    std::ostringstream oss;
    BDD::export_binary_multi(oss, bdds);
    std::istringstream iss(oss.str());
    std::vector<BDD> result = BDD::import_binary_multi(iss);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], f1);
    EXPECT_EQ(result[1], f2);
    EXPECT_EQ(result[2], f3);
}

TEST_F(BDDTest, BinaryMulti_BDD_EmptyVector) {
    std::vector<BDD> bdds;
    std::ostringstream oss;
    BDD::export_binary_multi(oss, bdds);
    std::istringstream iss(oss.str());
    std::vector<BDD> result = BDD::import_binary_multi(iss);
    EXPECT_TRUE(result.empty());
}

TEST_F(BDDTest, BinaryMulti_BDD_TerminalsOnly) {
    std::vector<BDD> bdds = {BDD::False, BDD::True, BDD::False};
    std::ostringstream oss;
    BDD::export_binary_multi(oss, bdds);
    std::istringstream iss(oss.str());
    std::vector<BDD> result = BDD::import_binary_multi(iss);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], BDD::False);
    EXPECT_EQ(result[1], BDD::True);
    EXPECT_EQ(result[2], BDD::False);
}

TEST_F(BDDTest, BinaryMulti_BDD_SharedNodes) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    BDD f1 = a & b;
    BDD f2 = a | b;  // shares nodes with f1
    std::vector<BDD> bdds = {f1, f2};
    std::ostringstream oss;
    BDD::export_binary_multi(oss, bdds);
    std::istringstream iss(oss.str());
    std::vector<BDD> result = BDD::import_binary_multi(iss);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], f1);
    EXPECT_EQ(result[1], f2);
}

TEST_F(BDDTest, BinaryMulti_BDD_FILE) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    BDD f1 = a & b;
    BDD f2 = ~a;
    std::vector<BDD> bdds = {f1, f2};
    FILE* fp = std::tmpfile();
    BDD::export_binary_multi(fp, bdds);
    std::rewind(fp);
    std::vector<BDD> result = BDD::import_binary_multi(fp);
    std::fclose(fp);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], f1);
    EXPECT_EQ(result[1], f2);
}

TEST_F(BDDTest, BinaryMulti_BDD_SingleExportMultiImport) {
    BDD f = BDD::prime(1) & BDD::prime(2);
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    std::vector<BDD> result = BDD::import_binary_multi(iss);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], f);
}

TEST_F(BDDTest, BinaryMulti_BDD_MultiExportSingleImport) {
    BDD f1 = BDD::prime(1) & BDD::prime(2);
    BDD f2 = BDD::prime(1) | BDD::prime(2);
    std::vector<BDD> bdds = {f1, f2};
    std::ostringstream oss;
    BDD::export_binary_multi(oss, bdds);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_binary(iss);
    EXPECT_EQ(g, f1);
}

TEST_F(BDDTest, BinaryMulti_ZDD_RoundtripStream) {
    std::vector<std::vector<bddvar>> sets1 = {{1, 2}, {3}};
    std::vector<std::vector<bddvar>> sets2 = {{1}, {2, 3}};
    ZDD f1 = ZDD::from_sets(sets1);
    ZDD f2 = ZDD::from_sets(sets2);
    std::vector<ZDD> zdds = {f1, f2};
    std::ostringstream oss;
    ZDD::export_binary_multi(oss, zdds);
    std::istringstream iss(oss.str());
    std::vector<ZDD> result = ZDD::import_binary_multi(iss);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].enumerate(), f1.enumerate());
    EXPECT_EQ(result[1].enumerate(), f2.enumerate());
}

TEST_F(BDDTest, BinaryMulti_ZDD_EmptyVector) {
    std::vector<ZDD> zdds;
    std::ostringstream oss;
    ZDD::export_binary_multi(oss, zdds);
    std::istringstream iss(oss.str());
    std::vector<ZDD> result = ZDD::import_binary_multi(iss);
    EXPECT_TRUE(result.empty());
}

TEST_F(BDDTest, BinaryMulti_ZDD_TerminalsOnly) {
    std::vector<ZDD> zdds = {ZDD::Empty, ZDD::Single, ZDD::Empty};
    std::ostringstream oss;
    ZDD::export_binary_multi(oss, zdds);
    std::istringstream iss(oss.str());
    std::vector<ZDD> result = ZDD::import_binary_multi(iss);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], ZDD::Empty);
    EXPECT_EQ(result[1], ZDD::Single);
    EXPECT_EQ(result[2], ZDD::Empty);
}

TEST_F(BDDTest, BinaryMulti_ZDD_FILE) {
    std::vector<std::vector<bddvar>> sets1 = {{1, 2}, {3}};
    ZDD f1 = ZDD::from_sets(sets1);
    ZDD f2 = ZDD::Single;
    std::vector<ZDD> zdds = {f1, f2};
    FILE* fp = std::tmpfile();
    ZDD::export_binary_multi(fp, zdds);
    std::rewind(fp);
    std::vector<ZDD> result = ZDD::import_binary_multi(fp);
    std::fclose(fp);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].enumerate(), f1.enumerate());
    EXPECT_EQ(result[1], ZDD::Single);
}

TEST_F(BDDTest, BinaryMulti_CLevel_BDD) {
    BDD a = BDD::prime(1);
    BDD b = BDD::prime(2);
    bddp roots[2] = {(a & b).GetID(), (a | b).GetID()};
    std::ostringstream oss;
    bdd_export_binary_multi(oss, roots, 2);
    std::istringstream iss(oss.str());
    std::vector<bddp> result = bdd_import_binary_multi(iss);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], roots[0]);
    EXPECT_EQ(result[1], roots[1]);
}

TEST_F(BDDTest, BinaryMulti_CLevel_ZDD) {
    std::vector<std::vector<bddvar>> sets = {{1, 2}, {3}};
    ZDD f = ZDD::from_sets(sets);
    bddp roots[1] = {f.GetID()};
    std::ostringstream oss;
    zdd_export_binary_multi(oss, roots, 1);
    std::istringstream iss(oss.str());
    std::vector<bddp> result = zdd_import_binary_multi(iss);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(ZDD_ID(result[0]).enumerate(), f.enumerate());
}

// --- Binary import type validation tests ---

TEST_F(BDDTest, Binary_TypeMismatch_BDD_from_ZDD) {
    std::vector<std::vector<bddvar>> sets = {{1, 2}, {3}};
    ZDD z = ZDD::from_sets(sets);
    std::ostringstream oss;
    z.export_binary(oss);
    std::istringstream iss(oss.str());
    EXPECT_THROW(BDD::import_binary(iss), std::runtime_error);
}

TEST_F(BDDTest, Binary_TypeMismatch_ZDD_from_BDD) {
    BDD f = BDD::prime(1) & BDD::prime(2);
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    EXPECT_THROW(ZDD::import_binary(iss), std::runtime_error);
}

TEST_F(BDDTest, Binary_TypeMismatch_QDD_from_BDD) {
    BDD f = BDD::prime(1) & BDD::prime(2);
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    EXPECT_THROW(QDD::import_binary(iss), std::runtime_error);
}

TEST_F(BDDTest, Binary_TypeMismatch_IgnoreType) {
    ZDD z = ZDD::from_sets({{1, 2}, {3}});
    std::ostringstream oss;
    z.export_binary(oss);
    std::istringstream iss(oss.str());
    EXPECT_NO_THROW(BDD::import_binary(iss, true));
}

TEST_F(BDDTest, Binary_TypeMismatch_UnreducedDD_AcceptsAny) {
    BDD f = BDD::prime(1) & BDD::prime(2);
    std::ostringstream oss;
    f.export_binary(oss);
    std::istringstream iss(oss.str());
    EXPECT_NO_THROW(UnreducedDD::import_binary(iss));
}

TEST_F(BDDTest, Binary_TypeMismatch_UnreducedDD_AcceptsZDD) {
    ZDD z = ZDD::from_sets({{1}, {2}});
    std::ostringstream oss;
    z.export_binary(oss);
    std::istringstream iss(oss.str());
    EXPECT_NO_THROW(UnreducedDD::import_binary(iss));
}

TEST_F(BDDTest, BinaryMulti_TypeMismatch_ZDD_from_BDD) {
    BDD f1 = BDD::prime(1);
    BDD f2 = BDD::prime(2);
    std::vector<BDD> bdds = {f1, f2};
    std::ostringstream oss;
    BDD::export_binary_multi(oss, bdds);
    std::istringstream iss(oss.str());
    EXPECT_THROW(ZDD::import_binary_multi(iss), std::runtime_error);
}

TEST_F(BDDTest, BinaryMulti_TypeMismatch_IgnoreType) {
    BDD f1 = BDD::prime(1);
    BDD f2 = BDD::prime(2);
    std::vector<BDD> bdds = {f1, f2};
    std::ostringstream oss;
    BDD::export_binary_multi(oss, bdds);
    std::istringstream iss(oss.str());
    EXPECT_NO_THROW(ZDD::import_binary_multi(iss, true));
}

TEST_F(BDDTest, Binary_TypeMismatch_ErrorMessage) {
    ZDD z = ZDD::from_sets({{1}});
    std::ostringstream oss;
    z.export_binary(oss);
    std::istringstream iss(oss.str());
    try {
        BDD::import_binary(iss);
        FAIL() << "Expected runtime_error";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("type mismatch"), std::string::npos);
        EXPECT_NE(msg.find("BDD"), std::string::npos);
        EXPECT_NE(msg.find("ZDD"), std::string::npos);
    }
}

TEST_F(BDDTest, Binary_TypeMismatch_CLevel_BDD_from_ZDD) {
    ZDD z = ZDD::from_sets({{1, 2}});
    std::ostringstream oss;
    zdd_export_binary(oss, z.GetID());
    std::istringstream iss(oss.str());
    EXPECT_THROW(bdd_import_binary(iss), std::runtime_error);
}

TEST_F(BDDTest, Binary_TypeMismatch_CLevel_IgnoreType) {
    ZDD z = ZDD::from_sets({{1, 2}});
    std::ostringstream oss;
    zdd_export_binary(oss, z.GetID());
    std::istringstream iss(oss.str());
    EXPECT_NO_THROW(bdd_import_binary(iss, true));
}

// --- Knuth format export/import tests (obsolete format) ---

TEST_F(BDDTest, Knuth_ZDD_TerminalEmpty) {
    std::ostringstream oss;
    ZDD(0).export_knuth(oss);
    EXPECT_EQ(oss.str(), "0\n");
    std::istringstream iss("0\n");
    EXPECT_EQ(ZDD::import_knuth(iss), ZDD::Empty);
}

TEST_F(BDDTest, Knuth_ZDD_TerminalSingle) {
    std::ostringstream oss;
    ZDD(1).export_knuth(oss);
    EXPECT_EQ(oss.str(), "1\n");
    std::istringstream iss("1\n");
    EXPECT_EQ(ZDD::import_knuth(iss), ZDD::Single);
}

TEST_F(BDDTest, Knuth_ZDD_RoundtripDecimal) {
    ZDD f = ZDD::power_set(3);
    std::ostringstream oss;
    f.export_knuth(oss, false);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_knuth(iss, false);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Knuth_ZDD_RoundtripHex) {
    ZDD f = ZDD::power_set(3);
    std::ostringstream oss;
    f.export_knuth(oss, true);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_knuth(iss, true);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Knuth_ZDD_RoundtripMultiLevel) {
    std::vector<std::vector<bddvar>> sets = {{1, 2}, {2, 3}, {1, 3}};
    ZDD f = ZDD::from_sets(sets);
    std::ostringstream oss;
    f.export_knuth(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_knuth(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Knuth_ZDD_RoundtripCombination) {
    ZDD f = ZDD::combination(4, 2);
    std::ostringstream oss;
    f.export_knuth(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_knuth(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Knuth_ZDD_RoundtripFromSets) {
    std::vector<std::vector<bddvar>> sets = {{1, 3}, {2}, {1, 2, 3}};
    ZDD f = ZDD::from_sets(sets);
    std::ostringstream oss;
    f.export_knuth(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_knuth(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Knuth_ZDD_ComplementEdge) {
    ZDD f = ~ZDD::power_set(2);
    std::ostringstream oss;
    f.export_knuth(oss);
    std::istringstream iss(oss.str());
    ZDD g = ZDD::import_knuth(iss);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Knuth_BDD_RoundtripStream) {
    BDD f = BDD::prime(1) & BDD::prime(2);
    std::ostringstream oss;
    f.export_knuth(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_knuth(iss);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Knuth_BDD_TerminalFalse) {
    std::ostringstream oss;
    BDD::False.export_knuth(oss);
    EXPECT_EQ(oss.str(), "0\n");
}

TEST_F(BDDTest, Knuth_BDD_TerminalTrue) {
    std::ostringstream oss;
    BDD::True.export_knuth(oss);
    EXPECT_EQ(oss.str(), "1\n");
}

TEST_F(BDDTest, Knuth_ExportFormatCheck) {
    // Verify the exact format: level headers and node lines
    ZDD f = ZDD::singleton(1);  // {{1}}, single node at level 1
    std::ostringstream oss;
    f.export_knuth(oss);
    std::string out = oss.str();
    // Root is at level 1 (KyotoDD), max_level=1, knuth_level = 1+1-1 = 1
    // Node with ID 2, lo=0 (empty), hi=1 (single)
    EXPECT_EQ(out, "#1\n2:0,1\n");
}

TEST_F(BDDTest, Knuth_ExportHexFormat) {
    ZDD f = ZDD::singleton(1);
    std::ostringstream oss;
    f.export_knuth(oss, true);
    EXPECT_EQ(oss.str(), "#1\n2:0,1\n");
}

TEST_F(BDDTest, Knuth_OffsetImport) {
    // Import with offset=2: knuth_level 1 → kyotodd_level = 1+1-1+2 = 3
    std::istringstream iss("#1\n2:0,1\n");
    ZDD z = ZDD::import_knuth(iss, false, 2);
    auto sets = z.enumerate();
    std::vector<std::vector<bddvar>> expected = {{3}};
    EXPECT_EQ(sets, expected);
}

TEST_F(BDDTest, Knuth_OffsetExport) {
    ZDD f = ZDD::singleton(1);
    std::ostringstream oss;
    f.export_knuth(oss, false, 2);
    // level=1, N=1, knuth_level = 1+1-1+2 = 3
    EXPECT_EQ(oss.str(), "#3\n2:0,1\n");
}

TEST_F(BDDTest, Knuth_FileRoundtrip) {
    ZDD f = ZDD::power_set(3);
    FILE* fp = std::tmpfile();
    ASSERT_NE(fp, nullptr);
    f.export_knuth(fp);
    std::rewind(fp);
    ZDD g = ZDD::import_knuth(fp);
    std::fclose(fp);
    EXPECT_EQ(f.enumerate(), g.enumerate());
}

TEST_F(BDDTest, Knuth_CLevel) {
    std::vector<std::vector<bddvar>> sets = {{1, 2}, {3}};
    ZDD f = ZDD::from_sets(sets);
    std::ostringstream oss;
    zdd_export_knuth(oss, f.GetID());
    std::istringstream iss(oss.str());
    bddp g = zdd_import_knuth(iss);
    ZDD gz = ZDD_ID(g);
    EXPECT_EQ(f.enumerate(), gz.enumerate());
}

// --- Knuth export/import fix tests ---

TEST_F(BDDTest, Knuth_BDD_ComplementEdge) {
    // BDD complement edge roundtrip: ~x1 should survive export/import
    BDD f = ~BDD::prime(1);
    std::ostringstream oss;
    f.export_knuth(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_knuth(iss);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Knuth_BDD_ComplementComplex) {
    // More complex BDD with complement edges
    BDD f = ~(BDD::prime(1) & BDD::prime(2));
    std::ostringstream oss;
    f.export_knuth(oss);
    std::istringstream iss(oss.str());
    BDD g = BDD::import_knuth(iss);
    EXPECT_EQ(f, g);
}

TEST_F(BDDTest, Knuth_ImportNoLevelHeader) {
    // Node line without preceding level header should throw
    std::istringstream iss("2:0,1\n");
    EXPECT_THROW(ZDD::import_knuth(iss), std::runtime_error);
}

TEST_F(BDDTest, Knuth_ImportNoLevelHeaderBDD) {
    std::istringstream iss("2:0,1\n");
    EXPECT_THROW(BDD::import_knuth(iss), std::runtime_error);
}

// --- Binary import terminal-only no variable pollution ---

TEST_F(BDDTest, Binary_TerminalOnly_NoVariablePollution) {
    // Importing a terminal-only binary should not create variables
    // when none existed before.
    // Note: this test relies on being in a fresh bddinit state.
    // We test by checking that the export of False doesn't force max_level=1.
    BDD f = BDD::False;
    std::ostringstream oss;
    f.export_binary(oss);
    std::string data = oss.str();
    // The header byte at offset 3+11 (magic 3 + header offset 11) = byte 14
    // is the start of max_level (uint64_t LE). For terminal-only, it should be 0.
    // magic(3) + version(1) = offset 4 in header, dd_type(1), num_arcs(2),
    // num_terminals(4), bits_for_level(1), bits_for_id(1), use_neg(1), max_level starts at header[11]
    // total offset from start: 3 (magic) + 11 = 14
    uint64_t max_level = 0;
    std::memcpy(&max_level, data.data() + 14, 8);
    EXPECT_EQ(max_level, 0u);
}

// --- BDD::getnode level validation ---

TEST_F(BDDTest, BddGetnodeLevelValidation) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Valid: children at lower levels
    bddp child = BDD::getnode(v1, bddfalse, bddtrue);
    EXPECT_NO_THROW(BDD::getnode(v2, child, bddtrue));
    EXPECT_NO_THROW(BDD::getnode(v3, child, bddtrue));

    // Valid: terminal children (always allowed)
    EXPECT_NO_THROW(BDD::getnode(v1, bddfalse, bddtrue));

    // Invalid: lo child at same level as parent
    bddp same_level = BDD::getnode_raw(v2, bddfalse, bddtrue);
    EXPECT_THROW(BDD::getnode(v2, same_level, bddtrue), std::invalid_argument);

    // Invalid: hi child at higher level than parent
    bddp higher = BDD::getnode_raw(v3, bddfalse, bddtrue);
    EXPECT_THROW(BDD::getnode(v2, bddfalse, higher), std::invalid_argument);

    // Invalid: var out of range
    EXPECT_THROW(BDD::getnode(0, bddfalse, bddtrue), std::invalid_argument);
    EXPECT_THROW(BDD::getnode(bddvarused() + 1, bddfalse, bddtrue), std::invalid_argument);

    // Validation runs before reduction rule:
    // node_v3 at level 3 > v1's level 1, so this throws
    bddp node_v3 = BDD::getnode_raw(v3, bddfalse, bddtrue);
    EXPECT_THROW(BDD::getnode(v1, node_v3, node_v3), std::invalid_argument);

    // Reduction rule with valid inputs: lo == hi returns lo
    EXPECT_EQ(BDD::getnode(v1, bddfalse, bddfalse), bddfalse);
    EXPECT_EQ(BDD::getnode(v1, bddtrue, bddtrue), bddtrue);
}

TEST_F(BDDTest, ZddGetnodeLevelValidation) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Valid: children at lower levels
    bddp child = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_NO_THROW(ZDD::getnode(v2, child, bddsingle));
    EXPECT_NO_THROW(ZDD::getnode(v3, child, bddsingle));

    // Valid: terminal children
    EXPECT_NO_THROW(ZDD::getnode(v1, bddempty, bddsingle));

    // Invalid: hi child at same level as parent
    bddp same_level = ZDD::getnode_raw(v2, bddempty, bddsingle);
    EXPECT_THROW(ZDD::getnode(v2, bddempty, same_level), std::invalid_argument);

    // Invalid: lo child at higher level than parent
    bddp higher = ZDD::getnode_raw(v3, bddempty, bddsingle);
    EXPECT_THROW(ZDD::getnode(v2, higher, bddsingle), std::invalid_argument);

    // Invalid: var out of range
    EXPECT_THROW(ZDD::getnode(0, bddempty, bddsingle), std::invalid_argument);
    EXPECT_THROW(ZDD::getnode(bddvarused() + 1, bddempty, bddsingle), std::invalid_argument);

    // Validation runs before zero-suppression rule:
    // node_v3 at level 3 > v1's level 1, so this throws
    bddp node_v3 = ZDD::getnode_raw(v3, bddempty, bddsingle);
    EXPECT_THROW(ZDD::getnode(v1, node_v3, bddempty), std::invalid_argument);

    // Zero-suppression with valid inputs: hi == bddempty returns lo
    EXPECT_EQ(ZDD::getnode(v1, bddempty, bddempty), bddempty);
    EXPECT_EQ(ZDD::getnode(v1, bddsingle, bddempty), bddsingle);
}

TEST_F(BDDTest, BddGetnodeClassTypeVersion) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // BDD class type version
    BDD lo = BDD::False;
    BDD hi = BDD::True;
    BDD result = BDD::getnode(v1, lo, hi);
    EXPECT_EQ(result.GetID(), BDD::getnode(v1, bddfalse, bddtrue));

    // ZDD class type version
    ZDD zlo = ZDD::Empty;
    ZDD zhi = ZDD::Single;
    ZDD zresult = ZDD::getnode(v1, zlo, zhi);
    EXPECT_EQ(zresult.GetID(), ZDD::getnode(v1, bddempty, bddsingle));
}

TEST_F(BDDTest, BddGetnodeRawNoValidation) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // getnode_raw does NOT validate levels — no exception
    bddp child_v2 = BDD::getnode_raw(v2, bddfalse, bddtrue);
    EXPECT_NO_THROW(BDD::getnode_raw(v1, child_v2, bddtrue));

    bddp zchild_v2 = ZDD::getnode_raw(v2, bddempty, bddsingle);
    EXPECT_NO_THROW(ZDD::getnode_raw(v1, zchild_v2, bddsingle));
}

// ---------------------------------------------------------------------------
// save_graphviz tests
// ---------------------------------------------------------------------------

TEST_F(BDDTest, SaveGraphviz_BDD_TerminalFalse) {
    std::ostringstream oss;
    BDD::False.save_graphviz(oss);
    std::string dot = oss.str();
    EXPECT_NE(dot.find("digraph {"), std::string::npos);
    EXPECT_NE(dot.find("t0"), std::string::npos);
    EXPECT_EQ(dot.find("t1"), std::string::npos);
    EXPECT_NE(dot.find("}"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_BDD_TerminalTrue) {
    std::ostringstream oss;
    BDD::True.save_graphviz(oss);
    std::string dot = oss.str();
    EXPECT_NE(dot.find("t1"), std::string::npos);
    EXPECT_EQ(dot.find("t0"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_ZDD_TerminalEmpty) {
    std::ostringstream oss;
    ZDD::Empty.save_graphviz(oss);
    std::string dot = oss.str();
    EXPECT_NE(dot.find("digraph {"), std::string::npos);
    EXPECT_NE(dot.find("t0"), std::string::npos);
    EXPECT_EQ(dot.find("t1"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_ZDD_TerminalSingle) {
    std::ostringstream oss;
    ZDD::Single.save_graphviz(oss);
    std::string dot = oss.str();
    EXPECT_NE(dot.find("t1"), std::string::npos);
    EXPECT_EQ(dot.find("t0"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_BDD_SingleVar_Expanded) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    std::ostringstream oss;
    x1.save_graphviz(oss);
    std::string dot = oss.str();
    // Should have digraph, one non-terminal node, two terminals
    EXPECT_NE(dot.find("digraph {"), std::string::npos);
    EXPECT_NE(dot.find("t0"), std::string::npos);
    EXPECT_NE(dot.find("t1"), std::string::npos);
    // Node should be labeled with variable number
    std::string var_label = "label = \"" + std::to_string(v1) + "\"";
    EXPECT_NE(dot.find(var_label), std::string::npos);
    // Lo edge should be dotted
    EXPECT_NE(dot.find("style = dotted"), std::string::npos);
    // Color should be present
    EXPECT_NE(dot.find("#81B65D"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_BDD_ComplementExpanded) {
    bddvar v1 = bddnewvar();
    BDD x1 = ~BDD::prime(v1);
    std::ostringstream oss;
    x1.save_graphviz(oss, DrawMode::Expanded);
    std::string dot = oss.str();
    // Should NOT have complement markers (arrowtail)
    EXPECT_EQ(dot.find("arrowtail"), std::string::npos);
    // Should still have valid structure
    EXPECT_NE(dot.find("digraph {"), std::string::npos);
    EXPECT_NE(dot.find("t0"), std::string::npos);
    EXPECT_NE(dot.find("t1"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_BDD_ComplementRaw) {
    bddvar v1 = bddnewvar();
    BDD x1 = ~BDD::prime(v1);
    std::ostringstream oss;
    x1.save_graphviz(oss, DrawMode::Raw);
    std::string dot = oss.str();
    // Raw mode with complement root should have entry point with arrowtail
    EXPECT_NE(dot.find("entry"), std::string::npos);
    EXPECT_NE(dot.find("arrowtail = odot"), std::string::npos);
    EXPECT_NE(dot.find("dir = both"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_BDD_MultiVar_Expanded) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD f = BDD::prime(v1) & BDD::prime(v2);
    std::ostringstream oss;
    f.save_graphviz(oss);
    std::string dot = oss.str();
    // Should have both variable labels
    std::string label1 = "label = \"" + std::to_string(v1) + "\"";
    std::string label2 = "label = \"" + std::to_string(v2) + "\"";
    EXPECT_NE(dot.find(label1), std::string::npos);
    EXPECT_NE(dot.find(label2), std::string::npos);
    // Should have rank constraints
    EXPECT_NE(dot.find("rank = same"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_ZDD_SingleVar_Expanded) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD::singleton(v1);
    std::ostringstream oss;
    z.save_graphviz(oss);
    std::string dot = oss.str();
    EXPECT_NE(dot.find("digraph {"), std::string::npos);
    std::string var_label = "label = \"" + std::to_string(v1) + "\"";
    EXPECT_NE(dot.find(var_label), std::string::npos);
    EXPECT_NE(dot.find("t0"), std::string::npos);
    EXPECT_NE(dot.find("t1"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_BDD_RawNoComplement) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    std::ostringstream oss;
    x1.save_graphviz(oss, DrawMode::Raw);
    std::string dot = oss.str();
    // Non-complemented BDD in raw mode should NOT have entry point or arrowtail
    EXPECT_EQ(dot.find("entry"), std::string::npos);
    EXPECT_EQ(dot.find("arrowtail"), std::string::npos);
    EXPECT_NE(dot.find("digraph {"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_BDD_FileOverload) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);

    // ostream version
    std::ostringstream oss;
    x1.save_graphviz(oss);

    // FILE* version
    FILE* tmp = tmpfile();
    ASSERT_NE(tmp, nullptr);
    x1.save_graphviz(tmp);
    long sz = ftell(tmp);
    rewind(tmp);
    std::string file_content(sz, '\0');
    size_t nread = fread(&file_content[0], 1, sz, tmp);
    file_content.resize(nread);
    fclose(tmp);

    EXPECT_EQ(oss.str(), file_content);
}

TEST_F(BDDTest, SaveGraphviz_ZDD_ComplementRaw) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // power_set has complement edges internally
    ZDD z = ZDD::power_set(v2);
    std::ostringstream oss_exp;
    z.save_graphviz(oss_exp, DrawMode::Expanded);
    std::ostringstream oss_raw;
    z.save_graphviz(oss_raw, DrawMode::Raw);
    // Both should be valid DOT
    EXPECT_NE(oss_exp.str().find("digraph {"), std::string::npos);
    EXPECT_NE(oss_raw.str().find("digraph {"), std::string::npos);
}

TEST_F(BDDTest, SaveGraphviz_BDD_Null) {
    // BDD::Null should produce empty output
    std::ostringstream oss;
    BDD::Null.save_graphviz(oss);
    EXPECT_TRUE(oss.str().empty());
}

// --- Variable reordering: count / exact_count / uniform_sample ---

TEST_F(BDDTest, CountWithReorderedVariables) {
    // Setup: var1->level2, var2->level3, var3->level1
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddnewvaroflev(1);  // var3 at level 1

    EXPECT_EQ(var2level[v1], 2u);
    EXPECT_EQ(var2level[v2], 3u);

    BDD f = BDDvar(v2);  // x2

    // x2 over {x1, x2}: satisfying = {(0,1),(1,1)} = 2
    EXPECT_DOUBLE_EQ(f.count(2), 2.0);
    // x2 over {x1, x2, x3}: satisfying = 4
    EXPECT_DOUBLE_EQ(f.count(3), 4.0);
}

TEST_F(BDDTest, ExactCountWithReorderedVariables) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddnewvaroflev(1);  // var3 at level 1

    BDD f = BDDvar(v2);  // x2

    // x2 over {x1, x2}: 2 satisfying assignments
    EXPECT_EQ(f.exact_count(2), bigint::BigInt(2));
    // x2 over {x1, x2, x3}: 4 satisfying assignments
    EXPECT_EQ(f.exact_count(3), bigint::BigInt(4));

    // x1 AND x2 over {x1, x2}: 1 satisfying assignment
    BDD g = BDDvar(v1) & BDDvar(v2);
    EXPECT_EQ(g.exact_count(2), bigint::BigInt(1));

    // x1 OR x2 over {x1, x2}: 3 satisfying assignments
    BDD h = BDDvar(v1) | BDDvar(v2);
    EXPECT_EQ(h.exact_count(2), bigint::BigInt(3));
}

TEST_F(BDDTest, UniformSampleWithReorderedVariables) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddnewvaroflev(1);  // var3 at level 1

    BDD f = BDDvar(v2);  // x2, n=2

    std::mt19937_64 rng(42);
    BddCountMemo memo(f.id(), 2);

    for (int i = 0; i < 100; ++i) {
        auto sample = f.uniform_sample(rng, 2, memo);
        // v2 must always be in the sample (x2=1 required)
        EXPECT_NE(std::find(sample.begin(), sample.end(), v2), sample.end());
        // No variable outside domain {1, 2} should appear
        for (bddvar v : sample) {
            EXPECT_LE(v, 2u) << "out-of-domain variable " << v << " in sample";
        }
    }
}

TEST_F(BDDTest, BDD_UniformSample_NExceedsVarUsed) {
    bddvar v1 = bddnewvar();
    (void)v1;
    BDD f = BDD::True;
    std::mt19937_64 rng(42);
    BddCountMemo memo(f.id(), 5);
    EXPECT_THROW(f.uniform_sample(rng, 5, memo), std::invalid_argument);
}

// --- ZDD_Import error handling ---

TEST_F(BDDTest, ZDD_ImportIstreamInvalidInputThrows) {
    std::istringstream bad("XXX");
    EXPECT_THROW(ZDD_Import(bad), std::runtime_error);
}

TEST_F(BDDTest, ZDD_ImportFilePtrInvalidInputThrows) {
    FILE* tmp = tmpfile();
    ASSERT_NE(tmp, nullptr);
    fprintf(tmp, "XXX");
    rewind(tmp);
    EXPECT_THROW(ZDD_Import(tmp), std::runtime_error);
    fclose(tmp);
}

// --- Negative count validation for bddnewvar / DDBase::new_var ---

TEST_F(BDDTest, BddNewVarNegativeCountThrows) {
    EXPECT_THROW(bddnewvar(-1), std::invalid_argument);
}

TEST_F(BDDTest, DDBaseNewVarNegativeCountThrows) {
    EXPECT_THROW(DDBase::new_var(-1, false), std::invalid_argument);
    EXPECT_THROW(DDBase::new_var(-1, true), std::invalid_argument);
}

TEST_F(BDDTest, BddNewVarZeroCountReturnsEmpty) {
    auto vars = bddnewvar(0);
    EXPECT_TRUE(vars.empty());
}

TEST_F(BDDTest, DDBaseNewVarZeroCountReturnsEmpty) {
    auto vars = DDBase::new_var(0, false);
    EXPECT_TRUE(vars.empty());
}

// --- ZDD::has_empty / enumerate on Null ---

TEST_F(BDDTest, ZDD_HasEmpty_Null) {
    ZDD n(-1);
    EXPECT_FALSE(n.has_empty());
}

TEST_F(BDDTest, ZDD_Enumerate_Null) {
    ZDD n(-1);
    auto sets = n.enumerate();
    EXPECT_TRUE(sets.empty());
}

// --- ZDD::single_set / from_sets duplicate elements ---

TEST_F(BDDTest, ZDD_SingleSet_DuplicateElements) {
    bddnewvar(); bddnewvar();
    // {1, 1} should deduplicate to {1}, not toggle to {}
    ZDD f = ZDD::single_set({1, 1});
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_EQ(sets[0], std::vector<bddvar>({1}));
}

TEST_F(BDDTest, ZDD_FromSets_DuplicateElements) {
    bddnewvar(); bddnewvar();
    // {{1, 2, 2}} should deduplicate to {{1, 2}}, not {{1}}
    ZDD f = ZDD::from_sets({{1, 2, 2}});
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    std::vector<bddvar> expected = {1, 2};
    EXPECT_EQ(sets[0], expected);
}

// --- ZDD::combination with reordered variables ---

TEST_F(BDDTest, ZDD_Combination_Reordered) {
    bddnewvar(); bddnewvar();
    bddnewvaroflev(1);  // var3 at level 1, shifts var1→lev2, var2→lev3

    // C(3,2) = {{1,2}, {1,3}, {2,3}} = 3 sets
    ZDD f = ZDD::combination(3, 2);
    EXPECT_EQ(f.Card(), 3u);

    // Verify ZDD structural invariant: parent level > child level
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 3u);
    // Each inner set should be sorted ascending
    for (auto& s : sets) {
        for (size_t i = 1; i < s.size(); ++i) {
            EXPECT_LT(s[i-1], s[i]);
        }
    }
}

// --- getnode rejects bddnull children ---

TEST_F(BDDTest, BddGetnodeRejectsBddnullChild) {
    bddvar v1 = bddnewvar();
    EXPECT_THROW(BDD::getnode(v1, bddnull, bddtrue), std::invalid_argument);
    EXPECT_THROW(BDD::getnode(v1, bddfalse, bddnull), std::invalid_argument);
    // Validation runs before reduction rule: bddnull is always rejected
    EXPECT_THROW(BDD::getnode(v1, bddnull, bddnull), std::invalid_argument);
}

TEST_F(BDDTest, ZddGetnodeRejectsBddnullChild) {
    bddvar v1 = bddnewvar();
    EXPECT_THROW(ZDD::getnode(v1, bddnull, bddsingle), std::invalid_argument);
    EXPECT_THROW(ZDD::getnode(v1, bddempty, bddnull), std::invalid_argument);
    EXPECT_THROW(ZDD::getnode(v1, bddnull, bddnull), std::invalid_argument);
}

/* ---- RecurGuard in counting functions ---- */
TEST_F(BDDTest, RecurGuard_CountingFunctions) {
    /* Verify RecurCount returns to 0 after counting operations,
       confirming BDD_RecurGuard is properly placed. */
    for (int i = 0; i < 10; i++) bddnewvar();
    bddp f = bddsingle;
    for (bddvar v = 1; v <= 10; v++) {
        f = ZDD::getnode(v, f, bddsingle);
    }
    EXPECT_EQ(BDD_RecurCount, 0);
    bddcard(f);
    EXPECT_EQ(BDD_RecurCount, 0);
    bddlit(f);
    EXPECT_EQ(BDD_RecurCount, 0);
    bddlen(f);
    EXPECT_EQ(BDD_RecurCount, 0);
    bddcount(f);
    EXPECT_EQ(BDD_RecurCount, 0);
    bddexactcount(f);
    EXPECT_EQ(BDD_RecurCount, 0);
}

/* ---- binary_import_multi_core bounds checks ---- */
TEST_F(BDDTest, BinaryImport_MaxLevelExceedsLimit) {
    /* Craft a minimal binary header with max_level = UINT64_MAX */
    std::string data;
    /* Magic: BDD */
    data += std::string("\x42\x44\x44", 3);
    /* Header: 91 bytes */
    uint8_t header[91] = {};
    header[0] = 1;  // version
    header[1] = 2;  // dd_type = BDD
    /* num_arcs = 2 (little-endian 16-bit) */
    header[2] = 2; header[3] = 0;
    /* num_terminals = 2 (little-endian 32-bit) */
    header[4] = 2; header[5] = 0; header[6] = 0; header[7] = 0;
    /* bits_for_level = 64 */
    header[8] = 64;
    /* bits_for_id = 8 */
    header[9] = 8;
    /* use_neg = 0 */
    header[10] = 0;
    /* max_level = UINT64_MAX (little-endian 64-bit) */
    for (int i = 0; i < 8; i++) header[11 + i] = 0xFF;
    /* num_roots = 1 (little-endian 64-bit) */
    header[19] = 1;
    data.append(reinterpret_cast<char*>(header), 91);
    std::istringstream iss(data);
    EXPECT_THROW(bdd_import_binary(iss), std::runtime_error);
}

TEST_F(BDDTest, BinaryImport_BitsForIdOver64Rejected) {
    // bits_for_id > 64 previously passed the % 8 check and caused
    // read_id_le to overrun its fixed 8-byte buffer.
    std::string data;
    data += std::string("\x42\x44\x44", 3);
    uint8_t header[91] = {};
    header[0] = 1;
    header[1] = 2;
    header[2] = 2; header[3] = 0;
    header[4] = 2; header[5] = 0; header[6] = 0; header[7] = 0;
    header[8] = 64;
    header[9] = 72;  // > 64 but multiple of 8 — must be rejected
    header[10] = 0;
    // max_level = 0, num_roots = 0
    data.append(reinterpret_cast<char*>(header), 91);
    std::istringstream iss(data);
    EXPECT_THROW(bdd_import_binary(iss), std::runtime_error);
}

TEST_F(BDDTest, BinaryImport_RejectsNumTerminalsNotTwo) {
    // BDD/ZDD/QDD must have exactly 2 terminals; reject 3 to prevent
    // allocating buffers sized from a malformed count.
    std::string data;
    data += std::string("\x42\x44\x44", 3);
    uint8_t header[91] = {};
    header[0] = 1;
    header[1] = 2;  // BDD
    header[2] = 2; header[3] = 0;
    header[4] = 3; header[5] = 0; header[6] = 0; header[7] = 0;  // num_terminals = 3
    header[8] = 64;
    header[9] = 8;
    header[10] = 0;
    data.append(reinterpret_cast<char*>(header), 91);
    std::istringstream iss(data);
    EXPECT_THROW(bdd_import_binary(iss), std::runtime_error);
}

// --- BDD/ZDD::getnode child validation ---

TEST_F(BDDTest, GetnodeRejectsInvalidChildNode) {
    bddnewvar();
    bddp fake_node = 0x100;  // not a valid allocated node
    EXPECT_THROW(BDD::getnode(1, fake_node, bddfalse), std::invalid_argument);
    EXPECT_THROW(BDD::getnode(1, bddfalse, fake_node), std::invalid_argument);
}

TEST_F(BDDTest, ZddGetnodeRejectsInvalidChildNode) {
    bddnewvar();
    bddp fake_node = 0x100;
    EXPECT_THROW(ZDD::getnode(1, fake_node, bddtrue), std::invalid_argument);
    EXPECT_THROW(ZDD::getnode(1, bddfalse, fake_node), std::invalid_argument);
}

// --- ZDD_Random level validation ---

TEST_F(BDDTest, ZDD_Random_ThrowsOnInvalidLevel) {
    bddnewvar();
    bddnewvar();
    EXPECT_THROW(ZDD_Random(100, 50), std::invalid_argument);
}

// --- bddsymset / bddcoimplyset terminal safety ---

TEST_F(BDDTest, SymSetTerminalSafety) {
    bddnewvar();
    bddnewvar();
    // bddsymset on terminal ZDDs should not crash
    EXPECT_EQ(bddsymset(bddempty, 1), bddempty);
    EXPECT_EQ(bddsymset(bddsingle, 1), bddempty);
}

TEST_F(BDDTest, CoImplySetTerminalSafety) {
    bddnewvar();
    bddnewvar();
    EXPECT_EQ(bddcoimplyset(bddempty, 1), bddempty);
    EXPECT_EQ(bddcoimplyset(bddsingle, 1), bddempty);
}

