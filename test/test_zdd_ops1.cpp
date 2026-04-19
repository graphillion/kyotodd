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

// --- bddoffset ---

TEST_F(BDDTest, BddoffsetTerminal) {
    bddvar v1 = bddnewvar();
    // offset of empty family is empty
    EXPECT_EQ(bddoffset(bddempty, v1), bddempty);
    // offset of {{}} is {{}} (no item to remove)
    EXPECT_EQ(bddoffset(bddsingle, v1), bddsingle);
}

TEST_F(BDDTest, BddoffsetSingletonContaining) {
    bddvar v1 = bddnewvar();
    // {{v1}} = ZDD::getnode(v1, bddempty, bddsingle)
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    // offset({{v1}}, v1) = sets not containing v1 = {} (empty family)
    EXPECT_EQ(bddoffset(z, v1), bddempty);
}

TEST_F(BDDTest, BddoffsetSingletonNotContaining) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}} — does not contain v2
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    // offset({{v1}}, v2) = {{v1}} (v2 not in any set)
    EXPECT_EQ(bddoffset(z, v2), z);
}

TEST_F(BDDTest, BddoffsetFamilyWithAndWithout) {
    bddvar v1 = bddnewvar();
    // {{}, {v1}} = ZDD::getnode(v1, bddsingle, bddsingle)
    // lo = {{}} (sets without v1), hi = {{}} (sets with v1, v1 removed)
    bddp z = ZDD::getnode(v1, bddsingle, bddsingle);
    // offset(z, v1) = sets not containing v1 = lo = {{}}
    EXPECT_EQ(bddoffset(z, v1), bddsingle);
}

TEST_F(BDDTest, BddoffsetTwoVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // Build {{v1, v2}}: at v1 (level 1), hi=bddsingle, lo=bddempty → {{v1}}
    // Then at v2 (level 2), lo=bddempty, hi={{v1}} → sets containing v2 AND v1
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, z_v1);        // {{v1, v2}}
    // offset({{v1,v2}}, v2) = sets not containing v2 = empty
    EXPECT_EQ(bddoffset(z_v1v2, v2), bddempty);
    // offset({{v1,v2}}, v1) should recurse: at v2, lo=empty→offset=empty, hi={{v1}}→offset({{v1}},v1)=empty
    // result = ZDD::getnode(v2, empty, empty) = empty
    EXPECT_EQ(bddoffset(z_v1v2, v1), bddempty);
}

TEST_F(BDDTest, BddoffsetMixedFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // Build {{v1}, {v2}}: at v2 (level 2, top), lo={{v1}}, hi={{}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z = ZDD::getnode(v2, z_v1, bddsingle);             // {{v1}, {v2}}
    // offset(z, v2) = sets not containing v2 = lo = {{v1}}
    EXPECT_EQ(bddoffset(z, v2), z_v1);
    // offset(z, v1): at v2, lo=offset({{v1}},v1)=empty, hi=offset({{}},v1)={{}}
    // = ZDD::getnode(v2, empty, single) = {{v2}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);     // {{v2}}
    EXPECT_EQ(bddoffset(z, v1), z_v2);
}

TEST_F(BDDTest, BddoffsetVarNotInFamily) {
    bddvar v1 = bddnewvar();
    (void)bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1}} — only involves v1, ask about v3 (higher level)
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    // v3 has higher level than v1 → var above top, return f
    EXPECT_EQ(bddoffset(z, v3), z);
}

// --- bddonset ---

TEST_F(BDDTest, BddonsetTerminal) {
    bddvar v1 = bddnewvar();
    // onset of empty family is empty
    EXPECT_EQ(bddonset(bddempty, v1), bddempty);
    // onset of {{}} is empty (empty set doesn't contain v1)
    EXPECT_EQ(bddonset(bddsingle, v1), bddempty);
}

TEST_F(BDDTest, BddonsetSingletonContaining) {
    bddvar v1 = bddnewvar();
    // {{v1}}
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    // onset({{v1}}, v1) = {{v1}} (keeps var)
    EXPECT_EQ(bddonset(z, v1), z);
}

TEST_F(BDDTest, BddonsetSingletonNotContaining) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}} — does not contain v2
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    // onset({{v1}}, v2) = empty
    EXPECT_EQ(bddonset(z, v2), bddempty);
}

TEST_F(BDDTest, BddonsetFamilyWithAndWithout) {
    bddvar v1 = bddnewvar();
    // {{}, {v1}} = ZDD::getnode(v1, bddsingle, bddsingle)
    bddp z = ZDD::getnode(v1, bddsingle, bddsingle);
    // onset(z, v1) = {{v1}} (sets containing v1, var kept)
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    EXPECT_EQ(bddonset(z, v1), z_v1);
}

TEST_F(BDDTest, BddonsetTwoVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, z_v1);        // {{v1, v2}}
    // onset({{v1,v2}}, v2) = {{v1,v2}}
    EXPECT_EQ(bddonset(z_v1v2, v2), z_v1v2);
    // onset({{v1,v2}}, v1) = {{v1,v2}}
    EXPECT_EQ(bddonset(z_v1v2, v1), z_v1v2);
}

TEST_F(BDDTest, BddonsetMixedFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}, {v2}}: at v2 (top), lo={{v1}}, hi={{}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z = ZDD::getnode(v2, z_v1, bddsingle);             // {{v1}, {v2}}
    // onset(z, v2) = {{v2}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddonset(z, v2), z_v2);
    // onset(z, v1) = {{v1}}
    EXPECT_EQ(bddonset(z, v1), z_v1);
}

TEST_F(BDDTest, BddonsetVarNotInFamily) {
    bddvar v1 = bddnewvar();
    (void)bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1}} — v3 not present
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddonset(z, v3), bddempty);
}

// --- bddonset0 ---

TEST_F(BDDTest, Bddonset0Terminal) {
    bddvar v1 = bddnewvar();
    EXPECT_EQ(bddonset0(bddempty, v1), bddempty);
    EXPECT_EQ(bddonset0(bddsingle, v1), bddempty);
}

TEST_F(BDDTest, Bddonset0SingletonContaining) {
    bddvar v1 = bddnewvar();
    // {{v1}}
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    // onset0({{v1}}, v1) = {{}} (var removed)
    EXPECT_EQ(bddonset0(z, v1), bddsingle);
}

TEST_F(BDDTest, Bddonset0SingletonNotContaining) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddonset0(z, v2), bddempty);
}

TEST_F(BDDTest, Bddonset0FamilyWithAndWithout) {
    bddvar v1 = bddnewvar();
    // {{}, {v1}}
    bddp z = ZDD::getnode(v1, bddsingle, bddsingle);
    // onset0(z, v1) = {{}} (sets containing v1 with v1 removed)
    EXPECT_EQ(bddonset0(z, v1), bddsingle);
}

TEST_F(BDDTest, Bddonset0TwoVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, z_v1);        // {{v1, v2}}
    // onset0({{v1,v2}}, v2) = {{v1}} (v2 removed)
    EXPECT_EQ(bddonset0(z_v1v2, v2), z_v1);
    // onset0({{v1,v2}}, v1) = {{v2}} (v1 removed)
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddonset0(z_v1v2, v1), z_v2);
}

TEST_F(BDDTest, Bddonset0MixedFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}, {v2}}: at v2 (top), lo={{v1}}, hi={{}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z = ZDD::getnode(v2, z_v1, bddsingle);
    // onset0(z, v2) = {{}} (v2 removed from {v2})
    EXPECT_EQ(bddonset0(z, v2), bddsingle);
    // onset0(z, v1) = {{}} (v1 removed from {v1})
    EXPECT_EQ(bddonset0(z, v1), bddsingle);
}

TEST_F(BDDTest, Bddonset0VarNotInFamily) {
    bddvar v1 = bddnewvar();
    (void)bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddonset0(z, v3), bddempty);
}

// --- bddchange ---

TEST_F(BDDTest, BddchangeTerminal) {
    bddvar v1 = bddnewvar();
    // change(empty, v1) = empty
    EXPECT_EQ(bddchange(bddempty, v1), bddempty);
    // change({{}}, v1) = {{v1}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddchange(bddsingle, v1), z_v1);
}

TEST_F(BDDTest, BddchangeSingletonToggle) {
    bddvar v1 = bddnewvar();
    // {{v1}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    // change({{v1}}, v1) = {{}} (remove v1)
    EXPECT_EQ(bddchange(z_v1, v1), bddsingle);
}

TEST_F(BDDTest, BddchangeDoubleToggle) {
    bddvar v1 = bddnewvar();
    // change twice should restore original
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddchange(bddchange(z_v1, v1), v1), z_v1);
    EXPECT_EQ(bddchange(bddchange(bddsingle, v1), v1), bddsingle);
}

