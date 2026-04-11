#ifndef KYOTODD_MVDD_H
#define KYOTODD_MVDD_H

#include "bdd_types.h"
#include "bdd_base.h"
#include "bdd_ops.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <memory>
#include <vector>
#include <string>
#include <iosfwd>
#include <stdexcept>

struct SvgParams;

// ============================================================
//  MVDDVarInfo
// ============================================================

/**
 * @brief Information about a single MVDD variable.
 */
struct MVDDVarInfo {
    bddvar mvdd_var;              /**< MVDD variable number (1-indexed). 0 = invalid. */
    int k;                        /**< Value domain size. */
    std::vector<bddvar> dd_vars;  /**< Internal DD variable numbers (size k-1). */

    /** @brief Default constructor. mvdd_var=0, k=0, dd_vars empty. */
    MVDDVarInfo();

    /** @brief Construct with specified values. */
    MVDDVarInfo(bddvar mv, int kval, const std::vector<bddvar>& dvars);
};

// ============================================================
//  MVDDVarTable
// ============================================================

/**
 * @brief Manages the bidirectional mapping between MVDD variables and
 *        internal DD variables. All MVDD variables share the same
 *        value domain size k.
 */
class MVDDVarTable {
public:
    /**
     * @brief Construct a variable table with the given domain size.
     * @param k Value domain size. Must be >= 2 and <= 65536.
     * @throws std::invalid_argument if k is out of range.
     */
    explicit MVDDVarTable(int k);

    /** @brief Return the value domain size. */
    int k() const;

    /** @brief Return the number of registered MVDD variables. */
    bddvar mvdd_var_count() const;

    /**
     * @brief Register a new MVDD variable with the given internal DD variables.
     * @param dd_vars Internal DD variable numbers (must have size k-1).
     * @return The new MVDD variable number (1-indexed).
     * @throws std::invalid_argument if dd_vars.size() != k-1.
     */
    bddvar register_var(const std::vector<bddvar>& dd_vars);

    /**
     * @brief Return the internal DD variable numbers for MVDD variable mv.
     * @throws std::out_of_range if mv is out of range.
     */
    const std::vector<bddvar>& dd_vars_of(bddvar mv) const;

    /**
     * @brief Return the MVDD variable number for internal DD variable dv.
     * @return MVDD variable number, or 0 if not found.
     */
    bddvar mvdd_var_of(bddvar dv) const;

    /**
     * @brief Return the index (0 to k-2) of internal DD variable dv
     *        within its MVDD variable.
     * @return Index, or -1 if not found.
     */
    int dd_var_index(bddvar dv) const;

    /**
     * @brief Return the info for MVDD variable mv.
     * @throws std::out_of_range if mv is out of range.
     */
    const MVDDVarInfo& var_info(bddvar mv) const;

    /**
     * @brief Return the first (lowest-level) DD variable for MVDD variable mv.
     * @return DD variable number, or 0 if the dd_vars list is empty.
     * @throws std::out_of_range if mv is out of range.
     */
    bddvar get_top_dd_var(bddvar mv) const;

private:
    int k_;
    std::vector<MVDDVarInfo> var_map_;
    std::vector<bddvar> dd_to_mvdd_var_;
    std::vector<int> dd_to_index_;
};

// ============================================================
//  MVBDD
// ============================================================

/**
 * @brief Multi-Valued BDD.
 *
 * Represents a Boolean function over multi-valued variables (each taking
 * values 0..k-1). Internally emulated by a standard BDD using a one-hot
 * style encoding: each MVDD variable with domain size k uses k-1 internal
 * DD variables.
 */
class MVBDD : public DDBase {
public:
    // --- Constructors ---

    /** @brief Default constructor. root=bddnull, var_table_=nullptr. */
    MVBDD();

    /**
     * @brief Construct with a new variable table.
     * @param k Value domain size (>= 2).
     * @param value Initial Boolean value (false or true).
     */
    explicit MVBDD(int k, bool value = false);

