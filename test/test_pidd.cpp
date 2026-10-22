#include "pidd.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

using namespace kyotodd;

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

TEST_F(PiDDTest, SvgVarNameMap) {
    PiDD_NewVar();  // element 1
    PiDD_NewVar();  // element 2
    PiDD_NewVar();  // element 3

    std::map<bddvar, std::string> m = PiDD::svg_var_name_map();
    // S3 has transpositions: (2,1), (3,1), (3,2)
    // Check that the map contains transposition labels
    bool found_21 = false, found_31 = false, found_32 = false;
    for (const auto& kv : m) {
        if (kv.second == "(2,1)") found_21 = true;
        if (kv.second == "(3,1)") found_31 = true;
        if (kv.second == "(3,2)") found_32 = true;
    }
    EXPECT_TRUE(found_21);
    EXPECT_TRUE(found_31);
    EXPECT_TRUE(found_32);

    // Use in SVG rendering — labels should appear in output
    PiDD id(1);
    PiDD s = id.Swap(1, 2) + id;
    SvgParams params;
    params.var_name_map = m;
    std::string svg = s.save_svg(params);
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    // The transposition label should appear as node text
    EXPECT_TRUE(svg.find("(2,1)") != std::string::npos);
}

/* ---- Execution mode dispatch (_rec / _iter) ---- */

namespace {

const BddExecMode kPiddModes[] = {BDD_EXEC_RECURSIVE, BDD_EXEC_ITERATIVE, BDD_EXEC_AUTO};

// All permutations of {1..n}, built as a product of (1 + Swap(x, y)) factors.
PiDD pidd_all_perms(int n) {
    PiDD u(1);
    for (int x = 2; x <= n; ++x) {
        PiDD g = u;
        for (int y = 1; y < x; ++y) g += u.swap(x, y);
        u = g;
    }
    return u;
}

std::string pidd_enum_str(const PiDD& p, BddExecMode mode, bool form2) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    try {
        if (form2) p.enumerate2(mode); else p.enumerate(mode);
    } catch (...) {
        std::cout.rdbuf(old);
        throw;
    }
    std::cout.rdbuf(old);
    return oss.str();
}

// { Swap(x, y) } for every transposition level: a ZDD chain with one node
// per level, so every PiDD recursion walks all n(n-1)/2 levels.
PiDD pidd_transposition_chain() {
    bddp f = bddempty;
    bddvar n = bddvarused();
    for (bddvar v = 1; v <= n; ++v) f = ZDD::getnode_raw(v, f, bddsingle);
    return PiDD(ZDD_ID(f));
}

}  // namespace

TEST_F(PiDDTest, ExecModesAgree) {
    for (int i = 0; i < 4; ++i) PiDD_NewVar();
    PiDD all = pidd_all_perms(4);
    PiDD f = all.swap(3, 1) - PiDD(1).swap(2, 1) - PiDD(1);
    PiDD g = PiDD(1).swap(4, 2) + PiDD(1).swap(3, 2).swap(2, 1);
    ASSERT_EQ(all.card(), 24u);

    // Reference values via the operator / Auto forms.
    PiDD ref_swap = f.swap(4, 1);
    PiDD ref_cof = f.cofact(2, 3);
    PiDD ref_odd = f.odd();
    PiDD ref_even = f.even();
    PiDD ref_mul = f * g;
    PiDD ref_div = all / g;
    std::string ref_e1 = pidd_enum_str(f, BDD_EXEC_AUTO, false);
    std::string ref_e2 = pidd_enum_str(f, BDD_EXEC_AUTO, true);
    EXPECT_EQ(ref_odd + ref_even, f);

    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kPiddModes[i];
        bddgc();  // clear the operation cache so each mode really recomputes
        EXPECT_EQ(f.swap(4, 1, m), ref_swap);
        EXPECT_EQ(f.cofact(2, 3, m), ref_cof);
        EXPECT_EQ(f.odd(m), ref_odd);
        EXPECT_EQ(f.even(m), ref_even);
        EXPECT_EQ(f.multiply(g, m), ref_mul);
        EXPECT_EQ(f.multiply(g), ref_mul);
        EXPECT_EQ(all.divide(g, m), ref_div);
        EXPECT_EQ(all.divide(g), ref_div);
        EXPECT_EQ(pidd_enum_str(f, m, false), ref_e1);
        EXPECT_EQ(pidd_enum_str(f, m, true), ref_e2);
        // Error paths are mode independent.
        EXPECT_THROW(f.swap(0, 1, m), std::invalid_argument);
        EXPECT_THROW(f.cofact(1, 5, m), std::invalid_argument);
        EXPECT_THROW(f.divide(PiDD(0), m), std::invalid_argument);
        EXPECT_EQ(PiDD(-1).swap(1, 2, m), PiDD(-1));
        EXPECT_EQ(PiDD(-1).odd(m), PiDD(-1));
        EXPECT_EQ(f.multiply(PiDD(1), m), f);
        EXPECT_EQ(f.divide(PiDD(1), m), f);
    }
}

