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

// --- bddmaximal ---

TEST_F(BDDTest, BddMaximalEmpty) {
    EXPECT_EQ(bddmaximal(bddempty), bddempty);
}

TEST_F(BDDTest, BddMaximalSingle) {
    EXPECT_EQ(bddmaximal(bddsingle), bddsingle);
}

TEST_F(BDDTest, BddMaximalSingleton) {
    // {{v1}} → maximal = {{v1}}
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddmaximal(z_v1), z_v1);
}

TEST_F(BDDTest, BddMaximalSubsetRemoved) {
    // F = {{v1}, {v1,v2}} → {v1} ⊊ {v1,v2}, so maximal = {{v1,v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddmaximal(F), z_v1v2);
}

TEST_F(BDDTest, BddMaximalNoSubsets) {
    // F = {{v1}, {v2}} → neither is subset of other → maximal = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddmaximal(F), F);
}

TEST_F(BDDTest, BddMaximalChain) {
    // F = {{v1}, {v1,v2}, {v1,v2,v3}} → maximal = {{v1,v2,v3}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v1v2v3 = ZDD::getnode(v3, bddempty,
        ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    bddp F = bddunion(bddunion(z_v1, z_v1v2), z_v1v2v3);
    EXPECT_EQ(bddmaximal(F), z_v1v2v3);
}

TEST_F(BDDTest, BddMaximalWithEmptySet) {
    // F = {∅, {v1}} → ∅ ⊊ {v1}, so maximal = {{v1}}
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddmaximal(F), z_v1);
}

TEST_F(BDDTest, BddMaximalMixed) {
    // F = {{v1}, {v2}, {v1,v2}} → maximal = {{v1,v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddmaximal(F), z_v1v2);
}

// --- bddminimal ---

TEST_F(BDDTest, BddMinimalEmpty) {
    EXPECT_EQ(bddminimal(bddempty), bddempty);
}

TEST_F(BDDTest, BddMinimalSingle) {
    EXPECT_EQ(bddminimal(bddsingle), bddsingle);
}

TEST_F(BDDTest, BddMinimalSingleton) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddminimal(z_v1), z_v1);
}

TEST_F(BDDTest, BddMinimalSupersetRemoved) {
    // F = {{v1}, {v1,v2}} → {v1} ⊊ {v1,v2}, so minimal = {{v1}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddminimal(F), z_v1);
}

TEST_F(BDDTest, BddMinimalNoSubsets) {
    // F = {{v1}, {v2}} → minimal = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminimal(F), F);
}

TEST_F(BDDTest, BddMinimalChain) {
    // F = {{v1}, {v1,v2}, {v1,v2,v3}} → minimal = {{v1}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v1v2v3 = ZDD::getnode(v3, bddempty,
        ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    bddp F = bddunion(bddunion(z_v1, z_v1v2), z_v1v2v3);
    EXPECT_EQ(bddminimal(F), z_v1);
}

TEST_F(BDDTest, BddMinimalWithEmptySet) {
    // F = {∅, {v1}} → minimal = {∅}
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddminimal(F), bddsingle);
}

TEST_F(BDDTest, BddMinimalMixed) {
    // F = {{v1}, {v2}, {v1,v2}} → minimal = {{v1}, {v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    bddp expected = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminimal(F), expected);
}

TEST_F(BDDTest, BddMaximalMinimalInverse) {
    // For any F: minimal(maximal(F)) = maximal(F) and maximal(minimal(F)) = minimal(F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1, z_v2), bddunion(z_v1v2, z_v2v3));
    bddp maxF = bddmaximal(F);
    bddp minF = bddminimal(F);
    EXPECT_EQ(bddminimal(maxF), maxF);
    EXPECT_EQ(bddmaximal(minF), minF);
}

// --- bddminhit ---

TEST_F(BDDTest, BddMinhitEmpty) {
    // minhit(∅) = {∅} (no constraints → empty set hits everything vacuously)
    EXPECT_EQ(bddminhit(bddempty), bddsingle);
}

TEST_F(BDDTest, BddMinhitSingle) {
    // minhit({∅}) = ∅ (impossible to hit ∅)
    EXPECT_EQ(bddminhit(bddsingle), bddempty);
}

TEST_F(BDDTest, BddMinhitSingleton) {
    // minhit({{v1}}) = {{v1}} (must contain v1)
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddminhit(z_v1), z_v1);
}

TEST_F(BDDTest, BddMinhitTwoDisjoint) {
    // minhit({{v1}, {v2}}) = {{v1,v2}} (must hit both)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(bddminhit(F), z_v1v2);
}

TEST_F(BDDTest, BddMinhitTwoOverlapping) {
    // minhit({{v1,v2}, {v2,v3}}) = {{v2}, {v1,v3}}
    // {v2} hits both, {v1,v3} also hits both ({v1}∩{v1,v2}≠∅, {v3}∩{v2,v3}≠∅)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp F = bddunion(z_v1v2, z_v2v3);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp expected = bddunion(z_v2, z_v1v3);
    EXPECT_EQ(bddminhit(F), expected);
}

TEST_F(BDDTest, BddMinhitMultipleSolutions) {
    // minhit({{v1}, {v2}}) = {{v1,v2}}
    // But minhit({{v1,v2}}) = {{v1}, {v2}} (pick either element)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp expected = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminhit(z_v1v2), expected);
}

TEST_F(BDDTest, BddMinhitWithEmptySet) {
    // minhit({∅, {v1}}) = ∅ (∅ ∈ F, impossible to hit)
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddminhit(F), bddempty);
}

TEST_F(BDDTest, BddMinhitDoubleApplication) {
    // minhit(minhit(F)) relates back to F in specific ways
    // For F = {{v1}, {v2}}: minhit = {{v1,v2}}, minhit(minhit) = {{v1},{v2}} = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminhit(bddminhit(F)), F);
}

TEST_F(BDDTest, BddMinhitThreeWay) {
    // minhit({{v1}, {v2}, {v3}}) = {{v1,v2,v3}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v3);
    bddp z_v1v2v3 = ZDD::getnode(v3, bddempty,
        ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(bddminhit(F), z_v1v2v3);
}

// --- bddclosure ---

TEST_F(BDDTest, BddClosureEmpty) {
    EXPECT_EQ(bddclosure(bddempty), bddempty);
}

TEST_F(BDDTest, BddClosureSingle) {
    EXPECT_EQ(bddclosure(bddsingle), bddsingle);
}

TEST_F(BDDTest, BddClosureSingleton) {
    // closure({{v1}}) = {{v1}, ∅}
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp expected = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddclosure(z_v1), expected);
}

TEST_F(BDDTest, BddClosureTwoSets) {
    // closure({{v1}, {v2}}) = {{v1}, {v2}, ∅} ({v1}∩{v2} = ∅)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp expected = bddunion(bddunion(z_v1, z_v2), bddsingle);
    EXPECT_EQ(bddclosure(F), expected);
}

TEST_F(BDDTest, BddClosureSubsets) {
    // closure({{v1}, {v1,v2}}) = {{v1,v2}, {v1}, {v2}, ∅}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    bddp expected = bddunion(bddunion(z_v1v2, bddunion(z_v1, z_v2)), bddsingle);
    EXPECT_EQ(bddclosure(F), expected);
}

TEST_F(BDDTest, BddClosureThreeSets) {
    // closure({{v1,v2}, {v2,v3}}) = {{v1,v2}, {v2,v3}, {v1}, {v2}, {v3}, ∅}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp F = bddunion(z_v1v2, z_v2v3);
    bddp expected = bddunion(bddunion(bddunion(z_v1v2, z_v2v3),
                              bddunion(z_v1, bddunion(z_v2, z_v3))),
                              bddsingle);
    EXPECT_EQ(bddclosure(F), expected);
}

