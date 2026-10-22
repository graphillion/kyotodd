// Iterative (explicit-stack) counterparts of the recursive RotPiDD
// operations in src/rotpidd.cpp. Each _iter shares its cache op code with
// the matching _rec, so results are interchangeable. Frames hold RotPiDD
// objects, whose internal ZDDs are registered as GC roots, so no
// bdd_gc_guard is needed (unlike the raw-bddp _iter functions elsewhere).
//
// Sub-operations of a different kind (e.g. the left_rot applied after a
// cofact, or the composition inside inverse) are delegated to that
// operation's own _iter, which keeps the native stack depth bounded by a
// small constant.

#include "rotpidd.h"
#include "bdd_internal.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kyotodd {

namespace {

// Generic frame phases: ENTER decomposes and pushes the first sub-problem;
// GOT_0 / GOT_1 / GOT_2 receive the results of the successive sub-problems.
enum class RotPhase : uint8_t { ENTER, GOT_0, GOT_1, GOT_2 };

} // anonymous namespace

/* ================================================================ */
/*  left_rot_iter                                                    */
/* ================================================================ */

RotPiDD RotPiDD::left_rot_iter(const RotPiDD& root, int root_u, int root_v)
{
    struct Frame {
        RotPiDD p;
        int u, v;
        int x, y;
        int ny;           // y of the final LeftRot(x, ny)
        bddp fx, gx;      // cache key
        RotPiDD p1;       // 1-branch
        RotPiDD np0;      // left_rot(p0, u, v)
        RotPhase phase;
        Frame(const RotPiDD& p_, int u_, int v_)
            : p(p_), u(u_), v(v_), x(0), y(0), ny(0), fx(0), gx(0), p1(), np0(),
              phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root, root_u, root_v));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            if (fr.u == fr.v) { result = fr.p; stack.pop_back(); break; }
            if (fr.u < fr.v) std::swap(fr.u, fr.v);
            /* Now u > v */

            int x = fr.p.top_x();
            if (x < fr.u) {
                /* All variables are below u; just toggle (u, v) */
                bddvar bv = bddvaroflev(
                    static_cast<bddvar>(RotPiDD_Lev_XY(fr.u, fr.v)));
                result = RotPiDD(fr.p.zdd_.Change(bv));
                stack.pop_back();
                break;
            }

            /* Cache lookup */
            fr.fx = fr.p.zdd_.GetID();
            fr.gx = static_cast<bddp>(fr.u * (RotPiDD_MaxVar + 1) + fr.v);
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_LEFTROT, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            fr.x = x;
            fr.y = fr.p.top_y();
            RotPiDD p0(fr.p.zdd_.OffSet(top));
            fr.p1 = RotPiDD(fr.p.zdd_.OnSet0(top));
            fr.phase = RotPhase::GOT_0;
            Frame child(p0, fr.u, fr.v);
            stack.push_back(child);  // fr is invalid after this
            break;
        }
        case RotPhase::GOT_0: {
            fr.np0 = result;
            /* Recursive computation based on relationship between [v..u] and [y..x] */
            int u = fr.u, v = fr.v, x = fr.x, y = fr.y;
            if (u < y) {
                /* [v..u] is entirely below [y..x], commutative */
                fr.ny = y;
                fr.phase = RotPhase::GOT_1;
                Frame child(fr.p1, u, v);
                stack.push_back(child);
            } else if (u == y) {
                /* LeftRot(u, v) o LeftRot(x, u) = LeftRot(x, v) */
                fr.ny = v;
                fr.phase = RotPhase::GOT_2;
                Frame child(fr.p1, x, v);
                stack.push_back(child);
            } else if (v <= y) {
                /* Overlapping: LeftRot(u, v) o LeftRot(x, y) = LeftRot(x, y+1) o LeftRot(u-1, v) */
                fr.ny = y + 1;
                fr.phase = RotPhase::GOT_1;
                Frame child(fr.p1, u - 1, v);
                stack.push_back(child);
            } else {
                /* [v..u] inside [y..x]: LeftRot(u, v) o LeftRot(x, y) = LeftRot(x, y) o LeftRot(u-1, v-1) */
                fr.ny = y;
                fr.phase = RotPhase::GOT_1;
                Frame child(fr.p1, u - 1, v - 1);
                stack.push_back(child);
            }
            break;
        }
        case RotPhase::GOT_1: {
            /* result holds np1; apply the final LeftRot(x, ny) */
            fr.phase = RotPhase::GOT_2;
            Frame child(result, fr.x, fr.ny);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_2: {
            RotPiDD r = fr.np0 + result;
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_LEFTROT, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

/* ================================================================ */
/*  cofact_iter                                                      */
/* ================================================================ */

RotPiDD RotPiDD::cofact_iter(const RotPiDD& root, int root_u, int root_v)
{
    struct Frame {
        RotPiDD p;
        int u, v;
        int x, y;
        bddp fx, gx;
        RotPiDD p1;
        RotPiDD r0;       // cofact(p0, u, v)
        bool tail;        // true: the result of cofact(p0) is the answer
        RotPhase phase;
        Frame(const RotPiDD& p_, int u_, int v_)
            : p(p_), u(u_), v(v_), x(0), y(0), fx(0), gx(0), p1(), r0(),
              tail(false), phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root, root_u, root_v));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            int x = fr.p.top_x();
            int y = fr.p.top_y();

            /* Terminal / early-exit cases */
            if (x < fr.u || x < fr.v) {
                result = (fr.u == fr.v) ? fr.p : RotPiDD(0);
                stack.pop_back();
                break;
            }
            if (x == fr.u && y > fr.v) {
                result = RotPiDD(0);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            RotPiDD p0(fr.p.zdd_.OffSet(top));
            RotPiDD p1(fr.p.zdd_.OnSet0(top));

            if (x == fr.u) {
                if (y == fr.v) { result = p1; stack.pop_back(); break; }
                fr.tail = true;
                fr.phase = RotPhase::GOT_0;
                Frame child(p0, fr.u, fr.v);
                stack.push_back(child);
                break;
            }
            if (y == fr.v) {
                fr.tail = true;
                fr.phase = RotPhase::GOT_0;
                Frame child(p0, fr.u, fr.v);
                stack.push_back(child);
                break;
            }

            /* Cache lookup */
            fr.fx = fr.p.zdd_.GetID();
            fr.gx = static_cast<bddp>(fr.u * (RotPiDD_MaxVar + 1) + fr.v);
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_COFACT, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            fr.x = x;
            fr.y = y;
            fr.p1 = p1;
            fr.phase = RotPhase::GOT_0;
            Frame child(p0, fr.u, fr.v);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            if (fr.tail) { stack.pop_back(); break; }  // pass result through
            fr.r0 = result;
            fr.phase = RotPhase::GOT_1;
            /* See cofact_rec: the 1-branch needs pi'(u) = v' with
               v' = v (v < y) or v - 1 (v > y). */
            Frame child(fr.p1, fr.u, (fr.v < fr.y) ? fr.v : fr.v - 1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            /* LeftRot(x - 1, y') with y' = y - 1 (v < y) or y (v > y). */
            RotPiDD s = (fr.v < fr.y)
                ? left_rot_iter(result, fr.x - 1, fr.y - 1)
                : left_rot_iter(result, fr.x - 1, fr.y);
            RotPiDD r = fr.r0 + s;
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_COFACT, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  odd_iter                                                         */
/* ================================================================ */

RotPiDD RotPiDD::odd_iter(const RotPiDD& root)
{
    struct Frame {
        RotPiDD p;
        int x, y;
        bddp fx;
        RotPiDD p1;
        RotPiDD r0;       // odd(p0)
        RotPhase phase;
        explicit Frame(const RotPiDD& p_)
            : p(p_), x(0), y(0), fx(0), p1(), r0(), phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            if (fr.p.top_x() == 0) {  /* identity permutation is even */
                result = RotPiDD(0);
                stack.pop_back();
                break;
            }

            /* Cache lookup */
            fr.fx = fr.p.zdd_.GetID();
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_ODD, fr.fx, 0);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            fr.x = fr.p.top_x();
            fr.y = fr.p.top_y();
            RotPiDD p0(fr.p.zdd_.OffSet(top));
            fr.p1 = RotPiDD(fr.p.zdd_.OnSet0(top));
            fr.phase = RotPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            fr.r0 = result;
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            /* LeftRot(x, y) is (x - y) transpositions, so parity = (x - y) mod 2 */
            RotPiDD branch = ((fr.x - fr.y) % 2 != 0)
                ? fr.p1 - result   /* Odd number of transpositions: parity flips */
                : result;          /* Even number of transpositions: parity preserved */
            RotPiDD r = fr.r0 + left_rot_iter(branch, fr.x, fr.y);
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_ODD, fr.fx, 0, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  multiply_iter (composition)                                      */
/* ================================================================ */

RotPiDD RotPiDD::multiply_iter(const RotPiDD& root_p, const RotPiDD& root_q)
{
    struct Frame {
        RotPiDD p, q;
        int qx, qy;
        bddp fx, gx;
        RotPiDD q1;
        RotPiDD r0;       // p * q0
        RotPhase phase;
        Frame(const RotPiDD& p_, const RotPiDD& q_)
            : p(p_), q(q_), qx(0), qy(0), fx(0), gx(0), q1(), r0(),
              phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root_p, root_q));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            bddp pid = fr.p.zdd_.GetID();
            bddp qid = fr.q.zdd_.GetID();
            if (pid == bddempty || qid == bddempty) {
                result = RotPiDD(0); stack.pop_back(); break;
            }
            if (pid == bddsingle) { result = fr.q; stack.pop_back(); break; }
            if (qid == bddsingle) { result = fr.p; stack.pop_back(); break; }
            if (pid == bddnull || qid == bddnull) {
                result = RotPiDD(-1); stack.pop_back(); break;
            }

            /* Cache lookup */
            fr.fx = pid;
            fr.gx = qid;
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_MULT, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Decompose q */
            fr.qx = fr.q.top_x();
            fr.qy = fr.q.top_y();
            bddvar top = fr.q.zdd_.Top();
            RotPiDD q0(fr.q.zdd_.OffSet(top));
            fr.q1 = RotPiDD(fr.q.zdd_.OnSet0(top));
            fr.phase = RotPhase::GOT_0;
            Frame child(fr.p, q0);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            fr.r0 = result;
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p, fr.q1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            RotPiDD r = fr.r0 + left_rot_iter(result, fr.qx, fr.qy);
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_MULT, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  order_iter                                                       */
/* ================================================================ */

RotPiDD RotPiDD::order_iter(const RotPiDD& root, int root_a, int root_b)
{
    struct Frame {
        RotPiDD p;
        int a, b;
        int x, y;
        bddp fx, gx;
        RotPiDD p1;
        RotPiDD r0;       // order(p0, a, b)
        RotPhase phase;
        Frame(const RotPiDD& p_, int a_, int b_)
            : p(p_), a(a_), b(b_), x(0), y(0), fx(0), gx(0), p1(), r0(),
              phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root, root_a, root_b));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            bddp pid = fr.p.zdd_.GetID();
            if (pid == bddnull) { result = RotPiDD(-1); stack.pop_back(); break; }
            if (pid == bddempty) { result = RotPiDD(0); stack.pop_back(); break; }
            if (pid == bddsingle) {
                result = (fr.a < fr.b) ? RotPiDD(1) : RotPiDD(0);
                stack.pop_back();
                break;
            }

            int x = fr.p.top_x();
            if (x < fr.a && fr.b < fr.a) { result = RotPiDD(0); stack.pop_back(); break; }
            if (x < fr.b && fr.a < fr.b) { result = fr.p; stack.pop_back(); break; }

            /* Cache lookup */
            fr.fx = pid;
            fr.gx = static_cast<bddp>(fr.a * (RotPiDD_MaxVar + 1) + fr.b);
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_ORDER, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            fr.x = x;
            fr.y = fr.p.top_y();
            RotPiDD p0(fr.p.zdd_.OffSet(top));
            fr.p1 = RotPiDD(fr.p.zdd_.OnSet0(top));
            fr.phase = RotPhase::GOT_0;
            Frame child(p0, fr.a, fr.b);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            fr.r0 = result;
            if (fr.y == fr.b) {
                RotPiDD r = fr.r0 + left_rot_iter(fr.p1, fr.x, fr.y);
                bddwcache(BDD_OP_ROTPIDD_ORDER, fr.fx, fr.gx, r.zdd_.GetID());
                result = r;
                stack.pop_back();
                break;
            }
            if (fr.y != fr.a) {
                int na = (fr.y < fr.a) ? fr.a - 1 : fr.a;
                int nb = (fr.y < fr.b) ? fr.b - 1 : fr.b;
                fr.phase = RotPhase::GOT_1;
                Frame child(fr.p1, na, nb);
                stack.push_back(child);
                break;
            }
            /* if y == a, nothing is added */
            bddwcache(BDD_OP_ROTPIDD_ORDER, fr.fx, fr.gx, fr.r0.zdd_.GetID());
            result = fr.r0;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_1: {
            RotPiDD r = fr.r0 + left_rot_iter(result, fr.x, fr.y);
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_ORDER, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  inverse_iter                                                     */
/* ================================================================ */

RotPiDD RotPiDD::inverse_iter(const RotPiDD& root)
{
    struct Frame {
        RotPiDD p;
        int x, y;
        bddp fx;
        RotPiDD p1;
        RotPiDD r0;       // inverse(p0)
        RotPhase phase;
        explicit Frame(const RotPiDD& p_)
            : p(p_), x(0), y(0), fx(0), p1(), r0(), phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            bddp pid = fr.p.zdd_.GetID();
            if (pid == bddnull || pid == bddempty || pid == bddsingle) {
                result = fr.p; stack.pop_back(); break;
            }

            /* Cache lookup */
            fr.fx = pid;
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_INVERSE, fr.fx, 0);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            fr.x = fr.p.top_x();
            fr.y = fr.p.top_y();
            RotPiDD p0(fr.p.zdd_.OffSet(top));
            fr.p1 = RotPiDD(fr.p.zdd_.OnSet0(top));
            fr.phase = RotPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            fr.r0 = result;
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            /* Build inverse of LeftRot(x, y): right rotation = LeftRot(y,y+1) o ... o LeftRot(x-1,x) */
            RotPiDD invrot(1);
            for (int i = fr.y; i <= fr.x - 1; i++) {
                invrot = left_rot_iter(invrot, i, i + 1);
            }
            RotPiDD r = fr.r0 + multiply_iter(invrot, result);
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_INVERSE, fr.fx, 0, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  insert_iter                                                      */
/* ================================================================ */

RotPiDD RotPiDD::insert_iter(const RotPiDD& root, int root_p, int root_v)
{
    struct Frame {
        RotPiDD f;
        int p, v;
        int nx, ny;
        bddp fx, gx;
        RotPiDD p1;
        RotPiDD r0;       // insert(p0, p, v)
        RotPhase phase;
        Frame(const RotPiDD& f_, int p_, int v_)
            : f(f_), p(p_), v(v_), nx(0), ny(0), fx(0), gx(0), p1(), r0(),
              phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root, root_p, root_v));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            bddp fid = fr.f.zdd_.GetID();
            if (fid == bddempty || fid == bddnull) {
                result = fr.f; stack.pop_back(); break;
            }

            /* Cache lookup */
            fr.fx = fid;
            fr.gx = static_cast<bddp>(fr.p * (RotPiDD_MaxVar + 1) + fr.v);
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_INSERT, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            int x = fr.f.top_x();
            int y = fr.f.top_y();

            if (fid == bddsingle || x + 1 < fr.v || x < fr.p) {
                RotPiDD r = left_rot_iter(fr.f, fr.p, fr.v);
                bddwcache(BDD_OP_ROTPIDD_INSERT, fr.fx, fr.gx, r.zdd_.GetID());
                result = r;
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.f.zdd_.Top();
            RotPiDD p0(fr.f.zdd_.OffSet(top));
            fr.p1 = RotPiDD(fr.f.zdd_.OnSet0(top));

            fr.nx = x + (fr.p <= x ? 1 : 0);
            fr.ny = y + (fr.v <= y ? 1 : 0);
            fr.phase = RotPhase::GOT_0;
            Frame child(p0, fr.p, fr.v);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            fr.r0 = result;
            int y = fr.f.top_y();
            int np = fr.p;
            int nv = fr.v - (y < fr.v ? 1 : 0);
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p1, np, nv);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            RotPiDD r = fr.r0 + left_rot_iter(result, fr.nx, fr.ny);
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_INSERT, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  remove_max_iter                                                  */
/* ================================================================ */

RotPiDD RotPiDD::remove_max_iter(const RotPiDD& root, int root_k)
{
    struct Frame {
        RotPiDD p;
        int k;
        int x, y;
        bddp fx, gx;
        RotPiDD p1;
        RotPiDD r0;       // remove_max(p0, k)
        RotPhase phase;
        Frame(const RotPiDD& p_, int k_)
            : p(p_), k(k_), x(0), y(0), fx(0), gx(0), p1(), r0(),
              phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root, root_k));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            bddp pid = fr.p.zdd_.GetID();
            if (fr.k <= 0 || pid == bddempty || pid == bddnull || pid == bddsingle) {
                result = fr.p; stack.pop_back(); break;
            }
            int x = fr.p.top_x();
            if (x < fr.k) { result = fr.p; stack.pop_back(); break; }

            /* Cache lookup */
            fr.fx = pid;
            fr.gx = static_cast<bddp>(fr.k);
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_REMOVEMAX, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            fr.x = x;
            fr.y = fr.p.top_y();
            RotPiDD p0(fr.p.zdd_.OffSet(top));
            fr.p1 = RotPiDD(fr.p.zdd_.OnSet0(top));
            fr.phase = RotPhase::GOT_0;
            Frame child(p0, fr.k);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            fr.r0 = result;
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p1, fr.k - 1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            RotPiDD r = fr.r0 + left_rot_iter(result, fr.x - 1, fr.y);
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_REMOVEMAX, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  normalize_iter (normalizeRotPiDD)                                */
/* ================================================================ */

RotPiDD RotPiDD::normalize_iter(const RotPiDD& root, int k)
{
    struct Frame {
        RotPiDD p;
        bddp fx, gx;
        RotPiDD p1;
        RotPiDD r0;       // normalize(p0, k)
        RotPhase phase;
        explicit Frame(const RotPiDD& p_)
            : p(p_), fx(0), gx(0), p1(), r0(), phase(RotPhase::ENTER) {}
    };

    RotPiDD result;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            int x = fr.p.top_x();
            if (x <= k) { result = fr.p; stack.pop_back(); break; }

            /* Cache lookup */
            fr.fx = fr.p.zdd_.GetID();
            fr.gx = static_cast<bddp>(k);
            ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_NORMALIZE, fr.fx, fr.gx);
            if (cache_result != ZDD::Null) {
                result = RotPiDD(cache_result);
                stack.pop_back();
                break;
            }

            /* Shannon decomposition */
            bddvar top = fr.p.zdd_.Top();
            RotPiDD p0(fr.p.zdd_.OffSet(top));
            fr.p1 = RotPiDD(fr.p.zdd_.OnSet0(top));
            fr.phase = RotPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            fr.r0 = result;
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            RotPiDD r = fr.r0 + result;
            /* Cache store */
            bddwcache(BDD_OP_ROTPIDD_NORMALIZE, fr.fx, fr.gx, r.zdd_.GetID());
            result = r;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

/* ================================================================ */
/*  extract_one_iter                                                 */
/* ================================================================ */

// The recursion only follows 1-edges, so it is a simple loop: collect the
// (x, y) of every node on the 1-path, then apply the rotations bottom-up.
RotPiDD RotPiDD::extract_one_iter(const RotPiDD& root)
{
    std::vector< std::pair<int, int> > rots;
    RotPiDD cur = root;
    while (!(cur.zdd_.GetID() == bddsingle || cur.zdd_.GetID() == bddempty ||
             cur.zdd_.GetID() == bddnull)) {
        rots.push_back(std::make_pair(cur.top_x(), cur.top_y()));
        bddvar top = cur.zdd_.Top();
        cur = RotPiDD(cur.zdd_.OnSet0(top));
    }
    RotPiDD r = cur;
    for (size_t i = rots.size(); i > 0; --i) {
        r = left_rot_iter(r, rots[i - 1].first, rots[i - 1].second);
    }
    return r;
}

/* ================================================================ */
/*  to_perms_iter (to_vector_of_perms)                               */
/* ================================================================ */


void RotPiDD::to_perms_iter(const RotPiDD& root, int dim, int* varmap,
                            std::vector< std::vector<int> >& result)
{
    struct Frame {
        RotPiDD p;
        int x, y;
        RotPiDD p1;
        RotPhase phase;
        explicit Frame(const RotPiDD& p_)
            : p(p_), x(0), y(0), p1(), phase(RotPhase::ENTER) {}
    };

    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            if (fr.p == RotPiDD(0)) { stack.pop_back(); break; }
            if (fr.p == RotPiDD(1)) {
                std::vector<int> perm(dim);
                for (int i = 0; i < dim; i++) perm[i] = varmap[i];
                result.push_back(perm);
                stack.pop_back();
                break;
            }

            fr.x = fr.p.top_x();
            fr.y = fr.p.top_y();

            /* p1 = part with LeftRot(x, y); p0 = part without */
            fr.p1 = cofact_iter(fr.p, fr.x, fr.y);
            RotPiDD p0 = fr.p - left_rot_iter(fr.p1, fr.x, fr.y);

            /* Enumerate p0 (VarMap unchanged) */
            fr.phase = RotPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            /* Apply LeftRot(x, y) to VarMap: rotate VarMap[y-1..x-1] left */
            std::rotate(varmap + fr.y - 1, varmap + fr.y, varmap + fr.x);
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            /* Restore VarMap: rotate back (right rotation) */
            std::rotate(varmap + fr.y - 1, varmap + fr.x - 1, varmap + fr.x);
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
}

/* ================================================================ */
/*  enumerate_iter — permutation vector notation                     */
/* ================================================================ */

void RotPiDD::enumerate_iter(const RotPiDD& root, int dim, int* varmap, bool& flag)
{
    struct Frame {
        RotPiDD p;
        int x, y;
        RotPiDD p1;
        RotPhase phase;
        explicit Frame(const RotPiDD& p_)
            : p(p_), x(0), y(0), p1(), phase(RotPhase::ENTER) {}
    };

    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            if (fr.p == RotPiDD(0)) { stack.pop_back(); break; }
            if (fr.p == RotPiDD(1)) {
                enumerate_print(dim, varmap, flag);
                stack.pop_back();
                break;
            }

            fr.x = fr.p.top_x();
            fr.y = fr.p.top_y();

            /* p1 = part with LeftRot(x, y); p0 = part without */
            fr.p1 = cofact_iter(fr.p, fr.x, fr.y);
            RotPiDD p0 = fr.p - left_rot_iter(fr.p1, fr.x, fr.y);

            /* Enumerate p0 first */
            fr.phase = RotPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            /* Apply LeftRot(x, y) to VarMap: rotate VarMap[y-1..x-1] left */
            std::rotate(varmap + fr.y - 1, varmap + fr.y, varmap + fr.x);
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            /* Restore VarMap */
            std::rotate(varmap + fr.y - 1, varmap + fr.x - 1, varmap + fr.x);
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
}

/* ================================================================ */
/*  enumerate2_iter — rotation notation                              */
/* ================================================================ */

void RotPiDD::enumerate2_iter(const RotPiDD& root, std::vector<int>& levs, bool& flag)
{
    struct Frame {
        RotPiDD p;
        int lev;
        RotPiDD p1;
        RotPhase phase;
        explicit Frame(const RotPiDD& p_)
            : p(p_), lev(0), p1(), phase(RotPhase::ENTER) {}
    };

    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            if (fr.p == RotPiDD(-1) || fr.p == RotPiDD(0)) { stack.pop_back(); break; }
            if (fr.p == RotPiDD(1)) {
                enumerate2_print(levs, flag);
                stack.pop_back();
                break;
            }

            int x = fr.p.top_x();
            int y = fr.p.top_y();
            fr.lev = fr.p.top_lev();

            /* p1 = part with LeftRot(x, y); p0 = part without */
            fr.p1 = cofact_iter(fr.p, x, y);
            RotPiDD p0 = fr.p - left_rot_iter(fr.p1, x, y);

            /* Enumerate p0 first */
            fr.phase = RotPhase::GOT_0;
            Frame child(p0);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            /* Record this level and enumerate p1 */
            levs.push_back(fr.lev);
            fr.phase = RotPhase::GOT_1;
            Frame child(fr.p1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            levs.pop_back();
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
}

/* ================================================================ */
/*  contmax_iter (contradictionMaximization)                         */
/* ================================================================ */

long long int RotPiDD::contmax_iter(
    const RotPiDD& root, unsigned long long int root_used_set,
    std::vector<int>& unused_list, int root_n, contmax_hash& hash,
    const std::vector< std::vector<int> >& w)
{
    struct Frame {
        RotPiDD p;
        unsigned long long int used_set;   // as passed in, then with x..n-1 marked
        unsigned long long int saved_used; // key for the final memo store
        int n;
        int x, y;
        bddvar top;
        long long int left_val;
        long long int right_partial;       // weight sum gathered before the 1-branch
        RotPhase phase;
        Frame(const RotPiDD& p_, unsigned long long int used_, int n_)
            : p(p_), used_set(used_), saved_used(used_), n(n_), x(0), y(0),
              top(0), left_val(0), right_partial(0), phase(RotPhase::ENTER) {}
    };

    long long int result = 0;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(Frame(root, root_used_set, root_n));

    while (!stack.empty()) {
        Frame& fr = stack.back();
        switch (fr.phase) {
        case RotPhase::ENTER: {
            if (fr.n > 63)
                throw std::invalid_argument(
                    "contradictionMaximization: n exceeds 63 (bitmask limit)");
            if (fr.n > static_cast<int>(unused_list.size()))
                throw std::invalid_argument(
                    "contradictionMaximization: n exceeds unused_list size");
            bddp pid = fr.p.zdd_.GetID();
            if (pid == bddempty || pid == bddnull) {
                result = -1000000000LL;
                stack.pop_back();
                break;
            }

            /* Memoization check */
            std::pair<bddp, unsigned long long int> key =
                std::make_pair(pid, fr.used_set);
            contmax_hash::iterator it = hash.find(key);
            if (it != hash.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }

            if (pid == bddsingle) {
                hash[key] = 0;
                result = 0;
                stack.pop_back();
                break;
            }

            fr.x = fr.p.top_x();
            fr.y = fr.p.top_y();
            fr.top = fr.p.zdd_.Top();

            /* Mark elements from x to n-1 as used */
            fr.saved_used = fr.used_set;
            for (int i = fr.x; i < fr.n; i++) {
                fr.used_set |= (1ULL << unused_list[i]);
            }

            /* 0-branch (no rotation) */
            RotPiDD rp0(fr.p.zdd_.OffSet(fr.top));
            fr.phase = RotPhase::GOT_0;
            Frame child(rp0, fr.used_set, fr.x);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_0: {
            fr.left_val = result;

            /* 1-branch (with rotation) */
            RotPiDD rp1(fr.p.zdd_.OnSet0(fr.top));

            /* Apply LeftRot(x, y) to unused_list */
            for (int i = fr.y - 1; i < fr.x - 1; i++) {
                std::swap(unused_list[i], unused_list[i + 1]);
            }

            int removed_element = unused_list[fr.x - 1];
            unsigned long long int used_set = fr.used_set | (1ULL << removed_element);

            long long int right_val = 0;
            for (int i = fr.y - 1; i < fr.x - 1; i++) {
                right_val += w[unused_list[fr.x - 1]][unused_list[i]];
            }
            fr.right_partial = right_val;
            fr.phase = RotPhase::GOT_1;
            Frame child(rp1, used_set, fr.x - 1);
            stack.push_back(child);
            break;
        }
        case RotPhase::GOT_1: {
            long long int right_val = fr.right_partial + result;

            /* Restore unused_list */
            for (int i = fr.x - 1; i > fr.y - 1; i--) {
                std::swap(unused_list[i], unused_list[i - 1]);
            }

            long long int res = std::max(fr.left_val, right_val);
            hash[std::make_pair(fr.p.zdd_.GetID(), fr.saved_used)] = res;
            result = res;
            stack.pop_back();
            break;
        }
        case RotPhase::GOT_2:
            break;  // unused
        }
    }
    return result;
}

} // namespace kyotodd
