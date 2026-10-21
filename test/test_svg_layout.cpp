// Geometric tests for the SVG layout engine (src/svg_export_internal.h).
//
// The string-based tests in test_svg_export.cpp check that elements exist;
// the tests here parse the emitted <circle>/<rect>/<text>/<path> elements
// and verify the geometry: arcs must not run through nodes that are not
// their endpoints, paths must not contain zero-length commands, and so on.
#include <gtest/gtest.h>
#include "bdd.h"
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

using namespace kyotodd;

namespace {

struct Pt { double x, y; };
struct Circle { double cx, cy, r, sw; };
struct Rect { double x, y, w, h, sw; };
struct Text { double x, y, fs; std::string s; };
struct Seg { char type; Pt p0, c, p1; };   // 'L': p0->p1, 'Q': p0, control c, p1
struct Path {
    std::vector<Seg> segs;
    Pt start, end;
    bool dashed;
    std::string mstart, mend;
    double sw;
};
struct Svg {
    double vx, vy, vw, vh;
    std::vector<Circle> circles;
    std::vector<Rect> rects;
    std::vector<Text> texts;
    std::vector<Path> paths;
};

double dist(Pt a, Pt b) { return std::hypot(a.x - b.x, a.y - b.y); }

// Attribute value of an element string ("<circle cx=\"1\" .../>"), or "".
std::string attr(const std::string& elem, const std::string& name) {
    std::string key = " " + name + "=\"";
    size_t p = elem.find(key);
    if (p == std::string::npos) return "";
    p += key.size();
    size_t q = elem.find('"', p);
    return elem.substr(p, q - p);
}
double num(const std::string& elem, const std::string& name, double dflt = 0.0) {
    std::string v = attr(elem, name);
    return v.empty() ? dflt : std::atof(v.c_str());
}

std::vector<Seg> parse_d(const std::string& d) {
    std::vector<Seg> segs;
    std::vector<std::string> tok;
    std::string cur;
    for (size_t i = 0; i <= d.size(); ++i) {
        char ch = (i < d.size()) ? d[i] : ' ';
        if (ch == ' ' || ch == ',') { if (!cur.empty()) { tok.push_back(cur); cur.clear(); } }
        else cur += ch;
    }
    Pt pos = {0, 0};
    size_t i = 0;
    auto rd = [&](size_t k) { return std::atof(tok[k].c_str()); };
    while (i < tok.size()) {
        const std::string& c = tok[i++];
        if (c == "M") { pos.x = rd(i); pos.y = rd(i + 1); i += 2; }
        else if (c == "L") { Seg s; s.type = 'L'; s.p0 = pos; s.p1.x = rd(i); s.p1.y = rd(i + 1); s.c = s.p0; i += 2; segs.push_back(s); pos = s.p1; }
        else if (c == "Q") { Seg s; s.type = 'Q'; s.p0 = pos; s.c.x = rd(i); s.c.y = rd(i + 1); s.p1.x = rd(i + 2); s.p1.y = rd(i + 3); i += 4; segs.push_back(s); pos = s.p1; }
        else { ADD_FAILURE() << "unexpected path token " << c; break; }
    }
    return segs;
}

Svg parse_svg(const std::string& svg) {
    Svg out;
    size_t p = svg.find("viewBox=\"");
    EXPECT_NE(p, std::string::npos);
    std::sscanf(svg.c_str() + p, "viewBox=\"%lf %lf %lf %lf\"", &out.vx, &out.vy, &out.vw, &out.vh);
    size_t body = svg.find("</defs>");
    size_t pos = (body == std::string::npos) ? 0 : body;
    while (true) {
        size_t lt = svg.find('<', pos);
        if (lt == std::string::npos) break;
        size_t gt = svg.find('>', lt);
        if (gt == std::string::npos) break;
        std::string elem = svg.substr(lt, gt - lt + 1);
        pos = gt + 1;
        if (elem.compare(0, 8, "<circle ") == 0) {
            Circle c = {num(elem, "cx"), num(elem, "cy"), num(elem, "r"), num(elem, "stroke-width", 1)};
            out.circles.push_back(c);
        } else if (elem.compare(0, 6, "<rect ") == 0) {
            Rect r = {num(elem, "x"), num(elem, "y"), num(elem, "width"), num(elem, "height"), num(elem, "stroke-width", 1)};
            out.rects.push_back(r);
        } else if (elem.compare(0, 6, "<text ") == 0) {
            size_t close = svg.find("</text>", pos);
            Text t = {num(elem, "x"), num(elem, "y"), num(elem, "font-size", 16), svg.substr(pos, close - pos)};
            out.texts.push_back(t);
            pos = close + 7;
        } else if (elem.compare(0, 6, "<path ") == 0) {
            Path pa;
            pa.segs = parse_d(attr(elem, "d"));
            if (pa.segs.empty()) continue;
            pa.start = pa.segs.front().p0;
            pa.end = pa.segs.back().p1;
            pa.dashed = !attr(elem, "stroke-dasharray").empty();
            pa.mstart = attr(elem, "marker-start");
            pa.mend = attr(elem, "marker-end");
            pa.sw = num(elem, "stroke-width", 1);
            out.paths.push_back(pa);
        }
    }
    return out;
}

// Points along the path at ~1 px spacing.
std::vector<Pt> sample(const Path& p) {
    std::vector<Pt> pts;
    for (size_t i = 0; i < p.segs.size(); ++i) {
        const Seg& s = p.segs[i];
        if (s.type == 'L') {
            int n = std::max(1, static_cast<int>(dist(s.p0, s.p1)));
            for (int k = 0; k <= n; ++k) {
                double t = static_cast<double>(k) / n;
                Pt q = {s.p0.x + (s.p1.x - s.p0.x) * t, s.p0.y + (s.p1.y - s.p0.y) * t};
                pts.push_back(q);
            }
        } else {
            int n = std::max(2, static_cast<int>(dist(s.p0, s.c) + dist(s.c, s.p1)));
            for (int k = 0; k <= n; ++k) {
                double t = static_cast<double>(k) / n, u = 1.0 - t;
                Pt q = {u * u * s.p0.x + 2 * u * t * s.c.x + t * t * s.p1.x,
                        u * u * s.p0.y + 2 * u * t * s.c.y + t * t * s.p1.y};
                pts.push_back(q);
            }
        }
    }
    return pts;
}

// Index of the circle whose boundary passes through q (within tol), or -1.
int circle_at(const Svg& s, Pt q, double tol = 2.5) {
    for (size_t i = 0; i < s.circles.size(); ++i) {
        Pt c = {s.circles[i].cx, s.circles[i].cy};
        if (std::fabs(dist(q, c) - s.circles[i].r) <= tol) return static_cast<int>(i);
    }
    return -1;
}

// Smallest ink-to-ink clearance between any arc and any circle that is not
// the arc's own start or end node (negative = the strokes overlap).
// Samples near the arc's own endpoints are ignored.
double min_foreign_circle_clearance(const Svg& s) {
    double best = 1e9;
    for (size_t pi = 0; pi < s.paths.size(); ++pi) {
        const Path& p = s.paths[pi];
        int sc = circle_at(s, p.start), ec = circle_at(s, p.end);
        std::vector<Pt> pts = sample(p);
        for (size_t ci = 0; ci < s.circles.size(); ++ci) {
            if (static_cast<int>(ci) == sc || static_cast<int>(ci) == ec) continue;
            const Circle& c = s.circles[ci];
            Pt cc = {c.cx, c.cy};
            double lim = c.r + c.sw / 2 + p.sw / 2;
            for (size_t k = 0; k < pts.size(); ++k) {
                if (dist(pts[k], p.start) <= 8 || dist(pts[k], p.end) <= 16) continue;
                double d = dist(pts[k], cc) - lim;
                if (d < best) best = d;
            }
        }
    }
    return best;
}

// Like min_foreign_circle_clearance, but only against circles on the same
// row as the arc's own parent (the row a routed arc has to leave sideways).
double min_parent_row_clearance(const Svg& s) {
    double best = 1e9;
    for (size_t pi = 0; pi < s.paths.size(); ++pi) {
        const Path& p = s.paths[pi];
        int sc = circle_at(s, p.start), ec = circle_at(s, p.end);
        if (sc < 0) continue;
        std::vector<Pt> pts = sample(p);
        for (size_t ci = 0; ci < s.circles.size(); ++ci) {
            if (static_cast<int>(ci) == sc || static_cast<int>(ci) == ec) continue;
            const Circle& c = s.circles[ci];
            if (std::fabs(c.cy - s.circles[sc].cy) > 0.5) continue;
            Pt cc = {c.cx, c.cy};
            double lim = c.r + c.sw / 2 + p.sw / 2;
            for (size_t k = 0; k < pts.size(); ++k) {
                if (dist(pts[k], p.start) <= 8) continue;
                double d = dist(pts[k], cc) - lim;
                if (d < best) best = d;
            }
        }
    }
    return best;
}

int count_degenerate_commands(const Svg& s) {
    int n = 0;
    for (size_t pi = 0; pi < s.paths.size(); ++pi) {
        for (size_t i = 0; i < s.paths[pi].segs.size(); ++i) {
            const Seg& g = s.paths[pi].segs[i];
            if (g.type == 'L' && dist(g.p0, g.p1) < 0.01) ++n;
            if (g.type == 'Q' && (dist(g.p0, g.c) < 0.01 || dist(g.c, g.p1) < 0.01)) ++n;
        }
    }
    return n;
}

ZDD family(const std::vector<std::vector<bddvar> >& sets) { return ZDD::from_sets(sets); }

} // namespace

class SvgLayoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        for (int i = 0; i < 16; ++i) bddnewvar();
    }
};

// --- Review item 1: lanes must reach the next drawn level when levels are skipped ---

TEST_F(SvgLayoutTest, SkipUnusedLevels_LaneReachesNextDrawnLevel) {
    // Variables 1,4,7,10,13 only: with skip_unused_levels every lane's
    // "lev-1" row is absent. Before the fix the lane collapsed to zero length
    // and the arc cut straight through the middle node of the next row.
    ZDD z = family({{1, 10, 13}, {7, 10}, {1, 4, 7}});
    SvgParams p;
    p.skip_unused_levels = true;
    Svg s = parse_svg(z.save_svg(p));
    ASSERT_EQ(s.circles.size(), 7u);
    EXPECT_EQ(count_degenerate_commands(s), 0);
    EXPECT_GE(min_foreign_circle_clearance(s), 0.0);
}

TEST_F(SvgLayoutTest, SkipUnusedLevels_NoZeroLengthCommands) {
    SvgParams p;
    p.skip_unused_levels = true;
    EXPECT_EQ(count_degenerate_commands(parse_svg(family({{1}, {4, 7}, {13}}).save_svg(p))), 0);
    EXPECT_EQ(count_degenerate_commands(parse_svg(family({{7}, {1, 13}, {4, 7, 10, 13}, {1, 10, 13}}).save_svg(p))), 0);
}

