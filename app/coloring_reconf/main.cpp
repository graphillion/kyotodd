/***************************************************************
 *  Coloring reconfiguration via MVZDD — CLI driver.             *
 *                                                               *
 *  Usage: coloring_reconf <file.txt>                            *
 *                                                               *
 *  See coloring_reconf.hpp for the input format and the core    *
 *  routines, and README.txt for the algorithm outline.          *
 ***************************************************************/

#include <cinttypes>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "coloring_reconf.hpp"

using namespace coloring_reconf;
using kyotodd::bddinit;
using kyotodd::bddfinal;

static void print_coloring(const std::vector<int>& coloring) {
    for (std::size_t v = 0; v < coloring.size(); ++v) {
        std::printf("%sv%zu=%d", v ? ", " : "", v + 1, coloring[v]);
    }
}

static void print_instance(const Instance& inst) {
    std::printf("Input instance:\n");
    std::printf("  n (vertices) = %d, edges = %d, k (colors) = %d\n", inst.n,
                (int)inst.edges.size(), inst.k);
    std::printf("  edge list    : ");
    for (std::size_t i = 0; i < inst.edges.size(); ++i) {
        std::printf("%s(%d,%d)", i ? " " : "", inst.edges[i].first,
                    inst.edges[i].second);
    }
    std::printf("\n");
    std::printf("  start (s)    : ");
    print_coloring(inst.start);
    std::printf("\n");
    std::printf("  goal  (t)    : ");
    print_coloring(inst.goal);
    std::printf("\n");
}

// All DD objects must be destroyed before bddfinal(), so the whole
// computation lives in this function rather than in main().
static int run(const Instance& inst) {
    if (!is_proper(inst, inst.start)) {
        std::fprintf(stderr,
                     "Error: the start coloring 's' is not a proper "
                     "coloring.\n");
        return 1;
    }
    if (!is_proper(inst, inst.goal)) {
        std::fprintf(stderr,
                     "Error: the goal coloring 't' is not a proper "
                     "coloring.\n");
        return 1;
    }

    MVZDD space = build_solution_space(inst);
    std::printf("\n=== Solution space (all proper %d-colorings) ===\n", inst.k);
    std::cout << "  #proper colorings    : " << space.exact_count() << std::endl;
    std::printf("  MVZDD nodes          : %" PRIu64 "\n",
                space.mvzdd_node_count());
    std::printf("  Underlying ZDD size  : %" PRIu64 "\n", space.size());

    std::printf("\n=== Reconfiguration BFS (single-vertex recoloring) ===\n");
    std::vector<MVZDD> layers;
    int steps = reconfigure(space, inst.start, inst.goal, &layers, &std::cout);
    if (steps < 0) {
        std::printf("  => NOT REACHABLE (the goal is in another component).\n");
        return 0;
    }
    std::printf("  => REACHABLE, shortest length = %d step%s.\n", steps,
                steps == 1 ? "" : "s");

    std::vector<std::vector<int> > sequence;
    if (!backtrack(inst.start, inst.goal, layers, &sequence)) {
        std::fprintf(stderr, "Error: backtrack failed.\n");
        return 1;
    }
    if (!verify_sequence(inst, sequence, &std::cerr)) {
        std::fprintf(stderr, "Error: the reconstructed sequence is invalid.\n");
        return 1;
    }

    std::printf("\n  Reconfiguration sequence (colors numbered 0..%d):\n",
                inst.k - 1);
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        std::printf("  [%zu] ", i);
        print_coloring(sequence[i]);
        if (i > 0) {
            int v = changed_vertex(sequence[i - 1], sequence[i]);
            if (v >= 0) {
                std::printf("   (recolor v%d: %d -> %d)", v + 1,
                            sequence[i - 1][v], sequence[i][v]);
            }
        }
        std::printf("\n");
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: coloring_reconf <file.txt>\n");
        return 1;
    }

    Instance inst;
    if (!read_instance_file(argv[1], inst, std::cerr)) return 1;
    print_instance(inst);

    if (bddinit(1024)) {
        std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
        return 1;
    }
    int rc = run(inst);
    bddfinal();
    return rc;
}