TEST_F(BDDTest, BddchangeVarNotPresent) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}}: change with v2 → add v2 to each set → {{v1, v2}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, z_v1);  // {{v1, v2}}
    EXPECT_EQ(bddchange(z_v1, v2), z_v1v2);
}

TEST_F(BDDTest, BddchangeFamilyWithAndWithout) {
    bddvar v1 = bddnewvar();
    // {{}, {v1}} = ZDD::getnode(v1, bddsingle, bddsingle)
    bddp z = ZDD::getnode(v1, bddsingle, bddsingle);
    // change(z, v1): sets without v1 ({{}}) get v1 → {{v1}}
    //                sets with v1 ({v1}) lose v1 → {{}}
    //                result = {{}, {v1}} = z (same family, swapped roles)
    EXPECT_EQ(bddchange(z, v1), z);
}

TEST_F(BDDTest, BddchangeMixedFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}, {v2}}: at v2 (top), lo={{v1}}, hi={{}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z = ZDD::getnode(v2, z_v1, bddsingle);
    // change(z, v1): toggle v1 in each set
    //   {v1} → {} (remove v1), {v2} → {v1, v2} (add v1)
    //   result = {{}, {v1, v2}}
    bddp expected = ZDD::getnode(v2, bddsingle, z_v1);  // {{}, {v1, v2}}
    EXPECT_EQ(bddchange(z, v1), expected);
}

TEST_F(BDDTest, BddchangeVarAboveTop) {
    bddvar v1 = bddnewvar();
    (void)bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1}}: change with v3 (higher level) → add v3 → {{v1, v3}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp expected = ZDD::getnode(v3, bddempty, z_v1);  // {{v1, v3}}
    EXPECT_EQ(bddchange(z_v1, v3), expected);
}

// --- bddchange auto-expansion ---

TEST_F(BDDTest, BddchangeAutoExpandVar) {
    bddvar old_count = bddvarused();
    bddvar target = old_count + 3;
    bddp result = bddchange(bddsingle, target);
    EXPECT_EQ(bddvarused(), target);
    EXPECT_EQ(bddtop(result), target);
}

TEST_F(BDDTest, BddchangeAutoExpandPreservesData) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddvar target = bddvarused() + 2;
    bddp result = bddchange(z_v1, target);
    EXPECT_EQ(bddvarused(), target);
    EXPECT_EQ(bddtop(result), target);
}

TEST_F(BDDTest, BddchangeRejectsVarAbove65536) {
    EXPECT_THROW(bddchange(bddsingle, 65537), std::invalid_argument);
}

TEST_F(BDDTest, BddchangeNullNoAutoExpand) {
    bddvar old_count = bddvarused();
    bddp result = bddchange(bddnull, old_count + 1);
    EXPECT_EQ(result, bddnull);
    EXPECT_EQ(bddvarused(), old_count);
}

TEST_F(BDDTest, BddchangeEmptyAutoExpand) {
    bddvar old_count = bddvarused();
    bddvar target = old_count + 2;
    bddp result = bddchange(bddempty, target);
    EXPECT_EQ(result, bddempty);
    EXPECT_EQ(bddvarused(), target);
}

// --- bddunion ---

TEST_F(BDDTest, BddunionTerminals) {
    // empty ∪ empty = empty
    EXPECT_EQ(bddunion(bddempty, bddempty), bddempty);
    // empty ∪ {{}} = {{}}
    EXPECT_EQ(bddunion(bddempty, bddsingle), bddsingle);
    EXPECT_EQ(bddunion(bddsingle, bddempty), bddsingle);
    // {{}} ∪ {{}} = {{}}
    EXPECT_EQ(bddunion(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, BddunionWithEmpty) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    EXPECT_EQ(bddunion(z_v1, bddempty), z_v1);
    EXPECT_EQ(bddunion(bddempty, z_v1), z_v1);
}

TEST_F(BDDTest, BddunionSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    EXPECT_EQ(bddunion(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, BddunionDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}
    // {{v1}} ∪ {{v2}} = {{v1}, {v2}}
    bddp expected = ZDD::getnode(v2, z_v1, bddsingle);
    EXPECT_EQ(bddunion(z_v1, z_v2), expected);
    // Commutative
    EXPECT_EQ(bddunion(z_v2, z_v1), expected);
}

TEST_F(BDDTest, BddunionAddEmptySet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    // {{v1}} ∪ {{}} = {{}, {v1}}
    bddp expected = ZDD::getnode(v1, bddsingle, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddunion(z_v1, bddsingle), expected);
}

TEST_F(BDDTest, BddunionOverlapping) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1_v2 = ZDD::getnode(v2, z_v1, bddsingle);      // {{v1}, {v2}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, z_v1);         // {{v1, v2}}
    // {{v1}, {v2}} ∪ {{v1, v2}} = {{v1}, {v2}, {v1, v2}}
    bddp result = bddunion(z_v1_v2, z_v1v2);
    // Expected: at v2, lo={{v1}} (sets without v2), hi=union({{},{}}, {{v1}})
    // hi = {{}, {v1}}
    bddp hi_expected = ZDD::getnode(v1, bddsingle, bddsingle);  // {{}, {v1}}
    bddp expected = ZDD::getnode(v2, z_v1, hi_expected);
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, BddunionThreeVars) {
    bddvar v1 = bddnewvar();
    (void)bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);     // {{v3}}
    // {{v1}} ∪ {{v3}} = at v3, lo={{v1}}, hi={{}}
    bddp expected = ZDD::getnode(v3, z_v1, bddsingle);
    EXPECT_EQ(bddunion(z_v1, z_v3), expected);
}

TEST_F(BDDTest, BddunionIdempotent) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp u = bddunion(z_v1, z_v2);
    // union(u, z_v1) == u (z_v1 already in u)
    EXPECT_EQ(bddunion(u, z_v1), u);
    EXPECT_EQ(bddunion(u, z_v2), u);
}

// --- bddintersec ---

TEST_F(BDDTest, BddintersecTerminals) {
    EXPECT_EQ(bddintersec(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddintersec(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddintersec(bddsingle, bddempty), bddempty);
    EXPECT_EQ(bddintersec(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, BddintersecWithEmpty) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddintersec(z_v1, bddempty), bddempty);
    EXPECT_EQ(bddintersec(bddempty, z_v1), bddempty);
}

TEST_F(BDDTest, BddintersecSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddintersec(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, BddintersecDisjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}
    // {{v1}} ∩ {{v2}} = {} (no common sets)
    EXPECT_EQ(bddintersec(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, BddintersecOverlapping) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1_v2 = ZDD::getnode(v2, z_v1, bddsingle);      // {{v1}, {v2}}
    // {{v1}, {v2}} ∩ {{v1}} = {{v1}}
    EXPECT_EQ(bddintersec(z_v1_v2, z_v1), z_v1);
    EXPECT_EQ(bddintersec(z_v1, z_v1_v2), z_v1);
}

TEST_F(BDDTest, BddintersecWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    // {{v1}} ∩ {{}} = {} (no common sets)
    EXPECT_EQ(bddintersec(z_v1, bddsingle), bddempty);
    // {{}, {v1}} ∩ {{}} = {{}}
    bddp z_both = ZDD::getnode(v1, bddsingle, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddintersec(z_both, bddsingle), bddsingle);
}

TEST_F(BDDTest, BddintersecComplex) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);      // {{v1}}
    bddp z_v1_v2 = ZDD::getnode(v2, z_v1, bddsingle);       // {{v1}, {v2}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, z_v1);          // {{v1, v2}}
    bddp z_v2_v1v2 = bddunion(
        ZDD::getnode(v2, bddempty, bddsingle), z_v1v2);       // {{v2}, {v1, v2}}
    // {{v1}, {v2}} ∩ {{v2}, {v1, v2}} = {{v2}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddintersec(z_v1_v2, z_v2_v1v2), z_v2);
}

TEST_F(BDDTest, BddintersecUnionIdentity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp u = bddunion(z_v1, z_v2);  // {{v1}, {v2}}
    // intersec(union(a,b), a) == a
    EXPECT_EQ(bddintersec(u, z_v1), z_v1);
    EXPECT_EQ(bddintersec(u, z_v2), z_v2);
}

// --- bddsubtract ---

TEST_F(BDDTest, BddsubtractTerminals) {
    EXPECT_EQ(bddsubtract(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddsubtract(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddsubtract(bddsingle, bddempty), bddsingle);
    EXPECT_EQ(bddsubtract(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, BddsubtractWithEmpty) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddsubtract(z_v1, bddempty), z_v1);
    EXPECT_EQ(bddsubtract(bddempty, z_v1), bddempty);
}

TEST_F(BDDTest, BddsubtractSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddsubtract(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, BddsubtractDisjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}
    // {{v1}} \ {{v2}} = {{v1}}
    EXPECT_EQ(bddsubtract(z_v1, z_v2), z_v1);
    // {{v2}} \ {{v1}} = {{v2}}
    EXPECT_EQ(bddsubtract(z_v2, z_v1), z_v2);
}

TEST_F(BDDTest, BddsubtractSubset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp u = bddunion(z_v1, z_v2);  // {{v1}, {v2}}
    // {{v1}, {v2}} \ {{v1}} = {{v2}}
    EXPECT_EQ(bddsubtract(u, z_v1), z_v2);
    // {{v1}, {v2}} \ {{v2}} = {{v1}}
    EXPECT_EQ(bddsubtract(u, z_v2), z_v1);
    // {{v1}} \ {{v1}, {v2}} = {}
    EXPECT_EQ(bddsubtract(z_v1, u), bddempty);
}

TEST_F(BDDTest, BddsubtractEmptySetMember) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_both = ZDD::getnode(v1, bddsingle, bddsingle);   // {{}, {v1}}
    // {{}, {v1}} \ {{v1}} = {{}}
    EXPECT_EQ(bddsubtract(z_both, z_v1), bddsingle);
    // {{}, {v1}} \ {{}} = {{v1}}
    EXPECT_EQ(bddsubtract(z_both, bddsingle), z_v1);
}

TEST_F(BDDTest, BddsubtractUnionIdentity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    // union(a, b) \ b == subtract(a, b) when a ∩ b = empty
    bddp u = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddsubtract(u, z_v2), z_v1);
    // a == union(intersec(a,b), subtract(a,b))
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, z_v1);  // {{v1, v2}}
    bddp big = bddunion(u, z_v1v2);  // {{v1}, {v2}, {v1,v2}}
    bddp common = bddintersec(big, u);
    bddp diff = bddsubtract(big, u);
    EXPECT_EQ(bddunion(common, diff), big);
}

