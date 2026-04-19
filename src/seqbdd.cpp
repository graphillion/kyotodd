#include "seqbdd.h"
#include "bdd_internal.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace kyotodd {


/* ================================================================ */
/*  onset0                                                          */
/* ================================================================ */

SeqBDD SeqBDD::onset0(int v) const
{
    bddvar vlev = bddlevofvar(static_cast<bddvar>(v));
    ZDD f = zdd_;
    bddvar ftop = f.Top();
    while (ftop != 0 && bddlevofvar(ftop) > vlev) {
        f = f.OffSet(ftop);
        ftop = f.Top();
    }
    return SeqBDD(f.OnSet0(static_cast<bddvar>(v)));
}

/* ================================================================ */
/*  offset                                                          */
/* ================================================================ */

SeqBDD SeqBDD::offset(int v) const
{
    if (static_cast<int>(zdd_.Top()) == v) {
        return SeqBDD(zdd_.OffSet(static_cast<bddvar>(v)));
    }
    return *this - onset0(v).push(v);
}

/* ================================================================ */
/*  onset                                                           */
/* ================================================================ */

SeqBDD SeqBDD::onset(int v) const
{
    return onset0(v).push(v);
}

/* ================================================================ */
/*  push                                                             */
/* ================================================================ */

SeqBDD SeqBDD::push(int v) const
{
    return SeqBDD(ZDD_ID(bddpush(zdd_.GetID(), static_cast<bddvar>(v))));
}

/* ================================================================ */
/*  operator* (concatenation)                                        */
/* ================================================================ */

