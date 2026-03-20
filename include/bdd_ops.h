#ifndef KYOTODD_BDD_OPS_H
#define KYOTODD_BDD_OPS_H

#include "bdd_types.h"
#include "bigint.hpp"

/**
 * @brief Logical NOT (complement).
 *
 * Toggles the complement flag (bit 0) of the node ID.
 * Returns bddnull if the input is bddnull.
 *
 * @param p A BDD or ZDD node ID.
 * @return The complemented node ID.
 */
inline bddp bddnot(bddp p) { if (p == bddnull) return bddnull; return p ^ BDD_COMP_FLAG; }

/**
 * @brief Logical AND of two BDDs.
 * @param f First operand.
 * @param g Second operand.
 * @return The BDD for f AND g, or bddnull if either input is bddnull.
 */
bddp bddand(bddp f, bddp g);

/**
 * @brief Logical OR of two BDDs.
 * @param f First operand.
 * @param g Second operand.
 * @return The BDD for f OR g, or bddnull if either input is bddnull.
 */
bddp bddor(bddp f, bddp g);

/**
 * @brief Logical XOR of two BDDs.
 * @param f First operand.
 * @param g Second operand.
 * @return The BDD for f XOR g, or bddnull if either input is bddnull.
 */
bddp bddxor(bddp f, bddp g);

/**
 * @brief Logical NAND of two BDDs.
 * @param f First operand.
 * @param g Second operand.
 * @return The BDD for NOT(f AND g).
 */
bddp bddnand(bddp f, bddp g);

/**
 * @brief Logical NOR of two BDDs.
 * @param f First operand.
 * @param g Second operand.
 * @return The BDD for NOT(f OR g).
 */
bddp bddnor(bddp f, bddp g);

/**
 * @brief Logical XNOR of two BDDs.
 * @param f First operand.
 * @param g Second operand.
 * @return The BDD for NOT(f XOR g), i.e. f ↔ g.
 */
bddp bddxnor(bddp f, bddp g);

/**
 * @brief Cofactor: restrict variable @p v to 0.
 * @param f A BDD node ID.
 * @param v Variable number to restrict.
 * @return The resulting BDD.
 */
bddp bddat0(bddp f, bddvar v);

/**
 * @brief Cofactor: restrict variable @p v to 1.
 * @param f A BDD node ID.
 * @param v Variable number to restrict.
 * @return The resulting BDD.
 */
bddp bddat1(bddp f, bddvar v);

/**
 * @brief If-then-else operation: (f AND g) OR (NOT f AND h).
 * @param f Condition BDD.
 * @param g Then-branch BDD.
 * @param h Else-branch BDD.
 * @return The resulting BDD, or bddnull if any input is bddnull.
 */
bddp bddite(bddp f, bddp g, bddp h);

/**
 * @brief Check implication: whether f implies g.
 * @param f The antecedent BDD.
 * @param g The consequent BDD.
 * @return 1 if f implies g (f AND NOT g == false), 0 otherwise,
 *         or -1 if either input is bddnull.
 */
int bddimply(bddp f, bddp g);

/**
 * @brief Compute the support set as a BDD (disjunction of variables).
 * @param f A BDD node ID.
 * @return A BDD representing the disjunction of all variables in the support.
 */
bddp bddsupport(bddp f);

/**
 * @brief Compute the support set as a vector of variable numbers.
 * @param f A BDD node ID.
 * @return A vector of variable numbers that @p f depends on.
 */
std::vector<bddvar> bddsupport_vec(bddp f);

/**
 * @brief Existential quantification by a cube BDD.
 *
 * Eliminates the variables in @p g (given as a conjunction) from @p f
 * by computing f|v=0 OR f|v=1 for each variable v in the cube.
 *
 * @param f The BDD to quantify.
 * @param g A cube BDD (conjunction of variables).
 * @return The resulting BDD.
 */
bddp bddexist(bddp f, bddp g);

/**
 * @brief Existential quantification by a list of variables.
 * @param f The BDD to quantify.
 * @param vars Variable numbers to quantify out.
 * @return The resulting BDD.
 */
