/***************************************************************
 *  Coloring reconfiguration via MVZDD — core routines.          *
 *                                                               *
 *  Header-only so that both the CLI driver (main.cpp) and the   *
 *  Google Test suite (test/test_coloring_reconf.cpp) can use    *
 *  the same code.                                               *
 *                                                               *
 *  The reconfiguration model is single-vertex recoloring:       *
 *  one step recolors exactly one vertex, and every intermediate *
 *  coloring must stay proper.                                   *
 ***************************************************************/

#ifndef COLORING_RECONF_HPP
#define COLORING_RECONF_HPP

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "bdd.h"

namespace coloring_reconf {

using kyotodd::BDD;
using kyotodd::ZDD;
using kyotodd::MVBDD;
using kyotodd::MVZDD;
using kyotodd::bddvar;

// ============================================================
//  Instance
// ============================================================

/**
 * @brief One coloring-reconfiguration instance.
 *
 * Vertices are 1-indexed in the input file but colorings are stored
 * 0-indexed: start[v - 1] is the color of vertex v.
 */
struct Instance {
    int n;                                   /**< Number of vertices. */
    int k;                                   /**< Number of colors. */
    std::vector<std::pair<int, int> > edges; /**< Undirected edges (1-indexed). */
    std::vector<int> start;                  /**< Start coloring, size n. */
    std::vector<int> goal;                   /**< Goal coloring, size n. */

