#ifndef KYOTODD_QDD_H
#define KYOTODD_QDD_H

#include "bdd_types.h"
#include <functional>

class QDD;

/**
 * @brief Wrap a raw bddp as a QDD.
 *
 * Validates that the node is reduced (if non-terminal, non-null).
 * @throws std::invalid_argument if the node is not reduced.
 */
QDD QDD_ID(bddp p);

/**
 * @brief A Quasi-reduced Decision Diagram (QDD).
 *
 * QDD does not apply the jump rule (lo == hi nodes are preserved),
 * but does use node sharing via the unique table. Every path from root
 * to terminal visits every variable level exactly once.
 *
 * Uses BDD complement edge semantics: ~(var, lo, hi) = (var, ~lo, ~hi).
 */
class QDD : public DDBase {
    friend QDD QDD_ID(bddp p);
public:
    /** @brief Default constructor. Constructs a false (0-terminal) QDD. */
    QDD() : DDBase() {}
    /**
     * @brief Construct from an integer value.
     * @param val 0 for false, 1 for true, negative for null.
     */
    explicit QDD(int val) : DDBase(val) {}
    /** @brief Copy constructor. */
    QDD(const QDD& other) : DDBase(other) {}
    /** @brief Move constructor. */
    QDD(QDD&& other) : DDBase(std::move(other)) {}

    /** @brief Copy assignment operator. */
    QDD& operator=(const QDD& other) {
        DDBase::operator=(other);
        return *this;
    }
    /** @brief Move assignment operator. */
    QDD& operator=(QDD&& other) {
        DDBase::operator=(std::move(other));
        return *this;
    }

    // --- Static factory functions ---

    /** @brief Return a false (0-terminal) QDD. */
    static QDD zero() { return QDD(); }
    /** @brief Return a true (1-terminal) QDD. */
    static QDD one() { return QDD(1); }

    /** @brief Create a QDD node with level validation.
     *
     * Children must be at the expected level (var's level - 1).
     * Does NOT apply the jump rule (lo == hi nodes are preserved).
     * Uses BDD complement edge normalization.
     *
     * @param var Variable number.
     * @param lo  The low (0-edge) child node ID.
     * @param hi  The high (1-edge) child node ID.
     * @return The node ID for the (var, lo, hi) triple.
     * @throws std::invalid_argument if children are not at the expected level.
     */
    static bddp getnode(bddvar var, bddp lo, bddp hi);

    /** @brief Create a QDD node with level validation (class type version).
     *
     * Same as the bddp version but takes and returns QDD objects.
     *
     * @param var Variable number.
     * @param lo  The low (0-edge) child.
     * @param hi  The high (1-edge) child.
     * @return The created QDD node.
     * @throws std::invalid_argument if children are not at the expected level.
     */
    static QDD getnode(bddvar var, const QDD& lo, const QDD& hi);

    // Advanced: no error checking — no level validation.
    // Use with extreme caution. Incorrect usage may create invalid QDDs.
    // C++ only. Not available in the Python binding.
    static bddp getnode_raw(bddvar var, bddp lo, bddp hi);

    // --- Operators ---

    /** @brief Complement (negate). O(1) via complement edge toggle. */
    QDD operator~() const;

    /** @brief Equality by bddp value. */
    bool operator==(const QDD& o) const { return root == o.root; }
    /** @brief Inequality by bddp value. */
    bool operator!=(const QDD& o) const { return root != o.root; }

    // --- Child accessors (static bddp versions) ---

    using DDBase::raw_child0;
    using DDBase::raw_child1;
    using DDBase::raw_child;
    /** @brief Get the 0-child (lo) with BDD complement edge resolution. */
    static bddp child0(bddp f);
    /** @brief Get the 1-child (hi) with BDD complement edge resolution. */
    static bddp child1(bddp f);
    /** @brief Get the child by index (0 or 1) with BDD complement edge resolution. */
    static bddp child(bddp f, int child);

    // --- Child accessors (member versions) ---

