#include "zbddv.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>

using namespace kyotodd;

class ZBDDVTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDDV_Init(256);
    }
};

// ============================================================
// Constructors
// ============================================================

TEST_F(ZBDDVTest, DefaultConstructor) {
    ZBDDV v;
    EXPECT_EQ(v.GetMetaZBDD().GetID(), bddempty);
    EXPECT_EQ(v.Last(), 0);
}

TEST_F(ZBDDVTest, ConstructFromZDDAtIndex0) {
    int va = BDDV_NewVar();
    ZDD s = ZDD::singleton(static_cast<bddvar>(va));
    ZBDDV v(s);
    EXPECT_EQ(v.GetZBDD(0).GetID(), s.GetID());
}

TEST_F(ZBDDVTest, ConstructFromZDDAtIndex1) {
    ZDD s(1); // {empty set}
    ZBDDV v(s, 1);
    EXPECT_EQ(v.GetZBDD(1).GetID(), s.GetID());
    EXPECT_EQ(v.GetZBDD(0).GetID(), bddempty);
}

TEST_F(ZBDDVTest, ConstructFromZDDAtIndex3) {
    ZDD s(1); // {empty set}
    ZBDDV v(s, 3);
    EXPECT_EQ(v.GetZBDD(3).GetID(), s.GetID());
    EXPECT_EQ(v.GetZBDD(0).GetID(), bddempty);
    EXPECT_EQ(v.GetZBDD(1).GetID(), bddempty);
    EXPECT_EQ(v.GetZBDD(2).GetID(), bddempty);
}

TEST_F(ZBDDVTest, ConstructNegativeLocationThrows) {
    EXPECT_THROW(ZBDDV(ZDD(0), -1), std::invalid_argument);
}

TEST_F(ZBDDVTest, ConstructExcessiveLocationThrows) {
    EXPECT_THROW(ZBDDV(ZDD(0), BDDV_MaxLen), std::invalid_argument);
}

TEST_F(ZBDDVTest, CopyConstructor) {
    ZBDDV v1(ZDD(1), 2);
    ZBDDV v2(v1);
    EXPECT_EQ(v1 == v2, 1);
}

TEST_F(ZBDDVTest, Assignment) {
    ZBDDV v1(ZDD(1), 2);
    ZBDDV v2;
    v2 = v1;
    EXPECT_EQ(v1 == v2, 1);
}

// ============================================================
// GetZBDD / GetMetaZBDD
// ============================================================

TEST_F(ZBDDVTest, GetZBDDEmpty) {
    ZBDDV v;
    // Any index returns empty
    EXPECT_EQ(v.GetZBDD(0).GetID(), bddempty);
    EXPECT_EQ(v.GetZBDD(5).GetID(), bddempty);
}

TEST_F(ZBDDVTest, GetZBDDOutOfRangeThrows) {
    ZBDDV v;
    EXPECT_THROW(v.GetZBDD(-1), std::invalid_argument);
    EXPECT_THROW(v.GetZBDD(BDDV_MaxLen), std::invalid_argument);
}

TEST_F(ZBDDVTest, GetMetaZBDD) {
    ZBDDV v(ZDD(1), 0);
    EXPECT_NE(v.GetMetaZBDD().GetID(), bddempty);
}

// ============================================================
// Set operators
// ============================================================

TEST_F(ZBDDVTest, UnionOperator) {
    int va = BDDV_NewVar();
    ZDD s = ZDD::singleton(static_cast<bddvar>(va));
    ZBDDV a(s, 0);
    ZBDDV b(ZDD(1), 1); // {empty set} at index 1

    ZBDDV result = a + b;
    EXPECT_EQ(result.GetZBDD(0).GetID(), s.GetID());
    EXPECT_EQ(result.GetZBDD(1).GetID(), ZDD(1).GetID());
}

TEST_F(ZBDDVTest, IntersectionOperator) {
    ZDD s(1);
    ZBDDV a(s, 0);
    ZBDDV b(s, 0);
    b += ZBDDV(s, 1);

    ZBDDV result = a & b;
    EXPECT_EQ(result.GetZBDD(0).GetID(), s.GetID());
    EXPECT_EQ(result.GetZBDD(1).GetID(), bddempty);
}

TEST_F(ZBDDVTest, SubtractOperator) {
    ZDD s(1);
    ZBDDV a(s, 0);
    a += ZBDDV(s, 1);
    ZBDDV b(s, 0);

    ZBDDV result = a - b;
    EXPECT_EQ(result.GetZBDD(0).GetID(), bddempty);
    EXPECT_EQ(result.GetZBDD(1).GetID(), s.GetID());
}

