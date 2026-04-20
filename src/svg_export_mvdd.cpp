// MVBDD/MVZDD SVG export (multi-valued, k-way branching).
// Shared layout helpers live in src/svg_export_internal.h.

#include "svg_export_internal.h"

#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace kyotodd {

using namespace svg_internal;

// Default color palette for k-way edges.
static const char* mv_default_colors[] = {
    "#1b3966", // 0: dark blue
    "#c0392b", // 1: red
    "#27ae60", // 2: green
    "#8e44ad", // 3: purple
    "#d35400", // 4: orange
    "#2980b9", // 5: light blue
    "#f39c12", // 6: yellow
    "#1abc9c", // 7: teal
};
static const int mv_default_colors_count = 8;

static const char* mv_edge_color(int value, const SvgParams& params) {
    if (!params.edge_colors.empty()) {
        int idx = value % static_cast<int>(params.edge_colors.size());
        return params.edge_colors[idx].c_str();
    }
    return mv_default_colors[value % mv_default_colors_count];
}

// MV-level node structure.
struct MvSvgNode {
    bddp key;                    // internal bddp (identity key)
    bddvar mv_var;               // MV variable number
    std::vector<bddp> children;  // k children (value 0..k-1)
};

// Extract child of MVBDD at logical level (replicates MVBDD::child() on raw bddp).
static bddp mvbdd_extract_child(bddp f, const MVDDVarTable* table,
                                 bddvar mv, int value) {
    int k = table->k();
    const std::vector<bddvar>& dvars = table->dd_vars_of(mv);

    if (value == 0) {
        for (int i = k - 2; i >= 0; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = BDD::child0(f);
        }
    } else {
        for (int i = k - 2; i >= value; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = BDD::child0(f);
        }
        if (!(f & BDD_CONST_FLAG) && node_var(f) == dvars[value - 1]) {
            f = BDD::child1(f);
        }
        for (int i = value - 2; i >= 0; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = BDD::child0(f);
        }
    }
    return f;
}

// Extract child of MVZDD at logical level (replicates MVZDD::child() on raw bddp).
static bddp mvzdd_extract_child(bddp f, const MVDDVarTable* table,
                                  bddvar mv, int value) {
    int k = table->k();
    const std::vector<bddvar>& dvars = table->dd_vars_of(mv);

    if (value == 0) {
        for (int i = k - 2; i >= 0; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = ZDD::child0(f);
        }
    } else {
        for (int i = k - 2; i >= value; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = ZDD::child0(f);
        }
        if (!(f & BDD_CONST_FLAG) && node_var(f) == dvars[value - 1]) {
            f = ZDD::child1(f);
        } else {
            f = bddempty;
        }
    }
    return f;
}

// Determine MV variable from bddp top variable.
static bddvar mvdd_top_var(bddp f, const MVDDVarTable* table) {
    if (f & BDD_CONST_FLAG) return 0;
    bddvar dv = node_var(f);
    return table->mvdd_var_of(dv);
}

// Collect MV-level nodes by DFS.
static void mvdd_svg_collect(
    bddp f, const MVDDVarTable* table, bool is_zdd,
    std::vector<MvSvgNode>& nodes,
    std::unordered_map<bddp, size_t>& id_map,
    bool& has_false, bool& has_true)
{
    int k = table->k();
    std::vector<bddp> stack;
    stack.push_back(f);

    while (!stack.empty()) {
        bddp p = stack.back();
        stack.pop_back();

        if (p & BDD_CONST_FLAG) {
            if (p == bddfalse || p == bddempty) has_false = true;
            else has_true = true;
            continue;
        }

        if (id_map.count(p)) continue;

        bddvar mv = mvdd_top_var(p, table);
        if (mv == 0) continue;

        size_t idx = nodes.size();
        id_map[p] = idx;

        MvSvgNode sn;
        sn.key = p;
        sn.mv_var = mv;
        sn.children.resize(k);
        for (int v = 0; v < k; ++v) {
            bddp child;
            if (is_zdd) {
                child = mvzdd_extract_child(p, table, mv, v);
            } else {
                child = mvbdd_extract_child(p, table, mv, v);
            }
            sn.children[v] = child;
        }
        nodes.push_back(sn);

        // Push children (reverse order for DFS)
        for (int v = k - 1; v >= 0; --v) {
            bddp child = nodes[idx].children[v];
            if (!(child & BDD_CONST_FLAG) && !id_map.count(child)) {
                stack.push_back(child);
            } else if (child & BDD_CONST_FLAG) {
                if (child == bddfalse || child == bddempty) has_false = true;
                else has_true = true;
            }
        }
    }
}

