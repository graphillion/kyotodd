// SVG export engine for KyotoDD BDD/ZDD/QDD/UnreducedDD.
// Faithfully ports the Python drawing pipeline (dd_draw/) to C++11.

#include "bdd.h"
#include "bdd_internal.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ========================================================================
//  Anonymous namespace — all internal classes and helpers
// ========================================================================
namespace {

// ---- Numeric formatting helper ------------------------------------------
// Format a double for SVG attribute values, trimming trailing zeros.
static std::string fmt_double(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.6g", v);
    return std::string(buf);
}

// ---- Node structures (same layout as GvNode in bdd_io.cpp) --------------

struct SvgNode {
    bddp  key;       // Expanded: full bddp, Raw: complement-stripped
    bddvar var;       // variable number
    int    level;     // level in DD (from bddlevofvar)
    bddp   lo;        // lo child key
    bddp   hi;        // hi child key
    bool   lo_comp;   // Raw mode: lo edge is complement
    bool   hi_comp;   // Raw mode: hi edge is complement
};

// ---- Child resolvers (BDD vs ZDD complement semantics) ------------------

struct BddChildResolver {
    void operator()(bddp raw, bool comp, bddp& lo, bddp& hi) const {
        lo = node_lo(raw);
        hi = node_hi(raw);
        if (comp) { lo = bddnot(lo); hi = bddnot(hi); }
    }
};

struct ZddChildResolver {
    void operator()(bddp raw, bool comp, bddp& lo, bddp& hi) const {
        lo = node_lo(raw);
        hi = node_hi(raw);
        if (comp) { lo = bddnot(lo); }
    }
};

// ---- Node collection (DFS, matching graphviz_collect in bdd_io.cpp) -----

template<typename ChildResolver>
static void svg_collect_expanded(
    bddp f, ChildResolver resolve,
    std::vector<SvgNode>& nodes,
    std::unordered_map<bddp, size_t>& id_map,
    bool& has_false, bool& has_true)
{
    std::vector<std::pair<bddp, bool> > stack;
    stack.push_back(std::make_pair(f, false));
    while (!stack.empty()) {
        bddp p = stack.back().first;
        bool expanded = stack.back().second;

        if (p & BDD_CONST_FLAG) {
            stack.pop_back();
            if (p == bddfalse) has_false = true;
            else               has_true  = true;
            continue;
        }

        if (id_map.count(p)) {
            stack.pop_back();
            continue;
        }

        if (!expanded) {
            stack.back().second = true;
            bddp raw = p & ~BDD_COMP_FLAG;
            bool comp = (p & BDD_COMP_FLAG) != 0;
            bddp lo, hi;
            resolve(raw, comp, lo, hi);
            // Track terminal children
            if (lo & BDD_CONST_FLAG) {
                if (lo == bddfalse) has_false = true; else has_true = true;
            }
            if (hi & BDD_CONST_FLAG) {
                if (hi == bddfalse) has_false = true; else has_true = true;
            }
            if (!(hi & BDD_CONST_FLAG) && !id_map.count(hi))
                stack.push_back(std::make_pair(hi, false));
            if (!(lo & BDD_CONST_FLAG) && !id_map.count(lo))
                stack.push_back(std::make_pair(lo, false));
        } else {
            stack.pop_back();
            bddp raw = p & ~BDD_COMP_FLAG;
            bool comp = (p & BDD_COMP_FLAG) != 0;
            bddp lo, hi;
            resolve(raw, comp, lo, hi);

            size_t idx = nodes.size();
            id_map[p] = idx;
            SvgNode sn;
            sn.key     = p;
            sn.var     = node_var(raw);
            sn.level   = static_cast<int>(bddlevofvar(sn.var));
            sn.lo      = lo;
            sn.hi      = hi;
            sn.lo_comp = false;
            sn.hi_comp = false;
            nodes.push_back(sn);
        }
    }
}

static void svg_collect_raw(
    bddp f,
    std::vector<SvgNode>& nodes,
    std::unordered_map<bddp, size_t>& id_map,
    bool& has_false, bool& has_true)
{
    std::vector<std::pair<bddp, bool> > stack;
    bddp root = f & ~BDD_COMP_FLAG;
    stack.push_back(std::make_pair(root, false));
    while (!stack.empty()) {
        bddp p = stack.back().first;
        bool expanded = stack.back().second;

        if (p & BDD_CONST_FLAG) {
            stack.pop_back();
            if (p == bddfalse) has_false = true;
            else               has_true  = true;
            continue;
        }

        if (id_map.count(p)) {
            stack.pop_back();
            continue;
        }

        if (!expanded) {
            stack.back().second = true;
            bddp lo = node_lo(p);
            bddp hi = node_hi(p);
            bddp lo_raw = (lo & BDD_CONST_FLAG) ? lo : (lo & ~BDD_COMP_FLAG);
            bddp hi_raw = (hi & BDD_CONST_FLAG) ? hi : (hi & ~BDD_COMP_FLAG);
            // Track terminal children
            if (lo_raw & BDD_CONST_FLAG) {
                if (lo_raw == bddfalse) has_false = true; else has_true = true;
            }
            if (hi_raw & BDD_CONST_FLAG) {
                if (hi_raw == bddfalse) has_false = true; else has_true = true;
            }
            if (!(hi_raw & BDD_CONST_FLAG) && !id_map.count(hi_raw))
                stack.push_back(std::make_pair(hi_raw, false));
            if (!(lo_raw & BDD_CONST_FLAG) && !id_map.count(lo_raw))
                stack.push_back(std::make_pair(lo_raw, false));
        } else {
            stack.pop_back();
            bddp lo = node_lo(p);
            bddp hi = node_hi(p);

            size_t idx = nodes.size();
            id_map[p] = idx;
            SvgNode sn;
            sn.key   = p;
            sn.var   = node_var(p);
            sn.level = static_cast<int>(bddlevofvar(sn.var));
            bool lo_is_term = (lo & BDD_CONST_FLAG) != 0;
            bool hi_is_term = (hi & BDD_CONST_FLAG) != 0;
            sn.lo      = lo_is_term ? lo : (lo & ~BDD_COMP_FLAG);
            sn.hi      = hi_is_term ? hi : (hi & ~BDD_COMP_FLAG);
            sn.lo_comp = !lo_is_term && (lo & BDD_COMP_FLAG) != 0;
            sn.hi_comp = !hi_is_term && (hi & BDD_COMP_FLAG) != 0;
            nodes.push_back(sn);
        }
    }
}

// ---- Coordinate types ---------------------------------------------------

typedef std::pair<double, double> Point;  // (x, y)

// Arc key: (node_index_in_nodes_vector, arc_type 0 or 1)
typedef std::pair<size_t, int> ArcKey;

// ---- SvgLaneManager (port of lane_manager.py) ---------------------------

class SvgLaneManager {
public:
    const SvgParams& params;

