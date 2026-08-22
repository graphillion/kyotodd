/***************************************************************
 *  Coloring reconfiguration — count-profile pattern driver.    *
 *                                                              *
 *  Usage:                                                      *
 *    coloring_reconf_pattern <instance.txt> [c0 c1 ...]        *
 *        Search a path from s to t whose collapsed             *
 *        used-color-count profile equals the given pattern     *
 *        (default, with k = K-1 and K the palette size:        *
 *         k-1 k k-1 k-2 k-1 k k+1 k k-1 k-2 k-3), and check    *
 *        that t is NOT reachable from s when the palette is    *
 *        restricted to any k colors (fixed-palette check) or   *
 *        when the used-color count must stay <= k (capped      *
 *        check).                                               *
 *                                                              *
 *    coloring_reconf_pattern --analyze <graph.txt>             *
 *        graph.txt holds only "n m K" and the edge list.       *
 *        Reports, for each pair of dropped colors, the family  *
 *        of (k-1)-count colorings that cannot reach ANY        *
 *        (k-3)-count coloring under the restriction.           *
 ***************************************************************/

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "color_count.hpp"
#include "coloring_reconf.hpp"
#include "pattern_search.hpp"

using namespace coloring_reconf;
using kyotodd::bddinit;
using kyotodd::bddfinal;

static void print_coloring(const std::vector<int>& coloring) {
    for (std::size_t v = 0; v < coloring.size(); ++v) {
        std::printf("%sv%zu=%d", v ? ", " : "", v + 1, coloring[v]);
    }
}

/** @brief Read only "n m K" and the edge list (no s/t lines). */
static bool read_graph_file(const std::string& path, Instance& inst,
                            std::ostream& err) {
    std::ifstream in(path.c_str());
    if (!in) {
        err << "Error: cannot open file: " << path << std::endl;
        return false;
    }
    std::string line;
    if (!next_data_line(in, line)) {
        err << "Error: missing header line." << std::endl;
        return false;
    }
    int m = 0;
    {
        std::istringstream hs(line);
        if (!(hs >> inst.n >> m >> inst.k)) {
            err << "Error: failed to parse \"n m k\"." << std::endl;
            return false;
        }
    }
    for (int i = 0; i < m; ++i) {
        if (!next_data_line(in, line)) {
            err << "Error: expected " << m << " edges." << std::endl;
            return false;
        }
        std::istringstream es(line);
        int u, v;
        if (!(es >> u >> v) || u < 1 || u > inst.n || v < 1 || v > inst.n) {
            err << "Error: bad edge line " << (i + 1) << "." << std::endl;
            return false;
        }
        if (u != v) inst.edges.push_back(std::make_pair(u, v));
    }
    return true;
}

static std::vector<int> default_pattern(int k) {
    std::vector<int> p;
    p.push_back(k - 1); p.push_back(k);     p.push_back(k - 1);
    p.push_back(k - 2); p.push_back(k - 1); p.push_back(k);
    p.push_back(k + 1); p.push_back(k);     p.push_back(k - 1);
    p.push_back(k - 2); p.push_back(k - 3);
    return p;
}

// ------------------------------------------------------------
//  --analyze mode
// ------------------------------------------------------------
static int run_analyze(const Instance& inst) {
    const int K = inst.k;   // palette size
    const int k = K - 1;    // usable colors in the restricted problem
    if (k - 3 < 1) {
        std::fprintf(stderr, "Error: need K >= 5.\n");
        return 1;
    }

    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> exact = exact_color_count_families(space);
    std::printf("=== Solution space (palette of %d colors) ===\n", K);
    std::cout << "  #proper colorings: " << space.exact_count() << std::endl;
    for (int c = 1; c <= K; ++c) {
        std::cout << "  exactly " << c << " colors: " << exact[c].exact_count()
                  << std::endl;
    }
    if (exact[k - 3].is_zero()) {
        std::printf("  no (k-3)=%d-count colorings: chi(G) > %d, no use.\n",
                    k - 3, k - 3);
        return 0;
    }

    // Capped analysis: colorings using <= k colors, drift allowed.
    {
        MVZDD capped = at_most_color_count(exact, k);
        std::printf("\n=== Capped (used-color count <= %d) ===\n", k);
        MVZDD reach = reach_closure(capped, exact[k - 3], &std::cout);
        MVZDD trapped = exact[k - 1] - reach;
        std::cout << "  (k-1)-count colorings that cannot reach any (k-3)-count"
                  << " coloring: " << trapped.exact_count() << std::endl;
        for (int i = 0; i < 3 && i < 100; ++i) {
            if (bigint::BigInt(i) >= trapped.exact_count()) break;
            std::printf("    sample: ");
            print_coloring(trapped.unrank(i));
            std::printf("\n");
        }
    }

    // Fixed-palette analysis: drop each pair of colors.
    std::printf("\n=== Fixed palettes of %d colors ===\n", k);
    std::vector<MVZDD> trap_d;
    for (int d = 0; d < K; ++d) {
        MVZDD space_d = avoid_color(space, d);
        MVZDD reach = reach_closure(space_d, exact[k - 3] & space_d);
        trap_d.push_back((exact[k - 1] & space_d) - reach);
        std::cout << "  palette minus {" << d << "}: trapped (k-1)-count: "
                  << trap_d[d].exact_count() << std::endl;
    }
    for (int d1 = 0; d1 < K; ++d1) {
        for (int d2 = d1 + 1; d2 < K; ++d2) {
            MVZDD both = trap_d[d1] & trap_d[d2];
            if (both.is_zero()) continue;
            std::cout << "  trapped in both palettes minus {" << d1 << "} and"
                      << " minus {" << d2 << "}: " << both.exact_count()
                      << std::endl;
            std::printf("    sample: ");
            print_coloring(both.unrank(0));
            std::printf("\n");
        }
    }
    return 0;
}