bddp bddexist(bddp f, const std::vector<bddvar>& vars);

/**
 * @brief Existential quantification of a single variable.
 * @param f The BDD to quantify.
 * @param v Variable number to quantify out.
 * @return The resulting BDD.
 */
bddp bddexistvar(bddp f, bddvar v);

/**
 * @brief Universal quantification by a cube BDD.
 *
 * Eliminates the variables in @p g from @p f by computing
 * f|v=0 AND f|v=1 for each variable v in the cube.
 *
 * @param f The BDD to quantify.
 * @param g A cube BDD (conjunction of variables).
 * @return The resulting BDD.
 */
bddp bdduniv(bddp f, bddp g);

/**
 * @brief Universal quantification by a list of variables.
 * @param f The BDD to quantify.
 * @param vars Variable numbers to quantify.
 * @return The resulting BDD.
 */
bddp bdduniv(bddp f, const std::vector<bddvar>& vars);

/**
 * @brief Universal quantification of a single variable.
 * @param f The BDD to quantify.
 * @param v Variable number to quantify.
 * @return The resulting BDD.
 */
bddp bddunivvar(bddp f, bddvar v);

/**
 * @brief BDD left shift: rename variable i to variable i+shift for all variables.
 * @param f A BDD node ID.
 * @param shift The number of positions to shift.
 * @return The resulting BDD.
 */
bddp bddlshiftb(bddp f, bddvar shift);

/**
 * @brief BDD right shift: rename variable i to variable i-shift for all variables.
 * @param f A BDD node ID.
 * @param shift The number of positions to shift.
 * @return The resulting BDD.
 */
bddp bddrshiftb(bddp f, bddvar shift);

/**
 * @brief Generalized cofactor of @p f by @p g.
 *
 * Computes the constrain (generalized cofactor) of f with respect to g.
 *
 * @param f The BDD to cofactor.
 * @param g The constraining BDD (must not be false).
 * @return The generalized cofactor.
 */
bddp bddcofactor(bddp f, bddp g);

/**
 * @brief Swap variables v1 and v2 in a BDD.
 * @param f A BDD node ID.
 * @param v1 First variable number.
 * @param v2 Second variable number.
 * @return The BDD with v1 and v2 swapped.
 */
bddp bddswap(bddp f, bddvar v1, bddvar v2);

/**
 * @brief Smooth (existential quantification) of a single variable.
 * @param f A BDD node ID.
 * @param v Variable number to quantify out.
 * @return The resulting BDD.
 */
bddp bddsmooth(bddp f, bddvar v);

/**
 * @brief Spread variable values to neighboring k levels.
 * @param f A BDD node ID.
 * @param k Number of levels to spread (must be >= 0).
 * @return The resulting BDD.
 */
bddp bddspread(bddp f, int k);

// ZDD operations

/**
 * @brief ZDD offset: select sets NOT containing variable @p var.
 *
 * Returns the subfamily of sets that do not contain @p var.
 *
 * @param f A ZDD node ID.
 * @param var Variable number.
 * @return The resulting ZDD.
 */
bddp bddoffset(bddp f, bddvar var);

/**
 * @brief ZDD onset: select sets containing variable @p var, keeping @p var in the result.
 *
 * Returns the subfamily of sets that contain @p var. The variable @p var
 * is retained in each set (unlike bddonset0 which removes it).
 *
 * @param f A ZDD node ID.
 * @param var Variable number.
 * @return The resulting ZDD.
 */
bddp bddonset(bddp f, bddvar var);

/**
 * @brief ZDD onset0: select sets containing variable @p var and remove @p var (1-cofactor).
 *
 * Returns the subfamily of sets that contain @p var, with @p var removed
 * from each set.
 *
 * @param f A ZDD node ID.
 * @param var Variable number.
 * @return The resulting ZDD.
 */
bddp bddonset0(bddp f, bddvar var);

/**
 * @brief ZDD change: toggle membership of variable @p var in all sets.
 *
 * For each set S in the family, if @p var is in S then remove it,
 * otherwise add it.
 *
 * @param f A ZDD node ID.
 * @param var Variable number.
 * @return The resulting ZDD.
 */
