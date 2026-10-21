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