// ------------------------------------------------------------
//  pattern mode
// ------------------------------------------------------------
static int run_pattern(const Instance& inst, const std::vector<int>& pattern) {
    const int K = inst.k;
    const int k = K - 1;
    if (!is_proper(inst, inst.start) || !is_proper(inst, inst.goal)) {
        std::fprintf(stderr, "Error: s or t is not a proper coloring.\n");
        return 1;
    }
    std::printf("pattern:");
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        std::printf(" %d", pattern[i]);
    }
    std::printf("\n");
    std::printf("count(s) = %d, count(t) = %d\n",
                count_used_colors(inst.start), count_used_colors(inst.goal));

    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> exact = exact_color_count_families(space);
    MVZDD empty = MVZDD::zero(space.var_table());
    MVZDD goal_f = MVZDD::from_sets(
        empty, std::vector<std::vector<int> >(1, inst.goal));

    // --- Restricted-reachability checks ---------------------------------
    unsigned used_mask = 0;
    for (int v = 0; v < inst.n; ++v) {
        used_mask |= 1u << inst.start[v];
        used_mask |= 1u << inst.goal[v];
    }
    std::printf("\n=== Fixed-palette checks (any %d of the %d colors) ===\n",
                k, K);
    bool all_unreachable = true;
    int relevant = 0;
    for (int d = 0; d < K; ++d) {
        if (used_mask & (1u << d)) continue;  // s or t uses color d
        ++relevant;
        MVZDD space_d = avoid_color(space, d);
        std::vector<MVZDD> layers;
        int steps = reconfigure(space_d, inst.start, inst.goal, &layers, 0);
        std::printf("  palette minus {%d}: %s\n", d,
                    steps < 0 ? "NOT reachable"
                              : "REACHABLE (condition violated)");
        if (steps >= 0) all_unreachable = false;
    }
    if (relevant == 0) {
        std::printf("  (no %d-color palette contains both s and t: vacuous)\n",
                    k);
        all_unreachable = false;
    }

    std::printf("\n=== Capped check (used-color count <= %d) ===\n", k);
    {
        MVZDD capped = at_most_color_count(exact, k);
        std::vector<MVZDD> layers;
        int steps = reconfigure(capped, inst.start, inst.goal, &layers, 0);
        std::printf("  %s\n", steps < 0 ? "NOT reachable"
                                        : "REACHABLE (weaker condition only)");
    }

    // --- Pattern path ---------------------------------------------------
    std::printf("\n=== Pattern-profile search (full palette of %d) ===\n", K);
    std::vector<std::vector<MVZDD> > layers;
    int steps = pattern_reconfigure(space, exact, pattern, inst.start, goal_f,
                                    &layers, &std::cout);
    if (steps < 0) {
        std::printf("  => NO sequence with this profile exists.\n");
        return 0;
    }
    std::printf("  => FOUND, %d steps.\n", steps);

    std::vector<std::vector<int> > seq;
    if (!backtrack_pattern(inst.start, inst.goal, layers, &seq)) {
        std::fprintf(stderr, "Error: backtrack failed.\n");
        return 1;
    }
    std::ostringstream err;
    if (!verify_pattern_sequence(inst, seq, pattern, &err)) {
        std::fprintf(stderr, "Error: invalid sequence: %s\n",
                     err.str().c_str());
        return 1;
    }
    std::printf("\n  Sequence (count in front, %s):\n",
                all_unreachable ? "s -/-> t under any k-color palette"
                                : "warning: restricted problem can reach t");
    for (std::size_t i = 0; i < seq.size(); ++i) {
        std::printf("  [%2zu] #colors=%d  ", i, count_used_colors(seq[i]));
        print_coloring(seq[i]);
        if (i > 0) {
            int v = changed_vertex(seq[i - 1], seq[i]);
            if (v >= 0) {
                std::printf("   (v%d: %d -> %d)", v + 1, seq[i - 1][v],
                            seq[i][v]);
            }
        }
        std::printf("\n");
    }
    return 0;
}

int main(int argc, char* argv[]) {
    bool analyze = false;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--analyze") analyze = true;
        else args.push_back(argv[i]);
    }
    if (args.empty()) {
        std::fprintf(stderr,
                     "Usage: coloring_reconf_pattern [--analyze] <file.txt> "
                     "[pattern...]\n");
        return 1;
    }

    Instance inst;
    if (analyze) {
        if (!read_graph_file(args[0], inst, std::cerr)) return 1;
    } else {
        if (!read_instance_file(args[0], inst, std::cerr)) return 1;
    }

    std::vector<int> pattern;
    if (args.size() > 1) {
        for (std::size_t i = 1; i < args.size(); ++i) {
            pattern.push_back(std::atoi(args[i].c_str()));
        }
    } else {
        pattern = default_pattern(inst.k - 1);
    }

    if (bddinit(1024)) {
        std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
        return 1;
    }
    int rc = analyze ? run_analyze(inst) : run_pattern(inst, pattern);
    bddfinal();
    return rc;
}