    // Per-level lanes: lane[level] = sorted vector of lane x positions
    std::map<int, std::vector<int> > lane;

    // Lane usage: (level, lane_x) -> list of (arc_key, prev_level_x)
    std::map<std::pair<int,int>, std::vector<std::pair<ArcKey,int> > > lane_usage;

    // Terminal lane usage: level -> list of (arc_key, parent_pos_x)
    std::map<int, std::vector<std::pair<ArcKey,int> > > leftmost_lane_usage;
    std::map<int, std::vector<std::pair<ArcKey,int> > > rightmost_lane_usage;

    int leftmost_x;
    int rightmost_x;

    // Arc lanes: arc_key -> list of (level, lane_x, prev_x)
    std::map<ArcKey, std::vector<std::tuple<int,int,int> > > arc_lanes;

    // Terminal lane to x: lane_x -> list of (arc_key, level, pos_x)
    struct TermLaneEntry {
        ArcKey arc_key;
        int level;
        int pos_x;
    };
    std::map<int, std::vector<TermLaneEntry> > leftmost_lane_to_x;
    std::map<int, std::vector<TermLaneEntry> > rightmost_lane_to_x;

    explicit SvgLaneManager(const SvgParams& p)
        : params(p), leftmost_x(0), rightmost_x(0) {}

    // --- calculate_lanes ---
    void calculate_lanes(
        const std::map<int, std::vector<size_t> >& all_nodes,
        const std::map<size_t, Point>& /* node_pos_map */,
        int node_x)
    {
        lane.clear();
        // Process levels in descending order
        for (auto it = all_nodes.rbegin(); it != all_nodes.rend(); ++it) {
            int level = it->first;
            const std::vector<size_t>& nodes = it->second;
            int num_nodes = static_cast<int>(nodes.size());

            int x = node_x / (num_nodes + 1) - (params.node_radius + params.node_interval_x);

            std::vector<int> lv;
            lv.push_back(x - params.node_radius * 2);

            for (int j = 0; j < num_nodes; ++j) {
                x += node_x / (num_nodes + 1);
                lv.push_back(x - node_x / (num_nodes + 1) / 2);
            }
            // Adjust last lane position
            lv.back() = x - node_x / (num_nodes + 1) + params.node_radius * 2;
            lane[level] = lv;
        }
    }

    // --- initialize_lane_usage ---
    void initialize_lane_usage() {
        lane_usage.clear();
        leftmost_lane_usage.clear();
        rightmost_lane_usage.clear();
        leftmost_lane_to_x.clear();
        rightmost_lane_to_x.clear();

        std::vector<int> all_x_coords;
        for (auto& kv : lane) {
            for (int v : kv.second) all_x_coords.push_back(v);
        }
        if (all_x_coords.empty()) {
            leftmost_x = 0;
            rightmost_x = 0;
        } else {
            leftmost_x  = *std::min_element(all_x_coords.begin(), all_x_coords.end()) - params.arc_terminal;
            rightmost_x = *std::max_element(all_x_coords.begin(), all_x_coords.end()) + params.arc_terminal;
        }
    }

    // --- record_terminal_lane_usage ---
    void record_terminal_lane_usage(int parent_level, bool is_terminal_0, ArcKey arc_key, int parent_pos_x) {
        assert(parent_level >= 2);
        if (is_terminal_0) {
            leftmost_lane_usage[parent_level - 1].push_back(std::make_pair(arc_key, parent_pos_x));
        } else {
            rightmost_lane_usage[parent_level - 1].push_back(std::make_pair(arc_key, parent_pos_x));
        }
    }

    // --- _get_next_pos_x ---
    int get_next_pos_x(int current_pos_x, int end_x, int lev) const {
        const std::vector<int>& lv = lane.at(lev);
        assert(!lv.empty());

        if (current_pos_x <= end_x) {
            // Going right: find first lane >= current_pos_x
            int next_x = lv.back(); // default: rightmost
            std::vector<int> sorted_lv = lv;
            std::sort(sorted_lv.begin(), sorted_lv.end());
            for (int v : sorted_lv) {
                if (v >= current_pos_x) {
                    next_x = v;
                    break;
                }
            }
            return next_x;
        } else {
            // Going left: find first lane <= current_pos_x (scanning from right)
            int next_x = lv.front(); // default: leftmost
            std::vector<int> sorted_lv = lv;
            std::sort(sorted_lv.begin(), sorted_lv.end());
            for (int i = static_cast<int>(sorted_lv.size()) - 1; i >= 0; --i) {
                if (sorted_lv[i] <= current_pos_x) {
                    next_x = sorted_lv[i];
                    break;
                }
            }
            return next_x;
        }
    }

    // --- calculate_normal_lanes ---
    // Returns: vector of (level, lane_x, prev_x)
    std::vector<std::tuple<int,int,int> > calculate_normal_lanes(
        int start_x, int end_x, int parent_level, int child_level) const
    {
        std::vector<std::tuple<int,int,int> > used_lanes;
        int current_pos_x = start_x;

        for (int lev = parent_level - 1; lev > child_level; --lev) {
            if (lane.find(lev) == lane.end()) continue;
            int next_pos_x = get_next_pos_x(current_pos_x, end_x, lev);
            used_lanes.push_back(std::make_tuple(lev, next_pos_x, current_pos_x));
            current_pos_x = next_pos_x;
        }
        return used_lanes;
    }