// --- bdddiv ---

TEST_F(BDDTest, BdddivTerminals) {
    // F / {∅} = F
    EXPECT_EQ(bdddiv(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bdddiv(bddsingle, bddsingle), bddsingle);
    // ∅ / G = ∅
    EXPECT_EQ(bdddiv(bddempty, bddempty), bddempty);
    // F / ∅ = ∅
    EXPECT_EQ(bdddiv(bddsingle, bddempty), bddempty);
    // {∅} / non-trivial = ∅ (G contains non-empty sets)
}

TEST_F(BDDTest, BdddivByEmptySet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    // {{v1}} / {∅} = {{v1}}
    EXPECT_EQ(bdddiv(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, BdddivSelf) {
    // F / F always contains {∅}
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    // {{v1}} / {{v1}}: x such that x ∪ {v1} ∈ {{v1}} and x ∩ {v1} = ∅
    // x = ∅: ∅ ∪ {v1} = {v1} ∈ F ✓ → {∅}
    EXPECT_EQ(bdddiv(z_v1, z_v1), bddsingle);
}

TEST_F(BDDTest, BdddivEmptySetInFNotG) {
    // {∅} / {{v1}} = ∅ (∅ ∪ {v1} = {v1} ∉ {∅})
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bdddiv(bddsingle, z_v1), bddempty);
}

TEST_F(BDDTest, BdddivGVarAboveF) {
    // G has variable not in F → ∅
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}
    // {{v1}} / {{v2}}: x ∪ {v2} ∈ {{v1}}, but no set in {{v1}} contains v2
    EXPECT_EQ(bdddiv(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, BdddivSimple) {
    // F = {{v1, v2}}, G = {{v2}} → F/G = {{v1}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);        // {{v1}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, z_v1);           // {{v1, v2}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);        // {{v2}}
    EXPECT_EQ(bdddiv(z_v1v2, z_v2), z_v1);
}

TEST_F(BDDTest, BdddivMultipleSets) {
    // F = {{a,c}, {a,d}, {c}, {d}}, G = {{c}, {d}}
    // F/G = {{a}, {∅}}:
    //   x={a}: {a}∪{c}={a,c}∈F, {a}∪{d}={a,d}∈F ✓
    //   x={∅}: {c}∈F, {d}∈F ✓
    bddvar a = bddnewvar();  // level 1
    bddvar c = bddnewvar();  // level 2
    bddvar d = bddnewvar();  // level 3

    bddp z_a = ZDD::getnode(a, bddempty, bddsingle);   // {{a}}
    bddp z_ac = ZDD::getnode(c, bddempty, z_a);        // {{a,c}}
    bddp z_ad = ZDD::getnode(d, bddempty, z_a);        // {{a,d}}
    bddp z_c = ZDD::getnode(c, bddempty, bddsingle);   // {{c}}
    bddp z_d = ZDD::getnode(d, bddempty, bddsingle);   // {{d}}

    bddp F = bddunion(bddunion(z_ac, z_ad), bddunion(z_c, z_d));
    bddp G = bddunion(z_c, z_d);

    // Expected: {{∅}, {a}}
    bddp expected = ZDD::getnode(a, bddsingle, bddsingle);
    EXPECT_EQ(bdddiv(F, G), expected);
}

TEST_F(BDDTest, BdddivPartialMatch) {
    // F = {{a,c}, {c}}, G = {{c}}
    // F/G: x ∪ {c} ∈ F and c ∉ x
    //   x={a}: {a,c} ∈ F ✓
    //   x={∅}: {c} ∈ F ✓
    // F/G = {{∅}, {a}}
    bddvar a = bddnewvar();
    bddvar c = bddnewvar();
    bddp z_a = ZDD::getnode(a, bddempty, bddsingle);
    bddp z_ac = ZDD::getnode(c, bddempty, z_a);         // {{a,c}}
    bddp z_c = ZDD::getnode(c, bddempty, bddsingle);    // {{c}}
    bddp F = bddunion(z_ac, z_c);                  // {{a,c}, {c}}

    bddp expected = ZDD::getnode(a, bddsingle, bddsingle);  // {{∅}, {a}}
    EXPECT_EQ(bdddiv(F, z_c), expected);
}

TEST_F(BDDTest, BdddivNoMatch) {
    // F = {{a}}, G = {{b}} where level(b) > level(a) → ∅
    bddvar a = bddnewvar();
    bddvar b = bddnewvar();
    bddp z_a = ZDD::getnode(a, bddempty, bddsingle);
    bddp z_b = ZDD::getnode(b, bddempty, bddsingle);
    EXPECT_EQ(bdddiv(z_a, z_b), bddempty);
}

TEST_F(BDDTest, BdddivQuotientTimesG) {
    // Verify: (F/G) · G ⊆ F  (via union/intersec identity)
    // F = {{a,b}, {a,c}, {b}, {c}}, G = {{b}, {c}}
    // F/G = {{a}, {∅}}
    // (F/G)·G = {{a,b}, {a,c}, {b}, {c}} = F
    // So F \ ((F/G)·G) should be ∅ (remainder = ∅)
    bddvar a = bddnewvar();
    bddvar b = bddnewvar();
    bddvar c = bddnewvar();
    bddp z_a = ZDD::getnode(a, bddempty, bddsingle);
    bddp z_ab = ZDD::getnode(b, bddempty, z_a);
    bddp z_ac = ZDD::getnode(c, bddempty, z_a);
    bddp z_b = ZDD::getnode(b, bddempty, bddsingle);
    bddp z_c = ZDD::getnode(c, bddempty, bddsingle);

    bddp F = bddunion(bddunion(z_ab, z_ac), bddunion(z_b, z_c));
    bddp G = bddunion(z_b, z_c);
    bddp Q = bdddiv(F, G);

    bddp expected_q = ZDD::getnode(a, bddsingle, bddsingle);  // {{∅}, {a}}
    EXPECT_EQ(Q, expected_q);
}

TEST_F(BDDTest, BdddivWithRemainder) {
    // F = {{a,b}, {c}}, G = {{b}}
    // F/G: x ∪ {b} ∈ F and b ∉ x
    //   x={a}: {a,b} ∈ F ✓
    //   x={c}: {b,c} ∉ F ✗
    // F/G = {{a}}
    bddvar a = bddnewvar();
    bddvar b = bddnewvar();
    bddvar c = bddnewvar();
    bddp z_a = ZDD::getnode(a, bddempty, bddsingle);
    bddp z_ab = ZDD::getnode(b, bddempty, z_a);
    bddp z_c = ZDD::getnode(c, bddempty, bddsingle);
    bddp F = bddunion(z_ab, z_c);
    bddp G = ZDD::getnode(b, bddempty, bddsingle);

    EXPECT_EQ(bdddiv(F, G), z_a);
}

TEST_F(BDDTest, ZDDOperatorDiv) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, z_v1.GetID()));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    // {{v1, v2}} / {{v2}} = {{v1}}
    ZDD result = z_v1v2 / z_v2;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorDivAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, z_v1.GetID()));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    z_v1v2 /= z_v2;
    EXPECT_EQ(z_v1v2, z_v1);
}