TEST_F(BDDTest, BddClosureIdempotent) {
    // closure(closure(F)) = closure(F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp C = bddclosure(F);
    EXPECT_EQ(bddclosure(C), C);
}

TEST_F(BDDTest, BddClosureContainsOriginal) {
    // closure(F) ⊇ F always
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp z_v1v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1v2, z_v2v3), z_v1v3);
    bddp C = bddclosure(F);
    EXPECT_EQ(bddunion(C, F), C);
}

// --- bddisbdd / bddiszbdd ---

TEST_F(BDDTest, BddIsBddNotSupported) {
    EXPECT_THROW(bddisbdd(bddfalse), std::logic_error);
    EXPECT_THROW(bddisbdd(bddtrue), std::logic_error);
}

TEST_F(BDDTest, BddIsZbddNotSupported) {
    EXPECT_THROW(bddiszbdd(bddfalse), std::logic_error);
    EXPECT_THROW(bddiszbdd(bddtrue), std::logic_error);
}

// --- bddcard tests ---

TEST_F(BDDTest, BddCardTerminals) {
    // Empty family has 0 elements
    EXPECT_EQ(bddcard(bddempty), 0u);
    // Single family {{}} has 1 element
    EXPECT_EQ(bddcard(bddsingle), 1u);
}

TEST_F(BDDTest, BddCardSingleVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // {{v1}} -> 1 element
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddcard(x1), 1u);

    // {{v2}} -> 1 element
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddcard(x2), 1u);
}

TEST_F(BDDTest, BddCardUnion) {
    bddvar v1 = bddnewvar();

    // {{}, {v1}} -> 2 elements
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // {{v1}} ∪ {{}} = {{}, {v1}}
    EXPECT_EQ(bddcard(f), 2u);
}

TEST_F(BDDTest, BddCardMultipleVariables) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // {{v1}, {v2}, {v3}} -> 3 elements
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp f = bddunion(bddunion(x1, x2), x3);
    EXPECT_EQ(bddcard(f), 3u);
}

TEST_F(BDDTest, BddCardPowerSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Power set of {v1,v2} = {{}, {v1}, {v2}, {v1,v2}} -> 4 elements
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x12 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp f = bddunion(bddunion(bddunion(bddsingle, x1), x2), x12);
    EXPECT_EQ(bddcard(f), 4u);
}

TEST_F(BDDTest, BddCardComplement) {
    bddvar v1 = bddnewvar();

    // bddnot on ZDD toggles empty set membership
    // {{v1}} -> complement -> {{}, {v1}} if ∅ was not in the family
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddcard(x1), 1u);
    EXPECT_EQ(bddcard(bddnot(x1)), 2u);

    // {{}} -> complement -> {} (empty family)
    EXPECT_EQ(bddcard(bddsingle), 1u);
    EXPECT_EQ(bddcard(bddnot(bddsingle)), 0u);

    // {} -> complement -> {{}}
    EXPECT_EQ(bddcard(bddempty), 0u);
    EXPECT_EQ(bddcard(bddnot(bddempty)), 1u);
}

TEST_F(BDDTest, BddCardComplementWithEmptySet) {
    bddvar v1 = bddnewvar();

    // Family {{}, {v1}} contains ∅ -> complement removes ∅ -> {{v1}}
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddcard(f), 2u);
    EXPECT_EQ(bddcard(bddnot(f)), 1u);
}

TEST_F(BDDTest, BddCardLargerFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Build {{v1}, {v2}, {v3}, {v1,v2}, {v1,v3}, {v2,v3}, {v1,v2,v3}} = 7 elements
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp x12 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp x13 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp x23 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp x123 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));

    bddp f = bddunion(bddunion(bddunion(x1, x2), bddunion(x3, x12)),
                       bddunion(bddunion(x13, x23), x123));
    EXPECT_EQ(bddcard(f), 7u);

    // Add empty set -> full power set: 8 elements
    bddp g = bddunion(f, bddsingle);
    EXPECT_EQ(bddcard(g), 8u);
}

// --- Issue #1: node_max exhaustion returns bddnull instead of OOB write ---

TEST_F(BDDTest, GetNodeThrowsWhenNodeMaxExhausted) {
    bddinit(1, 1);  // capacity=1, max=1
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // First prime uses 1 node — should succeed
    bddp p1 = bddprime(v1);
    bddgc_protect(&p1);
    EXPECT_NE(p1, bddnull);
    // Second prime needs another node but max is reached — should throw
    EXPECT_THROW(bddprime(v2), std::overflow_error);
    bddgc_unprotect(&p1);
}

TEST_F(BDDTest, GetZNodeThrowsWhenNodeMaxExhausted) {
    bddinit(1, 1);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddgc_protect(&z1);
    EXPECT_NE(z1, bddnull);
    EXPECT_THROW(ZDD::getnode(v2, bddempty, bddsingle), std::overflow_error);
    bddgc_unprotect(&z1);
}

// --- Issue #2: bddnull propagation through all operations ---

TEST_F(BDDTest, BddNullPropagatesThroughBddOps) {
    bddvar v = bddnewvar();
    bddp f = bddprime(v);

    // bddnot
    EXPECT_EQ(bddnot(bddnull), bddnull);

    // BDD binary ops with bddnull as first arg
    EXPECT_EQ(bddand(bddnull, f), bddnull);
    EXPECT_EQ(bddor(bddnull, f), bddnull);
    EXPECT_EQ(bddxor(bddnull, f), bddnull);
    EXPECT_EQ(bddnand(bddnull, f), bddnull);
    EXPECT_EQ(bddnor(bddnull, f), bddnull);
    EXPECT_EQ(bddxnor(bddnull, f), bddnull);

    // BDD binary ops with bddnull as second arg
    EXPECT_EQ(bddand(f, bddnull), bddnull);
    EXPECT_EQ(bddor(f, bddnull), bddnull);
    EXPECT_EQ(bddxor(f, bddnull), bddnull);

    // bddite
    EXPECT_EQ(bddite(bddnull, f, bddfalse), bddnull);
    EXPECT_EQ(bddite(f, bddnull, bddfalse), bddnull);
    EXPECT_EQ(bddite(f, bddtrue, bddnull), bddnull);

    // bddimply
    EXPECT_EQ(bddimply(bddnull, f), -1);
    EXPECT_EQ(bddimply(f, bddnull), -1);

    // cofactor
    EXPECT_EQ(bddat0(bddnull, v), bddnull);
    EXPECT_EQ(bddat1(bddnull, v), bddnull);
    EXPECT_EQ(bddcofactor(bddnull, f), bddnull);
    EXPECT_EQ(bddcofactor(f, bddnull), bddnull);

    // quantification
    EXPECT_EQ(bddexist(bddnull, f), bddnull);
    EXPECT_EQ(bdduniv(bddnull, f), bddnull);

    // shift
    EXPECT_EQ(bddlshiftb(bddnull, 1), bddnull);
    EXPECT_EQ(bddrshiftb(bddnull, 1), bddnull);
    EXPECT_EQ(bddlshiftz(bddnull, 1), bddnull);
    EXPECT_EQ(bddrshiftz(bddnull, 1), bddnull);

    // support
    EXPECT_EQ(bddsupport(bddnull), bddnull);
    EXPECT_TRUE(bddsupport_vec(bddnull).empty());
}