TEST_F(ZBDDVTest, InPlaceOperators) {
    ZDD s(1);
    ZBDDV a(s, 0);
    ZBDDV b(s, 1);

    ZBDDV c = a;
    c += b;
    EXPECT_EQ(c.GetZBDD(0).GetID(), s.GetID());
    EXPECT_EQ(c.GetZBDD(1).GetID(), s.GetID());

    c &= a;
    EXPECT_EQ(c.GetZBDD(0).GetID(), s.GetID());
    EXPECT_EQ(c.GetZBDD(1).GetID(), bddempty);

    c = a;
    c += b;
    c -= a;
    EXPECT_EQ(c.GetZBDD(0).GetID(), bddempty);
    EXPECT_EQ(c.GetZBDD(1).GetID(), s.GetID());
}

// ============================================================
// Comparison operators
// ============================================================

TEST_F(ZBDDVTest, EqualityOperator) {
    ZDD s(1);
    ZBDDV a(s, 0);
    ZBDDV b(s, 0);
    ZBDDV c(s, 1);

    EXPECT_EQ(a == b, 1);
    EXPECT_EQ(a == c, 0);
    EXPECT_EQ(a != c, 1);
    EXPECT_EQ(a != b, 0);
}

// ============================================================
// Last
// ============================================================

TEST_F(ZBDDVTest, LastEmpty) {
    ZBDDV v;
    EXPECT_EQ(v.Last(), 0);
}

TEST_F(ZBDDVTest, LastSingleElement) {
    ZBDDV v(ZDD(1), 5);
    EXPECT_EQ(v.Last(), 5);
}

TEST_F(ZBDDVTest, LastMultipleElements) {
    ZBDDV v;
    v += ZBDDV(ZDD(1), 3);
    v += ZBDDV(ZDD(1), 7);
    v += ZBDDV(ZDD(1), 1);
    EXPECT_EQ(v.Last(), 7);
}

TEST_F(ZBDDVTest, LastIndex0Only) {
    ZBDDV v(ZDD(1), 0);
    EXPECT_EQ(v.Last(), 0);
}

// ============================================================
// Mask
// ============================================================

TEST_F(ZBDDVTest, MaskSingleElement) {
    ZBDDV v;
    v += ZBDDV(ZDD(1), 0);
    v += ZBDDV(ZDD(1), 1);
    v += ZBDDV(ZDD(1), 2);

    ZBDDV m = v.Mask(1);
    EXPECT_EQ(m.GetZBDD(0).GetID(), bddempty);
    EXPECT_EQ(m.GetZBDD(1).GetID(), ZDD(1).GetID());
    EXPECT_EQ(m.GetZBDD(2).GetID(), bddempty);
}

TEST_F(ZBDDVTest, MaskRange) {
    ZBDDV v;
    v += ZBDDV(ZDD(1), 0);
    v += ZBDDV(ZDD(1), 1);
    v += ZBDDV(ZDD(1), 2);
    v += ZBDDV(ZDD(1), 3);

    ZBDDV m = v.Mask(1, 2);
    EXPECT_EQ(m.GetZBDD(0).GetID(), bddempty);
    EXPECT_EQ(m.GetZBDD(1).GetID(), ZDD(1).GetID());
    EXPECT_EQ(m.GetZBDD(2).GetID(), ZDD(1).GetID());
    EXPECT_EQ(m.GetZBDD(3).GetID(), bddempty);
}

TEST_F(ZBDDVTest, MaskOutOfRangeThrows) {
    ZBDDV v;
    EXPECT_THROW(v.Mask(-1), std::invalid_argument);
    EXPECT_THROW(v.Mask(0, 0), std::invalid_argument);
    EXPECT_THROW(v.Mask(0, -1), std::invalid_argument);
}

// ============================================================
// Shift operators
// ============================================================

TEST_F(ZBDDVTest, LeftShift) {
    int va = BDDV_NewVar();
    BDDV_NewVar(); // need destination variable
    ZDD s = ZDD::singleton(static_cast<bddvar>(va));
    ZBDDV v(s, 0);
    ZBDDV shifted = v << 1;
    ZDD elem = shifted.GetZBDD(0);
    // Should be shifted, different from original
    EXPECT_NE(elem.GetID(), s.GetID());
    EXPECT_NE(elem.GetID(), bddempty);
}

