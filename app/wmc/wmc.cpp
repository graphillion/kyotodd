/***************************************************************
 *  Weighted Model Counting (WMC) using KyotoDD MTBDDFloat.       *
 *                                                                *
 *  For a CNF formula phi(x_1, ..., x_n) and per-literal weights  *
 *  w_i(0), w_i(1), the WMC is                                    *
 *                                                                *
 *      WMC(phi) = sum_{sigma |= phi} prod_{i=1..n} w_i(sigma(x_i)) *
 *                                                                *
 *  Pipeline:                                                     *
 *    1. Parse DIMACS CNF + extra "w v wp wn" lines.              *
 *    2. Build CNF BDD F via repeated BDD::clause + AND.          *
 *    3. Build per-variable weight MTBDDFloat                     *
 *         W_i = ite(i, terminal(w_pos_i), terminal(w_neg_i))     *
 *       and the product W_total = W_1 * W_2 * ... * W_n.         *
 *    4. M = MTBDDFloat::from_bdd(F, 0.0, 1.0) * W_total.         *
 *       At any path of M, the terminal value is                  *
 *         (sigma satisfies F) ? prod w_i(sigma(x_i)) : 0.        *
 *    5. Sum M's terminal values over all 2^n assignments via a   *
 *       skip-aware top-down recursion with node-id memoization.  *
 *                                                                *
 *  Input format (text):                                          *
 *    p cnf <num_vars> <num_clauses>                              *
 *    <lit> <lit> ... 0                                           *
 *    ...                                                         *
 *    w <var> <w_pos> <w_neg>                                     *
 *  Lines starting with 'c', 'C', '#', or '%' are comments.       *
 *  Variables omitted from "w" lines default to (0.5, 0.5).       *
 *  Then WMC = (#SAT) / 2^n.                                      *
 *                                                                *
 *  Usage: wmc <file.txt> [svg_output]                            *
 ***************************************************************/

#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bdd.h"

using namespace kyotodd;
using MTBDDFloat = MTBDD<double>;

struct WMCInstance {
  int num_vars = 0;
  int num_clauses_declared = 0;
  std::vector<std::vector<int> > clauses;
  // weights[v] = (w_pos, w_neg). Size = num_vars+1; index 0 is unused.
  std::vector<std::pair<double, double> > weights;
  std::vector<bool> weight_set;  // true if weights[v] was explicitly assigned
  int weight_overrides = 0;
};

static bool is_comment_or_blank(const std::string& line) {
  std::size_t i = line.find_first_not_of(" \t\r");
  if (i == std::string::npos) return true;
  char c = line[i];
  return c == 'c' || c == 'C' || c == '#' || c == '%';
}