TEST_F(BDDTest, BddNullPropagatesThroughZddOps) {
    bddvar v = bddnewvar();
    bddp z = ZDD::getnode(v, bddempty, bddsingle);

    // ZDD unary-var ops
    EXPECT_EQ(bddoffset(bddnull, v), bddnull);
    EXPECT_EQ(bddonset(bddnull, v), bddnull);
    EXPECT_EQ(bddonset0(bddnull, v), bddnull);
    EXPECT_EQ(bddchange(bddnull, v), bddnull);

    // ZDD binary ops with bddnull as first arg
    EXPECT_EQ(bddunion(bddnull, z), bddnull);
    EXPECT_EQ(bddintersec(bddnull, z), bddnull);
    EXPECT_EQ(bddsubtract(bddnull, z), bddnull);
    EXPECT_EQ(bdddiv(bddnull, z), bddnull);
    EXPECT_EQ(bddsymdiff(bddnull, z), bddnull);
    EXPECT_EQ(bddjoin(bddnull, z), bddnull);
    EXPECT_EQ(bddmeet(bddnull, z), bddnull);
    EXPECT_EQ(bdddelta(bddnull, z), bddnull);
    EXPECT_EQ(bddremainder(bddnull, z), bddnull);
    EXPECT_EQ(bdddisjoin(bddnull, z), bddnull);
    EXPECT_EQ(bddjointjoin(bddnull, z), bddnull);
    EXPECT_EQ(bddrestrict(bddnull, z), bddnull);
    EXPECT_EQ(bddpermit(bddnull, z), bddnull);
    EXPECT_EQ(bddnonsup(bddnull, z), bddnull);
    EXPECT_EQ(bddnonsub(bddnull, z), bddnull);

    // ZDD binary ops with bddnull as second arg
    EXPECT_EQ(bddunion(z, bddnull), bddnull);
    EXPECT_EQ(bddintersec(z, bddnull), bddnull);
    EXPECT_EQ(bddsubtract(z, bddnull), bddnull);
    EXPECT_EQ(bdddiv(z, bddnull), bddnull);

    // ZDD unary ops
    EXPECT_EQ(bddmaximal(bddnull), bddnull);
    EXPECT_EQ(bddminimal(bddnull), bddnull);
    EXPECT_EQ(bddminhit(bddnull), bddnull);
    EXPECT_EQ(bddclosure(bddnull), bddnull);

    // bddcard
    EXPECT_EQ(bddcard(bddnull), 0u);
}

// --- Issue 4: bddvar range checks ---
TEST_F(BDDTest, BddVarRangeCheckThrowsForInvalidVar) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddprime(v1);
    bddp z = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // var == 0 is invalid
    EXPECT_THROW(bddprime(0), std::invalid_argument);
    EXPECT_THROW(bddat0(f, 0), std::invalid_argument);
    EXPECT_THROW(bddat1(f, 0), std::invalid_argument);
    EXPECT_THROW(bddoffset(z, 0), std::invalid_argument);
    EXPECT_THROW(bddonset(z, 0), std::invalid_argument);
    EXPECT_THROW(bddonset0(z, 0), std::invalid_argument);
    EXPECT_THROW(bddchange(z, 0), std::invalid_argument);

    // var > bdd_varcount: bddprime auto-expands, others throw
    bddvar bad = bddvarused() + 1;
    EXPECT_NO_THROW(bddprime(bad));  // auto-expands variables
    EXPECT_THROW(bddat0(f, bad + 1), std::invalid_argument);
    EXPECT_THROW(bddat1(f, bad + 1), std::invalid_argument);
    EXPECT_THROW(bddoffset(z, bad + 1), std::invalid_argument);
    EXPECT_THROW(bddonset(z, bad + 1), std::invalid_argument);
    EXPECT_THROW(bddonset0(z, bad + 1), std::invalid_argument);

    // very large var: bddprime rejects > 65536
    EXPECT_THROW(bddprime(999999), std::invalid_argument);
    EXPECT_THROW(bddat0(f, 999999), std::invalid_argument);
    EXPECT_THROW(bddat1(f, 999999), std::invalid_argument);

    // bddexist/bdduniv with vector containing invalid var
    std::vector<bddvar> bad_vars = {v1, 0};
    EXPECT_THROW(bddexist(f, bad_vars), std::invalid_argument);
    EXPECT_THROW(bdduniv(f, bad_vars), std::invalid_argument);

    std::vector<bddvar> bad_vars2 = {999999};
    EXPECT_THROW(bddexist(f, bad_vars2), std::invalid_argument);
    EXPECT_THROW(bdduniv(f, bad_vars2), std::invalid_argument);
}

TEST_F(BDDTest, BddVarRangeCheckValidVarsStillWork) {
    bddvar v1 = bddnewvar();
    (void)bddnewvar();

    // Valid operations should still work
    bddp f = bddprime(v1);
    EXPECT_NE(f, bddnull);

    bddp r0 = bddat0(f, v1);
    EXPECT_EQ(r0, bddfalse);

    bddp r1 = bddat1(f, v1);
    EXPECT_EQ(r1, bddtrue);

    // ZDD ops with valid vars
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_NO_THROW(bddoffset(z, v1));
    EXPECT_NO_THROW(bddonset(z, v1));
    EXPECT_NO_THROW(bddonset0(z, v1));
    EXPECT_NO_THROW(bddchange(z, v1));

    // bddexist/bdduniv with valid vars
    std::vector<bddvar> vars = {v1};
    EXPECT_NO_THROW(bddexist(bddprime(v1), vars));
    EXPECT_NO_THROW(bdduniv(bddprime(v1), vars));
}

// --- bddite: post-normalization g == h check ---

TEST_F(BDDTest, BddItePostNormalizationGEqualsH) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);

    // After f-complement normalization (swap g,h) and g-complement
    // normalization (negate g,h), g == h may arise.
    // ITE(~f, g, ~g) -> normalize f: ITE(f, ~g, g) -> normalize g: comp=true, g'=g, h'=~g
    // Then g' == ~h' ... not equal.
    // But ITE(~f, g, g) = g is caught before normalization.
    // Construct a case: ITE(f, ~a, a) with f complemented:
    // After f-norm: ITE(~f_raw, ~a, a) -> swap: ITE(f_raw, a, ~a)
    // After g-norm: g=a (non-comp), h=~a, comp=false. g != h, normal recursion.
    // Now: ITE(~f, a, ~a):
    // After f-norm: swap -> ITE(f, ~a, a)
    // After g-norm: g was ~a, so negate both: g=a, h=~a, comp=true. g != h.

    // Best case: ITE(~p1, p2, bddnot(p2))
    // = ~p1 ? p2 : ~p2 = p1 XOR p2 XOR p2... no.
    // = ITE(~p1, p2, ~p2) = XOR(p1, p2)... wait:
    // ITE(f, g, h) = f ? g : h. ITE(~p1, p2, ~p2) = ~p1 ? p2 : ~p2
    // = ~(p1 XOR p2)... no. Let me just verify correctness.
    // ITE(~p1, p2, ~p2) = ~p1 ? p2 : ~p2 = XOR(p1, p2)
    bddp r1 = bddite(bddnot(p1), p2, bddnot(p2));
    EXPECT_EQ(r1, bddxor(p1, p2));

    // ITE(p3, ~p1, ~p2): verify via cofactors
    bddp r2 = bddite(p3, bddnot(p1), bddnot(p2));
    EXPECT_EQ(bddat1(r2, v3), bddnot(p1));
    EXPECT_EQ(bddat0(r2, v3), bddnot(p2));

    // g == h caught before normalization
    EXPECT_EQ(bddite(p1, bddnot(p2), bddnot(p2)), bddnot(p2));
    EXPECT_EQ(bddite(bddnot(p1), p2, p2), p2);
}

