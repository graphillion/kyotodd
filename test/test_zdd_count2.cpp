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

// --- ZDD::Export ---

TEST_F(BDDTest, ZDD_ExportOstream) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    std::ostringstream oss1;
    z.Export(oss1);

    std::ostringstream oss2;
    bddp p = z.GetID();
    bddexport(oss2, &p, 1);

    EXPECT_EQ(oss1.str(), oss2.str());
}

TEST_F(BDDTest, ZDD_ExportFilePtr) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    z.Export(tmp);
    long len1 = std::ftell(tmp);

    std::rewind(tmp);
    // Verify roundtrip via ZDD_Import
    ZDD imported = ZDD_Import(tmp);
    std::fclose(tmp);
    EXPECT_GT(len1, 0);
    EXPECT_EQ(imported, z);
}

// --- bddispoly / ZDD::IsPoly ---

TEST(ZDD_IsPolyTest, EmptyAndSingle) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Empty family: 0 sets -> not poly
    EXPECT_EQ(bddispoly(bddempty), 0);
    // Single family {∅}: 1 set -> not poly
    EXPECT_EQ(bddispoly(bddsingle), 0);
    // Null -> error
    EXPECT_EQ(bddispoly(bddnull), -1);
}

TEST(ZDD_IsPolyTest, SingleSet) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {v1} = single set containing only variable 1 -> not poly
    bddp s1 = bddchange(bddsingle, 1);
    EXPECT_EQ(bddispoly(s1), 0);

    // {v1, v2} = single set -> not poly
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    EXPECT_EQ(bddispoly(s12), 0);
}

TEST(ZDD_IsPolyTest, MultipleSets) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}} = 2 sets -> poly
    bddp s1 = bddchange(bddsingle, 1);
    bddp s2 = bddchange(bddsingle, 2);
    bddp f = bddunion(s1, s2);
    EXPECT_EQ(bddispoly(f), 1);

    // {{∅}, {v1}} = 2 sets -> poly
    bddp g = bddunion(bddsingle, s1);
    EXPECT_EQ(bddispoly(g), 1);
}

TEST(ZDD_IsPolyTest, ClassWrapper) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    ZDD empty = ZDD::Empty;
    EXPECT_EQ(empty.IsPoly(), 0);

    ZDD single = ZDD::Single;
    EXPECT_EQ(single.IsPoly(), 0);

    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD f = s1 + s2;
    EXPECT_EQ(f.IsPoly(), 1);
}

// --- bddswapz / ZDD::Swap ---

TEST(ZDD_SwapTest, SameVariable) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    ZDD s1 = ZDD::Single.Change(1);
    EXPECT_EQ(s1.Swap(1, 1), s1);
}

TEST(ZDD_SwapTest, SwapSingletons) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {v1} swapped(1,2) = {v2}
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    EXPECT_EQ(s1.Swap(1, 2), s2);
    EXPECT_EQ(s2.Swap(1, 2), s1);
}

TEST(ZDD_SwapTest, BothPresent) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}} swapped(1,2) = {{v1, v2}} (unchanged)
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    EXPECT_EQ(s12.Swap(1, 2), s12);
}

TEST(ZDD_SwapTest, MixedFamily) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2, v3}} swapped(1,2) = {{v2}, {v1, v3}}
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s23 = ZDD::Single.Change(2).Change(3);
    ZDD f = s1 + s23;

    ZDD s2 = ZDD::Single.Change(2);
    ZDD s13 = ZDD::Single.Change(1).Change(3);
    ZDD expected = s2 + s13;

    EXPECT_EQ(f.Swap(1, 2), expected);
}

TEST(ZDD_SwapTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Empty family: swap is still empty
    EXPECT_EQ(bddswapz(bddempty, 1, 2), bddempty);
    // Single family {∅}: no variables, swap is identity
    EXPECT_EQ(bddswapz(bddsingle, 1, 2), bddsingle);
    // Null -> null
    EXPECT_EQ(bddswapz(bddnull, 1, 2), bddnull);
}

TEST(ZDD_SwapTest, Involution) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Swap is an involution: swap(swap(f, v1, v2), v1, v2) == f
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s23 = ZDD::Single.Change(2).Change(3);
    ZDD f = s1 + s23;

    EXPECT_EQ(f.Swap(1, 2).Swap(1, 2), f);
}

// --- bddimplychk / ZDD::ImplyChk ---

TEST(ZDD_ImplyChkTest, TrivialCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // v1 == v2: always true
    ZDD f = ZDD::Single.Change(1);
    EXPECT_EQ(f.ImplyChk(1, 1), 1);
    // Empty family: trivially true
    EXPECT_EQ(ZDD::Empty.ImplyChk(1, 2), 1);
    // Single family {∅}: trivially true (no set contains v1)
    EXPECT_EQ(ZDD::Single.ImplyChk(1, 2), 1);
    // Null -> error
    EXPECT_EQ(bddimplychk(bddnull, 1, 2), -1);
}

TEST(ZDD_ImplyChkTest, ImplicationHolds) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v3}}: every set with v1 also has v2 -> v1 implies v2
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD s3 = ZDD::Single.Change(3);
    ZDD f = s12 + s3;
    EXPECT_EQ(f.ImplyChk(1, 2), 1);
}

TEST(ZDD_ImplyChkTest, ImplicationFails) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: v1 does not imply v2 ({v1} has v1 but not v2)
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD f = s1 + s2;
    EXPECT_EQ(f.ImplyChk(1, 2), 0);
}

// --- bddcoimplychk / ZDD::CoImplyChk ---

TEST(ZDD_CoImplyChkTest, TrivialCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(ZDD::Empty.CoImplyChk(1, 2), 1);
    EXPECT_EQ(ZDD::Single.CoImplyChk(1, 2), 1);
    EXPECT_EQ(bddcoimplychk(bddnull, 1, 2), -1);
}

TEST(ZDD_CoImplyChkTest, ImplicationSubsumesCoImplication) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // If v1 implies v2, co-implication also holds
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD s3 = ZDD::Single.Change(3);
    ZDD f = s12 + s3;
    EXPECT_EQ(f.ImplyChk(1, 2), 1);
    EXPECT_EQ(f.CoImplyChk(1, 2), 1);
}

