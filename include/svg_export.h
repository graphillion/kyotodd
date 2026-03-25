#ifndef KYOTODD_SVG_EXPORT_H
#define KYOTODD_SVG_EXPORT_H

#include "bdd_types.h"
#include <iosfwd>
#include <string>
#include <map>

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
    const char* node_fill = "#deebf7";   ///< Node background fill color.
    const char* node_stroke = "#1b3966"; ///< Node border stroke color.
    int font_size = 24;         ///< Font size for node labels.
    /** Optional variable name map. If non-empty, var numbers are replaced by names. */
    std::map<bddvar, std::string> var_name_map;
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

#endif // KYOTODD_SVG_EXPORT_H