// --- bddexport: vector with large-ish data ---

TEST_F(BDDTest, ExportImportVectorRoundtrip) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);

    std::vector<bddp> roots = {bddand(p1, p2), bddor(p1, p2), bddtrue, bddfalse};
    std::ostringstream oss;
    bddexport(oss, roots);

    std::istringstream iss(oss.str());
    std::vector<bddp> imported;
    int ret = bddimport(iss, imported);
    EXPECT_EQ(ret, 4);
    ASSERT_EQ(imported.size(), 4u);
    EXPECT_EQ(imported[0], roots[0]);
    EXPECT_EQ(imported[1], roots[1]);
    EXPECT_EQ(imported[2], bddtrue);
    EXPECT_EQ(imported[3], bddfalse);
}

// --- BDD class high-level member functions ---

TEST_F(BDDTest, BDDClassAt0At1) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.At0(v1), BDD::False);
    EXPECT_EQ(f.At1(v1).GetID(), b.GetID());
    EXPECT_EQ(f.At0(v2).GetID(), BDD::False.GetID());
    EXPECT_EQ(f.At1(v2).GetID(), a.GetID());
}

TEST_F(BDDTest, BDDClassExist) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;

    // Exist with cube
    BDD cube = BDD_ID(bddprime(v1));
    EXPECT_EQ(f.Exist(cube).GetID(), b.GetID());

    // Exist with vector
    std::vector<bddvar> vars = {v1};
    EXPECT_EQ(f.Exist(vars).GetID(), b.GetID());

    // Exist with single variable
    EXPECT_EQ(f.Exist(v1).GetID(), b.GetID());
}

TEST_F(BDDTest, BDDClassUniv) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a | b;

    // Univ with cube
    BDD cube = BDD_ID(bddprime(v1));
    EXPECT_EQ(f.Univ(cube).GetID(), b.GetID());

    // Univ with vector
    std::vector<bddvar> vars = {v1};
    EXPECT_EQ(f.Univ(vars).GetID(), b.GetID());

    // Univ with single variable
    EXPECT_EQ(f.Univ(v1).GetID(), b.GetID());
}

TEST_F(BDDTest, BDDClassExistDuplicateVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;

    std::vector<bddvar> vars_dup = {v1, v1};
    std::vector<bddvar> vars_single = {v1};
    EXPECT_EQ(f.Exist(vars_dup).GetID(), f.Exist(vars_single).GetID());
}

TEST_F(BDDTest, BDDClassUnivDuplicateVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a | b;

    std::vector<bddvar> vars_dup = {v1, v1};
    std::vector<bddvar> vars_single = {v1};
    EXPECT_EQ(f.Univ(vars_dup).GetID(), f.Univ(vars_single).GetID());
}

TEST_F(BDDTest, BDDClassCofactor) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.Cofactor(a).GetID(), b.GetID());
}

TEST_F(BDDTest, BDDClassSupport) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;

    BDD sup = f.Support();
    EXPECT_EQ(sup.GetID(), bddsupport(f.GetID()));

    std::vector<bddvar> sv = f.SupportVec();
    EXPECT_EQ(sv.size(), 2u);
}

TEST_F(BDDTest, BDDClassImply) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.Imply(a), 1);
    EXPECT_EQ(a.Imply(f), 0);
}

TEST_F(BDDTest, BDDClassSize) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.Size(), bddsize(f.GetID()));
    EXPECT_EQ(BDD::True.Size(), 0u);
}

TEST_F(BDDTest, BDDClassIte) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);

    // ITE(a, b, False) = a & b
    BDD r = BDD::Ite(a, b, BDD::False);
    EXPECT_EQ(r.GetID(), (a & b).GetID());

    // ITE(a, True, b) = a | b
    BDD r2 = BDD::Ite(a, BDD::True, b);
    EXPECT_EQ(r2.GetID(), (a | b).GetID());
}

TEST_F(BDDTest, BDDClassPrint) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);

    // Capture stdout
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    f.Print();
    std::cout.rdbuf(old);

    std::string out = oss.str();
    bddvar top = bddtop(f.GetID());

    // Build expected string
    std::ostringstream expected;
    expected << "[ " << f.GetID()
             << " Var:" << top << "(" << bddlevofvar(top) << ")"
             << " Size:" << bddsize(f.GetID())
             << " ]";
    EXPECT_EQ(out, expected.str());
}

TEST_F(BDDTest, BDDClassPrintConstant) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    BDD::False.Print();
    std::cout.rdbuf(old);

    // Constant: Var:0(0) Size:0
    std::string out = oss.str();
    EXPECT_NE(out.find("Var:0(0)"), std::string::npos);
    EXPECT_NE(out.find("Size:0"), std::string::npos);
}

// --- BDD::Swap ---

TEST_F(BDDTest, BDDClassSwapSameVar) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    EXPECT_EQ(a.Swap(v1, v1), a);
}

TEST_F(BDDTest, BDDClassSwapSimple) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // Swap(v1, v2) on x1 should give x2
    BDD swapped = a.Swap(v1, v2);
    EXPECT_EQ(swapped, BDDvar(v2));
}

TEST_F(BDDTest, BDDClassSwapAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = BDDvar(v3);
    // f = v1 & v2, swap(v1, v3) should give v3 & v2
    BDD f = a & b;
    BDD swapped = f.Swap(v1, v3);
    EXPECT_EQ(swapped, c & b);
}

TEST_F(BDDTest, BDDClassSwapSymmetric) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);
    // Swapping twice should restore original
    EXPECT_EQ(f.Swap(v1, v2).Swap(v1, v2), f);
}

TEST_F(BDDTest, BDDClassSwapCommutative) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) | ~BDDvar(v2);
    // Swap(v1,v2) == Swap(v2,v1)
    EXPECT_EQ(f.Swap(v1, v2), f.Swap(v2, v1));
}

TEST_F(BDDTest, BDDClassSwapConstants) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    EXPECT_EQ(BDD::True.Swap(v1, v2), BDD::True);
    EXPECT_EQ(BDD::False.Swap(v1, v2), BDD::False);
    EXPECT_EQ(bddswap(bddnull, v1, v2), bddnull);
}

TEST_F(BDDTest, BDDClassSwapComplement) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & ~BDDvar(v2);
    // swap(~f, v1, v2) == ~swap(f, v1, v2)
    EXPECT_EQ((~f).Swap(v1, v2), ~f.Swap(v1, v2));
}

TEST_F(BDDTest, BDDClassSwapNonAdjacent) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddvar v4 = BDD_NewVar();
    bddvar v5 = BDD_NewVar();
    BDD a = BDDvar(v1), b = BDDvar(v2), c = BDDvar(v3);
    BDD d = BDDvar(v4), e = BDDvar(v5);
    // Swap v1 and v5 (4 variables between them)
    BDD f = (a & b) | (c & d) | e;
    BDD swapped = f.Swap(v1, v5);
    BDD expected = (e & b) | (c & d) | a;
    EXPECT_EQ(swapped, expected);
    // Involution: swap twice restores original
    EXPECT_EQ(swapped.Swap(v1, v5), f);
}