TEST(ZDD_CoImplyChkTest, CoImplicationHoldsButNotImplication) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v2}}: v1 does not imply v2 (set {v1,v3} has v1 but not v2)
    // f10 = {v3} (sets with v1 but not v2, v1 removed)
    // f01 = {∅}  (sets with v2 but not v1, v2 removed) -- wait, {v2} has v2 but not v1, so f01 = OnSet0(OffSet(f,v1),v2) = {∅}
    // f10 - f01 = {v3} - {∅} = {v3} != empty -> co-implication fails
    ZDD s13 = ZDD::Single.Change(1).Change(3);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD f = s13 + s2;
    EXPECT_EQ(f.ImplyChk(1, 2), 0);
    EXPECT_EQ(f.CoImplyChk(1, 2), 0);

    // {{v1}, {v2}}: f10 = {∅} (v1 only, v1 removed), f01 = {∅} (v2 only, v2 removed)
    // f10 - f01 = {∅} - {∅} = empty -> co-implication holds
    ZDD s1 = ZDD::Single.Change(1);
    ZDD g = s1 + s2;
    EXPECT_EQ(g.ImplyChk(1, 2), 0);
    EXPECT_EQ(g.CoImplyChk(1, 2), 1);
}

// --- bddpermitsym / ZDD::PermitSym ---

TEST(ZDD_PermitSymTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bddpermitsym(bddnull, 3), bddnull);
    EXPECT_EQ(bddpermitsym(bddempty, 3), bddempty);
    EXPECT_EQ(bddpermitsym(bddsingle, 3), bddsingle);
}

TEST(ZDD_PermitSymTest, NZero) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // n=0: only keep ∅ if present
    ZDD s1 = ZDD::Single.Change(1);
    ZDD f = ZDD::Single + s1;  // {∅, {v1}}
    EXPECT_EQ(f.PermitSym(0), ZDD::Single);  // only ∅

    // n=0 on family without ∅
    EXPECT_EQ(s1.PermitSym(0), ZDD::Empty);
}

TEST(ZDD_PermitSymTest, FilterBySize) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v1,v2}, {v1,v2,v3}}
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD s123 = ZDD::Single.Change(1).Change(2).Change(3);
    ZDD f = s1 + s12 + s123;

    // n=1: only {v1}
    EXPECT_EQ(f.PermitSym(1), s1);
    // n=2: {v1} and {v1,v2}
    EXPECT_EQ(f.PermitSym(2), s1 + s12);
    // n=3: all
    EXPECT_EQ(f.PermitSym(3), f);
    // n=10: all (n >= max size)
    EXPECT_EQ(f.PermitSym(10), f);
}

// --- bddalways / ZDD::Always ---

TEST(ZDD_AlwaysTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bddnull, bddalways(bddnull));
    // Empty family: no sets -> no common elements
    EXPECT_EQ(bddempty, bddalways(bddempty));
    // {∅}: empty set has no elements -> Always = ∅
    EXPECT_EQ(bddempty, bddalways(bddsingle));
}

TEST(ZDD_AlwaysTest, AllSetsContainVariable) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v1, v2}}: v1 is in all sets -> Always = {v1}
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD f = s1 + s12;
    ZDD expected = ZDD::Single.Change(1);  // {v1}
    EXPECT_EQ(f.Always(), expected);
}

TEST(ZDD_AlwaysTest, NoCommonVariable) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: no common variable -> Always = ∅
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD f = s1 + s2;
    EXPECT_EQ(f.Always(), ZDD::Empty);
}

TEST(ZDD_AlwaysTest, MultipleCommonVariables) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2, v3}, {v1, v2, v4}}: v1 and v2 are common
    // Always returns family of singletons: {{v1}, {v2}}
    ZDD s123 = ZDD::Single.Change(1).Change(2).Change(3);
    ZDD s124 = ZDD::Single.Change(1).Change(2).Change(4);
    ZDD f = s123 + s124;
    ZDD expected = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    EXPECT_EQ(f.Always(), expected);
}

TEST(ZDD_AlwaysTest, WithEmptySet) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {∅, {v1}}: ∅ has no elements -> Always = ∅
    ZDD f = ZDD::Single + ZDD::Single.Change(1);
    EXPECT_EQ(f.Always(), ZDD::Empty);
}

// --- bddsymchk / ZDD::SymChk ---

TEST(ZDD_SymChkTest, TrivialCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bddsymchk(bddnull, 1, 2), -1);
    EXPECT_EQ(ZDD::Empty.SymChk(1, 2), 1);
    EXPECT_EQ(ZDD::Single.SymChk(1, 2), 1);
    // Same variable -> trivially symmetric
    ZDD s1 = ZDD::Single.Change(1);
    EXPECT_EQ(s1.SymChk(1, 1), 1);
}

TEST(ZDD_SymChkTest, SymmetricFamily) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: swapping v1 and v2 gives the same family -> symmetric
    ZDD f = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    EXPECT_EQ(f.SymChk(1, 2), 1);

    // {{v1, v2}}: swapping gives {{v1, v2}} -> symmetric
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    EXPECT_EQ(s12.SymChk(1, 2), 1);
}

TEST(ZDD_SymChkTest, NotSymmetric) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}}: swapping v1,v2 gives {{v2}} != {{v1}} -> not symmetric
    ZDD s1 = ZDD::Single.Change(1);
    EXPECT_EQ(s1.SymChk(1, 2), 0);
}

TEST(ZDD_SymChkTest, ConsistentWithSwap) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Build a variety of families and verify: SymChk(v1,v2)==1 iff Swap(v1,v2)==f
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD s3 = ZDD::Single.Change(3);
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD s13 = ZDD::Single.Change(1).Change(3);
    ZDD s23 = ZDD::Single.Change(2).Change(3);

    ZDD families[] = {
        s1 + s2,          // {{v1}, {v2}}
        s1 + s3,          // {{v1}, {v3}}
        s12 + s3,         // {{v1,v2}, {v3}}
        s13 + s23,        // {{v1,v3}, {v2,v3}}
        s1 + s2 + s12,    // {{v1}, {v2}, {v1,v2}}
    };

    for (const auto& f : families) {
        for (int v1 = 1; v1 <= 3; v1++) {
            for (int v2 = v1 + 1; v2 <= 3; v2++) {
                bool swap_eq = (f.Swap(v1, v2) == f);
                bool sym = (f.SymChk(v1, v2) == 1);
                EXPECT_EQ(swap_eq, sym)
                    << "v1=" << v1 << " v2=" << v2
                    << " swap_eq=" << swap_eq << " sym=" << sym;
            }
        }
    }
}

