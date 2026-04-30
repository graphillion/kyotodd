/***************************************************************
 *  0/1 Knapsack solver using KyotoDD (ZDD).                     *
 *                                                               *
 *  Given N items with weights w_i and values v_i and a capacity *
 *  C, this program:                                             *
 *    1. Builds the ZDD of all subsets (power set of N items).   *
 *    2. Filters by cost_bound_le(weights, C) to keep subsets    *
 *       whose total weight is within the capacity.              *
 *    3. Reports #feasible solutions (exact BigInt count),       *
 *       the optimal (max-value) solution via max_weight_set,    *
 *       and the top-k heaviest (by value) solutions via         *
 *       get_k_heaviest. k is fixed to 5 (trimmed if fewer).     *
 *                                                               *
 *  Input format (text):                                         *
 *    N C         // item count, capacity                        *
 *    w1 v1       // weight and value of item 1                  *
 *    w2 v2                                                      *
 *    ...                                                        *
 *    wN vN                                                      *
 *  Lines starting with 'c', 'C', '#', or '%' are comments.      *
 *  Blank lines are ignored. Tokens may be separated by any      *
 *  whitespace; a single line for N/C and one line per item is   *
 *  the recommended layout.                                      *
 ***************************************************************/

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
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

static bool read_instance(const std::string& path, int& N, long long& C,
                          std::vector<int>& weights,
                          std::vector<int>& values) {
  std::ifstream in(path.c_str());
  if (!in) {
    std::cerr << "Error: cannot open file: " << path << std::endl;
    return false;
  }
  // Concatenate all non-comment tokens then parse.
  std::ostringstream oss;
  std::string line;
  while (std::getline(in, line)) {
    if (is_comment_or_blank(line)) continue;
    oss << line << ' ';
  }
  std::istringstream iss(oss.str());
  if (!(iss >> N >> C)) {
    std::cerr << "Error: failed to read N and C." << std::endl;
    return false;
  }
  if (N <= 0) {
    std::cerr << "Error: N must be positive (got " << N << ")." << std::endl;
    return false;
  }
  if (C < 0) {
    std::cerr << "Error: capacity C must be non-negative (got " << C
              << ")." << std::endl;
    return false;
  }
  // Index 0 unused; items are 1..N.
  weights.assign(N + 1, 0);
  values.assign(N + 1, 0);
  for (int i = 1; i <= N; ++i) {
    int w, v;
    if (!(iss >> w >> v)) {
      std::cerr << "Error: failed to read item " << i << "." << std::endl;
      return false;
    }
    if (w < 0 || v < 0) {
      std::cerr << "Error: item " << i << " has negative weight/value."
                << std::endl;
      return false;
    }
    weights[i] = w;
    values[i] = v;
  }
  return true;
}

static void print_item_table(int N, const std::vector<int>& weights,
                             const std::vector<int>& values, long long C) {
  std::printf("Input instance:\n");
  std::printf("  N = %d, capacity = %lld\n", N, C);
  std::printf("  item   weight   value\n");
  for (int i = 1; i <= N; ++i) {
    std::printf("  %4d   %6d   %5d\n", i, weights[i], values[i]);
  }
}

static long long set_weight(const std::vector<bddvar>& set,
                            const std::vector<int>& w) {
  long long s = 0;
  for (std::size_t i = 0; i < set.size(); ++i) s += w[set[i]];
  return s;
}

static void print_solution(const std::vector<bddvar>& set,
                           const std::vector<int>& weights,
                           const std::vector<int>& values) {
  long long w = set_weight(set, weights);
  long long v = set_weight(set, values);
  std::printf("  items = {");
  for (std::size_t i = 0; i < set.size(); ++i) {
    std::printf("%s%u", i ? ", " : "", (unsigned)set[i]);
  }
  std::printf("}  weight=%lld  value=%lld\n", w, v);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::fprintf(stderr, "Usage: knapsack <file.txt>\n");
    return 1;
  }

  int N = 0;
  long long C = 0;
  std::vector<int> weights, values;
  if (!read_instance(argv[1], N, C, weights, values)) return 1;

  print_item_table(N, weights, values, C);

  if (bddinit(1024)) {
    std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
    return 1;
  }
  for (int i = 1; i <= N; ++i) bddnewvar();

  std::printf("\nBuilding ZDD...\n");
  ZDD all = ZDD::power_set((bddvar)N);
  std::printf("  power_set size       : %" PRIu64 "\n",
              (uint64_t)all.raw_size());

  CostBoundMemo memo;
  ZDD feasible = all.cost_bound_le(weights, C, memo);
  std::printf("  feasible ZDD size    : %" PRIu64 "\n",
              (uint64_t)feasible.raw_size());

  bigint::BigInt num_feasible = feasible.exact_count();
  std::cout << "  #feasible solutions  : " << num_feasible << std::endl;

  if (feasible == ZDD::Empty || num_feasible == bigint::BigInt(0)) {
    std::printf("\nNo feasible solution fits the capacity.\n");
    return 0;
  }

  std::printf("\n=== Optimal (max-value) solution ===\n");
  std::vector<bddvar> best = feasible.max_weight_set(values);
  print_solution(best, weights, values);

  std::printf("\n=== Top-%d solutions by value ===\n", TOPK);
  int64_t k = TOPK;
  if (num_feasible < bigint::BigInt(TOPK)) {
    k = (int64_t)std::atoll(num_feasible.to_string().c_str());
  }
  ZDD topk = feasible.get_k_heaviest(k, values, 0);
  std::vector<std::vector<bddvar> > sols = topk.enumerate();

  // Sort enumerated sets by total value (descending), ties broken by weight
  // ascending then by lexicographic item list. enumerate() order depends on
  // ZDD structure, which does not coincide with value-descending order.
  std::vector<std::size_t> idx(sols.size());
  for (std::size_t i = 0; i < sols.size(); ++i) idx[i] = i;
  // Insertion sort keeps code simple for small k.
  for (std::size_t i = 1; i < idx.size(); ++i) {
    std::size_t cur = idx[i];
    long long vc = set_weight(sols[cur], values);
    long long wc = set_weight(sols[cur], weights);
    std::size_t j = i;
    while (j > 0) {
      std::size_t prev = idx[j - 1];
      long long vp = set_weight(sols[prev], values);
      long long wp = set_weight(sols[prev], weights);
      bool swap = (vp < vc) || (vp == vc && wp > wc);
      if (!swap) break;
      idx[j] = prev;
      --j;
    }
    idx[j] = cur;
  }

  for (std::size_t rank = 0; rank < idx.size(); ++rank) {
    std::printf("[#%zu] ", rank + 1);
    print_solution(sols[idx[rank]], weights, values);
  }

  return 0;
}
