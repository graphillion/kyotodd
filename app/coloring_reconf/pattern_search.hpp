/***************************************************************
 *  Coloring reconfiguration — count-profile pattern search.    *
 *                                                              *
 *  Searches the reconfiguration graph (single-vertex           *
 *  recoloring over proper colorings) for a path whose          *
 *  USED-COLOR-COUNT profile, after collapsing runs of equal    *
 *  counts, equals a prescribed pattern such as                 *
 *      k-1, k, k-1, k-2, k-1, k, k+1, k, k-1, k-2, k-3.       *
 *                                                              *
 *  The search is a layered BFS over the product of the         *
 *  solution space with the pattern position ("phase"): a step  *
 *  either stays in the current phase (count unchanged) or      *
 *  advances to the next phase (count changes accordingly).     *
 *  All frontiers are MVZDD families, as in reconfigure().      *
 *                                                              *
 *  Header-only, shared by the CLI driver and the tests.        *
 ***************************************************************/

#ifndef COLORING_RECONF_PATTERN_SEARCH_HPP
#define COLORING_RECONF_PATTERN_SEARCH_HPP

#include <ostream>
#include <stdexcept>
#include <vector>

#include "bdd.h"
#include "color_count.hpp"
#include "coloring_reconf.hpp"

namespace coloring_reconf {

/**
 * @brief All colorings reachable from a seed family inside a space.
 *
 * BFS to the fixpoint: R_0 = seeds & space, and R grows by
 * (space & recolor_one(frontier)) until nothing new appears.  This is
 * the union of the connected components of the reconfiguration graph
 * of `space` that meet `seeds`.
 *
 * @param space The family of admissible colorings (the BFS never
 *              leaves it).  Pass a restricted space to model palette
 *              or used-color-count restrictions.
 * @param seeds Any family; only seeds & space is used.
 * @param log If not null, one progress line per BFS step.
 * @return The union of all reachable colorings (including the seeds).
 */
inline MVZDD reach_closure(const MVZDD& space, const MVZDD& seeds,
                           std::ostream* log = 0) {
    MVZDD reached = seeds & space;
    MVZDD frontier = reached;
    int step = 0;
    while (!frontier.is_zero()) {
        ++step;
        MVZDD next = (space & recolor_one(frontier)) - reached;
        reached = reached + next;
        if (log) {
            *log << "  closure step " << step << ": frontier = "
                 << next.exact_count() << ", total = "
                 << reached.exact_count() << std::endl;
        }
        frontier = next;
    }
    return reached;
}

/**
 * @brief Check that a count pattern is well-formed.
 *
 * Consecutive entries must differ by exactly 1 (a single-vertex
 * recoloring changes the number of used colors by at most 1, and equal
 * consecutive entries would not be a collapsed profile).
 *
 * @throws std::invalid_argument if the pattern is empty, has an entry
 *         outside [1, k], or has an invalid consecutive difference.
 */
inline void validate_pattern(const std::vector<int>& pattern, int k) {
    if (pattern.empty()) {
        throw std::invalid_argument("pattern must be non-empty");
    }
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] < 1 || pattern[i] > k) {
            throw std::invalid_argument(
                "pattern entry out of range [1, k]");
        }
        if (i > 0) {
            int d = pattern[i] - pattern[i - 1];
            if (d != 1 && d != -1) {
                throw std::invalid_argument(
                    "consecutive pattern entries must differ by exactly 1");
            }
        }
    }
}

/**
 * @brief Collapse runs of equal used-color counts along a sequence.
 *
 * @param sequence A sequence of colorings.
 * @return The count profile with consecutive duplicates removed, e.g.
 *         counts 4 4 5 5 5 4 give {4, 5, 4}.
 */
inline std::vector<int> collapse_count_profile(
    const std::vector<std::vector<int> >& sequence) {
    std::vector<int> profile;
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        int c = count_used_colors(sequence[i]);
        if (profile.empty() || profile.back() != c) profile.push_back(c);
    }
    return profile;
}

/**
 * @brief Layered BFS for a path realizing a used-color-count pattern.
 *
 * State = (coloring, phase); phase j means the collapsed profile so far
 * equals pattern[0..j].  A recoloring step keeps the phase when the
 * count stays pattern[j], and advances to phase j+1 when the count
 * becomes pattern[j+1]; any other count change is not allowed.  The
 * search succeeds when a goal coloring is reached in the last phase.
 *
 * @param space The family of all admissible colorings.
 * @param exact Per-count families from exact_color_count_families(space).
 * @param pattern The collapsed count profile to realize.
 * @param start Start coloring; its count must equal pattern[0].
 * @param goal_family Family of acceptable goal colorings (each relevant
 *        member should use exactly pattern.back() colors; others can
 *        never be reached in the last phase and are ignored).
 * @param layers Receives one entry per BFS layer; layers[i][j] is the
 *        family of colorings first reached at step i in phase j.
 *        Never null.
 * @param log If not null, one progress line per step.
 * @return The number of steps of a shortest pattern-realizing path to
 *         some member of goal_family, or -1 if none exists.
 */