static bool parse_input(const std::string& path, WMCInstance& inst) {
  std::ifstream in(path.c_str());
  if (!in) {
    std::cerr << "Error: cannot open file: " << path << std::endl;
    return false;
  }

  bool header_seen = false;
  std::vector<int> current;
  std::string line;

  while (std::getline(in, line)) {
    if (is_comment_or_blank(line)) continue;

    std::size_t pos = line.find_first_not_of(" \t\r");
    char c0 = line[pos];

    if (c0 == 'p') {
      std::istringstream ss(line);
      std::string tok_p, tok_cnf;
      ss >> tok_p >> tok_cnf >> inst.num_vars >> inst.num_clauses_declared;
      if (tok_p != "p" || tok_cnf != "cnf") {
        std::cerr << "Error: malformed header: " << line << std::endl;
        return false;
      }
      if (inst.num_vars < 0) {
        std::cerr << "Error: negative num_vars in header." << std::endl;
        return false;
      }
      inst.weights.assign((std::size_t)inst.num_vars + 1,
                          std::make_pair(0.5, 0.5));
      inst.weight_set.assign((std::size_t)inst.num_vars + 1, false);
      header_seen = true;
      continue;
    }

    if (c0 == 'w') {
      if (!header_seen) {
        std::cerr << "Error: 'w' line before 'p cnf' header." << std::endl;
        return false;
      }
      std::istringstream ss(line);
      std::string tok_w;
      int v;
      double wp, wn;
      ss >> tok_w >> v >> wp >> wn;
      if (!ss || tok_w != "w") {
        std::cerr << "Error: malformed weight line: " << line << std::endl;
        return false;
      }
      if (v < 1 || v > inst.num_vars) {
        std::cerr << "Error: weight var " << v << " out of range [1, "
                  << inst.num_vars << "]." << std::endl;
        return false;
      }
      if (wp < 0.0 || wn < 0.0) {
        std::cerr << "Error: negative weights are not allowed (var " << v
                  << ")." << std::endl;
        return false;
      }
      if (inst.weight_set[v]) {
        std::cerr << "Warning: duplicate weight line for var " << v
                  << "; previous value (" << inst.weights[v].first << ", "
                  << inst.weights[v].second << ") overwritten by ("
                  << wp << ", " << wn << ")." << std::endl;
      }
      inst.weights[v] = std::make_pair(wp, wn);
      inst.weight_set[v] = true;
      inst.weight_overrides++;
      continue;
    }

    if (!header_seen) {
      std::cerr << "Error: clause data before 'p cnf' header." << std::endl;
      return false;
    }

    std::istringstream ss(line);
    int lit;
    while (ss >> lit) {
      if (lit == 0) {
        inst.clauses.push_back(current);
        current.clear();
      } else {
        current.push_back(lit);
      }
    }
  }

  if (!current.empty()) {
    // Accept trailing clause missing the 0 terminator.
    inst.clauses.push_back(current);
  }

  if (!header_seen) {
    std::cerr << "Error: missing 'p cnf' header." << std::endl;
    return false;
  }

  for (std::size_t i = 0; i < inst.clauses.size(); ++i) {
    for (std::size_t j = 0; j < inst.clauses[i].size(); ++j) {
      int v = inst.clauses[i][j];
      int a = v < 0 ? -v : v;
      if (a < 1 || a > inst.num_vars) {
        std::cerr << "Error: literal " << v << " out of range [1, "
                  << inst.num_vars << "]." << std::endl;
        return false;
      }
    }
  }
  return true;
}

static void print_instance(const WMCInstance& inst) {
  std::printf("Input instance:\n");
  std::printf("  variables          : %d\n", inst.num_vars);
  std::printf("  clauses            : %zu (declared %d)\n",
              inst.clauses.size(), inst.num_clauses_declared);
  std::printf("  weight overrides   : %d (others default to (0.5, 0.5))\n",
              inst.weight_overrides);
  if (inst.weight_overrides > 0) {
    std::printf("  per-variable weights (only overridden):\n");
    for (int v = 1; v <= inst.num_vars; ++v) {
      double wp = inst.weights[v].first, wn = inst.weights[v].second;
      if (wp != 0.5 || wn != 0.5) {
        std::printf("    x%-4d  w_pos=%-8g w_neg=%-8g\n", v, wp, wn);
      }
    }
  }
}

static BDD build_cnf_bdd(const WMCInstance& inst) {
  BDD F = BDD::True;
  for (std::size_t i = 0; i < inst.clauses.size(); ++i) {
    BDD C = BDD::clause(inst.clauses[i]);
    F &= C;
    if (F == BDD::False) break;
  }
  return F;
}

static MTBDDFloat build_total_weight_mtbdd(const WMCInstance& inst) {
  MTBDDFloat W = MTBDDFloat::terminal(1.0);
  for (int v = 1; v <= inst.num_vars; ++v) {
    double wp = inst.weights[v].first;
    double wn = inst.weights[v].second;
    MTBDDFloat hi = MTBDDFloat::terminal(wp);
    MTBDDFloat lo = MTBDDFloat::terminal(wn);
    MTBDDFloat W_v = MTBDDFloat::ite((bddvar)v, hi, lo);
    W = W * W_v;
  }
  return W;
}

static MTBDDFloat combine_with_weights(const BDD& F,
                                       const MTBDDFloat& W_total) {
  MTBDDFloat M0 = MTBDDFloat::from_bdd(F, 0.0, 1.0);
  return M0 * W_total;
}

