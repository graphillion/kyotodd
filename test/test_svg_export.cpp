#include <gtest/gtest.h>
#include "bdd.h"
#include "qdd.h"
#include "unreduced_dd.h"
#include "mtbdd.h"
#include "mvdd.h"
#include <sstream>
#include <fstream>
#include <cstdio>

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

    // Default: all levels allocated
    SvgParams params_default;
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
