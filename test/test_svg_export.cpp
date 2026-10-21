#include <gtest/gtest.h>
#include "bdd.h"
#include "qdd.h"
#include "unreduced_dd.h"
#include "mtbdd.h"
#include "mvdd.h"
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace kyotodd;

class SvgExportTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// --- SVG structure validation helpers ---

static bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

// --- Terminal BDD/ZDD ---

TEST_F(SvgExportTest, BDD_Terminal_True) {
    BDD t(1);
    std::string svg = t.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "<rect"));
    EXPECT_TRUE(contains(svg, ">1<"));
    EXPECT_TRUE(contains(svg, "</svg>"));
}

TEST_F(SvgExportTest, BDD_Terminal_False) {
    BDD f;
    std::string svg = f.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "<rect"));
    EXPECT_TRUE(contains(svg, ">0<"));
}

TEST_F(SvgExportTest, ZDD_Terminal_Empty) {
    ZDD z;
    std::string svg = z.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "<rect"));
}

// --- Single variable BDD ---

TEST_F(SvgExportTest, BDD_SingleVar) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    std::string svg = x1.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "<circle"));
    EXPECT_TRUE(contains(svg, "<defs>"));
    EXPECT_TRUE(contains(svg, "marker id=\"arrow\""));
    // Should have dashed line (0-edge) and solid line (1-edge)
    EXPECT_TRUE(contains(svg, "stroke-dasharray=\"10,5\""));
    EXPECT_TRUE(contains(svg, "</svg>"));
}

// --- Multi variable BDD ---

TEST_F(SvgExportTest, BDD_MultiVar) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    BDD f = (BDD::prime(v1) & BDD::prime(v2)) | BDD::prime(v3);
    std::string svg = f.save_svg();
    EXPECT_TRUE(contains(svg, "<circle"));
    // Should have var number labels
    EXPECT_TRUE(contains(svg, ">" + std::to_string(v1) + "<"));
    EXPECT_TRUE(contains(svg, ">" + std::to_string(v3) + "<"));
}

// --- ZDD ---

TEST_F(SvgExportTest, ZDD_Singleton) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD::singleton(v1);
    std::string svg = z.save_svg();
    EXPECT_TRUE(contains(svg, "<circle"));
    EXPECT_TRUE(contains(svg, "<rect"));
    EXPECT_TRUE(contains(svg, "</svg>"));
}

TEST_F(SvgExportTest, ZDD_PowerSet) {
    bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z = ZDD::power_set(v2);
    std::string svg = z.save_svg();
    EXPECT_TRUE(contains(svg, "<circle"));
}

// --- Complement edge modes ---

TEST_F(SvgExportTest, BDD_Expanded_NoComplement) {
    bddvar v1 = bddnewvar();
    BDD x1 = ~BDD::prime(v1);
    SvgParams params;
    params.mode = DrawMode::Expanded;
    std::string svg = x1.save_svg(params);
    EXPECT_TRUE(contains(svg, "<circle"));
    // Expanded mode should not have complement markers
    EXPECT_FALSE(contains(svg, "marker id=\"odot\""));
}

TEST_F(SvgExportTest, BDD_Raw_ComplementMarkers) {
    bddvar v1 = bddnewvar();
    BDD x1 = ~BDD::prime(v1);
    SvgParams params;
    params.mode = DrawMode::Raw;
    std::string svg = x1.save_svg(params);
    EXPECT_TRUE(contains(svg, "<circle"));
    // Raw mode should have complement markers
    EXPECT_TRUE(contains(svg, "marker id=\"odot\""));
}

TEST_F(SvgExportTest, ZDD_Raw_Mode) {
    bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z = ZDD::power_set(v2);
    SvgParams params;
    params.mode = DrawMode::Raw;
    std::string svg = z.save_svg(params);
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "</svg>"));
}

// --- draw_zero toggle ---

TEST_F(SvgExportTest, BDD_DrawZero_True) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    SvgParams params;
    params.draw_zero = true;
    std::string svg = x1.save_svg(params);
    // Should have two rect elements (0 and 1 terminal)
    size_t first = svg.find("<rect");
    ASSERT_NE(first, std::string::npos);
    size_t second = svg.find("<rect", first + 1);
    EXPECT_NE(second, std::string::npos);
}

