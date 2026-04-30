/***************************************************************
 *  Graph coloring via MVBDD (multi-valued BDD).                 *
 *                                                               *
 *  Each vertex v becomes an MVDD variable with domain {0..k-1}. *
 *  Internally, an MV variable with domain k is backed by (k-1)  *
 *  one-hot DD variables d_0, ..., d_{k-2}: value=0 means all    *
 *  zero, value=c (c>0) means d_{c-1}=1 and don't-care above.    *
 *  To make the exact_count and enumerate passes unambiguous we  *
 *  AND in a "canonical" at-most-one constraint per vertex (at   *
 *  most one of the k-1 DD vars is 1), which pins the remaining  *
 *  don't-care bits to 0 and gives a unique bit pattern per      *
 *  color. The coloring constraint for each edge (u, v) is then  *
 *      ~( OR_{c=0..k-1} (x_u == c & x_v == c) )                 *
 *  = "u and v have different colors". All constraints are ANDed *
 *  into F; a proper k-coloring is any assignment with F = 1.    *
 *                                                               *
 *  Output:                                                      *
 *    * MVBDD / underlying BDD sizes                             *
 *    * #proper k-colorings (BigInt) via exact_count on F.to_bdd *
 *      over n*(k-1) DD variables                                *
 *    * up to 5 sample colorings (first enumerated via MVZDD)    *
 *    * chromatic number search over k' = 1..k                   *
 *                                                               *
 *  Input format (text):                                         *
 *    First non-comment tokens: n m k                            *
 *    Then m lines, one edge per line: "<u> <v>"                 *
 *  Vertices are 1-indexed. Self-loops are ignored with a        *
 *  warning. Lines starting with 'c', 'C', '#', '%' are comments.*
 ***************************************************************/

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "bdd.h"

using namespace kyotodd;

static const int MAX_SAMPLES = 5;

static bool is_comment_or_blank(const std::string& line) {
  std::size_t i = line.find_first_not_of(" \t\r");
  if (i == std::string::npos) return true;
  char c = line[i];
  return c == 'c' || c == 'C' || c == '#' || c == '%';
}

static bool next_data_line(std::istream& in, std::string& out) {
  std::string line;
  while (std::getline(in, line)) {
    if (is_comment_or_blank(line)) continue;
    out = line;
    return true;
  }
  return false;
}

struct Graph {
  int n;
  int k;
  std::vector<std::pair<int, int> > edges;
};

static bool read_graph(const std::string& path, Graph& g) {
  std::ifstream in(path.c_str());
  if (!in) {
    std::cerr << "Error: cannot open file: " << path << std::endl;
    return false;
  }
  std::string line;
  if (!next_data_line(in, line)) {
    std::cerr << "Error: missing header line (expected \"n m k\")."
              << std::endl;
    return false;
  }
  int m = 0;
  {
    std::istringstream hs(line);
    if (!(hs >> g.n >> m >> g.k)) {
      std::cerr << "Error: failed to parse \"n m k\" from: " << line
                << std::endl;
      return false;
    }
  }
  if (g.n <= 0) {
    std::cerr << "Error: n must be positive (got " << g.n << ")." << std::endl;
    return false;
  }
  if (m < 0) {
    std::cerr << "Error: m must be non-negative (got " << m << ")."
              << std::endl;
    return false;
  }
  if (g.k < 1) {
    std::cerr << "Error: k must be >= 1 (got " << g.k << ")." << std::endl;
    return false;
  }
  g.edges.clear();
  g.edges.reserve(m);
  for (int i = 0; i < m; ++i) {
    if (!next_data_line(in, line)) {
      std::cerr << "Error: expected " << m << " edge lines, got only " << i
                << "." << std::endl;
      return false;
    }
    std::istringstream es(line);
    int u, v;
    if (!(es >> u >> v)) {
      std::cerr << "Error: failed to parse edge line " << (i + 1) << "."
                << std::endl;
      return false;
    }
    if (u < 1 || u > g.n || v < 1 || v > g.n) {
      std::cerr << "Error: edge " << (i + 1) << " vertex out of range [1, "
                << g.n << "]." << std::endl;
      return false;
    }
    if (u == v) {
      std::cerr << "Warning: edge " << (i + 1) << " is a self-loop on " << u
                << "; ignored." << std::endl;
      continue;
    }
    g.edges.push_back(std::make_pair(u, v));
  }
  return true;
}

static void print_graph(const Graph& g) {
  std::printf("Input graph:\n");
  std::printf("  n (vertices) = %d, edges = %d, k (colors) = %d\n", g.n,
              (int)g.edges.size(), g.k);
  std::printf("  edge list    : ");
  for (std::size_t i = 0; i < g.edges.size(); ++i) {
    std::printf("%s(%d,%d)", i ? " " : "", g.edges[i].first,
                g.edges[i].second);
  }
  std::printf("\n");
}

struct Result {
  uint64_t mv_nodes;
  uint64_t bdd_size;
  bigint::BigInt count;
  std::vector<std::vector<int> > samples;
};

// "At most one of vars is true" as a plain BDD — pairwise naive encoding,
// O(|vars|^2) clauses. Used to pin an MV variable's don't-care DD bits to 0.
static BDD at_most_one_bdd(const std::vector<bddvar>& vars) {
  BDD result = BDD::True;
  for (std::size_t i = 0; i < vars.size(); ++i) {
    for (std::size_t j = i + 1; j < vars.size(); ++j) {
      result = result & ~(BDD::prime(vars[i]) & BDD::prime(vars[j]));
    }
  }
  return result;
}