    // --- determine_arc_lanes ---
    void determine_arc_lanes(
        const std::map<bddp, std::vector<std::pair<size_t,int> > >& dest_info,
        const std::map<size_t, Point>& node_pos_map,
        const std::vector<SvgNode>& nodes,
        const std::map<ArcKey, bool>& arc_is_straight_map,
        bool draw_zero)
    {
        arc_lanes.clear();

        for (auto& kv : dest_info) {
            bddp child = kv.first;
            std::vector<std::pair<size_t,int> > arcs = kv.second; // copy for sorting

            bool child_is_false = (child == bddfalse);
            bool child_is_terminal = (child & BDD_CONST_FLAG) != 0;
            int child_level = child_is_terminal ? 0 : nodes[0].level; // fallback, will compute properly

            if (!draw_zero && child_is_false) continue;

            // Sort arcs by parent x coordinate
            std::sort(arcs.begin(), arcs.end(),
                [&](const std::pair<size_t,int>& a, const std::pair<size_t,int>& b) {
                    return node_pos_map.at(a.first).first < node_pos_map.at(b.first).first;
                });

            for (size_t i = 0; i < arcs.size(); ++i) {
                size_t parent_idx = arcs[i].first;
                int arc_type = arcs[i].second;
                Point parent_pos = node_pos_map.at(parent_idx);
                int parent_level = nodes[parent_idx].level;

                if (child_is_terminal) {
                    child_level = 0;
                } else {
                    // child is a node key; find its index
                    // In dest_info keys are bddp values; for non-terminal, look up index
                    // child_level will be determined below
                    // We need the child's level; but child is a bddp key
                    // Lookup in id_map would be needed; instead, we store it via nodes
                    // For simplicity, we find the child node index
                    child_level = 0; // placeholder
                    for (size_t ni = 0; ni < nodes.size(); ++ni) {
                        if (nodes[ni].key == child) {
                            child_level = nodes[ni].level;
                            break;
                        }
                    }
                }

                ArcKey arc_key = std::make_pair(parent_idx, arc_type);
                auto sit = arc_is_straight_map.find(arc_key);
                bool is_straight = (sit != arc_is_straight_map.end()) ? sit->second : true;

                if (!is_straight) {
                    if (child_is_terminal && parent_level - 1 > child_level) {
                        record_terminal_lane_usage(parent_level, child_is_false,
                                                   arc_key, static_cast<int>(parent_pos.first));
                    } else if (parent_level - 1 > child_level) {
                        int child_pos_x = 0;
                        // For non-terminal children
                        for (size_t ni = 0; ni < nodes.size(); ++ni) {
                            if (nodes[ni].key == child) {
                                child_pos_x = static_cast<int>(node_pos_map.at(ni).first);
                                break;
                            }
                        }
                        std::vector<std::tuple<int,int,int> > used =
                            calculate_normal_lanes(
                                static_cast<int>(parent_pos.first),
                                child_pos_x,
                                parent_level, child_level);

                        arc_lanes[arc_key] = used;

                        for (size_t li = 0; li < used.size(); ++li) {
                            int lev = std::get<0>(used[li]);
                            int lx  = std::get<1>(used[li]);
                            int px  = std::get<2>(used[li]);
                            std::pair<int,int> lane_key = std::make_pair(lev, lx);
                            lane_usage[lane_key].push_back(std::make_pair(arc_key, px));
                        }
                    }
                }
            }
        }

        adjust_lane_usage();
    }

    void adjust_lane_usage() {
        // Sort lane usage by "prev_x"
        for (auto& kv : lane_usage) {
            std::sort(kv.second.begin(), kv.second.end(),
                [](const std::pair<ArcKey,int>& a, const std::pair<ArcKey,int>& b) {
                    return a.second < b.second;
                });
        }

        // Build leftmost_lane_to_x
        for (auto& kv : leftmost_lane_usage) {
            int level = kv.first;
            int lane_x = get_leftmost_lane(level);
            for (auto& ap : kv.second) {
                TermLaneEntry e;
                e.arc_key = ap.first;
                e.level = level;
                e.pos_x = ap.second;
                leftmost_lane_to_x[lane_x].push_back(e);
            }
        }

        // Build rightmost_lane_to_x
        for (auto& kv : rightmost_lane_usage) {
            int level = kv.first;
            int lane_x = get_rightmost_lane(level);
            for (auto& ap : kv.second) {
                TermLaneEntry e;
                e.arc_key = ap.first;
                e.level = level;
                e.pos_x = ap.second;
                rightmost_lane_to_x[lane_x].push_back(e);
            }
        }

        // Sort leftmost_lane_to_x entries: by (-level * 10000000 + pos_x)
        for (auto& kv : leftmost_lane_to_x) {
            std::sort(kv.second.begin(), kv.second.end(),
                [](const TermLaneEntry& a, const TermLaneEntry& b) {
                    long long va = static_cast<long long>(a.level) * -10000000LL + a.pos_x;
                    long long vb = static_cast<long long>(b.level) * -10000000LL + b.pos_x;
                    return va < vb;
                });
        }

        // Sort rightmost_lane_to_x entries: by (level * 10000000 + pos_x)
        for (auto& kv : rightmost_lane_to_x) {
            std::sort(kv.second.begin(), kv.second.end(),
                [](const TermLaneEntry& a, const TermLaneEntry& b) {
                    long long va = static_cast<long long>(a.level) * 10000000LL + a.pos_x;
                    long long vb = static_cast<long long>(b.level) * 10000000LL + b.pos_x;
                    return va < vb;
                });
        }
    }

    int get_leftmost_lane(int level) const {
        int min_left_x = 99999999;
        for (int i = level; i >= 1; --i) {
            auto it = lane.find(i);
            if (it != lane.end() && !it->second.empty()) {
                int m = *std::min_element(it->second.begin(), it->second.end());
                if (m < min_left_x) min_left_x = m;
            }
        }
        return min_left_x - 5;
    }

    int get_rightmost_lane(int level) const {
        int max_right_x = -99999999;
        for (int i = level; i >= 1; --i) {
            auto it = lane.find(i);
            if (it != lane.end() && !it->second.empty()) {
                int m = *std::max_element(it->second.begin(), it->second.end());
                if (m > max_right_x) max_right_x = m;
            }
        }
        return max_right_x + 5;
    }

    // --- calculate_lane_offset ---
    int calculate_lane_offset(ArcKey arc_key, int lane_x, int level) const {
        int arc_interval = params.arc_interval;
        std::pair<int,int> lane_key = std::make_pair(level, lane_x);
        auto it = lane_usage.find(lane_key);
        assert(it != lane_usage.end());
        int total_users = static_cast<int>(it->second.size());

        int offset_start;
        if (total_users % 2 == 1) {
            offset_start = -arc_interval * (total_users / 2);
        } else {
            offset_start = -arc_interval * ((total_users - 1) / 2);
        }

        int user_index = 0;
        for (auto& entry : it->second) {
            if (entry.first == arc_key) break;
            user_index++;
        }

        int x_offset = offset_start + user_index * arc_interval;
        return lane_x + x_offset;
    }

    // --- find_valid_level ---
    int find_valid_level(int parent_level, int child_level,
                         const std::map<int,int>& level_y_map) const
    {
        int next_level = parent_level - 1;
        while (next_level > child_level && level_y_map.find(next_level) == level_y_map.end()) {
            --next_level;
        }
        if (next_level <= child_level || level_y_map.find(next_level) == level_y_map.end()) {
            next_level = parent_level;
        }
        return next_level;
    }
};

// ---- SvgPositionManager (port of position_manager.py) -------------------

class SvgPositionManager {
public:
    const SvgParams& params;
    const std::vector<SvgNode>& nodes;
    bool draw_zero;

    // all_nodes: level -> vector of node indices (into nodes[])
    std::map<int, std::vector<size_t> > all_nodes;

    // Position maps (indexed by node index in nodes[])
    std::map<size_t, Point> node_pos_map;