TEST(ZDD_SymChkTest, IntermediateVariables) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v2, v3}}: v1,v2 symmetric (v3 is intermediate between v1,v2 and both branches have it)
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(2).Change(3);
    EXPECT_EQ(f.SymChk(1, 2), 1);

    // {{v1, v3}, {v2}}: v1,v2 not symmetric
    ZDD g = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(2);
    EXPECT_EQ(g.SymChk(1, 2), 0);
}

// --- bddimplyset / ZDD::ImplySet ---

TEST(ZDD_ImplySetTest, Basic) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v1, v3}}: v1 implies nothing else (v2 sometimes, v3 sometimes)
    ZDD f = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(1).Change(3);
    EXPECT_EQ(f.ImplySet(1), ZDD::Empty);

    // {{v1, v2}, {v3}}: v1 implies v2 (every set with v1 also has v2)
    ZDD g = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(3);
    ZDD expected = ZDD::Single.Change(2);  // {v2} as singleton family
    EXPECT_EQ(g.ImplySet(1), expected);
}

TEST(ZDD_ImplySetTest, ConsistentWithImplyChk) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2, v3}, {v1, v2}}: v1 implies v2
    ZDD f = ZDD::Single.Change(1).Change(2).Change(3) + ZDD::Single.Change(1).Change(2);
    ZDD iset = f.ImplySet(1);

    // Verify: for each variable v != 1, ImplyChk should agree with ImplySet
    for (int v = 2; v <= 5; v++) {
        ZDD sv = ZDD::Single.Change(v);
        bool in_iset = ((iset & sv) != ZDD::Empty);
        bool chk = (f.ImplyChk(1, v) == 1);
        EXPECT_EQ(in_iset, chk) << "v=" << v;
    }
}

// --- bddsymgrp / ZDD::SymGrp ---

TEST(ZDD_SymGrpTest, NoGroups) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}}: no variable pair is symmetric -> empty result
    ZDD f = ZDD::Single.Change(1);
    EXPECT_EQ(f.SymGrp(), ZDD::Empty);
}

TEST(ZDD_SymGrpTest, OneGroup) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: v1 and v2 are symmetric -> group {v1, v2}
    ZDD f = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    ZDD expected = ZDD::Single.Change(1).Change(2);  // {{v1, v2}}
    EXPECT_EQ(f.SymGrp(), expected);
}

TEST(ZDD_SymGrpTest, MultipleGroups) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v1, v4}, {v2, v3}, {v2, v4}}: v1↔v2 symmetric, v3↔v4 symmetric
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(1).Change(4)
          + ZDD::Single.Change(2).Change(3) + ZDD::Single.Change(2).Change(4);

    ZDD grp = f.SymGrp();
    // Should contain {v1,v2} and {v3,v4}
    ZDD g12 = ZDD::Single.Change(1).Change(2);
    ZDD g34 = ZDD::Single.Change(3).Change(4);
    EXPECT_EQ(grp, g12 + g34);
}

// --- bddsymgrpnaive / ZDD::SymGrpNaive ---

TEST(ZDD_SymGrpNaiveTest, IncludesSingletons) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}}: SymGrpNaive includes size-1 groups
    ZDD f = ZDD::Single.Change(1);
    ZDD result = f.SymGrpNaive();
    // Should contain {v1} as a singleton group
    ZDD expected = ZDD::Single.Change(1);
    EXPECT_EQ(result, expected);
}

TEST(ZDD_SymGrpNaiveTest, ConsistentWithSymGrp) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v1, v4}, {v2, v3}, {v2, v4}}
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(1).Change(4)
          + ZDD::Single.Change(2).Change(3) + ZDD::Single.Change(2).Change(4);

    ZDD grp = f.SymGrp();
    ZDD naive = f.SymGrpNaive();

    // SymGrp (size>=2) should be subset of SymGrpNaive (all sizes)
    // For this case, naive should also have {v1,v2} and {v3,v4} (no singletons since all are in groups)
    EXPECT_EQ(grp, naive);
}

// --- bddsymset / ZDD::SymSet ---

TEST(ZDD_SymSetTest, Basic) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: v1 and v2 are symmetric -> SymSet(v1) = {{v2}}
    ZDD f = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    ZDD expected = ZDD::Single.Change(2);
    EXPECT_EQ(f.SymSet(1), expected);
    EXPECT_EQ(f.SymSet(2), ZDD::Single.Change(1));
}

TEST(ZDD_SymSetTest, ConsistentWithSymChk) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v1, v4}, {v2, v3}, {v2, v4}}
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(1).Change(4)
          + ZDD::Single.Change(2).Change(3) + ZDD::Single.Change(2).Change(4);

    // For each variable v, SymSet(v) should contain exactly those w where SymChk(v, w)==1
    for (int v = 1; v <= 4; v++) {
        ZDD sset = f.SymSet(v);
        for (int w = 1; w <= 4; w++) {
            if (w == v) continue;
            ZDD sw = ZDD::Single.Change(w);
            bool in_sset = ((sset & sw) != ZDD::Empty);
            bool chk = (f.SymChk(v, w) == 1);
            EXPECT_EQ(in_sset, chk) << "v=" << v << " w=" << w;
        }
    }
}

TEST(ZDD_SymSetTest, IntermediateVariables) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v2, v3}}: v1 and v2 are symmetric
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(2).Change(3);
    EXPECT_EQ(f.SymSet(1), ZDD::Single.Change(2));
    EXPECT_EQ(f.SymSet(2), ZDD::Single.Change(1));
}

// --- bddcoimplyset / ZDD::CoImplySet ---

TEST(ZDD_CoImplySetTest, Basic) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v3}}: v1 co-implies v2 (ImplyChk is true, so CoImply is also true)
    ZDD f = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(3);
    ZDD cset = f.CoImplySet(1);
    // v2 should be in co-imply set
    EXPECT_NE(cset & ZDD::Single.Change(2), ZDD::Empty);
}

