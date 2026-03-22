#ifndef KYOTODD_CTOI_H
#define KYOTODD_CTOI_H

/**
 * @file ctoi.h
 * @brief CtoI class — Combination-to-Integer mapping using ZBDDs.
 *
 * CtoI represents an integer-valued function that maps combinations
 * (subsets of items) to integer values. Internally it uses a single ZBDD
 * where user variables represent items and system variables encode
 * binary digit positions.
 *
 * This header is NOT included by the umbrella header @c bdd.h.
 * You must explicitly @c \#include @c "ctoi.h" to use this class.
 *
 * Requires BDDV_Init() to have been called (from bddv.h).
 */

#include "bddv.h"

// --- Operation cache codes for CtoI ---
static const uint8_t BC_CtoI_MULT = 68;
static const uint8_t BC_CtoI_DIV  = 69;
static const uint8_t BC_CtoI_TV   = 70;
static const uint8_t BC_CtoI_TVI  = 71;
static const uint8_t BC_CtoI_RI   = 72;
static const uint8_t BC_CtoI_FPA  = 73;
static const uint8_t BC_CtoI_FPA2 = 74;
static const uint8_t BC_CtoI_FPAV = 75;
static const uint8_t BC_CtoI_FPM  = 76;
static const uint8_t BC_CtoI_FPC  = 77;
static const uint8_t BC_CtoI_MEET = 78;

class CtoI {
    ZDD _zbdd;

