/***************************************************************
 *  Random sampling of satisfying assignments of a CNF formula   *
 *  using KyotoDD (BDD class).                                   *
 *                                                               *
 *  Input : DIMACS CNF file                                      *
 *  Output: BDD size, #solutions, and N random satisfying        *
 *          assignments (x_i = 0/1).                             *
 ***************************************************************/

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "bdd.h"

using namespace kyotodd;

struct CNF {
  int num_vars = 0;
  int num_clauses_declared = 0;
  std::vector<std::vector<int> > clauses;
};

static bool parse_cnf(const std::string& path, CNF& out) {
  std::ifstream in(path.c_str());
  if (!in) {
    std::cerr << "Error: cannot open file: " << path << std::endl;
    return false;
  }

  bool header_seen = false;
  std::vector<int> current;
  std::string line;

  while (std::getline(in, line)) {
    // skip empty lines and comments
    std::size_t pos = line.find_first_not_of(" \t\r");
    if (pos == std::string::npos) continue;
    char c0 = line[pos];
    if (c0 == 'c' || c0 == '%') continue;

    if (c0 == 'p') {
      std::istringstream ss(line);
      std::string tok_p, tok_cnf;
      ss >> tok_p >> tok_cnf >> out.num_vars >> out.num_clauses_declared;
      if (tok_p != "p" || tok_cnf != "cnf") {
        std::cerr << "Error: malformed header: " << line << std::endl;
        return false;
      }
      header_seen = true;
      continue;
    }

    // literal stream
    std::istringstream ss(line);
    int lit;
    while (ss >> lit) {
      if (lit == 0) {
        out.clauses.push_back(current);
        current.clear();
      } else {
        current.push_back(lit);
      }
    }
  }

  if (!current.empty()) {
    // accept trailing clause missing the 0 terminator
    out.clauses.push_back(current);
  }

  if (!header_seen) {
    std::cerr << "Error: missing 'p cnf' header." << std::endl;
    return false;
  }

  // validate literals
  for (std::size_t i = 0; i < out.clauses.size(); ++i) {
    for (std::size_t j = 0; j < out.clauses[i].size(); ++j) {
      int v = out.clauses[i][j];
      int a = v < 0 ? -v : v;
      if (a < 1 || a > out.num_vars) {
        std::cerr << "Error: literal " << v << " out of range [1,"
                  << out.num_vars << "]." << std::endl;
        return false;
      }
    }
  }
  return true;
}

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 4) {
    std::fprintf(stderr,
                 "Usage: cnfsample <file.cnf> [num_samples=10] [seed=0]\n");
    return 1;
  }

  std::string path = argv[1];
  int num_samples = 10;
  uint64_t seed = 0;
  if (argc >= 3) num_samples = std::atoi(argv[2]);
  if (argc >= 4) seed = std::strtoull(argv[3], NULL, 10);
  if (num_samples < 0) num_samples = 0;

  CNF cnf;
  if (!parse_cnf(path, cnf)) return 1;

  std::printf("Input file : %s\n", path.c_str());
  std::printf("Variables  : %d\n", cnf.num_vars);
  std::printf("Clauses    : %zu (declared %d)\n",
              cnf.clauses.size(), cnf.num_clauses_declared);
  std::printf("Samples    : %d\n", num_samples);
  std::printf("Seed       : %" PRIu64 "\n", seed);

  // Initialize BDD package.
  if (bddinit(256)) {
    std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
    return 1;
  }

  // Allocate variables 1..num_vars in order.
  for (int i = 0; i < cnf.num_vars; ++i) {
    bddnewvar();
  }

  // Build BDD: conjunction of all clauses.
  BDD F = BDD::True;
  for (std::size_t i = 0; i < cnf.clauses.size(); ++i) {
    BDD C = BDD::clause(cnf.clauses[i]);
    F &= C;
    if (F == BDD::False) break;  // short-circuit once unsat
  }

  std::printf("\n=== BDD ===\n");
  std::printf("BDD size   : %" PRIu64 "\n", (uint64_t)F.raw_size());

  bigint::BigInt count = F.exact_count((bddvar)cnf.num_vars);
  std::cout << "#Solutions : " << count << std::endl;

  if (F == BDD::False || count == bigint::BigInt(0)) {
    std::printf("\nFormula is UNSAT — no samples.\n");
    return 0;
  }

  std::printf("\n=== Random samples ===\n");
  std::mt19937_64 rng(seed);
  BddCountMemo memo(F, (bddvar)cnf.num_vars);

  for (int k = 0; k < num_samples; ++k) {
    std::vector<bddvar> ones =
        F.uniform_sample(rng, (bddvar)cnf.num_vars, memo);

    std::vector<char> assign((std::size_t)cnf.num_vars + 1, 0);
    for (std::size_t i = 0; i < ones.size(); ++i) {
      bddvar v = ones[i];
      if (v >= 1 && v <= (bddvar)cnf.num_vars) assign[v] = 1;
    }

    std::printf("[%2d]", k + 1);
    for (int v = 1; v <= cnf.num_vars; ++v) {
      std::printf(" x%d=%d", v, (int)assign[v]);
    }
    std::printf("\n");
  }

  return 0;
}
