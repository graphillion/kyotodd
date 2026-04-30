/***************************************************************
 *  Weighted 0/1 Set Cover solver using KyotoDD (BDD + ZDD).     *
 *                                                               *
 *  Given a universe U = {1..n} and a family F = {S_1,...,S_m}   *
 *  of subsets, each with a non-negative cost c_i, find subsets  *
 *  of F whose union covers U with minimum total cost.           *
 *                                                               *
 *  Encoding: one Boolean variable x_i per subset, i = 1..m.     *
 *    For each element e in U, add the positive clause           *
 *       OR_{i : e in S_i} x_i                                   *
 *    (i.e., "at least one subset containing e is chosen").      *
 *    AND all clauses to obtain the characteristic BDD F, then   *
 *    convert to the ZDD of covers via BDD::to_zdd(m).           *
 *                                                               *
 *  Output:                                                      *
 *    * #feasible covers (exact BigInt count)                    *
 *    * #(subset-minimal) covers = minimal().exact_count()       *
 *    * the minimum-cost cover via ZDD::min_weight_set           *
 *    * the top-5 lightest covers via ZDD::get_k_lightest        *
 *                                                               *
 *  Input format (text):                                         *
 *    First non-comment tokens: m n                              *
 *    Then m lines, one subset per line:                         *
 *        <cost_i>  <e_{i,1}>  <e_{i,2}> ...                     *
 *    Element numbers are 1..n. Duplicate elements within a      *
 *    line are deduplicated. Whitespace separates tokens; lines  *
 *    starting with 'c', 'C', '#' or '%' are comments; blank     *
 *    lines are ignored.                                         *
 ***************************************************************/

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "bdd.h"

using namespace kyotodd;

static const int TOPK = 5;

static bool is_comment_or_blank(const std::string& line) {
  std::size_t i = line.find_first_not_of(" \t\r");
  if (i == std::string::npos) return true;
  char c = line[i];
  return c == 'c' || c == 'C' || c == '#' || c == '%';
}

// Pull the next non-comment, non-blank line from the stream.
static bool next_data_line(std::istream& in, std::string& out) {
  std::string line;
  while (std::getline(in, line)) {
    if (is_comment_or_blank(line)) continue;
    out = line;
    return true;
  }
  return false;
}

static bool read_instance(const std::string& path, int& m, int& n,
                          std::vector<int>& costs,
                          std::vector<std::vector<int> >& subsets) {
  std::ifstream in(path.c_str());
  if (!in) {
    std::cerr << "Error: cannot open file: " << path << std::endl;
    return false;
  }

  // First non-comment line: "m n".
  std::string line;
  if (!next_data_line(in, line)) {
    std::cerr << "Error: missing header line (expected \"m n\")." << std::endl;
    return false;
  }
  {
    std::istringstream hs(line);
    if (!(hs >> m >> n)) {
      std::cerr << "Error: failed to parse \"m n\" from: " << line << std::endl;
      return false;
    }
  }
  if (m <= 0) {
    std::cerr << "Error: m must be positive (got " << m << ")." << std::endl;
    return false;
  }
  if (n <= 0) {
    std::cerr << "Error: n must be positive (got " << n << ")." << std::endl;
    return false;
  }

  costs.assign(m + 1, 0);
  subsets.assign(m + 1, std::vector<int>());

  for (int i = 1; i <= m; ++i) {
    if (!next_data_line(in, line)) {
      std::cerr << "Error: expected " << m << " subset lines, got only "
                << (i - 1) << "." << std::endl;
      return false;
    }
    std::istringstream ss(line);
    int cost;
    if (!(ss >> cost)) {
      std::cerr << "Error: missing cost on subset line " << i << "."
                << std::endl;
      return false;
    }
    if (cost < 0) {
      std::cerr << "Error: subset " << i << " has negative cost." << std::endl;
      return false;
    }
    costs[i] = cost;
    std::set<int> elems;
    int e;
    while (ss >> e) {
      if (e < 1 || e > n) {
        std::cerr << "Error: subset " << i << " contains element " << e
                  << " out of range [1, " << n << "]." << std::endl;
        return false;
      }
      elems.insert(e);
    }
    subsets[i].assign(elems.begin(), elems.end());
  }
  return true;
}

static void print_instance(int m, int n, const std::vector<int>& costs,
                           const std::vector<std::vector<int> >& subsets) {
  std::printf("Input instance:\n");
  std::printf("  m = %d, universe size = %d\n", m, n);
  std::printf("  subset   cost   size   elements\n");
  for (int i = 1; i <= m; ++i) {
    std::printf("  %6d   %4d   %4d   {", i, costs[i], (int)subsets[i].size());
    for (std::size_t j = 0; j < subsets[i].size(); ++j) {
      std::printf("%s%d", j ? ", " : "", subsets[i][j]);
    }
    std::printf("}\n");
  }
}