TEST(ZDD_CoImplySetTest, ConsistentWithCoImplyChk) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: v1 co-implies v2 (f10={∅}, f01={∅}, f10-f01=empty)
    ZDD f = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    ZDD cset = f.CoImplySet(1);

    for (int w = 2; w <= 5; w++) {
        ZDD sw = ZDD::Single.Change(w);
        bool in_cset = ((cset & sw) != ZDD::Empty);
        bool chk = (f.CoImplyChk(1, w) == 1);
        EXPECT_EQ(in_cset, chk) << "w=" << w;
    }
}

TEST(ZDD_CoImplySetTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bddcoimplyset(bddnull, 1), bddnull);
    EXPECT_EQ(bddcoimplyset(bddempty, 1), bddempty);
    EXPECT_EQ(bddcoimplyset(bddsingle, 1), bddempty);
}

TEST(ZDD_CoImplySetTest, ExhaustiveConsistencyWithChk) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Test several families where the recursive decomposition hits
    // the f11==bddempty / f10==bddempty branches
    std::vector<ZDD> families;

    // f = {{v1,v3}, {v2,v3}, {v2,v4}}
    families.push_back(
        ZDD::Single.Change(1).Change(3) +
        ZDD::Single.Change(2).Change(3) +
        ZDD::Single.Change(2).Change(4));

    // f = {{v1,v2,v3}, {v3,v4}}
    families.push_back(
        ZDD::Single.Change(1).Change(2).Change(3) +
        ZDD::Single.Change(3).Change(4));

    // f = {{v1}, {v2}, {v3,v4,v5}}
    families.push_back(
        ZDD::Single.Change(1) +
        ZDD::Single.Change(2) +
        ZDD::Single.Change(3).Change(4).Change(5));

    // f = {{v1,v2}, {v1,v3}, {v2,v4}, {v3,v5}}
    families.push_back(
        ZDD::Single.Change(1).Change(2) +
        ZDD::Single.Change(1).Change(3) +
        ZDD::Single.Change(2).Change(4) +
        ZDD::Single.Change(3).Change(5));

    for (const ZDD& f : families) {
        for (bddvar v = 1; v <= 5; v++) {
            ZDD cset = f.CoImplySet(v);
            for (bddvar w = 1; w <= 5; w++) {
                if (w == v) continue;
                ZDD sw = ZDD::Single.Change(w);
                bool in_cset = ((cset & sw) != ZDD::Empty);
                bool chk = (f.CoImplyChk(v, w) == 1);
                EXPECT_EQ(in_cset, chk)
                    << "family=" << f.id()
                    << " v=" << v << " w=" << w;
            }
        }
    }
}

// --- bdddivisor / ZDD::Divisor ---

TEST(ZDD_DivisorTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bdddivisor(bddnull), bddnull);
    EXPECT_EQ(bdddivisor(bddempty), bddempty);
    // Single set (monomial) -> returns single (unit element)
    ZDD s1 = ZDD::Single.Change(1);
    EXPECT_EQ(s1.Divisor(), ZDD::Single);
}

TEST(ZDD_DivisorTest, SimplePolynomial) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v1, v3}}: restricting by v1 gives {{v2}, {v3}} which is still polynomial
    // So divisor should be {{v2}, {v3}} or a further restriction
    ZDD f = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(1).Change(3);
    ZDD d = f.Divisor();
    // Divisor should have ≥ 2 sets (still polynomial) or be the end result
    EXPECT_EQ(d.IsPoly(), 1);
}

TEST(ZDD_DivisorTest, DivisorDivides) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v1, v3}}: the divisor should divide the original
    ZDD f = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(1).Change(3);
    ZDD d = f.Divisor();
    // f / d should not be empty (d is a factor)
    ZDD q = f / d;
    EXPECT_NE(q, ZDD::Empty);
    // q * d should give back f (or a subset of f)
    EXPECT_EQ(q * d, f);
}

TEST(ZDD_DivisorTest, CounterexampleThreeSets) {
    // Counterexample from review: f = {{v1}, {v2}, {v1,v2}}
    // Divisor should either return d where q*d == f, or bddsingle
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD f = s1 + s2 + s12;

    ZDD d = f.Divisor();
    ZDD q = f / d;
    EXPECT_EQ(q * d, f);
}

TEST(ZDD_PermitSymTest, NegativeN) {
    // n < 0: no set has negative cardinality, should return empty
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(ZDD::Single.PermitSym(-1), ZDD::Empty);

    ZDD s1 = ZDD::Single.Change(1);
    ZDD f = ZDD::Single + s1;  // {∅, {v1}}
    EXPECT_EQ(f.PermitSym(-1), ZDD::Empty);
    EXPECT_EQ(f.PermitSym(-100), ZDD::Empty);
}

// --- ZDD::Meet ---

TEST_F(BDDTest, ZDD_Meet_MatchesFreeFunction) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;  // {{v1}, {v2}}

    EXPECT_EQ(F.Meet(z_v1), ZDD_Meet(F, z_v1));
}

TEST_F(BDDTest, ZDD_Meet_WithSelf) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    EXPECT_EQ(z.Meet(z), z);
}

TEST_F(BDDTest, ZDD_Meet_WithEmpty) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD e(0);

    EXPECT_EQ(z.Meet(e), e);
}

TEST_F(BDDTest, ZDD_Meet_WithSingle) {
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD s(1);  // {{}}

    EXPECT_EQ(z_v1.Meet(s), s);
}

// --- bddimport invalid input tests ---