    /**
     * @brief Construct from an existing variable table and a BDD.
     * @param table Shared variable table.
     * @param bdd Internal BDD representation.
     */
    MVBDD(std::shared_ptr<MVDDVarTable> table, const BDD& bdd);

    /** @brief Copy constructor. */
    MVBDD(const MVBDD& other);
    /** @brief Move constructor. */
    MVBDD(MVBDD&& other);
    /** @brief Copy assignment. */
    MVBDD& operator=(const MVBDD& other);
    /** @brief Move assignment. */
    MVBDD& operator=(MVBDD&& other);

    // --- Factory methods ---

    /** @brief Constant false, sharing the given table. */
    static MVBDD zero(std::shared_ptr<MVDDVarTable> table);
    /** @brief Constant true, sharing the given table. */
    static MVBDD one(std::shared_ptr<MVDDVarTable> table);

    /**
     * @brief Create a literal: MVDD variable mv equals value.
     *
     * Returns a Boolean function that is true iff variable mv takes the
     * specified value (don't care for other variables).
     *
     * @param base An MVBDD providing the variable table.
     * @param mv MVDD variable number (1-indexed).
     * @param value The value (0 to k-1).
     */
    static MVBDD singleton(const MVBDD& base, bddvar mv, int value);

    // --- Variable management ---

    /**
     * @brief Create a new MVDD variable.
     *
     * Creates k-1 internal DD variables via bddnewvar() and registers
     * them in the shared variable table.
     *
     * @return The new MVDD variable number (1-indexed).
     */
    bddvar new_var();

    /** @brief Return the value domain size k. */
    int k() const;

    /** @brief Return the shared variable table. */
    std::shared_ptr<MVDDVarTable> var_table() const;

    // --- ITE construction ---

    /**
     * @brief Build an MVBDD by specifying a child for each value of variable mv.
     *
     * @param base An MVBDD providing the variable table.
     * @param mv MVDD variable number (1-indexed).
     * @param children children[i] is the sub-function when mv == i. Size must be k.
     * @throws std::invalid_argument on invalid arguments.
     */
    static MVBDD ite(const MVBDD& base, bddvar mv,
                     const std::vector<MVBDD>& children);

    /** @brief Initializer-list version of ite(). */
    static MVBDD ite(const MVBDD& base, bddvar mv,
                     std::initializer_list<MVBDD> children);

    // --- Node reference ---

    /** @brief Return the raw node ID (same as DDBase::get_id()). */
    bddp get_id() const;

    // --- Child node access ---

    /**
     * @brief Return the cofactor when the top MVDD variable takes the given value.
     * @param value The value (0 to k-1).
     * @throws std::invalid_argument on terminal or invalid value.
     */
    MVBDD child(int value) const;

    /**
     * @brief Return the top MVDD variable number.
     * @return MVDD variable number, or 0 for terminals/invalid.
     */
    bddvar top_var() const;

    // --- Boolean operations ---

    MVBDD operator&(const MVBDD& other) const;
    MVBDD operator|(const MVBDD& other) const;
    MVBDD operator^(const MVBDD& other) const;
    MVBDD operator~() const;
    MVBDD& operator&=(const MVBDD& other);
    MVBDD& operator|=(const MVBDD& other);
    MVBDD& operator^=(const MVBDD& other);

    // --- Evaluation ---

    /**
     * @brief Evaluate the Boolean function for the given MVDD assignment.
     * @param assignment 0-indexed: assignment[i] is the value of MVDD variable i+1.
     *        Size must equal the number of registered MVDD variables.
     * @throws std::invalid_argument if assignment size is wrong.
     */
    bool evaluate(const std::vector<int>& assignment) const;

    // --- Conversion ---

    /** @brief Return the internal BDD (by value). */
    BDD to_bdd() const;

    /**
     * @brief Wrap a BDD as an MVBDD using base's variable table.
     * @param base An MVBDD providing the variable table.
     * @param bdd The BDD to wrap.
     */
    static MVBDD from_bdd(const MVBDD& base, const BDD& bdd);

    // --- Terminal checks ---

    bool is_zero() const;
    bool is_one() const;
    bool is_terminal() const;