bddp bddchange(bddp f, bddvar var);

/**
 * @brief Union of two ZDD families.
 * @param f First family.
 * @param g Second family.
 * @return The ZDD for f ∪ g, or bddnull if either input is bddnull.
 */
bddp bddunion(bddp f, bddp g);

/**
 * @brief Intersection of two ZDD families.
 * @param f First family.
 * @param g Second family.
 * @return The ZDD for f ∩ g, or bddnull if either input is bddnull.
 */
bddp bddintersec(bddp f, bddp g);

/**
 * @brief Subtraction (set difference) of two ZDD families.
 * @param f First family.
 * @param g Second family.
 * @return The ZDD for f \\ g, or bddnull if either input is bddnull.
 */
bddp bddsubtract(bddp f, bddp g);

/**
 * @brief Division (quotient) of two ZDD families.
 *
 * Returns the family Q such that Q * g ⊆ f, where * is join.
 * The quotient assumes that elements of Q and g are disjoint
 * (non-overlapping).
 *
 * @param f Dividend family.
 * @param g Divisor family.
 * @return The quotient ZDD.
 */
bddp bdddiv(bddp f, bddp g);

/**
 * @brief Symmetric difference of two ZDD families.
 * @param f First family.
 * @param g Second family.
 * @return The ZDD for f △ g (union minus intersection).
 */
bddp bddsymdiff(bddp f, bddp g);

/**
 * @brief Join (cross product with union of elements) of two ZDD families.
 *
 * For each pair (A, B) where A ∈ f and B ∈ g, include A ∪ B in the result.
 *
 * @param f First family.
 * @param g Second family.
 * @return The resulting ZDD.
 */
bddp bddjoin(bddp f, bddp g);

/**
 * @brief Meet (cross product with intersection of elements) of two ZDD families.
 *
 * For each pair (A, B) where A ∈ f and B ∈ g, include A ∩ B in the result.
 *
 * @param f First family.
 * @param g Second family.
 * @return The resulting ZDD.
 */
bddp bddmeet(bddp f, bddp g);

/**
 * @brief Delta operation on two ZDD families.
 *
 * For each pair (A, B) where A ∈ f and B ∈ g, include the symmetric
 * difference A △ B in the result.
 *
 * @param f First family.
 * @param g Second family.
 * @return The resulting ZDD.
 */
bddp bdddelta(bddp f, bddp g);

/**
 * @brief Remainder (modulo) of two ZDD families.
 *
 * Returns f - (f / g) * g, where / is division and * is join.
 *
 * @param f Dividend family.
 * @param g Divisor family.
 * @return The remainder ZDD.
 */
bddp bddremainder(bddp f, bddp g);

/**
 * @brief Disjoint product of two ZDD families.
 *
 * For each pair (A, B) where A ∈ f and B ∈ g, if A ∩ B = ∅,
 * include A ∪ B in the result.
 *
 * @param f First family.
 * @param g Second family.
 * @return The resulting ZDD.
 */
bddp bdddisjoin(bddp f, bddp g);

/**
 * @brief Joint join of two ZDD families.
 *
 * For each pair (A, B) where A ∈ f and B ∈ g with A ∩ B ≠ ∅,
 * include A ∪ B in the result. Pairs with no overlap are excluded.
 *
 * @param f First family.
 * @param g Second family.
 * @return The resulting ZDD.
 */
bddp bddjointjoin(bddp f, bddp g);

/**
 * @brief Restrict to sets that are subsets of some set in @p g.
 *
 * Returns the subfamily of f containing only those sets S such that
 * S ⊆ T for some T ∈ g.
 *
 * @param f The family to restrict.
 * @param g The constraining family.
 * @return The restricted ZDD.
 */
bddp bddrestrict(bddp f, bddp g);

/**
 * @brief Keep sets whose elements are all permitted by @p g.
 * @param f The family to filter.
 * @param g The permitting family.
 * @return The resulting ZDD.
 */
bddp bddpermit(bddp f, bddp g);

