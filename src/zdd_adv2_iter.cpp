#include "bdd.h"
#include "bdd_internal.h"
#include <cstdint>
#include <vector>

namespace kyotodd {

namespace {

// Same helper as the static support_to_singletons in zdd_adv2.cpp.
//
// PRECONDITION: Must be invoked under a bdd_gc_guard scope. The intermediate
// `result` accumulator holds an unprotected bddp across bddunion / bddchange
// calls, so GC must be deferred. All callers (bddsymset_iter in this file)
// already satisfy this precondition.
bddp support_to_singletons_iter(bddp f) {
    std::vector<bddvar> vars = bddsupport_vec(f);
    bddp result = bddempty;
    for (bddvar v : vars) {
        result = bddunion(result, bddchange(bddsingle, v));
    }
    return result;
}

} // namespace

// =====================================================================
// bddpermitsym_iter — explicit-stack rewrite of bddpermitsym_rec.
//
// The recursion in zdd_adv2.cpp:
//   - terminal: f == empty → empty; n < 0 → empty; f == single → single
//   - n == 0 → bddintersec(f, bddsingle)
//   - else: r1 = bddchange(rec(f_hi, n-1), top)
//           r0 = rec(f_lo, n)
//           result = bddunion(r1, r0)
//
// PRECONDITION: Must be invoked under a bdd_gc_guard scope. Intermediate bddp
// values on the iteration stack are not registered as GC roots, so GC must be
// deferred (bdd_gc_depth > 0) for the duration of this call. The public
// wrapper bddpermitsym() satisfies this precondition.
// =====================================================================
bddp bddpermitsym_iter(bddp f, int n) {
    enum class Phase : uint8_t { ENTER, GOT_R1, GOT_R0 };
    struct Frame {
        bddp f;          // original (cache key)
        int n;
        bddvar top_var;
        bddp f_lo;       // for second recursion
        bddp f_hi;       // for first recursion
        bddp r1;         // saved bddchange(rec(f_hi, n-1), top)
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.n = n;
    init.top_var = 0;
    init.f_lo = 0; init.f_hi = 0;
    init.r1 = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            int cn = frame.n;
            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cn < 0) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddsingle; stack.pop_back(); break; }
            if (cn == 0) {
                result = bddintersec(cf, bddsingle);
                stack.pop_back();
                break;
            }
            if (cf & BDD_CONST_FLAG) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_PERMITSYM, cf,
                                    static_cast<bddp>(cn));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;
            bddvar top = node_var(f_raw);
            bddp f_lo = node_lo(f_raw);
            bddp f_hi = node_hi(f_raw);
            if (comp) f_lo = bddnot(f_lo);

            frame.top_var = top;
            frame.f_lo = f_lo;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_R1;

            Frame child;
            child.f = f_hi; child.n = cn - 1;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.r1 = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_R1: {
            frame.r1 = bddchange(result, frame.top_var);
            frame.phase = Phase::GOT_R0;
            Frame child;
            child.f = frame.f_lo; child.n = frame.n;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.r1 = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_R0: {
            bddp combined = bddunion(frame.r1, result);
            bddwcache(BDD_OP_PERMITSYM, frame.f,
                      static_cast<bddp>(frame.n), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddalways_iter — explicit-stack rewrite of bddalways_rec.
//
// The recursion:
//   - terminal: f is constant → empty
//   - h = rec(f_hi)
//   - if f_lo == empty:   result = bddunion(h, bddchange(bddsingle, top))
//   - elif h == empty:    result = bddempty
//   - else:               result = bddintersec(h, rec(f_lo))
//
// PRECONDITION: See bddpermitsym_iter.
// =====================================================================
bddp bddalways_iter(bddp f) {
    enum class Phase : uint8_t { ENTER, GOT_H, GOT_F0 };
    struct Frame {
        bddp f;          // cache key
        bddvar top_var;
        bddp f_lo;       // for second recursion (f0)
        bddp f_hi;       // for first recursion (f1)
        bddp h;          // saved rec(f_hi) result
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0;
    init.f_lo = 0; init.f_hi = 0;
    init.h = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf & BDD_CONST_FLAG) {
                result = bddempty;
                stack.pop_back();
                break;
            }

            bddp cached = bddrcache(BDD_OP_ALWAYS, cf, 0);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;
            bddvar top = node_var(f_raw);
            bddp f_lo = node_lo(f_raw);
            bddp f_hi = node_hi(f_raw);
            if (comp) f_lo = bddnot(f_lo);

            frame.top_var = top;
            frame.f_lo = f_lo;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_H;

            Frame child;
            child.f = f_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.h = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_H: {
            bddp h = result;
            if (frame.f_lo == bddempty) {
                bddp combined = bddunion(
                    h, bddchange(bddsingle, frame.top_var));
                bddwcache(BDD_OP_ALWAYS, frame.f, 0, combined);
                result = combined;
                stack.pop_back();
                break;
            }
            if (h == bddempty) {
                bddwcache(BDD_OP_ALWAYS, frame.f, 0, bddempty);
                result = bddempty;
                stack.pop_back();
                break;
            }
            frame.h = h;
            frame.phase = Phase::GOT_F0;
            Frame child;
            child.f = frame.f_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.h = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_F0: {
            bddp combined = bddintersec(frame.h, result);
            bddwcache(BDD_OP_ALWAYS, frame.f, 0, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddsymchk_iter — explicit-stack rewrite of bddsymchk_rec (int return).
//
// The recursion has two main shapes that both reduce to AND of two
// sub-recursions (with short-circuit on 0):
//   - branch A (top above v1): rec(f_t1, v1, v2) && rec(f_t0, v1, v2)
//   - branch B (top at/below v1):
//       direct comparison case: no sub-recursion
//       intermediate-var case: rec(ft2_yes, v1, v2) && rec(ft2_no, v1, v2)
//
// PRECONDITION: See bddpermitsym_iter. Caller must have normalized
// (v1, v2) so that var2level[v1] >= var2level[v2] (the public wrapper
// bddsymchk does this).
// =====================================================================
int bddsymchk_iter(bddp f, bddvar v1, bddvar v2) {
    enum class Phase : uint8_t { ENTER, GOT_FIRST, GOT_SECOND };
    struct Frame {
        bddp f;          // cache key
        bddvar v1, v2;
        bddp g2;         // saved second AND child
        Phase phase;
    };

    int result = 0;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.v1 = v1; init.v2 = v2;
    init.g2 = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddvar cv1 = frame.v1;
            bddvar cv2 = frame.v2;

            if (cf & BDD_CONST_FLAG) {
                result = 1;
                stack.pop_back();
                break;
            }

            bddp cached = bddrcache3(BDD_OP_SYMCHK, cf,
                                     static_cast<bddp>(cv1),
                                     static_cast<bddp>(cv2));
            if (cached != bddnull) {
                result = static_cast<int>(cached);
                stack.pop_back();
                break;
            }

            bddvar t = bddtop(cf);
            bddvar t_level = var2level[t];
            bddvar v1_level = var2level[cv1];

            bddp child1, child2;

            if (t_level > v1_level) {
                // Branch A: decompose at top, recurse on both branches.
                child1 = bddonset0(cf, t);
                child2 = bddoffset(cf, t);
            } else {
                // Branch B: decompose at v1.
                bddp f0 = bddoffset(cf, cv1);
                bddp f1 = bddonset0(cf, cv1);

                bddvar f0_top = bddtop(f0);
                bddvar f1_top = bddtop(f1);
                bddvar f0_level = (f0_top == 0) ? 0 : var2level[f0_top];
                bddvar f1_level = (f1_top == 0) ? 0 : var2level[f1_top];
                bddvar v2_level = var2level[cv2];

                if (f0_level <= v2_level && f1_level <= v2_level) {
                    int direct = (bddonset0(f0, cv2) == bddoffset(f1, cv2))
                                     ? 1 : 0;
                    bddwcache3(BDD_OP_SYMCHK, cf,
                               static_cast<bddp>(cv1),
                               static_cast<bddp>(cv2),
                               static_cast<bddp>(direct));
                    result = direct;
                    stack.pop_back();
                    break;
                }

                bddvar t2 = (f0_level > f1_level) ? f0_top : f1_top;
                bddp f00 = bddoffset(f0, t2);
                bddp f01 = bddonset0(f0, t2);
                bddp f10 = bddoffset(f1, t2);
                bddp f11 = bddonset0(f1, t2);
                child1 = ZDD::getnode_raw(cv1, f01, f11);
                child2 = ZDD::getnode_raw(cv1, f00, f10);
            }

            frame.g2 = child2;
            frame.phase = Phase::GOT_FIRST;

            Frame child;
            child.f = child1; child.v1 = cv1; child.v2 = cv2;
            child.g2 = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_FIRST: {
            if (result == 0) {
                bddwcache3(BDD_OP_SYMCHK, frame.f,
                           static_cast<bddp>(frame.v1),
                           static_cast<bddp>(frame.v2),
                           static_cast<bddp>(0));
                stack.pop_back();
                break;
            }
            frame.phase = Phase::GOT_SECOND;
            Frame child;
            child.f = frame.g2; child.v1 = frame.v1; child.v2 = frame.v2;
            child.g2 = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_SECOND: {
            // result already 0 or 1 from the second sub-call.
            bddwcache3(BDD_OP_SYMCHK, frame.f,
                       static_cast<bddp>(frame.v1),
                       static_cast<bddp>(frame.v2),
                       static_cast<bddp>(result));
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddsymset_iter — explicit-stack rewrite of bddsymset_rec.
//
// The recursion takes (f0, f1) and decomposes both at the higher-level
// top variable t:
//   case A (f11 == empty):
//     base = rec(f00, f10);  result = base - support_to_singletons(f01)
//   case B (f10 == empty):
//     base = rec(f01, f11);  result = base - support_to_singletons(f00)
//   case C (both nonempty):
//     result = bddintersec(rec(f00, f10), rec(f01, f11))
//   then if (f10 == f01): result = bddunion(result, bddchange(bddsingle, t))
//
// PRECONDITION: See bddpermitsym_iter. The public wrapper bddsymset()
// supplies (f0, f1) computed from (f, v).
// =====================================================================
bddp bddsymset_iter(bddp f0_arg, bddp f1_arg) {
    enum class Phase : uint8_t { ENTER, GOT_FIRST, GOT_SECOND };
    enum class SubCase : uint8_t { A, B, C };
    struct Frame {
        bddp f0;             // cache key (paired with f1)
        bddp f1;
        bddvar top_var;
        bddp f00, f01, f10, f11;
        SubCase subcase;
        bddp first_result;   // saved first sub-recursion result (case C)
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f0 = f0_arg; init.f1 = f1_arg;
    init.top_var = 0;
    init.f00 = 0; init.f01 = 0; init.f10 = 0; init.f11 = 0;
    init.subcase = SubCase::C;
    init.first_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf0 = frame.f0;
            bddp cf1 = frame.f1;

            if (cf1 == bddempty) {
                result = bddempty;
                stack.pop_back();
                break;
            }
            if (cf1 == bddsingle && (cf0 & BDD_CONST_FLAG)) {
                result = bddempty;
                stack.pop_back();
                break;
            }

            bddp cached = bddrcache(BDD_OP_SYMSET, cf0, cf1);
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            bddvar f0_top = bddtop(cf0);
            bddvar f1_top = bddtop(cf1);
            bddvar f0_level = (f0_top == 0) ? 0 : var2level[f0_top];
            bddvar f1_level = (f1_top == 0) ? 0 : var2level[f1_top];
            if (f0_level == 0 && f1_level == 0) {
                bddwcache(BDD_OP_SYMSET, cf0, cf1, bddempty);
                result = bddempty;
                stack.pop_back();
                break;
            }
            bddvar t = (f0_level > f1_level) ? f0_top : f1_top;

            bddp f00 = bddoffset(cf0, t);
            bddp f01 = bddonset0(cf0, t);
            bddp f10 = bddoffset(cf1, t);
            bddp f11 = bddonset0(cf1, t);

            frame.top_var = t;
            frame.f00 = f00; frame.f01 = f01;
            frame.f10 = f10; frame.f11 = f11;

            if (f11 == bddempty) {
                frame.subcase = SubCase::A;
                frame.phase = Phase::GOT_FIRST;
                Frame child;
                child.f0 = f00; child.f1 = f10;
                child.top_var = 0;
                child.f00 = 0; child.f01 = 0; child.f10 = 0; child.f11 = 0;
                child.subcase = SubCase::C;
                child.first_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else if (f10 == bddempty) {
                frame.subcase = SubCase::B;
                frame.phase = Phase::GOT_FIRST;
                Frame child;
                child.f0 = f01; child.f1 = f11;
                child.top_var = 0;
                child.f00 = 0; child.f01 = 0; child.f10 = 0; child.f11 = 0;
                child.subcase = SubCase::C;
                child.first_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                frame.subcase = SubCase::C;
                frame.phase = Phase::GOT_FIRST;
                Frame child;
                child.f0 = f00; child.f1 = f10;
                child.top_var = 0;
                child.f00 = 0; child.f01 = 0; child.f10 = 0; child.f11 = 0;
                child.subcase = SubCase::C;
                child.first_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_FIRST: {
            if (frame.subcase == SubCase::A) {
                bddp base = result;
                bddp combined = bddsubtract(
                    base, support_to_singletons_iter(frame.f01));
                if (frame.f10 == frame.f01) {
                    combined = bddunion(
                        combined, bddchange(bddsingle, frame.top_var));
                }
                bddwcache(BDD_OP_SYMSET, frame.f0, frame.f1, combined);
                result = combined;
                stack.pop_back();
                break;
            }
            if (frame.subcase == SubCase::B) {
                bddp base = result;
                bddp combined = bddsubtract(
                    base, support_to_singletons_iter(frame.f00));
                if (frame.f10 == frame.f01) {
                    combined = bddunion(
                        combined, bddchange(bddsingle, frame.top_var));
                }
                bddwcache(BDD_OP_SYMSET, frame.f0, frame.f1, combined);
                result = combined;
                stack.pop_back();
                break;
            }
            // SubCase::C — push second sub-recursion (f01, f11).
            frame.first_result = result;
            frame.phase = Phase::GOT_SECOND;
            Frame child;
            child.f0 = frame.f01; child.f1 = frame.f11;
            child.top_var = 0;
            child.f00 = 0; child.f01 = 0; child.f10 = 0; child.f11 = 0;
            child.subcase = SubCase::C;
            child.first_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_SECOND: {
            bddp combined = bddintersec(frame.first_result, result);
            if (frame.f10 == frame.f01) {
                combined = bddunion(
                    combined, bddchange(bddsingle, frame.top_var));
            }
            bddwcache(BDD_OP_SYMSET, frame.f0, frame.f1, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddcoimplyset_iter — explicit-stack rewrite of bddcoimplyset_rec.
//
// Same shape as bddsymset_iter but:
//   case A: result = rec(f00, f10)     (no support subtract)
//   case B: result = rec(f01, f11)
//   case C: result = bddintersec(rec(f00, f10), rec(f01, f11))
//   then if (bddsubtract(f10, f01) == empty):
//        result = bddunion(result, bddchange(bddsingle, t))
//
// PRECONDITION: See bddpermitsym_iter.
// =====================================================================
bddp bddcoimplyset_iter(bddp f0_arg, bddp f1_arg) {
    enum class Phase : uint8_t { ENTER, GOT_FIRST, GOT_SECOND };
    enum class SubCase : uint8_t { A, B, C };
    struct Frame {
        bddp f0;
        bddp f1;
        bddvar top_var;
        bddp f00, f01, f10, f11;
        SubCase subcase;
        bddp first_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f0 = f0_arg; init.f1 = f1_arg;
    init.top_var = 0;
    init.f00 = 0; init.f01 = 0; init.f10 = 0; init.f11 = 0;
    init.subcase = SubCase::C;
    init.first_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf0 = frame.f0;
            bddp cf1 = frame.f1;

            if (cf1 == bddempty) {
                result = bddempty;
                stack.pop_back();
                break;
            }
            if (cf1 == bddsingle && (cf0 & BDD_CONST_FLAG)) {
                result = bddempty;
                stack.pop_back();
                break;
            }

            bddp cached = bddrcache(BDD_OP_COIMPLYSET, cf0, cf1);
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            bddvar f0_top = bddtop(cf0);
            bddvar f1_top = bddtop(cf1);
            bddvar f0_level = (f0_top == 0) ? 0 : var2level[f0_top];
            bddvar f1_level = (f1_top == 0) ? 0 : var2level[f1_top];
            if (f0_level == 0 && f1_level == 0) {
                bddwcache(BDD_OP_COIMPLYSET, cf0, cf1, bddempty);
                result = bddempty;
                stack.pop_back();
                break;
            }
            bddvar t = (f0_level > f1_level) ? f0_top : f1_top;

            bddp f00 = bddoffset(cf0, t);
            bddp f01 = bddonset0(cf0, t);
            bddp f10 = bddoffset(cf1, t);
            bddp f11 = bddonset0(cf1, t);

            frame.top_var = t;
            frame.f00 = f00; frame.f01 = f01;
            frame.f10 = f10; frame.f11 = f11;

            if (f11 == bddempty) {
                frame.subcase = SubCase::A;
                frame.phase = Phase::GOT_FIRST;
                Frame child;
                child.f0 = f00; child.f1 = f10;
                child.top_var = 0;
                child.f00 = 0; child.f01 = 0; child.f10 = 0; child.f11 = 0;
                child.subcase = SubCase::C;
                child.first_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else if (f10 == bddempty) {
                frame.subcase = SubCase::B;
                frame.phase = Phase::GOT_FIRST;
                Frame child;
                child.f0 = f01; child.f1 = f11;
                child.top_var = 0;
                child.f00 = 0; child.f01 = 0; child.f10 = 0; child.f11 = 0;
                child.subcase = SubCase::C;
                child.first_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                frame.subcase = SubCase::C;
                frame.phase = Phase::GOT_FIRST;
                Frame child;
                child.f0 = f00; child.f1 = f10;
                child.top_var = 0;
                child.f00 = 0; child.f01 = 0; child.f10 = 0; child.f11 = 0;
                child.subcase = SubCase::C;
                child.first_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_FIRST: {
            if (frame.subcase != SubCase::C) {
                bddp combined = result;
                if (bddsubtract(frame.f10, frame.f01) == bddempty) {
                    combined = bddunion(
                        combined, bddchange(bddsingle, frame.top_var));
                }
                bddwcache(BDD_OP_COIMPLYSET, frame.f0, frame.f1, combined);
                result = combined;
                stack.pop_back();
                break;
            }
            // SubCase::C
            frame.first_result = result;
            frame.phase = Phase::GOT_SECOND;
            Frame child;
            child.f0 = frame.f01; child.f1 = frame.f11;
            child.top_var = 0;
            child.f00 = 0; child.f01 = 0; child.f10 = 0; child.f11 = 0;
            child.subcase = SubCase::C;
            child.first_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_SECOND: {
            bddp combined = bddintersec(frame.first_result, result);
            if (bddsubtract(frame.f10, frame.f01) == bddempty) {
                combined = bddunion(
                    combined, bddchange(bddsingle, frame.top_var));
            }
            bddwcache(BDD_OP_COIMPLYSET, frame.f0, frame.f1, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
