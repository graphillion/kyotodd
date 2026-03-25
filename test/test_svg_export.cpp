#include <gtest/gtest.h>
#include "bdd.h"
#include "qdd.h"
#include "unreduced_dd.h"
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
    std::string svg = bdd_save_svg(x1.get_id());
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
}

TEST_F(SvgExportTest, FreeFn_zdd_save_svg) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD::singleton(v1);
    std::string svg = zdd_save_svg(z.get_id());
    EXPECT_TRUE(contains(svg, "<svg xmlns="));
}