    // Arc points: arc_key -> vector of (x,y) waypoints
    std::map<ArcKey, std::vector<Point> > arc_pos_map;
    std::map<ArcKey, Point> arc_adjusted_start_point_map;
    std::map<ArcKey, Point> arc_end_point_map;
    std::map<ArcKey, bool>  arc_is_straight_map;

    // Level -> y coordinate
    std::map<int, int> level_y_map;

    // dest_info: child_key(bddp) -> list of (parent_node_idx, arc_type)
    std::map<bddp, std::vector<std::pair<size_t,int> > > dest_info;

    int node_x; // canvas width

    // Terminal position keys: we use SIZE_MAX and SIZE_MAX-1 as special indices
    // for terminals in node_pos_map
    // Use functions instead of static const to avoid ODR issues in C++11
    static size_t TERM_0_IDX() { return SIZE_MAX; }
    static size_t TERM_1_IDX() { return SIZE_MAX - 1; }

    bool has_false;
    bool has_true;

    int root_level;

    SvgLaneManager lane_manager;

    SvgPositionManager(const SvgParams& p,
                       const std::vector<SvgNode>& n,
                       bool hf, bool ht)
        : params(p), nodes(n), draw_zero(p.draw_zero),
          node_x(0), has_false(hf), has_true(ht),
          root_level(0), lane_manager(p)
    {
        // Build all_nodes grouped by level, preserving DFS order within each level
        for (size_t i = 0; i < nodes.size(); ++i) {
            all_nodes[nodes[i].level].push_back(i);
        }

        // Find root level = max level
        for (auto& kv : all_nodes) {
            if (kv.first > root_level) root_level = kv.first;
        }
    }

    void compute_all_positions() {
        calculate_node_positions();
        calculate_terminal_positions();
        calculate_arc_info();
        calculate_all_end_points();
        calculate_arc_connection_points();
    }

private:
    // --- circle point helpers ---
    static int circle_pos_x(int x, int r, double rad) {
        return x + static_cast<int>(r * std::cos(rad));
    }
    static int circle_pos_y(int y, int r, double rad) {
        return y - static_cast<int>(r * std::sin(rad));
    }

    // --- segment-circle intersection test ---
    static bool segment_intersects_any_circle(
        Point p1, Point p2, int r,
        const std::vector<Point>& centers)
    {
        double x1 = p1.first, y1 = p1.second;
        double x2 = p2.first, y2 = p2.second;
        double dx = x2 - x1;
        double dy = y2 - y1;
        double length_sq = dx*dx + dy*dy;
        double r2 = static_cast<double>(r) * r;

        for (size_t i = 0; i < centers.size(); ++i) {
            double cx = centers[i].first;
            double cy = centers[i].second;
            // Check endpoints inside circle
            if ((cx-x1)*(cx-x1) + (cy-y1)*(cy-y1) <= r2) return true;
            if ((cx-x2)*(cx-x2) + (cy-y2)*(cy-y2) <= r2) return true;
            if (length_sq == 0.0) continue;
            double t = ((cx-x1)*dx + (cy-y1)*dy) / length_sq;
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            double closest_x = x1 + t * dx;
            double closest_y = y1 + t * dy;
            double dist_sq = (closest_x-cx)*(closest_x-cx) + (closest_y-cy)*(closest_y-cy);
            if (dist_sq <= r2) return true;
        }
        return false;
    }

    // --- get_nodes_between: nodes whose y is strictly between y_start and y_end ---
    std::vector<Point> get_nodes_between(double y_start, double y_end) const {
        std::vector<Point> result;
        for (auto& kv : node_pos_map) {
            double py = kv.second.second;
            if (py > y_start && py < y_end) {
                result.push_back(kv.second);
            }
        }
        return result;
    }

    // --- calculate_node_positions ---
    void calculate_node_positions() {
        int node_radius = params.node_radius;
        int node_interval_x = params.node_interval_x;
        int node_interval_y = params.node_interval_y;

        int y = node_radius;

        // Find max nodes at any level
        int max_nodes = 0;
        for (auto& kv : all_nodes) {
            int cnt = static_cast<int>(kv.second.size());
            if (cnt > max_nodes) max_nodes = cnt;
        }
        if (max_nodes < 2) max_nodes = 2;

        node_x = (2 * node_radius + 1) * max_nodes + node_interval_x * (max_nodes + 1);

        // Iterate levels from root_level down to 1
        for (int level = root_level; level >= 1; --level) {
            auto it = all_nodes.find(level);
            if (params.skip_unused_levels && it == all_nodes.end()) {
                continue;  // skip levels with no nodes
            }

            level_y_map[level] = y;

            if (it != all_nodes.end()) {
                const std::vector<size_t>& level_nodes = it->second;
                int num_nodes = static_cast<int>(level_nodes.size());
                int x = node_x / (num_nodes + 1) - (node_radius + node_interval_x);

                for (int j = 0; j < num_nodes; ++j) {
                    node_pos_map[level_nodes[j]] = Point(static_cast<double>(x),
                                                          static_cast<double>(y));
                    x += node_x / (num_nodes + 1);
                }
            }

            y += 2 * node_radius + node_interval_y;
        }

        // Terminal y
        y += params.terminal_y / 2 - node_radius;
        level_y_map[0] = y;

        // Delegate lane calculation
        lane_manager.calculate_lanes(all_nodes, node_pos_map, node_x);
    }

    // --- calculate_terminal_positions ---
    void calculate_terminal_positions() {
        int terminal_x = params.terminal_x;
        int node_interval_x = params.node_interval_x;
        int y = level_y_map[0];

        int num_terms = (draw_zero && has_false) ? 2 : 1;
        int tx = node_x / (num_terms + 1) - (terminal_x / 2 + node_interval_x);

        if (draw_zero && has_false) {
            node_pos_map[TERM_0_IDX()] = Point(static_cast<double>(tx),
                                              static_cast<double>(y));
            tx += node_x / (num_terms + 1);
        }

        if (has_true) {
            node_pos_map[TERM_1_IDX()] = Point(static_cast<double>(tx),
                                              static_cast<double>(y));
        }
    }

    // --- calculate_arc_info ---
    void calculate_arc_info() {
        dest_info.clear();
        for (auto it = all_nodes.rbegin(); it != all_nodes.rend(); ++it) {
            for (size_t idx : it->second) {
                const SvgNode& sn = nodes[idx];
                // lo child
                dest_info[sn.lo].push_back(std::make_pair(idx, 0));
                // hi child
                dest_info[sn.hi].push_back(std::make_pair(idx, 1));
            }
        }
    }