    // --- Node count ---

    /** @brief MVDD-level logical node count. */
    uint64_t mvbdd_node_count() const;

    /** @brief Internal BDD node count (same as raw_size()). */
    uint64_t size() const;

    // --- Comparison ---

    bool operator==(const MVBDD& other) const;
    bool operator!=(const MVBDD& other) const;

    // --- SVG export ---
    void save_svg(const char* filename, const SvgParams& params) const;
    void save_svg(const char* filename) const;
    void save_svg(std::ostream& strm, const SvgParams& params) const;
    void save_svg(std::ostream& strm) const;
    std::string save_svg(const SvgParams& params) const;
    std::string save_svg() const;

private:
    std::shared_ptr<MVDDVarTable> var_table_;

    void check_compatible(const MVBDD& other) const;
    MVBDD make_result(bddp p) const;
    MVBDD(std::shared_ptr<MVDDVarTable> table, bddp p);
};

// ============================================================
//  MVZDD
// ============================================================

/**
 * @brief Multi-Valued ZDD.
 *
 * Represents a family of multi-valued assignments (each variable takes
 * values 0..k-1). Internally emulated by a standard ZDD using a one-hot
 * style encoding.
 */
class MVZDD : public DDBase {
public:
    // --- Constructors ---

    /** @brief Default constructor. root=bddnull, var_table_=nullptr. */
    MVZDD();

    /**
     * @brief Construct with a new variable table.
     * @param k Value domain size (>= 2).
     * @param value false = empty family, true = family containing the all-zero assignment.
     */
    explicit MVZDD(int k, bool value = false);

    /**
     * @brief Construct from an existing variable table and a ZDD.
     * @param table Shared variable table.
     * @param zdd Internal ZDD representation.
     */
    MVZDD(std::shared_ptr<MVDDVarTable> table, const ZDD& zdd);

    /** @brief Copy constructor. */
    MVZDD(const MVZDD& other);
    /** @brief Move constructor. */
    MVZDD(MVZDD&& other);
    /** @brief Copy assignment. */
    MVZDD& operator=(const MVZDD& other);
    /** @brief Move assignment. */
    MVZDD& operator=(MVZDD&& other);

    // --- Factory methods ---

    /** @brief Empty family, sharing the given table. */
    static MVZDD zero(std::shared_ptr<MVDDVarTable> table);
    /** @brief Family containing only the all-zero assignment, sharing the given table. */
    static MVZDD one(std::shared_ptr<MVDDVarTable> table);

    /**
     * @brief Create a singleton family: one assignment where mv=value, all others=0.
     *
     * @param base An MVZDD providing the variable table.
     * @param mv MVDD variable number (1-indexed).
     * @param value The value (0 to k-1).
     */
    static MVZDD singleton(const MVZDD& base, bddvar mv, int value);

    /**
     * @brief Construct an MVZDD from a list of assignments.
     *
     * This is the inverse of enumerate(). Each inner vector represents
     * one multi-valued assignment, where element j is the value of MVDD
     * variable j+1 (0 to k-1). Duplicate assignments are accepted
     * (they are unified).
     *
     * @param base An MVZDD providing the variable table.
     * @param sets Vector of assignments. Each inner vector must have size
     *             equal to the number of registered MVDD variables.
     * @return An MVZDD containing exactly those assignments.
     * @throws std::invalid_argument if base has no var table, or any
     *         inner vector has wrong size, or any value is out of range.
     */
    static MVZDD from_sets(const MVZDD& base,
                           const std::vector<std::vector<int>>& sets);

    // --- Variable management ---

    /** @brief Create a new MVDD variable (same as MVBDD::new_var()). */
    bddvar new_var();

    /** @brief Return the value domain size k. */
    int k() const;

    /** @brief Return the shared variable table. */
    std::shared_ptr<MVDDVarTable> var_table() const;

    // --- ITE construction ---

    /**
     * @brief Build an MVZDD by specifying a child for each value of variable mv.
     *
     * @param base An MVZDD providing the variable table.
     * @param mv MVDD variable number (1-indexed).
     * @param children children[i] is the sub-family when mv == i. Size must be k.
     */
    static MVZDD ite(const MVZDD& base, bddvar mv,
                     const std::vector<MVZDD>& children);