TEST_F(BDDTest, ImportEmptyStream) {
    std::istringstream empty("");
    bddp p = bddnull;
    int ret = bddimport(empty, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportMalformedHeader) {
    std::istringstream bad("garbage data\n");
    bddp p = bddnull;
    int ret = bddimport(bad, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportMissingNodeSection) {
    std::istringstream bad("_i 2\n_o 1\n_n 1\n");
    bddp p = bddnull;
    int ret = bddimport(bad, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportzEmptyStream) {
    std::istringstream empty("");
    bddp p = bddnull;
    int ret = bddimportz(empty, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportRejectsTrailingGarbageInTerminal) {
    // "FZ" should not be accepted as bddfalse
    std::istringstream ss("_i 1\n_o 1\n_n 0\nFZ\n");
    bddp p = bddnull;
    int ret = bddimport(ss, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportRejectsTrailingGarbageInNodeId) {
    // "2abc" should not be accepted as node ID 2
    bddvar v1 = bddnewvar();
    (void)v1;
    std::istringstream ss("_i 1\n_o 1\n_n 1\n2 1 F T\n2abc\n");
    bddp p = bddnull;
    int ret = bddimport(ss, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportRejectsDuplicateNodeId) {
    bddvar v1 = bddnewvar();
    (void)v1;
    // Two nodes with the same old_id = 2
    std::istringstream ss("_i 1\n_o 1\n_n 2\n2 1 F T\n2 1 T F\nT\n");
    bddp p = bddnull;
    int ret = bddimport(ss, &p, 1);
    EXPECT_LT(ret, 0);
}

// --- Garbage Collection ---

TEST_F(BDDTest, GCManualCallReducesNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // Create some nodes without protecting them
    {
        BDD a = BDDvar(v1) & BDDvar(v2);
        BDD b = BDDvar(v1) | BDDvar(v2);
        // a, b are protected via BDD class
    }
    // a, b destructors ran; nodes are now unreachable
    uint64_t before = bddlive();
    bddgc();
    uint64_t after = bddlive();
    EXPECT_LE(after, before);
}

TEST_F(BDDTest, GCProtectUnprotect) {
    bddvar v1 = bddnewvar();
    bddp node = bddand(BDDvar(v1).GetID(), bddtrue);
    bddgc_protect(&node);
    EXPECT_EQ(bddgc_rootcount(), 1u);
    bddgc();
    // node should still be valid after GC
    EXPECT_EQ(bddtop(node), v1);
    bddgc_unprotect(&node);
    EXPECT_EQ(bddgc_rootcount(), 0u);
}

TEST_F(BDDTest, GCThreshold) {
    double orig = bddgc_getthreshold();
    EXPECT_DOUBLE_EQ(orig, 0.9);
    bddgc_setthreshold(0.5);
    EXPECT_DOUBLE_EQ(bddgc_getthreshold(), 0.5);
    bddgc_setthreshold(0.9);
}

// --- bddfinal ---

TEST_F(BDDTest, BddFinalReleasesResources) {
    bddvar v1 = bddnewvar();
    (void)v1;
    bddfinal();
    // After bddfinal, re-init should work
    bddinit(256, UINT64_MAX);
    bddvar v2 = bddnewvar();
    EXPECT_EQ(v2, 1u);
}

TEST_F(BDDTest, BddFinalDoubleCallSafe) {
    bddfinal();
    bddfinal();  // should not crash
    bddinit(256, UINT64_MAX);
}

TEST_F(BDDTest, BddFinalResetState) {
    bddnewvar();
    bddgc_setthreshold(0.5);
    bddfinal();
    bddinit(256, UINT64_MAX);
    // Threshold should be reset to default
    EXPECT_DOUBLE_EQ(bddgc_getthreshold(), 0.9);
    // Variable count should be 0
    EXPECT_EQ(bddvarused(), 0u);
}

// --- Memory pressure ---

TEST_F(BDDTest, NodeMaxLimitTriggersGC) {
    // Re-init with small node_max to force GC
    bddfinal();
    bddinit(16, 64);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // Create many nodes in a loop; GC should keep things running
    for (int i = 0; i < 100; i++) {
        BDD a = BDDvar(v1) & BDDvar(v2);
        BDD b = a | BDDvar(v3);
        (void)b;
    }
    // If we got here without overflow, GC is working
}

TEST_F(BDDTest, NodeMaxExhaustion) {
    bddfinal();
    bddinit(4, 4);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddvar v4 = bddnewvar();
    // With only 4 slots and many variables, eventually nodes exhaust
    EXPECT_THROW({
        BDD a = BDDvar(v1) & BDDvar(v2) & BDDvar(v3) & BDDvar(v4);
        BDD b = BDDvar(v1) | BDDvar(v2) | BDDvar(v3) | BDDvar(v4);
        BDD c = a ^ b;
        (void)c;
    }, std::overflow_error);
}

TEST_F(BDDTest, BddfinalWithLiveObjectsThrows) {
    bddvar v = bddnewvar();
    {
        BDD x = BDDvar(v);
        EXPECT_THROW(bddfinal(), std::runtime_error);
        // x is still valid
        EXPECT_EQ(x.Top(), v);
    }
    // After x is destroyed, bddfinal succeeds
    bddfinal();
}

TEST_F(BDDTest, BddinitWithLiveObjectsThrows) {
    bddvar v = bddnewvar();
    {
        BDD x = BDDvar(v);  // non-terminal node
        EXPECT_THROW(bddinit(256, UINT64_MAX), std::runtime_error);
        (void)x;
    }
}

// --- bddplainsize / plain_size ---

TEST_F(BDDTest, PlainSize_Terminals) {
    EXPECT_EQ(bddplainsize(bddfalse, false), 0u);
    EXPECT_EQ(bddplainsize(bddtrue, false), 0u);
    EXPECT_EQ(bddplainsize(bddnull, false), 0u);
    EXPECT_EQ(bddplainsize(bddempty, true), 0u);
    EXPECT_EQ(bddplainsize(bddsingle, true), 0u);
}

TEST_F(BDDTest, PlainSize_BDD_NoComplement) {
    // A single variable BDD (no complement edges) should have same plain_size and Size
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    EXPECT_EQ(x.plain_size(), x.Size());
}

TEST_F(BDDTest, PlainSize_BDD_WithComplement) {
    // ~x uses a complement edge, so plain_size should be >= Size
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    BDD nx = ~x;
    // x and ~x share the same physical node, so Size is 1 for both
    EXPECT_EQ(x.Size(), 1u);
    EXPECT_EQ(nx.Size(), 1u);
    // plain_size of ~x: the root is complemented, so both children are
    // flipped. This may create a distinct logical node.
    EXPECT_GE(nx.plain_size(), nx.Size());
}

TEST_F(BDDTest, PlainSize_BDD_Complex) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;     // AND: no complement edges needed
    BDD g = ~(a & b);  // NAND: complement at root
    // NAND has same physical nodes as AND, but plain_size may differ
    EXPECT_EQ(f.Size(), g.Size());
    EXPECT_GE(g.plain_size(), g.Size());
}

TEST_F(BDDTest, PlainSize_ZDD_NoComplement) {
    bddvar v = bddnewvar();
    bddp z = bddchange(bddsingle, v);  // {{v}}
    EXPECT_EQ(bddplainsize(z, true), bddsize(z));
}

TEST_F(BDDTest, PlainSize_ZDD_WithComplement) {
    bddvar v = bddnewvar();
    bddp z = bddchange(bddsingle, v);  // {{v}}
    bddp nz = z ^ BDD_COMP_FLAG;       // complement edge
    // ZDD complement only affects lo child
    EXPECT_GE(bddplainsize(nz, true), bddsize(nz));
}

TEST_F(BDDTest, PlainSize_ZDD_MemberFunction) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(bddchange(bddsingle, v1));
    ZDD b = ZDD_ID(bddchange(bddsingle, v2));
    ZDD u = a + b;  // {{v1}, {v2}}
    EXPECT_GE(u.plain_size(), 1u);
    // plain_size >= Size always
    EXPECT_GE(u.plain_size(), u.Size());
}

// --- bddrawsize / bddplainsize (vector) ---

TEST_F(BDDTest, RawSize_Vector_BDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    BDD g = a | b;
    // Shared count should be <= sum of individual counts
    std::vector<BDD> vec = {f, g};
    EXPECT_LE(BDD::raw_size(vec), f.raw_size() + g.raw_size());
    // Shared count >= max of individual counts
    EXPECT_GE(BDD::raw_size(vec),
              std::max(f.raw_size(), g.raw_size()));
}

TEST_F(BDDTest, RawSize_Vector_ZDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(bddchange(bddsingle, v1));
    ZDD b = ZDD_ID(bddchange(bddsingle, v2));
    ZDD u = a + b;
    ZDD c = a * b;
    std::vector<ZDD> vec = {u, c};
    EXPECT_LE(ZDD::raw_size(vec), u.raw_size() + c.raw_size());
    EXPECT_GE(ZDD::raw_size(vec),
              std::max(u.raw_size(), c.raw_size()));
}

TEST_F(BDDTest, PlainSize_Vector_BDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    BDD g = ~(a & b);
    std::vector<BDD> vec = {f, g};
    EXPECT_GE(BDD::plain_size(vec), BDD::raw_size(vec));
}

TEST_F(BDDTest, PlainSize_Vector_ZDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(bddchange(bddsingle, v1));
    ZDD b = ZDD_ID(bddchange(bddsingle, v2));
    ZDD u = a + b;
    ZDD c = a * b;
    std::vector<ZDD> vec = {u, c};
    EXPECT_GE(ZDD::plain_size(vec), ZDD::raw_size(vec));
}

TEST_F(BDDTest, RawSize_Vector_Empty) {
    std::vector<BDD> empty_bdd;
    EXPECT_EQ(BDD::raw_size(empty_bdd), 0u);
    std::vector<ZDD> empty_zdd;
    EXPECT_EQ(ZDD::raw_size(empty_zdd), 0u);
}

TEST_F(BDDTest, RawSize_Vector_bddp) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    bddp g = bddor(bddprime(v1), bddprime(v2));
    std::vector<bddp> vec = {f, g};
    EXPECT_LE(bddrawsize(vec), bddsize(f) + bddsize(g));
}

// --- BDD satisfiability counting (double) ---

TEST_F(BDDTest, BDDCount_False) {
    EXPECT_EQ(bddcount(bddfalse, (bddvar)0), 0.0);
    EXPECT_EQ(bddcount(bddfalse, (bddvar)3), 0.0);
}

TEST_F(BDDTest, BDDCount_True) {
    EXPECT_EQ(bddcount(bddtrue, (bddvar)0), 1.0);
    EXPECT_EQ(bddcount(bddtrue, (bddvar)3), 8.0);
    EXPECT_EQ(bddcount(bddtrue, (bddvar)10), 1024.0);
}

TEST_F(BDDTest, BDDCount_SingleVar) {
    bddvar v1 = bddnewvar();
    bddp x1 = bddprime(v1);
    EXPECT_EQ(bddcount(x1, (bddvar)1), 1.0);
    bddnewvar(); bddnewvar();
    EXPECT_EQ(bddcount(x1, (bddvar)3), 4.0);
}

TEST_F(BDDTest, BDDCount_NotVar) {
    bddvar v1 = bddnewvar();
    bddp x1 = bddprime(v1);
    bddp not_x1 = bddnot(x1);
    EXPECT_EQ(bddcount(not_x1, (bddvar)1), 1.0);
    bddnewvar(); bddnewvar();
    EXPECT_EQ(bddcount(not_x1, (bddvar)3), 4.0);
}

TEST_F(BDDTest, BDDCount_And) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddcount(f, (bddvar)2), 1.0);
}

TEST_F(BDDTest, BDDCount_Or) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddor(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddcount(f, (bddvar)2), 3.0);
}

