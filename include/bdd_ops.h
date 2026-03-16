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
 * @brief Compute the support set as a BDD (conjunction of variables).
 * @param f A BDD node ID.
 * @return A BDD representing the conjunction of all variables in the support.
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
 * @brief Left shift: rename variable i to variable i+shift for all variables.
 * @param f A BDD node ID.
 * @param shift The number of positions to shift.
 * @return The resulting BDD.
 */
bddp bddlshift(bddp f, bddvar shift);

/**
 * @brief Right shift: rename variable i to variable i-shift for all variables.
 * @param f A BDD node ID.
 * @param shift The number of positions to shift.
 * @return The resulting BDD.
 */
bddp bddrshift(bddp f, bddvar shift);

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
 * @brief ZDD onset: select sets containing variable @p var, then remove @p var.
 *
 * Returns the subfamily of sets that contain @p var, with @p var removed
 * from each set.
 *
 * @param f A ZDD node ID.
 * @param var Variable number.
 * @return The resulting ZDD.
 */
bddp bddonset(bddp f, bddvar var);

/**
 * @brief ZDD onset0: select sets NOT containing variable @p var (same as offset).
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
 * For each pair (A, B) where A ∈ f and B ∈ g, include A ∪ B in the result
 * (regardless of whether A and B overlap).
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

// ZDD counting

/**
 * @brief Count the number of sets in a ZDD family.
 * @param f A ZDD node ID.
 * @return The cardinality of the family.
 */
uint64_t bddcard(bddp f);

/**
 * @brief Return the total literal count of a ZDD family.
 *
 * Sums the sizes of all sets in the family represented by @p f.
 * For example, if f = {{a,b},{a},{b,c,d}}, returns 2+1+3 = 6.
 *
 * @param f A ZDD node ID.
 * @return The total literal count.
 */
uint64_t bddlit(bddp f);

/**
 * @brief Return the maximum set size in a ZDD family.
 *
 * Returns the size of the largest set in the family represented by @p f.
 * For example, if f = {{a,b},{a},{b,c,d}}, returns max(2,1,3) = 3.
 *
 * @param f A ZDD node ID.
 * @return The maximum set size, or 0 for the empty family or {∅}.
 */
uint64_t bddlen(bddp f);

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
 * @brief Count the number of sets in a ZDD family as a hex string.
 *
 * Legacy compatibility wrapper around bddexactcount.
 * Returns the cardinality as an uppercase hexadecimal string with no
 * leading zeros.
 *
 * @param f A ZDD node ID.
 * @param s If non-NULL, the result is written into this buffer (caller is
 *          responsible for ensuring sufficient size). If NULL, a new buffer
 *          is allocated with malloc.
 * @return Pointer to the hex string (either @p s or a malloc'd buffer).
 */
char *bddcardmp16(bddp f, char *s);

#endif
