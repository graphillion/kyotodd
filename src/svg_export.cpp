// SVG export engine for KyotoDD BDD/ZDD/QDD/UnreducedDD.
// Faithfully ports the Python drawing pipeline (dd_draw/) to C++11.
//
// Layout helpers live in src/svg_export_internal.h.
// MTBDD/MTZDD live in src/svg_export_mt.cpp.
// MVBDD/MVZDD live in src/svg_export_mvdd.cpp.

#include "svg_export_internal.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace kyotodd {

using namespace svg_internal;

// ---- Main SVG generation (common for BDD/ZDD/QDD/UnreducedDD) ------------

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

// ========================================================================
//  Public API implementations for BDD/ZDD/QDD/UnreducedDD
// ========================================================================

// --- BDD SVG export ---

static std::string bdd_svg_generate(bddp f, const SvgParams& params) {
    return svg_generate_impl(f, params, DD_BDD);
}

void bdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    if (!filename)
        throw std::runtime_error("bdd_save_svg: filename is null");
    std::string svg = bdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("bdd_save_svg: cannot open ") + filename);
    ofs << svg;
    if (!ofs)
        throw std::runtime_error(std::string("bdd_save_svg: write failed for ") + filename);
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
    if (!filename)
        throw std::runtime_error("zdd_save_svg: filename is null");
    std::string svg = zdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("zdd_save_svg: cannot open ") + filename);
    ofs << svg;
    if (!ofs)
        throw std::runtime_error(std::string("zdd_save_svg: write failed for ") + filename);
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
    if (!filename)
        throw std::runtime_error("qdd_save_svg: filename is null");
    std::string svg = qdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("qdd_save_svg: cannot open ") + filename);
    ofs << svg;
    if (!ofs)
        throw std::runtime_error(std::string("qdd_save_svg: write failed for ") + filename);
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
    if (!filename)
        throw std::runtime_error("unreduced_save_svg: filename is null");
    std::string svg = unreduced_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("unreduced_save_svg: cannot open ") + filename);
    ofs << svg;
    if (!ofs)
        throw std::runtime_error(std::string("unreduced_save_svg: write failed for ") + filename);
}

void unreduced_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << unreduced_svg_generate(f, params);
}

std::string unreduced_save_svg(bddp f, const SvgParams& params) {
    return unreduced_svg_generate(f, params);
}

} // namespace kyotodd