TEST_F(SvgExportTest, BDD_DrawZero_False) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    SvgParams params;
    params.draw_zero = false;
    std::string svg = x1.save_svg(params);
    // Should have only one rect element (1 terminal)
    size_t first = svg.find("<rect");
    ASSERT_NE(first, std::string::npos);
    size_t second = svg.find("<rect", first + 1);
    EXPECT_EQ(second, std::string::npos);
}

// --- var_name_map ---

TEST_F(SvgExportTest, BDD_VarNameMap) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    SvgParams params;
    params.var_name_map[v1] = "A";
    std::string svg = x1.save_svg(params);
    EXPECT_TRUE(contains(svg, ">A<"));
}

// --- Output targets ---

TEST_F(SvgExportTest, BDD_OstreamOutput) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    std::ostringstream oss;
    x1.save_svg(oss);
    std::string svg_stream = oss.str();
    std::string svg_string = x1.save_svg();
    EXPECT_EQ(svg_stream, svg_string);
}

TEST_F(SvgExportTest, BDD_FileOutput) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    const char* tmpfile = "/tmp/test_kyotodd_svg.svg";
    x1.save_svg(tmpfile);
    std::ifstream ifs(tmpfile);
    std::string file_content((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());
    std::string svg_string = x1.save_svg();
    EXPECT_EQ(file_content, svg_string);
    std::remove(tmpfile);
}

// --- QDD ---

TEST_F(SvgExportTest, QDD_SaveSvg) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD f = BDD::prime(v1) & BDD::prime(v2);
    QDD q = f.to_qdd();
    std::string svg = q.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "<circle"));
    EXPECT_TRUE(contains(svg, "</svg>"));
}

// --- UnreducedDD ---

TEST_F(SvgExportTest, UnreducedDD_SaveSvg) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    UnreducedDD n0 = UnreducedDD(0);
    UnreducedDD n1 = UnreducedDD(1);
    UnreducedDD node2 = UnreducedDD::getnode(v2, n0, n1);
    UnreducedDD node1 = UnreducedDD::getnode(v1, node2, n1);
    std::string svg = node1.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "<circle"));
    EXPECT_TRUE(contains(svg, "</svg>"));
}

// --- SvgParams customization ---

TEST_F(SvgExportTest, SvgParams_CustomColors) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    SvgParams params;
    params.node_fill = "#ff0000";
    params.node_stroke = "#00ff00";
    std::string svg = x1.save_svg(params);
    EXPECT_TRUE(contains(svg, "#ff0000"));
    EXPECT_TRUE(contains(svg, "#00ff00"));
}

TEST_F(SvgExportTest, SvgParams_CustomRadius) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    SvgParams params;
    params.node_radius = 30;
    std::string svg = x1.save_svg(params);
    EXPECT_TRUE(contains(svg, "r=\"30\""));
}

// --- Free functions ---

TEST_F(SvgExportTest, FreeFn_bdd_save_svg) {
    bddvar v1 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    std::string svg = bdd_save_svg(x1.id());
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
}

TEST_F(SvgExportTest, FreeFn_zdd_save_svg) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD::singleton(v1);
    std::string svg = zdd_save_svg(z.id());
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
}

TEST_F(SvgExportTest, SkipUnusedLevels) {
    // Create vars 1-5, then build a BDD that uses only vars 1 and 5.
    // Levels 2,3,4 are unused. With skip_unused_levels=true,
    // the SVG should be shorter (fewer vertical levels).
    bddvar v1 = bddnewvar();
    bddnewvar();  // v2 (unused)
    bddnewvar();  // v3 (unused)
    bddnewvar();  // v4 (unused)
    bddvar v5 = bddnewvar();
    BDD f = BDD::prime(v1) & BDD::prime(v5);

    // All levels allocated (skip_unused_levels defaults to true since
    // review-SVG-20261021 item 8, so ask for the full layout explicitly)
    SvgParams params_default;
    params_default.skip_unused_levels = false;
    std::string svg_default = f.save_svg(params_default);

    // With skip: unused levels omitted
    SvgParams params_skip;
    params_skip.skip_unused_levels = true;
    std::string svg_skip = f.save_svg(params_skip);

    EXPECT_TRUE(contains(svg_default, "<svg"));
    EXPECT_TRUE(contains(svg_skip, "<svg"));

    // Extract height from viewBox: viewBox="x y w h"
    // The skipped version should have a smaller height
    auto extract_height = [](const std::string& svg) -> int {
        size_t pos = svg.find("viewBox=\"");
        if (pos == std::string::npos) return -1;
        pos += 9;
        // skip x, y, w
        int count = 0;
        while (count < 3 && pos < svg.size()) {
            if (svg[pos] == ' ') count++;
            pos++;
        }
        return std::atoi(svg.c_str() + pos);
    };

    int h_default = extract_height(svg_default);
    int h_skip = extract_height(svg_skip);
    EXPECT_GT(h_default, h_skip);
}