static Result solve_k(const Graph& g, int k_colors, int max_samples) {
  Result r;
  r.mv_nodes = 0;
  r.bdd_size = 0;

  // k_colors == 1: only the all-color-0 assignment; feasible iff no edges.
  if (k_colors == 1) {
    if (g.edges.empty()) {
      r.count = bigint::BigInt(1);
      if (max_samples > 0) {
        r.samples.push_back(std::vector<int>(g.n, 0));
      }
    } else {
      r.count = bigint::BigInt(0);
    }
    return r;
  }

  MVBDD base(k_colors);
  std::vector<bddvar> vars(g.n + 1, 0);
  for (int v = 1; v <= g.n; ++v) vars[v] = base.new_var();

  MVBDD F = MVBDD::one(base.var_table());
  // Canonical: at most one of each MV variable's DD vars is 1. This pins
  // the don't-care bits that MVBDD::singleton leaves free (c-1)-th bit =
  // 1, other bits = 0) so counting and enumeration are unambiguous.
  for (int v = 1; v <= g.n; ++v) {
    const std::vector<bddvar>& dvars =
        base.var_table()->dd_vars_of(vars[v]);
    MVBDD canonical = MVBDD::from_bdd(base, at_most_one_bdd(dvars));
    F = F & canonical;
  }
  for (std::size_t i = 0; i < g.edges.size(); ++i) {
    int u = g.edges[i].first;
    int w = g.edges[i].second;
    MVBDD equal_uw = MVBDD::zero(base.var_table());
    for (int c = 0; c < k_colors; ++c) {
      MVBDD both_c = MVBDD::singleton(base, vars[u], c)
                   & MVBDD::singleton(base, vars[w], c);
      equal_uw = equal_uw | both_c;
    }
    F = F & ~equal_uw;
  }

  r.mv_nodes = F.mvbdd_node_count();
  r.bdd_size = F.size();

  BDD bdd = F.to_bdd();
  int total_dd = g.n * (k_colors - 1);
  r.count = bdd.exact_count(total_dd);

  if (max_samples > 0 && r.count > bigint::BigInt(0)) {
    ZDD zdd = bdd.to_zdd(total_dd);
    MVZDD mvz(base.var_table(), zdd);
    std::vector<std::vector<int> > all = mvz.enumerate();
    int take = (int)all.size();
    if (take > max_samples) take = max_samples;
    for (int i = 0; i < take; ++i) r.samples.push_back(all[i]);
  }
  return r;
}

static void print_sample(const std::vector<int>& assign) {
  std::printf("    ");
  for (std::size_t v = 0; v < assign.size(); ++v) {
    std::printf("%sv%zu=%d", v ? ", " : "", v + 1, assign[v]);
  }
  std::printf("\n");
}

// Each solve_k call allocates fresh DD variables via bddnewvar(), so the
// global variable pool grows monotonically within a single bddinit
// session. When the chromatic search runs solve_k multiple times, later
// calls would reference DD var levels beyond the n passed to exact_count.
// To keep each solve independent we bddinit / bddfinal around every call.
static Result solve_k_isolated(const Graph& g, int k_colors,
                               int max_samples) {
  if (bddinit(1024)) {
    std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
    std::exit(1);
  }
  Result r = solve_k(g, k_colors, max_samples);
  bddfinal();
  return r;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::fprintf(stderr, "Usage: coloring <file.txt>\n");
    return 1;
  }

  Graph g;
  if (!read_graph(argv[1], g)) return 1;
  print_graph(g);

  std::printf("\n=== %d-coloring of the graph ===\n", g.k);
  Result primary = solve_k_isolated(g, g.k, MAX_SAMPLES);
  std::printf("  MVBDD nodes          : %" PRIu64 "\n", primary.mv_nodes);
  std::printf("  Underlying BDD size  : %" PRIu64 "\n", primary.bdd_size);
  std::cout << "  #proper colorings    : " << primary.count << std::endl;
  if (primary.count == bigint::BigInt(0)) {
    std::printf("  => The graph is NOT %d-colorable.\n", g.k);
  } else {
    std::printf("  => The graph IS %d-colorable.\n", g.k);
    std::printf("\n  Up to %d sample colorings (colors numbered 0..%d):\n",
                MAX_SAMPLES, g.k - 1);
    for (std::size_t i = 0; i < primary.samples.size(); ++i) {
      std::printf("  [#%zu]\n", i + 1);
      print_sample(primary.samples[i]);
    }
  }

  std::printf("\n=== Chromatic number search (k' = 1..%d) ===\n", g.k);
  int chromatic = -1;
  for (int kk = 1; kk <= g.k; ++kk) {
    Result rr = solve_k_isolated(g, kk, 0);
    std::cout << "  k'=" << kk << ": #proper colorings = " << rr.count;
    if (chromatic < 0 && rr.count > bigint::BigInt(0)) {
      chromatic = kk;
      std::cout << "   <-- first feasible k'";
    }
    std::cout << std::endl;
  }
  if (chromatic < 0) {
    std::printf("  => chromatic number > %d (not found within search range).\n",
                g.k);
  } else {
    std::printf("  => chromatic number = %d.\n", chromatic);
  }

  return 0;
}