TEST_F(PiDDTest, ExecModesDeep) {
    const int n = 129;  // n(n-1)/2 = 8256 levels > BDD_RecurLimit
    for (int i = 0; i < n; ++i) PiDD_NewVar();
    ASSERT_GT(static_cast<int>(bddvarused()), BDD_RecurLimit);
    PiDD chain = pidd_transposition_chain();
    const double levels = static_cast<double>(bddvarused());
    ASSERT_EQ(chain.get_zdd().count(), levels);

    // swap: forced recursion must hit the limit, iterative must succeed.
    EXPECT_THROW(chain.swap(2, 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    PiDD s = chain.swap(2, 1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(s.get_zdd().count(), levels);
    EXPECT_EQ(s, chain.swap(2, 1));
    EXPECT_EQ(s.swap(2, 1, BDD_EXEC_ITERATIVE), chain);  // involution

    // cofact
    EXPECT_THROW(chain.cofact(1, 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    PiDD c = chain.cofact(1, 1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(c, chain.cofact(1, 1));
    // Swap(x, y) fixes position 1 iff y != 1: (n-1)(n-2)/2 such transpositions.
    EXPECT_EQ(c.get_zdd().count(), levels - static_cast<double>(n - 1));

    // odd / even: every transposition is odd.
    EXPECT_THROW(chain.odd(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(chain.odd(BDD_EXEC_ITERATIVE), chain);
    EXPECT_EQ(chain.even(BDD_EXEC_ITERATIVE), PiDD(0));
    EXPECT_EQ(chain.odd(), chain);

    // multiply: recursion runs over the right operand.
    PiDD t = PiDD(1).swap(2, 1);
    EXPECT_THROW(t.multiply(chain, BDD_EXEC_RECURSIVE), std::overflow_error);
    PiDD m = t.multiply(chain, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(m, t * chain);
    EXPECT_EQ(m.get_zdd().count(), levels);

    // divide: the cofactor step walks the whole chain.
    EXPECT_THROW(chain.divide(t, BDD_EXEC_RECURSIVE), std::overflow_error);
    PiDD d = chain.divide(t, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(d, chain / t);

    // enumerate / enumerate2
    EXPECT_THROW(pidd_enum_str(chain, BDD_EXEC_RECURSIVE, false), std::overflow_error);
    std::string e1 = pidd_enum_str(chain, BDD_EXEC_ITERATIVE, false);
    EXPECT_EQ(e1, pidd_enum_str(chain, BDD_EXEC_AUTO, false));
    EXPECT_THROW(pidd_enum_str(chain, BDD_EXEC_RECURSIVE, true), std::overflow_error);
    std::string e2 = pidd_enum_str(chain, BDD_EXEC_ITERATIVE, true);
    EXPECT_EQ(e2, pidd_enum_str(chain, BDD_EXEC_AUTO, true));
    // One entry per transposition, joined by " + ".
    size_t seps = 0;
    for (size_t pos = e2.find(" + "); pos != std::string::npos; pos = e2.find(" + ", pos + 3)) ++seps;
    EXPECT_EQ(seps + 1, static_cast<size_t>(levels));
    EXPECT_NE(e2.find("(129:1)"), std::string::npos);
    EXPECT_NE(e2.find("(2:1)"), std::string::npos);
}

// --- count / exact_count ------------------------------------------------

TEST_F(PiDDTest, CountAndExactCount) {
    for (int i = 0; i < 4; ++i) PiDD_NewVar();
    PiDD all = pidd_all_perms(4);
    EXPECT_EQ(all.card(), 24u);
    EXPECT_EQ(all.count(), 24.0);
    EXPECT_EQ(all.exact_count(), bigint::BigInt(24));
    PiDD f = all - PiDD(1);
    EXPECT_EQ(f.count(), 23.0);
    EXPECT_EQ(f.exact_count().to_string(), "23");
    EXPECT_EQ(PiDD(0).count(), 0.0);
    EXPECT_EQ(PiDD(0).exact_count(), bigint::BigInt(0));
    EXPECT_EQ(PiDD(1).count(), 1.0);
    EXPECT_EQ(PiDD(1).exact_count(), bigint::BigInt(1));
}

TEST_F(PiDDTest, CountAndExactCountDeep) {
    const int n = 129;  // n(n-1)/2 = 8256 levels > BDD_RecurLimit
    for (int i = 0; i < n; ++i) PiDD_NewVar();
    PiDD chain = pidd_transposition_chain();
    const double levels = static_cast<double>(bddvarused());
    // card() wraps the recursive bddcard and hits the depth limit, while
    // count() / exact_count() switch to the iterative counters.
    EXPECT_THROW(chain.card(), std::overflow_error);
    EXPECT_EQ(chain.count(), levels);
    EXPECT_EQ(chain.exact_count(), bigint::BigInt(static_cast<long long>(levels)));
}