inline int pattern_reconfigure(const MVZDD& space,
                               const std::vector<MVZDD>& exact,
                               const std::vector<int>& pattern,
                               const std::vector<int>& start,
                               const MVZDD& goal_family,
                               std::vector<std::vector<MVZDD> >* layers,
                               std::ostream* log = 0) {
    std::shared_ptr<MVDDVarTable> table = space.var_table();
    const int k = table->k();
    validate_pattern(pattern, k);
    const int np = (int)pattern.size();
    const MVZDD empty = MVZDD::zero(table);

    layers->clear();
    if (count_used_colors(start) != pattern[0]) return -1;
    MVZDD start_f = MVZDD::from_sets(
        empty, std::vector<std::vector<int> >(1, start));
    if ((start_f & space).is_zero()) return -1;

    std::vector<MVZDD> visited(np, empty);
    std::vector<MVZDD> front(np, empty);
    front[0] = start_f;
    visited[0] = start_f;
    layers->push_back(front);
    if (np == 1 && !(start_f & goal_family).is_zero()) return 0;

    for (int step = 1; ; ++step) {
        // moved[j] = one-step neighborhood of the previous frontier of
        // phase j, clipped to the space.
        std::vector<MVZDD> moved(np, empty);
        for (int j = 0; j < np; ++j) {
            if (!front[j].is_zero()) {
                moved[j] = space & recolor_one(front[j]);
            }
        }
        std::vector<MVZDD> next(np, empty);
        bool any = false;
        for (int j = 0; j < np; ++j) {
            MVZDD in = moved[j];
            if (j > 0) in = in + moved[j - 1];
            MVZDD nf = (exact[pattern[j]] & in) - visited[j];
            if (!nf.is_zero()) any = true;
            visited[j] = visited[j] + nf;
            next[j] = nf;
        }
        layers->push_back(next);
        if (log) {
            *log << "  pattern step " << step << ":";
            for (int j = 0; j < np; ++j) {
                if (!next[j].is_zero()) {
                    *log << " |phase" << j << "|=" << next[j].exact_count();
                }
            }
            *log << std::endl;
        }
        if (!(next[np - 1] & goal_family).is_zero()) return step;
        if (!any) return -1;
        front = next;
    }
}

/**
 * @brief Reconstruct one pattern-realizing sequence from BFS layers.
 *
 * Walks backwards from the goal like backtrack(): the predecessor of a
 * phase-j coloring at layer i is any single-vertex recoloring found in
 * layers[i-1][j] (count unchanged) or layers[i-1][j-1] (the phase
 * advance).  The goal must be a member of the last layer's last phase.
 *
 * @param start Start coloring (phase 0, layer 0).
 * @param goal Goal coloring, in layers.back()[pattern.size()-1].
 * @param layers Layers produced by pattern_reconfigure().
 * @param sequence Receives layers.size() colorings, start first.
 * @return false if no predecessor was found at some layer (should not
 *         happen for layers produced by pattern_reconfigure()).
 */
inline bool backtrack_pattern(const std::vector<int>& start,
                              const std::vector<int>& goal,
                              const std::vector<std::vector<MVZDD> >& layers,
                              std::vector<std::vector<int> >* sequence) {
    if (layers.empty() || layers[0].empty()) return false;
    const int k = layers[0][0].var_table()->k();
    const int np = (int)layers[0].size();
    const int last = (int)layers.size() - 1;

    sequence->assign(layers.size(), std::vector<int>());
    (*sequence)[last] = goal;
    int phase = np - 1;

    for (int i = last - 1; i >= 1; --i) {
        const std::vector<int>& cur = (*sequence)[i + 1];
        std::vector<int> cand = cur;
        bool found = false;
        for (std::size_t v = 0; v < cur.size() && !found; ++v) {
            const int orig = cand[v];
            for (int c = 0; c < k && !found; ++c) {
                if (c == orig) continue;
                cand[v] = c;
                if (layers[i][phase].contains(cand)) {
                    (*sequence)[i] = cand;
                    found = true;
                } else if (phase > 0 &&
                           layers[i][phase - 1].contains(cand)) {
                    (*sequence)[i] = cand;
                    --phase;
                    found = true;
                }
            }
            cand[v] = orig;
        }
        if (!found) return false;
    }
    (*sequence)[0] = start;
    return true;
}

/**
 * @brief Check a sequence against an instance and a count pattern.
 *
 * Verifies the reconfiguration rules (proper colorings, one vertex per
 * step, endpoints) via verify_sequence(), and that the collapsed
 * used-color-count profile equals the pattern.
 *
 * @param inst The instance; inst.start / inst.goal are the endpoints.
 * @param sequence The sequence to check.
 * @param pattern The expected collapsed count profile.
 * @param err If not null, the reason for a rejection is written here.
 */
inline bool verify_pattern_sequence(
    const Instance& inst, const std::vector<std::vector<int> >& sequence,
    const std::vector<int>& pattern, std::ostream* err) {
    if (!verify_sequence(inst, sequence, err)) return false;
    std::vector<int> profile = collapse_count_profile(sequence);
    if (profile != pattern) {
        if (err) {
            *err << "collapsed count profile {";
            for (std::size_t i = 0; i < profile.size(); ++i) {
                *err << (i ? "," : "") << profile[i];
            }
            *err << "} does not match the pattern" << std::endl;
        }
        return false;
    }
    return true;
}

}  // namespace coloring_reconf

#endif  // COLORING_RECONF_PATTERN_SEARCH_HPP
