#include "pidd.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

class PiDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        PiDD_TopVar = 0;
        PiDD_VarTableSize = 16;
        if (PiDD_XOfLev) { delete[] PiDD_XOfLev; PiDD_XOfLev = 0; }
    }
};

/* ---- PiDD_NewVar ---- */
TEST_F(PiDDTest, NewVarBasic) {
    EXPECT_EQ(PiDD_VarUsed(), 0);
    int v1 = PiDD_NewVar();
    EXPECT_EQ(v1, 1);
    EXPECT_EQ(PiDD_VarUsed(), 1);

    int v2 = PiDD_NewVar();
    EXPECT_EQ(v2, 2);
    EXPECT_EQ(PiDD_VarUsed(), 2);

    int v3 = PiDD_NewVar();
    EXPECT_EQ(v3, 3);
    EXPECT_EQ(PiDD_VarUsed(), 3);

    int v4 = PiDD_NewVar();
    EXPECT_EQ(v4, 4);
    EXPECT_EQ(PiDD_VarUsed(), 4);
}

/* ---- Level layout ---- */
TEST_F(PiDDTest, LevelLayout) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    /* After 4 PiDD vars, BDD levels should be:
       lev 1: (2,1), lev 2: (3,2), lev 3: (3,1),
       lev 4: (4,3), lev 5: (4,2), lev 6: (4,1) */
    EXPECT_EQ(PiDD_X_Lev(1), 2);
    EXPECT_EQ(PiDD_Y_Lev(1), 1);

    EXPECT_EQ(PiDD_X_Lev(2), 3);
    EXPECT_EQ(PiDD_Y_Lev(2), 2);

    EXPECT_EQ(PiDD_X_Lev(3), 3);
    EXPECT_EQ(PiDD_Y_Lev(3), 1);

    EXPECT_EQ(PiDD_X_Lev(4), 4);
    EXPECT_EQ(PiDD_Y_Lev(4), 3);

    EXPECT_EQ(PiDD_X_Lev(5), 4);
    EXPECT_EQ(PiDD_Y_Lev(5), 2);

    EXPECT_EQ(PiDD_X_Lev(6), 4);
    EXPECT_EQ(PiDD_Y_Lev(6), 1);
}

/* ---- PiDD_Lev_XY ---- */
TEST_F(PiDDTest, LevXY) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    EXPECT_EQ(PiDD_Lev_XY(2, 1), 1);
    EXPECT_EQ(PiDD_Lev_XY(3, 2), 2);
    EXPECT_EQ(PiDD_Lev_XY(3, 1), 3);
    EXPECT_EQ(PiDD_Lev_XY(4, 3), 4);
    EXPECT_EQ(PiDD_Lev_XY(4, 2), 5);
    EXPECT_EQ(PiDD_Lev_XY(4, 1), 6);
}

/* ---- Constructors ---- */
TEST_F(PiDDTest, Constructors) {
    PiDD empty(0);
    PiDD identity(1);
    PiDD undef(-1);

    EXPECT_EQ(empty.Card(), 0u);
    EXPECT_EQ(identity.Card(), 1u);
    EXPECT_TRUE(empty == PiDD(0));
    EXPECT_TRUE(identity == PiDD(1));
    EXPECT_TRUE(empty != identity);
}

/* ---- Swap basic ---- */
TEST_F(PiDDTest, SwapIdentity) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);  /* identity permutation */

    /* Swapping identity with (2,1) gives the single transposition {(2,1)} */
    PiDD s = id.Swap(2, 1);
    EXPECT_EQ(s.Card(), 1u);

    /* Swapping twice returns to identity */
    PiDD s2 = s.Swap(2, 1);
    EXPECT_TRUE(s2 == id);
}

TEST_F(PiDDTest, SwapSameElement) {
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD s = id.Swap(1, 1);
    EXPECT_TRUE(s == id);
}

/* ---- Set operations ---- */
TEST_F(PiDDTest, SetOperations) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);  /* transposition (2,1) */
    PiDD t13 = id.Swap(3, 1);  /* transposition (3,1) */

    PiDD sum = t12 + t13;
    EXPECT_EQ(sum.Card(), 2u);

    PiDD inter = sum & t12;
    EXPECT_TRUE(inter == t12);

    PiDD diff = sum - t12;
    EXPECT_TRUE(diff == t13);
}

/* ---- Composition ---- */
TEST_F(PiDDTest, CompositionIdentity) {
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);

    /* id * t12 = t12 */
    EXPECT_TRUE((id * t12) == t12);
    /* t12 * id = t12 */
    EXPECT_TRUE((t12 * id) == t12);
}