    /** @brief Initializer-list version of ite(). */
    static MVZDD ite(const MVZDD& base, bddvar mv,
                     std::initializer_list<MVZDD> children);

    // --- Node reference ---

    /** @brief Return the raw node ID. */
    bddp get_id() const;

    // --- Child node access ---

    /**
     * @brief Return the sub-family when the top MVDD variable takes the given value.
     * @param value The value (0 to k-1).
     * @throws std::invalid_argument on terminal or invalid value.
     */
    MVZDD child(int value) const;

    /**
     * @brief Return the top MVDD variable number.
     * @return MVDD variable number, or 0 for terminals/invalid.
     */
    bddvar top_var() const;

    // --- Set family operations ---

    MVZDD operator+(const MVZDD& other) const;   // union
    MVZDD operator-(const MVZDD& other) const;   // difference
    MVZDD operator&(const MVZDD& other) const;   // intersection
    MVZDD operator^(const MVZDD& other) const;   // symmetric difference
    MVZDD& operator+=(const MVZDD& other);
    MVZDD& operator-=(const MVZDD& other);
    MVZDD& operator&=(const MVZDD& other);
    MVZDD& operator^=(const MVZDD& other);

    // --- Membership ---

    /** @brief Check if the all-zero assignment is in the family. */
    bool has_empty() const;

    /**
     * @brief Check if the given assignment is in the family.
     * @param s MVDD assignment (0-indexed: s[i] is the value of MVDD variable i+1).
     *          Size must equal the number of registered MVDD variables.
     */
    bool contains(const std::vector<int>& s) const;

    /**
     * @brief Check if this family is a subset of g (F ⊆ G).
     * @param g Another MVZDD with the same variable table.
     * @return true if every assignment in this family is also in g.
     */
    bool is_subset_family(const MVZDD& g) const;

    /**
     * @brief Check if this family and g are disjoint (F ∩ G = ∅).
     * @param g Another MVZDD with the same variable table.
     * @return true if the two families share no assignment.
     */
    bool is_disjoint(const MVZDD& g) const;

    /**
     * @brief Count the intersection size |F ∩ G| without building it.
     * @param g Another MVZDD with the same variable table.
     */
    bigint::BigInt count_intersec(const MVZDD& g) const;

    /**
     * @brief Jaccard index |F ∩ G| / |F ∪ G|.
     * @param g Another MVZDD with the same variable table.
     * @return 1.0 when both families are empty.
     */
    double jaccard_index(const MVZDD& g) const;

    /**
     * @brief Symmetric difference size |F △ G|.
     * @param g Another MVZDD with the same variable table.
     */
    bigint::BigInt hamming_distance(const MVZDD& g) const;

    // --- Support ---

    /**
     * @brief Return all MVDD variable numbers appearing in the family.
     * @return Sorted vector of MVDD variable numbers.
     */
    std::vector<bddvar> support_vars() const;

    // --- Counting ---

    /** @brief Count the number of MVDD assignments (double). */
    double count() const;

    /** @brief Count the number of MVDD assignments (exact). */
    bigint::BigInt exact_count() const;

    /**
     * @brief Count the number of MVDD assignments (exact, with memo).
     * @param memo A ZddCountMemo created for the internal ZDD.
     */
    bigint::BigInt exact_count(ZddCountMemo& memo) const;

    // --- Sampling ---

    /**
     * @brief Uniformly sample one assignment from the family at random.
     *
     * Each assignment in the family is selected with equal probability.
     *
     * @tparam RNG A uniform random bit generator (e.g. std::mt19937_64).
     * @param rng The random number generator.
     * @param memo A ZddCountMemo created for the internal ZDD.
     * @return The sampled assignment as a vector of values
     *         (0-indexed: result[i] is the value of MVDD variable i+1).
     * @throws std::invalid_argument if the family is empty.
     */
    template<typename RNG>
    std::vector<int> uniform_sample(RNG& rng, ZddCountMemo& memo);

