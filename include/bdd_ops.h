#ifndef KYOTODD_BDD_OPS_H
#define KYOTODD_BDD_OPS_H

#include "bdd_types.h"
#include "bigint.hpp"

namespace kyotodd {

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
 * @brief Logical AND of two BDDs with execution mode selection.
 * @param f First operand.
 * @param g Second operand.
 * @param mode Execution mode (Recursive, Iterative, or Auto).
 *             Auto selects recursive if max operand level <= BDD_RecurLimit, else iterative.
 * @return The BDD for f AND g, or bddnull if either input is bddnull.
 */
bddp bddand(bddp f, bddp g, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddand.
 *
 * Uses an explicit heap stack. No recursion depth limit.
 * Inputs must be pre-validated and normalized (f <= g, no null/terminal fast-paths).
 * @param f First operand.
 * @param g Second operand.
 * @return The BDD for f AND g.
 */
bddp bddand_iter(bddp f, bddp g);

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
 * @brief Existential quantification by a variable-set BDD.
 *
 * Eliminates the variables in @p g from @p f
 * by computing f|v=0 OR f|v=1 for each variable v in the set.
 *
 * @param f The BDD to quantify.
 * @param g A variable-set BDD (as returned by bddsupport()).
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
 * @brief Universal quantification by a variable-set BDD.
 *
 * Eliminates the variables in @p g from @p f by computing
 * f|v=0 AND f|v=1 for each variable v in the set.
 *
 * @param f The BDD to quantify.
 * @param g A variable-set BDD (as returned by bddsupport()).
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
 * @brief Cross product of two ZDD families over disjoint variable sets.
 *
 * For each pair (A, B) where A ∈ f and B ∈ g, include A ∪ B in the result.
 * The two families must use disjoint variable sets (behavior is undefined otherwise).
 * More efficient than bddjoin when variables are known to be disjoint.
 *
 * @param f First family.
 * @param g Second family.
 * @return The resulting ZDD.
 */
bddp bddproduct(bddp f, bddp g);

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
 * Saturates at (2^39 - 1) for very large families.
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
 * @brief Return the minimum set size in a ZDD family.
 *
 * Returns the size of the smallest set in the family represented by @p f.
 * For example, if f = {{a,b},{a},{b,c,d}}, returns min(2,1,3) = 1.
 *
 * @param f A ZDD node ID.
 *
 * @return The minimum set size, or 0 for the empty family or {∅}.
 */
uint64_t bddminsize(bddp f);

/**
 * @brief Return the maximum set size in a ZDD family.
 *
 * Equivalent to bddlen(). Provided for symmetry with bddminsize().
 *
 * @param f A ZDD node ID.
 *
 * @return The maximum set size, or 0 for the empty family or {∅}.
 *         Returns (2^39 - 1) if saturated.
 */
uint64_t bddmaxsize(bddp f);

/**
 * @brief Check if two ZDD families are disjoint (F ∩ G = ∅).
 *
 * Returns true if f and g have no common set. Uses early termination:
 * returns false as soon as a common set is found.
 *
 * @param f A ZDD node ID.
 * @param g A ZDD node ID.
 *
 * @return true if F ∩ G = ∅, false otherwise.
 */
bool bddisdisjoint(bddp f, bddp g);

/**
 * @brief Count the number of sets in the intersection of two ZDD families.
 *
 * Computes |F ∩ G| without materializing the intersection ZDD.
 * More memory-efficient than bddintersec followed by bddexactcount.
 *
 * @param f A ZDD node ID.
 * @param g A ZDD node ID.
 * @return |F ∩ G| as a BigInt.
 */
bigint::BigInt bddcountintersec(bddp f, bddp g);

/**
 * @brief Compute the Jaccard index of two ZDD families.
 *
 * J(F, G) = |F ∩ G| / |F ∪ G|.
 * Uses count_intersec internally; does not build intermediate ZDDs.
 * Returns 1.0 when both families are empty (by convention).
 *
 * @param f A ZDD node ID.
 * @param g A ZDD node ID.
 * @return The Jaccard index in [0, 1], or 0.0 if either is null.
 */
double bddjaccardindex(bddp f, bddp g);

/**
 * @brief Compute the Hamming distance (symmetric difference size) of two ZDD families.
 *
 * |F △ G| = |F| + |G| - 2|F ∩ G|.
 *
 * @param f A ZDD node ID.
 * @param g A ZDD node ID.
 * @return |F △ G| as a BigInt, or 0 if either is null.
 */
bigint::BigInt bddhammingdist(bddp f, bddp g);

/**
 * @brief Compute the overlap coefficient of two ZDD families.
 *
 * O(F, G) = |F ∩ G| / min(|F|, |G|).
 * Returns 1.0 when both are empty, 0.0 if either is null.
 *
 * @param f A ZDD node ID.
 * @param g A ZDD node ID.
 * @return The overlap coefficient in [0, 1].
 */
double bddoverlapcoeff(bddp f, bddp g);

/**
 * @brief Extract supersets of a given set from a ZDD family.
 * Returns all sets A in f where s ⊆ A.
 */
bddp bddsupersets_of(bddp f, const std::vector<bddvar>& s);

/**
 * @brief Extract subsets of a given set from a ZDD family.
 * Returns all sets A in f where A ⊆ s.
 */
bddp bddsubsets_of(bddp f, const std::vector<bddvar>& s);

/**
 * @brief Project (remove) specified variables from all sets in a ZDD family.
 */
bddp bddproject(bddp f, const std::vector<bddvar>& vars);

/**
 * @brief Coalesce (merge) two variables in a ZDD family.
 *
 * Merges variable @p v2 into @p v1. In each set of the family,
 * if v1 or v2 (or both) is present, the result set contains v1;
 * v2 is removed. Duplicate sets are eliminated.
 *
 * @param f A ZDD node ID.
 * @param v1 The surviving variable.
 * @param v2 The variable to merge into v1 (eliminated).
 * @return A ZDD node ID representing the coalesced family.
 */
bddp bddcoalesce(bddp f, bddvar v1, bddvar v2);

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
 * @brief Check if a specific set is a member of the ZDD family.
 *
 * Tests whether the set @p s belongs to the family represented by @p f.
 * Traverses the ZDD iteratively in O(|s| + depth) time.
 *
 * @param f A ZDD node ID.
 * @param s The set to test (variable numbers). Duplicates and order are ignored.
 * @return true if @p s is in the family, false otherwise.
 */
bool bddcontains(bddp f, const std::vector<bddvar>& s);

/**
 * @brief Check if one ZDD family is a subset of another.
 *
 * Returns true if every set in @p f is also in @p g (i.e. f ⊆ g).
 * More efficient than `bddsubtract(f, g) == bddempty` because it
 * can terminate early as soon as a non-member is found.
 *
 * @param f The candidate subset family.
 * @param g The candidate superset family.
 * @return true if f ⊆ g.
 */
bool bddissubset(bddp f, bddp g);

/**
 * @brief Return the union of all sets in the family as a single-set ZDD.
 *
 * For F = {S1, S2, ...}, returns {{S1 ∪ S2 ∪ ...}}.
 * Returns null for null input, empty for empty family.
 *
 * @param f A ZDD node ID.
 * @return A ZDD node ID representing {{union of all sets}}.
 */
bddp bddflatten(bddp f);

/**
 * @brief Filter to sets of exactly k elements.
 *
 * Returns a ZDD containing only those sets in the family represented
 * by @p f that have exactly @p k elements.
 *
 * @param f A ZDD node ID.
 * @param k Required set size (must be >= 0).
 * @return A ZDD node ID representing the filtered family.
 */
bddp bddchoose(bddp f, int k);

/**
 * @brief Return the set size distribution of a ZDD family (arbitrary precision).
 *
 * Returns a vector where result[i] is the number of sets with exactly i elements.
 * The vector length is max_set_size + 1.
 *
 * @param f A ZDD node ID.
 * @return The profile vector (empty for null or empty family).
 */
std::vector<bigint::BigInt> bddprofile(bddp f);

/**
 * @brief Return the set size distribution of a ZDD family (double precision).
 *
 * Same as bddprofile but returns doubles for efficiency with small families.
 *
 * @param f A ZDD node ID.
 * @return The profile vector (empty for null or empty family).
 */
std::vector<double> bddprofile_double(bddp f);

/**
 * @brief Return the element frequency of a ZDD family (arbitrary precision).
 *
 * For each variable v, result[v] is the number of sets in the family
 * that contain v. The vector length is max_variable + 1.
 *
 * @param f A ZDD node ID.
 * @return The frequency vector (empty for null, empty, or unit family).
 */
std::vector<bigint::BigInt> bddelmfreq(bddp f);

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
 * @warning When @p s is non-NULL, the caller must ensure the buffer is
 *          large enough. No buffer size validation is performed, which
 *          may cause buffer overflows. Prefer bddexactcount() instead.
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
 * @brief Find an approximate divisor of a ZDD family (as polynomial).
 *
 * Uses a greedy heuristic to find a non-trivial divisor. The result
 * is not guaranteed to be optimal (i.e., the best possible divisor).
 *
 * @param f A ZDD node ID.
 * @return A ZDD representing a divisor, or bddsingle if f is monomial.
 */
bddp bdddivisor(bddp f);

/**
 * @brief Compute the total weight sum over all sets in a ZDD family.
 *
 * For each set S in the family, computes sum(weights[v] for v in S),
 * then returns the total of all such sums as a BigInt.
 *
 * @param f A ZDD node ID.
 * @param weights Weight vector indexed by variable number.
 *                Size must be > top variable of f.
 * @return The total weight sum.
 * @throws std::invalid_argument if f is bddnull or weights is too small.
 */
bigint::BigInt bddweightsum(bddp f, const std::vector<int>& weights);

/**
 * @brief Find the minimum weight sum among all sets in a ZDD family.
 *
 * Each variable v has weight weights[v]. Returns the minimum of
 * sum(weights[v] for v in S) over all sets S in the family.
 *
 * @param f A ZDD node ID (must not be bddempty or bddnull).
 * @param weights Weight vector indexed by variable number.
 *                Size must be > bdd_varcount.
 * @return The minimum weight sum.
 * @throws std::invalid_argument if f is empty/null or weights is too small.
 */
long long bddminweight(bddp f, const std::vector<int>& weights);

/**
 * @brief Find the maximum weight sum among all sets in a ZDD family.
 *
 * @param f A ZDD node ID (must not be bddempty or bddnull).
 * @param weights Weight vector indexed by variable number.
 *                Size must be > bdd_varcount.
 * @return The maximum weight sum.
 * @throws std::invalid_argument if f is empty/null or weights is too small.
 */
long long bddmaxweight(bddp f, const std::vector<int>& weights);

/**
 * @brief Find a set with the minimum weight sum in a ZDD family.
 *
 * @param f A ZDD node ID (must not be bddempty or bddnull).
 * @param weights Weight vector indexed by variable number.
 *                Size must be > bdd_varcount.
 * @return A vector of variable numbers forming a set with minimum weight sum.
 * @throws std::invalid_argument if f is empty/null or weights is too small.
 */
std::vector<bddvar> bddminweightset(bddp f, const std::vector<int>& weights);

/**
 * @brief Find a set with the maximum weight sum in a ZDD family.
 *
 * @param f A ZDD node ID (must not be bddempty or bddnull).
 * @param weights Weight vector indexed by variable number.
 *                Size must be > bdd_varcount.
 * @return A vector of variable numbers forming a set with maximum weight sum.
 * @throws std::invalid_argument if f is empty/null or weights is too small.
 */
std::vector<bddvar> bddmaxweightset(bddp f, const std::vector<int>& weights);

/**
 * @brief Extract cost-bounded solutions from a ZDD using BkTrk-IntervalMemo.
 *
 * Constructs a ZDD h = {X ∈ S_f | Cost(X) ≤ b} where Cost(X) = Σ weights[v]
 * for v ∈ X. Uses interval-memoizing technique for efficient caching.
 *
 * @param f A ZDD node ID.
 * @param weights Cost vector indexed by variable number (1-based).
 *                Size must be > bdd_varcount.
 * @param b Cost bound. Solutions with cost ≤ b are included.
 * @param memo Interval-memoization table.
 * @return A ZDD node ID representing all cost-bounded solutions.
 * @throws std::invalid_argument if f is null or weights is too small.
 */
bddp bddcostbound_le(bddp f, const std::vector<int>& weights, long long b,
                      CostBoundMemo& memo);

/**
 * @brief Cost-bound filtering (>=) via BkTrk-IntervalMemo.
 *
 * Constructs a ZDD h = {X ∈ S_f | Cost(X) ≥ b}.
 * Implemented by negating weights and calling bddcostbound_le.
 *
 * @param f A ZDD node ID.
 * @param weights Cost vector indexed by variable number (1-based).
 *                Size must be > bdd_varcount.
 * @param b Cost bound. Solutions with cost ≥ b are included.
 * @param memo Interval-memoization table. Note: internally binds to
 *             negated weights, so cannot share a memo with bddcostbound_le.
 * @return A ZDD node ID representing all solutions with cost >= b.
 * @throws std::invalid_argument if f is null or weights is too small.
 */
bddp bddcostbound_ge(bddp f, const std::vector<int>& weights, long long b,
                      CostBoundMemo& memo);

// ZDD ranking / unranking

/**
 * @brief Rank a set in the ZDD family (int64_t version).
 *
 * Returns the 0-based index of set @p s in the ZDD family's
 * structure-based ordering (∅ first, then hi-edge sets before lo-edge sets
 * at each node).
 *
 * @param f A ZDD node ID.
 * @param s The set to rank (variable numbers). Duplicates and order are ignored.
 * @return The rank of the set, or -1 if @p s is not in the family.
 * @throws std::invalid_argument if f is bddnull.
 * @throws std::overflow_error if the rank exceeds int64_t range.
 */
int64_t bddrank(bddp f, const std::vector<bddvar>& s);

/**
 * @brief Rank a set in the ZDD family (arbitrary precision).
 *
 * @param f A ZDD node ID.
 * @param s The set to rank (variable numbers). Duplicates and order are ignored.
 * @return The rank of the set as a BigInt, or BigInt(-1) if not in the family.
 * @throws std::invalid_argument if f is bddnull.
 */
bigint::BigInt bddexactrank(bddp f, const std::vector<bddvar>& s);

/**
 * @brief Rank a set in the ZDD family (arbitrary precision, with memo).
 *
 * @param f A ZDD node ID.
 * @param s The set to rank (variable numbers). Duplicates and order are ignored.
 * @param memo The count memo table to use and populate.
 * @return The rank of the set as a BigInt, or BigInt(-1) if not in the family.
 * @throws std::invalid_argument if f is bddnull.
 */
bigint::BigInt bddexactrank(bddp f, const std::vector<bddvar>& s,
                            CountMemoMap& memo);

/**
 * @brief Unrank: retrieve the set at a given index in the ZDD family (int64_t).
 *
 * Returns the set at position @p order in the ZDD family's structure-based
 * ordering.
 *
 * @param f A ZDD node ID.
 * @param order The 0-based index.
 * @return The set as a sorted vector of variable numbers.
 * @throws std::invalid_argument if f is bddnull.
 * @throws std::out_of_range if @p order is negative or >= family size.
 */
std::vector<bddvar> bddunrank(bddp f, int64_t order);

/**
 * @brief Unrank: retrieve the set at a given index (arbitrary precision).
 *
 * @param f A ZDD node ID.
 * @param order The 0-based index as a BigInt.
 * @return The set as a sorted vector of variable numbers.
 * @throws std::invalid_argument if f is bddnull.
 * @throws std::out_of_range if @p order is out of range.
 */
std::vector<bddvar> bddexactunrank(bddp f, const bigint::BigInt& order);

/**
 * @brief Unrank: retrieve the set at a given index (arbitrary precision, with memo).
 *
 * @param f A ZDD node ID.
 * @param order The 0-based index as a BigInt.
 * @param memo The count memo table to use and populate.
 * @return The set as a sorted vector of variable numbers.
 * @throws std::invalid_argument if f is bddnull.
 * @throws std::out_of_range if @p order is out of range.
 */
std::vector<bddvar> bddexactunrank(bddp f, const bigint::BigInt& order,
                                   CountMemoMap& memo);

// ZDD get_k_sets

/**
 * @brief Return the first k sets from a ZDD family in structure order (int64_t).
 *
 * Structure order: empty set (if present), then hi-edge sets before lo-edge
 * sets at each node. This is the same ordering used by rank/unrank.
 *
 * @param f A ZDD node ID.
 * @param k Number of sets to extract (must be >= 0).
 * @return A ZDD node ID representing the first k sets.
 * @throws std::invalid_argument if f is null or k is negative.
 */
bddp bddgetksets(bddp f, int64_t k);

/**
 * @brief Return the first k sets from a ZDD family in structure order (BigInt).
 *
 * @param f A ZDD node ID.
 * @param k Number of sets to extract (must be >= 0).
 * @return A ZDD node ID representing the first k sets.
 * @throws std::invalid_argument if f is null or k is negative.
 */
bddp bddgetksets(bddp f, const bigint::BigInt& k);

/**
 * @brief Return the first k sets, reusing a count memo.
 *
 * @param f A ZDD node ID.
 * @param k Number of sets to extract (must be >= 0).
 * @param memo Count memo table to use and populate.
 * @return A ZDD node ID representing the first k sets.
 * @throws std::invalid_argument if f is null or k is negative.
 */
bddp bddgetksets(bddp f, const bigint::BigInt& k, CountMemoMap& memo);

// ZDD get_k_lightest / get_k_heaviest

/**
 * @brief Return the k lightest sets from a ZDD family (int64_t).
 *
 * Uses binary search on cost bounds to find the k sets with smallest
 * total weight. Tie-breaking at the boundary cost tier is controlled
 * by the @p strict parameter.
 *
 * @param f A ZDD node ID.
 * @param k Number of sets to extract (must be >= 0).
 * @param weights Cost vector indexed by variable number. Size must be > bddvarused().
 * @param strict Tie-breaking: 0 = exactly k (structure order), <0 = fewer (all lighter), >0 = more (includes full tier).
 * @return A ZDD node ID representing the selected sets.
 * @throws std::invalid_argument if f is null, k is negative, or weights is too small.
 */
bddp bddgetklightest(bddp f, int64_t k,
                      const std::vector<int>& weights, int strict);

/**
 * @brief Return the k lightest sets from a ZDD family (BigInt).
 *
 * @param f A ZDD node ID.
 * @param k Number of sets to extract (must be >= 0).
 * @param weights Cost vector indexed by variable number. Size must be > bddvarused().
 * @param strict Tie-breaking: 0 = exactly k, <0 = fewer, >0 = more.
 * @return A ZDD node ID representing the selected sets.
 * @throws std::invalid_argument if f is null, k is negative, or weights is too small.
 */
bddp bddgetklightest(bddp f, const bigint::BigInt& k,
                      const std::vector<int>& weights, int strict);

/**
 * @brief Return the k heaviest sets from a ZDD family (int64_t).
 *
 * Implemented as f - get_k_lightest(f, |F|-k, weights, -strict).
 *
 * @param f A ZDD node ID.
 * @param k Number of sets to extract (must be >= 0).
 * @param weights Cost vector indexed by variable number. Size must be > bddvarused().
 * @param strict Tie-breaking: 0 = exactly k, <0 = fewer (all heavier), >0 = more (includes full tier).
 * @return A ZDD node ID representing the selected sets.
 * @throws std::invalid_argument if f is null, k is negative, or weights is too small.
 */
bddp bddgetkheaviest(bddp f, int64_t k,
                      const std::vector<int>& weights, int strict);

/**
 * @brief Return the k heaviest sets from a ZDD family (BigInt).
 *
 * @param f A ZDD node ID.
 * @param k Number of sets to extract (must be >= 0).
 * @param weights Cost vector indexed by variable number. Size must be > bddvarused().
 * @param strict Tie-breaking: 0 = exactly k, <0 = fewer, >0 = more.
 * @return A ZDD node ID representing the selected sets.
 * @throws std::invalid_argument if f is null, k is negative, or weights is too small.
 */
bddp bddgetkheaviest(bddp f, const bigint::BigInt& k,
                      const std::vector<int>& weights, int strict);

/** @brief LCM algorithm (all frequent itemsets). */
ZDD ZDD_LCM_A(char* filename, int threshold);
/** @brief LCM algorithm (closed frequent itemsets). */
ZDD ZDD_LCM_C(char* filename, int threshold);
/** @brief LCM algorithm (maximal frequent itemsets). */
ZDD ZDD_LCM_M(char* filename, int threshold);

} // namespace kyotodd

#endif