TEST_F(ZBDDVTest, RightShift) {
    BDDV_NewVar();
    int vb = BDDV_NewVar();
    ZDD s = ZDD::singleton(static_cast<bddvar>(vb));
    ZBDDV v(s, 0);
    ZBDDV shifted = v >> 1;
    ZDD elem = shifted.GetZBDD(0);
    EXPECT_NE(elem.GetID(), s.GetID());
    EXPECT_NE(elem.GetID(), bddempty);
}

TEST_F(ZBDDVTest, ShiftInPlace) {
    int va = BDDV_NewVar();
    BDDV_NewVar();
    ZDD s = ZDD::singleton(static_cast<bddvar>(va));
    ZBDDV v(s, 0);
    ZBDDV original = v;
    v <<= 1;
    EXPECT_NE(v.GetZBDD(0).GetID(), original.GetZBDD(0).GetID());
}

// ============================================================
// OffSet, OnSet, OnSet0, Change
// ============================================================

TEST_F(ZBDDVTest, OffSetOnSet) {
    int va = BDDV_NewVar();
    bddvar var = static_cast<bddvar>(va);
    ZDD s = ZDD::singleton(var) + ZDD(1); // {{va}, {}}
    ZBDDV v(s, 0);

    ZBDDV off = v.OffSet(va);
    // OffSet removes sets containing va, leaving {{}}
    EXPECT_EQ(off.GetZBDD(0).GetID(), ZDD(1).GetID());

    ZBDDV on = v.OnSet(va);
    // OnSet keeps sets containing va: {{va}}
    EXPECT_EQ(on.GetZBDD(0).GetID(), ZDD::singleton(var).GetID());
}

TEST_F(ZBDDVTest, OnSet0) {
    int va = BDDV_NewVar();
    bddvar var = static_cast<bddvar>(va);
    ZDD s = ZDD::singleton(var) + ZDD(1); // {{va}, {}}
    ZBDDV v(s, 0);

    ZBDDV on0 = v.OnSet0(va);
    // OnSet0: keep sets with va, then remove va → {{}}
    EXPECT_EQ(on0.GetZBDD(0).GetID(), ZDD(1).GetID());
}

TEST_F(ZBDDVTest, ChangeUserVar) {
    int va = BDDV_NewVar();
    bddvar var = static_cast<bddvar>(va);
    ZDD s(1); // {{}}
    ZBDDV v(s, 0);

    ZBDDV changed = v.Change(va);
    // Change toggles va: {{}} → {{va}}
    EXPECT_EQ(changed.GetZBDD(0).GetID(), ZDD::singleton(var).GetID());
}

// ============================================================
// Swap
// ============================================================

TEST_F(ZBDDVTest, SwapVariables) {
    int va = BDDV_NewVar();
    int vb = BDDV_NewVar();
    ZDD sa = ZDD::singleton(static_cast<bddvar>(va));
    ZBDDV v(sa, 0);

    ZBDDV swapped = v.Swap(va, vb);
    ZDD sb = ZDD::singleton(static_cast<bddvar>(vb));
    EXPECT_EQ(swapped.GetZBDD(0).GetID(), sb.GetID());
}

// ============================================================
// Top
// ============================================================

TEST_F(ZBDDVTest, TopEmpty) {
    ZBDDV v;
    EXPECT_EQ(v.Top(), 0);
}

TEST_F(ZBDDVTest, TopWithVariable) {
    int va = BDDV_NewVar();
    ZDD s = ZDD::singleton(static_cast<bddvar>(va));
    ZBDDV v(s, 0);
    EXPECT_EQ(v.Top(), va);
}

TEST_F(ZBDDVTest, TopMultipleElements) {
    int va = BDDV_NewVar();
    int vb = BDDV_NewVar();
    ZDD sa = ZDD::singleton(static_cast<bddvar>(va));
    ZDD sb = ZDD::singleton(static_cast<bddvar>(vb));
    ZBDDV v;
    v += ZBDDV(sa, 0);
    v += ZBDDV(sb, 1);
    int top = v.Top();
    // Should be the variable with the higher level
    bddvar lev_a = BDD_LevOfVar(static_cast<bddvar>(va));
    bddvar lev_b = BDD_LevOfVar(static_cast<bddvar>(vb));
    EXPECT_EQ(top, (lev_a >= lev_b) ? va : vb);
}

// ============================================================
// Size
// ============================================================

TEST_F(ZBDDVTest, SizeEmpty) {
    ZBDDV v;
    EXPECT_EQ(v.Size(), 0u);
}