    /**
     * @brief Uniformly sample k assignments and return as a new MVZDD.
     *
     * Uses hypergeometric distribution at each node.
     *
     * @tparam RNG A uniform random bit generator.
     * @param k Number of assignments to sample.
     * @param rng The random number generator.
     * @param memo A ZddCountMemo created for the internal ZDD.
     * @return An MVZDD containing the sampled k assignments.
     * @throws std::invalid_argument if k < 0 or memo mismatch.
     */
    template<typename RNG>
    MVZDD sample_k(int64_t k, RNG& rng, ZddCountMemo& memo);

    /**
     * @brief Include each assignment independently with probability p.
     *
     * @tparam RNG A uniform random bit generator.
     * @param p Inclusion probability (0.0 to 1.0).
     * @param rng The random number generator.
     * @return An MVZDD containing the randomly selected assignments.
     * @throws std::invalid_argument if p is not in [0.0, 1.0].
     */
    template<typename RNG>
    MVZDD random_subset(double p, RNG& rng);

    // --- Weight operations ---

    /**
     * @brief Find the minimum weight sum among all assignments in the family.
     *
     * @param weights 2D weight table. weights[i][v] is the weight when MVDD
     *        variable i+1 takes value v. Size must be mvdd_var_count, and each
     *        inner vector must have size k.
     * @return The minimum total weight.
     * @throws std::invalid_argument if the family is empty or weights size is wrong.
     */
    long long min_weight(const std::vector<std::vector<int>>& weights) const;

    /**
     * @brief Find the maximum weight sum among all assignments in the family.
     *
     * @param weights 2D weight table (same format as min_weight).
     * @return The maximum total weight.
     * @throws std::invalid_argument if the family is empty or weights size is wrong.
     */
    long long max_weight(const std::vector<std::vector<int>>& weights) const;

    /**
     * @brief Find an assignment with the minimum weight sum.
     *
     * @param weights 2D weight table (same format as min_weight).
     * @return The assignment as a vector of values
     *         (0-indexed: result[i] is the value of MVDD variable i+1).
     * @throws std::invalid_argument if the family is empty or weights size is wrong.
     */
    std::vector<int> min_weight_set(const std::vector<std::vector<int>>& weights) const;

    /**
     * @brief Find an assignment with the maximum weight sum.
     *
     * @param weights 2D weight table (same format as min_weight).
     * @return The assignment as a vector of values
     *         (0-indexed: result[i] is the value of MVDD variable i+1).
     * @throws std::invalid_argument if the family is empty or weights size is wrong.
     */
    std::vector<int> max_weight_set(const std::vector<std::vector<int>>& weights) const;

    // --- Cost-bound filtering ---

    /**
     * @brief Extract all assignments with total cost <= b.
     *
     * @param weights 2D weight table (same format as min_weight).
     * @param b Cost bound.
     * @param memo CostBoundMemo for caching across calls with different bounds.
     *             The memo binds to the internally converted DD-level weights.
     * @return An MVZDD containing assignments with cost <= b.
     */
    MVZDD cost_bound_le(const std::vector<std::vector<int>>& weights,
                         long long b, CostBoundMemo& memo) const;

    /** @brief Convenience overload without memo. */
    MVZDD cost_bound_le(const std::vector<std::vector<int>>& weights,
                         long long b) const;

    /**
     * @brief Extract all assignments with total cost >= b.
     *
     * @param weights 2D weight table (same format as min_weight).
     * @param b Cost bound.
     * @param memo CostBoundMemo (not used internally; accepted for API consistency).
     * @return An MVZDD containing assignments with cost >= b.
     */
    MVZDD cost_bound_ge(const std::vector<std::vector<int>>& weights,
                         long long b, CostBoundMemo& memo) const;

    /** @brief Convenience overload without memo. */
    MVZDD cost_bound_ge(const std::vector<std::vector<int>>& weights,
                         long long b) const;