    // Helper to get the index for a child bddp in node_pos_map
    size_t child_pos_idx(bddp child) const {
        if (child == bddfalse) return TERM_0_IDX();
        if (child == bddtrue)  return TERM_1_IDX();
        if (child & BDD_CONST_FLAG) return TERM_1_IDX(); // other constants -> treat as true
        // Non-terminal: find node index
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].key == child) return i;
        }
        return TERM_1_IDX(); // fallback
    }

    int child_level(bddp child) const {
        if (child & BDD_CONST_FLAG) return 0;
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].key == child) return nodes[i].level;
        }
        return 0;
    }

    // --- calculate_all_end_points ---
    void calculate_all_end_points() {
        int node_radius = params.node_radius;

        for (auto& kv : dest_info) {
            bddp child = kv.first;
            bool child_is_false = (child == bddfalse);
            if (!draw_zero && child_is_false) continue;

            std::vector<std::pair<size_t,int> >& arcs = kv.second;

            // Sort arcs by parent x coordinate
            std::sort(arcs.begin(), arcs.end(),
                [&](const std::pair<size_t,int>& a, const std::pair<size_t,int>& b) {
                    return node_pos_map[a.first].first < node_pos_map[b.first].first;
                });

            // Initial angle
            double rad = 2.0 / 3.0 * M_PI;
            if (arcs.size() == 1) {
                rad = 0.5 * M_PI; // single connection: from top
            }

            size_t child_idx = child_pos_idx(child);

            for (size_t i = 0; i < arcs.size(); ++i) {
                size_t parent_idx = arcs[i].first;
                int arc_type = arcs[i].second;

                ArcKey arc_key = std::make_pair(parent_idx, arc_type);

                // End point
                Point child_pos = node_pos_map[child_idx];
                int pos_end_x, pos_end_y;

                if (child & BDD_CONST_FLAG) {
                    // Terminal: connect to top center of rectangle
                    pos_end_x = static_cast<int>(child_pos.first);
                    pos_end_y = static_cast<int>(child_pos.second) - params.terminal_y / 2;
                } else {
                    pos_end_x = circle_pos_x(static_cast<int>(child_pos.first), node_radius, rad);
                    pos_end_y = circle_pos_y(static_cast<int>(child_pos.second), node_radius, rad);
                }

                arc_end_point_map[arc_key] = Point(pos_end_x, pos_end_y);

                // Update angle for next arc
                if (arcs.size() >= 2) {
                    rad -= M_PI / 3.0 / (static_cast<double>(arcs.size()) - 1.0);
                }

                // Start point
                Point start = calculate_arc_start_point(parent_idx, arc_type);
                double pos_start_x = start.first;
                double pos_start_y = start.second;

                // Adjust start if arc crosses to wrong side
                if ((arc_type == 0 && pos_start_x < pos_end_x) ||
                    (arc_type == 1 && pos_start_x > pos_end_x)) {
                    pos_start_y += node_radius / 2;
                }
                arc_adjusted_start_point_map[arc_key] = Point(pos_start_x, pos_start_y);

                // Straightness check
                std::vector<Point> between = get_nodes_between(pos_start_y, pos_end_y);
                if (segment_intersects_any_circle(
                        Point(pos_start_x, pos_start_y),
                        Point(pos_end_x, pos_end_y),
                        node_radius, between)) {
                    arc_is_straight_map[arc_key] = false;
                } else {
                    arc_is_straight_map[arc_key] = true;
                }
            }
        }
    }

    // --- calculate_arc_start_point ---
    Point calculate_arc_start_point(size_t parent_idx, int arc_type) const {
        int node_radius = params.node_radius;
        Point parent_pos = node_pos_map.at(parent_idx);

        double parent_rad;
        if (arc_type == 0) {
            parent_rad = 4.0 / 3.0 * M_PI; // left-bottom (240 degrees)
        } else {
            parent_rad = 5.0 / 3.0 * M_PI; // right-bottom (300 degrees)
        }

        int px = circle_pos_x(static_cast<int>(parent_pos.first), node_radius, parent_rad);
        int py = circle_pos_y(static_cast<int>(parent_pos.second), node_radius, parent_rad);
        return Point(px, py);
    }

    // --- calculate_arc_connection_points ---
    void calculate_arc_connection_points() {
        lane_manager.initialize_lane_usage();
        lane_manager.determine_arc_lanes(dest_info, node_pos_map, nodes, arc_is_straight_map, draw_zero);
        calculate_node_connection_angles();
    }

    // --- calculate_node_connection_angles ---
    void calculate_node_connection_angles() {
        int node_radius = params.node_radius;

        for (auto& kv : dest_info) {
            bddp child = kv.first;
            bool child_is_false = (child == bddfalse);
            bool child_is_terminal = (child & BDD_CONST_FLAG) != 0;
            if (!draw_zero && child_is_false) continue;

            std::vector<std::pair<size_t,int> >& arcs = kv.second;

            // Sort arcs by parent x
            std::sort(arcs.begin(), arcs.end(),
                [&](const std::pair<size_t,int>& a, const std::pair<size_t,int>& b) {
                    return node_pos_map[a.first].first < node_pos_map[b.first].first;
                });

            int c_level = child_level(child);

            for (size_t i = 0; i < arcs.size(); ++i) {
                size_t parent_idx = arcs[i].first;
                int arc_type = arcs[i].second;
                int parent_level = nodes[parent_idx].level;

                ArcKey arc_key = std::make_pair(parent_idx, arc_type);

                // Start point
                Point start = calculate_arc_start_point(parent_idx, arc_type);
                double pos_start_x = start.first;
                double pos_start_y = start.second;

                std::vector<Point> arc_pos;
                arc_pos.push_back(Point(pos_start_x, pos_start_y));

                Point end_pt = arc_end_point_map[arc_key];
                double pos_end_x = end_pt.first;
                double pos_end_y = end_pt.second;

                // Adjust start if arc crosses
                if ((arc_type == 0 && pos_start_x < pos_end_x) ||
                    (arc_type == 1 && pos_start_x > pos_end_x)) {
                    arc_pos.push_back(Point(pos_start_x, pos_start_y + node_radius / 2));
                }

                if (!arc_is_straight_map[arc_key]) {
                    if (child_is_terminal && parent_level - 1 > c_level) {
                        handle_terminal_connections(arc_pos, child, parent_idx, arc_type,
                                                    parent_level, c_level);
                    } else {
                        handle_normal_connections(arc_pos, parent_idx, child, arc_type,
                                                  parent_level, c_level, pos_end_x);
                    }
                }

                // Append end point
                arc_pos.push_back(Point(pos_end_x, pos_end_y));

                arc_pos_map[arc_key] = arc_pos;
            }
        }
    }

    // --- handle_terminal_connections ---
    void handle_terminal_connections(
        std::vector<Point>& arc_pos, bddp child,
        size_t parent_idx, int arc_type,
        int parent_level, int child_level_val)
    {
        int node_radius = params.node_radius;
        int node_interval_y = params.node_interval_y;
        int arc_interval = params.arc_interval;

        bool is_term0 = (child == bddfalse);
        ArcKey arc_key = std::make_pair(parent_idx, arc_type);

        int target_x;
        if (is_term0) {
            int lane_x = lane_manager.get_leftmost_lane(parent_level - 1);
            target_x = lane_x;
            auto it = lane_manager.leftmost_lane_to_x.find(lane_x);
            if (it != lane_manager.leftmost_lane_to_x.end()) {
                // Walk backwards to find our arc_key; offset for earlier entries
                for (int j = static_cast<int>(it->second.size()) - 1; j >= 0; --j) {
                    if (it->second[j].arc_key == arc_key) break;
                    target_x -= arc_interval;
                }
            }
        } else {
            int lane_x = lane_manager.get_rightmost_lane(parent_level - 1);
            target_x = lane_x;
            auto it = lane_manager.rightmost_lane_to_x.find(lane_x);
            if (it != lane_manager.rightmost_lane_to_x.end()) {
                for (size_t j = 0; j < it->second.size(); ++j) {
                    if (it->second[j].arc_key == arc_key) break;
                    target_x += arc_interval;
                }
            }
        }

        int next_level = lane_manager.find_valid_level(parent_level, child_level_val, level_y_map);
        int y_horizontal = level_y_map[next_level] - (node_radius + node_interval_y / 2);
        arc_pos.push_back(Point(target_x, y_horizontal));

        int y_vertical = level_y_map[child_level_val] - (node_radius * 2);
        arc_pos.push_back(Point(target_x, y_vertical));
    }

    // --- handle_normal_connections ---
    void handle_normal_connections(
        std::vector<Point>& arc_pos,
        size_t parent_idx, bddp /* child */, int arc_type,
        int parent_level, int child_level_val, double /* pos_end_x */)
    {
        int node_radius = params.node_radius;
        int node_interval_y = params.node_interval_y;

        if (parent_level - 1 > child_level_val) {
            ArcKey arc_key = std::make_pair(parent_idx, arc_type);
            auto it = lane_manager.arc_lanes.find(arc_key);
            if (it != lane_manager.arc_lanes.end()) {
                for (size_t li = 0; li < it->second.size(); ++li) {
                    int lev = std::get<0>(it->second[li]);
                    int lane_x = std::get<1>(it->second[li]);

                    int adjusted_x = lane_manager.calculate_lane_offset(arc_key, lane_x, lev);

                    arc_pos.push_back(Point(adjusted_x,
                        level_y_map[lev] - (node_radius + node_interval_y / 2)));
                    // Find lev-1 in level_y_map; if not present, use closest
                    auto lym = level_y_map.find(lev - 1);
                    int y_below;
                    if (lym != level_y_map.end()) {
                        y_below = lym->second;
                    } else {
                        y_below = level_y_map[lev]; // fallback
                    }
                    arc_pos.push_back(Point(adjusted_x,
                        y_below - (node_radius + node_interval_y / 2)));
                }
            }
        }
    }
};