TEST_F(BDDTest, BDDCount_Xor) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddxor(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddcount(f, (bddvar)2), 2.0);
}

TEST_F(BDDTest, BDDCount_DontCareGap) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v3));
    EXPECT_EQ(bddcount(f, (bddvar)3), 2.0);
}

TEST_F(BDDTest, BDDCount_VarGtN_Error) {
    bddnewvar();
    bddvar v2 = bddnewvar();
    bddp x2 = bddprime(v2);
    EXPECT_THROW(bddcount(x2, (bddvar)1), std::invalid_argument);
}

TEST_F(BDDTest, BDDCount_Null) {
    EXPECT_EQ(bddcount(bddnull, (bddvar)3), 0.0);
}

TEST_F(BDDTest, BDDCount_ComplementSymmetry) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddor(bddprime(v2), bddprime(v3)));
    bddp nf = bddnot(f);
    EXPECT_EQ(bddcount(f, (bddvar)3) + bddcount(nf, (bddvar)3), 8.0);
}

// --- BDD satisfiability counting (exact) ---

TEST_F(BDDTest, BDDExactCount_False) {
    EXPECT_EQ(bddexactcount(bddfalse, (bddvar)0), bigint::BigInt(0));
    EXPECT_EQ(bddexactcount(bddfalse, (bddvar)3), bigint::BigInt(0));
}