/**
 * @brief Remove supersets: remove sets that are supersets of some set in @p g.
 * @param f The family to filter.
 * @param g The constraining family.
 * @return The resulting ZDD.
 */
bddp bddnonsup(bddp f, bddp g);

/**
 * @brief Remove subsets: remove sets that are subsets of some set in @p g.
 * @param f The family to filter.
 * @param g The constraining family.
 * @return The resulting ZDD.
 */
bddp bddnonsub(bddp f, bddp g);

// Unary ZDD operations

/**
 * @brief Extract maximal sets (no proper superset in the family).
 * @param f A ZDD node ID.
 * @return A ZDD containing only the maximal sets of @p f.
 */
bddp bddmaximal(bddp f);

/**
 * @brief Extract minimal sets (no proper subset in the family).
 * @param f A ZDD node ID.
 * @return A ZDD containing only the minimal sets of @p f.
 */
bddp bddminimal(bddp f);

/**
 * @brief Compute minimum hitting sets.
 *
 * A hitting set intersects every set in the family. Returns the
 * family of all inclusion-minimal hitting sets.
 *
 * @param f A ZDD node ID.
 * @return A ZDD of minimal hitting sets.
 */
bddp bddminhit(bddp f);

/**
 * @brief Compute the downward closure.
 *
 * Returns all subsets of sets in the family.
 *
 * @param f A ZDD node ID.
 * @return A ZDD representing the downward closure.
 */
bddp bddclosure(bddp f);

/**
 * @brief Push a variable onto a ZDD node.
 *
 * Creates a new ZDD node with 0-edge = bddempty and 1-edge = f.
 * No level ordering check is performed (for Sequence BDD support).
 *
 * @param f A ZDD node ID.
 * @param v Variable number to push.
 * @return The resulting ZDD node ID.
 */
bddp bddpush(bddp f, bddvar v);

/**
 * @brief ZDD left shift: rename variable i to variable i+shift for all variables.
 * @param f A ZDD node ID.
 * @param shift The number of positions to shift.
 * @return The resulting ZDD.
 */
bddp bddlshiftz(bddp f, bddvar shift);

/**
 * @brief ZDD right shift: rename variable i to variable i-shift for all variables.
 * @param f A ZDD node ID.
 * @param shift The number of positions to shift.
 * @return The resulting ZDD.
 */
bddp bddrshiftz(bddp f, bddvar shift);

/**
 * @brief @deprecated Use bddlshiftb or bddlshiftz instead.
 */
bddp bddlshift(bddp f, bddvar shift);

/**
 * @brief @deprecated Use bddrshiftb or bddrshiftz instead.
 */
bddp bddrshift(bddp f, bddvar shift);

// ZDD counting

/**
 * @brief Count the number of sets in a ZDD family.
 *
 * @deprecated Use bddexactcount() instead.
 * Saturates at (2^39 - 1) for very large families.
 *
 * @param f A ZDD node ID.
 * @return The cardinality of the family, or (2^39 - 1) if saturated.
 */
uint64_t bddcard(bddp f);

/**
 * @brief Count the number of sets in a ZDD family (double approximation).
 *
 * Returns the cardinality as a double. For families whose cardinality
 * exceeds the precision of double (~2^53), the result is approximate.
 *
 * @param f A ZDD node ID.
 * @return The cardinality of the family as a double.
 */
double bddcount(bddp f);

/**
 * @brief Return the total literal count of a ZDD family.
 *
 * Sums the sizes of all sets in the family represented by @p f.
 * For example, if f = {{a,b},{a},{b,c,d}}, returns 2+1+3 = 6.
 *
 * @param f A ZDD node ID.
 * Saturates at (2^39 - 1) for very large families. Use bddexactlit()
 * for exact results beyond this limit.
 *
 * @return The total literal count, or (2^39 - 1) if saturated.
 */
uint64_t bddlit(bddp f);