    /**
     * @brief Extract all assignments with total cost exactly b.
     *
     * Computed as cost_bound_le(b) - cost_bound_le(b - 1).
     *
     * @param weights 2D weight table (same format as min_weight).
     * @param b Cost bound.
     * @param memo CostBoundMemo for caching (shared between the two le calls).
     * @return An MVZDD containing assignments with cost == b.
     */
    MVZDD cost_bound_eq(const std::vector<std::vector<int>>& weights,
                         long long b, CostBoundMemo& memo) const;

    /** @brief Convenience overload without memo. */
    MVZDD cost_bound_eq(const std::vector<std::vector<int>>& weights,
                         long long b) const;

    /**
     * @brief Extract all assignments with total cost in [lo, hi].
     *
     * @param weights 2D weight table (same format as min_weight).
     * @param lo Lower cost bound (inclusive).
     * @param hi Upper cost bound (inclusive).
     * @param memo CostBoundMemo for caching.
     * @return An MVZDD containing assignments with lo <= cost <= hi.
     */
    MVZDD cost_bound_range(const std::vector<std::vector<int>>& weights,
                            long long lo, long long hi,
                            CostBoundMemo& memo) const;

    /** @brief Convenience overload without memo. */
    MVZDD cost_bound_range(const std::vector<std::vector<int>>& weights,
                            long long lo, long long hi) const;

    // --- Evaluation ---

    /**
     * @brief Check if the given assignment is in the family.
     * @param assignment 0-indexed: assignment[i] is the value of MVDD variable i+1.
     *        Size must equal the number of registered MVDD variables.
     */
    bool evaluate(const std::vector<int>& assignment) const;

    // --- Enumeration / display ---

    /**
     * @brief Enumerate all MVDD assignments in the family.
     * @return Vector of assignments. Each inner vector has size = mvdd_var_count,
     *         where element j is the value of MVDD variable j+1.
     */
    std::vector<std::vector<int>> enumerate() const;

    /** @brief Print all assignments to the stream. */
    void print_sets(std::ostream& os) const;

    /** @brief Print all assignments with variable names. */
    void print_sets(std::ostream& os,
                    const std::vector<std::string>& var_names) const;

    /** @brief Return a string representation of all assignments. */
    std::string to_str() const;

    // --- Conversion ---

    /** @brief Return the internal ZDD (by value). */
    ZDD to_zdd() const;

    /**
     * @brief Wrap a ZDD as an MVZDD using base's variable table.
     * @param base An MVZDD providing the variable table.
     * @param zdd The ZDD to wrap.
     */
    static MVZDD from_zdd(const MVZDD& base, const ZDD& zdd);

    // --- Terminal checks ---

    bool is_zero() const;
    bool is_one() const;
    bool is_terminal() const;

    // --- Node count ---

    /** @brief MVDD-level logical node count. */
    uint64_t mvzdd_node_count() const;

    /** @brief Internal ZDD node count (same as raw_size()). */
    uint64_t size() const;

    // --- Comparison ---

    bool operator==(const MVZDD& other) const;
    bool operator!=(const MVZDD& other) const;

    // --- SVG export ---
    void save_svg(const char* filename, const SvgParams& params) const;
    void save_svg(const char* filename) const;
    void save_svg(std::ostream& strm, const SvgParams& params) const;
    void save_svg(std::ostream& strm) const;
    std::string save_svg(const SvgParams& params) const;
    std::string save_svg() const;

private:
    std::shared_ptr<MVDDVarTable> var_table_;

    void check_compatible(const MVZDD& other) const;
    MVZDD make_result(bddp p) const;
    MVZDD(std::shared_ptr<MVDDVarTable> table, bddp p);

    /**
     * @brief Validate and convert 2D MVZDD weights to DD-level weights.
     * @param weights 2D weight table.
     * @param[out] dd_weights DD-level weight vector.
     * @param[out] base_weight Constant offset (sum of weights[mv-1][0]).
     * @param caller Function name for error messages.
     */
    void convert_weights(const std::vector<std::vector<int>>& weights,
                         std::vector<int>& dd_weights,
                         long long& base_weight,
                         const char* caller) const;