// --- MTBDD SVG ---

TEST_F(SvgExportTest, MTBDD_Terminal) {
    auto mt = MTBDD<double>::terminal(3.14);
    std::string svg = mt.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "3.14"));
    EXPECT_TRUE(contains(svg, "<rect"));
}

TEST_F(SvgExportTest, MTBDD_MultiTerminal) {
    bddvar v1 = bddnewvar();
    auto hi = MTBDD<double>::terminal(3.14);
    auto lo = MTBDD<double>::terminal(2.71);
    auto mt = MTBDD<double>::ite(v1, hi, lo);
    std::string svg = mt.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "<circle"));
    EXPECT_TRUE(contains(svg, "3.14"));
    EXPECT_TRUE(contains(svg, "2.71"));
}

TEST_F(SvgExportTest, MTBDD_ThreeTerminals) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    auto t0 = MTBDD<double>::terminal(0.0);
    auto t1 = MTBDD<double>::terminal(1.5);
    auto t2 = MTBDD<double>::terminal(9.9);
    auto inner = MTBDD<double>::ite(v1, t2, t1);
    auto mt = MTBDD<double>::ite(v2, inner, t0);
    std::string svg = mt.save_svg();
    EXPECT_TRUE(contains(svg, "1.5"));
    EXPECT_TRUE(contains(svg, "9.9"));
    EXPECT_TRUE(contains(svg, "0"));
}

TEST_F(SvgExportTest, MTBDD_DrawZeroFalse) {
    bddvar v1 = bddnewvar();
    auto hi = MTBDD<double>::terminal(5.0);
    auto lo = MTBDD<double>::terminal(0.0);
    auto mt = MTBDD<double>::ite(v1, hi, lo);
    SvgParams params;
    params.draw_zero = false;
    std::string svg = mt.save_svg(params);
    EXPECT_TRUE(contains(svg, "5"));
    // Only one terminal rect should be drawn
    size_t first = svg.find("<rect");
    ASSERT_NE(first, std::string::npos);
    size_t second = svg.find("<rect", first + 1);
    EXPECT_EQ(second, std::string::npos);
}

TEST_F(SvgExportTest, MTBDD_StreamOutput) {
    bddvar v1 = bddnewvar();
    auto hi = MTBDD<double>::terminal(1.0);
    auto lo = MTBDD<double>::terminal(2.0);
    auto mt = MTBDD<double>::ite(v1, hi, lo);
    std::ostringstream oss;
    mt.save_svg(oss);
    EXPECT_EQ(oss.str(), mt.save_svg());
}

TEST_F(SvgExportTest, MTBDD_Int) {
    bddvar v1 = bddnewvar();
    auto hi = MTBDD<int64_t>::terminal(42);
    auto lo = MTBDD<int64_t>::terminal(-7);
    auto mt = MTBDD<int64_t>::ite(v1, hi, lo);
    std::string svg = mt.save_svg();
    EXPECT_TRUE(contains(svg, "42"));
    EXPECT_TRUE(contains(svg, "-7"));
}

// --- MTZDD SVG ---

TEST_F(SvgExportTest, MTZDD_Basic) {
    bddvar v1 = bddnewvar();
    auto hi = MTZDD<double>::terminal(3.14);
    auto lo = MTZDD<double>::terminal(1.0);
    auto mt = MTZDD<double>::ite(v1, hi, lo);
    std::string svg = mt.save_svg();
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
    EXPECT_TRUE(contains(svg, "<circle"));
    EXPECT_TRUE(contains(svg, "3.14"));
}