// --- bddsymdiff ---

TEST_F(BDDTest, ZDDSymdiffTerminalCases) {
    EXPECT_EQ(bddsymdiff(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddsymdiff(bddempty, bddsingle), bddsingle);
    EXPECT_EQ(bddsymdiff(bddsingle, bddempty), bddsingle);
    EXPECT_EQ(bddsymdiff(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDSymdiffWithSingleVar) {
    bddvar v1 = bddnewvar();
    // z_v1 = {{v1}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    // {{v1}} ^ {} = {{v1}}
    EXPECT_EQ(bddsymdiff(z_v1, bddempty), z_v1);
    EXPECT_EQ(bddsymdiff(bddempty, z_v1), z_v1);

    // {{v1}} ^ {{v1}} = {}
    EXPECT_EQ(bddsymdiff(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, ZDDSymdiffDisjointFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // z_v1 = {{v1}}, z_v2 = {{v2}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // {{v1}} ^ {{v2}} = {{v1}, {v2}} = union
    bddp result = bddsymdiff(z_v1, z_v2);
    bddp expected = bddunion(z_v1, z_v2);
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, ZDDSymdiffOverlappingFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v2}}
    bddp F = bddunion(z_v1, z_v2);
    bddp G = z_v2;

    // F ^ G = {{v1}}
    EXPECT_EQ(bddsymdiff(F, G), z_v1);
}

TEST_F(BDDTest, ZDDSymdiffEqualsSubtractUnion) {
    // Verify F ^ G = (F \ G) ∪ (G \ F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v2}, {v3}}
    bddp F = bddunion(z_v1, z_v2);
    bddp G = bddunion(z_v2, z_v3);

    bddp result = bddsymdiff(F, G);
    bddp expected = bddunion(bddsubtract(F, G), bddsubtract(G, F));
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, ZDDSymdiffCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);  // {{v1}, {}}
    bddp G = z_v2;                         // {{v2}}

    EXPECT_EQ(bddsymdiff(F, G), bddsymdiff(G, F));
}

TEST_F(BDDTest, ZDDSymdiffWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    // F = {{v1}, {}}, G = {{}}
    bddp F = bddunion(z_v1, bddsingle);
    bddp G = bddsingle;

    // F ^ G = {{v1}}
    EXPECT_EQ(bddsymdiff(F, G), z_v1);
}

TEST_F(BDDTest, ZDDOperatorSymdiff) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v2.GetID()));
    ZDD G = z_v2;

    // F ^ G = {{v1}}
    ZDD result = F ^ G;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorSymdiffAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v2.GetID()));
    F ^= z_v2;
    EXPECT_EQ(F, z_v1);
}

// --- bddjoin ---

TEST_F(BDDTest, ZDDJoinTerminalCases) {
    EXPECT_EQ(bddjoin(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddjoin(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddjoin(bddsingle, bddempty), bddempty);
    EXPECT_EQ(bddjoin(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDJoinIdentity) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // {∅} ⊔ F = F
    EXPECT_EQ(bddjoin(bddsingle, z_v1), z_v1);
    EXPECT_EQ(bddjoin(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, ZDDJoinDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1}} ⊔ {{v2}} = {{v1, v2}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(bddjoin(z_v1, z_v2), z_v1v2);
}

TEST_F(BDDTest, ZDDJoinSameVar) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // {{v1}} ⊔ {{v1}} = {{v1}} (union is idempotent)
    EXPECT_EQ(bddjoin(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDJoinFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}

    // F = {{v1}, {v2}}, G = {{v1}}
    bddp F = bddunion(z_v1, z_v2);
    bddp G = z_v1;

    // F ⊔ G = { A∪B | A∈F, B∈G }
    //   = {v1}∪{v1}, {v2}∪{v1} = {{v1}, {v1,v2}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp expected = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddjoin(F, G), expected);
}

TEST_F(BDDTest, ZDDJoinCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);  // {{v1}, {}}
    bddp G = z_v2;                         // {{v2}}

    EXPECT_EQ(bddjoin(F, G), bddjoin(G, F));
}

TEST_F(BDDTest, ZDDJoinWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F = {{v1}, {}}, G = {{v2}}
    // F ⊔ G = {{v1,v2}, {v2}} ({}∪{v2}={v2}, {v1}∪{v2}={v1,v2})
    bddp F = bddunion(z_v1, bddsingle);
    bddp G = z_v2;
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp expected = bddunion(z_v2, z_v1v2);
    EXPECT_EQ(bddjoin(F, G), expected);
}

TEST_F(BDDTest, ZDDOperatorJoin) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    // {{v1}} * {{v2}} = {{v1, v2}}
    ZDD result = z_v1 * z_v2;
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(result.GetID(), z_v1v2);
}

TEST_F(BDDTest, ZDDOperatorJoinAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    z_v1 *= z_v2;
    EXPECT_EQ(z_v1.GetID(), z_v1v2);
}

// --- bddmeet ---

TEST_F(BDDTest, ZDDMeetTerminalCases) {
    EXPECT_EQ(bddmeet(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddmeet(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddmeet(bddsingle, bddempty), bddempty);
    // {∅} ⊓ {∅} = {∅∩∅} = {∅}
    EXPECT_EQ(bddmeet(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDMeetWithSingle) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // {∅} ⊓ {{v1}} = {∅∩{v1}} = {∅}
    EXPECT_EQ(bddmeet(bddsingle, z_v1), bddsingle);
    EXPECT_EQ(bddmeet(z_v1, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDMeetSameFamily) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // {{v1}} ⊓ {{v1}} = {{v1}∩{v1}} = {{v1}}
    EXPECT_EQ(bddmeet(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDMeetDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1}} ⊓ {{v2}} = {{v1}∩{v2}} = {∅}
    EXPECT_EQ(bddmeet(z_v1, z_v2), bddsingle);
}

TEST_F(BDDTest, ZDDMeetOverlapping) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));  // {{v1,v2}}

    // {{v1,v2}} ⊓ {{v1}} = {{v1,v2}∩{v1}} = {{v1}}
    EXPECT_EQ(bddmeet(z_v1v2, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDMeetFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1}, {v1,v2}}, G = {{v2}, {v1,v2}}
    bddp F = bddunion(z_v1, z_v1v2);
    bddp G = bddunion(z_v2, z_v1v2);

    // F ⊓ G = { A∩B | A∈F, B∈G }
    //   {v1}∩{v2}=∅, {v1}∩{v1,v2}={v1},
    //   {v1,v2}∩{v2}={v2}, {v1,v2}∩{v1,v2}={v1,v2}
    //   = {∅, {v1}, {v2}, {v1,v2}}
    bddp expected = bddunion(bddunion(bddsingle, z_v1), bddunion(z_v2, z_v1v2));
    EXPECT_EQ(bddmeet(F, G), expected);
}

TEST_F(BDDTest, ZDDMeetCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    bddp F = bddunion(z_v1, z_v1v2);
    bddp G = z_v2;

    EXPECT_EQ(bddmeet(F, G), bddmeet(G, F));
}

// --- bdddelta ---

TEST_F(BDDTest, ZDDDeltaTerminalCases) {
    EXPECT_EQ(bdddelta(bddempty, bddempty), bddempty);
    EXPECT_EQ(bdddelta(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bdddelta(bddsingle, bddempty), bddempty);
    // {∅} ⊞ {∅} = {∅⊕∅} = {∅}
    EXPECT_EQ(bdddelta(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDDeltaIdentity) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // {∅} ⊞ F = F (∅⊕A = A)
    EXPECT_EQ(bdddelta(bddsingle, z_v1), z_v1);
    EXPECT_EQ(bdddelta(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, ZDDDeltaSameSet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // {{v1}} ⊞ {{v1}} = {{v1}⊕{v1}} = {∅}
    EXPECT_EQ(bdddelta(z_v1, z_v1), bddsingle);
}

TEST_F(BDDTest, ZDDDeltaDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1}} ⊞ {{v2}} = {{v1}⊕{v2}} = {{v1,v2}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(bdddelta(z_v1, z_v2), z_v1v2);
}

TEST_F(BDDTest, ZDDDeltaFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v1}}
    bddp F = bddunion(z_v1, z_v2);
    bddp G = z_v1;

    // F ⊞ G = { A⊕B | A∈F, B∈G }
    //   {v1}⊕{v1}=∅, {v2}⊕{v1}={v1,v2}
    //   = {∅, {v1,v2}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp expected = bddunion(bddsingle, z_v1v2);
    EXPECT_EQ(bdddelta(F, G), expected);
}

TEST_F(BDDTest, ZDDDeltaCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);  // {{v1}, {}}
    bddp G = z_v2;                         // {{v2}}

    EXPECT_EQ(bdddelta(F, G), bdddelta(G, F));
}

TEST_F(BDDTest, ZDDDeltaThreeVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1,v2}}, G = {{v2,v3}}
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp F = z_v1v2;
    bddp G = z_v2v3;

    // {v1,v2}⊕{v2,v3} = {v1,v3}
    bddp z_v1v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(bdddelta(F, G), z_v1v3);
}

// --- bddremainder ---

TEST_F(BDDTest, ZDDRemainderTerminalCases) {
    // F % G = F \ (G ⊔ (F / G))
    EXPECT_EQ(bddremainder(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddremainder(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddremainder(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDRemainderNoDivisor) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1}} / {{v2}} = {} (no quotient), so remainder = {{v1}} \ ({v2} ⊔ {}) = {{v1}}
    EXPECT_EQ(bddremainder(z_v1, z_v2), z_v1);
}

TEST_F(BDDTest, ZDDRemainderExactDivision) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));  // {{v1,v2}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}

    // F = {{v1,v2}}, G = {{v2}}
    // F / G = {{v1}}, G ⊔ (F/G) = {{v2}} ⊔ {{v1}} = {{v1,v2}}
    // F % G = {{v1,v2}} \ {{v1,v2}} = {}
    EXPECT_EQ(bddremainder(z_v1v2, z_v2), bddempty);
}

