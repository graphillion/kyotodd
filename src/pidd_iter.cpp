// Iterative (explicit-stack) counterparts of the recursive PiDD operations
// in src/pidd.cpp. Each _iter shares its cache op code with the matching
// _rec, so results are interchangeable. Frames hold PiDD objects, whose
// internal ZDDs are registered as GC roots, so no bdd_gc_guard is needed
// (unlike the raw-bddp _iter functions elsewhere in the library).
//
// Sub-operations of a different kind (e.g. the swap applied after a
// cofact) are delegated to that operation's own _iter, which keeps the
// native stack depth bounded by a small constant.

#include "pidd.h"
#include "bdd_internal.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace kyotodd {

namespace {

// Generic frame phases: ENTER decomposes and pushes the first sub-problem;
// GOT_0 / GOT_1 / GOT_2 receive the results of the successive sub-problems.
enum class PiddPhase : uint8_t { ENTER, GOT_0, GOT_1, GOT_2 };

} // anonymous namespace

/* ================================================================ */
/*  swap_iter                                                       */
/* ================================================================ */

PiDD PiDD::swap_iter(const PiDD& root, int root_u, int root_v)
{
    struct Frame {
        PiDD p;
        int u, v;
        int x, y;
        bddp fx, gx;      // cache key
        PiDD p1;          // 1-branch: operand of the second sub-problem
        PiDD r0;          // swap(p0, u, v)
        PiddPhase phase;
        Frame(const PiDD& p_, int u_, int v_)
            : p(p_), u(u_), v(v_), x(0), y(0), fx(0), gx(0), p1(), r0(),
              phase(PiddPhase::ENTER) {}
    };

    PiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root, root_u, root_v));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case PiddPhase::ENTER: {
            if (fr.u == fr.v) { result = fr.p; stack.pop_back(); break; }
            if (fr.u < fr.v) std::swap(fr.u, fr.v);
            /* Now u > v */

            int x = fr.p.top_x();
            if (x < fr.u) {
                /* No transposition involves u; just toggle (u, v) */
                bddvar bv = bddvaroflev(
                    static_cast<bddvar>(PiDD_Lev_XY(fr.u, fr.v)));
                result = PiDD(fr.p.zdd_.Change(bv));
                stack.pop_back();
                break;
            }

            /* Cache lookup */
            fr.fx = fr.p.zdd_.GetID();
            fr.gx = static_cast<bddp>(fr.u * (PiDD_MaxVar + 1) + fr.v);
            ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_SWAP, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = PiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            fr.x = x;
            fr.y = fr.p.top_y();
            PiDD p0(fr.p.zdd_.OffSet(top));
            fr.p1 = PiDD(fr.p.zdd_.OnSet0(top));
            fr.phase = PiddPhase::GOT_0;
            Frame child(p0, fr.u, fr.v);
            stack.push_back(child);  // fr is invalid after this
            break;
        }
        case PiddPhase::GOT_0: {
            fr.r0 = result;
            fr.phase = PiddPhase::GOT_1;
            Frame child(fr.p1, PiDD_U_XYU(fr.x, fr.y, fr.u), fr.v);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_1: {
            fr.phase = PiddPhase::GOT_2;
            Frame child(result, fr.x, PiDD_Y_YUV(fr.y, fr.u, fr.v));
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_2: {
            PiDD r = fr.r0 + result;
            /* Cache store */
            bddwcache(BDD_OP_PIDD_SWAP, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

/* ================================================================ */
/*  cofact_iter                                                     */
/* ================================================================ */

PiDD PiDD::cofact_iter(const PiDD& root, int root_u, int root_v)
{
    struct Frame {
        PiDD p;
        int u, v;
        int x, y;
        bddp fx, gx;
        PiDD p1;
        PiDD r0;          // cofact(p0, u, v)
        bool tail;        // true: the result of cofact(p0) is the answer
        PiddPhase phase;
        Frame(const PiDD& p_, int u_, int v_)
            : p(p_), u(u_), v(v_), x(0), y(0), fx(0), gx(0), p1(), r0(),
              tail(false), phase(PiddPhase::ENTER) {}
    };

    PiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root, root_u, root_v));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case PiddPhase::ENTER: {
            int x = fr.p.top_x();
            int y = fr.p.top_y();

            /* Terminal / early-exit cases */
            if (x < fr.u && x < fr.v) {
                result = (fr.u == fr.v) ? fr.p : PiDD(0);
                stack.pop_back();
                break;
            }
            if (x == fr.u && y > fr.v) {
                result = PiDD(0);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            PiDD p0(fr.p.zdd_.OffSet(top));
            PiDD p1(fr.p.zdd_.OnSet0(top));

            if (x == fr.u) {
                if (y == fr.v) { result = p1; stack.pop_back(); break; }
                fr.tail = true;
                fr.phase = PiddPhase::GOT_0;
                Frame child(p0, fr.u, fr.v);
                stack.push_back(child);
                break;
            }
            if (y == fr.v) {
                fr.tail = true;
                fr.phase = PiddPhase::GOT_0;
                Frame child(p0, fr.u, fr.v);
                stack.push_back(child);
                break;
            }

            /* Cache lookup */
            fr.fx = fr.p.zdd_.GetID();
            fr.gx = static_cast<bddp>(fr.u * (PiDD_MaxVar + 1) + fr.v);
            ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_COFACT, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = PiDD(cache_result);
                stack.pop_back();
                break;
            }

            fr.x = x;
            fr.y = y;
            fr.p1 = p1;
            fr.phase = PiddPhase::GOT_0;
            Frame child(p0, fr.u, fr.v);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_0: {
            if (fr.tail) { stack.pop_back(); break; }  // pass result through
            fr.r0 = result;
            fr.phase = PiddPhase::GOT_1;
            if (fr.u >= fr.v) {
                Frame child(fr.p1, fr.u, fr.v);
                stack.push_back(child);
            } else {
                Frame child(fr.p1, fr.u, PiDD_U_XYU(fr.x, fr.y, fr.v));
                stack.push_back(child);
            }
            break;
        }
        case PiddPhase::GOT_1: {
            PiDD s = (fr.u >= fr.v)
                ? swap_iter(result, fr.x, PiDD_Y_YUV(fr.y, fr.u, fr.v))
                : swap_iter(result, fr.x, PiDD_Y_YUV(fr.y, fr.v, fr.u));
            PiDD r = fr.r0 + s;
            /* Cache store */
            bddwcache(BDD_OP_PIDD_COFACT, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case PiddPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  odd_iter                                                        */
/* ================================================================ */

PiDD PiDD::odd_iter(const PiDD& root)
{
    struct Frame {
        PiDD p;
        int x, y;
        bddp fx;
        PiDD p1;
        PiDD r0;          // odd(p0)
        PiddPhase phase;
        explicit Frame(const PiDD& p_)
            : p(p_), x(0), y(0), fx(0), p1(), r0(), phase(PiddPhase::ENTER) {}
    };

    PiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case PiddPhase::ENTER: {
            if (fr.p.top_x() == 0) {  /* identity permutation is even */
                result = PiDD(0);
                stack.pop_back();
                break;
            }

            /* Cache lookup */
            fr.fx = fr.p.zdd_.GetID();
            ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_ODD, fr.fx, 0);
            if (cache_result != ZDD::Null) {
                result = PiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            fr.x = fr.p.top_x();
            fr.y = fr.p.top_y();
            PiDD p0(fr.p.zdd_.OffSet(top));
            fr.p1 = PiDD(fr.p.zdd_.OnSet0(top));
            fr.phase = PiddPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_0: {
            fr.r0 = result;
            fr.phase = PiddPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_1: {
            /* Odd of p0; Even of p1 swapped back (parity flips) */
            PiDD r = fr.r0 + swap_iter(fr.p1 - result, fr.x, fr.y);
            /* Cache store */
            bddwcache(BDD_OP_PIDD_ODD, fr.fx, 0, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case PiddPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  multiply_iter (composition)                                     */
/* ================================================================ */

PiDD PiDD::multiply_iter(const PiDD& root_p, const PiDD& root_q)
{
    struct Frame {
        PiDD p, q;
        int qx, qy;
        bddp fx, gx;
        PiDD q1;
        PiDD r0;          // p * q0
        PiddPhase phase;
        Frame(const PiDD& p_, const PiDD& q_)
            : p(p_), q(q_), qx(0), qy(0), fx(0), gx(0), q1(), r0(),
              phase(PiddPhase::ENTER) {}
    };

    PiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root_p, root_q));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case PiddPhase::ENTER: {
            bddp pid = fr.p.zdd_.GetID();
            bddp qid = fr.q.zdd_.GetID();
            if (pid == bddempty || qid == bddempty) {
                result = PiDD(0); stack.pop_back(); break;
            }
            if (pid == bddsingle) { result = fr.q; stack.pop_back(); break; }
            if (qid == bddsingle) { result = fr.p; stack.pop_back(); break; }
            if (pid == bddnull || qid == bddnull) {
                result = PiDD(-1); stack.pop_back(); break;
            }

            /* Cache lookup */
            fr.fx = pid;
            fr.gx = qid;
            ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_MULT, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = PiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Decompose q */
            fr.qx = fr.q.top_x();
            fr.qy = fr.q.top_y();
            bddvar top = fr.q.zdd_.Top();
            PiDD q0(fr.q.zdd_.OffSet(top));
            fr.q1 = PiDD(fr.q.zdd_.OnSet0(top));
            fr.phase = PiddPhase::GOT_0;
            Frame child(fr.p, q0);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_0: {
            fr.r0 = result;
            fr.phase = PiddPhase::GOT_1;
            Frame child(fr.p, fr.q1);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_1: {
            PiDD r = fr.r0 + swap_iter(result, fr.qx, fr.qy);
            /* Cache store */
            bddwcache(BDD_OP_PIDD_MULT, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case PiddPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  divide_iter (division)                                          */
/* ================================================================ */

PiDD PiDD::divide_iter(const PiDD& root_f, const PiDD& root_p)
{
    struct Frame {
        PiDD f, p;
        int py;
        bddvar top;
        bddp fx, gx;
        PiDD q;           // partial quotient while waiting for f / p0
        PiddPhase phase;
        Frame(const PiDD& f_, const PiDD& p_)
            : f(f_), p(p_), py(0), top(0), fx(0), gx(0), q(),
              phase(PiddPhase::ENTER) {}
    };

    PiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root_f, root_p));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case PiddPhase::ENTER: {
            bddp fid = fr.f.zdd_.GetID();
            bddp pid = fr.p.zdd_.GetID();
            if (fid == bddnull || pid == bddnull) {
                result = PiDD(-1); stack.pop_back(); break;
            }
            if (pid == bddsingle) { result = fr.f; stack.pop_back(); break; }
            if (pid == bddempty) {
                throw std::invalid_argument("PiDD: division by zero");
            }
            if (fr.f.top_x() < fr.p.top_x()) {
                result = PiDD(0); stack.pop_back(); break;
            }

            /* Cache lookup */
            fr.fx = fid;
            fr.gx = pid;
            ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_DIV, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = PiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Decompose p */
            int px = fr.p.top_x();
            fr.py = fr.p.top_y();
            fr.top = fr.p.zdd_.Top();
            PiDD p1(fr.p.zdd_.OnSet0(fr.top));

            /* q = (f.cofact(px, py) / p1).cofact(py, py) */
            PiDD fc = cofact_iter(fr.f, px, fr.py);
            fr.phase = PiddPhase::GOT_0;
            Frame child(fc, p1);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_0: {
            PiDD q = cofact_iter(result, fr.py, fr.py);
            if (q.zdd_.GetID() != bddempty) {
                PiDD p0(fr.p.zdd_.OffSet(fr.top));
                if (p0.zdd_.GetID() != bddempty) {
                    fr.q = q;
                    fr.phase = PiddPhase::GOT_1;
                    Frame child(fr.f, p0);
                    stack.push_back(child);
                    break;
                }
            }
            /* Cache store */
            bddwcache(BDD_OP_PIDD_DIV, fr.fx, fr.gx, q.zdd_.GetID());
            result = q;
            stack.pop_back();
            break;
        }
        case PiddPhase::GOT_1: {
            PiDD q = fr.q & result;
            /* Cache store */
            bddwcache(BDD_OP_PIDD_DIV, fr.fx, fr.gx, q.zdd_.GetID());
            result = q;
            stack.pop_back();
            break;
        }
        case PiddPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  enumerate_iter — permutation vector notation                    */
/* ================================================================ */

void PiDD::enumerate_iter(const PiDD& root, int dim, int* varmap, bool& flag)
{
    struct Frame {
        PiDD p;
        int x, y;
        PiDD p1;
        PiddPhase phase;
        explicit Frame(const PiDD& p_)
            : p(p_), x(0), y(0), p1(), phase(PiddPhase::ENTER) {}
    };

    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case PiddPhase::ENTER: {
            if (fr.p == PiDD(0)) { stack.pop_back(); break; }
            if (fr.p == PiDD(1)) {
                enumerate_print(dim, varmap, flag);
                stack.pop_back();
                break;
            }

            fr.x = fr.p.top_x();
            fr.y = fr.p.top_y();

            /* p1 = part containing (x,y); p0 = part not containing (x,y) */
            fr.p1 = cofact_iter(fr.p, fr.x, fr.y);
            PiDD p0 = fr.p - swap_iter(fr.p1, fr.x, fr.y);

            /* Enumerate p0 first */
            fr.phase = PiddPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_0: {
            /* Apply transposition (x, y) to VarMap and enumerate p1 */
            std::swap(varmap[fr.x - 1], varmap[fr.y - 1]);
            fr.phase = PiddPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_1: {
            /* Restore VarMap */
            std::swap(varmap[fr.x - 1], varmap[fr.y - 1]);
            stack.pop_back();
            break;
        }
        case PiddPhase::GOT_2:
            break;  // unused
        }
    }
}

/* ================================================================ */
/*  enumerate2_iter — transposition notation                        */
/* ================================================================ */

void PiDD::enumerate2_iter(const PiDD& root, std::vector<int>& levs, bool& flag)
{
    struct Frame {
        PiDD p;
        int lev;
        PiDD p1;
        PiddPhase phase;
        explicit Frame(const PiDD& p_)
            : p(p_), lev(0), p1(), phase(PiddPhase::ENTER) {}
    };

    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case PiddPhase::ENTER: {
            if (fr.p == PiDD(0)) { stack.pop_back(); break; }
            if (fr.p == PiDD(1)) {
                enumerate2_print(levs, flag);
                stack.pop_back();
                break;
            }

            int x = fr.p.top_x();
            int y = fr.p.top_y();
            fr.lev = fr.p.top_lev();

            /* p1 = part containing (x,y); p0 = part not containing (x,y) */
            fr.p1 = cofact_iter(fr.p, x, y);
            PiDD p0 = fr.p - swap_iter(fr.p1, x, y);

            /* Enumerate p0 first */
            fr.phase = PiddPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_0: {
            /* Record this level and enumerate p1 */
            levs.push_back(fr.lev);
            fr.phase = PiddPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_1: {
            levs.pop_back();
            stack.pop_back();
            break;
        }
        case PiddPhase::GOT_2:
            break;  // unused
        }
    }
}

/* ================================================================ */
/*  to_perms_iter — collect permutations as Permutation objects      */
/* ================================================================ */

void PiDD::to_perms_iter(const PiDD& root, int dim, int* varmap,
                         std::vector<Permutation>& result)
{
    struct Frame {
        PiDD p;
        int x, y;
        PiDD p1;
        PiddPhase phase;
        explicit Frame(const PiDD& p_)
            : p(p_), x(0), y(0), p1(), phase(PiddPhase::ENTER) {}
    };

    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case PiddPhase::ENTER: {
            if (fr.p == PiDD(0)) { stack.pop_back(); break; }
            if (fr.p == PiDD(1)) {
                result.push_back(Permutation(std::vector<int>(varmap, varmap + dim)));
                stack.pop_back();
                break;
            }

            fr.x = fr.p.top_x();
            fr.y = fr.p.top_y();

            /* p1 = part containing (x,y); p0 = part not containing (x,y) */
            fr.p1 = cofact_iter(fr.p, fr.x, fr.y);
            PiDD p0 = fr.p - swap_iter(fr.p1, fr.x, fr.y);

            /* Collect p0 first (VarMap unchanged) */
            fr.phase = PiddPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_0: {
            /* Apply transposition (x, y) to VarMap and collect p1 */
            std::swap(varmap[fr.x - 1], varmap[fr.y - 1]);
            fr.phase = PiddPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case PiddPhase::GOT_1: {
            /* Restore VarMap */
            std::swap(varmap[fr.x - 1], varmap[fr.y - 1]);
            stack.pop_back();
            break;
        }
        case PiddPhase::GOT_2:
            break;  // unused
        }
    }
}

} // namespace kyotodd