TEST_F(SvgExportTest, MTZDD_Terminal) {
    auto mt = MTZDD<int64_t>::terminal(99);
    std::string svg = mt.save_svg();
    EXPECT_TRUE(contains(svg, "99"));
    EXPECT_TRUE(contains(svg, "<rect"));
}

// --- XML escape in text labels ---

TEST_F(SvgExportTest, XmlEscape_VarNameMap) {
    bddvar v = bddnewvar();
    BDD x = BDD::prime(v);
    SvgParams params;
    params.var_name_map[v] = "A<&B>";
    std::string svg = x.save_svg(params);
    // Must not contain raw < > & inside text
    EXPECT_TRUE(contains(svg, "A&lt;&amp;B&gt;"));
    EXPECT_FALSE(contains(svg, ">A<&B><"));
}

TEST_F(SvgExportTest, XmlEscape_TerminalNameMap) {
    bddvar v = bddnewvar();
    auto hi = MTBDD<double>::terminal(1.0);
    auto lo = MTBDD<double>::terminal(0.0);
    auto mt = MTBDD<double>::ite(v, hi, lo);
    SvgParams params;
    params.terminal_name_map[bddtrue] = "T\"1\"";
    std::string svg = mt.save_svg(params);
    EXPECT_TRUE(contains(svg, "T&quot;1&quot;"));
}

// --- Duplicate arrow marker ---

TEST_F(SvgExportTest, MTBDD_NoDuplicateArrowMarker) {
    bddvar v = bddnewvar();
    auto hi = MTBDD<double>::terminal(1.0);
    auto lo = MTBDD<double>::terminal(0.0);
    auto mt = MTBDD<double>::ite(v, hi, lo);
    std::string svg = mt.save_svg();
    // Count occurrences of id="arrow"
    size_t count = 0;
    size_t pos = 0;
    while ((pos = svg.find("id=\"arrow\"", pos)) != std::string::npos) {
        ++count;
        pos += 10;
    }
    EXPECT_EQ(count, 1u);
}

TEST_F(SvgExportTest, MVBDD_NoDuplicateArrowMarker) {
    MVBDD base(3);
    bddvar mv_v = base.new_var();
    auto lit = MVBDD::singleton(base, mv_v, 1);
    std::string svg = lit.save_svg();
    size_t count = 0;
    size_t pos = 0;
    while ((pos = svg.find("id=\"arrow\"", pos)) != std::string::npos) {
        ++count;
        pos += 10;
    }
    EXPECT_EQ(count, 1u);
}

// --- nullptr filename ---

TEST_F(SvgExportTest, NullFilename_BDD) {
    BDD t(1);
    EXPECT_THROW(t.save_svg((const char*)nullptr), std::runtime_error);
}

TEST_F(SvgExportTest, NullFilename_ZDD) {
    ZDD t(1);
    EXPECT_THROW(t.save_svg((const char*)nullptr), std::runtime_error);
}

TEST_F(SvgExportTest, NullFilename_MTBDD) {
    auto mt = MTBDD<double>::terminal(1.0);
    EXPECT_THROW(mt.save_svg((const char*)nullptr), std::runtime_error);
}

// --- nullptr var_table for MVBDD/MVZDD ---

TEST_F(SvgExportTest, MVBDD_NullTable_Throws) {
    bddvar v = bddnewvar();
    BDD b = BDD::prime(v);
    MVBDD mv(std::shared_ptr<MVDDVarTable>(), b);
    // Expanded mode with null table should throw, not segfault
    EXPECT_THROW(mv.save_svg(), std::runtime_error);
}

TEST_F(SvgExportTest, MVZDD_NullTable_Throws) {
    bddvar v = bddnewvar();
    ZDD z = ZDD::singleton(v);
    MVZDD mv(std::shared_ptr<MVDDVarTable>(), z);
    EXPECT_THROW(mv.save_svg(), std::runtime_error);
}

// --- Horizontal node spacing: node_interval_x is the minimum gap ---

