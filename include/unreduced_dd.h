#ifndef KYOTODD_UNREDUCED_DD_H
#define KYOTODD_UNREDUCED_DD_H

#include "bdd_types.h"
#include <functional>

/**
 * @brief An unreduced Binary Decision Diagram (BDD).
 *
 * Unlike BDD, UnreducedBDD does not apply BDD reduction rules at node
 * creation time. Nodes where lo == hi are allowed, complement edges are
 * not normalized, and nodes are not stored in the unique table.
 *
 * Unreduced BDDs are not canonical: different structures may represent
 * the same Boolean function. Call reduce() to obtain a canonical BDD.
 *
 * Supports top-down construction: create a node with bddnull children
 * (placeholders), then fill them in later via set_child0/set_child1.
 */
class UnreducedBDD : public DDBase {
public:
    /** @brief Default constructor. Constructs a false (0-terminal) UnreducedBDD. */
    UnreducedBDD() : DDBase() {}
    /**
     * @brief Construct from an integer value.
     * @param val 0 for false, 1 for true, negative for null.
     */
    explicit UnreducedBDD(int val) : DDBase(val) {}
    /** @brief Copy constructor. */
    UnreducedBDD(const UnreducedBDD& other) : DDBase(other) {}
    /** @brief Move constructor. */
    UnreducedBDD(UnreducedBDD&& other) : DDBase(std::move(other)) {}
    /**
     * @brief Convert a reduced BDD to an UnreducedBDD.
     *
     * Wraps the same bddp. The resulting UnreducedBDD has is_reduced() == true.
     */
    explicit UnreducedBDD(const BDD& bdd) : DDBase() {
        root = bdd.get_id();
    }

    /** @brief Copy assignment operator. */
    UnreducedBDD& operator=(const UnreducedBDD& other) {
        DDBase::operator=(other);
        return *this;
    }
    /** @brief Move assignment operator. */
    UnreducedBDD& operator=(UnreducedBDD&& other) {
        DDBase::operator=(std::move(other));
        return *this;
    }

    // --- Static factory functions ---

    /** @brief Return a false (0-terminal) UnreducedBDD. */
    static UnreducedBDD zero() { return UnreducedBDD(); }
    /** @brief Return a true (1-terminal) UnreducedBDD. */
    static UnreducedBDD one() { return UnreducedBDD(1); }

    /**
     * @brief Create an unreduced BDD node.
     *
     * If both children are reduced and lo != hi, delegates to getnode()
     * to produce a canonical reduced node. Otherwise, allocates a new
     * unreduced node without complement normalization or unique table
     * insertion.
     *
     * @param var Variable number (must be a valid, allocated variable).
     * @param lo  The low (0-edge) child.
     * @param hi  The high (1-edge) child.
     * @return The created UnreducedBDD node.
     */
    static UnreducedBDD node(bddvar var, const UnreducedBDD& lo,
                             const UnreducedBDD& hi);

    // --- Child accessors (static bddp versions) ---

    /** @brief Get the raw 0-child (lo) without complement resolution. */
    static bddp raw_child0(bddp f);
    /** @brief Get the raw 1-child (hi) without complement resolution. */
    static bddp raw_child1(bddp f);
    /** @brief Get the raw child by index (0 or 1) without complement resolution. */
    static bddp raw_child(bddp f, int child);
    /** @brief Get the 0-child (lo) with BDD complement edge resolution. */
    static bddp child0(bddp f);
    /** @brief Get the 1-child (hi) with BDD complement edge resolution. */
    static bddp child1(bddp f);
    /** @brief Get the child by index (0 or 1) with BDD complement edge resolution. */
    static bddp child(bddp f, int child);

    // --- Child accessors (member versions) ---

    /** @brief Get the raw 0-child as an UnreducedBDD. */
    UnreducedBDD raw_child0() const;
    /** @brief Get the raw 1-child as an UnreducedBDD. */
    UnreducedBDD raw_child1() const;
    /** @brief Get the raw child by index as an UnreducedBDD. */
    UnreducedBDD raw_child(int child) const;
    /** @brief Get the 0-child as an UnreducedBDD (with complement resolution). */
    UnreducedBDD child0() const;
    /** @brief Get the 1-child as an UnreducedBDD (with complement resolution). */
    UnreducedBDD child1() const;
    /** @brief Get the child by index as an UnreducedBDD (with complement resolution). */
    UnreducedBDD child(int child) const;

    // --- Child mutation (top-down construction) ---

    /**
     * @brief Set the 0-child (lo) of this unreduced node.
     *
     * Only valid on unreduced, non-terminal, non-complemented nodes.
     * @throws std::invalid_argument If the node is reduced, terminal,
     *         null, or the reference is complemented.
     */
    void set_child0(const UnreducedBDD& child);
    /**
     * @brief Set the 1-child (hi) of this unreduced node.
     *
     * Only valid on unreduced, non-terminal, non-complemented nodes.
     * @throws std::invalid_argument If the node is reduced, terminal,
     *         null, or the reference is complemented.
     */
    void set_child1(const UnreducedBDD& child);

    // --- Operators ---

