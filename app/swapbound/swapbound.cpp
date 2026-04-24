/*********************************************************************
 *  swapbound - k-swap reachable permutations via PiDD.                *
 *                                                                     *
 *  Given a permutation size n and a bound k, this program builds the  *
 *  set of permutations in S_n reachable from the identity by at most  *
 *  k transpositions (Cayley distance <= k) using PiDD.                *
 *                                                                     *
 *  Approach:                                                          *
 *    1. Build "step" = identity + every transposition (i, j) with     *
 *       1 <= j < i <= n. This is all of S_n at Cayley distance <= 1.  *
 *    2. Starting from R_0 = {identity}, iterate                       *
 *         R_i = (R_{i-1} * step).swap_bound(i)                        *
 *       k times. PiDD's operator* composes each element in R_{i-1}    *
 *       with each element of step. After i iterations, R_i contains   *
 *       exactly the permutations expressible as a product of <= i     *
 *       transpositions; swap_bound(i) trims the canonical encoding.   *
 *    3. Print |R_i| and the internal ZDD node count for each i.       *
 *       When |R_k| is small (<= ENUM_LIMIT), enumerate all            *
 *       permutations; when an SVG output path is given, save the      *
 *       internal ZDD as SVG with transposition labels (x, y).         *
 *                                                                     *
 *  PiDD's canonical encoding of pi uses n - c(pi) transpositions,     *
 *  where c(pi) is the number of cycles. Hence for i >= n - 1 the set  *
 *  saturates to all of S_n.                                           *
 *                                                                     *
 *  Usage:                                                             *
 *    swapbound <n> <k> [svg_out]                                      *
 *********************************************************************/

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "bdd.h"
#include "pidd.h"
#include "svg_export.h"

using namespace kyotodd;

static const uint64_t ENUM_LIMIT = 120;  /* enumerate only if |R_k| <= 5! */

static uint64_t factorial_saturating(int n) {
  uint64_t f = 1;
  for (int i = 2; i <= n; ++i) {
    if (f > UINT64_MAX / (uint64_t)i) return UINT64_MAX;
    f *= (uint64_t)i;
  }
  return f;
}

int main(int argc, char* argv[]) {
  if (argc < 3 || argc > 4) {
    std::fprintf(stderr, "Usage: swapbound <n> <k> [svg_out]\n");
    return 1;
  }

  int n = std::atoi(argv[1]);
  int k = std::atoi(argv[2]);
  const char* svg_out = (argc == 4) ? argv[3] : NULL;

  if (n < 1) {
    std::fprintf(stderr, "Error: n must be >= 1 (got %d).\n", n);
    return 1;
  }
  if (n > PiDD_MaxVar) {
    std::fprintf(stderr, "Error: n must be <= %d (got %d).\n",
                 PiDD_MaxVar, n);
    return 1;
  }
  if (k < 0) {
    std::fprintf(stderr, "Error: k must be >= 0 (got %d).\n", k);
    return 1;
  }

  if (bddinit(1024, bddnull)) {
    std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
    return 1;
  }
  for (int i = 0; i < n; ++i) PiDD_NewVar();

  uint64_t n_fact = factorial_saturating(n);
  std::printf("SwapBound reachability for S_%d, k = %d\n", n, k);
  if (n_fact == UINT64_MAX) {
    std::printf("  n = %d, k = %d, |S_n| = n!  (overflows uint64)\n", n, k);
  } else {
    std::printf("  n = %d, k = %d, |S_n| = n! = %" PRIu64 "\n",
                n, k, n_fact);
  }
  std::printf("\n");

  /* step = identity + all C(n, 2) transpositions */
  PiDD id(1);
  PiDD step = id;
  for (int i = 2; i <= n; ++i) {
    for (int j = 1; j < i; ++j) {
      step = step + id.swap(i, j);
    }
  }

  /* Iterate R_0 = id, R_i = (R_{i-1} * step).swap_bound(i) */
  PiDD reachable = id;
  std::printf("  step        card         size\n");
  std::printf("  %4d  %10" PRIu64 "   %10" PRIu64 "\n",
              0, (uint64_t)reachable.card(), (uint64_t)reachable.size());
  for (int i = 1; i <= k; ++i) {
    reachable = (reachable * step).swap_bound(i);
    std::printf("  %4d  %10" PRIu64 "   %10" PRIu64 "\n",
                i, (uint64_t)reachable.card(), (uint64_t)reachable.size());
  }
  std::printf("\n");

  uint64_t final_card = reachable.card();
  if (final_card <= ENUM_LIMIT) {
    std::printf("Enumerating reachable permutations (card = %" PRIu64 "):\n",
                final_card);
    reachable.enumerate();
    std::printf("\n");
  } else {
    std::printf("Skipping enumeration: card = %" PRIu64 " > %" PRIu64 ".\n",
                final_card, (uint64_t)ENUM_LIMIT);
  }

  if (svg_out != NULL) {
    SvgParams params;
    params.var_name_map = PiDD::svg_var_name_map();
    reachable.save_svg(svg_out, params);
    std::printf("SVG saved to: %s\n", svg_out);
  }

  return 0;
}
