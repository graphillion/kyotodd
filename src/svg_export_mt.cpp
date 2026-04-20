// MTBDD/MTZDD SVG export (multi-terminal, no complement edges).
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

// Collect nodes for MTBDD/MTZDD (no complement edge handling).
static void svg_collect_mt(
    bddp f,
    std::vector<SvgNode>& nodes,
    std::unordered_map<bddp, size_t>& id_map,
    std::set<bddp>& terminals)
{
    std::vector<std::pair<bddp, bool> > stack;
    stack.push_back(std::make_pair(f, false));
    while (!stack.empty()) {
        bddp p = stack.back().first;
        bool expanded = stack.back().second;

        if (p & BDD_CONST_FLAG) {
            stack.pop_back();
            terminals.insert(p);
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
            if (lo & BDD_CONST_FLAG) terminals.insert(lo);
            if (hi & BDD_CONST_FLAG) terminals.insert(hi);
            if (!(hi & BDD_CONST_FLAG) && !id_map.count(hi))
                stack.push_back(std::make_pair(hi, false));
            if (!(lo & BDD_CONST_FLAG) && !id_map.count(lo))
                stack.push_back(std::make_pair(lo, false));
        } else {
            stack.pop_back();
            bddp lo = node_lo(p);
            bddp hi = node_hi(p);

            size_t idx = nodes.size();
            id_map[p] = idx;
            SvgNode sn;
            sn.key     = p;
            sn.var     = node_var(p);
            sn.level   = static_cast<int>(bddlevofvar(sn.var));
            sn.lo      = lo;
            sn.hi      = hi;
            sn.lo_comp = false;
            sn.hi_comp = false;
            nodes.push_back(sn);
        }
    }
}

// Get terminal label from terminal_name_map or fall back to index.
static std::string mt_terminal_label(bddp t, const SvgParams& params) {
    auto it = params.terminal_name_map.find(t);
    if (it != params.terminal_name_map.end()) {
        return it->second;
    }
    // Fall back: show index
    uint64_t idx = t & ~BDD_CONST_FLAG;
    return std::to_string(idx);
}

// Terminal-only SVG for MTBDD/MTZDD.
static std::string make_mt_terminal_svg(bddp f, const SvgParams& params) {
    int tx = params.terminal_x;
    int ty = params.terminal_y;
    int mx = params.margin_x;
    int my = params.margin_y;

    SvgWriter svg(mx, my);

    // Adjust width for longer labels
    std::string label = mt_terminal_label(f, params);
    int label_width = static_cast<int>(label.size()) * params.font_size * 6 / 10;
    int rect_w = (label_width > tx) ? label_width + 10 : tx;

    svg.emit_rect(mx, my, rect_w, ty,
                  params.node_fill, params.node_stroke, params.arc_width);
    svg.emit_text(mx + rect_w / 2.0, my + ty / 2.0 + params.label_y,
                  label, params.font_size, "middle");

    return svg.to_string();
}