TEST_F(SvgLayoutTest, ConsecutiveLanes_NoZeroLengthCommands) {
    // A 4-node chain whose hi arc from the root skips two rows uses the same
    // lane x on consecutive levels; the shared corner point must not produce
    // zero-length L/Q commands.
    Svg s = parse_svg(family({{2, 3}, {}, {1, 4}}).save_svg());
    EXPECT_EQ(count_degenerate_commands(s), 0);
    EXPECT_GE(min_foreign_circle_clearance(s), 0.0);
}

// --- Review items 2 and 6: routed arcs drop into the gap, runs do not merge ---

namespace {

// ZDD node (v, lo, hi) built with set operations (top(lo), top(hi) < v).
ZDD znode(int v, const ZDD& lo, const ZDD& hi) { return lo + hi.change(v); }

// Six level-4 nodes whose lo arcs all skip to the same level-1 node, plus a
// wide level 2: many arcs funnel into the same lanes and terminal lanes.
ZDD lane_funnel() {
    ZDD B1 = ZDD::singleton(1);
    ZDD B2 = ZDD::power_set(1);
    ZDD los[4] = { ZDD(0), ZDD(1), B1, B2 };
    ZDD his[3] = { ZDD(1), B1, B2 };
    std::vector<ZDD> L2;
    for (int a = 0; a < 4; ++a)
        for (int b = 0; b < 3; ++b) L2.push_back(znode(2, los[a], his[b]));
    std::vector<ZDD> L3;
    for (int i = 0; i < 6; ++i) L3.push_back(znode(3, L2[i], L2[(i + 3) % 12]));
    std::vector<ZDD> L4;
    for (int i = 0; i < 6; ++i) L4.push_back(znode(4, B2, L3[i]));
    std::vector<ZDD> L5;
    for (int i = 0; i < 6; i += 2) L5.push_back(znode(5, L4[i], L4[i + 1]));
    ZDD L6a = znode(6, L5[0], L5[1]);
    return znode(7, L6a, L5[2]);
}

// Horizontal straight segments (lane runs) longer than min_len.
struct HRun { size_t path; double x0, x1, y; };
std::vector<HRun> horizontal_runs(const Svg& s, double min_len = 20.0) {
    std::vector<HRun> out;
    for (size_t pi = 0; pi < s.paths.size(); ++pi) {
        for (size_t i = 0; i < s.paths[pi].segs.size(); ++i) {
            const Seg& g = s.paths[pi].segs[i];
            if (g.type != 'L') continue;
            if (std::fabs(g.p0.y - g.p1.y) > 0.01) continue;
            if (std::fabs(g.p0.x - g.p1.x) < min_len) continue;
            HRun r = {pi, std::min(g.p0.x, g.p1.x), std::max(g.p0.x, g.p1.x), g.p0.y};
            out.push_back(r);
        }
    }
    return out;
}

// Number of pairs of horizontal runs from different arcs that lie on the
// same y and overlap in x (i.e. are drawn on top of each other).
int overlapping_run_pairs(const Svg& s) {
    std::vector<HRun> runs = horizontal_runs(s);
    int n = 0;
    for (size_t i = 0; i < runs.size(); ++i)
        for (size_t j = i + 1; j < runs.size(); ++j)
            if (runs[i].path != runs[j].path && std::fabs(runs[i].y - runs[j].y) < 0.5 &&
                runs[i].x0 < runs[j].x1 - 1 && runs[j].x0 < runs[i].x1 - 1)
                ++n;
    return n;
}

} // namespace