    /** @brief Complement (negate). O(1) via complement edge toggle. */
    UnreducedBDD operator~() const;

    /** @brief Equality by bddp value (not semantic equality). */
    bool operator==(const UnreducedBDD& o) const { return root == o.root; }
    /** @brief Inequality by bddp value. */
    bool operator!=(const UnreducedBDD& o) const { return root != o.root; }
    /** @brief Less-than by bddp value (for ordered containers). */
    bool operator<(const UnreducedBDD& o) const { return root < o.root; }

    // --- Query ---

    /**
     * @brief Check if this DD is fully reduced (canonical).
     *
     * Returns false for bddnull, true for terminals, and checks the
     * reduced flag for non-terminal nodes.
     */
    bool is_reduced() const;

    // --- Reduction ---

    /**
     * @brief Reduce to a canonical BDD.
     *
     * Recursively reduces the tree bottom-up using getnode().
     * All children must be non-null (no bddnull placeholders).
     *
     * @return The canonical BDD.
     * @throws std::invalid_argument If any node has a bddnull child.
     */
    BDD reduce() const;
};

/**
 * @brief An unreduced Zero-suppressed Decision Diagram (ZDD).
 *
 * Unlike ZDD, UnreducedZDD does not apply the ZDD zero-suppression rule
 * (hi == bddempty => return lo) at node creation time. Complement edges
 * are not normalized and nodes are not stored in the unique table.
 *
 * Call reduce() to obtain a canonical ZDD.
 */
class UnreducedZDD : public DDBase {
public:
    /** @brief Default constructor. Constructs an empty family (0-terminal). */
    UnreducedZDD() : DDBase() {}
    /**
     * @brief Construct from an integer value.
     * @param val 0 for empty, 1 for unit family {{}}, negative for null.
     */
    explicit UnreducedZDD(int val) : DDBase(val) {}
    /** @brief Copy constructor. */
    UnreducedZDD(const UnreducedZDD& other) : DDBase(other) {}
    /** @brief Move constructor. */
    UnreducedZDD(UnreducedZDD&& other) : DDBase(std::move(other)) {}
    /**
     * @brief Convert a reduced ZDD to an UnreducedZDD.
     */
    explicit UnreducedZDD(const ZDD& zdd) : DDBase() {
        root = zdd.get_id();
    }

    /** @brief Copy assignment operator. */
    UnreducedZDD& operator=(const UnreducedZDD& other) {
        DDBase::operator=(other);
        return *this;
    }
    /** @brief Move assignment operator. */
    UnreducedZDD& operator=(UnreducedZDD&& other) {
        DDBase::operator=(std::move(other));
        return *this;
    }

    // --- Static factory functions ---

    /** @brief Return an empty family (0-terminal). */
    static UnreducedZDD empty() { return UnreducedZDD(); }
    /** @brief Return the unit family {{}} (1-terminal). */
    static UnreducedZDD single() { return UnreducedZDD(1); }

    /**
     * @brief Create an unreduced ZDD node.
     *
     * If both children are reduced and hi != bddempty, delegates to
     * getznode(). Otherwise, allocates a new unreduced node.
     */
    static UnreducedZDD node(bddvar var, const UnreducedZDD& lo,
                             const UnreducedZDD& hi);

    // --- Child accessors (static bddp versions) ---

    static bddp raw_child0(bddp f);
    static bddp raw_child1(bddp f);
    static bddp raw_child(bddp f, int child);
    /** @brief Get 0-child with ZDD complement resolution (only lo affected). */
    static bddp child0(bddp f);
    /** @brief Get 1-child (complement does NOT affect hi in ZDD). */
    static bddp child1(bddp f);
    static bddp child(bddp f, int child);

    // --- Child accessors (member versions) ---

    UnreducedZDD raw_child0() const;
    UnreducedZDD raw_child1() const;
    UnreducedZDD raw_child(int child) const;
    UnreducedZDD child0() const;
    UnreducedZDD child1() const;
    UnreducedZDD child(int child) const;

    // --- Child mutation ---

    void set_child0(const UnreducedZDD& child);
    void set_child1(const UnreducedZDD& child);

    // --- Operators ---

    /** @brief Complement. Toggles empty set membership. O(1). */
    UnreducedZDD operator~() const;

    bool operator==(const UnreducedZDD& o) const { return root == o.root; }
    bool operator!=(const UnreducedZDD& o) const { return root != o.root; }
    bool operator<(const UnreducedZDD& o) const { return root < o.root; }

    // --- Query ---

    bool is_reduced() const;

    // --- Reduction ---

    /**
     * @brief Reduce to a canonical ZDD.
     *
     * Recursively reduces using getznode().
     */
    ZDD reduce() const;
};

namespace std {
    template<> struct hash<UnreducedBDD> {
        size_t operator()(const UnreducedBDD& u) const {
            return std::hash<uint64_t>()(u.get_id());
        }
    };
    template<> struct hash<UnreducedZDD> {
        size_t operator()(const UnreducedZDD& u) const {
            return std::hash<uint64_t>()(u.get_id());
        }
    };
}

#endif