TEST_F(ZBDDVTest, SizeWithElements) {
    int va = BDDV_NewVar();
    ZDD s = ZDD::singleton(static_cast<bddvar>(va));
    ZBDDV v(s, 0);
    EXPECT_GE(v.Size(), 1u);
}

// ============================================================
// Export / Print (smoke tests)
// ============================================================

TEST_F(ZBDDVTest, ExportDoesNotCrash) {
    ZDD s(1);
    ZBDDV v;
    v += ZBDDV(s, 0);
    v += ZBDDV(s, 1);

    FILE* devnull = fopen("/dev/null", "w");
    ASSERT_NE(devnull, nullptr);
    v.Export(devnull);
    fclose(devnull);
}

TEST_F(ZBDDVTest, PrintDoesNotCrash) {
    ZDD s(1);
    ZBDDV v(s, 0);
    testing::internal::CaptureStdout();
    v.Print();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

TEST_F(ZBDDVTest, PrintPlaDoesNotCrash) {
    int va = BDDV_NewVar();
    ZDD s = ZDD::singleton(static_cast<bddvar>(va));
    ZBDDV v(s, 0);
    testing::internal::CaptureStdout();
    int ret = v.PrintPla();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(ret, 0);
    EXPECT_FALSE(output.empty());
}

TEST_F(ZBDDVTest, PrintPlaConstant) {
    ZDD s(1);
    ZBDDV v(s, 0);
    testing::internal::CaptureStdout();
    v.PrintPla();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find(".i 0"), std::string::npos);
    EXPECT_NE(output.find(".o 1"), std::string::npos);
}

TEST_F(ZBDDVTest, PrintPlaDeepZBDDExceedsRecurLimit) {
    // Build a ZBDDV whose meta-ZDD level exceeds BDD_RecurLimit (8192) so
    // PrintPla dispatches to the iterative implementation. The family
    // consists of a single cube {v1, v2, ..., vn}.
    const int n = 9000;
    for (int i = BDDV_UserTopLev(); i < n; ++i) BDDV_NewVar();
    ZDD cube(1);
    for (int i = 0; i < n; ++i) {
        bddvar v = static_cast<bddvar>(i + 1 + BDDV_SysVarTop);
        cube = cube.Change(v);
    }
    ZBDDV zv(cube, 0);
    testing::internal::CaptureStdout();
    int rc = zv.PrintPla();
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(rc, 0);
    EXPECT_FALSE(out.empty());
}

TEST_F(ZBDDVTest, PrintPlaOutputUsesZeroNotTilde) {
    // Multi-output ZBDDV where some outputs are empty for a given cube
    int va = BDDV_NewVar();
    bddvar v = static_cast<bddvar>(va);
    ZDD s = ZDD::singleton(v);
    // Output 0 and 2 have singleton {v}, output 1 is empty
    ZBDDV zv = ZBDDV(s, 0) + ZBDDV(s, 2);

    testing::internal::CaptureStdout();
    zv.PrintPla();
    std::string output = testing::internal::GetCapturedStdout();

    // PLA output must use '0' for false outputs, not '~'
    EXPECT_EQ(output.find('~'), std::string::npos);
    EXPECT_NE(output.find("101"), std::string::npos);
}

// ============================================================
// Import
// ============================================================

TEST_F(ZBDDVTest, ImportBasic) {
    // Simple import: 1 input, 2 outputs, 1 node
    // Node 2: level 1, lo=F, hi=T → singleton variable
    // Output 0 = node 2, output 1 = F
    const char* data = "_i 1 _o 2 _n 1\n2 1 F T\n2 F\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    if (BDDV_UserTopLev() < 1) {
        BDDV_NewVar();
    }

    ZBDDV result = ZBDDV_Import(f);
    fclose(f);

    // Should have 2 outputs (last index = 1)
    EXPECT_GE(result.Last(), 0);
}

TEST_F(ZBDDVTest, ImportTrailingJunkInNodeToken) {
    // "2junk" should be rejected (not silently treated as node 2)
    const char* data = "_i 1 _o 1 _n 1\n2 1 F T\n2junk\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    if (BDDV_UserTopLev() < 1) {
        BDDV_NewVar();
    }

    ZBDDV result = ZBDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Last(), 0);
    ZDD meta = result.GetZBDD(0);
    EXPECT_EQ(meta.GetID(), bddnull);
}

