/***************************************************************
 *  TdZdd specs with KyotoDD: UnreducedDD::construct and          *
 *  ZDD::subset / UnreducedDD::zdd_subset.                       *
 *                                                               *
 *  A TdZdd "spec" describes a decision diagram top-down: a      *
 *  state at each level and a transition for value 0 / 1.        *
 *  KyotoDD can build such a spec directly into its own node     *
 *  table (UnreducedDD::construct) and can restrict an existing  *
 *  ZDD to the sets a spec accepts (ZDD::subset, TdZdd's         *
 *  zddSubset) without ever materializing the spec's ZDD.        *
 *                                                               *
 *  Example problem: n items with weights w_1..w_n.              *
 *    1. WeightBound: all subsets with total weight <= C.         *
 *       When the weight reaches C exactly, the spec jumps       *
 *       straight to the 1-terminal (all remaining items must    *
 *       be left out). Such level-skipping edges mean "0" in a   *
 *       ZDD but "don't care" in a BDD; SkipPadding::Zdd makes   *
 *       the ZDD meaning explicit before reduce_as_bdd().        *
 *    2. NoAdjacent: no two consecutive items chosen.            *
 *       Applied to the ZDD from step 1 via ZDD::subset.         *
 *                                                               *
 *  Build: requires the TdZdd submodule (third_party/TdZdd).     *
 *  Usage: tdzdd_sample [n] [C]   (default n = 12, C = 20)       *
 ***************************************************************/

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "bdd.h"
#include "unreduced_dd_tdzdd.h"   // not part of bdd.h: include explicitly

using namespace kyotodd;

// TdZdd levels run from n (first decision) down to 1 (last decision).
// With KyotoDD's default variable order, TdZdd level i is variable i,
// so we simply treat "item i" as "level i".

// All subsets whose total weight is <= capacity. State: weight so far.
class WeightBound : public tdzdd::DdSpec<WeightBound, int, 2> {
    int n_;
    int capacity_;
    std::vector<int> w_;   // w_[i] = weight of item i (1-based)
public:
    WeightBound(const std::vector<int>& w, int capacity)
        : n_(static_cast<int>(w.size()) - 1), capacity_(capacity), w_(w) {}

    int getRoot(int& weight) const {
        weight = 0;
        return n_;                     // start at the top level
    }

    int getChild(int& weight, int level, int value) const {
        if (value) {
            weight += w_[level];
            if (weight > capacity_) return 0;      // 0-terminal: rejected
            if (weight == capacity_) return -1;    // 1-terminal: nothing more fits
        }
        return (level == 1) ? -1 : level - 1;      // next level, or accept
    }
    // hashCode / equalTo default to the raw bytes of the int state.
};

// Subsets with no two consecutive items. State: whether the item just
// above (level + 1) was chosen.
class NoAdjacent : public tdzdd::DdSpec<NoAdjacent, int, 2> {
    int n_;
public:
    explicit NoAdjacent(int n) : n_(n) {}
    int getRoot(int& prev) const { prev = 0; return n_; }
    int getChild(int& prev, int level, int value) const {
        if (value && prev) return 0;               // two in a row: rejected
        prev = value;
        return (level == 1) ? -1 : level - 1;
    }
};

static void print_family(const char* title, const ZDD& z, size_t limit) {
    std::printf("%s: %s sets\n", title, z.exact_count().to_string().c_str());
    std::vector<std::vector<bddvar> > sets = z.enumerate();
    for (size_t k = 0; k < sets.size() && k < limit; ++k) {
        std::printf("  {");
        for (size_t i = 0; i < sets[k].size(); ++i)
            std::printf("%s%u", i ? ", " : "", sets[k][i]);
        std::printf("}\n");
    }
    if (sets.size() > limit) std::printf("  ...\n");
}

int main(int argc, char** argv) {
    int n = (argc > 1) ? std::atoi(argv[1]) : 12;
    int capacity = (argc > 2) ? std::atoi(argv[2]) : 20;
    if (n < 1 || capacity < 0) {
        std::fprintf(stderr, "usage: %s [n >= 1] [C >= 0]\n", argv[0]);
        return 1;
    }

    BDD_Init(1 << 16, UINT64_MAX);
    // construct() creates the variables it needs; creating them up front
    // is only required for ZDD::power_set below.
    bddnewvar(n);

    std::vector<int> w(n + 1, 0);
    for (int i = 1; i <= n; ++i) w[i] = 1 + (i * 7) % 5;   // weights 1..5
    std::printf("n = %d, C = %d, weights:", n, capacity);
    for (int i = 1; i <= n; ++i) std::printf(" w%d=%d", i, w[i]);
    std::printf("\n\n");

    // ---- 1. UnreducedDD::construct ------------------------------------
    WeightBound bound(w, capacity);
    UnreducedDD u = UnreducedDD::construct(bound);
    std::printf("construct(WeightBound): %zu unreduced nodes\n", u.raw_size());

    ZDD light = u.reduce_as_zdd();
    print_family("subsets with weight <= C (ZDD)", light, 5);

    // The same family, built the classic way, for comparison.
    ZDD check = ZDD::power_set(static_cast<bddvar>(n));
    check = check.cost_bound_le(w, capacity);   // w is indexed by variable
    std::printf("matches power_set().cost_bound_le(): %s\n\n",
                light.id() == check.id() ? "yes" : "no");

    // ---- SkipPadding: what a level-skipping edge means -----------------
    // WeightBound jumps to the 1-terminal when the weight hits C. In the
    // ZDD reading the skipped items are 0 (reduce_as_zdd above is right).
    // reduce_as_bdd() would read them as don't-care instead; padding the
    // edges first (SkipPadding::Zdd) pins them to 0.
    UnreducedDD padded = UnreducedDD::construct(bound, SkipPadding::Zdd);
    BDD as_bdd_raw = u.reduce_as_bdd();
    BDD as_bdd_pad = padded.reduce_as_bdd();
    BDD expected = light.to_bdd(n);   // characteristic function of the family
    std::printf("reduce_as_bdd without padding == to_bdd(): %s\n",
                as_bdd_raw.id() == expected.id() ? "yes" : "no");
    std::printf("reduce_as_bdd with SkipPadding::Zdd == to_bdd(): %s\n\n",
                as_bdd_pad.id() == expected.id() ? "yes" : "no");

    // ---- 2. ZDD::subset ----------------------------------------------
    NoAdjacent no_adj(n);
    ZDD sparse = light.subset(no_adj);
    print_family("... and no two consecutive items (ZDD::subset)", sparse, 5);

    // subset() equals intersecting with the spec's own ZDD, but never
    // builds that ZDD.
    ZDD via_intersection = light & UnreducedDD::construct(no_adj).reduce_as_zdd();
    std::printf("matches light & construct(NoAdjacent): %s\n\n",
                sparse.id() == via_intersection.id() ? "yes" : "no");

    // UnreducedDD::zdd_subset is the same operation on an unreduced
    // input, returning an unreduced result (reduce it yourself).
    UnreducedDD sparse_u = u.zdd_subset(no_adj);
    std::printf("zdd_subset on the unreduced DD: %zu unreduced nodes, "
                "same ZDD after reduction: %s\n",
                sparse_u.raw_size(),
                sparse_u.reduce_as_zdd().id() == sparse.id() ? "yes" : "no");
    return 0;
}