// Main SVG generation for MVBDD/MVZDD expanded mode (k-way branching).
static std::string mvdd_svg_generate_expanded(
    bddp f, const MVDDVarTable* table, bool is_zdd,
    const SvgParams& params)
{
    if (!table)
        throw std::runtime_error("mvdd_svg_generate_expanded: var_table is null");
    if (f == bddnull) return "";

    // Terminal-only case
    if (f & BDD_CONST_FLAG) {
        return make_terminal_svg(f, params);
    }

    int k = table->k();

    // 1. Collect MV nodes
    std::vector<MvSvgNode> nodes;
    std::unordered_map<bddp, size_t> id_map;
    bool has_false = false, has_true = false;
    mvdd_svg_collect(f, table, is_zdd, nodes, id_map, has_false, has_true);

    if (nodes.empty()) {
        return make_terminal_svg(f, params);
    }

    // 2. Assign MV levels.
    // MV variables are ordered by their DD level (higher DD level = higher MV level).
    // Collect all MV vars from the table (or only used ones if skip_unused_levels).
    std::set<bddvar> used_mv_vars_set;
    for (auto& sn : nodes) {
        used_mv_vars_set.insert(sn.mv_var);
    }

    std::vector<bddvar> mv_vars;
    if (params.skip_unused_levels) {
        // Only include MV vars that appear in the DD
        mv_vars.assign(used_mv_vars_set.begin(), used_mv_vars_set.end());
    } else {
        // Include all registered MV vars from the table
        bddvar total = table->mvdd_var_count();
        for (bddvar v = 1; v <= total; ++v) {
            mv_vars.push_back(v);
        }
    }

    // Sort MV vars by DD level of their top DD var (descending = root first)
    std::sort(mv_vars.begin(), mv_vars.end(), [&](bddvar a, bddvar b) {
        bddvar dv_a = table->get_top_dd_var(a);
        bddvar dv_b = table->get_top_dd_var(b);
        return bddlevofvar(dv_a) > bddlevofvar(dv_b);
    });

    // Assign MV level 1,2,... (from top)
    std::map<bddvar, int> mv_level_map;
    for (size_t i = 0; i < mv_vars.size(); ++i) {
        mv_level_map[mv_vars[i]] = static_cast<int>(i + 1);
    }

    // Group nodes by MV level
    std::map<int, std::vector<size_t> > all_nodes;
    for (size_t i = 0; i < nodes.size(); ++i) {
        int lev = mv_level_map[nodes[i].mv_var];
        all_nodes[lev].push_back(i);
    }

    int num_levels = static_cast<int>(mv_vars.size());

    // 3. Compute positions
    int node_radius = params.node_radius;
    int node_interval_x = params.node_interval_x;
    int node_interval_y = params.node_interval_y;
    int terminal_x = params.terminal_x;
    int terminal_y = params.terminal_y;

    // Max nodes at any level (including terminals)
    int max_nodes = 2; // at least 2 for terminals
    for (auto& kv : all_nodes) {
        int cnt = static_cast<int>(kv.second.size());
        if (cnt > max_nodes) max_nodes = cnt;
    }

    // Scale horizontal spacing for k-way
    int hscale = (k > 2) ? k / 2 + 1 : 1;
    int canvas_w = (2 * node_radius + 1) * max_nodes * hscale
                   + node_interval_x * (max_nodes + 1) * hscale;

    std::map<size_t, Point> pos_map;
    int y = node_radius;

    for (int lev = 1; lev <= num_levels; ++lev) {
        auto it = all_nodes.find(lev);
        if (it != all_nodes.end()) {
            const std::vector<size_t>& level_nodes = it->second;
            int num = static_cast<int>(level_nodes.size());
            int x = canvas_w / (num + 1) - (node_radius + node_interval_x);
            for (int j = 0; j < num; ++j) {
                pos_map[level_nodes[j]] = Point(static_cast<double>(x),
                                                static_cast<double>(y));
                x += canvas_w / (num + 1);
            }
        }
        y += 2 * node_radius + node_interval_y;
    }

    // Terminal positions
    y += terminal_y / 2 - node_radius;
    bool draw_zero = params.draw_zero;
    int num_terms = (draw_zero && has_false) ? 2 : (has_true ? 1 : 0);
    if (!has_true && has_false && draw_zero) num_terms = 1;
    if (has_true && has_false && draw_zero) num_terms = 2;
    if (has_true && !has_false) num_terms = 1;
    if (!has_true && !has_false) num_terms = 0;
    if (has_false && !draw_zero) {
        num_terms = has_true ? 1 : 0;
    }

    size_t TERM_0 = SIZE_MAX;
    size_t TERM_1 = SIZE_MAX - 1;

    if (num_terms > 0) {
        int tx = canvas_w / (num_terms + 1) - (terminal_x / 2 + node_interval_x);
        if (draw_zero && has_false) {
            pos_map[TERM_0] = Point(static_cast<double>(tx), static_cast<double>(y));
            tx += canvas_w / (num_terms + 1);
        }
        if (has_true) {
            pos_map[TERM_1] = Point(static_cast<double>(tx), static_cast<double>(y));
        }
    }

    // Helper: get child position
    auto get_child_pos = [&](bddp child) -> Point {
        if (child & BDD_CONST_FLAG) {
            if ((child == bddfalse || child == bddempty) && pos_map.count(TERM_0))
                return pos_map[TERM_0];
            if (pos_map.count(TERM_1))
                return pos_map[TERM_1];
            return Point(0, 0);
        }
        auto nit = id_map.find(child);
        if (nit != id_map.end() && pos_map.count(nit->second))
            return pos_map[nit->second];
        return Point(0, 0);
    };

    auto is_zero_terminal = [](bddp p) -> bool {
        return p == bddfalse || p == bddempty;
    };

    // 4. Draw SVG
    SvgWriter svg(params.margin_x, params.margin_y);

    int r = params.node_radius;

    // Draw internal nodes
    for (int lev = 1; lev <= num_levels; ++lev) {
        auto it = all_nodes.find(lev);
        if (it == all_nodes.end()) continue;
        for (size_t idx : it->second) {
            const MvSvgNode& sn = nodes[idx];
            Point pos = pos_map[idx];

            // Draw circle
            svg.emit_circle(pos.first, pos.second, r,
                            params.node_fill, params.node_stroke, params.arc_width);

            // Label: MV variable number or name from var_name_map
            std::string label;
            auto name_it = params.var_name_map.find(sn.mv_var);
            if (name_it != params.var_name_map.end()) {
                label = name_it->second;
            } else {
                label = std::to_string(static_cast<unsigned>(sn.mv_var));
            }
            svg.emit_text(pos.first, pos.second + params.label_y,
                          label, params.font_size, "middle");

            // Draw k edges
            for (int v = 0; v < k; ++v) {
                bddp child = sn.children[v];

                // Skip hidden zero terminals
                if ((child & BDD_CONST_FLAG) && is_zero_terminal(child) && !draw_zero)
                    continue;

                Point cp = get_child_pos(child);
                double dx = cp.first - pos.first;
                double dy = cp.second - pos.second;
                double dist = std::sqrt(dx * dx + dy * dy);
                if (dist < 1.0) continue;

                // Offset parallel edges: spread departure points
                double base_angle = std::atan2(dy, dx);
                double spread = 0.0;
                if (k > 1) {
                    double range = 0.6; // radians spread
                    spread = -range / 2.0 + v * range / (k - 1);
                }
                double dep_angle = base_angle + spread;
                double sx = pos.first + r * std::cos(dep_angle);
                double sy = pos.second + r * std::sin(dep_angle);

                double ex, ey;
                if (child & BDD_CONST_FLAG) {
                    ex = cp.first;
                    ey = cp.second - terminal_y / 2.0;
                } else {
                    double arr_angle = std::atan2(-dy, -dx);
                    ex = cp.first + r * std::cos(arr_angle);
                    ey = cp.second + r * std::sin(arr_angle);
                }

                std::vector<Point> pts;
                pts.push_back(Point(sx, sy));
                pts.push_back(Point(ex, ey));

                const char* color = mv_edge_color(v, params);
                const char* dash = NULL;
                // MVZDD: value 0 is dashed
                if (is_zdd && v == 0) {
                    dash = "10,5";
                }

                svg.emit_smooth_path(pts, color, params.arc_width,
                                     dash, "url(#arrow)", NULL);

                // Edge labels
                if (params.draw_edge_labels) {
                    double lx = sx + (ex - sx) * 0.25;
                    double ly = sy + (ey - sy) * 0.25;
                    // Offset label to the side
                    double nx = -(ey - sy);
                    double ny = (ex - sx);
                    double nd = std::sqrt(nx * nx + ny * ny);
                    if (nd > 1.0) {
                        lx += nx / nd * 12;
                        ly += ny / nd * 12;
                    }
                    svg.emit_text(lx, ly, std::to_string(v),
                                  params.font_size * 2 / 3, "middle", color);
                }
            }
        }
    }

    // Draw terminals
    auto draw_term = [&](size_t tidx, const char* label) {
        if (pos_map.count(tidx) == 0) return;
        Point tpos = pos_map[tidx];
        svg.emit_rect(tpos.first - terminal_x / 2.0, tpos.second - terminal_y / 2.0,
                      terminal_x, terminal_y,
                      params.node_fill, params.node_stroke, params.arc_width);
        svg.emit_text(tpos.first, tpos.second + params.label_y,
                      label, params.font_size, "middle");
    };
    if (draw_zero && has_false) draw_term(TERM_0, "0");
    if (has_true) draw_term(TERM_1, "1");

    svg.align_to_origin();
    return svg.to_string();
}