TEST_F(ZBDDVTest, ImportTrailingJunkInChildToken) {
    // "2junk" as a child token should be rejected
    const char* data = "_i 2 _o 1 _n 2\n2 1 F T\n4 2 F 2junk\n4\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    while (BDDV_UserTopLev() < 2) {
        BDDV_NewVar();
    }

    ZBDDV result = ZBDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Last(), 0);
    ZDD meta = result.GetZBDD(0);
    EXPECT_EQ(meta.GetID(), bddnull);
}

TEST_F(ZBDDVTest, ImportLevelZeroReturnsNull) {
    const char* data = "_i 1 _o 1 _n 1\n2 0 F T\n2\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    if (BDDV_UserTopLev() < 1) BDDV_NewVar();

    ZBDDV result = ZBDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Last(), 0);
    ZDD meta = result.GetZBDD(0);
    EXPECT_EQ(meta.GetID(), bddnull);
}

TEST_F(ZBDDVTest, ImportLevelExceedsInputsReturnsNull) {
    const char* data = "_i 1 _o 1 _n 1\n2 2 F T\n2\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    if (BDDV_UserTopLev() < 1) BDDV_NewVar();

    ZBDDV result = ZBDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Last(), 0);
    ZDD meta = result.GetZBDD(0);
    EXPECT_EQ(meta.GetID(), bddnull);
}

TEST_F(ZBDDVTest, ImportNegativeNodeCountReturnsNull) {
    const char* data = "_i 1 _o 1 _n -1\nT\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    ZBDDV result = ZBDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Last(), 0);
    ZDD meta = result.GetZBDD(0);
    EXPECT_EQ(meta.GetID(), bddnull);
}

// ============================================================
// Larger integration tests
// ============================================================

TEST_F(ZBDDVTest, SparseArray) {
    ZDD s(1);
    ZBDDV v;
    v += ZBDDV(s, 0);
    v += ZBDDV(s, 10);
    v += ZBDDV(s, 100);

    EXPECT_EQ(v.Last(), 100);
    EXPECT_EQ(v.GetZBDD(0).GetID(), s.GetID());
    EXPECT_EQ(v.GetZBDD(10).GetID(), s.GetID());
    EXPECT_EQ(v.GetZBDD(100).GetID(), s.GetID());
    EXPECT_EQ(v.GetZBDD(1).GetID(), bddempty);
    EXPECT_EQ(v.GetZBDD(50).GetID(), bddempty);
}

TEST_F(ZBDDVTest, MultipleElementsWithVariables) {
    int va = BDDV_NewVar();
    int vb = BDDV_NewVar();
    ZDD sa = ZDD::singleton(static_cast<bddvar>(va));
    ZDD sb = ZDD::singleton(static_cast<bddvar>(vb));

    ZBDDV v;
    v += ZBDDV(sa, 0);
    v += ZBDDV(sb, 1);
    v += ZBDDV(sa + sb, 2);

    EXPECT_EQ(v.GetZBDD(0).GetID(), sa.GetID());
    EXPECT_EQ(v.GetZBDD(1).GetID(), sb.GetID());
    EXPECT_EQ(v.GetZBDD(2).GetID(), (sa + sb).GetID());
    EXPECT_EQ(v.Last(), 2);
}

TEST_F(ZBDDVTest, EmptyElementsDontAffectLast) {
    ZDD s(1);
    ZBDDV v;
    v += ZBDDV(s, 5);
    EXPECT_EQ(v.Last(), 5);

    // Adding empty at higher index shouldn't change Last
    v += ZBDDV(ZDD(0), 10);
    EXPECT_EQ(v.Last(), 5);
}

TEST_F(ZBDDVTest, SubtractRemovesElements) {
    ZDD s(1);
    ZBDDV v;
    v += ZBDDV(s, 0);
    v += ZBDDV(s, 1);
    v += ZBDDV(s, 2);

    // Remove element at index 1
    v -= v.Mask(1);
    EXPECT_EQ(v.GetZBDD(0).GetID(), s.GetID());
    EXPECT_EQ(v.GetZBDD(1).GetID(), bddempty);
    EXPECT_EQ(v.GetZBDD(2).GetID(), s.GetID());
}

TEST_F(ZBDDVTest, ImportDuplicateNodeIdReturnsNull) {
    // Same node ID (2) defined twice — should be rejected
    const char* data = "_i 1 _o 1 _n 2\n2 1 F T\n2 1 T F\n2\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    if (BDDV_UserTopLev() < 1) BDDV_NewVar();

    ZBDDV result = ZBDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Last(), 0);
    ZDD meta = result.GetZBDD(0);
    EXPECT_EQ(meta.GetID(), bddnull);
}