TEST_F(SvgLayoutTest, RoutedArcs_DoNotGrazeParentRow) {
    // Before the fix the sideways run started right at the parent's exit
    // point and passed 0-3 px below every sibling circle on its way to the
    // terminal lane.
    Svg s = parse_svg(lane_funnel().save_svg());
    ASSERT_GT(s.circles.size(), 20u);
    EXPECT_GE(min_parent_row_clearance(s), 4.0);
}

TEST_F(SvgLayoutTest, RoutedArcs_RunsInOneGapHaveDistinctHeights) {
    Svg s = parse_svg(lane_funnel().save_svg());
    std::vector<HRun> runs = horizontal_runs(s);
    ASSERT_GE(runs.size(), 4u) << "expected several lane runs in this drawing";
    EXPECT_EQ(overlapping_run_pairs(s), 0);
    // Every run stays inside the gap between two rows: at least 4 px from
    // the outline of the circles above and below it.
    for (size_t i = 0; i < runs.size(); ++i) {
        for (size_t ci = 0; ci < s.circles.size(); ++ci) {
            const Circle& c = s.circles[ci];
            if (c.cx + c.r < runs[i].x0 || c.cx - c.r > runs[i].x1) continue;
            EXPECT_GE(std::fabs(runs[i].y - c.cy), c.r + c.sw / 2 + 4.0)
                << "run at y=" << runs[i].y << " too close to circle at y=" << c.cy;
        }
    }
}

TEST_F(SvgLayoutTest, RoutedArcs_SmallFamily_NoGrazing) {
    ZDD z = family({{1, 3, 4}, {1, 2, 4}, {3, 4}, {1, 3, 5, 6}, {1, 2, 3, 4, 6},
                    {1, 6}, {2, 3}, {1, 2, 3, 5}, {1, 5}});
    Svg s = parse_svg(z.save_svg());
    EXPECT_GE(min_parent_row_clearance(s), 4.0);
    EXPECT_EQ(overlapping_run_pairs(s), 0);
}

