#ifndef KYOTODD_UNREDUCED_DD_H
#define KYOTODD_UNREDUCED_DD_H

#include "bdd_types.h"
#include "svg_export.h"
#include <functional>

namespace kyotodd {

struct SvgParams;
class QDD;

/**
 * @brief A type-agnostic unreduced Decision Diagram.
 *
 * UnreducedDD does not apply any reduction rules at node creation time:
 * lo == hi nodes, hi == bddempty nodes, and complement edges are all
 * preserved as-is. Nodes are not stored in the unique table.
 *
 * Complement edges are NOT interpreted by UnreducedDD. They are stored
 * raw and only gain meaning when reduce_as_bdd(), reduce_as_zdd(), or
 * reduce_as_qdd() is called. At that point the appropriate complement
 * normalization and reduction rules are applied.
 *
 * Supports top-down construction: create a node with bddnull children
 * (placeholders), then fill them in later via set_child0/set_child1.
 */
class UnreducedDD : public DDBase {
public:
    /** @brief Default constructor. Constructs a 0-terminal. */
    UnreducedDD() : DDBase() {}
    /**
     * @brief Construct from an integer value.
     * @param val 0 for 0-terminal, 1 for 1-terminal, negative for null.
     */
    explicit UnreducedDD(int val) : DDBase(val) {}
    /** @brief Copy constructor. */
    UnreducedDD(const UnreducedDD& other) : DDBase(other) {}
    /** @brief Move constructor. */
    UnreducedDD(UnreducedDD&& other) : DDBase(std::move(other)) {}

    /**
     * @brief Convert a BDD to an UnreducedDD with complement expansion.
     *
     * Recursively traverses the BDD, expanding all complement edges
     * using BDD complement semantics (~(var,lo,hi) = (var,~lo,~hi)).
     * The resulting UnreducedDD has no complement edges.
     */
    explicit UnreducedDD(const BDD& bdd);
    /**
     * @brief Convert a ZDD to an UnreducedDD with complement expansion.
     *
     * Recursively traverses the ZDD, expanding all complement edges
     * using ZDD complement semantics (~(var,lo,hi) = (var,~lo,hi)).
     * The resulting UnreducedDD has no complement edges.
     */
    explicit UnreducedDD(const ZDD& zdd);
    /**
     * @brief Convert a QDD to an UnreducedDD with complement expansion.
     *
     * Uses BDD complement semantics (same as BDD constructor).
     */
    explicit UnreducedDD(const QDD& qdd);

    /** @brief Copy assignment operator. */
    UnreducedDD& operator=(const UnreducedDD& other) {
        DDBase::operator=(other);
        return *this;
    }
    /** @brief Move assignment operator. */
    UnreducedDD& operator=(UnreducedDD&& other) {
        DDBase::operator=(std::move(other));
        return *this;
    }

    // --- Static factory functions ---

    /** @brief Return a 0-terminal UnreducedDD. */
    static UnreducedDD zero() { return UnreducedDD(); }
    /** @brief Return a 1-terminal UnreducedDD. */
    static UnreducedDD one() { return UnreducedDD(1); }

    // --- Raw wrap (complement edges preserved) ---

    /**
     * @brief Wrap a BDD's bddp directly without complement expansion.
     *
     * The resulting UnreducedDD preserves the original complement edges.
     * Only use reduce_as_bdd() on the result; using reduce_as_zdd()
     * or reduce_as_qdd() gives undefined results because the
     * complement normalization was done for BDD semantics.
     */
    static UnreducedDD wrap_raw(const BDD& bdd);
    /**
     * @brief Wrap a ZDD's bddp directly without complement expansion.
     *
     * Only use reduce_as_zdd() on the result; cross-type reduce
     * gives undefined results.
     */
    static UnreducedDD wrap_raw(const ZDD& zdd);
    /**
     * @brief Wrap a QDD's bddp directly without complement expansion.
     *
     * Only use reduce_as_qdd() on the result; cross-type reduce
     * gives undefined results.
     */
    static UnreducedDD wrap_raw(const QDD& qdd);

    // --- Node creation ---

    /**
     * @brief Create an unreduced DD node.
     *
     * Always allocates a new unreduced node. No complement
     * normalization, no reduction rules, no unique table insertion.
     *
     * @param var Variable number (must be a valid, allocated variable).
     * @param lo  The low (0-edge) child.
     * @param hi  The high (1-edge) child.
     * @return The created UnreducedDD node.
     */
    static UnreducedDD getnode(bddvar var, const UnreducedDD& lo,
                               const UnreducedDD& hi);

    /**
     * @brief Create an unreduced node from raw bddp values.
     *
     * Always allocates a new unreduced node. No complement
     * normalization, no reduction rules, no unique table insertion.
     * Matches the import_nodefn_t signature for binary import.
     *
     * @note C++ only. Not available in the Python binding.
     *
     * @param var Variable number.
     * @param lo  The low (0-edge) child node ID.
     * @param hi  The high (1-edge) child node ID.
     * @return The new node ID (always even, unreduced flag).
     */
    static bddp getnode_raw(bddvar var, bddp lo, bddp hi);

    // --- Child accessors (raw only, no complement interpretation) ---

