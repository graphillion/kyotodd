#ifndef KYOTODD_BTOI_H
#define KYOTODD_BTOI_H

/**
 * @file btoi.h
 * @brief BtoI class — Binary-to-Integer mapping using BDD vectors.
 *
 * BtoI represents an integer-valued function that depends on BDD variables.
 * Internally it uses a BDDV (BDD vector) in two's complement representation.
 * Each bit can be a constant BDD or a variable-dependent function, allowing
 * BtoI to represent "a function that takes different integer values depending
 * on BDD variable assignments".
 *
 * This header is NOT included by the umbrella header @c bdd.h.
 * You must explicitly @c \#include @c "btoi.h" to use this class.
 *
 * Requires BDDV_Init() to have been called (from bddv.h).
 */

#include "bddv.h"

class BtoI {
    BDDV _bddv;

    /** @brief Bit-shift helper. Positive = left shift, negative = arithmetic right shift. */
    BtoI Shift(int power) const;

    /** @brief Remove redundant sign-extension bits. */
    BtoI Sup() const;

    friend BtoI operator+(const BtoI& a, const BtoI& b);
    friend BtoI operator-(const BtoI& a, const BtoI& b);
    friend BtoI operator*(const BtoI& a, const BtoI& b);
    friend BtoI operator/(const BtoI& a, const BtoI& b);
    friend BtoI operator%(const BtoI& a, const BtoI& b);
    friend BtoI operator&(const BtoI& a, const BtoI& b);
    friend BtoI operator|(const BtoI& a, const BtoI& b);
    friend BtoI operator^(const BtoI& a, const BtoI& b);
    friend int operator==(const BtoI& a, const BtoI& b);
    friend int operator!=(const BtoI& a, const BtoI& b);
    friend BtoI BtoI_ITE(const BDD& f, const BtoI& a, const BtoI& b);
    friend BtoI BtoI_EQ(const BtoI& a, const BtoI& b);
    friend BtoI BtoI_GT(const BtoI& a, const BtoI& b);
    friend BtoI BtoI_LT(const BtoI& a, const BtoI& b);
    friend BtoI BtoI_GE(const BtoI& a, const BtoI& b);
    friend BtoI BtoI_LE(const BtoI& a, const BtoI& b);
    friend BtoI BtoI_NE(const BtoI& a, const BtoI& b);

public:
    /** @brief Default constructor. Represents integer 0. */
    BtoI();

    /** @brief Copy constructor. */
    BtoI(const BtoI& fv);

    /** @brief Construct from BDDV (caller ensures valid two's complement). */
    BtoI(const BDDV& fv);

    /**
     * @brief Construct from a single BDD.
     *
     * BDD(0) → integer 0, BDD(1) or variable-dependent → 0-or-1 integer.
     * BDD(-1) → overflow.
     */
    BtoI(const BDD& f);

    /** @brief Construct from C++ int value. */
    BtoI(int n);

    /** @brief Destructor. */
    ~BtoI();

    /** @brief Copy assignment. */
    BtoI& operator=(const BtoI& fv);

    // --- Compound assignment ---
    BtoI& operator+=(const BtoI& fv);
    BtoI& operator-=(const BtoI& fv);
    BtoI& operator*=(const BtoI& fv);
    BtoI& operator/=(const BtoI& fv);
    BtoI& operator%=(const BtoI& fv);
    BtoI& operator&=(const BtoI& fv);
    BtoI& operator|=(const BtoI& fv);
    BtoI& operator^=(const BtoI& fv);
    BtoI& operator<<=(const BtoI& fv);
    BtoI& operator>>=(const BtoI& fv);

    // --- Unary operators ---
    BtoI operator-() const;
    BtoI operator~() const;
    BtoI operator!() const;

    // --- Shift operators ---
    BtoI operator<<(const BtoI& fv) const;
    BtoI operator>>(const BtoI& fv) const;

    // --- Bounds ---
    BtoI UpperBound() const;
    BtoI UpperBound(const BDD& f) const;
    BtoI LowerBound() const;
    BtoI LowerBound(const BDD& f) const;

    // --- Cofactor / expansion ---
    BtoI At0(int v) const;
    BtoI At1(int v) const;
    BtoI Cofact(const BtoI& fv) const;
    BtoI Spread(int k) const;

    // --- Access ---
    int Top() const;
    BDD GetSignBDD() const;
    BDD GetBDD(int i) const;
    BDDV GetMetaBDDV() const;
    int Len() const;

    // --- Conversion ---
    int GetInt() const;
    int StrNum10(char* s) const;
    int StrNum16(char* s) const;

    // --- Other ---
    uint64_t Size() const;
    void Print() const;
};

// --- Binary operators ---
BtoI operator+(const BtoI& a, const BtoI& b);
BtoI operator-(const BtoI& a, const BtoI& b);
BtoI operator*(const BtoI& a, const BtoI& b);
BtoI operator/(const BtoI& a, const BtoI& b);
BtoI operator%(const BtoI& a, const BtoI& b);
BtoI operator&(const BtoI& a, const BtoI& b);
BtoI operator|(const BtoI& a, const BtoI& b);
BtoI operator^(const BtoI& a, const BtoI& b);
int operator==(const BtoI& a, const BtoI& b);
int operator!=(const BtoI& a, const BtoI& b);

// --- External functions ---

/** @brief If-then-else: f ? a : b (BDD condition). */
BtoI BtoI_ITE(const BDD& f, const BtoI& a, const BtoI& b);

/** @brief If-then-else: a != 0 ? b : c (BtoI condition). */
BtoI BtoI_ITE(const BtoI& a, const BtoI& b, const BtoI& c);

/** @brief Equality: returns 1 where a == b, 0 elsewhere. */
BtoI BtoI_EQ(const BtoI& a, const BtoI& b);

/** @brief Greater-than: returns 1 where a > b. */
BtoI BtoI_GT(const BtoI& a, const BtoI& b);

/** @brief Less-than: returns 1 where a < b. */
BtoI BtoI_LT(const BtoI& a, const BtoI& b);

/** @brief Greater-or-equal: returns 1 where a >= b. */
BtoI BtoI_GE(const BtoI& a, const BtoI& b);

/** @brief Less-or-equal: returns 1 where a <= b. */
BtoI BtoI_LE(const BtoI& a, const BtoI& b);

/** @brief Not-equal: returns 1 where a != b. */
BtoI BtoI_NE(const BtoI& a, const BtoI& b);

/** @brief Parse string to BtoI (decimal, hex "0x...", or binary "0b..."). */
BtoI BtoI_atoi(const char* s);

#endif