TEST_F(BDDTest, ZDDRemainderPartialDivision) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v1v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1,v2}, {v1,v3}}, G = {{v2}}
    bddp F = bddunion(z_v1v2, z_v1v3);

    // F / G = {{v1}} (only {v1,v2} is divisible by {v2})
    // G ⊔ (F/G) = {{v2}} ⊔ {{v1}} = {{v1,v2}}
    // F % G = {{v1,v2},{v1,v3}} \ {{v1,v2}} = {{v1,v3}}
    EXPECT_EQ(bddremainder(F, z_v2), z_v1v3);
}

TEST_F(BDDTest, ZDDRemainderDefinition) {
    // Verify F % G = F \ (G ⊔ (F / G)) with a complex example
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));

    // F = {{v1,v2}, {v2,v3}, {v1}}, G = {{v2}}
    bddp F = bddunion(bddunion(z_v1v2, z_v2v3), z_v1);
    bddp G = z_v2;

    bddp result = bddremainder(F, G);
    bddp expected = bddsubtract(F, bddjoin(G, bdddiv(F, G)));
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, ZDDOperatorRemainder) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    // {{v1}} % {{v2}} = {{v1}} (no divisor)
    ZDD result = z_v1 % z_v2;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorRemainderAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    ZDD F = ZDD_ID(z_v1v2);  // {{v1,v2}}
    ZDD G = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));  // {{v2}}

    // {{v1,v2}} % {{v2}} = {} (exact division)
    F %= G;
    EXPECT_EQ(F, ZDD::Empty);
}

TEST_F(BDDTest, ZDDRemainderEmptyDividend) {
    bddvar v1 = bddnewvar();
    bddp g = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddremainder(bddempty, g), bddempty);
}

TEST_F(BDDTest, ZDDRemainderSingletonDivisor) {
    bddvar v1 = bddnewvar();
    bddp f = ZDD::getnode(v1, bddempty, bddsingle);
    // F % {∅} = ∅ (since {∅} divides everything)
    EXPECT_EQ(bddremainder(f, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDRemainderSelf) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddunion(ZDD::getnode(v1, bddempty, bddsingle),
                       ZDD::getnode(v2, bddempty, bddsingle));
    // F % F = ∅
    EXPECT_EQ(bddremainder(f, f), bddempty);
}

TEST_F(BDDTest, ZDDRemainderCacheConsistency) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp sv1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp sv2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp sv3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp f = bddunion(bddjoin(sv1, sv2), bddjoin(sv1, sv3));
    bddp g = sv1;
    // Call twice — second call should use cache and return same result
    bddp r1 = bddremainder(f, g);
    bddp r2 = bddremainder(f, g);
    EXPECT_EQ(r1, r2);
}

// --- ZDD operator+ / operator+= ---

TEST_F(BDDTest, ZDDOperatorPlus) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD result = z_v1 + z_v2;
    EXPECT_EQ(result.GetID(), bddunion(z_v1.GetID(), z_v2.GetID()));
}

TEST_F(BDDTest, ZDDOperatorPlusAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    bddp expected = bddunion(z_v1.GetID(), z_v2.GetID());
    z_v1 += z_v2;
    EXPECT_EQ(z_v1.GetID(), expected);
}

// --- ZDD operator- / operator-= ---

TEST_F(BDDTest, ZDDOperatorMinus) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v2.GetID()));
    ZDD result = F - z_v2;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorMinusAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v2.GetID()));
    F -= z_v2;
    EXPECT_EQ(F, z_v1);
}

// --- ZDD operator& / operator&= ---

TEST_F(BDDTest, ZDDOperatorIntersec) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v2.GetID()));
    ZDD result = F & z_v1;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorIntersecAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v2.GetID()));
    F &= z_v1;
    EXPECT_EQ(F, z_v1);
}

// --- ZDD operator== / operator!= ---

TEST_F(BDDTest, ZDDOperatorEqual) {
    bddvar v1 = bddnewvar();
    ZDD a = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD b = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST_F(BDDTest, ZDDOperatorNotEqual) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD b = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST_F(BDDTest, ZDDOperatorEqualConstants) {
    EXPECT_TRUE(ZDD::Empty == ZDD::Empty);
    EXPECT_TRUE(ZDD::Single == ZDD::Single);
    EXPECT_FALSE(ZDD::Empty == ZDD::Single);
    EXPECT_TRUE(ZDD::Empty != ZDD::Single);
}

// --- ZDD::Change ---

TEST_F(BDDTest, ZDDChangeMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD result = z_v1.Change(v2);
    EXPECT_EQ(result.GetID(), bddchange(z_v1.GetID(), v2));
}

// --- ZDD::OffSet ---

TEST_F(BDDTest, ZDDOffsetMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v1v2.GetID()));
    ZDD result = F.OffSet(v2);
    EXPECT_EQ(result, z_v1);
}

// --- ZDD::OnSet ---

TEST_F(BDDTest, ZDDOnSetMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v1v2.GetID()));
    ZDD result = F.OnSet(v2);
    EXPECT_EQ(result, z_v1v2);
}

// --- ZDD::OnSet0 ---

TEST_F(BDDTest, ZDDOnSet0Method) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = ZDD_ID(bddunion(z_v1.GetID(), z_v1v2.GetID()));
    // OnSet0(v2) returns sets containing v2 with v2 removed = {{v1}}
    ZDD result = F.OnSet0(v2);
    EXPECT_EQ(result, z_v1);
}

// --- bdddisjoin ---

TEST_F(BDDTest, ZDDDisjoinTerminalCases) {
    EXPECT_EQ(bdddisjoin(bddempty, bddempty), bddempty);
    EXPECT_EQ(bdddisjoin(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bdddisjoin(bddsingle, bddempty), bddempty);
    EXPECT_EQ(bdddisjoin(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDDisjoinIdentity) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // {∅} ▷◁˙ F = F
    EXPECT_EQ(bdddisjoin(bddsingle, z_v1), z_v1);
    EXPECT_EQ(bdddisjoin(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, ZDDDisjoinDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // {{v1}} ▷◁˙ {{v2}} = {{v1,v2}} (disjoint, same as join)
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(bdddisjoin(z_v1, z_v2), z_v1v2);
}

TEST_F(BDDTest, ZDDDisjoinSameVar) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // {{v1}} ▷◁˙ {{v1}} = {} (v1 ∩ v1 = {v1} ≠ ∅, excluded)
    EXPECT_EQ(bdddisjoin(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, ZDDDisjoinOverlappingFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v1}}
    // Pairs: ({v1},{v1}): {v1}∩{v1}≠∅ excluded; ({v2},{v1}): disjoint → {v1,v2}
    bddp F = bddunion(z_v1, z_v2);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(bdddisjoin(F, z_v1), z_v1v2);
}

TEST_F(BDDTest, ZDDDisjoinCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);
    bddp G = bddunion(z_v2, z_v1);

    EXPECT_EQ(bdddisjoin(F, G), bdddisjoin(G, F));
}

TEST_F(BDDTest, ZDDDisjoinJoinDecomposition) {
    // Join = Disjoint join ∪ Joint join
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    bddp F = bddunion(z_v1, z_v1v2);   // {{v1}, {v1,v2}}
    bddp G = bddunion(z_v2, z_v3);      // {{v2}, {v3}}

    bddp join = bddjoin(F, G);
    bddp disjoin = bdddisjoin(F, G);
    bddp jointjoin = bddjointjoin(F, G);
    EXPECT_EQ(bddunion(disjoin, jointjoin), join);
}

TEST_F(BDDTest, ZDDDisjoinThreeVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1,v2}}, G = {{v2}, {v3}}
    bddp F = z_v1v2;
    bddp G = bddunion(z_v2, z_v3);

    // ({v1,v2},{v2}): {v1,v2}∩{v2}={v2}≠∅ → excluded
    // ({v1,v2},{v3}): disjoint → {v1,v2,v3}
    bddp z_v1v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(bdddisjoin(F, G), z_v1v2v3);
}

