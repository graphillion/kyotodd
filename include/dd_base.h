#ifndef KYOTODD_DD_BASE_H
#define KYOTODD_DD_BASE_H

// This header is included by bdd_types.h after type/constant definitions.
// Do NOT include this header directly; include bdd_types.h or bdd.h instead.

/**
 * @brief Base class for decision diagram types (BDD, ZDD).
 *
 * Holds the root node ID and manages GC protection. Provides common
 * non-virtual query methods. Not intended for polymorphic use — serves
 * as a code-sharing base class only.
 */
class DDBase {
protected:
    bddp root;  /**< @brief The root node ID. */

    /** @brief Default constructor. Constructs with false (0-terminal). */
    DDBase() : root(bddfalse) {
        bddgc_protect(&root);
    }

    /**
     * @brief Construct from an integer value.
     * @param val 0 for false/empty, 1 for true/single, negative for null.
     */
    explicit DDBase(int val)
        : root(val < 0 ? bddnull : val == 0 ? bddfalse : bddtrue)
    {
        bddgc_protect(&root);
    }

    /** @brief Copy constructor. */
    DDBase(const DDBase& other) : root(other.root) {
        bddgc_protect(&root);
    }

    /** @brief Move constructor. */
    DDBase(DDBase&& other) : root(other.root) {
        bddgc_protect(&root);
        other.root = bddnull;
    }

    /** @brief Destructor. Unregisters GC root. */
    ~DDBase() {
        bddgc_unprotect(&root);
    }

    /** @brief Copy assignment operator. */
    DDBase& operator=(const DDBase& other) {
        root = other.root;
        return *this;
    }

    /** @brief Move assignment operator. */
    DDBase& operator=(DDBase&& other) {
        root = other.root;
        other.root = bddnull;
        return *this;
    }

public:
    /** @brief Get the raw node ID. */
    bddp get_id() const { return root; }

    /** @brief Get the top variable number (0 for terminals). */
    bddvar top() const;  // defined in bdd.h

    /** @brief Check if the root is a terminal node. */
    bool is_terminal() const { return (root & BDD_CONST_FLAG) != 0; }

    /** @brief Check if the root is the 1-terminal (true / single). */
    bool is_one() const { return root == bddtrue; }

    /** @brief Check if the root is the 0-terminal (false / empty). */
    bool is_zero() const { return root == bddfalse; }

    /** @brief Return the number of nodes (with complement edge sharing). */
    uint64_t raw_size() const;  // defined in bdd.h

    /** @brief Get the raw 0-child (lo) node ID without complement resolution. */
    static bddp raw_child0(bddp f);  // defined in bdd.h
    /** @brief Get the raw 1-child (hi) node ID without complement resolution. */
    static bddp raw_child1(bddp f);  // defined in bdd.h
    /** @brief Get the raw child node ID by index (0 or 1) without complement resolution. */
    static bddp raw_child(bddp f, int child);  // defined in bdd.h
};

#endif
