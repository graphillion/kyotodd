/***************************************************************
 *  4x4 Sudoku solver using KyotoDD (BDD).                       *
 *                                                               *
 *  Encodes a 4x4 Sudoku as a Boolean formula over 64 variables  *
 *  (x_{r,c,d} = "cell (r,c) holds digit d") with "exactly one"  *
 *  constraints for each cell / row / column / 2x2 block, plus   *
 *  unit clauses for the given clues. The characteristic BDD is  *
 *  converted to a ZDD, and up to 5 solutions are sampled with   *
 *  ZDD::sample_k.                                               *
 *                                                               *
 *  Input:  a text file containing 16 cells. Cells use '1'..'4'  *
 *          for givens and '.' or '0' for blanks. Whitespace,    *
 *          '|', '-', '+', ',' are ignored, and lines beginning  *
 *          with 'c', 'C', '#', or '%' are treated as comments.  *
 *          Both "4 lines of 4 chars" and "16 chars on 1 line"   *
 *          layouts work.                                        *
 ***************************************************************/

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "bdd.h"

using namespace kyotodd;

static const int N = 4;         // side length of the grid
static const int BS = 2;        // block size (BS*BS = N)
static const int NC = N * N;    // number of cells
static const int NV = N * N * N;  // number of Boolean variables

// Variable numbering: x_{r,c,d} with r,c,d in 1..N.
// Order: (r-1)*N*N + (c-1)*N + d  ∈ [1, NV].
// d is innermost, so the N variables for "which digit does cell (r,c) hold"
// are adjacent in the DD order.
static inline int var_of(int r, int c, int d) {
  return (r - 1) * N * N + (c - 1) * N + d;
}

static bool read_puzzle(const std::string& path,
                        std::vector<std::vector<int> >& grid) {
  std::ifstream in(path.c_str());
  if (!in) {
    std::cerr << "Error: cannot open file: " << path << std::endl;
    return false;
  }
  grid.assign(N, std::vector<int>(N, 0));
  int count = 0;
  std::string line;
  while (count < NC && std::getline(in, line)) {
    std::size_t first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos) continue;
    char c0 = line[first];
    if (c0 == 'c' || c0 == 'C' || c0 == '#' || c0 == '%') continue;

    for (std::size_t i = 0; i < line.size() && count < NC; ++i) {
      char ch = line[i];
      if (ch == ' ' || ch == '\t' || ch == '\r' ||
          ch == '|' || ch == '-' || ch == '+' || ch == ',') continue;
      int r = count / N;
      int c = count % N;
      if (ch == '.' || ch == '0') {
        grid[r][c] = 0;
      } else if (ch >= '1' && ch <= '0' + N) {
        grid[r][c] = ch - '0';
      } else {
        std::cerr << "Error: invalid character '" << ch << "' at cell "
                  << count << std::endl;
        return false;
      }
      ++count;
    }
  }
  if (count < NC) {
    std::cerr << "Error: only " << count << " cells read, expected " << NC
              << "." << std::endl;
    return false;
  }
  return true;
}

static void print_grid(const std::vector<std::vector<int> >& grid) {
  for (int r = 0; r < N; ++r) {
    if (r > 0 && r % BS == 0) std::printf("  -----+-----\n");
    std::printf("  ");
    for (int c = 0; c < N; ++c) {
      if (c > 0 && c % BS == 0) std::printf("| ");
      if (grid[r][c] == 0) std::printf(". ");
      else std::printf("%d ", grid[r][c]);
    }
    std::printf("\n");
  }
}

