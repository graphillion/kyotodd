#ifndef KYOTODD_DD_BASE_H
#define KYOTODD_DD_BASE_H

// This header is included by bdd_types.h after type/constant definitions.
// Do NOT include this header directly; include bdd_types.h or bdd.h instead.

#include <stdexcept>

/// @cond INTERNAL
// Forward declarations (defined in bdd_base.h)
int bddinit(uint64_t node_count, uint64_t node_max);
bddvar bddnewvar();
bddvar bddnewvaroflev(bddvar lev);
bddvar bddlevofvar(bddvar var);
bddvar bddvaroflev(bddvar lev);
bddvar bddvarused();
uint64_t bddused();
int bddgc();
/// @endcond

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
    /** @brief Initialize the BDD library (alias of bddinit()). */
    static int init(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX) {
        return bddinit(node_count, node_max);
    }

    /** @brief Create a new variable.
     *  @param reverse If true, insert at level 1 (reverses var/lev ordering).
     */
    static bddvar new_var(bool reverse = false) {
        return reverse ? bddnewvaroflev(1) : bddnewvar();
    }

    /** @brief Create n new variables and return their variable numbers.
     *  @param n Number of variables to create.
     *  @param reverse If true, insert each at level 1 (reverses var/lev ordering).
     */
    static std::vector<bddvar> new_var(int n, bool reverse = false) {
        if (n < 0) {
            throw std::invalid_argument("DDBase::new_var: n must be non-negative");
        }
        std::vector<bddvar> vars(n);
        for (int i = 0; i < n; ++i)
            vars[i] = reverse ? bddnewvaroflev(1) : bddnewvar();
        return vars;
    }

    /** @brief Return the number of variables created so far (alias of bddvarused()). */
    static bddvar var_used() { return bddvarused(); }
    /** @brief Return the number of nodes currently in use (alias of bddused()). */
    static uint64_t node_count() { return bddused(); }
    /** @brief Manually invoke garbage collection (alias of bddgc()). */
    static int gc() { return bddgc(); }

    /** @brief Convert a variable number to its level (alias of bddlevofvar()). */
    static bddvar to_level(bddvar var) { return bddlevofvar(var); }

    /** @brief Convert a level to its variable number (alias of bddvaroflev()). */
    static bddvar to_var(bddvar lev) { return bddvaroflev(lev); }

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