TEST_F(PiDDTest, CompositionSelfInverse) {
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);

    /* (2,1) composed with (2,1) = identity */
    PiDD result = t12 * t12;
    EXPECT_TRUE(result == id);
}

/* ---- Odd / Even ---- */
TEST_F(PiDDTest, OddEven) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);
    PiDD t13 = id.Swap(3, 1);

    /* Single transpositions are odd permutations */
    PiDD set = id + t12 + t13;
    EXPECT_EQ(set.Card(), 3u);

    PiDD odd = set.Odd();
    PiDD even = set.Even();

    /* t12 and t13 are odd; id is even */
    EXPECT_EQ(odd.Card(), 2u);
    EXPECT_EQ(even.Card(), 1u);
    EXPECT_TRUE(even == id);
}

/* ---- Cofact ---- */
TEST_F(PiDDTest, CofactBasic) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);  /* maps 1->2, 2->1, 3->3 */
    PiDD t13 = id.Swap(3, 1);  /* maps 1->3, 3->1, 2->2 */

    PiDD set = id + t12 + t13;

    /* Cofact(1, 1): permutations where pi(1) = 1 => only identity */
    PiDD c11 = set.Cofact(1, 1);
    EXPECT_EQ(c11.Card(), 1u);

    /* Cofact(1, 2): permutations where pi(1) = 2 => t12 */
    PiDD c12 = set.Cofact(1, 2);
    EXPECT_EQ(c12.Card(), 1u);

    /* Cofact(1, 3): permutations where pi(1) = 3 => t13 */
    PiDD c13 = set.Cofact(1, 3);
    EXPECT_EQ(c13.Card(), 1u);
}

/* ---- Division ---- */
TEST_F(PiDDTest, DivisionByIdentity) {
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);
    PiDD set = id + t12;

    /* f / {id} = f */
    PiDD q = set / id;
    EXPECT_TRUE(q == set);
}

TEST_F(PiDDTest, DivisionProperty) {
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);
    PiDD set = id + t12;

    /* (f / p) * p should be a subset of f */
    PiDD q = set / t12;
    PiDD product = q * t12;
    EXPECT_TRUE((product - set) == PiDD(0));
}

/* ---- SwapBound ---- */
TEST_F(PiDDTest, SwapBoundZero) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);
    PiDD t13 = id.Swap(3, 1);

    PiDD set = id + t12 + t13;

    /* SwapBound(0): only identity (0 transpositions) */
    PiDD bounded0 = set.SwapBound(0);
    EXPECT_EQ(bounded0.Card(), 1u);
    EXPECT_TRUE(bounded0 == id);

    /* SwapBound(1): permutations with at most 1 transposition */
    PiDD bounded1 = set.SwapBound(1);
    EXPECT_EQ(bounded1.Card(), 3u);  /* id, t12, t13 all have <= 1 transposition */
}

/* ---- Remainder ---- */
TEST_F(PiDDTest, RemainderProperty) {
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);

    /* f % id = f - (f / id) * id = f - f * id = f - f = 0 */
    PiDD set = id + t12;
    PiDD rem = set % id;
    EXPECT_EQ(rem.Card(), 0u);
}

/* ---- Composition of three-element permutations ---- */
TEST_F(PiDDTest, CompositionS3) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD t12 = id.Swap(2, 1);
    PiDD t23 = id.Swap(3, 2);

    /* (1,2) then (2,3): first swap 1<->2, then swap 2<->3 */
    /* = cycle (1 3 2): 1->2->3, 2->1, 3->2... wait let me think carefully */
    /* Composition: (2,3) ∘ (1,2) means apply (1,2) first, then (2,3) */
    /* (1,2): 1->2, 2->1, 3->3 */
    /* then (2,3): 2->3, 3->2 */
    /* So: 1->2->3, 2->1->1, 3->3->2 = [3 1 2] */
    PiDD cycle = t23 * t12;
    EXPECT_EQ(cycle.Card(), 1u);

    /* cycle * cycle should give another 3-cycle */
    PiDD cycle2 = cycle * cycle;
    EXPECT_EQ(cycle2.Card(), 1u);

    /* cycle^3 = identity */
    PiDD cycle3 = cycle2 * cycle;
    EXPECT_TRUE(cycle3 == id);
}

// --- Terminal API without NewVar ---

TEST_F(PiDDTest, TerminalTopXY_NoNewVar) {
    PiDD empty(0);
    PiDD identity(1);
    EXPECT_EQ(empty.TopX(), 0);
    EXPECT_EQ(empty.TopY(), 0);
    EXPECT_EQ(identity.TopX(), 0);
    EXPECT_EQ(identity.TopY(), 0);
}