// Collect (cx, cy, r) of every <circle> in the SVG body (markers in <defs>
// are excluded because they have no cx/cy attributes with large values).
static std::vector<std::vector<double>> svg_circles(const std::string& svg) {
    std::vector<std::vector<double>> out;
    size_t body = svg.find("</defs>");
    size_t pos = (body == std::string::npos) ? 0 : body;
    while ((pos = svg.find("<circle cx=\"", pos)) != std::string::npos) {
        double cx, cy; int r;
        if (std::sscanf(svg.c_str() + pos, "<circle cx=\"%lf\" cy=\"%lf\" r=\"%d\"", &cx, &cy, &r) == 3) {
            out.push_back({cx, cy, static_cast<double>(r)});
        }
        pos += 8;
    }
    return out;
}

// Every pair of circles on the same level must be at least
// 2 * node_radius + node_interval_x apart (center to center).
static void expect_min_gap(const std::string& svg, int node_radius, int node_interval_x) {
    auto cs = svg_circles(svg);
    ASSERT_GE(cs.size(), 2u);
    bool same_level_pair = false;
    for (size_t i = 0; i < cs.size(); ++i) {
        for (size_t j = i + 1; j < cs.size(); ++j) {
            if (cs[i][1] != cs[j][1]) continue;
            same_level_pair = true;
            double dx = std::fabs(cs[i][0] - cs[j][0]);
            EXPECT_GE(dx, 2 * node_radius + node_interval_x)
                << "circles " << i << " and " << j << " on y=" << cs[i][1];
        }
    }
    EXPECT_TRUE(same_level_pair);
}

TEST_F(SvgExportTest, ZDD_NodeSpacing_LargeRadiusSmallInterval) {
    std::vector<bddvar> vs = bddnewvar(4);
    ZDD z = ZDD::combination(4, 2);   // several nodes per level
    SvgParams p;
    p.node_radius = 30;
    p.node_interval_x = 5;            // old formula produced overlapping circles here
    expect_min_gap(z.save_svg(p), p.node_radius, p.node_interval_x);
}

TEST_F(SvgExportTest, BDD_NodeSpacing_DefaultParams) {
    std::vector<bddvar> vs = bddnewvar(3);
    BDD f = BDD::prime(vs[0]) ^ BDD::prime(vs[1]) ^ BDD::prime(vs[2]);
    SvgParams p;
    expect_min_gap(f.save_svg(p), p.node_radius, p.node_interval_x);
}

TEST_F(SvgExportTest, MTBDD_NodeSpacing_LargeRadiusSmallInterval) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    auto t0 = MTBDD<double>::terminal(0.0);
    auto t1 = MTBDD<double>::terminal(1.5);
    auto a = MTBDD<double>::ite(v1, t1, t0);
    auto b = MTBDD<double>::ite(v1, t0, t1);
    auto mt = MTBDD<double>::ite(v2, a, b);   // two nodes on the v1 level, two terminals
    SvgParams p;
    p.node_radius = 30;
    p.node_interval_x = 5;
    expect_min_gap(mt.save_svg(p), p.node_radius, p.node_interval_x);
}

TEST_F(SvgExportTest, MVBDD_NodeSpacing_LargeRadiusSmallInterval) {
    MVBDD base(2);   // k = 2: no extra horizontal scaling
    bddvar a = base.new_var();
    bddvar b = base.new_var();
    // Two distinct nodes on the level of b: value-dependent functions of b
    auto f = (MVBDD::singleton(base, a, 0) & MVBDD::singleton(base, b, 1)) |
             (MVBDD::singleton(base, a, 1) & MVBDD::singleton(base, b, 0));
    SvgParams p;
    p.node_radius = 30;
    p.node_interval_x = 5;
    expect_min_gap(f.save_svg(p), p.node_radius, p.node_interval_x);
}

// --- width/height must equal the viewBox size exactly ---

static void expect_size_matches_viewbox(const std::string& svg) {
    double w, h, vx, vy, vw, vh;
    size_t pos = svg.find("<svg ");
    ASSERT_NE(pos, std::string::npos);
    ASSERT_EQ(std::sscanf(svg.c_str() + pos,
                          "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%lf\" height=\"%lf\" "
                          "viewBox=\"%lf %lf %lf %lf\"", &w, &h, &vx, &vy, &vw, &vh), 6);
    EXPECT_DOUBLE_EQ(w, vw);
    EXPECT_DOUBLE_EQ(h, vh);
}