// Main SVG generation for MTBDD/MTZDD.
static std::string svg_generate_mt_impl(bddp f, const SvgParams& params) {
    if (f == bddnull) return "";

    // Terminal-only case
    if (f & BDD_CONST_FLAG) {
        return make_mt_terminal_svg(f, params);
    }

    // 1. Collect nodes
    std::vector<SvgNode> nodes;
    std::unordered_map<bddp, size_t> id_map;
    std::set<bddp> terminals;
    svg_collect_mt(f, nodes, id_map, terminals);

    if (nodes.empty()) {
        return make_mt_terminal_svg(f, params);
    }

    // Build terminal list (filtered by draw_zero, sorted for deterministic order)
    bddp zero_terminal = BDD_CONST_FLAG; // index 0
    std::vector<bddp> term_list;
    for (bddp t : terminals) {
        if (!params.draw_zero && t == zero_terminal) continue;
        term_list.push_back(t);
    }
    std::sort(term_list.begin(), term_list.end());

    // Map terminal bddp -> position index (SIZE_MAX - i)
    std::map<bddp, size_t> term_idx_map;
    for (size_t i = 0; i < term_list.size(); ++i) {
        term_idx_map[term_list[i]] = SIZE_MAX - i;
    }

    // 2. Compute positions directly (not using SvgPositionManager's private methods)
    int node_radius = params.node_radius;
    int node_interval_x = params.node_interval_x;
    int node_interval_y = params.node_interval_y;
    int terminal_x = params.terminal_x;

    // Group nodes by level
    std::map<int, std::vector<size_t> > all_nodes;
    int root_level = 0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        all_nodes[nodes[i].level].push_back(i);
        if (nodes[i].level > root_level) root_level = nodes[i].level;
    }

    // Compute max label width for terminals
    int max_label_chars = 1;
    for (bddp t : term_list) {
        std::string lbl = mt_terminal_label(t, params);
        if (static_cast<int>(lbl.size()) > max_label_chars)
            max_label_chars = static_cast<int>(lbl.size());
    }
    int term_rect_w = max_label_chars * params.font_size * 6 / 10;
    if (term_rect_w < terminal_x) term_rect_w = terminal_x;

    // Find max nodes at any level (including terminals)
    int max_nodes = static_cast<int>(term_list.size());
    for (auto& kv : all_nodes) {
        int cnt = static_cast<int>(kv.second.size());
        if (cnt > max_nodes) max_nodes = cnt;
    }
    if (max_nodes < 2) max_nodes = 2;

    int node_w = 2 * node_radius + 1;
    int term_w = term_rect_w + 1;
    int elem_w = (node_w > term_w) ? node_w : term_w;
    int canvas_w = elem_w * max_nodes + node_interval_x * (max_nodes + 1);

    // Position internal nodes
    std::map<size_t, Point> pos_map;
    int y = node_radius;
    for (int level = root_level; level >= 1; --level) {
        auto it = all_nodes.find(level);
        if (params.skip_unused_levels && it == all_nodes.end()) continue;
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

    // Position terminals
    y += params.terminal_y / 2 - node_radius;
    int num_terms = static_cast<int>(term_list.size());
    if (num_terms > 0) {
        int tx = canvas_w / (num_terms + 1) - (term_rect_w / 2 + node_interval_x);
        for (int i = 0; i < num_terms; ++i) {
            size_t tidx = term_idx_map[term_list[i]];
            pos_map[tidx] = Point(static_cast<double>(tx), static_cast<double>(y));
            tx += canvas_w / (num_terms + 1);
        }
    }

    // Helper: get position of a child
    auto get_child_pos = [&](bddp child) -> Point {
        if (child & BDD_CONST_FLAG) {
            auto tit = term_idx_map.find(child);
            if (tit != term_idx_map.end() && pos_map.count(tit->second)) {
                return pos_map[tit->second];
            }
            return Point(0, 0);
        } else {
            auto nit = id_map.find(child);
            if (nit != id_map.end() && pos_map.count(nit->second)) {
                return pos_map[nit->second];
            }
            return Point(0, 0);
        }
    };

    // 3. Draw SVG
    SvgWriter svg(params.margin_x, params.margin_y);

    int r = params.node_radius;

    // Draw internal nodes
    for (auto it = all_nodes.begin(); it != all_nodes.end(); ++it) {
        for (size_t idx : it->second) {
            const SvgNode& sn = nodes[idx];
            Point pos = pos_map[idx];

            svg.emit_circle(pos.first, pos.second, r,
                            params.node_fill, params.node_stroke, params.arc_width);

            std::string label = node_label(sn.var, params);
            svg.emit_text(pos.first, pos.second + params.label_y,
                          label, params.font_size, "middle");

            // Draw lo edge (dashed)
            if (!(sn.lo & BDD_CONST_FLAG) || params.draw_zero || sn.lo != zero_terminal) {
                Point cp = get_child_pos(sn.lo);
                double dx = cp.first - pos.first;
                double dy = cp.second - pos.second;
                double dist = std::sqrt(dx * dx + dy * dy);
                if (dist > 1.0) {
                    double sx = pos.first + r * dx / dist;
                    double sy = pos.second + r * dy / dist;
                    double ex, ey;
                    if (sn.lo & BDD_CONST_FLAG) {
                        ex = cp.first; ey = cp.second - params.terminal_y / 2.0;
                    } else {
                        ex = cp.first - r * dx / dist; ey = cp.second - r * dy / dist;
                    }
                    std::vector<Point> pts;
                    pts.push_back(Point(sx, sy));
                    pts.push_back(Point(ex, ey));
                    svg.emit_smooth_path(pts, params.node_stroke, params.arc_width,
                                         "10,5", "url(#arrow)", NULL);
                }
            }

            // Draw hi edge (solid)
            if (!(sn.hi & BDD_CONST_FLAG) || params.draw_zero || sn.hi != zero_terminal) {
                Point cp = get_child_pos(sn.hi);
                double dx = cp.first - pos.first;
                double dy = cp.second - pos.second;
                double dist = std::sqrt(dx * dx + dy * dy);
                if (dist > 1.0) {
                    double sx = pos.first + r * dx / dist;
                    double sy = pos.second + r * dy / dist;
                    double ex, ey;
                    if (sn.hi & BDD_CONST_FLAG) {
                        ex = cp.first; ey = cp.second - params.terminal_y / 2.0;
                    } else {
                        ex = cp.first - r * dx / dist; ey = cp.second - r * dy / dist;
                    }
                    std::vector<Point> pts;
                    pts.push_back(Point(sx, sy));
                    pts.push_back(Point(ex, ey));
                    svg.emit_smooth_path(pts, params.node_stroke, params.arc_width,
                                         NULL, "url(#arrow)", NULL);
                }
            }
        }
    }

    // Draw terminal nodes
    for (size_t i = 0; i < term_list.size(); ++i) {
        bddp t = term_list[i];
        size_t tidx = term_idx_map[t];
        if (pos_map.count(tidx) == 0) continue;
        Point tpos = pos_map[tidx];
        int ty = params.terminal_y;

        svg.emit_rect(tpos.first - term_rect_w / 2.0, tpos.second - ty / 2.0,
                      term_rect_w, ty, params.node_fill, params.node_stroke, params.arc_width);
        std::string label = mt_terminal_label(t, params);
        svg.emit_text(tpos.first, tpos.second + params.label_y,
                      label, params.font_size, "middle");
    }

    svg.align_to_origin();
    return svg.to_string();
}