TEST_F(PiDDTest, OddEven_Terminal_NoNewVar) {
    PiDD identity(1);
    PiDD odd = identity.Odd();
    EXPECT_EQ(odd.Card(), 0u);  // identity is even
    PiDD even = identity.Even();
    EXPECT_EQ(even.Card(), 1u);
}

/* ---- XOfLev zero-initialization after resize ---- */
TEST_F(PiDDTest, XOfLev_ZeroInitAfterResize) {
    /* Create enough PiDD variables to trigger XOfLev table resize.
       Initial size is 16; 7 PiDD vars use 1+2+3+4+5+6 = 21 BDD levels,
       which exceeds 16 and triggers resize. */
    for (int i = 0; i < 7; i++) {
        PiDD_NewVar();
    }
    EXPECT_GT(PiDD_VarTableSize, 16);
    /* Check that XOfLev entries beyond those written are 0 */
    int toplev = static_cast<int>(bddlevofvar(bddvarused()));
    for (int lev = toplev + 1; lev < PiDD_VarTableSize; lev++) {
        EXPECT_EQ(PiDD_XOfLev[lev], 0);
    }
}

/* ---- Enum/Enum2 output ---- */
TEST_F(PiDDTest, Enum_Output) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD id(1);
    PiDD swap12 = id.Swap(2, 1);
    PiDD set = id + swap12;  // {identity, (2 1)}
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    set.Enum();
    std::cout.rdbuf(old);
    std::string result = oss.str();
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("+"), std::string::npos);  // two permutations
}

/* ---- Move semantics ---- */
TEST_F(PiDDTest, MoveConstructor) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD a = PiDD(1).Swap(2, 1);
    uint64_t expected_card = a.Card();
    PiDD b(std::move(a));
    EXPECT_EQ(b.Card(), expected_card);
}

TEST_F(PiDDTest, MoveAssignment) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD a = PiDD(1).Swap(2, 1);
    uint64_t expected_card = a.Card();
    PiDD b;
    b = std::move(a);
    EXPECT_EQ(b.Card(), expected_card);
}

TEST_F(PiDDTest, Enum2_Output) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD id(1);
    PiDD swap12 = id.Swap(2, 1);
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    swap12.Enum2();
    std::cout.rdbuf(old);
    std::string result = oss.str();
    EXPECT_FALSE(result.empty());
}

/* ---- Error handling: exceptions instead of std::exit ---- */
TEST_F(PiDDTest, SwapThrowsOnOutOfRange) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD id(1);
    EXPECT_THROW(id.Swap(0, 1), std::invalid_argument);
    EXPECT_THROW(id.Swap(1, 0), std::invalid_argument);
    EXPECT_THROW(id.Swap(3, 1), std::invalid_argument);
    EXPECT_THROW(id.Swap(1, 3), std::invalid_argument);
}

TEST_F(PiDDTest, CofactThrowsOnOutOfRange) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD id(1);
    EXPECT_THROW(id.Cofact(0, 1), std::invalid_argument);
    EXPECT_THROW(id.Cofact(3, 1), std::invalid_argument);
}

TEST_F(PiDDTest, DivisionByZeroThrows) {
    PiDD_NewVar();
    PiDD id(1);
    PiDD empty(0);
    EXPECT_THROW(id / empty, std::invalid_argument);
}

/* ---- save_svg ---- */
TEST_F(PiDDTest, SaveSvgString) {
    PiDD_NewVar();
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD s12 = id.Swap(1, 2);  // transposition (1,2)
    PiDD perm = s12 + id;      // {identity, (1,2)}

    // String output
    std::string svg = perm.save_svg();
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_TRUE(svg.find("</svg>") != std::string::npos);

    // Must match internal ZDD's SVG output
    EXPECT_EQ(svg, perm.GetZDD().save_svg());
}

TEST_F(PiDDTest, SaveSvgStream) {
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    PiDD s12 = id.Swap(1, 2);

    std::ostringstream oss;
    s12.save_svg(oss);
    std::string svg = oss.str();
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_EQ(svg, s12.save_svg());
}

TEST_F(PiDDTest, SaveSvgWithParams) {
    PiDD_NewVar();
    PiDD_NewVar();

    PiDD id(1);
    SvgParams params;
    params.mode = DrawMode::Raw;
    params.draw_zero = false;

    std::string svg = id.save_svg(params);
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_EQ(svg, id.GetZDD().save_svg(params));
}

TEST_F(PiDDTest, SaveSvgTerminal) {
    // Terminal PiDD (empty set)
    PiDD empty(0);
    std::string svg = empty.save_svg();
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_EQ(svg, empty.GetZDD().save_svg());
}
