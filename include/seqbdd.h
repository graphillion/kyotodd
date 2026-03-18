#ifndef KYOTODD_SEQBDD_H
#define KYOTODD_SEQBDD_H

#include "bdd.h"

/* --- Operation codes for SeqBDD cache --- */
static const uint8_t BDD_OP_SEQBDD_CONCAT = 66;
static const uint8_t BDD_OP_SEQBDD_LQUOT  = 67;

/* --- SeqBDD class --- */
class SeqBDD {
    ZDD zdd_;

    friend SeqBDD operator&(const SeqBDD& f, const SeqBDD& g);
    friend SeqBDD operator+(const SeqBDD& f, const SeqBDD& g);
    friend SeqBDD operator-(const SeqBDD& f, const SeqBDD& g);
    friend SeqBDD operator*(const SeqBDD& f, const SeqBDD& g);
    friend SeqBDD operator/(const SeqBDD& f, const SeqBDD& p);
    friend bool operator==(const SeqBDD& f, const SeqBDD& g);

public:
    SeqBDD() : zdd_(0) {}
    explicit SeqBDD(int val) : zdd_(val) {}
    SeqBDD(const SeqBDD& f) : zdd_(f.zdd_) {}
    explicit SeqBDD(const ZDD& zbdd) : zdd_(zbdd) {}
    ~SeqBDD() {}

    SeqBDD& operator=(const SeqBDD& f) { zdd_ = f.zdd_; return *this; }

    /* Compound assignment (set operations) */
    SeqBDD& operator&=(const SeqBDD& f) { *this = *this & f; return *this; }
    SeqBDD& operator+=(const SeqBDD& f) { *this = *this + f; return *this; }
    SeqBDD& operator-=(const SeqBDD& f) { *this = *this - f; return *this; }
    SeqBDD& operator*=(const SeqBDD& f) { *this = *this * f; return *this; }
    SeqBDD& operator/=(const SeqBDD& f) { *this = *this / f; return *this; }
    SeqBDD& operator%=(const SeqBDD& f);

    /* Query methods */
    int top() const { return static_cast<int>(zdd_.Top()); }
    ZDD get_zdd() const { return zdd_; }
    uint64_t size() const { return zdd_.Size(); }
    uint64_t card() const { return zdd_.Card(); }
    uint64_t lit() const { return zdd_.Lit(); }
    uint64_t len() const { return zdd_.Len(); }

    /* Node operations */
    SeqBDD off_set(int v) const;
    SeqBDD on_set0(int v) const;
    SeqBDD on_set(int v) const;
    SeqBDD push(int v) const;

    /* Output */
    void print() const;
    void export_to(FILE* strm = stdout) const;
    void export_to(std::ostream& strm) const;
    void print_seq() const;
    std::string seq_str() const;
};

/* Set operations (friend, inline) */
inline SeqBDD operator&(const SeqBDD& f, const SeqBDD& g) { return SeqBDD(f.zdd_ & g.zdd_); }
inline SeqBDD operator+(const SeqBDD& f, const SeqBDD& g) { return SeqBDD(f.zdd_ + g.zdd_); }
inline SeqBDD operator-(const SeqBDD& f, const SeqBDD& g) { return SeqBDD(f.zdd_ - g.zdd_); }
inline bool operator==(const SeqBDD& f, const SeqBDD& g) { return f.zdd_ == g.zdd_; }
inline bool operator!=(const SeqBDD& f, const SeqBDD& g) { return !(f == g); }

/* Remainder (inline) */
inline SeqBDD operator%(const SeqBDD& f, const SeqBDD& p) { return f - (f / p) * p; }

inline SeqBDD& SeqBDD::operator%=(const SeqBDD& f) { *this = *this % f; return *this; }

/* Friend function: create SeqBDD from raw node ID */
inline SeqBDD SeqBDD_ID(bddp p) { return SeqBDD(ZDD_ID(p)); }

#endif