    /** @brief Get the raw 0-child as a QDD. */
    QDD raw_child0() const;
    /** @brief Get the raw 1-child as a QDD. */
    QDD raw_child1() const;
    /** @brief Get the raw child by index as a QDD. */
    QDD raw_child(int child) const;
    /** @brief Get the 0-child as a QDD (with complement resolution). */
    QDD child0() const;
    /** @brief Get the 1-child as a QDD (with complement resolution). */
    QDD child1() const;
    /** @brief Get the child by index as a QDD (with complement resolution). */
    QDD child(int child) const;

    // --- Binary format I/O ---

    /** @brief Export this QDD in BDD binary format to a FILE stream. */
    void export_binary(FILE* strm) const;
    /** @brief Export this QDD in BDD binary format to an output stream. */
    void export_binary(std::ostream& strm) const;
    /** @brief Import a QDD from BDD binary format from a FILE stream.
     *  @param strm Input FILE stream.
     *  @param ignore_type If true, skip dd_type validation (default: false). */
    static QDD import_binary(FILE* strm, bool ignore_type = false);
    /** @brief Import a QDD from BDD binary format from an input stream. */
    static QDD import_binary(std::istream& strm, bool ignore_type = false);
    /** @brief Export multiple QDDs in binary format to a FILE stream. */
    static void export_binary_multi(FILE* strm, const std::vector<QDD>& qdds);
    /** @brief Export multiple QDDs in binary format to an output stream. */
    static void export_binary_multi(std::ostream& strm, const std::vector<QDD>& qdds);
    /** @brief Import multiple QDDs from binary format from a FILE stream. */
    static std::vector<QDD> import_binary_multi(FILE* strm, bool ignore_type = false);
    /** @brief Import multiple QDDs from binary format from an input stream. */
    static std::vector<QDD> import_binary_multi(std::istream& strm, bool ignore_type = false);

    // --- Conversion ---

    /** @brief Convert QDD to canonical BDD by applying jump rule via getnode(). */
    BDD to_bdd() const;
    /** @brief Convert QDD to canonical ZDD. */
    ZDD to_zdd() const;

    /** @brief Read 2-operand cache and return as QDD. Returns QDD::Null on miss. */
    static QDD cache_get(uint8_t op, const QDD& f, const QDD& g);
    /** @brief Write 2-operand cache entry. */
    static void cache_put(uint8_t op, const QDD& f, const QDD& g, const QDD& result);

    static const QDD False;  /**< @brief Constant false QDD. */
    static const QDD True;   /**< @brief Constant true QDD. */
    static const QDD Null;   /**< @brief Null (error) QDD. */
};

namespace std {
    template<> struct hash<QDD> {
        size_t operator()(const QDD& q) const {
            return std::hash<uint64_t>()(q.get_id());
        }
    };
}

// --- Inline definitions ---

inline QDD QDD::operator~() const {
    QDD q(0);
    q.root = bddnot(root);
    return q;
}

inline bddp QDD::child0(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("QDD::child0: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("QDD::child0: terminal node");
    bddp lo = node_lo(f);
    if (f & BDD_COMP_FLAG) lo = bddnot(lo);
    return lo;
}

inline bddp QDD::child1(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("QDD::child1: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("QDD::child1: terminal node");
    bddp hi = node_hi(f);
    if (f & BDD_COMP_FLAG) hi = bddnot(hi);
    return hi;
}

inline bddp QDD::child(bddp f, int child) {
    if (child == 0) return child0(f);
    if (child == 1) return child1(f);
    throw std::invalid_argument("QDD::child: child must be 0 or 1");
}

inline QDD QDD::raw_child0() const {
    QDD q(0);
    q.root = DDBase::raw_child0(root);
    return q;
}

inline QDD QDD::raw_child1() const {
    QDD q(0);
    q.root = DDBase::raw_child1(root);
    return q;
}

inline QDD QDD::raw_child(int child) const {
    QDD q(0);
    q.root = DDBase::raw_child(root, child);
    return q;
}

inline QDD QDD::child0() const {
    QDD q(0);
    q.root = QDD::child0(root);
    return q;
}

inline QDD QDD::child1() const {
    QDD q(0);
    q.root = QDD::child1(root);
    return q;
}

inline QDD QDD::child(int child) const {
    QDD q(0);
    q.root = QDD::child(root, child);
    return q;
}

#endif
