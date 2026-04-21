CNFSample - Random sampling of satisfying assignments of a CNF formula
=======================================================================

Builds a BDD from a DIMACS CNF file, reports the BDD size and number of
satisfying assignments, then uniformly samples N assignments at random.

Usage:
  cnfsample <file.cnf> [num_samples=10] [seed=0]

Examples (run from the build directory):
  ./cnfsample ../app/CNFSample/example.cnf
  ./cnfsample ../app/CNFSample/example.cnf 20 42
  ./cnfsample ../app/CNFSample/pigeon.cnf   # UNSAT — no samples

DIMACS CNF format:
  - Lines starting with 'c' or '%' are comments.
  - Header line: "p cnf <num_vars> <num_clauses>".
  - Each clause is a whitespace-separated list of non-zero integers
    terminated by 0. Positive = variable, negative = negated variable.