TEST_F(SvgExportTest, SizeMatchesViewBox_FractionalBounds) {
    std::vector<bddvar> vs = bddnewvar(3);
    BDD f = BDD::prime(vs[0]) ^ BDD::prime(vs[1]) ^ BDD::prime(vs[2]);
    SvgParams p;
    p.arc_width = 3;   // odd stroke width gives half-unit bounds (x.5)
    std::string svg = f.save_svg(p);
    EXPECT_NE(svg.find(".5 "), std::string::npos);   // the viewBox really is fractional
    expect_size_matches_viewbox(svg);
    expect_size_matches_viewbox(ZDD::combination(4, 2).save_svg(p));
    expect_size_matches_viewbox(BDD(1).save_svg(p));
}

// --- Markers follow node_stroke (review-SVG-20261021 item 11) ---

static size_t count_substr(const std::string& s, const std::string& sub) {
    size_t n = 0, pos = 0;
    while ((pos = s.find(sub, pos)) != std::string::npos) { ++n; pos += sub.size(); }
    return n;
}

TEST_F(SvgExportTest, Markers_FollowNodeStroke) {
    std::vector<bddvar> vs = bddnewvar(3);
    ZDD z = ZDD::combination(3, 2);
    SvgParams p;
    p.node_stroke = "#c00000";
    p.node_fill = "#ffe0e0";
    std::string svg = z.save_svg(p);
    EXPECT_EQ(count_substr(svg, "#1b3966"), 0u);
    // arrowhead polygon is filled with the stroke colour
    EXPECT_TRUE(contains(svg, "fill=\"#c00000\" stroke=\"none\""));
    p.mode = DrawMode::Raw;
    std::string raw = z.save_svg(p);
    EXPECT_EQ(count_substr(raw, "#1b3966"), 0u);
    EXPECT_TRUE(contains(raw, "marker id=\"odot\""));
    EXPECT_TRUE(contains(raw, "fill=\"white\" stroke=\"#c00000\""));
    // terminal-only drawings and the other exporters use the same writer
    EXPECT_EQ(count_substr(ZDD(1).save_svg(p), "#1b3966"), 0u);
    auto mt = MTBDD<double>::ite(vs[0], MTBDD<double>::terminal(1.0), MTBDD<double>::terminal(2.0));
    EXPECT_EQ(count_substr(mt.save_svg(p), "#1b3966"), 0u);
}

TEST_F(SvgExportTest, Markers_StrokeColorIsEscaped) {
    bddvar v = bddnewvar();
    ZDD z = ZDD::singleton(v);
    SvgParams p;
    p.node_stroke = "\"><script>";
    std::string svg = z.save_svg(p);
    EXPECT_FALSE(contains(svg, "<script>"));
}

// --- font-family, label centring, marker size, marker bounds, text width ---
// (review-SVG-20261021 items 12, 13, 15, 16, 17)

TEST_F(SvgExportTest, Text_HasFontFamily) {
    bddvar v = bddnewvar();
    ZDD z = ZDD::singleton(v);
    SvgParams p;
    EXPECT_TRUE(contains(z.save_svg(p), "font-family=\"sans-serif\""));
    p.font_family = "serif";
    EXPECT_TRUE(contains(z.save_svg(p), "font-family=\"serif\""));
    p.font_family = "";
    EXPECT_FALSE(contains(z.save_svg(p), "font-family"));
    p.font_family = "a\"b";
    EXPECT_TRUE(contains(z.save_svg(p), "font-family=\"a&quot;b\""));
    EXPECT_TRUE(contains(ZDD(1).save_svg(SvgParams()), "font-family=\"sans-serif\""));
}

