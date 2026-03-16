#ifndef KYOTODD_BDD_BASE_H
#define KYOTODD_BDD_BASE_H

#include "bdd_types.h"

/**
 * @brief Initialize the BDD library.
 *
 * Allocates the node table, unique tables, operation cache, and resets all
 * internal state. Must be called before any other BDD/ZDD operations.
 * Can be called again to re-initialize (frees all previous allocations).
 *
 * @param node_count Initial node table capacity (default: 256).
 * @param node_max   Maximum node table size (default: UINT64_MAX).
 * @return 0 on success.
 * @throws std::overflow_error If allocation sizes overflow.
 * @throws std::bad_alloc If memory allocation fails.
 */
int bddinit(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX);

/** @brief Initialize the BDD library (alias of bddinit()). */
inline int BDD_Init(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX) { return bddinit(node_count, node_max); }

/**
 * @brief Create a new Boolean variable.
 *
 * Allocates a new variable with the next sequential variable number.
 * The initial variable ordering maps variable i to level i.
 *
 * @return The variable number of the newly created variable.
 */
bddvar bddnewvar();

/** @brief Create a new variable (alias of bddnewvar()). */
inline bddvar BDD_NewVar() { return bddnewvar(); }

/**
 * @brief Create a new variable at a specific level.
 *
 * Inserts a new variable at the given level, shifting existing
 * variables at that level and above upward.
 *
 * @param lev The level at which to insert the new variable.
 * @return The variable number of the newly created variable.
 */
bddvar bddnewvaroflev(bddvar lev);

/**
 * @brief Return the level of the given variable.
 * @param var Variable number.
 * @return The level assigned to @p var.
 */
bddvar bddlevofvar(bddvar var);

/**
 * @brief Return the variable at the given level.
 * @param lev Level number.
 * @return The variable number assigned to @p lev.
 */
bddvar bddvaroflev(bddvar lev);

/**
 * @brief Return the number of variables created so far.
 * @return The total number of variables.
 */
bddvar bddvarused();

/**
 * @brief Return the top (highest-level) variable of a node.
 * @param f A node ID.
 * @return The variable number of the top node, or 0 if @p f is a terminal.
 */
bddvar bddtop(bddp f);

/**
 * @brief Copy a node ID (identity operation).
 *
 * This is a no-op provided for API symmetry with bddfree().
 * @param f A node ID.
 * @return The same node ID.
 */
inline bddp bddcopy(bddp f) { return f; }

/**
 * @brief Free a node ID (no-op in this implementation).
 *
 * Provided for API compatibility. Actual memory reclamation is
 * handled by garbage collection.
 * @param f A node ID.
 */
void bddfree(bddp f);

/**
 * @brief Return the number of nodes currently in use.
 * @return The count of allocated node slots (including dead nodes awaiting GC).
 */
uint64_t bddused();

/**
 * @brief Return the number of nodes in the DAG rooted at @p f.
 * @param f A node ID.
 * @return The DAG node count.
 */
uint64_t bddsize(bddp f);

/**
 * @brief Return the total number of nodes in multiple DAGs (array version).
 *
 * Counts shared nodes only once.
 * @param p Array of root node IDs.
 * @param lim Number of roots in the array.
 * @return The combined DAG node count.
 */
uint64_t bddvsize(bddp* p, int lim);

/**
 * @brief Return the total number of nodes in multiple DAGs (vector version).
 *
 * Counts shared nodes only once.
 * @param v Vector of root node IDs.
 * @return The combined DAG node count.
 */
uint64_t bddvsize(const std::vector<bddp>& v);

bddp BDD_UniqueTableLookup(bddvar var, bddp lo, bddp hi);
void BDD_UniqueTableInsert(bddvar var, bddp lo, bddp hi, bddp node_id);

/**
 * @brief Create a BDD node with the given variable and children.
 *
 * Applies BDD reduction rules and complement edge normalization.
 * Returns an existing node if one with the same (var, lo, hi) exists.
 *
 * @param var Variable number.
 * @param lo  The low (0-edge) child node ID.
 * @param hi  The high (1-edge) child node ID.
 * @return The node ID for the (var, lo, hi) triple.
 */