// --- Review item 4: straight arcs keep a clearance from every other node ---

namespace {

// Eight distinct nodes on level 2 feeding two nodes on level 1: long,
// shallow one-row arcs.
ZDD wide_level2() {
    ZDD B1 = ZDD::singleton(1);
    ZDD B2 = ZDD::power_set(1);
    ZDD los[4] = { ZDD(0), ZDD(1), B1, B2 };
    ZDD his[3] = { ZDD(1), B1, B2 };
    std::vector<ZDD> L2;
    for (int a = 0; a < 4; ++a)
        for (int b = 0; b < 3; ++b) L2.push_back(znode(2, los[a], his[b]));
    int idx[8] = {0, 11, 1, 10, 2, 9, 5, 7};
    std::vector<ZDD> pick;
    for (int i = 0; i < 8; ++i) pick.push_back(L2[idx[i]]);
    std::vector<ZDD> L3;
    for (int i = 0; i < 8; i += 2) L3.push_back(znode(3, pick[i], pick[i + 1]));
    std::vector<ZDD> L4;
    for (int i = 0; i < 4; i += 2) L4.push_back(znode(4, L3[i], L3[i + 1]));
    return znode(5, L4[0], L4[1]);
}

} // namespace

TEST_F(SvgLayoutTest, StraightArcs_KeepClearanceFromIntermediateRows) {
    // combination(6,3): the lo arc of the leftmost "3" node passed 0.9 px
    // into the outline of the left "2" node (the old check used the bare
    // radius, ignoring both stroke widths).
    Svg s = parse_svg(ZDD::combination(6, 3).save_svg());
    ASSERT_EQ(s.circles.size(), 12u);
    EXPECT_GE(min_foreign_circle_clearance(s), 2.0);
    EXPECT_GE(min_foreign_circle_clearance(parse_svg(family({{3}, {1, 3, 4}, {3, 4}}).save_svg())), 2.0);
    EXPECT_GE(min_foreign_circle_clearance(parse_svg(family({{3}, {1}, {2, 3, 4}}).save_svg())), 2.0);
}

TEST_F(SvgLayoutTest, StraightArcs_KeepClearanceFromChildRow) {
    // Long one-row arcs used to graze the neighbours of their target node;
    // they are now routed through the gap instead.
    Svg s = parse_svg(wide_level2().save_svg());
    ASSERT_EQ(s.circles.size(), 17u);
    EXPECT_GE(min_foreign_circle_clearance(s), 0.0);
    EXPECT_EQ(overlapping_run_pairs(s), 0);
}

TEST_F(SvgLayoutTest, AllArcs_ClearOfOtherNodes_Funnel) {
    EXPECT_GE(min_foreign_circle_clearance(parse_svg(lane_funnel().save_svg())), 0.0);
    ZDD z = family({{1, 3, 4}, {1, 2, 4}, {3, 4}, {1, 3, 5, 6}, {1, 2, 3, 4, 6},
                    {1, 6}, {2, 3}, {1, 2, 3, 5}, {1, 5}});
    EXPECT_GE(min_foreign_circle_clearance(parse_svg(z.save_svg())), 0.0);
}

