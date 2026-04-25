Weighted Model Counting (WMC) via MTBDDFloat
============================================

For a CNF formula phi(x_1, ..., x_n) and per-literal weights
w_i(0), w_i(1), the weighted model count is

    WMC(phi) = sum_{sigma |= phi} prod_{i=1..n} w_i(sigma(x_i))

This sample demonstrates an MTBDD (Multi-Terminal BDD) realization of
WMC. The pipeline is:

  1. Parse a DIMACS-style CNF file plus extra "w" lines that give
     per-variable literal weights.
  2. Build the CNF BDD F by AND-ing BDD::clause(c) for each clause.
  3. Build the per-variable weight MTBDDFloat
       W_i = MTBDDFloat::ite(i, terminal(w_pos_i), terminal(w_neg_i))
     and their product W_total = W_1 * W_2 * ... * W_n.
  4. Form M = MTBDDFloat::from_bdd(F, 0.0, 1.0) * W_total. Any path of
     M carries a terminal value equal to
       (path satisfies phi) ? prod w_i : 0.
  5. Sum M's terminal values over all 2^n assignments using a
     skip-aware top-down recursion with node-id memoization.

Files:
  wmc.cpp       implementation (~260 lines)
  small.txt     hand-checkable 3-var instance (WMC = 0.53)
  medium.txt    12-var instance, default weights; WMC * 4096 = #SAT
  unsat.txt     UNSAT instance; WMC = 0


Usage
-----
    wmc <file.txt> [svg_output]

The optional second argument writes the combined MTBDDFloat M as SVG
(variable nodes labeled x1, x2, ... and terminal nodes carrying the
numeric weight).


Input format
------------
DIMACS-style CNF, with optional weight lines:

    p cnf <num_vars> <num_clauses>
    <lit> <lit> ... 0
    ...
    w <var> <w_pos> <w_neg>       # optional, per variable

Lines starting with 'c', 'C', '#', or '%' are comments; blank lines are
ignored. A trailing clause may omit the 0 terminator. Variables omitted
from "w" lines default to (0.5, 0.5), which makes WMC = (#SAT) / 2^n.
Negative weights are rejected.


Sample commands
---------------
    ./wmc app/wmc/small.txt
    ./wmc app/wmc/medium.txt
    ./wmc app/wmc/unsat.txt
    ./wmc app/wmc/small.txt /tmp/wmc_small.svg


Verification
------------
* small.txt: printed WMC must equal 0.53 to ~1e-12 precision. The
  comments in small.txt enumerate all four satisfying assignments and
  their products.

* medium.txt: all weights default to (0.5, 0.5), so
      WMC * 2^12 = #SAT.
  Cross-check with the existing sample:
      ./cnfsample app/wmc/medium.txt 0
  and verify round(WMC * 4096) matches the printed #Solutions.

* unsat.txt: UNSAT is detected during BDD construction; the program
  prints "Formula is UNSAT -- WMC = 0" and exits with WMC = 0.


Implementation notes
--------------------
* The sub-sum recursion keys its memo on bddp alone: the per-variable
  weights are already baked into the MTBDDFloat structure, so the
  sub-sum at any node depends only on that node's id.
* Skip-correction (for variables that BDD reduction elided between a
  node and its children) is computed on the parent side using
  std::ldexp(x, k) = x * 2^k.
* Variables are allocated 1..n via bddnewvar() in order, so the default
  variable-to-level mapping is var i <-> level i.
