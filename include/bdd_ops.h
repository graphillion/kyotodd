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
 * Advanced API. Uses an explicit heap stack; no recursion depth limit.
 * Inputs must be pre-validated and normalized (f != bddnull, g != bddnull,
 * no null/terminal fast-paths; operands already ordered f <= g if the caller
 * wants the cache-normalized form). Must run inside a @c bdd_gc_guard scope:
 * intermediate @c bddp values on the iteration stack are not registered as
 * GC roots, so GC must be deferred for the duration of this call. The public
 * @ref bddand wrappers satisfy these preconditions.
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
 * @brief Logical XOR of two BDDs with execution mode selection.
 * @param f First operand.
 * @param g Second operand.
 * @param mode Execution mode (Recursive, Iterative, or Auto).
 *             Auto selects recursive if max operand level <= BDD_RecurLimit, else iterative.
 * @return The BDD for f XOR g, or bddnull if either input is bddnull.
 */
bddp bddxor(bddp f, bddp g, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddxor.
 *
 * Advanced API. Preconditions as in @ref bddand_iter (validated non-null
 * inputs, must run inside a @c bdd_gc_guard scope). Internally composes
 * via @ref bddand_iter, so the same GC-guard requirement applies
 * transitively.
 * @param f First operand.
 * @param g Second operand.
 * @return The BDD for f XOR g.
 */
bddp bddxor_iter(bddp f, bddp g);

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
 * @brief Cofactor restrict v=0 with execution mode selection.
 */
bddp bddat0(bddp f, bddvar v, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddat0.
 *
 * Advanced API. Inputs must be pre-validated (f != bddnull,
 * 1 <= v <= bdd_varcount; the body reads @c var2level[v] without range
 * checks). Must run inside a @c bdd_gc_guard scope. The public @ref bddat0
 * wrappers satisfy these preconditions.
 */
bddp bddat0_iter(bddp f, bddvar v);

/**
 * @brief Cofactor: restrict variable @p v to 1.
 * @param f A BDD node ID.
 * @param v Variable number to restrict.
 * @return The resulting BDD.
 */
bddp bddat1(bddp f, bddvar v);

/**
 * @brief Cofactor restrict v=1 with execution mode selection.
 */
bddp bddat1(bddp f, bddvar v, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddat1.
 *
 * Advanced API. Preconditions as in @ref bddat0_iter (validated
 * non-null @p f, @p v in range, @c bdd_gc_guard active).
 */
bddp bddat1_iter(bddp f, bddvar v);

/**
 * @brief If-then-else operation: (f AND g) OR (NOT f AND h).
 * @param f Condition BDD.
 * @param g Then-branch BDD.
 * @param h Else-branch BDD.
 * @return The resulting BDD, or bddnull if any input is bddnull.
 */
bddp bddite(bddp f, bddp g, bddp h);
bddp bddite(bddp f, bddp g, bddp h, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddite.
 *
 * Advanced API. Inputs must be pre-validated (all three operands non-null).
 * Must run inside a @c bdd_gc_guard scope. Internally composes via
 * @ref bddand_iter for 2-operand reductions, so the same GC-guard
 * requirement applies transitively.
 */
bddp bddite_iter(bddp f, bddp g, bddp h);

/**
 * @brief Check implication: whether f implies g.
 * @param f The antecedent BDD.
 * @param g The consequent BDD.
 * @return 1 if f implies g (f AND NOT g == false), 0 otherwise,
 *         or -1 if either input is bddnull.
 */
int bddimply(bddp f, bddp g);

/**
 * @brief Iterative (non-recursive) implementation of bddimply.
 *
 * Advanced API. Inputs must be pre-validated (no bddnull). Must run inside
 * a @c bdd_gc_guard scope. The public @ref bddimply wrapper satisfies these
 * preconditions.
 */
int bddimply_iter(bddp f, bddp g);

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
 * @brief Existential quantification with execution mode selection.
 */
bddp bddexist(bddp f, bddp g, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddexist.
 *
 * Advanced API. Preconditions as in @ref bddand_iter (validated non-null
 * inputs, must run inside a @c bdd_gc_guard scope). Internally composes
 * via @ref bddand_iter.
 */
bddp bddexist_iter(bddp f, bddp g);

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
 * @brief Universal quantification with execution mode selection.
 */
bddp bdduniv(bddp f, bddp g, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bdduniv.
 *
 * Advanced API. Preconditions as in @ref bddand_iter. Internally composes
 * via @ref bddand_iter.
 */
bddp bdduniv_iter(bddp f, bddp g);

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
 * @brief Generalized cofactor with execution mode selection.
 */
bddp bddcofactor(bddp f, bddp g, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddcofactor.
 *
 * Advanced API. Preconditions as in @ref bddand_iter (validated non-null
 * inputs, must run inside a @c bdd_gc_guard scope).
 */
bddp bddcofactor_iter(bddp f, bddp g);

/**
 * @brief Swap variables v1 and v2 in a BDD.
 * @param f A BDD node ID.
 * @param v1 First variable number.
 * @param v2 Second variable number.
 * @return The BDD with v1 and v2 swapped.
 */
bddp bddswap(bddp f, bddvar v1, bddvar v2);
bddp bddswap(bddp f, bddvar v1, bddvar v2, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddswap.
 *
 * Advanced API. The caller is responsible for full pre-validation and
 * normalization that the public @ref bddswap wrappers perform:
 *   - @p f must be non-null and non-terminal.
 *   - 1 <= @p v1, @p v2 <= bdd_varcount and @p v1 != @p v2.
 *   - @p lev1 = @c var2level[v1], @p lev2 = @c var2level[v2], and
 *     @p lev1 > @p lev2 (v1 must be at the higher level).
 * Must run inside a @c bdd_gc_guard scope. Internally composes via
 * @ref bddite_iter (and therefore @ref bddand_iter).
 */
bddp bddswap_iter(bddp f, bddvar v1, bddvar v2, bddvar lev1, bddvar lev2);

/**
 * @brief Smooth (existential quantification) of a single variable.
 * @param f A BDD node ID.
 * @param v Variable number to quantify out.
 * @return The resulting BDD.
 */
bddp bddsmooth(bddp f, bddvar v);

/**
 * @brief Smooth with execution mode selection.
 */
bddp bddsmooth(bddp f, bddvar v, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddsmooth.
 *
 * Advanced API. Preconditions as in @ref bddat0_iter (validated non-null
 * @p f, 1 <= @p v <= bdd_varcount, @c bdd_gc_guard active). Internally
 * composes via @ref bddand_iter.
 */
bddp bddsmooth_iter(bddp f, bddvar v);

/**
 * @brief Spread variable values to neighboring k levels.
 * @param f A BDD node ID.
 * @param k Number of levels to spread (must be >= 0).
 * @return The resulting BDD.
 */
bddp bddspread(bddp f, int k);
bddp bddspread(bddp f, int k, BddExecMode mode);

/**
 * @brief Iterative (non-recursive) implementation of bddspread.
 *
 * Advanced API. Inputs must be pre-validated: @p f non-null, @p k >= 0
 * (the @c k == 0 fast-path assumes non-negative input; a negative value
 * drives @c frame.k - 1 into signed overflow and produces a corrupt cache
 * key from @c static_cast<bddp>(frame.k)). Must run inside a
 * @c bdd_gc_guard scope. Internally composes via @ref bddand_iter.
 */
bddp bddspread_iter(bddp f, int k);

} // namespace kyotodd

// ZDD operation declarations live in a dedicated header so this file
// stays under the 2000-line limit from CLAUDE.md. Included here so
// existing code that includes "bdd_ops.h" keeps compiling unchanged.
#include "zdd_ops.h"

#endif
