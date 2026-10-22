#ifndef KYOTODD_PERMUTATION_H
#define KYOTODD_PERMUTATION_H

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace kyotodd {

/**
 * @brief A single permutation in one-line notation.
 *
 * A Permutation stores the images pi(1), pi(2), ..., pi(n) of the
 * positions 1..n (1-based): the vector `[2 3 1]` is the permutation with
 * pi(1) = 2, pi(2) = 3 and pi(3) = 1. This is the notation used by
 * PiDD::enumerate(), RotPiDD::from_vector() and
 * RotPiDD::to_vector_of_perms().
 *
 * Every position beyond size() is a fixed point, so a Permutation is an
 * element of the symmetric group on {1, 2, ...} with finite support.
 * This matches PiDD / RotPiDD, where PiDD(1) is the identity whatever
 * the current variable count is. Consequently:
 *
 * - operator()(i) returns i for i > size().
 * - operator==, operator< and hash() ignore trailing fixed points, so
 *   `Permutation({2, 1}) == Permutation({2, 1, 3})`.
 * - operator* extends the shorter operand with fixed points.
 *
 * Composition: `(p * q)(i) == q(p(i))`, i.e. the LEFT operand is applied
 * first. This is the convention of PiDD::operator* and
 * RotPiDD::operator*, so that
 * `PiDD::from_permutation(p) * PiDD::from_permutation(q)
 *  == PiDD::from_permutation(p * q)` (and likewise for RotPiDD).
 */
class Permutation {
    std::vector<int> v_;   // v_[i-1] = pi(i)

    struct Unchecked {};
    Permutation(const std::vector<int>& v, Unchecked) : v_(v) {}

public:
    /** @brief Construct the identity permutation of size 0. */
    Permutation() : v_() {}
    /**
     * @brief Construct from one-line notation.
     * @param v The images v[0] = pi(1), ..., v[n-1] = pi(n). Must be a
     *          rearrangement of 1..n.
     * @throws std::invalid_argument if v is not a rearrangement of 1..n.
     */
    explicit Permutation(const std::vector<int>& v);

    /**
     * @brief The identity permutation of size n.
     * @param n Number of positions (n >= 0).
     * @throws std::invalid_argument if n < 0.
     */
    static Permutation identity(int n);

    /** @brief Number of stored positions n. */
    int size() const { return static_cast<int>(v_.size()); }

    /**
     * @brief The image pi(i) of position i.
     * @param i A 1-based position.
     * @return pi(i); i itself when i > size() (fixed point).
     * @throws std::out_of_range if i < 1.
     */
    int operator()(int i) const;

    /** @brief The stored one-line notation (v[i-1] = pi(i)). */
    const std::vector<int>& to_vector() const { return v_; }

    /** @brief One-line notation as text, e.g. "[2 3 1]" ("[]" for size 0). */
    std::string to_string() const;

    /** @brief The inverse permutation (same size). */
    Permutation inverse() const;

    /**
     * @brief Composition: apply *this first, then q.
     *
     * `(p * q)(i) == q(p(i))`. The result has size max(size(), q.size()).
     */
    Permutation operator*(const Permutation& q) const;
    /** @brief In-place composition (apply *this first, then q). */
    Permutation& operator*=(const Permutation& q) { *this = *this * q; return *this; }

    /** @brief True if the permutation is a product of an even number of transpositions. */
    bool is_even() const;
    /** @brief True if the permutation is a product of an odd number of transpositions. */
    bool is_odd() const { return !is_even(); }

    /**
     * @brief Hash value consistent with operator== (trailing fixed points
     *        are ignored).
     */
    std::size_t hash() const;
};

/** @brief Equality as permutations; trailing fixed points are ignored. */
bool operator==(const Permutation& a, const Permutation& b);
/** @brief Inequality as permutations. */
inline bool operator!=(const Permutation& a, const Permutation& b) { return !(a == b); }
/**
 * @brief Lexicographic order of the images pi(1), pi(2), ...
 *        (the shorter operand is extended with fixed points).
 */
bool operator<(const Permutation& a, const Permutation& b);
/** @brief Print the one-line notation (see Permutation::to_string()). */
std::ostream& operator<<(std::ostream& os, const Permutation& p);

} // namespace kyotodd

#endif