    /**
     * @brief Convert a DD-level variable set to an MVZDD assignment vector.
     */
    std::vector<int> dd_set_to_assignment(const std::vector<bddvar>& dd_set) const;
};

// ========================================================================
//  MVBDD/MVZDD save_svg inline implementations
// ========================================================================

#include "svg_export.h"

inline void MVBDD::save_svg(const char* filename, const SvgParams& params) const {
    if (params.mode == DrawMode::Raw) {
        bdd_save_svg(filename, root, params);
    } else {
        mvbdd_save_svg(filename, root, var_table_.get(), params);
    }
}
inline void MVBDD::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
inline void MVBDD::save_svg(std::ostream& strm, const SvgParams& params) const {
    if (params.mode == DrawMode::Raw) {
        bdd_save_svg(strm, root, params);
    } else {
        mvbdd_save_svg(strm, root, var_table_.get(), params);
    }
}
inline void MVBDD::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
inline std::string MVBDD::save_svg(const SvgParams& params) const {
    if (params.mode == DrawMode::Raw) {
        return bdd_save_svg(root, params);
    } else {
        return mvbdd_save_svg(root, var_table_.get(), params);
    }
}
inline std::string MVBDD::save_svg() const {
    return save_svg(SvgParams());
}

inline void MVZDD::save_svg(const char* filename, const SvgParams& params) const {
    if (params.mode == DrawMode::Raw) {
        zdd_save_svg(filename, root, params);
    } else {
        mvzdd_save_svg(filename, root, var_table_.get(), params);
    }
}
inline void MVZDD::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
inline void MVZDD::save_svg(std::ostream& strm, const SvgParams& params) const {
    if (params.mode == DrawMode::Raw) {
        zdd_save_svg(strm, root, params);
    } else {
        mvzdd_save_svg(strm, root, var_table_.get(), params);
    }
}
inline void MVZDD::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
inline std::string MVZDD::save_svg(const SvgParams& params) const {
    if (params.mode == DrawMode::Raw) {
        return zdd_save_svg(root, params);
    } else {
        return mvzdd_save_svg(root, var_table_.get(), params);
    }
}
inline std::string MVZDD::save_svg() const {
    return save_svg(SvgParams());
}

// ========================================================================
//  MVZDD template implementations
//  (requires ZDD::uniform_sample template from bdd.h — include bdd.h
//   before using this template, or use the umbrella header)
// ========================================================================

template<typename RNG>
std::vector<int> MVZDD::uniform_sample(RNG& rng, ZddCountMemo& memo) {
    if (!var_table_) {
        throw std::logic_error("MVZDD::uniform_sample: no var table");
    }
    ZDD z = to_zdd();
    std::vector<bddvar> dd_set = z.uniform_sample(rng, memo);

    // Convert DD-level set to MVDD assignment
    bddvar n = var_table_->mvdd_var_count();
    std::vector<int> assign(n, 0);
    for (size_t j = 0; j < dd_set.size(); ++j) {
        bddvar dv = dd_set[j];
        bddvar mv = var_table_->mvdd_var_of(dv);
        if (mv == 0) {
            throw std::logic_error(
                "MVZDD::uniform_sample: DD variable " + std::to_string(dv) +
                " is not registered in the var table");
        }
        int idx = var_table_->dd_var_index(dv);
        assign[mv - 1] = idx + 1;
    }
    return assign;
}

template<typename RNG>
MVZDD MVZDD::sample_k(int64_t k, RNG& rng, ZddCountMemo& memo) {
    if (!var_table_) {
        throw std::logic_error("MVZDD::sample_k: no var table");
    }
    ZDD z = to_zdd();
    ZDD sampled = z.sample_k(k, rng, memo);
    return make_result(sampled.get_id());
}

template<typename RNG>
MVZDD MVZDD::random_subset(double p, RNG& rng) {
    if (!var_table_) {
        throw std::logic_error("MVZDD::random_subset: no var table");
    }
    ZDD z = to_zdd();
    ZDD sampled = z.random_subset(p, rng);
    return make_result(sampled.get_id());
}

#endif