// y of the first <text> minus cy of the first <circle>
static int label_baseline_offset(const std::string& svg) {
    double cx, cy, tx, ty;
    size_t c = svg.find("<circle cx=\"");
    size_t t = svg.find("<text x=\"");
    EXPECT_NE(c, std::string::npos);
    EXPECT_NE(t, std::string::npos);
    std::sscanf(svg.c_str() + c, "<circle cx=\"%lf\" cy=\"%lf\"", &cx, &cy);
    std::sscanf(svg.c_str() + t, "<text x=\"%lf\" y=\"%lf\"", &tx, &ty);
    return static_cast<int>(ty - cy);
}

TEST_F(SvgExportTest, Label_CentredForAnyFontSize) {
    bddvar v = bddnewvar();
    ZDD z = ZDD::singleton(v);
    SvgParams p;
    EXPECT_EQ(label_baseline_offset(z.save_svg(p)), 7);      // 0.3 * 24
    p.font_size = 12;
    EXPECT_EQ(label_baseline_offset(z.save_svg(p)), 4);      // 0.3 * 12 = 3.6
    p.font_size = 40;
    EXPECT_EQ(label_baseline_offset(z.save_svg(p)), 12);
    p.label_y = 2;                                            // explicit value wins
    EXPECT_EQ(label_baseline_offset(z.save_svg(p)), 2);
    p.label_y = 0;
    EXPECT_EQ(label_baseline_offset(z.save_svg(p)), 0);
}

TEST_F(SvgExportTest, RawMarkers_SizedFromNodeRadius) {
    bddvar v = bddnewvar();
    ZDD z = ZDD::singleton(v);
    SvgParams p;
    p.mode = DrawMode::Raw;
    std::string svg = z.save_svg(p);
    EXPECT_TRUE(contains(svg, "markerUnits=\"userSpaceOnUse\""));
    EXPECT_TRUE(contains(svg, "r=\"4\" fill="));          // node_radius 20 -> dot radius 4
    p.node_radius = 40;
    EXPECT_TRUE(contains(z.save_svg(p), "r=\"8\" fill="));
    p.node_radius = 40;
    p.arc_width = 9;                                       // stroke width no longer scales the dots
    EXPECT_TRUE(contains(z.save_svg(p), "r=\"8\" fill="));
}

TEST_F(SvgExportTest, RawRootMarker_InsideMargin) {
    std::vector<bddvar> vs = bddnewvar(2);
    ZDD z = ~ZDD::singleton(vs[0]) + ZDD::singleton(vs[1]);   // complemented root
    SvgParams p;
    p.mode = DrawMode::Raw;
    std::string svg = z.save_svg(p);
    double vx, vy, vw, vh;
    size_t pos = svg.find("viewBox=\"");
    ASSERT_NE(pos, std::string::npos);
    std::sscanf(svg.c_str() + pos, "viewBox=\"%lf %lf %lf %lf\"", &vx, &vy, &vw, &vh);
    // the entry stub starts 20 px above the root circle; its odot marker
    // (radius node_radius/5 = 4, plus outline) must lie inside the margin
    double root_cy = 1e9;
    size_t body = svg.find("</defs>");                     // skip the marker circles
    size_t c = svg.find("<circle cx=\"", body == std::string::npos ? 0 : body);
    while (c != std::string::npos) {
        double cx, cy;
        std::sscanf(svg.c_str() + c, "<circle cx=\"%lf\" cy=\"%lf\"", &cx, &cy);
        if (cy < root_cy) root_cy = cy;
        c = svg.find("<circle cx=\"", c + 8);
    }
    double stub_top = root_cy - p.node_radius - 20;
    EXPECT_LE(vy + p.margin_y, stub_top - 5.0 + 0.01);
}

#include "../src/svg_export_internal.h"

TEST_F(SvgExportTest, TextWidthEstimate_CountsCodePoints) {
    using kyotodd::svg_internal::estimate_text_width;
    double w = estimate_text_width("(2,1)", 24);
    EXPECT_GE(w, 40.0);                                   // real width ~55 px
    EXPECT_LE(w, 62.0);
    EXPECT_NEAR(estimate_text_width("100", 24), 43.2, 0.1);
    // two CJK characters: about two em, not six bytes * 0.6 em
    std::string cjk = "\xE6\x97\xA5\xE6\x9C\xAC";
    EXPECT_NEAR(estimate_text_width(cjk, 24), 48.0, 0.1);
    EXPECT_LT(estimate_text_width("ill", 24), estimate_text_width("WWW", 24));
}