TEST_F(SvgLayoutTest, RoutedArcs_FinalApproachClearsChildRow) {
    // The last lane of a routed arc may end far from the child; the final
    // approach used to sweep across the child's row and graze its
    // neighbours. Two random-search families that showed this.
    ZDD a = family({{2, 7, 8}, {1, 3, 5, 6, 7, 8}, {1, 3, 6, 8}, {2, 5, 7, 8}, {1, 5},
                    {1, 2, 4, 7, 8}, {2, 3, 7, 8}, {1, 3, 5, 7}, {3, 4, 8}, {1, 3, 5, 7, 8},
                    {4, 5, 6, 7}, {1}, {2, 4, 6, 8}, {1, 3, 4, 8}, {2, 3, 4, 6, 7, 8}});
    Svg sa = parse_svg(a.save_svg());
    EXPECT_GE(min_foreign_circle_clearance(sa), 0.0);
    EXPECT_EQ(overlapping_run_pairs(sa), 0);
    ZDD b = family({{2, 4, 5}, {1, 2, 5, 7}, {1, 2, 4, 6}, {3, 4}, {2, 3, 4, 5}, {3}, {1, 4, 6},
                    {2, 3, 5, 7}, {1, 2, 4, 5}, {3, 5, 6, 7}, {4, 7}, {3, 5, 6, 7}, {3, 5},
                    {1, 2, 3, 4, 5, 7}, {2, 4, 5, 6}, {2, 3, 7}, {3, 4}, {6, 7}, {2, 3, 5, 6},
                    {5, 7}, {1, 3, 6, 7}, {1, 2, 3, 6, 7}, {1, 2, 3, 4, 5}});
    Svg sb = parse_svg(b.save_svg());
    EXPECT_GE(min_foreign_circle_clearance(sb), 0.0);
    EXPECT_EQ(overlapping_run_pairs(sb), 0);
}

// --- Review item 3: arcs attach on the side of the child that faces the parent ---

namespace {

// Number of arcs whose end point lies on the far side of the child circle
// (parent clearly left but end point right of the top sector, or vice
// versa), plus arcs whose final segment does not point into the circle
// (tangential or outward arrival, i.e. the arc crossed the circle to reach
// its point).
int count_bad_arrivals(const Svg& s) {
    int bad = 0;
    for (size_t pi = 0; pi < s.paths.size(); ++pi) {
        const Path& p = s.paths[pi];
        int sc = circle_at(s, p.start), ec = circle_at(s, p.end);
        if (sc < 0 || ec < 0) continue;
        const Circle& c = s.circles[ec];
        double ang = std::atan2(-(p.end.y - c.cy), p.end.x - c.cx) * 180.0 / M_PI;
        double px = s.circles[sc].cx;
        if ((px < c.cx - 30 && ang < 60) || (px > c.cx + 30 && ang > 120)) { ++bad; continue; }
        const Seg& last = p.segs.back();
        Pt prev = (last.type == 'L') ? last.p0 : last.c;
        double dx = p.end.x - prev.x, dy = p.end.y - prev.y;
        double nx = p.end.x - c.cx, ny = p.end.y - c.cy;
        double L = std::hypot(dx, dy), N = std::hypot(nx, ny);
        if (L > 0 && N > 0 && (dx * nx + dy * ny) / (L * N) > -0.3) ++bad;
    }
    return bad;
}

} // namespace

TEST_F(SvgLayoutTest, EndPoints_FaceTheParent) {
    // Both families had an arc whose parent is left of the child but which
    // was given the upper-right attachment point (end angles were handed
    // out by rank among the incoming arcs, not by direction).
    EXPECT_EQ(count_bad_arrivals(parse_svg(family({{1, 2, 3}, {1}, {1, 2, 4}}).save_svg())), 0);
    EXPECT_EQ(count_bad_arrivals(parse_svg(family({{1, 2, 3, 5}, {1, 5}, {3, 4}}).save_svg())), 0);
    EXPECT_EQ(count_bad_arrivals(parse_svg(lane_funnel().save_svg())), 0);
    EXPECT_EQ(count_bad_arrivals(parse_svg(wide_level2().save_svg())), 0);
}

TEST_F(SvgLayoutTest, EndPoints_StayInTopSector) {
    Svg s = parse_svg(lane_funnel().save_svg());
    for (size_t pi = 0; pi < s.paths.size(); ++pi) {
        int ec = circle_at(s, s.paths[pi].end);
        if (ec < 0) continue;
        const Circle& c = s.circles[ec];
        double ang = std::atan2(-(s.paths[pi].end.y - c.cy), s.paths[pi].end.x - c.cx) * 180.0 / M_PI;
        // sector [30, 150] degrees; end points are rounded to integer px
        EXPECT_GE(ang, 27.0);
        EXPECT_LE(ang, 153.0);
    }
}