// --- bddjointjoin ---

TEST_F(BDDTest, ZDDJointjoinTerminalCases) {
    EXPECT_EQ(bddjointjoin(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddjointjoin(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddjointjoin(bddsingle, bddempty), bddempty);
    // {∅} ▷◁ˆ {∅}: ∅ ∩ ∅ = ∅, not ≠ ∅ → excluded
    EXPECT_EQ(bddjointjoin(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDJointjoinWithSingle) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    // {∅} ▷◁ˆ {{v1}}: ∅ ∩ {v1} = ∅ → excluded
    EXPECT_EQ(bddjointjoin(bddsingle, z_v1), bddempty);
    EXPECT_EQ(bddjointjoin(z_v1, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDJointjoinSameVar) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    // {{v1}} ▷◁ˆ {{v1}} = {{v1}} ({v1}∩{v1}={v1}≠∅, {v1}∪{v1}={v1})
    EXPECT_EQ(bddjointjoin(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDJointjoinDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // {{v1}} ▷◁ˆ {{v2}}: {v1}∩{v2}=∅ → excluded
    EXPECT_EQ(bddjointjoin(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, ZDDJointjoinOverlappingFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v1}}
    // ({v1},{v1}): {v1}∩{v1}≠∅ → {v1}
    // ({v2},{v1}): {v2}∩{v1}=∅ → excluded
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddjointjoin(F, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDJointjoinCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);
    bddp G = bddunion(z_v2, z_v1);

    EXPECT_EQ(bddjointjoin(F, G), bddjointjoin(G, F));
}

TEST_F(BDDTest, ZDDJointjoinComplex) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1,v2}}, G = {{v2}, {v3}}
    bddp F = z_v1v2;
    bddp G = bddunion(z_v2, z_v3);

    // ({v1,v2},{v2}): {v1,v2}∩{v2}={v2}≠∅ → {v1,v2}
    // ({v1,v2},{v3}): {v1,v2}∩{v3}=∅ → excluded
    EXPECT_EQ(bddjointjoin(F, G), z_v1v2);
}

TEST_F(BDDTest, ZDDJointjoinSubsetOfJoin) {
    // Joint join ⊆ Join (every joint join result is also a join result)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    bddp F = bddunion(z_v1, z_v1v2);
    bddp G = bddunion(z_v2, z_v3);

    bddp join = bddjoin(F, G);
    bddp jointjoin = bddjointjoin(F, G);
    // jointjoin ⊆ join means jointjoin \ join = ∅
    EXPECT_EQ(bddsubtract(jointjoin, join), bddempty);
}

TEST_F(BDDTest, ZDDDisjoinJointjoinCoverJoin) {
    // Join = Disjoin ∪ Jointjoin (with different families to avoid overlap)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));

    // F = {{v1}}, G = {{v2}, {v2,v3}}
    bddp F = z_v1;
    bddp G = bddunion(z_v2, z_v2v3);

    // All pairs are disjoint (v1 doesn't appear in G)
    bddp join = bddjoin(F, G);
    bddp disjoin = bdddisjoin(F, G);
    bddp jointjoin = bddjointjoin(F, G);
    EXPECT_EQ(bddunion(disjoin, jointjoin), join);
    EXPECT_EQ(disjoin, join);
    EXPECT_EQ(jointjoin, bddempty);
}

// --- bddrestrict ---

TEST_F(BDDTest, ZDDRestrictTerminalCases) {
    EXPECT_EQ(bddrestrict(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddrestrict(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddrestrict(bddsingle, bddempty), bddempty);
    // {∅} △ {∅}: ∃B∈{∅}: B⊆∅ → yes → {∅}
    EXPECT_EQ(bddrestrict(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDRestrictEmptySetFilter) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // G = {∅}: ∅ ⊆ every A → return F
    EXPECT_EQ(bddrestrict(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, ZDDRestrictSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddrestrict(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDRestrictSubsetFilter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));  // {{v1,v2}}

    // F = {{v1}, {v2}, {v1,v2}}, G = {{v1}}
    // {v1}⊆{v1}? yes. {v1}⊆{v2}? no. {v1}⊆{v1,v2}? yes.
    // result = {{v1}, {v1,v2}}
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    bddp expected = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddrestrict(F, z_v1), expected);
}

TEST_F(BDDTest, ZDDRestrictNoMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);  // {{v2}}

    // F = {{v1}}, G = {{v2}}: {v2}⊆{v1}? no → {}
    EXPECT_EQ(bddrestrict(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, ZDDRestrictMultipleFilters) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp z_v1v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));

    // F = {{v1,v2}, {v2,v3}, {v1,v2,v3}}, G = {{v1}, {v3}}
    // {v1}⊆{v1,v2}? yes. {v1}⊆{v2,v3}? no. {v3}⊆{v2,v3}? yes.
    // {v1}⊆{v1,v2,v3}? yes.
    // result = {{v1,v2}, {v2,v3}, {v1,v2,v3}}
    bddp F = bddunion(bddunion(z_v1v2, z_v2v3), z_v1v2v3);
    bddp G = bddunion(z_v1, z_v3);
    EXPECT_EQ(bddrestrict(F, G), F);
}

TEST_F(BDDTest, ZDDRestrictEmptySetInG) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F = {{v1}}, G = {{v2}} → {v2}⊄{v1} → {}
    EXPECT_EQ(bddrestrict(z_v1, z_v2), bddempty);

    // G = {{v2}, {}} → ∅⊆{v1} → {{v1}}
    bddp G2 = bddunion(z_v2, bddsingle);
    EXPECT_EQ(bddrestrict(z_v1, G2), z_v1);
}

TEST_F(BDDTest, ZDDRestrictContainsEmptyCheck) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // F = {∅}, G = {{v1}}: need B⊆∅, so B=∅. ∅∉G → {}
    EXPECT_EQ(bddrestrict(bddsingle, z_v1), bddempty);

    // G = {{v1}, {∅}}: ∅∈G → {∅}
    bddp G2 = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddrestrict(bddsingle, G2), bddsingle);
}

// --- bddpermit ---

TEST_F(BDDTest, ZDDPermitTerminalCases) {
    EXPECT_EQ(bddpermit(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddpermit(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddpermit(bddsingle, bddempty), bddempty);
    // {∅} ⊘ {∅}: ∅⊆∅ → {∅}
    EXPECT_EQ(bddpermit(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDPermitEmptySetAlwaysPermitted) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // F = {∅}: ∅ ⊆ any B → {∅}
    EXPECT_EQ(bddpermit(bddsingle, z_v1), bddsingle);
}

TEST_F(BDDTest, ZDDPermitSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddpermit(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDPermitSupersetFilter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1}, {v2}, {v1,v2}}, G = {{v1,v2}}
    // {v1}⊆{v1,v2}? yes. {v2}⊆{v1,v2}? yes. {v1,v2}⊆{v1,v2}? yes.
    // result = {{v1}, {v2}, {v1,v2}}
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddpermit(F, z_v1v2), F);
}

TEST_F(BDDTest, ZDDPermitStrictFilter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1}, {v2}, {v1,v2}}, G = {{v1}}
    // {v1}⊆{v1}? yes. {v2}⊆{v1}? no. {v1,v2}⊆{v1}? no.
    // result = {{v1}}
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddpermit(F, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDPermitNoMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F = {{v1}}, G = {{v2}}: {v1}⊆{v2}? no → {}
    EXPECT_EQ(bddpermit(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, ZDDPermitWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F = {{v1}, {∅}}, G = {{v2}}
    // {v1}⊆{v2}? no. ∅⊆{v2}? yes.
    // result = {{∅}}
    bddp F = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddpermit(F, z_v2), bddsingle);
}

TEST_F(BDDTest, ZDDPermitMultipleOptions) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1}, {v3}}, G = {{v1,v2}, {v3}}
    // {v1}⊆{v1,v2}? yes. {v3}⊆{v3}? yes.
    // result = {{v1}, {v3}}
    bddp F = bddunion(z_v1, z_v3);
    bddp G = bddunion(z_v1v2, z_v3);
    EXPECT_EQ(bddpermit(F, G), F);
}