/**
 * @brief Return the maximum set size in a ZDD family.
 *
 * Returns the size of the largest set in the family represented by @p f.
 * For example, if f = {{a,b},{a},{b,c,d}}, returns max(2,1,3) = 3.
 *
 * @param f A ZDD node ID.
 * Saturates at (2^39 - 1) for very deep families.
 *
 * @return The maximum set size, or 0 for the empty family or {∅}.
 *         Returns (2^39 - 1) if saturated.
 */
uint64_t bddlen(bddp f);

/**
 * @brief Check if the empty set (∅) is a member of a ZDD family.
 *
 * Returns true if ∅ ∈ F, i.e. the family represented by @p f
 * contains the empty set. Correctly handles complement edges.
 *
 * @param f A ZDD node ID.
 * @return true if the empty set is in the family, false otherwise.
 */
bool bddhasempty(bddp f);

/**
 * @brief Count the number of sets in a ZDD family (arbitrary precision).
 *
 * Same computation as bddcard, but returns a BigInt so the result
 * is exact regardless of cardinality.
 *
 * @param f A ZDD node ID.
 * @return The cardinality of the family as a BigInt.
 */
bigint::BigInt bddexactcount(bddp f);

/**
 * @brief Count the number of sets in a ZDD family (arbitrary precision),
 *        using an external memo table.
 *
 * The memo table is both read and written: entries already present are
 * reused, and newly computed entries are added.
 *
 * @param f A ZDD node ID.
 * @param memo The memo table to use and populate.
 * @return The cardinality of the family as a BigInt.
 */
bigint::BigInt bddexactcount(bddp f, CountMemoMap& memo);

/**
 * @brief Count the number of sets in a ZDD family as a hex string.
 *
 * @deprecated Use bddexactcount() instead.
 * Legacy compatibility wrapper around bddexactcount.
 * Returns the cardinality as an uppercase hexadecimal string with no
 * leading zeros.
 *
 * @param f A ZDD node ID.
 * @param s If non-NULL, the result is written into this buffer. The caller
 *          must ensure the buffer is large enough (no size check is performed).
 *          Prefer passing NULL to let the function allocate, or use
 *          bddexactcount() which returns a BigInt. If NULL, a new buffer
 *          is allocated with malloc (caller must free).
 * @return Pointer to the hex string (either @p s or a malloc'd buffer).
 */
char *bddcardmp16(bddp f, char *s);

// BDD satisfiability counting

/**
 * @brief Count the number of satisfying assignments of a BDD.
 *
 * Given a BDD representing a Boolean function f over variables 1..n,
 * returns the number of assignments (x1,...,xn) in {0,1}^n where
 * f(x1,...,xn) = 1.
 *
 * Variables numbered 1..n that do not appear in the BDD are treated
 * as don't-cares (each doubles the count). Throws std::invalid_argument
 * if any BDD node has a variable number > n.
 *
 * @param f A BDD node ID.
 * @param n The number of variables (1..n).
 * @return The satisfying assignment count as a double.
 */
double bddcount(bddp f, bddvar n);

/**
 * @brief Count the number of satisfying assignments of a BDD (exact).
 *
 * Same as bddcount(bddp, bddvar) but returns an exact BigInt result.
 *
 * @param f A BDD node ID.
 * @param n The number of variables (1..n).
 * @return The satisfying assignment count as a BigInt.
 */
bigint::BigInt bddexactcount(bddp f, bddvar n);

/**
 * @brief Count the number of satisfying assignments of a BDD (exact),
 *        using an external memo table.
 *
 * Same as bddexactcount(bddp, bddvar) but uses and populates an external
 * memo table for caching intermediate results.
 *
 * @param f A BDD node ID.
 * @param n The number of variables (1..n).
 * @param memo The memo table to use and populate.
 * @return The satisfying assignment count as a BigInt.
 */
bigint::BigInt bddexactcount(bddp f, bddvar n, CountMemoMap& memo);

/**
 * @brief Generate a random ZDD over the lowest @p lev levels.
 *
 * Recursively builds a random family of sets. Each terminal is
 * independently set to 1 with probability @p density / 100.
 *
 * @param lev Number of variable levels to use (1..bdd_varcount).
 * @param density Probability (0–100) for each terminal to be 1. Default 50.
 * @return A random ZDD.
 */