TEST_F(BDDTest, BDDClassSwapNonAdjacentComplex) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddvar v4 = BDD_NewVar();
    BDD a = BDDvar(v1), b = BDDvar(v2), c = BDDvar(v3), d = BDDvar(v4);
    // f involves v1 and v3 with v2 between them
    BDD f = (a & b & c) | (~a & d);
    BDD swapped = f.Swap(v1, v3);
    BDD expected = (c & b & a) | (~c & d);
    EXPECT_EQ(swapped, expected);
    // Commutativity
    EXPECT_EQ(f.Swap(v1, v3), f.Swap(v3, v1));
    // Involution
    EXPECT_EQ(f.Swap(v1, v3).Swap(v1, v3), f);
}

TEST_F(BDDTest, BDDClassSwapVariableOutOfRange) {
    bddvar v1 = BDD_NewVar();
    BDD f = BDDvar(v1);
    EXPECT_THROW(f.Swap(0, v1), std::invalid_argument);
    EXPECT_THROW(f.Swap(v1, bdd_varcount + 1), std::invalid_argument);
}

// --- BDD::Smooth ---

TEST_F(BDDTest, BDDClassSmoothConstant) {
    bddvar v1 = BDD_NewVar();
    EXPECT_EQ(BDD::True.Smooth(v1), BDD::True);
    EXPECT_EQ(BDD::False.Smooth(v1), BDD::False);
}

TEST_F(BDDTest, BDDClassSmoothSingleVar) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // ∃v1. v1 = true
    EXPECT_EQ(a.Smooth(v1), BDD::True);
}

TEST_F(BDDTest, BDDClassSmoothNotInBDD) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // ∃v2. v1 = v1  (v2 not in BDD)
    EXPECT_EQ(a.Smooth(v2), a);
}

TEST_F(BDDTest, BDDClassSmoothAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    // ∃v1. (v1 ∧ v2) = v2
    EXPECT_EQ(f.Smooth(v1), b);
}

TEST_F(BDDTest, BDDClassSmoothMatchesExist) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    BDD f = (BDDvar(v1) & BDDvar(v2)) | BDDvar(v3);
    // Smooth(v) should be the same as Exist(v)
    EXPECT_EQ(f.Smooth(v1), f.Exist(v1));
    EXPECT_EQ(f.Smooth(v2), f.Exist(v2));
    EXPECT_EQ(f.Smooth(v3), f.Exist(v3));
}

// --- BDD::Spread ---

TEST_F(BDDTest, BDDClassSpreadZero) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // Spread(0) = identity
    EXPECT_EQ(a.Spread(0), a);
}

TEST_F(BDDTest, BDDClassSpreadConstant) {
    (void)BDD_NewVar();
    EXPECT_EQ(BDD::True.Spread(1), BDD::True);
    EXPECT_EQ(BDD::False.Spread(1), BDD::False);
}

TEST_F(BDDTest, BDDClassSpreadNegativeThrows) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    EXPECT_THROW(a.Spread(-1), std::invalid_argument);
}

TEST_F(BDDTest, BDDClassSpreadSingleVar) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // Spread(1) on v1: at node v1, f0=false, f1=true
    // lo = f0.Spread(1) | f1.Spread(0) = false | true = true
    // hi = f1.Spread(1) | f0.Spread(0) = true | false = true
    // BDD::getnode(v1, true, true) = true (reduced)
    EXPECT_EQ(a.Spread(1), BDD::True);
}

TEST_F(BDDTest, BDDClassSpreadTwoVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    // f = v1, Spread(1): at top variable v2 (or v1 depending on ordering),
    // the 0-branch and 1-branch mix via OR
    // For a single variable f = v1 with 2 vars:
    // BDD has top = v2 (higher level), At0(v2) = v1, At1(v2) = v1
    // Wait, v1 doesn't depend on v2, so the BDD just has v1 at its node.
    // Actually the BDD of v1 only has one node with var v1.
    // Spread(1) decomposes by v1: f0 = false, f1 = true
    // lo = f0.Spread(1) | f1.Spread(0) = false | true = true
    // hi = f1.Spread(1) | f0.Spread(0) = true | false = true
    // result = BDD::getnode(v1, true, true) = true
    // So Spread(1) on a single var BDD with >1 vars defined = true
    BDD result = a.Spread(1);
    EXPECT_EQ(result, BDD::True);
}

TEST_F(BDDTest, BDDClassSpreadAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);
    // Spread should make result >= f (more true values)
    // f => Spread(f, k) for all k >= 0
    BDD spread1 = f.Spread(1);
    EXPECT_EQ(f.Imply(spread1), 1);
}

TEST_F(BDDTest, BDDClassXPrint0) {
    BDD f = BDDvar(BDD_NewVar());
    EXPECT_THROW(f.XPrint0(), std::logic_error);
}

TEST_F(BDDTest, BDDClassXPrint) {
    BDD f = BDDvar(BDD_NewVar());
    EXPECT_THROW(f.XPrint(), std::logic_error);
}

TEST_F(BDDTest, BDDClassExportOstream) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);

    std::ostringstream oss1;
    f.Export(oss1);

    std::ostringstream oss2;
    bddp p = f.GetID();
    bddexport(oss2, &p, 1);

    EXPECT_EQ(oss1.str(), oss2.str());
}

TEST_F(BDDTest, BDDClassExportFilePtr) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    f.Export(tmp);
    long len = std::ftell(tmp);
    EXPECT_GT(len, 0);

    std::rewind(tmp);
    bddp p;
    int ret = bddimport(tmp, &p, 1);
    std::fclose(tmp);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(p, f.GetID());
}

// --- ZDD class high-level member functions ---

TEST_F(BDDTest, ZDDClassMaximalMinimal) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    // {∅, {1}, {2}, {1,2}} → maximal = {{1,2}}, minimal = {∅}
    bddp z1 = bddchange(bddsingle, v1);  // {{1}}
    bddp z2 = bddchange(bddsingle, v2);  // {{2}}
    bddp z12 = bddchange(z1, v2);        // {{1,2}}
    bddp all = bddunion(bddunion(bddunion(bddsingle, z1), z2), z12);

    ZDD zall = ZDD_ID(all);
    EXPECT_EQ(zall.maximal().GetID(), z12);
    EXPECT_EQ(zall.minimal().GetID(), bddsingle);
}

TEST_F(BDDTest, ZDDClassMinhitClosure) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    // {{1}, {2}} → minhit = {{1,2}}
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp f = bddunion(z1, z2);

    ZDD zf = ZDD_ID(f);
    ZDD mh = zf.minhit();
    bddp z12 = bddchange(z1, v2);
    EXPECT_EQ(mh.GetID(), z12);

    // closure of {{1}} = {∅, {1}}
    ZDD z1w = ZDD_ID(z1);
    ZDD cl = z1w.closure();
    EXPECT_EQ(cl.GetID(), bddclosure(z1));
}

TEST_F(BDDTest, ZDDClassCard) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp f = bddunion(bddunion(bddsingle, z1), z2);

    ZDD zf = ZDD_ID(f);
    EXPECT_EQ(zf.Card(), 3u);
    EXPECT_EQ(ZDD::Empty.Card(), 0u);
    EXPECT_EQ(ZDD::Single.Card(), 1u);
}

TEST_F(BDDTest, ZDDClassRestrictPermit) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp z12 = bddchange(z1, v2);
    bddp f = bddunion(bddunion(z1, z2), z12);

    ZDD zf = ZDD_ID(f);
    ZDD zp = ZDD_ID(z1);  // permit set: {{1}}

    ZDD restricted = zf.Restrict(zp);
    EXPECT_EQ(restricted.GetID(), bddrestrict(f, z1));

    ZDD permitted = zf.Permit(zp);
    EXPECT_EQ(permitted.GetID(), bddpermit(f, z1));
}