TEST_F(BDDTest, BDDExactCount_True) {
    EXPECT_EQ(bddexactcount(bddtrue, (bddvar)0), bigint::BigInt(1));
    EXPECT_EQ(bddexactcount(bddtrue, (bddvar)3), bigint::BigInt(8));
}

TEST_F(BDDTest, BDDExactCount_And) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddexactcount(f, (bddvar)2), bigint::BigInt(1));
}

TEST_F(BDDTest, BDDExactCount_Or) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddor(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddexactcount(f, (bddvar)2), bigint::BigInt(3));
}

TEST_F(BDDTest, BDDExactCount_NotOr) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddnot(bddor(bddprime(v1), bddprime(v2)));
    EXPECT_EQ(bddexactcount(f, (bddvar)2), bigint::BigInt(1));
}

TEST_F(BDDTest, BDDExactCount_ComplementSymmetry) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    bddp nf = bddnot(f);
    bigint::BigInt cf = bddexactcount(f, (bddvar)3);
    bigint::BigInt cnf = bddexactcount(nf, (bddvar)3);
    EXPECT_EQ(cf + cnf, bigint::BigInt(8));
}

TEST_F(BDDTest, BDDExactCount_DontCareGap) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v3));
    EXPECT_EQ(bddexactcount(f, (bddvar)3), bigint::BigInt(2));
}

TEST_F(BDDTest, BDDExactCount_VarGtN_Error) {
    bddnewvar();
    bddvar v2 = bddnewvar();
    bddp x2 = bddprime(v2);
    EXPECT_THROW(bddexactcount(x2, (bddvar)1), std::invalid_argument);
}

// --- BDD/ZDD child accessor functions ---

// BDD raw_child0/1: basic node
TEST_F(BDDTest, BDD_RawChild_BasicNode) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    // BDDvar(v1) creates node with lo=bddfalse, hi=bddtrue
    EXPECT_EQ(f.raw_child0().GetID(), bddfalse);
    EXPECT_EQ(f.raw_child1().GetID(), bddtrue);
    // raw_child(int) should match
    EXPECT_EQ(f.raw_child(0).GetID(), bddfalse);
    EXPECT_EQ(f.raw_child(1).GetID(), bddtrue);
}

// BDD raw_child: complement node returns stored values (not resolved)
TEST_F(BDDTest, BDD_RawChild_ComplementNode) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    BDD nf = ~f;
    // raw_child ignores complement bit, returns stored lo/hi
    EXPECT_EQ(nf.raw_child0().GetID(), bddfalse);
    EXPECT_EQ(nf.raw_child1().GetID(), bddtrue);
}

// BDD child0/1: basic node (no complement)
TEST_F(BDDTest, BDD_Child_BasicNode) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    // No complement on root, so child == raw_child
    EXPECT_EQ(f.child0().GetID(), bddfalse);
    EXPECT_EQ(f.child1().GetID(), bddtrue);
    EXPECT_EQ(f.child(0).GetID(), bddfalse);
    EXPECT_EQ(f.child(1).GetID(), bddtrue);
}

// BDD child0/1: complement node (both children flipped)
TEST_F(BDDTest, BDD_Child_ComplementNode) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    BDD nf = ~f;
    // BDD complement: both lo and hi get flipped
    EXPECT_EQ(nf.child0().GetID(), bddtrue);
    EXPECT_EQ(nf.child1().GetID(), bddfalse);
    EXPECT_EQ(nf.child(0).GetID(), bddtrue);
    EXPECT_EQ(nf.child(1).GetID(), bddfalse);
}

// BDD child: multi-variable node
TEST_F(BDDTest, BDD_Child_MultiVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;  // v1 AND v2
    // Top variable is v2 (higher level = closer to root).
    // lo = restrict v2=0: v1 AND 0 = false
    // hi = restrict v2=1: v1 AND 1 = v1
    EXPECT_EQ(f.child0(), BDD::False);
    EXPECT_EQ(f.child1(), a);
}

// BDD child: complement of multi-variable node
TEST_F(BDDTest, BDD_Child_ComplementMultiVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    BDD nf = ~f;
    // ~(v1 AND v2): complement flips both children
    EXPECT_EQ(nf.child0(), BDD::True);
    EXPECT_EQ(nf.child1(), ~a);
}

// BDD static versions match member versions
TEST_F(BDDTest, BDD_StaticChild_MatchesMember) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD f = BDDvar(v1) | BDDvar(v2);
    bddp fid = f.GetID();
    EXPECT_EQ(BDD::raw_child0(fid), f.raw_child0().GetID());
    EXPECT_EQ(BDD::raw_child1(fid), f.raw_child1().GetID());
    EXPECT_EQ(BDD::raw_child(fid, 0), f.raw_child(0).GetID());
    EXPECT_EQ(BDD::raw_child(fid, 1), f.raw_child(1).GetID());
    EXPECT_EQ(BDD::child0(fid), f.child0().GetID());
    EXPECT_EQ(BDD::child1(fid), f.child1().GetID());
    EXPECT_EQ(BDD::child(fid, 0), f.child(0).GetID());
    EXPECT_EQ(BDD::child(fid, 1), f.child(1).GetID());
}

// BDD child: terminal node throws
TEST_F(BDDTest, BDD_Child_TerminalThrows) {
    EXPECT_THROW(BDD::raw_child0(bddfalse), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child1(bddtrue), std::invalid_argument);
    EXPECT_THROW(BDD::child0(bddfalse), std::invalid_argument);
    EXPECT_THROW(BDD::child1(bddtrue), std::invalid_argument);
    EXPECT_THROW(BDD::child(bddfalse, 0), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child(bddtrue, 1), std::invalid_argument);
    // Member versions on terminal BDD
    BDD f(0);  // false
    EXPECT_THROW(f.child0(), std::invalid_argument);
    EXPECT_THROW(f.raw_child1(), std::invalid_argument);
}