// exactly_one(x_1, ..., x_k): O(k)-size BDD built as an ITE cascade.
//   all_zero = "none of the remaining vars is set"
//   one_set  = "exactly one of the vars seen so far is set"
// At each step i:
//   one_set'  = x_i ? all_zero : one_set
//   all_zero' = ~x_i AND all_zero
static BDD exactly_one(const std::vector<int>& vars) {
  BDD all_zero = BDD::True;
  BDD one_set = BDD::False;
  for (int i = (int)vars.size() - 1; i >= 0; --i) {
    BDD xi = BDD::prime(vars[i]);
    BDD next_one_set = BDD::ite(xi, all_zero, one_set);
    all_zero = BDD::prime_not(vars[i]) & all_zero;
    one_set = next_one_set;
  }
  return one_set;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::fprintf(stderr, "Usage: sudoku <file.txt>\n");
    return 1;
  }

  std::vector<std::vector<int> > puzzle;
  if (!read_puzzle(argv[1], puzzle)) return 1;

  std::printf("Input puzzle (4x4):\n");
  print_grid(puzzle);

  if (bddinit(1024, bddnull)) {
    std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
    return 1;
  }
  for (int i = 1; i <= NV; ++i) bddnewvar();

  std::printf("\nBuilding BDD...\n");
  BDD F = BDD::True;

  // Fix the given clues first — this greatly shrinks intermediate BDDs.
  int clues = 0;
  for (int r = 1; r <= N; ++r) {
    for (int c = 1; c <= N; ++c) {
      int d = puzzle[r - 1][c - 1];
      if (d != 0) {
        F &= BDD::prime(var_of(r, c, d));
        ++clues;
      }
    }
  }
  std::printf("  clues           : %d\n", clues);
  std::printf("  after clues     : size=%" PRIu64 "\n",
              (uint64_t)F.raw_size());
  std::fflush(stdout);

  // Each cell contains exactly one digit.
  for (int r = 1; r <= N; ++r) {
    for (int c = 1; c <= N; ++c) {
      std::vector<int> vs;
      for (int d = 1; d <= N; ++d) vs.push_back(var_of(r, c, d));
      F &= exactly_one(vs);
    }
  }
  std::printf("  after cells     : size=%" PRIu64 "\n",
              (uint64_t)F.raw_size());
  std::fflush(stdout);

  // Each row contains each digit exactly once.
  for (int r = 1; r <= N; ++r) {
    for (int d = 1; d <= N; ++d) {
      std::vector<int> vs;
      for (int c = 1; c <= N; ++c) vs.push_back(var_of(r, c, d));
      F &= exactly_one(vs);
    }
  }
  std::printf("  after rows      : size=%" PRIu64 "\n",
              (uint64_t)F.raw_size());
  std::fflush(stdout);

  // Each column contains each digit exactly once.
  for (int c = 1; c <= N; ++c) {
    for (int d = 1; d <= N; ++d) {
      std::vector<int> vs;
      for (int r = 1; r <= N; ++r) vs.push_back(var_of(r, c, d));
      F &= exactly_one(vs);
    }
  }
  std::printf("  after columns   : size=%" PRIu64 "\n",
              (uint64_t)F.raw_size());
  std::fflush(stdout);

  // Each BS x BS block contains each digit exactly once.
  for (int br = 0; br < BS; ++br) {
    for (int bc = 0; bc < BS; ++bc) {
      for (int d = 1; d <= N; ++d) {
        std::vector<int> vs;
        for (int dr = 1; dr <= BS; ++dr) {
          for (int dc = 1; dc <= BS; ++dc) {
            vs.push_back(var_of(br * BS + dr, bc * BS + dc, d));
          }
        }
        F &= exactly_one(vs);
      }
    }
  }
  std::printf("  after blocks    : size=%" PRIu64 "\n",
              (uint64_t)F.raw_size());
  std::fflush(stdout);

  std::printf("\nFinal BDD size  : %" PRIu64 "\n", (uint64_t)F.raw_size());

  bigint::BigInt sol_count = F.exact_count(NV);
  std::cout << "#Solutions      : " << sol_count << std::endl;

  if (F == BDD::False || sol_count == bigint::BigInt(0)) {
    std::printf("\nNo solution.\n");
    return 0;
  }

  ZDD Fz = F.to_zdd(NV);
  ZddCountMemo zmemo(Fz);

  std::mt19937_64 rng(0);
  int64_t k = 5;
  if (sol_count < bigint::BigInt(5)) {
    k = (int64_t)std::atoll(sol_count.to_string().c_str());
  }
  ZDD sampled = Fz.sample_k(k, rng, zmemo);

  std::vector<std::vector<bddvar> > sols = sampled.enumerate();
  std::printf("\n=== Up to 5 solutions ===\n");
  int shown = 0;
  for (std::size_t s = 0; s < sols.size(); ++s) {
    std::vector<std::vector<int> > g(N, std::vector<int>(N, 0));
    for (std::size_t i = 0; i < sols[s].size(); ++i) {
      int v = (int)sols[s][i] - 1;        // 0 .. NV-1
      int r = v / (N * N) + 1;            // 1..N
      int rest = v % (N * N);
      int c = rest / N + 1;               // 1..N
      int d = rest % N + 1;               // 1..N
      g[r - 1][c - 1] = d;
    }
    std::printf("\n[solution %d]\n", ++shown);
    print_grid(g);
  }
  if (shown == 0) std::printf("(no solutions sampled)\n");

  return 0;
}