TEST_F(BDDTest, ZDDClassNonsupNonsub) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp z12 = bddchange(z1, v2);
    bddp f = bddunion(bddunion(z1, z2), z12);

    ZDD zf = ZDD_ID(f);
    ZDD zg = ZDD_ID(z12);

    EXPECT_EQ(zf.remove_supersets(zg).GetID(), bddnonsup(f, z12));
    EXPECT_EQ(zf.remove_subsets(zg).GetID(), bddnonsub(f, z12));
}

TEST_F(BDDTest, ZDDClassDisjoinJointjoin) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);

    ZDD za = ZDD_ID(z1);
    ZDD zb = ZDD_ID(z2);

    EXPECT_EQ(za.disjoin(zb).GetID(), bdddisjoin(z1, z2));
    EXPECT_EQ(za.joint_join(zb).GetID(), bddjointjoin(z1, z2));
}

TEST_F(BDDTest, ZDDClassDelta) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp z12 = bddchange(z1, v2);
    bddp f = bddunion(z1, z12);

    ZDD zf = ZDD_ID(f);
    ZDD zg = ZDD_ID(z2);

    EXPECT_EQ(zf.delta(zg).GetID(), bdddelta(f, z2));
}

// --- Garbage collection tests ---

TEST_F(BDDTest, GCBasicProtectUnprotect) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp a = bddand(p1, p2);

    // Protect a, then GC — a should survive
    bddgc_protect(&a);
    uint64_t live_before = bddlive();
    bddgc();
    uint64_t live_after = bddlive();
    // a and its children (p1, p2) are reachable — they survive
    // p1 and p2 are also reachable from a
    EXPECT_LE(live_after, live_before);
    // The node for a should still be valid
    EXPECT_EQ(bddtop(a), v1 > v2 ? v1 : v2);
    bddgc_unprotect(&a);
}

TEST_F(BDDTest, GCCollectsUnreachableNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Create nodes that are only referenced by raw bddp (not protected)
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp unreachable = bddand(p1, p2);
    (void)unreachable;

    // Create a protected node
    bddp kept = bddprime(v3);
    bddgc_protect(&kept);

    uint64_t live_before = bddlive();
    bddgc();
    uint64_t live_after = bddlive();

    // Unprotected nodes (p1, p2, unreachable) should be collected
    // Only kept (v3 prime) should survive
    EXPECT_LT(live_after, live_before);
    EXPECT_EQ(bddtop(kept), v3);

    bddgc_unprotect(&kept);
}

TEST_F(BDDTest, GCFreeListReuse) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp a = bddand(p1, p2);
    (void)a;

    uint64_t used_before = bddused();

    // Nothing protected — GC should free all nodes
    bddgc();
    uint64_t live_after = bddlive();
    EXPECT_EQ(live_after, 0u);

    // Allocating new nodes should reuse freed slots
    bddp p3 = bddprime(v1);
    bddgc_protect(&p3);
    uint64_t used_after = bddused();
    // bddused() is high-water mark, should not increase
    EXPECT_EQ(used_after, used_before);

    bddgc_unprotect(&p3);
}

TEST_F(BDDTest, GCBDDClassAutoProtection) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // BDD objects automatically protect their root
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = a & b;

    bddp expected_root = c.GetID();

    bddgc();

    // c should survive because BDD object protects &c.root
    EXPECT_EQ(c.GetID(), expected_root);
    EXPECT_EQ(bddtop(c.GetID()), v1 > v2 ? v1 : v2);
}

TEST_F(BDDTest, GCBDDClassScopeProtection) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    uint64_t live_after_create;
    {
        BDD a = BDDvar(v1);
        BDD b = BDDvar(v2);
        BDD c = a & b;
        live_after_create = bddlive();
        EXPECT_GT(live_after_create, 0u);
    }
    // a, b, c are out of scope — destructors unprotected their roots

    bddgc();
    uint64_t live_after_gc = bddlive();
    EXPECT_EQ(live_after_gc, 0u);
}

TEST_F(BDDTest, GCZDDClassAutoProtection) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z1 = ZDD_ID(bddchange(bddsingle, v1));
    ZDD z2 = ZDD_ID(bddchange(bddsingle, v2));
    ZDD z3 = z1 + z2;

    bddp expected_root = z3.GetID();

    bddgc();

    // z3 should survive because ZDD object protects &z3.root
    EXPECT_EQ(z3.GetID(), expected_root);
}

TEST_F(BDDTest, GCAutoTriggerOnExhaustion) {
    // Use a small node_max so GC is triggered automatically
    bddinit(4, 8);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddvar v4 = bddnewvar();

    // Fill up nodes in a scope so they become unreachable
    {
        BDD a = BDDvar(v1);
        BDD b = BDDvar(v2);
        BDD c = a & b;
        BDD d = a | b;
        (void)c;
        (void)d;
    }
    // All BDD objects out of scope — nodes are unprotected

    // This operation should succeed because GC frees dead nodes
    // even though we're at the node limit
    BDD e = BDDvar(v3);
    BDD f = BDDvar(v4);
    BDD g = e & f;

    EXPECT_EQ(bddtop(g.GetID()), v3 > v4 ? v3 : v4);
}

TEST_F(BDDTest, GCPreservesOperationCorrectness) {
    bddinit(16, 32);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = BDDvar(v3);

    // Build some intermediate results that will require GC
    BDD r1 = a & b;
    BDD r2 = b | c;
    BDD r3 = r1 ^ r2;

    // Force GC
    bddgc();

    // Verify results are still correct after GC
    // r3 = (v1 & v2) ^ (v2 | v3)
    // Check by evaluating at all variable assignments
    for (int i = 0; i < 8; i++) {
        bool x1 = (i >> 0) & 1;
        bool x2 = (i >> 1) & 1;
        bool x3 = (i >> 2) & 1;

        bddp f = r3.GetID();
        f = x1 ? bddat1(f, v1) : bddat0(f, v1);
        f = x2 ? bddat1(f, v2) : bddat0(f, v2);
        f = x3 ? bddat1(f, v3) : bddat0(f, v3);

        bool expected = (x1 && x2) ^ (x2 || x3);
        EXPECT_EQ(f, expected ? bddtrue : bddfalse);
    }
}

TEST_F(BDDTest, GCManualProtectRawBddp) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Manual protect for raw bddp values
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp a = bddand(p1, p2);

    bddgc_protect(&a);
    bddgc();

    // a is still valid
    EXPECT_EQ(bddtop(a), v1 > v2 ? v1 : v2);

    // Unprotect and GC again — should be collected
    bddgc_unprotect(&a);
    bddgc();
    EXPECT_EQ(bddlive(), 0u);
}

TEST_F(BDDTest, GCDepthPreventsCollection) {
    bddvar v1 = bddnewvar();
    bddp p1 = bddprime(v1);
    (void)p1;

    uint64_t live_before = bddlive();
    EXPECT_GT(live_before, 0u);

    // NOTE: Directly manipulating internal bdd_gc_depth to test GC guard behavior.
    // This couples the test to the implementation detail that bddgc() is a no-op
    // when bdd_gc_depth > 0.
    extern int bdd_gc_depth;
    bdd_gc_depth = 1;
    bddgc();
    // GC should be a no-op at depth > 0
    EXPECT_EQ(bddlive(), live_before);

    bdd_gc_depth = 0;
    bddgc();
    // Now GC runs — p1 is unprotected
    EXPECT_EQ(bddlive(), 0u);
}