// Sub-sum at node f: complete sum of f's terminal values over all
// assignments to the variables strictly below level(f). Skip-correction
// is applied at the *parent* (since S(f) only depends on the absolute
// level of f's children, the memo is keyed on bddp alone).
static double sum_rec(bddp f, std::unordered_map<bddp, double>& memo) {
  if (f & BDD_CONST_FLAG) {
    uint64_t idx = MTBDDTerminalTable<double>::terminal_index(f);
    return MTBDDTerminalTable<double>::instance().get_value(idx);
  }
  std::unordered_map<bddp, double>::const_iterator it = memo.find(f);
  if (it != memo.end()) return it->second;

  bddvar v = node_var(f);
  bddvar k = bddlevofvar(v);  // level of f
  bddp lo = node_lo(f);
  bddp hi = node_hi(f);

  int level_lo = (lo & BDD_CONST_FLAG) ? 0 : (int)bddlevofvar(node_var(lo));
  int level_hi = (hi & BDD_CONST_FLAG) ? 0 : (int)bddlevofvar(node_var(hi));
  int skip_lo = (int)k - 1 - level_lo;
  int skip_hi = (int)k - 1 - level_hi;
  // MTBDD reduction guarantees children are at strictly lower level than the
  // parent, so skip counts are non-negative. Assert as a safety net against
  // ill-formed inputs (e.g., manually constructed nodes that violate the
  // canonical level ordering).
  assert(skip_lo >= 0 && skip_hi >= 0);

  double s_lo = sum_rec(lo, memo);
  double s_hi = sum_rec(hi, memo);
  double total = std::ldexp(s_lo, skip_lo) + std::ldexp(s_hi, skip_hi);
  memo[f] = total;
  return total;
}

static double total_weight_sum(const MTBDDFloat& M, int num_vars) {
  bddp root = M.id();
  std::unordered_map<bddp, double> memo;
  double s = sum_rec(root, memo);
  int root_lev = (root & BDD_CONST_FLAG) ? 0 :
                 (int)bddlevofvar(node_var(root));
  return std::ldexp(s, num_vars - root_lev);
}

static void save_svg_if_requested(const MTBDDFloat& M,
                                  const std::string& svg_path,
                                  int num_vars) {
  if (svg_path.empty()) return;
  SvgParams params;
  for (int v = 1; v <= num_vars; ++v) {
    std::ostringstream ns;
    ns << "x" << v;
    params.var_name_map[(bddvar)v] = ns.str();
  }
  M.save_svg(svg_path.c_str(), params);
  std::printf("SVG written to: %s\n", svg_path.c_str());
}

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    std::fprintf(stderr, "Usage: wmc <file.txt> [svg_output]\n");
    return 1;
  }
  std::string path = argv[1];
  std::string svg_path = (argc == 3) ? argv[2] : "";

  WMCInstance inst;
  if (!parse_input(path, inst)) return 1;
  print_instance(inst);

  if (bddinit(1024)) {
    std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
    return 1;
  }
  for (int i = 0; i < inst.num_vars; ++i) bddnewvar();

  std::printf("\n=== BDD construction ===\n");
  BDD F = build_cnf_bdd(inst);
  std::printf("CNF BDD size       : %" PRIu64 "\n", (uint64_t)F.raw_size());

  if (F == BDD::False) {
    std::printf("\nFormula is UNSAT \xe2\x80\x94 WMC = 0\n");
    if (!svg_path.empty()) {
      MTBDDFloat M_zero = MTBDDFloat::from_bdd(F, 0.0, 1.0);
      save_svg_if_requested(M_zero, svg_path, inst.num_vars);
    }
    std::printf("\n=== Result ===\n");
    std::printf("WMC                : 0\n");
    return 0;
  }

  std::printf("\n=== Weight MTBDD construction ===\n");
  MTBDDFloat W_total = build_total_weight_mtbdd(inst);
  std::printf("Weight MTBDD size  : %" PRIu64 "\n",
              (uint64_t)W_total.raw_size());

  MTBDDFloat M = combine_with_weights(F, W_total);
  std::printf("Combined MTBDD size: %" PRIu64 "\n", (uint64_t)M.raw_size());

  double wmc = total_weight_sum(M, inst.num_vars);

  std::printf("\n=== Result ===\n");
  std::printf("Variables          : %d\n", inst.num_vars);
  std::printf("Clauses            : %zu\n", inst.clauses.size());
  std::printf("WMC                : %.12g\n", wmc);

  save_svg_if_requested(M, svg_path, inst.num_vars);
  return 0;
}