// ---- SvgWriter (port of mysvg.py) ----------------------------------------

class SvgWriter {
public:
    int margin_x;
    int margin_y;
    std::vector<std::string> defs;
    std::vector<std::string> elements;
    double min_x, min_y, max_x, max_y;

    SvgWriter(int mx, int my)
        : margin_x(mx), margin_y(my),
          min_x(1e18), min_y(1e18), max_x(-1e18), max_y(-1e18)
    {
        add_arrow_marker();
    }

    void add_arrow_marker() {
        defs.push_back(
            "<marker id=\"arrow\" viewBox=\"-10 -4 20 8\" "
            "markerWidth=\"10\" markerHeight=\"10\" orient=\"auto\">\n"
            "        <polygon points=\"-10,-4 0,0 -10,4\" "
            "fill=\"#1b3966\" stroke=\"none\" />\n"
            "    </marker>");
    }

    // Raw mode markers
    void add_raw_markers() {
        // Filled circle (normal edge)
        defs.push_back(
            "<marker id=\"dot\" viewBox=\"-4 -4 8 8\" "
            "markerWidth=\"8\" markerHeight=\"8\" orient=\"auto\" refX=\"0\" refY=\"0\">\n"
            "        <circle cx=\"0\" cy=\"0\" r=\"3\" "
            "fill=\"#1b3966\" stroke=\"none\" />\n"
            "    </marker>");
        // Open circle (complement edge)
        defs.push_back(
            "<marker id=\"odot\" viewBox=\"-4 -4 8 8\" "
            "markerWidth=\"8\" markerHeight=\"8\" orient=\"auto\" refX=\"0\" refY=\"0\">\n"
            "        <circle cx=\"0\" cy=\"0\" r=\"3\" "
            "fill=\"white\" stroke=\"#1b3966\" stroke-width=\"1.5\" />\n"
            "    </marker>");
    }

    void update_bounds(double x, double y) {
        if (x < min_x) min_x = x;
        if (y < min_y) min_y = y;
        if (x > max_x) max_x = x;
        if (y > max_y) max_y = y;
    }