TEST_F(BDDTest, ZDDPermitGSingletonOnly) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F = {{v1}, {v1,v2}}, G = {∅}
    // A ⊆ ∅ only when A = ∅. Neither {v1} nor {v1,v2} is ∅ → {}
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddpermit(F, bddsingle), bddempty);
}

// --- bddnonsup ---

TEST_F(BDDTest, ZDDNonsupTerminalCases) {
    EXPECT_EQ(bddnonsup(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddnonsup(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddnonsup(bddsingle, bddempty), bddsingle);
    // G={∅}: ∅⊆every A → none qualify
    EXPECT_EQ(bddnonsup(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDNonsupEmptyG) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    // G=∅ → all qualify
    EXPECT_EQ(bddnonsup(z_v1, bddempty), z_v1);
}

TEST_F(BDDTest, ZDDNonsupSingleG) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    // G={∅}: ∅⊆{v1} → none qualify
    EXPECT_EQ(bddnonsup(z_v1, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDNonsupSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddnonsup(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, ZDDNonsupNoSubset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F={{v1}}, G={{v2}}: {v2}⊄{v1} → {{v1}} qualifies
    EXPECT_EQ(bddnonsup(z_v1, z_v2), z_v1);
}

TEST_F(BDDTest, ZDDNonsupPartialMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F={{v1},{v2},{v1,v2}}, G={{v1}}
    // {v1}⊆{v1}? yes→out. {v1}⊆{v2}? no→keep. {v1}⊆{v1,v2}? yes→out.
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddnonsup(F, z_v1), z_v2);
}

TEST_F(BDDTest, ZDDNonsupEqualsSubtractRestrict) {
    // nonsup(F,G) = F \ restrict(F,G)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    bddp F = bddunion(bddunion(z_v1, z_v2), bddunion(z_v1v2, z_v3));
    bddp G = bddunion(z_v1, z_v3);

    EXPECT_EQ(bddnonsup(F, G), bddsubtract(F, bddrestrict(F, G)));
}

TEST_F(BDDTest, ZDDNonsupCheckEmptyInG) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    // F={∅}, G={{v1}}: need ∀B∈G: B⊄∅. {v1}⊄∅? yes → {∅} qualifies
    EXPECT_EQ(bddnonsup(bddsingle, z_v1), bddsingle);

    // F={∅}, G={{v1},{∅}}: ∅⊆∅ → fails
    bddp G2 = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddnonsup(bddsingle, G2), bddempty);
}

// --- bddnonsub ---

TEST_F(BDDTest, ZDDNonsubTerminalCases) {
    EXPECT_EQ(bddnonsub(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddnonsub(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddnonsub(bddsingle, bddempty), bddsingle);
    // F={∅}, G={∅}: ∅⊆∅ → fails
    EXPECT_EQ(bddnonsub(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDNonsubEmptyG) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddnonsub(z_v1, bddempty), z_v1);
}

TEST_F(BDDTest, ZDDNonsubSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddnonsub(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, ZDDNonsubNoSuperset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F={{v1}}, G={{v2}}: {v1}⊄{v2} → {{v1}} qualifies
    EXPECT_EQ(bddnonsub(z_v1, z_v2), z_v1);
}

TEST_F(BDDTest, ZDDNonsubPartialMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F={{v1},{v2},{v1,v2}}, G={{v1,v2}}
    // {v1}⊆{v1,v2}? yes→out. {v2}⊆{v1,v2}? yes→out. {v1,v2}⊆{v1,v2}? yes→out.
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddnonsub(F, z_v1v2), bddempty);
}

TEST_F(BDDTest, ZDDNonsubStrictFilter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F={{v1},{v2},{v1,v2}}, G={{v1}}
    // {v1}⊆{v1}? yes→out. {v2}⊆{v1}? no→keep. {v1,v2}⊆{v1}? no→keep.
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    bddp expected = bddunion(z_v2, z_v1v2);
    EXPECT_EQ(bddnonsub(F, z_v1), expected);
}

TEST_F(BDDTest, ZDDNonsubEqualsSubtractPermit) {
    // nonsub(F,G) = F \ permit(F,G)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    bddp F = bddunion(bddunion(z_v1, z_v2), bddunion(z_v1v2, z_v3));
    bddp G = bddunion(z_v1v2, z_v3);

    EXPECT_EQ(bddnonsub(F, G), bddsubtract(F, bddpermit(F, G)));
}

TEST_F(BDDTest, ZDDNonsubFVarNotInG) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // F={{v1},{v1,v2}}, G={{v2}}
    // {v1}⊆{v2}? no→keep. {v1,v2}⊆{v2}? no→keep.
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddnonsub(F, z_v2), F);
}

TEST_F(BDDTest, ZDDNonsubWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);

    // F={{v1},{∅}}, G={{v2}}
    // {v1}⊆{v2}? no→keep. ∅⊆{v2}? yes→out.
    bddp F = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddnonsub(F, z_v2), z_v1);
}

// ============================================================
// Parameterized tests for Recursive/Iterative modes (Batch 2)
// ============================================================

// bddand / bddand_iter share the same operation-cache op code. A naive
// "EXPECT_EQ(op(..., mode), op(...))" pattern can hide bugs: whichever
// invocation runs first caches its result, and the second invocation
// short-circuits at the top-level cache lookup without executing its own
// implementation. To actually exercise both the iterative and recursive
// paths we clear the operation cache before each call.
#define EXPECT_MODE_EQ(expr_mode, expr_default)               \
    do {                                                      \
        bdd_cache_clear();                                    \
        auto _actual_mode = (expr_mode);                      \
        bdd_cache_clear();                                    \
        auto _expected_def = (expr_default);                  \
        EXPECT_EQ(_actual_mode, _expected_def);               \
    } while (0)

class ZddOpsModeTest : public ::testing::TestWithParam<BddExecMode> {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

INSTANTIATE_TEST_SUITE_P(
    ExecModes,
    ZddOpsModeTest,
    ::testing::Values(BddExecMode::Recursive, BddExecMode::Iterative, BddExecMode::Auto),
    [](const ::testing::TestParamInfo<BddExecMode>& info) {
        switch (info.param) {
        case BddExecMode::Recursive: return "Recursive";
        case BddExecMode::Iterative: return "Iterative";
        case BddExecMode::Auto: return "Auto";
        }
        return "Unknown";
    }
);

// Build a moderately non-trivial ZDD family shared by several tests.
namespace {
struct TestFamilies {
    bddvar v1, v2, v3;
    bddp single_v1;     // {{v1}}
    bddp single_v2;     // {{v2}}
    bddp single_v3;     // {{v3}}
    bddp v1v2;          // {{v1,v2}}
    bddp v2v3;          // {{v2,v3}}
    bddp complex;       // union of v1, v1v2, v2v3
};

static TestFamilies make_families() {
    TestFamilies t;
    t.v1 = bddnewvar();
    t.v2 = bddnewvar();
    t.v3 = bddnewvar();
    t.single_v1 = ZDD::getnode(t.v1, bddempty, bddsingle);
    t.single_v2 = ZDD::getnode(t.v2, bddempty, bddsingle);
    t.single_v3 = ZDD::getnode(t.v3, bddempty, bddsingle);
    // {v1,v2} via join of singletons: but simpler: build manually
    // {v1,v2} = change(single_v1, v2) — or via join
    t.v1v2 = bddjoin(t.single_v1, t.single_v2);
    t.v2v3 = bddjoin(t.single_v2, t.single_v3);
    t.complex = bddunion(bddunion(t.single_v1, t.v1v2), t.v2v3);
    // Clear the op cache so that mode-comparison tests do not read back
    // results that were cached while the fixture was being built
    // (e.g. bddjoin/bddunion entries would otherwise short-circuit the
    // JoinFamily/UnionFamily tests before reaching either implementation).
    bdd_cache_clear();
    return t;
}
} // namespace

// --- Group A: unary + var ---

TEST_P(ZddOpsModeTest, OffsetTerminal) {
    auto t = make_families();
    EXPECT_EQ(bddoffset(bddempty, t.v1, GetParam()), bddempty);
    EXPECT_EQ(bddoffset(bddsingle, t.v1, GetParam()), bddsingle);
}

TEST_P(ZddOpsModeTest, OffsetFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddoffset(t.complex, t.v1, GetParam()), bddoffset(t.complex, t.v1));
    EXPECT_MODE_EQ(bddoffset(t.complex, t.v2, GetParam()), bddoffset(t.complex, t.v2));
    EXPECT_MODE_EQ(bddoffset(t.complex, t.v3, GetParam()), bddoffset(t.complex, t.v3));
}

