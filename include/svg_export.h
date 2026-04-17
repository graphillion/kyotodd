#ifndef KYOTODD_SVG_EXPORT_H
#define KYOTODD_SVG_EXPORT_H

#include "bdd_types.h"
#include <iosfwd>
#include <string>
#include <map>
#include <vector>

namespace kyotodd {

class MVDDVarTable;

/**
 * @brief Parameters for SVG export of decision diagrams.
 *
 * All fields have default values matching the standard BDD visualization style.
 * Customize individual fields as needed before passing to save_svg().
 */
struct SvgParams {
    DrawMode mode = DrawMode::Expanded; ///< Drawing mode (Expanded or Raw).
    int node_radius = 20;       ///< Circle radius for internal nodes (px).
    int node_interval_x = 30;   ///< Horizontal spacing between nodes at same level.
    int node_interval_y = 40;   ///< Vertical spacing between levels.
    int terminal_x = 30;        ///< Terminal node rectangle width.
    int terminal_y = 40;        ///< Terminal node rectangle height.
    int margin_x = 20;          ///< SVG horizontal padding.
    int margin_y = 20;          ///< SVG vertical padding.
    int label_y = 7;            ///< Vertical offset for node label text.
    int arc_width = 3;          ///< Edge line stroke width.
    int arc_interval = 4;       ///< Horizontal offset between parallel edges on same lane.
    int arc_terminal = 20;      ///< Extra distance for terminal lane margins.
    bool draw_zero = true;      ///< Whether to draw the 0-terminal node.
    bool skip_unused_levels = false; ///< Skip levels that have no nodes (compact layout).
    const char* node_fill = "#deebf7";   ///< Node background fill color.
    const char* node_stroke = "#1b3966"; ///< Node border stroke color.
    int font_size = 24;         ///< Font size for node labels.
    /** Optional variable name map. If non-empty, var numbers are replaced by names. */
    std::map<bddvar, std::string> var_name_map;
    /** Terminal node labels for MTBDD/MTZDD. Maps terminal bddp to display string. */
    std::map<bddp, std::string> terminal_name_map;
    bool draw_edge_labels = false; ///< Show value labels on MVBDD/MVZDD edges.
    /** Color palette for k-way edges (MVBDD/MVZDD). Empty = use defaults. */
    std::vector<std::string> edge_colors;
};

// --- BDD SVG export ---

/** @brief Save BDD as SVG to a file. */
void bdd_save_svg(const char* filename, bddp f,
                  const SvgParams& params = SvgParams());
/** @brief Save BDD as SVG to an output stream. */
void bdd_save_svg(std::ostream& strm, bddp f,
                  const SvgParams& params = SvgParams());
/** @brief Export BDD as SVG and return the SVG string. */
std::string bdd_save_svg(bddp f, const SvgParams& params = SvgParams());

// --- ZDD SVG export ---

/** @brief Save ZDD as SVG to a file. */
void zdd_save_svg(const char* filename, bddp f,
                  const SvgParams& params = SvgParams());
/** @brief Save ZDD as SVG to an output stream. */
void zdd_save_svg(std::ostream& strm, bddp f,
                  const SvgParams& params = SvgParams());
/** @brief Export ZDD as SVG and return the SVG string. */
std::string zdd_save_svg(bddp f, const SvgParams& params = SvgParams());

// --- QDD SVG export ---

/** @brief Save QDD as SVG to a file. */
void qdd_save_svg(const char* filename, bddp f,
                  const SvgParams& params = SvgParams());
/** @brief Save QDD as SVG to an output stream. */
void qdd_save_svg(std::ostream& strm, bddp f,
                  const SvgParams& params = SvgParams());
/** @brief Export QDD as SVG and return the SVG string. */
std::string qdd_save_svg(bddp f, const SvgParams& params = SvgParams());

// --- UnreducedDD SVG export ---

/** @brief Save UnreducedDD as SVG to a file. */
void unreduced_save_svg(const char* filename, bddp f,
                        const SvgParams& params = SvgParams());
/** @brief Save UnreducedDD as SVG to an output stream. */
void unreduced_save_svg(std::ostream& strm, bddp f,
                        const SvgParams& params = SvgParams());
/** @brief Export UnreducedDD as SVG and return the SVG string. */
std::string unreduced_save_svg(bddp f, const SvgParams& params = SvgParams());

// --- MTBDD SVG export ---

/** @brief Save MTBDD as SVG to a file. Uses terminal_name_map for labels. */
void mtbdd_save_svg(const char* filename, bddp f,
                    const SvgParams& params = SvgParams());
/** @brief Save MTBDD as SVG to an output stream. */
void mtbdd_save_svg(std::ostream& strm, bddp f,
                    const SvgParams& params = SvgParams());
/** @brief Export MTBDD as SVG and return the SVG string. */
std::string mtbdd_save_svg(bddp f, const SvgParams& params = SvgParams());

// --- MTZDD SVG export ---

/** @brief Save MTZDD as SVG to a file. Uses terminal_name_map for labels. */
void mtzdd_save_svg(const char* filename, bddp f,
                    const SvgParams& params = SvgParams());
/** @brief Save MTZDD as SVG to an output stream. */
void mtzdd_save_svg(std::ostream& strm, bddp f,
                    const SvgParams& params = SvgParams());
/** @brief Export MTZDD as SVG and return the SVG string. */
std::string mtzdd_save_svg(bddp f, const SvgParams& params = SvgParams());

// --- MVBDD SVG export (expanded: k-way branching) ---

/** @brief Save MVBDD as SVG to a file. */
void mvbdd_save_svg(const char* filename, bddp f,
                    const MVDDVarTable* table,
                    const SvgParams& params = SvgParams());
/** @brief Save MVBDD as SVG to an output stream. */
void mvbdd_save_svg(std::ostream& strm, bddp f,
                    const MVDDVarTable* table,
                    const SvgParams& params = SvgParams());
/** @brief Export MVBDD as SVG and return the SVG string. */
std::string mvbdd_save_svg(bddp f,
                           const MVDDVarTable* table,
                           const SvgParams& params = SvgParams());

// --- MVZDD SVG export (expanded: k-way branching) ---

/** @brief Save MVZDD as SVG to a file. */
void mvzdd_save_svg(const char* filename, bddp f,
                    const MVDDVarTable* table,
                    const SvgParams& params = SvgParams());
/** @brief Save MVZDD as SVG to an output stream. */
void mvzdd_save_svg(std::ostream& strm, bddp f,
                    const MVDDVarTable* table,
                    const SvgParams& params = SvgParams());
/** @brief Export MVZDD as SVG and return the SVG string. */
std::string mvzdd_save_svg(bddp f,
                           const MVDDVarTable* table,
                           const SvgParams& params = SvgParams());

} // namespace kyotodd

#endif // KYOTODD_SVG_EXPORT_H