// ========================================================================
//  Public API implementations for MVBDD/MVZDD
// ========================================================================

// --- MVBDD SVG export ---

void mvbdd_save_svg(const char* filename, bddp f,
                    const MVDDVarTable* table, const SvgParams& params) {
    if (!filename)
        throw std::runtime_error("mvbdd_save_svg: filename is null");
    std::string svg = mvbdd_save_svg(f, table, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("mvbdd_save_svg: cannot open ") + filename);
    ofs << svg;
    if (!ofs)
        throw std::runtime_error(std::string("mvbdd_save_svg: write failed for ") + filename);
}

void mvbdd_save_svg(std::ostream& strm, bddp f,
                    const MVDDVarTable* table, const SvgParams& params) {
    strm << mvbdd_save_svg(f, table, params);
}

std::string mvbdd_save_svg(bddp f,
                           const MVDDVarTable* table, const SvgParams& params) {
    return mvdd_svg_generate_expanded(f, table, false, params);
}

// --- MVZDD SVG export ---

void mvzdd_save_svg(const char* filename, bddp f,
                    const MVDDVarTable* table, const SvgParams& params) {
    if (!filename)
        throw std::runtime_error("mvzdd_save_svg: filename is null");
    std::string svg = mvzdd_save_svg(f, table, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("mvzdd_save_svg: cannot open ") + filename);
    ofs << svg;
    if (!ofs)
        throw std::runtime_error(std::string("mvzdd_save_svg: write failed for ") + filename);
}

void mvzdd_save_svg(std::ostream& strm, bddp f,
                    const MVDDVarTable* table, const SvgParams& params) {
    strm << mvzdd_save_svg(f, table, params);
}

std::string mvzdd_save_svg(bddp f,
                           const MVDDVarTable* table, const SvgParams& params) {
    return mvdd_svg_generate_expanded(f, table, true, params);
}

} // namespace kyotodd