    // --- emit_circle ---
    void emit_circle(double cx, double cy, int r,
                     const char* fill, const char* stroke, int stroke_width)
    {
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "<circle cx=\"%s\" cy=\"%s\" r=\"%d\" "
            "fill=\"%s\" stroke=\"%s\" stroke-width=\"%d\" />",
            fmt_double(cx).c_str(), fmt_double(cy).c_str(), r,
            fill, stroke, stroke_width);
        elements.push_back(std::string(buf));
        double total_r = r + stroke_width / 2.0;
        update_bounds(cx - total_r, cy - total_r);
        update_bounds(cx + total_r, cy + total_r);
    }

    // --- emit_rect (x,y = top-left corner) ---
    void emit_rect(double x, double y, int w, int h,
                   const char* fill, const char* stroke, int stroke_width)
    {
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "<rect x=\"%s\" y=\"%s\" width=\"%d\" height=\"%d\" "
            "fill=\"%s\" stroke=\"%s\" stroke-width=\"%d\" />",
            fmt_double(x).c_str(), fmt_double(y).c_str(), w, h,
            fill, stroke, stroke_width);
        elements.push_back(std::string(buf));
        double hw = stroke_width / 2.0;
        update_bounds(x - hw, y - hw);
        update_bounds(x + w + hw, y + h + hw);
    }

    // --- emit_text ---
    void emit_text(double x, double y, const std::string& text,
                   int font_size, const char* text_anchor, const char* fill = "black")
    {
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "<text x=\"%s\" y=\"%s\" font-size=\"%d\" "
            "text-anchor=\"%s\" fill=\"%s\">%s</text>",
            fmt_double(x).c_str(), fmt_double(y).c_str(), font_size,
            text_anchor, fill, text.c_str());
        elements.push_back(std::string(buf));
        double tw_est = text.size() * font_size * 0.6;
        if (std::string(text_anchor) == "middle") {
            update_bounds(x - tw_est / 2, y - font_size);
            update_bounds(x + tw_est / 2, y);
        } else {
            update_bounds(x, y - font_size);
            update_bounds(x + tw_est, y);
        }
    }

    // --- emit_smooth_path ---
    // Implements draw_smooth_line from mysvg.py:
    // M -> L (interpolated) -> Q (bezier) -> L -> ... -> L (end)
    void emit_smooth_path(const std::vector<Point>& points,
                          const char* stroke, int stroke_width,
                          const char* dash, const char* marker_end,
                          const char* marker_start = NULL)
    {
        if (points.size() < 2) return;

        std::string d;

        // M start
        d += "M " + fmt_double(points[0].first) + "," + fmt_double(points[0].second);

        if (points.size() == 2) {
            d += " L " + fmt_double(points[1].first) + "," + fmt_double(points[1].second);
        } else {
            // First segment interpolation
            double div_r = get_dividing_ratio(points[0], points[1]);
            double cur_x = points[0].first * div_r + points[1].first * (1.0 - div_r);
            double cur_y = points[0].second * div_r + points[1].second * (1.0 - div_r);
            d += " L " + fmt_double(cur_x) + "," + fmt_double(cur_y);

            for (size_t i = 1; i + 1 < points.size(); ++i) {
                double dr = get_dividing_ratio(points[i], points[i + 1]);
                double dest_x = points[i].first * (1.0 - dr) + points[i + 1].first * dr;
                double dest_y = points[i].second * (1.0 - dr) + points[i + 1].second * dr;

                d += " Q " + fmt_double(points[i].first) + "," +
                     fmt_double(points[i].second) + " " +
                     fmt_double(dest_x) + "," + fmt_double(dest_y);

                cur_x = points[i].first * dr + points[i + 1].first * (1.0 - dr);
                cur_y = points[i].second * dr + points[i + 1].second * (1.0 - dr);
                d += " L " + fmt_double(cur_x) + "," + fmt_double(cur_y);
            }

            d += " L " + fmt_double(points.back().first) + "," +
                 fmt_double(points.back().second);
        }

        // Build <path> element
        std::string elem = "<path d=\"" + d + "\" fill=\"none\" stroke=\"";
        elem += stroke;
        elem += "\" stroke-width=\"";
        {
            char sw[16];
            std::snprintf(sw, sizeof(sw), "%d", stroke_width);
            elem += sw;
        }
        elem += "\"";
        if (dash && dash[0]) {
            elem += " stroke-dasharray=\"";
            elem += dash;
            elem += "\"";
        }
        if (marker_end && marker_end[0]) {
            elem += " marker-end=\"";
            elem += marker_end;
            elem += "\"";
        }
        if (marker_start && marker_start[0]) {
            elem += " marker-start=\"";
            elem += marker_start;
            elem += "\"";
        }
        elem += " />";
        elements.push_back(elem);

        // Update bounds for all points
        double hw = stroke_width / 2.0;
        for (size_t i = 0; i < points.size(); ++i) {
            update_bounds(points[i].first - hw, points[i].second - hw);
            update_bounds(points[i].first + hw, points[i].second + hw);
        }
    }

    // --- align_to_origin and generate ---
    std::string to_string() const {
        if (min_x > 1e17) {
            // No elements
            return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\">\n</svg>";
        }

        // Calculate viewBox after align_to_origin
        double vx = min_x - margin_x;
        double vy = min_y - margin_y;
        double vw = max_x - min_x + 2 * margin_x;
        double vh = max_y - min_y + 2 * margin_y;
        int w = static_cast<int>(vw);
        int h = static_cast<int>(vh);
        if (w < 10) w = 10;
        if (h < 10) h = 10;

        std::string result;
        result += "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"";
        result += std::to_string(w);
        result += "\" height=\"";
        result += std::to_string(h);
        result += "\" viewBox=\"";
        result += fmt_double(vx) + " " + fmt_double(vy) + " " +
                  fmt_double(vw) + " " + fmt_double(vh);
        result += "\">\n";

        if (!defs.empty()) {
            result += "<defs>\n";
            for (size_t i = 0; i < defs.size(); ++i) {
                result += "    " + defs[i] + "\n";
            }
            result += "</defs>\n";
        }

        for (size_t i = 0; i < elements.size(); ++i) {
            result += "    " + elements[i] + "\n";
        }

        result += "</svg>";
        return result;
    }

    // No-op: viewBox-based rendering handles coordinate range automatically.
    // Kept for API compatibility.
    void align_to_origin() {}

private:
    static double get_dividing_ratio(const Point& p1, const Point& p2) {
        double dx = p2.first - p1.first;
        double dy = p2.second - p1.second;
        double r = std::sqrt(dx * dx + dy * dy);
        if (r == 0.0) return 0.0;
        double ratio = 30.0 / r;
        if (ratio > 0.25) ratio = 0.25;
        return ratio;
    }
};

// ---- Terminal-only SVG ---------------------------------------------------

static std::string make_terminal_svg(bddp f, const SvgParams& params) {
    int tx = params.terminal_x;
    int ty = params.terminal_y;
    int mx = params.margin_x;
    int my = params.margin_y;

    SvgWriter svg(mx, my);

    svg.emit_rect(mx, my, tx, ty,
                  params.node_fill, params.node_stroke, params.arc_width);

    const char* label = (f == bddtrue) ? "1" : "0";
    svg.emit_text(mx + tx / 2.0, my + ty / 2.0 + params.label_y,
                  label, params.font_size, "middle");

    return svg.to_string();
}

// ---- Get label for a node ------------------------------------------------
static std::string node_label(bddvar var, const SvgParams& params) {
    auto it = params.var_name_map.find(var);
    if (it != params.var_name_map.end()) {
        return it->second;
    }
    return std::to_string(static_cast<unsigned>(var));
}

// ---- Main SVG generation (common for all DD types) -----------------------

enum DdType {
    DD_BDD,
    DD_ZDD,
    DD_QDD,
    DD_UNREDUCED
};