// --- Empty levels are skipped by default (review-SVG-20261021 item 8) ---

TEST_F(SvgExportTest, SkipUnusedLevels_IsDefault) {
    SvgParams p;
    EXPECT_TRUE(p.skip_unused_levels);
    // ZDD::singleton(5) with vars 1..5: one node row directly above the
    // terminals, no empty rows in between.
    std::vector<bddvar> vs = bddnewvar(5);
    std::string svg = ZDD::singleton(vs[4]).save_svg();
    double cx, cy, rx, ry;
    size_t c = svg.find("<circle cx=\"", svg.find("</defs>"));
    size_t r = svg.find("<rect x=\"");
    ASSERT_NE(c, std::string::npos);
    ASSERT_NE(r, std::string::npos);
    std::sscanf(svg.c_str() + c, "<circle cx=\"%lf\" cy=\"%lf\"", &cx, &cy);
    std::sscanf(svg.c_str() + r, "<rect x=\"%lf\" y=\"%lf\"", &rx, &ry);
    // circle bottom (cy + r) to terminal top: exactly one node_interval_y gap
    EXPECT_DOUBLE_EQ(ry - (cy + p.node_radius), p.node_interval_y);
}

// --- Labels that do not fit are drawn smaller (review-SVG-20261021 item 9) ---

// font-size and y of the <text> element whose content is `label` (first match)
static bool text_font_size(const std::string& svg, const std::string& label, int& fs, double& y) {
    size_t pos = 0;
    while ((pos = svg.find("<text x=\"", pos)) != std::string::npos) {
        size_t end = svg.find("</text>", pos);
        std::string elem = svg.substr(pos, end - pos);
        if (elem.size() >= label.size() + 1 && elem.compare(elem.size() - label.size(), label.size(), label) == 0 &&
            elem[elem.size() - label.size() - 1] == '>') {
            double x;
            std::sscanf(elem.c_str(), "<text x=\"%lf\" y=\"%lf\" font-size=\"%d\"", &x, &y, &fs);
            return true;
        }
        pos = end;
    }
    return false;
}

TEST_F(SvgExportTest, Labels_ShrinkToFitNode) {
    std::vector<bddvar> vs = bddnewvar(3);
    ZDD z = ZDD::combination(3, 2);
    SvgParams p;
    p.var_name_map[vs[2]] = "color";    // top node
    p.var_name_map[vs[0]] = "x";
    std::string svg = z.save_svg(p);
    int fs; double y;
    ASSERT_TRUE(text_font_size(svg, "color", fs, y));
    EXPECT_LT(fs, 24);
    EXPECT_GE(fs, 6);
    double room = 2.0 * p.node_radius - p.arc_width - 4.0;
    EXPECT_LE(kyotodd::svg_internal::estimate_text_width("color", fs), room);
    EXPECT_EQ(static_cast<int>(y) % 1000 >= 0, true);
    // short labels keep the full size; the baseline follows the smaller font
    int fs2; double y2;
    ASSERT_TRUE(text_font_size(svg, "x", fs2, y2));
    EXPECT_EQ(fs2, 24);
    ASSERT_TRUE(text_font_size(svg, "2", fs2, y2));
    EXPECT_EQ(fs2, 24);
    ASSERT_TRUE(text_font_size(svg, "0", fs2, y2));   // terminal labels untouched
    EXPECT_EQ(fs2, 24);
}

TEST_F(SvgExportTest, Labels_ThreeDigitVariablesShrink) {
    std::vector<bddvar> vs = bddnewvar(120);
    std::vector<std::vector<bddvar> > sets;
    sets.push_back({100, 110, 120});
    sets.push_back({100, 120});
    std::string svg = ZDD::from_sets(sets).save_svg();
    int fs; double y;
    ASSERT_TRUE(text_font_size(svg, "120", fs, y));
    EXPECT_EQ(fs, 18);                                  // 24 * 33 / 43.2
    SvgParams p;
    p.node_radius = 30;                                 // enough room: full size again
    ASSERT_TRUE(text_font_size(ZDD::from_sets(sets).save_svg(p), "120", fs, y));
    EXPECT_EQ(fs, 24);
}