    using DDBase::raw_child0;
    using DDBase::raw_child1;
    using DDBase::raw_child;
    /** @brief Get the raw 0-child as an UnreducedDD. */
    UnreducedDD raw_child0() const;
    /** @brief Get the raw 1-child as an UnreducedDD. */
    UnreducedDD raw_child1() const;
    /** @brief Get the raw child by index as an UnreducedDD. */
    UnreducedDD raw_child(int child) const;

    // --- Child mutation (top-down construction) ---

    /**
     * @brief Set the 0-child (lo) of this unreduced node.
     *
     * Only valid on unreduced, non-terminal, non-complemented nodes.
     * @throws std::invalid_argument If the node is reduced, terminal,
     *         null, or the reference is complemented.
     */
    void set_child0(const UnreducedDD& child);
    /**
     * @brief Set the 1-child (hi) of this unreduced node.
     *
     * Only valid on unreduced, non-terminal, non-complemented nodes.
     * @throws std::invalid_argument If the node is reduced, terminal,
     *         null, or the reference is complemented.
     */
    void set_child1(const UnreducedDD& child);

    // --- Operators ---

    /** @brief Toggle complement bit (bit 0). O(1).
     *
     * The complement bit has no semantics in UnreducedDD; it is only
     * interpreted at reduce time.
     */
    UnreducedDD operator~() const;

    /** @brief Equality by bddp value (not semantic equality). */
    bool operator==(const UnreducedDD& o) const { return root == o.root; }
    /** @brief Inequality by bddp value. */
    bool operator!=(const UnreducedDD& o) const { return root != o.root; }
    /** @brief Less-than by bddp value (for ordered containers). */
    bool operator<(const UnreducedDD& o) const { return root < o.root; }

    // --- Binary format I/O ---

    /** @brief Export this UnreducedDD in BDD binary format to a FILE stream. */
    void export_binary(FILE* strm) const;
    /** @brief Export this UnreducedDD in BDD binary format to an output stream. */
    void export_binary(std::ostream& strm) const;
    /** @brief Import an UnreducedDD from BDD binary format from a FILE stream. */
    static UnreducedDD import_binary(FILE* strm);
    /** @brief Import an UnreducedDD from BDD binary format from an input stream. */
    static UnreducedDD import_binary(std::istream& strm);

    // --- SVG export ---

    /** @brief Save UnreducedDD as SVG to a file (always Raw mode). */
    void save_svg(const char* filename, const SvgParams& params) const;
    void save_svg(const char* filename) const;
    /** @brief Save UnreducedDD as SVG to an output stream (always Raw mode). */
    void save_svg(std::ostream& strm, const SvgParams& params) const;
    void save_svg(std::ostream& strm) const;
    /** @brief Export UnreducedDD as SVG string (always Raw mode). */
    std::string save_svg(const SvgParams& params) const;
    std::string save_svg() const;

    // --- Query ---

    /**
     * @brief Check if this DD is fully reduced (canonical).
     *
     * Returns false for bddnull, true for terminals, and checks the
     * reduced flag for non-terminal nodes.
     */
    bool is_reduced() const;

    // --- Reduce (complement edges are first interpreted here) ---

    /**
     * @brief Reduce to a canonical BDD.
     *
     * Applies BDD complement semantics and BDD reduction rules
     * (jump rule: lo == hi => return lo). All children must be
     * non-null (no bddnull placeholders).
     *
     * @return The canonical BDD.
     * @throws std::invalid_argument If any node has a bddnull child.
     */
    BDD reduce_as_bdd() const;

    /**
     * @brief Reduce to a canonical ZDD.
     *
     * Applies ZDD complement semantics and ZDD reduction rules
     * (zero-suppression: hi == bddempty => return lo). All children
     * must be non-null (no bddnull placeholders).
     *
     * @return The canonical ZDD.
     * @throws std::invalid_argument If any node has a bddnull child.
     */
    ZDD reduce_as_zdd() const;

    /**
     * @brief Reduce to a canonical QDD.
     *
     * Equivalent to reduce_as_bdd().to_qdd(). Uses BDD complement
     * semantics and inserts skip nodes at missing levels.
     *
     * @return The canonical QDD.
     * @throws std::invalid_argument If any node has a bddnull child.
     */
    QDD reduce_as_qdd() const;
};

} // namespace kyotodd

namespace std {
    template<> struct hash<kyotodd::UnreducedDD> {
        size_t operator()(const kyotodd::UnreducedDD& u) const {
            return std::hash<uint64_t>()(u.id());
        }
    };
}

namespace kyotodd {

// --- UnreducedDD SVG export inline implementations ---

inline void UnreducedDD::save_svg(const char* filename, const SvgParams& params) const {
    unreduced_save_svg(filename, root, params);
}
inline void UnreducedDD::save_svg(const char* filename) const {
    unreduced_save_svg(filename, root);
}
inline void UnreducedDD::save_svg(std::ostream& strm, const SvgParams& params) const {
    unreduced_save_svg(strm, root, params);
}
inline void UnreducedDD::save_svg(std::ostream& strm) const {
    unreduced_save_svg(strm, root);
}
inline std::string UnreducedDD::save_svg(const SvgParams& params) const {
    return unreduced_save_svg(root, params);
}
inline std::string UnreducedDD::save_svg() const {
    return unreduced_save_svg(root);
}

} // namespace kyotodd

#endif