// ========================================================================
//  Public API implementations for MTBDD/MTZDD
// ========================================================================

// --- MTBDD SVG export ---

void mtbdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    if (!filename)
        throw std::runtime_error("mtbdd_save_svg: filename is null");
    std::string svg = svg_generate_mt_impl(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("mtbdd_save_svg: cannot open ") + filename);
    ofs << svg;
    if (!ofs)
        throw std::runtime_error(std::string("mtbdd_save_svg: write failed for ") + filename);
}

void mtbdd_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << svg_generate_mt_impl(f, params);
}

std::string mtbdd_save_svg(bddp f, const SvgParams& params) {
    return svg_generate_mt_impl(f, params);
}

// --- MTZDD SVG export ---

void mtzdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    if (!filename)
        throw std::runtime_error("mtzdd_save_svg: filename is null");
    std::string svg = svg_generate_mt_impl(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("mtzdd_save_svg: cannot open ") + filename);
    ofs << svg;
    if (!ofs)
        throw std::runtime_error(std::string("mtzdd_save_svg: write failed for ") + filename);
}

void mtzdd_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << svg_generate_mt_impl(f, params);
}

std::string mtzdd_save_svg(bddp f, const SvgParams& params) {
    return svg_generate_mt_impl(f, params);
}

} // namespace kyotodd