// BDD child: bddnull throws
TEST_F(BDDTest, BDD_Child_NullThrows) {
    EXPECT_THROW(BDD::raw_child0(bddnull), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child1(bddnull), std::invalid_argument);
    EXPECT_THROW(BDD::child0(bddnull), std::invalid_argument);
    EXPECT_THROW(BDD::child1(bddnull), std::invalid_argument);
    // Member version
    BDD n(-1);  // null
    EXPECT_THROW(n.child0(), std::invalid_argument);
    EXPECT_THROW(n.raw_child0(), std::invalid_argument);
}

// BDD child(int): invalid argument throws
TEST_F(BDDTest, BDD_Child_InvalidArgThrows) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    bddp fid = f.GetID();
    EXPECT_THROW(BDD::child(fid, 2), std::invalid_argument);
    EXPECT_THROW(BDD::child(fid, -1), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child(fid, 2), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child(fid, -1), std::invalid_argument);
    EXPECT_THROW(f.child(2), std::invalid_argument);
    EXPECT_THROW(f.raw_child(-1), std::invalid_argument);
}

// ZDD raw_child0/1: basic node
TEST_F(BDDTest, ZDD_RawChild_BasicNode) {
    bddvar v1 = bddnewvar();
    // {{v1}} = ZDD::getnode(v1, bddempty, bddsingle)
    ZDD f = ZDD::Single.Change(v1);
    // Change creates a node with lo=bddempty, hi=bddsingle
    EXPECT_EQ(f.raw_child0().GetID(), bddempty);
    EXPECT_EQ(f.raw_child1().GetID(), bddsingle);
    EXPECT_EQ(f.raw_child(0).GetID(), bddempty);
    EXPECT_EQ(f.raw_child(1).GetID(), bddsingle);
}

// ZDD raw_child: complement node returns stored values
TEST_F(BDDTest, ZDD_RawChild_ComplementNode) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    ZDD nf = ~f;
    // raw_child ignores complement, returns stored lo/hi
    EXPECT_EQ(nf.raw_child0().GetID(), bddempty);
    EXPECT_EQ(nf.raw_child1().GetID(), bddsingle);
}

// ZDD child0/1: basic node (no complement)
TEST_F(BDDTest, ZDD_Child_BasicNode) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    EXPECT_EQ(f.child0().GetID(), bddempty);
    EXPECT_EQ(f.child1().GetID(), bddsingle);
    EXPECT_EQ(f.child(0).GetID(), bddempty);
    EXPECT_EQ(f.child(1).GetID(), bddsingle);
}

// ZDD child0/1: complement node (ZDD semantics: only lo flipped)
TEST_F(BDDTest, ZDD_Child_ComplementNode) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    ZDD nf = ~f;
    // ZDD complement: only lo is flipped, hi stays
    EXPECT_EQ(nf.child0().GetID(), bddnot(bddempty));  // lo flipped
    EXPECT_EQ(nf.child1().GetID(), bddsingle);          // hi unchanged
}

// ZDD child: multi-variable node
TEST_F(BDDTest, ZDD_Child_MultiVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}} = Single.Change(v1).Change(v2)
    ZDD f = ZDD::Single.Change(v1).Change(v2);
    // Top variable is v2 (higher level = closer to root)
    // child0 = empty, child1 = {{v1}} = Single.Change(v1)
    EXPECT_EQ(f.child0(), ZDD::Empty);
    EXPECT_EQ(f.child1(), ZDD::Single.Change(v1));
}

// ZDD child: complement semantics differ from BDD
TEST_F(BDDTest, ZDD_Child_ComplementSemanticsDifferFromBDD) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    ZDD nf = ~f;
    // ZDD: hi is NOT flipped (unlike BDD)
    // raw hi = bddsingle, child1 should also be bddsingle
    EXPECT_EQ(nf.child1().GetID(), nf.raw_child1().GetID());
    // But lo IS flipped
    EXPECT_NE(nf.child0().GetID(), nf.raw_child0().GetID());
    EXPECT_EQ(nf.child0().GetID(), bddnot(nf.raw_child0().GetID()));
}

// ZDD static versions match member versions
TEST_F(BDDTest, ZDD_StaticChild_MatchesMember) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1) + ZDD::Single.Change(v2);
    bddp fid = f.GetID();
    EXPECT_EQ(ZDD::raw_child0(fid), f.raw_child0().GetID());
    EXPECT_EQ(ZDD::raw_child1(fid), f.raw_child1().GetID());
    EXPECT_EQ(ZDD::raw_child(fid, 0), f.raw_child(0).GetID());
    EXPECT_EQ(ZDD::raw_child(fid, 1), f.raw_child(1).GetID());
    EXPECT_EQ(ZDD::child0(fid), f.child0().GetID());
    EXPECT_EQ(ZDD::child1(fid), f.child1().GetID());
    EXPECT_EQ(ZDD::child(fid, 0), f.child(0).GetID());
    EXPECT_EQ(ZDD::child(fid, 1), f.child(1).GetID());
}

// ZDD child: terminal node throws
TEST_F(BDDTest, ZDD_Child_TerminalThrows) {
    EXPECT_THROW(ZDD::raw_child0(bddempty), std::invalid_argument);
    EXPECT_THROW(ZDD::raw_child1(bddsingle), std::invalid_argument);
    EXPECT_THROW(ZDD::child0(bddempty), std::invalid_argument);
    EXPECT_THROW(ZDD::child1(bddsingle), std::invalid_argument);
    ZDD e(0);  // empty
    EXPECT_THROW(e.child0(), std::invalid_argument);
    EXPECT_THROW(e.raw_child1(), std::invalid_argument);
}

// ZDD child: bddnull throws
TEST_F(BDDTest, ZDD_Child_NullThrows) {
    EXPECT_THROW(ZDD::raw_child0(bddnull), std::invalid_argument);
    EXPECT_THROW(ZDD::child0(bddnull), std::invalid_argument);
    ZDD n(-1);  // null
    EXPECT_THROW(n.child0(), std::invalid_argument);
}

// ZDD child(int): invalid argument throws
TEST_F(BDDTest, ZDD_Child_InvalidArgThrows) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    bddp fid = f.GetID();
    EXPECT_THROW(ZDD::child(fid, 2), std::invalid_argument);
    EXPECT_THROW(ZDD::raw_child(fid, -1), std::invalid_argument);
    EXPECT_THROW(f.child(3), std::invalid_argument);
    EXPECT_THROW(f.raw_child(-1), std::invalid_argument);
}