SeqBDD operator*(const SeqBDD& f, const SeqBDD& g)
{
    bddp fx = f.zdd_.GetID();
    bddp gx = g.zdd_.GetID();

    /* Terminal cases */
    if (fx == bddnull || gx == bddnull) return SeqBDD(-1);
    if (fx == bddempty || gx == bddempty) return SeqBDD(0);
    if (fx == bddsingle) return g;
    if (gx == bddsingle) return f;

    /* Cache lookup */
    ZDD cache_result = BDD_CacheZDD(BDD_OP_SEQBDD_CONCAT, fx, gx);
    if (cache_result != ZDD::Null) return SeqBDD(cache_result);

    /* Decompose on f's top variable */
    int ftop = f.top();
    SeqBDD f1 = f.onset0(ftop);
    SeqBDD f0(f.zdd_.OffSet(static_cast<bddvar>(ftop)));

    /* result = (f1 * g).push(ftop) + (f0 * g) */
    SeqBDD r = (f1 * g).push(ftop) + (f0 * g);

    /* Cache store */
    bddwcache(BDD_OP_SEQBDD_CONCAT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  operator/ (left quotient)                                        */
/* ================================================================ */

SeqBDD operator/(const SeqBDD& f, const SeqBDD& p)
{
    bddp fx = f.zdd_.GetID();
    bddp px = p.zdd_.GetID();

    /* Terminal cases */
    if (fx == bddnull || px == bddnull) return SeqBDD(-1);
    if (px == bddempty) {
        throw std::invalid_argument("SeqBDD: division by empty set");
    }
    if (px == bddsingle) return f;
    if (fx == px) return SeqBDD(1);

    /* If f's top level < p's top level, no prefix match possible */
    bddvar ftop_var = f.zdd_.Top();
    bddvar ptop_var = p.zdd_.Top();
    if (ftop_var == 0 || bddlevofvar(ftop_var) < bddlevofvar(ptop_var)) {
        return SeqBDD(0);
    }

    /* Cache lookup */
    ZDD cache_result = BDD_CacheZDD(BDD_OP_SEQBDD_LQUOT, fx, px);
    if (cache_result != ZDD::Null) return SeqBDD(cache_result);

    /* Decompose on p's top variable */
    int ptop = p.top();
    SeqBDD q = f.onset0(ptop) / p.onset0(ptop);

    if (q != SeqBDD(0)) {
        SeqBDD p0 = p.offset(ptop);
        if (p0 != SeqBDD(0)) {
            q &= f.offset(ptop) / p0;
        }
    }

    /* Cache store */
    bddwcache(BDD_OP_SEQBDD_LQUOT, fx, px, q.zdd_.GetID());
    return q;
}

/* ================================================================ */
/*  print / export_to                                                */
/* ================================================================ */

void SeqBDD::print() const
{
    zdd_.Print();
}

void SeqBDD::export_to(FILE* strm) const
{
    zdd_.Export(strm);
}

void SeqBDD::export_to(std::ostream& strm) const
{
    zdd_.Export(strm);
}

/* ================================================================ */
/*  print_seq / seq_str                                              */
/* ================================================================ */

// Iterative counterpart to print_seq_rec. Avoids C++ call stack growth
// proportional to sequence length; uses an explicit frame stack instead.
static void print_seq_iter(std::ostream& os, SeqBDD root_f,
                           std::vector<int>& arr, int& idx, bool& flag)
{
    enum class Phase : uint8_t { ENTER, AFTER_F1, AFTER_F0 };
    struct Frame {
        SeqBDD f;
        SeqBDD f0;
        bool pushed;  // did we push a variable level onto arr?
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f;
    init.f0 = SeqBDD(0);
    init.pushed = false;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            SeqBDD f = frame.f;
            if ((f & SeqBDD(1)) == SeqBDD(1)) {
                if (flag) os << " + ";
                flag = true;
                if (idx == 0) {
                    os << "e";
                } else {
                    for (int i = 0; i < idx; i++) {
                        if (i > 0) os << " ";
                        os << arr[i];
                    }
                }
                f -= SeqBDD(1);
            }
            if (f == SeqBDD(0)) { stack.pop_back(); break; }

            int ftop = f.top();
            SeqBDD f1 = f.onset0(ftop);
            SeqBDD f0 = f.offset(ftop);
            if (idx >= static_cast<int>(arr.size())) {
                throw std::overflow_error("print_seq_iter: sequence depth exceeds expected maximum length");
            }
            arr[idx] = static_cast<int>(bddlevofvar(static_cast<bddvar>(ftop)));
            idx++;

            frame.pushed = true;
            frame.f0 = f0;
            frame.phase = Phase::AFTER_F1;

            Frame child;
            child.f = f1;
            child.f0 = SeqBDD(0);
            child.pushed = false;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::AFTER_F1: {
            if (frame.pushed) idx--;
            frame.phase = Phase::AFTER_F0;
            Frame child;
            child.f = frame.f0;
            child.f0 = SeqBDD(0);
            child.pushed = false;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::AFTER_F0: {
            stack.pop_back();
            break;
        }
        }
    }
}

static void print_seq_rec(std::ostream& os, SeqBDD f,
                          std::vector<int>& arr, int& idx, bool& flag)
{
    BDD_RecurGuard guard;
    /* Check if empty sequence (epsilon) is in f */
    if ((f & SeqBDD(1)) == SeqBDD(1)) {
        if (flag) os << " + ";
        flag = true;
        if (idx == 0) {
            os << "e";
        } else {
            for (int i = 0; i < idx; i++) {
                if (i > 0) os << " ";
                os << arr[i];
            }
        }
        f -= SeqBDD(1);
    }

    if (f == SeqBDD(0)) return;

    int ftop = f.top();

    /* Recurse on sequences starting with ftop */
    SeqBDD f1 = f.onset0(ftop);
    if (idx >= static_cast<int>(arr.size())) {
        throw std::overflow_error("print_seq_rec: sequence depth exceeds expected maximum length");
    }
    arr[idx] = static_cast<int>(bddlevofvar(static_cast<bddvar>(ftop)));
    idx++;
    print_seq_rec(os, f1, arr, idx, flag);
    idx--;

    /* Recurse on sequences NOT starting with ftop */
    SeqBDD f0 = f.offset(ftop);
    print_seq_rec(os, f0, arr, idx, flag);
}

void SeqBDD::print_seq() const
{
    if (*this == SeqBDD(-1)) {
        std::cout << "(undefined)" << std::endl;
        return;
    }
    if (*this == SeqBDD(0)) {
        std::cout << "(empty)" << std::endl;
        return;
    }

    uint64_t maxlen = len();
    std::vector<int> arr(maxlen > 0 ? maxlen : 1);
    int idx = 0;
    bool flag = false;

    if (use_iter_1op(zdd_.GetID())) {
        print_seq_iter(std::cout, *this, arr, idx, flag);
    } else {
        print_seq_rec(std::cout, *this, arr, idx, flag);
    }
    std::cout << std::endl;
}

std::string SeqBDD::seq_str() const
{
    if (*this == SeqBDD(-1)) return "(undefined)";
    if (*this == SeqBDD(0)) return "(empty)";

    uint64_t maxlen = len();
    std::vector<int> arr(maxlen > 0 ? maxlen : 1);
    int idx = 0;
    bool flag = false;

    std::ostringstream oss;
    if (use_iter_1op(zdd_.GetID())) {
        print_seq_iter(oss, *this, arr, idx, flag);
    } else {
        print_seq_rec(oss, *this, arr, idx, flag);
    }
    return oss.str();
}

} // namespace kyotodd