TEST_F(BDDTest, GCThresholdSetting) {
    double orig = bddgc_getthreshold();
    bddgc_setthreshold(0.5);
    EXPECT_DOUBLE_EQ(bddgc_getthreshold(), 0.5);
    bddgc_setthreshold(orig);
    EXPECT_DOUBLE_EQ(bddgc_getthreshold(), orig);
}

TEST_F(BDDTest, GCLiveCount) {
    EXPECT_EQ(bddlive(), 0u);

    bddvar v1 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddgc_protect(&p1);

    EXPECT_EQ(bddlive(), 1u);

    bddvar v2 = bddnewvar();
    bddp p2 = bddprime(v2);
    bddgc_protect(&p2);

    EXPECT_EQ(bddlive(), 2u);

    bddgc_unprotect(&p2);
    bddgc();
    // p2 collected, p1 survives
    EXPECT_EQ(bddlive(), 1u);

    bddgc_unprotect(&p1);
    bddgc();
    EXPECT_EQ(bddlive(), 0u);
}

TEST_F(BDDTest, GCReinitClearsState) {
    bddvar v1 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddgc_protect(&p1);

    EXPECT_GT(bddlive(), 0u);

    // Re-initialize throws while non-terminal roots exist
    EXPECT_THROW(bddinit(256), std::runtime_error);

    // After unprotecting, re-initialize succeeds
    bddgc_unprotect(&p1);
    bddinit(256);
    EXPECT_EQ(bddlive(), 0u);
    EXPECT_EQ(bddused(), 0u);
}

TEST_F(BDDTest, GCZDDAutoTrigger) {
    bddinit(4, 8);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Create and discard ZDD nodes
    {
        ZDD z1 = ZDD_ID(bddchange(bddsingle, v1));
        ZDD z2 = ZDD_ID(bddchange(bddsingle, v2));
        ZDD z3 = z1 + z2;
        (void)z3;
    }
    // All out of scope

    // Should succeed via auto GC
    ZDD z4 = ZDD_ID(bddchange(bddsingle, v3));
    ZDD z5 = ZDD_ID(bddchange(bddsingle, v1));
    ZDD z6 = z4 + z5;

    EXPECT_NE(z6.GetID(), bddempty);
}

// --- Reduced flag tests ---

TEST_F(BDDTest, ReducedFlag_TerminalsAreReduced) {
    EXPECT_TRUE(bddp_is_reduced(bddfalse));
    EXPECT_TRUE(bddp_is_reduced(bddtrue));
    EXPECT_TRUE(bddp_is_reduced(bddempty));
    EXPECT_TRUE(bddp_is_reduced(bddsingle));
}

TEST_F(BDDTest, ReducedFlag_PrimeNodeIsReduced) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_TRUE(bddp_is_reduced(p));
    EXPECT_TRUE(node_is_reduced(p));
}

TEST_F(BDDTest, ReducedFlag_BDDOperations) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp x1 = bddprime(v1);
    bddp x2 = bddprime(v2);
    bddp x3 = bddprime(v3);

    // AND
    bddp a = bddand(x1, x2);
    EXPECT_TRUE(bddp_is_reduced(a));
    EXPECT_TRUE(bdd_check_reduced(a));

    // OR
    bddp o = bddor(x1, x2);
    EXPECT_TRUE(bddp_is_reduced(o));
    EXPECT_TRUE(bdd_check_reduced(o));

    // XOR
    bddp x = bddxor(x1, x2);
    EXPECT_TRUE(bddp_is_reduced(x));
    EXPECT_TRUE(bdd_check_reduced(x));

    // NOT (complement edge)
    bddp n = bddnot(x1);
    EXPECT_TRUE(bddp_is_reduced(n));
    EXPECT_TRUE(bdd_check_reduced(n));

    // ITE
    bddp ite = bddite(x1, x2, x3);
    EXPECT_TRUE(bddp_is_reduced(ite));
    EXPECT_TRUE(bdd_check_reduced(ite));

    // Cofactor
    bddp c0 = bddat0(a, v1);
    EXPECT_TRUE(bddp_is_reduced(c0));
    bddp c1 = bddat1(a, v1);
    EXPECT_TRUE(bddp_is_reduced(c1));

    // Quantification
    bddp ex = bddexistvar(a, v1);
    EXPECT_TRUE(bddp_is_reduced(ex));
    bddp un = bddunivvar(a, v1);
    EXPECT_TRUE(bddp_is_reduced(un));
}

TEST_F(BDDTest, ReducedFlag_ZDDOperations) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();

    // Build ZDD sets: {v1}, {v2}, {v3}
    bddp s1 = bddchange(bddsingle, v1);
    bddp s2 = bddchange(bddsingle, v2);
    bddp s3 = bddchange(bddsingle, v3);

    EXPECT_TRUE(bddp_is_reduced(s1));
    EXPECT_TRUE(bddp_is_reduced(s2));
    EXPECT_TRUE(bddp_is_reduced(s3));

    // Union
    bddp u = bddunion(s1, s2);
    EXPECT_TRUE(bddp_is_reduced(u));
    EXPECT_TRUE(bdd_check_reduced(u));

    // Intersect
    bddp i = bddintersec(u, s1);
    EXPECT_TRUE(bddp_is_reduced(i));

    // Subtract
    bddp sub = bddsubtract(u, s1);
    EXPECT_TRUE(bddp_is_reduced(sub));

    // Onset/Offset
    bddp on = bddonset(u, v1);
    EXPECT_TRUE(bddp_is_reduced(on));
    bddp off = bddoffset(u, v1);
    EXPECT_TRUE(bddp_is_reduced(off));

    // Change
    bddp ch = bddchange(u, v3);
    EXPECT_TRUE(bddp_is_reduced(ch));
    EXPECT_TRUE(bdd_check_reduced(ch));
}

TEST_F(BDDTest, ReducedFlag_ComplexBDD_CheckReduced) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddvar v4 = BDD_NewVar();
    bddp x1 = bddprime(v1);
    bddp x2 = bddprime(v2);
    bddp x3 = bddprime(v3);
    bddp x4 = bddprime(v4);

    // Build complex BDD: (x1 & x2) | (x3 ^ x4)
    bddp complex = bddor(bddand(x1, x2), bddxor(x3, x4));
    EXPECT_TRUE(bdd_check_reduced(complex));

    // bdd_check_reduced on terminals
    EXPECT_TRUE(bdd_check_reduced(bddfalse));
    EXPECT_TRUE(bdd_check_reduced(bddtrue));
    EXPECT_TRUE(bdd_check_reduced(bddnull));
}

TEST_F(BDDTest, ReducedFlag_ComplexZDD_CheckReduced) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();

    // Build ZDD family: {{v1,v2}, {v2,v3}, {v1}}
    bddp s12 = bddchange(bddchange(bddsingle, v1), v2);
    bddp s23 = bddchange(bddchange(bddsingle, v2), v3);
    bddp s1 = bddchange(bddsingle, v1);
    bddp family = bddunion(bddunion(s12, s23), s1);
    EXPECT_TRUE(bdd_check_reduced(family));
}

TEST_F(BDDTest, ReducedFlag_GCPreservesFlag) {
    BDD_Init(64, 64);
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp x1 = bddprime(v1);
    bddp x2 = bddprime(v2);

    bddp result = bddand(x1, x2);
    bddgc_protect(&result);
    bddgc();
    EXPECT_TRUE(bddp_is_reduced(result));
    EXPECT_TRUE(bdd_check_reduced(result));
    bddgc_unprotect(&result);
}