    Instance() : n(0), k(0) {}
};

// ============================================================
//  Input parsing
// ============================================================

/** @brief A line is skipped when blank or starting with 'c', 'C', '#', '%'. */
inline bool is_comment_or_blank(const std::string& line) {
    std::size_t i = line.find_first_not_of(" \t\r");
    if (i == std::string::npos) return true;
    char c = line[i];
    return c == 'c' || c == 'C' || c == '#' || c == '%';
}

/** @brief Read the next line that is neither blank nor a comment. */
inline bool next_data_line(std::istream& in, std::string& out) {
    std::string line;
    while (std::getline(in, line)) {
        if (is_comment_or_blank(line)) continue;
        out = line;
        return true;
    }
    return false;
}

/**
 * @brief Parse an "s <c1> ... <cn>" / "t <c1> ... <cn>" coloring line.
 *
 * @param line The whole line, including the leading marker.
 * @param n Expected number of colors to read.
 * @param k Color domain size; each value must be in [0, k-1].
 * @param marker Receives the lowercased marker character ('s' or 't').
 * @param colors Receives the n parsed colors.
 * @param err Stream for error messages.
 */
inline bool parse_coloring_line(const std::string& line, int n, int k,
                                char& marker, std::vector<int>& colors,
                                std::ostream& err) {
    std::istringstream ls(line);
    std::string tag;
    if (!(ls >> tag)) return false;
    if (tag.size() != 1) {
        err << "Error: expected a single-character marker, got \"" << tag
            << "\"." << std::endl;
        return false;
    }
    marker = static_cast<char>(tag[0] | 0x20);  // to lowercase
    if (marker != 's' && marker != 't') {
        err << "Error: expected marker 's' or 't', got '" << tag[0] << "'."
            << std::endl;
        return false;
    }
    colors.clear();
    colors.reserve(n);
    for (int i = 0; i < n; ++i) {
        int c;
        if (!(ls >> c)) {
            err << "Error: '" << marker << "' line has only " << i
                << " colors, expected " << n << "." << std::endl;
            return false;
        }
        if (c < 0 || c >= k) {
            err << "Error: '" << marker << "' line: color " << c
                << " for vertex " << (i + 1) << " is out of range [0, "
                << (k - 1) << "]." << std::endl;
            return false;
        }
        colors.push_back(c);
    }
    int extra;
    if (ls >> extra) {
        err << "Error: '" << marker << "' line has more than " << n
            << " colors." << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief Read an instance in the extended app/coloring text format.
 *
 * Format (comments and blank lines may appear anywhere):
 *
 *     n m k
 *     <u> <v>        x m
 *     s <c1> ... <cn>
 *     t <c1> ... <cn>
 *
 * Self-loops are reported and ignored. The 's' and 't' lines may appear
 * in either order.
 */
inline bool read_instance(std::istream& in, Instance& inst, std::ostream& err) {
    std::string line;
    if (!next_data_line(in, line)) {
        err << "Error: missing header line (expected \"n m k\")." << std::endl;
        return false;
    }
    int m = 0;
    {
        std::istringstream hs(line);
        if (!(hs >> inst.n >> m >> inst.k)) {
            err << "Error: failed to parse \"n m k\" from: " << line
                << std::endl;
            return false;
        }
    }
    if (inst.n <= 0) {
        err << "Error: n must be positive (got " << inst.n << ")." << std::endl;
        return false;
    }
    if (m < 0) {
        err << "Error: m must be non-negative (got " << m << ")." << std::endl;
        return false;
    }
    // k == 1 leaves a single candidate coloring, so no recoloring step is
    // ever possible; MVBDD also requires a domain size of at least 2.
    if (inst.k < 2) {
        err << "Error: k must be >= 2 for reconfiguration (got " << inst.k
            << ")." << std::endl;
        return false;
    }

    inst.edges.clear();
    inst.edges.reserve(m);
    for (int i = 0; i < m; ++i) {
        if (!next_data_line(in, line)) {
            err << "Error: expected " << m << " edge lines, got only " << i
                << "." << std::endl;
            return false;
        }
        std::istringstream es(line);
        int u, v;
        if (!(es >> u >> v)) {
            err << "Error: failed to parse edge line " << (i + 1) << "."
                << std::endl;
            return false;
        }
        if (u < 1 || u > inst.n || v < 1 || v > inst.n) {
            err << "Error: edge " << (i + 1) << " vertex out of range [1, "
                << inst.n << "]." << std::endl;
            return false;
        }
        if (u == v) {
            err << "Warning: edge " << (i + 1) << " is a self-loop on " << u
                << "; ignored." << std::endl;
            continue;
        }
        inst.edges.push_back(std::make_pair(u, v));
    }

    bool has_start = false, has_goal = false;
    for (int i = 0; i < 2; ++i) {
        if (!next_data_line(in, line)) {
            err << "Error: missing " << (has_start ? "'t'" : "'s'")
                << " line (start/goal coloring)." << std::endl;
            return false;
        }
        char marker = 0;
        std::vector<int> colors;
        if (!parse_coloring_line(line, inst.n, inst.k, marker, colors, err)) {
            return false;
        }
        if (marker == 's') {
            if (has_start) {
                err << "Error: duplicate 's' line." << std::endl;
                return false;
            }
            inst.start = colors;
            has_start = true;
        } else {
            if (has_goal) {
                err << "Error: duplicate 't' line." << std::endl;
                return false;
            }
            inst.goal = colors;
            has_goal = true;
        }
    }
    if (!has_start || !has_goal) {
        err << "Error: both an 's' line and a 't' line are required."
            << std::endl;
        return false;
    }
    return true;
}

/** @brief read_instance() from a file path. */
inline bool read_instance_file(const std::string& path, Instance& inst,
                               std::ostream& err) {
    std::ifstream in(path.c_str());
    if (!in) {
        err << "Error: cannot open file: " << path << std::endl;
        return false;
    }
    return read_instance(in, inst, err);
}

// ============================================================
//  Proper-coloring check (graph level, no DD involved)
// ============================================================

/** @brief True iff the assignment gives different colors to every edge. */
inline bool is_proper(const Instance& inst, const std::vector<int>& coloring) {
    if (static_cast<int>(coloring.size()) != inst.n) return false;
    for (std::size_t i = 0; i < inst.edges.size(); ++i) {
        int u = inst.edges[i].first;
        int v = inst.edges[i].second;
        if (coloring[u - 1] == coloring[v - 1]) return false;
    }
    return true;
}

// ============================================================
//  Solution space
// ============================================================

/**
 * @brief "At most one of vars is true" as a plain BDD.
 *
 * Pairwise naive encoding, O(|vars|^2) clauses. Used to pin an MV
 * variable's don't-care DD bits to 0 so that each color corresponds to
 * exactly one bit pattern.
 */
inline BDD at_most_one_bdd(const std::vector<bddvar>& vars) {
    BDD result = BDD::True;
    for (std::size_t i = 0; i < vars.size(); ++i) {
        for (std::size_t j = i + 1; j < vars.size(); ++j) {
            result = result & ~(BDD::prime(vars[i]) & BDD::prime(vars[j]));
        }
    }
    return result;
}

/**
 * @brief Build the family of all proper k-colorings as an MVZDD.
 *
 * MVDD variable v (1-indexed) corresponds to vertex v, and its value is
 * the color of that vertex. The characteristic function is first built as
 * an MVBDD (same construction as app/coloring), then converted to a ZDD
 * over the one-hot DD variables and wrapped back as an MVZDD.
 *
 * PRECONDITION: the DD manager has just been initialized, i.e. the DD
 * variable pool is empty. The BDD -> ZDD conversion fills every DD level
 * up to the last variable of the table, so leftover variables from an
 * earlier construction would be turned into don't-cares.
 *
 * @param inst The instance (only n, k and edges are used).
 * @return The family of proper colorings. Its var_table() maps MVDD
 *         variable v to vertex v.
 */
inline MVZDD build_solution_space(const Instance& inst) {
    MVBDD base(inst.k);
    std::vector<bddvar> vars(inst.n + 1, 0);
    for (int v = 1; v <= inst.n; ++v) vars[v] = base.new_var();

    MVBDD f = MVBDD::one(base.var_table());

    // Canonical form: at most one of each MV variable's DD vars is 1.
    // MVBDD::singleton leaves the other bits as don't-care, which would
    // make one color match several DD assignments.
    for (int v = 1; v <= inst.n; ++v) {
        const std::vector<bddvar>& dvars = base.var_table()->dd_vars_of(vars[v]);
        f = f & MVBDD::from_bdd(base, at_most_one_bdd(dvars));
    }

    // Proper-coloring constraint: ~(OR_c (x_u == c & x_w == c)) per edge.
    for (std::size_t i = 0; i < inst.edges.size(); ++i) {
        int u = inst.edges[i].first;
        int w = inst.edges[i].second;
        MVBDD equal_uw = MVBDD::zero(base.var_table());
        for (int c = 0; c < inst.k; ++c) {
            equal_uw = equal_uw | (MVBDD::singleton(base, vars[u], c) &
                                   MVBDD::singleton(base, vars[w], c));
        }
        f = f & ~equal_uw;
    }

    // The one-hot DD variables of the last MV variable are the topmost
    // levels in use, so filling up to there covers exactly our variables.
    const std::vector<bddvar>& last_dvars =
        base.var_table()->dd_vars_of(vars[inst.n]);
    int top_dd_var = static_cast<int>(last_dvars[last_dvars.size() - 1]);

    ZDD zdd = f.to_bdd().to_zdd(top_dd_var);
    return MVZDD(base.var_table(), zdd);
}

}  // namespace coloring_reconf

#endif  // COLORING_RECONF_HPP