    friend CtoI operator+(const CtoI& a, const CtoI& b);
    friend CtoI operator-(const CtoI& a, const CtoI& b);
    friend CtoI operator*(const CtoI& a, const CtoI& b);
    friend CtoI operator/(const CtoI& a, const CtoI& b);
    friend CtoI operator%(const CtoI& a, const CtoI& b);
    friend int operator==(const CtoI& a, const CtoI& b);
    friend int operator!=(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_Intsec(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_Union(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_Diff(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_GT(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_GE(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_NE(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_EQ(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_ITE(const CtoI& a, const CtoI& b, const CtoI& c);
    friend CtoI CtoI_Meet(const CtoI& a, const CtoI& b);
    friend CtoI CtoI_Null();

public:
    /** @brief Default constructor. Empty CtoI (no combinations assigned). */
    CtoI();

    /** @brief Copy constructor. */
    CtoI(const CtoI& a);

    /** @brief Construct from ZBDD directly. */
    CtoI(const ZDD& f);

    /**
     * @brief Construct from int value.
     *
     * Creates a constant function: empty combination → value n.
     * 0 → empty ZBDD, 1 → singleton ZBDD, n >= 2 → built via repeated doubling.
     */
    CtoI(int n);

    /** @brief Destructor. */
    ~CtoI();

    /** @brief Copy assignment. */
    CtoI& operator=(const CtoI& a);

    // --- Compound assignment ---
    CtoI& operator+=(const CtoI& a);
    CtoI& operator-=(const CtoI& a);
    CtoI& operator*=(const CtoI& a);
    CtoI& operator/=(const CtoI& a);
    CtoI& operator%=(const CtoI& a);

    /** @brief Arithmetic negation: CtoI(0) - *this. */
    CtoI operator-() const;

    // --- Access / predicates ---

    /** @brief Top variable ID of internal ZBDD. */
    int Top() const;

    /** @brief Top user variable (item) ID. 0 if no items. */
    int TopItem() const;

    /** @brief Top digit index. 0 if boolean. */
    int TopDigit() const;

    /** @brief True if boolean (items only, values 0 or 1). */
    int IsBool() const;

    /** @brief True if constant (no item dependency). */
    int IsConst() const;

    // --- Factor decomposition ---

    /** @brief 0-factor by variable v (sets NOT containing v). */
    CtoI Factor0(int v) const;

    /** @brief 1-factor by variable v (sets containing v, v removed). */
    CtoI Factor1(int v) const;

    /** @brief Affix variable v: merge Factor0 and Factor1, then add v to all. */
    CtoI AffixVar(int v) const;

    // --- Filter operations ---

    /** @brief Keep combinations where condition a is non-zero. */
    CtoI FilterThen(const CtoI& a) const;

    /** @brief Keep combinations where condition a is zero. */
    CtoI FilterElse(const CtoI& a) const;

    /** @brief Restrict filter. */
    CtoI FilterRestrict(const CtoI& a) const;

    /** @brief Permit filter. */
    CtoI FilterPermit(const CtoI& a) const;

    /** @brief Symmetric permit filter. */
    CtoI FilterPermitSym(int n) const;

    // --- Set operations ---

    /** @brief Boolean CtoI of combinations with non-zero value. */
    CtoI NonZero() const;

    /** @brief Support (set of variables appearing). */
    CtoI Support() const;

    /** @brief Constant term (value for empty combination only). */
    CtoI ConstTerm() const;

    // --- Digit operations ---

    /** @brief Extract boolean CtoI for given digit index. */
    CtoI Digit(int index) const;

    /** @brief Multiply by weight of system variable v. */
    CtoI TimesSysVar(int v) const;

    /** @brief Divide by weight of system variable v. */
    CtoI DivBySysVar(int v) const;

    /** @brief Shift digits: multiply value by 2^pow. */
    CtoI ShiftDigit(int pow) const;

    // --- Constant comparisons ---

    /** @brief Combinations where value == a. */
    CtoI EQ_Const(const CtoI& a) const;

    /** @brief Combinations where value != a. */
    CtoI NE_Const(const CtoI& a) const;

    /** @brief Combinations where value > a. */
    CtoI GT_Const(const CtoI& a) const;

    /** @brief Combinations where value >= a. */
    CtoI GE_Const(const CtoI& a) const;

    /** @brief Combinations where value < a. */
    CtoI LT_Const(const CtoI& a) const;

    /** @brief Combinations where value <= a. */
    CtoI LE_Const(const CtoI& a) const;

    // --- Max / Min ---

    /** @brief Maximum value across all combinations. */
    CtoI MaxVal() const;

    /** @brief Minimum value across all combinations. */
    CtoI MinVal() const;

    // --- Aggregation ---

    /** @brief Number of combinations (count of non-zero entries). */
    CtoI CountTerms() const;

    /** @brief Sum of values across all combinations. */
    CtoI TotalVal() const;

    /** @brief Sum of (value × item count) across all combinations. */
    CtoI TotalValItems() const;

    // --- Absolute value / sign ---

    /** @brief Absolute value for each combination. */
    CtoI Abs() const;

    /** @brief Sign function: +1, -1, or 0 for each combination. */
    CtoI Sign() const;

    // --- Conversion / output ---

    /** @brief Return the internal ZBDD. */
    ZDD GetZBDD() const;

    /** @brief BDD node count. */
    uint64_t Size() const;

    /** @brief Convert constant CtoI to int. */
    int GetInt() const;

    /** @brief Convert to decimal string. @return 0 on success, 1 on error. */
    int StrNum10(char* s) const;

    /** @brief Convert to hex string. @return 0 on success, 1 on error. */
    int StrNum16(char* s) const;

    /** @brief Print in polynomial form. @return 0 on success, 1 on error. */
    int PutForm() const;

    /** @brief Print internal ZBDD. */
    void Print() const;

    // --- Frequent pattern mining ---

    /** @brief Reduce items by filter b. */
    CtoI ReduceItems(const CtoI& b) const;

    /** @brief Frequent patterns (anti-monotone, all occurrences). */
    CtoI FreqPatA(int Val) const;

    /** @brief Frequent patterns (improved version with pruning). */
    CtoI FreqPatA2(int Val) const;

    /** @brief Frequent patterns with values (anti-monotone). */
    CtoI FreqPatAV(int Val) const;

    /** @brief Frequent patterns (maximal, monotone). */
    CtoI FreqPatM(int Val) const;

    /** @brief Frequent patterns (closed). */
    CtoI FreqPatC(int Val) const;
};

// --- Binary operators ---
CtoI operator+(const CtoI& a, const CtoI& b);
CtoI operator-(const CtoI& a, const CtoI& b);
CtoI operator*(const CtoI& a, const CtoI& b);
CtoI operator/(const CtoI& a, const CtoI& b);
CtoI operator%(const CtoI& a, const CtoI& b);
int operator==(const CtoI& a, const CtoI& b);
int operator!=(const CtoI& a, const CtoI& b);

// --- Set operations ---
CtoI CtoI_Intsec(const CtoI& a, const CtoI& b);
CtoI CtoI_Union(const CtoI& a, const CtoI& b);
CtoI CtoI_Diff(const CtoI& a, const CtoI& b);

// --- Comparison ---
CtoI CtoI_GT(const CtoI& a, const CtoI& b);
CtoI CtoI_GE(const CtoI& a, const CtoI& b);
CtoI CtoI_LT(const CtoI& a, const CtoI& b);
CtoI CtoI_LE(const CtoI& a, const CtoI& b);
CtoI CtoI_NE(const CtoI& a, const CtoI& b);
CtoI CtoI_EQ(const CtoI& a, const CtoI& b);

// --- Conditional ---
CtoI CtoI_ITE(const CtoI& a, const CtoI& b, const CtoI& c);
CtoI CtoI_Max(const CtoI& a, const CtoI& b);
CtoI CtoI_Min(const CtoI& a, const CtoI& b);

// --- Special ---
CtoI CtoI_Null();
CtoI CtoI_Meet(const CtoI& a, const CtoI& b);
CtoI CtoI_atoi(const char* s);

#endif
