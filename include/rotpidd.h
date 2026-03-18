#ifndef KYOTODD_ROTPIDD_H
#define KYOTODD_ROTPIDD_H

#include "bdd.h"
#include <vector>
#include <unordered_map>

/* --- Constants --- */
static const int RotPiDD_MaxVar = 254;

/* --- Operation codes for RotPiDD cache --- */
static const uint8_t BDD_OP_ROTPIDD_LEFTROT   = 54;
static const uint8_t BDD_OP_ROTPIDD_COFACT    = 55;
static const uint8_t BDD_OP_ROTPIDD_MULT      = 56;
static const uint8_t BDD_OP_ROTPIDD_ODD       = 57;
static const uint8_t BDD_OP_ROTPIDD_SWAP      = 58;
static const uint8_t BDD_OP_ROTPIDD_REVERSE   = 59;
static const uint8_t BDD_OP_ROTPIDD_ORDER     = 60;
static const uint8_t BDD_OP_ROTPIDD_INVERSE   = 61;
static const uint8_t BDD_OP_ROTPIDD_INSERT    = 62;
static const uint8_t BDD_OP_ROTPIDD_REMOVEMAX = 63;
static const uint8_t BDD_OP_ROTPIDD_NORMALIZE = 64;
static const uint8_t BDD_OP_ROTPIDD_CONTMAX   = 65;

/* --- Global variables --- */
extern int RotPiDD_TopVar;
extern int RotPiDD_VarTableSize;
extern int RotPiDD_LevOfX[];
extern int* RotPiDD_XOfLev;

/* --- Global functions --- */
int RotPiDD_NewVar();
int RotPiDD_VarUsed();

/* --- Level conversion macros --- */
#define RotPiDD_X_Lev(lev) (RotPiDD_XOfLev[(lev)])
#define RotPiDD_Y_Lev(lev) (RotPiDD_LevOfX[RotPiDD_XOfLev[(lev)]] - (lev) + 1)
#define RotPiDD_Lev_XY(x, y) (RotPiDD_LevOfX[(x)] - (y) + 1)

/* --- Rotation algebra macros --- */
#define RotPiDD_Y_YUV(y, u, v) ((y) == (u) ? (v) : (y) == (v) ? (u) : (y))
#define RotPiDD_U_XYU(x, y, u) ((x) == (u) ? (y) : (u))

/* --- RotPiDD class --- */
class RotPiDD {
    ZDD zdd_;

    friend RotPiDD operator&(const RotPiDD& p, const RotPiDD& q);
    friend RotPiDD operator+(const RotPiDD& p, const RotPiDD& q);
    friend RotPiDD operator-(const RotPiDD& p, const RotPiDD& q);
    friend RotPiDD operator*(const RotPiDD& p, const RotPiDD& q);
    friend bool operator==(const RotPiDD& p, const RotPiDD& q);

public:
    RotPiDD() : zdd_(0) {}
    explicit RotPiDD(int a) : zdd_(a) {}
    RotPiDD(const RotPiDD& f) : zdd_(f.zdd_) {}
    explicit RotPiDD(const ZDD& zbdd) : zdd_(zbdd) {}
    ~RotPiDD() {}

    RotPiDD& operator=(const RotPiDD& f) { zdd_ = f.zdd_; return *this; }

    /* Set operations (inline) */
    RotPiDD& operator&=(const RotPiDD& f) { *this = *this & f; return *this; }
    RotPiDD& operator+=(const RotPiDD& f) { *this = *this + f; return *this; }
    RotPiDD& operator-=(const RotPiDD& f) { *this = *this - f; return *this; }
    RotPiDD& operator*=(const RotPiDD& f) { *this = *this * f; return *this; }

    /* Information */
    int TopX() const { return RotPiDD_X_Lev(TopLev()); }
    int TopY() const { return RotPiDD_Y_Lev(TopLev()); }
    int TopLev() const { return static_cast<int>(bddlevofvar(zdd_.Top())); }
    uint64_t Size() const { return zdd_.Size(); }
    uint64_t Card() const { return zdd_.Card(); }
    ZDD GetZDD() const { return zdd_; }

    /* Core operations */
    RotPiDD LeftRot(int u, int v) const;
    RotPiDD Swap(int a, int b) const;
    RotPiDD Reverse(int l, int r) const;
    RotPiDD Cofact(int u, int v) const;
    RotPiDD Odd() const;
    RotPiDD Even() const;
    RotPiDD RotBound(int n) const;
    RotPiDD Order(int a, int b) const;
    RotPiDD Inverse() const;
    RotPiDD Insert(int p, int v) const;
    RotPiDD RemoveMax(int k) const;
    RotPiDD normalizeRotPiDD(int k) const;

    /* Conversion */
    static void normalizePerm(std::vector<int>& v);
    static RotPiDD VECtoRotPiDD(std::vector<int> v);
    std::vector< std::vector<int> > RotPiDDToVectorOfPerms() const;

    /* Extraction & enumeration */
    RotPiDD Extract_One();
    void Print() const;
    void Enum() const;
    void Enum2() const;

    /* Optimization */
    struct hash_func {
        size_t operator()(const std::pair<bddp, unsigned long long int>& a) const {
            return static_cast<size_t>((a.first + 1) * (a.second + 1));
        }
    };

    long long int contradictionMaximization(
        unsigned long long int used_set,
        std::vector<int>& unused_list,
        int n,
        std::unordered_map< std::pair<bddp, unsigned long long int>, long long int, hash_func >& hash,
        const std::vector< std::vector<int> >& w) const;
};

/* Set operations (friend, inline) */
inline RotPiDD operator&(const RotPiDD& p, const RotPiDD& q) { return RotPiDD(p.zdd_ & q.zdd_); }
inline RotPiDD operator+(const RotPiDD& p, const RotPiDD& q) { return RotPiDD(p.zdd_ + q.zdd_); }
inline RotPiDD operator-(const RotPiDD& p, const RotPiDD& q) { return RotPiDD(p.zdd_ - q.zdd_); }
inline bool operator==(const RotPiDD& p, const RotPiDD& q) { return p.zdd_ == q.zdd_; }
inline bool operator!=(const RotPiDD& p, const RotPiDD& q) { return !(p == q); }

#endif