static long long set_cost(const std::vector<bddvar>& chosen,
                          const std::vector<int>& costs) {
  long long s = 0;
  for (std::size_t i = 0; i < chosen.size(); ++i) s += costs[chosen[i]];
  return s;
}

static std::set<int> covered_elements(
    const std::vector<bddvar>& chosen,
    const std::vector<std::vector<int> >& subsets) {
  std::set<int> u;
  for (std::size_t i = 0; i < chosen.size(); ++i) {
    int idx = (int)chosen[i];
    const std::vector<int>& s = subsets[idx];
    u.insert(s.begin(), s.end());
  }
  return u;
}

static void print_cover(const std::vector<bddvar>& chosen, int n,
                        const std::vector<int>& costs,
                        const std::vector<std::vector<int> >& subsets) {
  long long cost = set_cost(chosen, costs);
  std::set<int> u = covered_elements(chosen, subsets);
  std::printf("  subsets = {");
  for (std::size_t i = 0; i < chosen.size(); ++i) {
    std::printf("%s%u", i ? ", " : "", (unsigned)chosen[i]);
  }
  std::printf("}  cost=%lld  (covers %d/%d elements%s)\n", cost,
              (int)u.size(), n, (int)u.size() == n ? " ✓" : " !");
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::fprintf(stderr, "Usage: setcover <file.txt>\n");
    return 1;
  }

  int m = 0, n = 0;
  std::vector<int> costs;
  std::vector<std::vector<int> > subsets;
  if (!read_instance(argv[1], m, n, costs, subsets)) return 1;

  print_instance(m, n, costs, subsets);

  if (bddinit(1024)) {
    std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
    return 1;
  }
  for (int i = 1; i <= m; ++i) bddnewvar();

  // Build the coverage BDD: for each element e in 1..n, at least one
  // subset containing e must be chosen.
  std::printf("\nBuilding BDD...\n");
  BDD F = BDD::True;
  for (int e = 1; e <= n; ++e) {
    BDD clause = BDD::False;
    for (int i = 1; i <= m; ++i) {
      const std::vector<int>& s = subsets[i];
      for (std::size_t k = 0; k < s.size(); ++k) {
        if (s[k] == e) { clause = clause | BDD::prime(i); break; }
      }
    }
    if (clause == BDD::False) {
      std::printf("  element %d has no covering subset — infeasible.\n", e);
    }
    F = F & clause;
  }
  std::printf("  final BDD size       : %" PRIu64 "\n",
              (uint64_t)F.raw_size());

  ZDD covers = F.to_zdd(m);
  std::printf("  ZDD (covers) size    : %" PRIu64 "\n",
              (uint64_t)covers.raw_size());

  bigint::BigInt num_covers = covers.exact_count();
  std::cout << "  #feasible covers     : " << num_covers << std::endl;

  if (F == BDD::False || num_covers == bigint::BigInt(0)) {
    std::printf("\nNo feasible cover exists.\n");
    return 0;
  }

  ZDD mins = covers.minimal();
  bigint::BigInt num_minimal = mins.exact_count();
  std::cout << "  #\u2286-minimal covers  : " << num_minimal << std::endl;

  std::printf("\n=== Optimal (min-cost) cover ===\n");
  std::vector<bddvar> best = covers.min_weight_set(costs);
  print_cover(best, n, costs, subsets);

  std::printf("\n=== Top-%d covers by cost ===\n", TOPK);
  int64_t k = TOPK;
  if (num_covers < bigint::BigInt(TOPK)) {
    k = (int64_t)std::atoll(num_covers.to_string().c_str());
  }
  ZDD topk = covers.get_k_lightest(k, costs, 0);
  std::vector<std::vector<bddvar> > sols = topk.enumerate();

  // ZDD enumeration is in structural order; re-sort by cost ascending,
  // ties broken by smaller |chosen| then lexicographic subset indices.
  std::vector<std::size_t> idx(sols.size());
  for (std::size_t i = 0; i < sols.size(); ++i) idx[i] = i;
  for (std::size_t i = 1; i < idx.size(); ++i) {
    std::size_t cur = idx[i];
    long long cc = set_cost(sols[cur], costs);
    std::size_t j = i;
    while (j > 0) {
      std::size_t prev = idx[j - 1];
      long long cp = set_cost(sols[prev], costs);
      bool swap;
      if (cp != cc) {
        swap = cp > cc;
      } else if (sols[prev].size() != sols[cur].size()) {
        swap = sols[prev].size() > sols[cur].size();
      } else {
        swap = sols[prev] > sols[cur];
      }
      if (!swap) break;
      idx[j] = prev;
      --j;
    }
    idx[j] = cur;
  }

  for (std::size_t rank = 0; rank < idx.size(); ++rank) {
    std::printf("[#%zu] ", rank + 1);
    print_cover(sols[idx[rank]], n, costs, subsets);
  }

  return 0;
}