TEST_P(ZddOpsModeTest, OnsetFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddonset(t.complex, t.v1, GetParam()), bddonset(t.complex, t.v1));
    EXPECT_MODE_EQ(bddonset(t.complex, t.v2, GetParam()), bddonset(t.complex, t.v2));
    EXPECT_MODE_EQ(bddonset(t.complex, t.v3, GetParam()), bddonset(t.complex, t.v3));
}

TEST_P(ZddOpsModeTest, Onset0Family) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddonset0(t.complex, t.v1, GetParam()), bddonset0(t.complex, t.v1));
    EXPECT_MODE_EQ(bddonset0(t.complex, t.v2, GetParam()), bddonset0(t.complex, t.v2));
}

TEST_P(ZddOpsModeTest, ChangeFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddchange(t.complex, t.v1, GetParam()), bddchange(t.complex, t.v1));
    EXPECT_MODE_EQ(bddchange(t.complex, t.v2, GetParam()), bddchange(t.complex, t.v2));
    EXPECT_MODE_EQ(bddchange(t.complex, t.v3, GetParam()), bddchange(t.complex, t.v3));
}

// --- Group B: simple binary ---

TEST_P(ZddOpsModeTest, UnionFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddunion(t.complex, t.v2v3, GetParam()), bddunion(t.complex, t.v2v3));
    EXPECT_MODE_EQ(bddunion(t.single_v1, t.v1v2, GetParam()), bddunion(t.single_v1, t.v1v2));
    EXPECT_EQ(bddunion(bddempty, t.complex, GetParam()), t.complex);
    EXPECT_EQ(bddunion(t.complex, bddempty, GetParam()), t.complex);
}

TEST_P(ZddOpsModeTest, IntersecFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddintersec(t.complex, t.v1v2, GetParam()), bddintersec(t.complex, t.v1v2));
    EXPECT_MODE_EQ(bddintersec(t.complex, t.v2v3, GetParam()), bddintersec(t.complex, t.v2v3));
    EXPECT_EQ(bddintersec(bddempty, t.complex, GetParam()), bddempty);
}

TEST_P(ZddOpsModeTest, SubtractFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddsubtract(t.complex, t.v1v2, GetParam()), bddsubtract(t.complex, t.v1v2));
    EXPECT_MODE_EQ(bddsubtract(t.complex, t.single_v1, GetParam()), bddsubtract(t.complex, t.single_v1));
    EXPECT_EQ(bddsubtract(t.complex, t.complex, GetParam()), bddempty);
}

TEST_P(ZddOpsModeTest, SymdiffFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddsymdiff(t.complex, t.v1v2, GetParam()), bddsymdiff(t.complex, t.v1v2));
    EXPECT_EQ(bddsymdiff(t.complex, t.complex, GetParam()), bddempty);
}

// --- Group C: bool binary ---

TEST_P(ZddOpsModeTest, IsSubset) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddissubset(t.single_v1, t.complex, GetParam()),
                   bddissubset(t.single_v1, t.complex));
    EXPECT_EQ(bddissubset(t.complex, t.complex, GetParam()), true);
    EXPECT_MODE_EQ(bddissubset(t.complex, t.single_v1, GetParam()),
                   bddissubset(t.complex, t.single_v1));
    EXPECT_EQ(bddissubset(bddempty, t.complex, GetParam()), true);
}

TEST_P(ZddOpsModeTest, IsDisjoint) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddisdisjoint(t.single_v1, t.single_v2, GetParam()),
                   bddisdisjoint(t.single_v1, t.single_v2));
    EXPECT_MODE_EQ(bddisdisjoint(t.complex, t.v2v3, GetParam()),
                   bddisdisjoint(t.complex, t.v2v3));
    EXPECT_EQ(bddisdisjoint(t.complex, t.complex, GetParam()), false);
    EXPECT_EQ(bddisdisjoint(bddempty, t.complex, GetParam()), true);
}

// --- Group D: composite binary ---

TEST_P(ZddOpsModeTest, DivFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bdddiv(t.complex, t.single_v1, GetParam()),
                   bdddiv(t.complex, t.single_v1));
    EXPECT_EQ(bdddiv(t.complex, bddsingle, GetParam()), t.complex);
    EXPECT_EQ(bdddiv(bddempty, t.single_v1, GetParam()), bddempty);
}

TEST_P(ZddOpsModeTest, JoinFamily) {
    auto t = make_families();
    EXPECT_EQ(bddjoin(t.single_v1, t.single_v2, GetParam()), t.v1v2);
    EXPECT_MODE_EQ(bddjoin(t.complex, t.single_v3, GetParam()),
                   bddjoin(t.complex, t.single_v3));
    EXPECT_EQ(bddjoin(t.complex, bddsingle, GetParam()), t.complex);
    EXPECT_EQ(bddjoin(bddempty, t.complex, GetParam()), bddempty);
}

TEST_P(ZddOpsModeTest, ProductFamily) {
    // product requires disjoint var sets; use single_v1 and single_v2
    auto t = make_families();
    EXPECT_MODE_EQ(bddproduct(t.single_v1, t.single_v2, GetParam()),
                   bddproduct(t.single_v1, t.single_v2));
    EXPECT_EQ(bddproduct(bddsingle, t.single_v1, GetParam()), t.single_v1);
}

TEST_P(ZddOpsModeTest, MeetFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bddmeet(t.complex, t.v1v2, GetParam()), bddmeet(t.complex, t.v1v2));
    EXPECT_EQ(bddmeet(t.complex, bddempty, GetParam()), bddempty);
    EXPECT_EQ(bddmeet(t.complex, bddsingle, GetParam()), bddsingle);
}

TEST_P(ZddOpsModeTest, DeltaFamily) {
    auto t = make_families();
    EXPECT_MODE_EQ(bdddelta(t.complex, t.v1v2, GetParam()),
                   bdddelta(t.complex, t.v1v2));
    EXPECT_EQ(bdddelta(t.complex, bddsingle, GetParam()), t.complex);
}

// Cross-validation test: build a deeper ZDD and ensure iterative matches recursive.
// Note: n=12 keeps the test fast. BDD_RecurLimit is 8192, so the Auto parameter
// dispatches to _rec at this depth and does not exercise the iterative path
// through the Auto selector — the explicit Iterative parameter covers that.
TEST_P(ZddOpsModeTest, CrossValidationLinearChain) {
    // Build a linear chain: {{v1},{v1,v2},{v1,v2,v3},...}
    const int n = 12;
    std::vector<bddvar> vars;
    for (int i = 0; i < n; ++i) vars.push_back(bddnewvar());

    // accum = {{v1}}, {{v1,v2}}, ..., {{v1,...,vn}}
    bddp s = bddsingle;
    bddp fam = bddempty;
    for (int i = 0; i < n; ++i) {
        s = bddjoin(s, ZDD::getnode(vars[i], bddempty, bddsingle));
        fam = bddunion(fam, s);
    }
    bddp other = bddunion(
        ZDD::getnode(vars[1], bddempty, bddsingle),
        ZDD::getnode(vars[4], bddempty, bddsingle));

    // Verify each op is consistent across modes. EXPECT_MODE_EQ clears the
    // op cache between the two invocations so both paths really execute.
    EXPECT_MODE_EQ(bddoffset(fam, vars[0], GetParam()), bddoffset(fam, vars[0]));
    EXPECT_MODE_EQ(bddonset(fam, vars[5], GetParam()), bddonset(fam, vars[5]));
    EXPECT_MODE_EQ(bddchange(fam, vars[3], GetParam()), bddchange(fam, vars[3]));
    EXPECT_MODE_EQ(bddunion(fam, other, GetParam()), bddunion(fam, other));
    EXPECT_MODE_EQ(bddintersec(fam, other, GetParam()), bddintersec(fam, other));
    EXPECT_MODE_EQ(bddsubtract(fam, other, GetParam()), bddsubtract(fam, other));
    EXPECT_MODE_EQ(bddsymdiff(fam, other, GetParam()), bddsymdiff(fam, other));
    EXPECT_MODE_EQ(bddmeet(fam, other, GetParam()), bddmeet(fam, other));
    EXPECT_MODE_EQ(bdddelta(fam, other, GetParam()), bdddelta(fam, other));
    EXPECT_EQ(bddjoin(fam, bddsingle, GetParam()), fam);
    EXPECT_EQ(bddissubset(fam, fam, GetParam()), true);
    EXPECT_EQ(bddisdisjoint(fam, fam, GetParam()), false);
}