static std::string svg_generate_impl(
    bddp f, const SvgParams& params, DdType dd_type)
{
    if (f == bddnull) return "";

    // Terminal-only case
    if (f & BDD_CONST_FLAG) {
        return make_terminal_svg(f, params);
    }

    bool is_expanded = (params.mode == DrawMode::Expanded) && (dd_type != DD_UNREDUCED);
    bool is_zdd = (dd_type == DD_ZDD);

    // ---- 1. Collect nodes ----
    std::vector<SvgNode> nodes;
    std::unordered_map<bddp, size_t> id_map;
    bool has_false = false, has_true = false;

    if (is_expanded) {
        if (is_zdd) {
            svg_collect_expanded(f, ZddChildResolver(), nodes, id_map, has_false, has_true);
        } else {
            svg_collect_expanded(f, BddChildResolver(), nodes, id_map, has_false, has_true);
        }
    } else {
        svg_collect_raw(f, nodes, id_map, has_false, has_true);
    }

    if (nodes.empty()) {
        // Only terminals reachable
        return make_terminal_svg(f, params);
    }

    // ---- 2. Compute positions ----
    SvgPositionManager pos_mgr(params, nodes, has_false, has_true);
    pos_mgr.compute_all_positions();

    // ---- 3. Draw SVG ----
    SvgWriter svg(params.margin_x, params.margin_y);

    // Add raw mode markers if needed
    bool raw_mode = !is_expanded;
    if (raw_mode) {
        svg.add_raw_markers();
    }

    // Draw internal nodes
    for (auto it = pos_mgr.all_nodes.begin(); it != pos_mgr.all_nodes.end(); ++it) {
        for (size_t idx : it->second) {
            const SvgNode& sn = nodes[idx];
            Point pos = pos_mgr.node_pos_map[idx];

            // Draw circle
            svg.emit_circle(pos.first, pos.second, params.node_radius,
                            params.node_fill, params.node_stroke, params.arc_width);

            // Draw label (variable number)
            std::string label = node_label(sn.var, params);
            svg.emit_text(pos.first, pos.second + params.label_y,
                          label, params.font_size, "middle");

            // Draw lo edge (0-edge)
            ArcKey lo_key = std::make_pair(idx, 0);
            if (pos_mgr.arc_pos_map.count(lo_key)) {
                const char* dash = "10,5"; // dashed
                const char* m_start = NULL;
                if (raw_mode && sn.lo_comp) {
                    m_start = "url(#odot)";
                } else if (raw_mode) {
                    m_start = "url(#dot)";
                }
                svg.emit_smooth_path(pos_mgr.arc_pos_map[lo_key],
                    params.node_stroke, params.arc_width,
                    dash, "url(#arrow)", m_start);
            }

            // Draw hi edge (1-edge)
            ArcKey hi_key = std::make_pair(idx, 1);
            if (pos_mgr.arc_pos_map.count(hi_key)) {
                const char* m_start = NULL;
                if (raw_mode && sn.hi_comp) {
                    m_start = "url(#odot)";
                } else if (raw_mode) {
                    m_start = "url(#dot)";
                }
                svg.emit_smooth_path(pos_mgr.arc_pos_map[hi_key],
                    params.node_stroke, params.arc_width,
                    NULL, "url(#arrow)", m_start);
            }
        }
    }

    // Draw terminal nodes
    auto draw_terminal = [&](size_t term_idx, const char* label) {
        if (pos_mgr.node_pos_map.count(term_idx) == 0) return;
        Point tpos = pos_mgr.node_pos_map[term_idx];
        int tx = params.terminal_x;
        int ty = params.terminal_y;

        svg.emit_rect(tpos.first - tx / 2.0, tpos.second - ty / 2.0,
                      tx, ty, params.node_fill, params.node_stroke, params.arc_width);
        svg.emit_text(tpos.first, tpos.second + params.label_y,
                      label, params.font_size, "middle");
    };

    if (params.draw_zero && has_false) {
        draw_terminal(SvgPositionManager::TERM_0_IDX(), "0");
    }
    if (has_true) {
        draw_terminal(SvgPositionManager::TERM_1_IDX(), "1");
    }

    // Raw mode: draw complement root entry marker if root is complemented
    if (raw_mode && (f & BDD_COMP_FLAG)) {
        // Draw a small "entry" arrow pointing to root node with complement marker
        bddp root_raw = f & ~BDD_COMP_FLAG;
        auto root_it = id_map.find(root_raw);
        if (root_it != id_map.end()) {
            size_t root_idx = root_it->second;
            if (pos_mgr.node_pos_map.count(root_idx)) {
                Point rpos = pos_mgr.node_pos_map[root_idx];
                // Draw entry point above root
                double ex = rpos.first;
                double ey = rpos.second - params.node_radius - 20;
                double ey2 = rpos.second - params.node_radius;
                std::vector<Point> entry_pts;
                entry_pts.push_back(Point(ex, ey));
                entry_pts.push_back(Point(ex, ey2));
                svg.emit_smooth_path(entry_pts, params.node_stroke, params.arc_width,
                                     NULL, "url(#arrow)", "url(#odot)");
            }
        }
    }

    svg.align_to_origin();
    return svg.to_string();
}

} // anonymous namespace

// ========================================================================
//  Public API implementations
// ========================================================================

// --- BDD SVG export ---

static std::string bdd_svg_generate(bddp f, const SvgParams& params) {
    return svg_generate_impl(f, params, DD_BDD);
}

void bdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    std::string svg = bdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("bdd_save_svg: cannot open ") + filename);
    ofs << svg;
}

void bdd_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << bdd_svg_generate(f, params);
}

std::string bdd_save_svg(bddp f, const SvgParams& params) {
    return bdd_svg_generate(f, params);
}

// --- ZDD SVG export ---

static std::string zdd_svg_generate(bddp f, const SvgParams& params) {
    return svg_generate_impl(f, params, DD_ZDD);
}

void zdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    std::string svg = zdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("zdd_save_svg: cannot open ") + filename);
    ofs << svg;
}

void zdd_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << zdd_svg_generate(f, params);
}

std::string zdd_save_svg(bddp f, const SvgParams& params) {
    return zdd_svg_generate(f, params);
}

// --- QDD SVG export ---

static std::string qdd_svg_generate(bddp f, const SvgParams& params) {
    return svg_generate_impl(f, params, DD_QDD);
}

void qdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    std::string svg = qdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("qdd_save_svg: cannot open ") + filename);
    ofs << svg;
}

void qdd_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << qdd_svg_generate(f, params);
}

std::string qdd_save_svg(bddp f, const SvgParams& params) {
    return qdd_svg_generate(f, params);
}

// --- UnreducedDD SVG export ---

static std::string unreduced_svg_generate(bddp f, const SvgParams& params) {
    // UnreducedDD always uses Raw mode, regardless of params.mode
    SvgParams raw_params = params;
    raw_params.mode = DrawMode::Raw;
    return svg_generate_impl(f, raw_params, DD_UNREDUCED);
}

void unreduced_save_svg(const char* filename, bddp f, const SvgParams& params) {
    std::string svg = unreduced_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("unreduced_save_svg: cannot open ") + filename);
    ofs << svg;
}

void unreduced_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << unreduced_svg_generate(f, params);
}

std::string unreduced_save_svg(bddp f, const SvgParams& params) {
    return unreduced_svg_generate(f, params);
}