ZDD ZDD_Random(int lev, int density = 50);

/**
 * @brief Check if a ZDD represents a polynomial (family with ≥ 2 sets).
 * @param f A ZDD node ID.
 * @return 1 if the family has 2 or more sets, 0 otherwise, or -1 on error.
 */
int bddispoly(bddp f);

/**
 * @brief Swap two variables in a ZDD family.
 * @param f A ZDD node ID.
 * @param v1 First variable number.
 * @param v2 Second variable number.
 * @return A ZDD with v1 and v2 swapped.
 */
bddp bddswapz(bddp f, bddvar v1, bddvar v2);

/**
 * @brief Check if v1 implies v2 in a ZDD family.
 * @param f A ZDD node ID.
 * @param v1 First variable number.
 * @param v2 Second variable number.
 * @return 1 if every set containing v1 also contains v2, 0 otherwise, -1 on error.
 */
int bddimplychk(bddp f, bddvar v1, bddvar v2);

/**
 * @brief Check co-implication between v1 and v2 in a ZDD family.
 * @param f A ZDD node ID.
 * @param v1 First variable number.
 * @param v2 Second variable number.
 * @return 1 if co-implication holds, 0 otherwise, -1 on error.
 */
int bddcoimplychk(bddp f, bddvar v1, bddvar v2);

/**
 * @brief Symmetric permit: keep sets with at most n elements.
 * @param f A ZDD node ID.
 * @param n Maximum number of elements.
 * @return A ZDD containing only sets with ≤ n elements.
 */
bddp bddpermitsym(bddp f, int n);

/**
 * @brief Find elements common to ALL sets in a ZDD family.
 * @param f A ZDD node ID.
 * @return A ZDD family of singletons, one for each always-present variable.
 *         For example, if f = {{1,2,3},{1,2}}, returns {{1},{2}}.
 */
bddp bddalways(bddp f);

/**
 * @brief Check if two variables are symmetric in a ZDD family.
 * @param f A ZDD node ID.
 * @param v1 First variable number.
 * @param v2 Second variable number.
 * @return 1 if symmetric, 0 if not, -1 on error.
 */
int bddsymchk(bddp f, bddvar v1, bddvar v2);

/**
 * @brief Find all variables implied by v in a ZDD family.
 * @param f A ZDD node ID.
 * @param v Variable number.
 * @return A ZDD (family of singletons) of variables that v implies.
 */
bddp bddimplyset(bddp f, bddvar v);

/**
 * @brief Find all symmetry groups (size ≥ 2) in a ZDD family.
 * @param f A ZDD node ID.
 * @return A ZDD family where each set is a symmetry group.
 */
bddp bddsymgrp(bddp f);

/**
 * @brief Find all symmetry groups (naive method, includes size 1).
 * @param f A ZDD node ID.
 * @return A ZDD family where each set is a symmetry group.
 */
bddp bddsymgrpnaive(bddp f);

/**
 * @brief Find all variables symmetric with v in a ZDD family.
 * @param f A ZDD node ID.
 * @param v Variable number.
 * @return A ZDD (single set) of variables symmetric with v.
 */
bddp bddsymset(bddp f, bddvar v);

/**
 * @brief Find all variables in co-implication relation with v.
 * @param f A ZDD node ID.
 * @param v Variable number.
 * @return A ZDD (single set) of variables co-implied by v.
 */
bddp bddcoimplyset(bddp f, bddvar v);

/**
 * @brief Find a non-trivial divisor of a ZDD family (as polynomial).
 * @param f A ZDD node ID.
 * @return A ZDD representing a divisor, or bddsingle if f is monomial.
 */
bddp bdddivisor(bddp f);

/** @brief LCM algorithm (all frequent itemsets). */
ZDD ZDD_LCM_A(char* filename, int threshold);
/** @brief LCM algorithm (closed frequent itemsets). */
ZDD ZDD_LCM_C(char* filename, int threshold);
/** @brief LCM algorithm (maximal frequent itemsets). */
ZDD ZDD_LCM_M(char* filename, int threshold);

#endif