TEST_F(BDDTest, ReducedFlag_ImportExport) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp x1 = bddprime(v1);
    bddp x2 = bddprime(v2);
    bddp orig = bddand(x1, x2);

    // Export
    std::ostringstream oss;
    bddp arr[1] = { orig };
    bddexport(oss, arr, 1);

    // Re-init and import
    BDD_Init(1024, UINT64_MAX);
    BDD_NewVar();
    BDD_NewVar();
    std::istringstream iss(oss.str());
    bddp imported;
    int ret = bddimport(iss, &imported, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(bddp_is_reduced(imported));
    EXPECT_TRUE(bdd_check_reduced(imported));
}

TEST_F(BDDTest, ReducedFlag_ZDD_ID) {
    bddvar v = BDD_NewVar();
    bddp p = bddchange(bddsingle, v);
    ZDD z = ZDD_ID(p);
    EXPECT_EQ(z.GetID(), p);
}

TEST_F(BDDTest, ReducedFlag_BDD_ID_ValidatesReduced) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // Should succeed — node is reduced
    EXPECT_NO_THROW(BDD_ID(p));
    // Terminals should succeed
    EXPECT_NO_THROW(BDD_ID(bddfalse));
    EXPECT_NO_THROW(BDD_ID(bddtrue));
}

// --- bdddump / bddvdump ---

static std::string capture_stdout(std::function<void()> fn) {
    fflush(stdout);
    int pipefd[2];
    if (pipe(pipefd) != 0) return "";
    int saved = dup(STDOUT_FILENO);
    if (saved < 0) { close(pipefd[0]); close(pipefd[1]); return ""; }
    if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
        close(saved); close(pipefd[0]); close(pipefd[1]); return "";
    }
    close(pipefd[1]);
    fn();
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);
    std::string result;
    char buf[1024];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        result.append(buf, n);
    }
    close(pipefd[0]);
    return result;
}

TEST_F(BDDTest, Bdddump_Null) {
    std::string out = capture_stdout([]{ bdddump(bddnull); });
    EXPECT_EQ(out, "RT = NULL\n\n");
}

TEST_F(BDDTest, Bdddump_False) {
    std::string out = capture_stdout([]{ bdddump(bddfalse); });
    EXPECT_EQ(out, "RT = 0\n\n");
}

TEST_F(BDDTest, Bdddump_True) {
    std::string out = capture_stdout([]{ bdddump(bddtrue); });
    EXPECT_EQ(out, "RT = ~0\n\n");
}

TEST_F(BDDTest, Bdddump_SingleVar) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);  // V1: lo=false, hi=true
    std::string out = capture_stdout([f]{ bdddump(f); });
    // f is node_id=2, index=1, var=1, lev=1
    // lo=bddfalse(val 0), hi=bddtrue=~bddfalse -> complement of const 0
    // bddtrue = BDD_CONST_FLAG | 1, but with complement edge representation:
    // BDD::getnode(v, bddfalse, bddtrue): lo=bddfalse (no comp), hi=bddtrue
    // bddtrue = 0x800000000001, which has BDD_CONST_FLAG set and value=1
    // So hi is a constant with value 1, no complement edge (BDD_COMP_FLAG=bit0, but
    // BDD_CONST_FLAG is bit 47). bddtrue has bit 0 set but it's a constant value, not comp flag.
    // Actually: bddtrue = BDD_CONST_FLAG | 1. The "1" here IS the value.
    // For display: f1 = bddtrue. Is BDD_COMP_FLAG set? BDD_COMP_FLAG = bit 0 = 1.
    // bddtrue & BDD_COMP_FLAG = 1. So neg1 = true.
    // f1abs = bddtrue & ~BDD_COMP_FLAG = BDD_CONST_FLAG | 0 = bddfalse.
    // f1abs & BDD_CONST_FLAG = true. val = f1abs & ~BDD_CONST_FLAG = 0.
    // So output: ~0. This matches the spec (bddtrue displays as ~0).
    uint64_t ndx = (f & ~BDD_COMP_FLAG) / 2;
    std::string expected = "N" + std::to_string(ndx) + " = [V1(1), 0, ~0]\n"
                           "RT = N" + std::to_string(ndx) + "\n\n";
    EXPECT_EQ(out, expected);
}

TEST_F(BDDTest, Bdddump_TwoVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp p1 = bddprime(v1);  // V1
    bddp p2 = bddprime(v2);  // V2
    bddp f = bddxor(p1, p2); // V1 XOR V2
    std::string out = capture_stdout([f]{ bdddump(f); });
    // Should contain node lines and RT line, ending with blank line
    EXPECT_NE(out.find("RT = "), std::string::npos);
    EXPECT_TRUE(out.size() >= 2 && out.substr(out.size()-2) == "\n\n");
}

TEST_F(BDDTest, Bdddump_NegatedRoot) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);
    bddp nf = bddnot(f);
    std::string out = capture_stdout([nf]{ bdddump(nf); });
    uint64_t ndx = (f & ~BDD_COMP_FLAG) / 2;
    std::string expected = "N" + std::to_string(ndx) + " = [V1(1), 0, ~0]\n"
                           "RT = ~N" + std::to_string(ndx) + "\n\n";
    EXPECT_EQ(out, expected);
}

TEST_F(BDDTest, Bddvdump_Basic) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    bddp arr[2] = {f, p1};
    std::string out = capture_stdout([&arr]{ bddvdump(arr, 2); });
    EXPECT_NE(out.find("RT0 = "), std::string::npos);
    EXPECT_NE(out.find("RT1 = "), std::string::npos);
    EXPECT_TRUE(out.size() >= 2 && out.substr(out.size()-2) == "\n\n");
}

TEST_F(BDDTest, Bddvdump_NullSentinel) {
    bddvar v1 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddp arr[3] = {p1, bddnull, bddfalse};
    std::string out = capture_stdout([&arr]{ bddvdump(arr, 3); });
    // RT0 should show the node, RT1 should show NULL, RT2 should not appear
    EXPECT_NE(out.find("RT0 = "), std::string::npos);
    EXPECT_NE(out.find("RT1 = NULL"), std::string::npos);
    EXPECT_EQ(out.find("RT2"), std::string::npos);
}

TEST_F(BDDTest, Bddvdump_SharedNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    // f shares p1 as a subnode; shared nodes should be printed only once
    bddp arr[2] = {f, bddnot(p1)};
    std::string out = capture_stdout([&arr]{ bddvdump(arr, 2); });
    uint64_t ndx1 = (p1 & ~BDD_COMP_FLAG) / 2;
    std::string nstr = "N" + std::to_string(ndx1) + " = ";
    // Count occurrences of the node line
    size_t count = 0;
    size_t pos = 0;
    while ((pos = out.find(nstr, pos)) != std::string::npos) {
        count++;
        pos += nstr.size();
    }
    EXPECT_EQ(count, 1u);
}

TEST_F(BDDTest, Bddgraph0_Throws) {
    EXPECT_THROW(bddgraph0(bddfalse), std::logic_error);
}

TEST_F(BDDTest, Bddgraph_Throws) {
    EXPECT_THROW(bddgraph(bddfalse), std::logic_error);
}

TEST_F(BDDTest, Bddvgraph0_Throws) {
    bddp arr[1] = {bddfalse};
    EXPECT_THROW(bddvgraph0(arr, 1), std::logic_error);
}

TEST_F(BDDTest, Bddvgraph_Throws) {
    bddp arr[1] = {bddfalse};
    EXPECT_THROW(bddvgraph(arr, 1), std::logic_error);
}

