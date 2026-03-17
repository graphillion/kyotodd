#ifndef KYOTODD_PIDD_H
#define KYOTODD_PIDD_H

#include "bdd.h"

/* --- Constants --- */
static const int PiDD_MaxVar = 254;

/* --- Operation codes for PiDD cache --- */
static const uint8_t BDD_OP_PIDD_SWAP   = 49;
static const uint8_t BDD_OP_PIDD_COFACT = 50;
static const uint8_t BDD_OP_PIDD_MULT   = 51;
static const uint8_t BDD_OP_PIDD_DIV    = 52;
static const uint8_t BDD_OP_PIDD_ODD    = 53;

/* --- Global variables --- */
extern int PiDD_TopVar;
extern int PiDD_VarTableSize;
extern int PiDD_LevOfX[];
extern int* PiDD_XOfLev;

/* --- Global functions --- */
int PiDD_NewVar();
int PiDD_VarUsed();

/* --- Level conversion macros --- */
#define PiDD_X_Lev(lev) (PiDD_XOfLev[(lev)])
#define PiDD_Y_Lev(lev) (PiDD_LevOfX[PiDD_XOfLev[(lev)]] - (lev) + 1)
#define PiDD_Lev_XY(x, y) (PiDD_LevOfX[(x)] - (y) + 1)

/* --- Transposition algebra macros --- */
#define PiDD_Y_YUV(y, u, v) ((y) == (u) ? (v) : (y) == (v) ? (u) : (y))
#define PiDD_U_XYU(x, y, u) ((x) == (u) ? (y) : (u))

/* --- PiDD class --- */
class PiDD {
    ZDD zdd_;

    friend PiDD operator&(const PiDD& p, const PiDD& q);
    friend PiDD operator+(const PiDD& p, const PiDD& q);
    friend PiDD operator-(const PiDD& p, const PiDD& q);
    friend PiDD operator*(const PiDD& p, const PiDD& q);
    friend PiDD operator/(const PiDD& f, const PiDD& p);
    friend bool operator==(const PiDD& p, const PiDD& q);

public:
    PiDD() : zdd_(0) {}
    explicit PiDD(int a) : zdd_(a) {}
    PiDD(const PiDD& f) : zdd_(f.zdd_) {}
    explicit PiDD(const ZDD& zbdd) : zdd_(zbdd) {}
    ~PiDD() {}

    PiDD& operator=(const PiDD& f) { zdd_ = f.zdd_; return *this; }

    /* Set operations (inline) */
    PiDD& operator&=(const PiDD& f) { *this = *this & f; return *this; }
    PiDD& operator+=(const PiDD& f) { *this = *this + f; return *this; }
    PiDD& operator-=(const PiDD& f) { *this = *this - f; return *this; }
    PiDD& operator*=(const PiDD& f) { *this = *this * f; return *this; }
    PiDD& operator/=(const PiDD& f) { *this = *this / f; return *this; }
    PiDD& operator%=(const PiDD& f);

    /* Information */
    int TopX() const { return PiDD_X_Lev(TopLev()); }
    int TopY() const { return PiDD_Y_Lev(TopLev()); }
    int TopLev() const { return static_cast<int>(bddlevofvar(zdd_.Top())); }
    uint64_t Size() const { return zdd_.Size(); }
    uint64_t Card() const { return zdd_.Card(); }
    ZDD GetZDD() const { return zdd_; }

    /* Core operations */
    PiDD Swap(int u, int v) const;
    PiDD Cofact(int u, int v) const;
    PiDD Odd() const;
    PiDD Even() const;
    PiDD SwapBound(int n) const;

    /* Output */
    void Print() const;
    void Enum() const;
    void Enum2() const;
};

/* Set operations (friend, inline) */
inline PiDD operator&(const PiDD& p, const PiDD& q) { return PiDD(p.zdd_ & q.zdd_); }
inline PiDD operator+(const PiDD& p, const PiDD& q) { return PiDD(p.zdd_ + q.zdd_); }
inline PiDD operator-(const PiDD& p, const PiDD& q) { return PiDD(p.zdd_ - q.zdd_); }
inline bool operator==(const PiDD& p, const PiDD& q) { return p.zdd_ == q.zdd_; }
inline bool operator!=(const PiDD& p, const PiDD& q) { return !(p == q); }

/* Remainder (inline) */
inline PiDD operator%(const PiDD& f, const PiDD& p) { return f - (f / p) * p; }

inline PiDD& PiDD::operator%=(const PiDD& f) { *this = *this % f; return *this; }

#endif