bddp getnode(bddvar var, bddp lo, bddp hi);

/**
 * @brief Create a ZDD node with the given variable and children.
 *
 * Applies ZDD reduction rules and complement edge normalization.
 * Returns an existing node if one with the same (var, lo, hi) exists.
 *
 * @param var Variable number.
 * @param lo  The low (0-edge) child node ID.
 * @param hi  The high (1-edge) child node ID.
 * @return The node ID for the (var, lo, hi) triple.
 */
bddp getznode(bddvar var, bddp lo, bddp hi);

/**
 * @brief Create a terminal node with the given constant value.
 *
 * @param val The constant value (0 to bddvalmax).
 * @return The node ID for the constant terminal.
 * @throws std::invalid_argument If @p val exceeds bddvalmax.
 */
bddp bddconst(uint64_t val);

/**
 * @brief Create a BDD representing a single positive literal.
 *
 * Returns a BDD for the function (var = true), equivalent to
 * getnode(v, bddfalse, bddtrue).
 *
 * @param v Variable number.
 * @return The BDD node ID for variable @p v.
 */
bddp bddprime(bddvar v);

/**
 * @brief Wrap a raw node ID into a BDD object.
 *
 * Validates that the node is reduced. Throws std::invalid_argument
 * if the node is not reduced.
 * @param p A BDD node ID.
 * @return A BDD object holding @p p.
 */
BDD BDD_ID(bddp p);

/**
 * @brief Wrap a raw node ID into a ZDD object.
 *
 * Validates that the node is reduced. Throws std::invalid_argument
 * if the node is not reduced.
 * @param p A ZDD node ID.
 * @return A ZDD object holding @p p.
 */
ZDD ZDD_ID(bddp p);

/**
 * @brief Create a BDD object for variable @p v.
 *
 * Convenience function equivalent to BDD_ID(bddprime(v)).
 * @param v Variable number.
 * @return A BDD object representing variable @p v.
 */
BDD BDDvar(bddvar v);

bddp bddrcache(uint8_t op, bddp f, bddp g);
void bddwcache(uint8_t op, bddp f, bddp g, bddp result);
bddp bddrcache3(uint8_t op, bddp f, bddp g, bddp h);
void bddwcache3(uint8_t op, bddp f, bddp g, bddp h, bddp result);

/**
 * @brief Manually invoke garbage collection.
 *
 * Reclaims dead nodes via mark-and-sweep. No-op if called from within
 * a recursive operation (bdd_gc_depth > 0).
 * @return 0 on success, 1 if no free nodes after GC.
 */
int bddgc();

/**
 * @brief Register a raw bddp pointer as a GC root.
 *
 * The pointed-to node ID will be preserved during garbage collection.
 * Every call to bddgc_protect() must be paired with bddgc_unprotect().
 *
 * @param p Pointer to a bddp value to protect.
 */
void bddgc_protect(bddp* p);

/**
 * @brief Unregister a raw bddp pointer from GC roots.
 * @param p Pointer previously registered with bddgc_protect().
 */
void bddgc_unprotect(bddp* p);

/**
 * @brief Set the GC threshold.
 *
 * GC triggers when live nodes exceed node_max * threshold and
 * the node array is at maximum capacity.
 *
 * @param threshold A value between 0.0 and 1.0 (default: 0.9).
 */
void bddgc_setthreshold(double threshold);

/**
 * @brief Get the current GC threshold.
 * @return The current threshold value.
 */
double bddgc_getthreshold();

/**
 * @brief Return the number of live (reachable) nodes.
 * @return bdd_node_used minus free list count.
 */
uint64_t bddlive();

// Obsolete: always throws. Retained for API compatibility.
/** @brief @deprecated Always throws. Use BDD/ZDD class types instead. */
int bddisbdd(bddp f);
/** @brief @deprecated Always throws. Use BDD/ZDD class types instead. */
int bddiszbdd(bddp f);

/**
 * @brief Check that all non-terminal nodes reachable from @p root are reduced.
 * @param root A BDD/ZDD node ID.
 * @return true if all reachable non-terminal nodes have the reduced flag set.
 */
bool bdd_check_reduced(bddp root);

#endif
