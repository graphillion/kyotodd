/**
 * SVG Export Examples for KyotoDD
 *
 * Generates 24 SVG files demonstrating various BDD/ZDD/QDD/UnreducedDD/
 * PiDD/SeqBDD/RotPiDD/MTBDD/MTZDD/MVBDD/MVZDD visualization features
 * including Expanded/Raw modes, custom parameters, and variable name maps.
 */
#include "bdd.h"
#include "qdd.h"
#include "unreduced_dd.h"
#include "pidd.h"
#include "seqbdd.h"
#include "rotpidd.h"
#include "mtbdd.h"
#include "mvdd.h"
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <set>
#include <vector>

using namespace kyotodd;

static void save(const std::string& filename, const std::string& svg) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Error: cannot open " << filename << std::endl;
        return;
    }
    ofs << svg;
    std::cout << "  " << filename << " (" << svg.size() << " bytes)" << std::endl;
}

int main() {
    BDD::init(256);

    // Allocate variables
    bddvar v1 = BDD::new_var();  // var 1
    bddvar v2 = BDD::new_var();  // var 2
    bddvar v3 = BDD::new_var();  // var 3
    bddvar v4 = BDD::new_var();  // var 4
    bddvar v5 = BDD::new_var();  // var 5

    std::cout << "Generating SVG examples..." << std::endl;

    // ========================================================
    // Example 1: Simple BDD — single variable (x1)
    // ========================================================
    {
        BDD f = BDD::prime(v1);
        save("example01_bdd_single_var.svg", f.save_svg());
    }

    // ========================================================
    // Example 2: BDD — AND of two variables (x1 & x2)
    // ========================================================
    {
        BDD f = BDD::prime(v1) & BDD::prime(v2);
        save("example02_bdd_and.svg", f.save_svg());
    }

    // ========================================================
    // Example 3: BDD — (x1 & x2) | x3
    // ========================================================
    {
        BDD f = (BDD::prime(v1) & BDD::prime(v2)) | BDD::prime(v3);
        save("example03_bdd_or.svg", f.save_svg());
    }

    // ========================================================
    // Example 4: BDD — XOR of 3 variables (x1 ^ x2 ^ x3)
    // ========================================================
    {
        BDD f = BDD::prime(v1) ^ BDD::prime(v2) ^ BDD::prime(v3);
        save("example04_bdd_xor3.svg", f.save_svg());
    }

    // ========================================================
    // Example 5: BDD — complement with Raw mode
    //   Shows Knuth-style dot/odot complement markers
    // ========================================================
    {
        BDD f = ~(BDD::prime(v1) & BDD::prime(v2));
        SvgParams params;
        params.mode = DrawMode::Raw;
        save("example05_bdd_raw_complement.svg", f.save_svg(params));
    }

    // ========================================================
    // Example 6: BDD — custom variable names
    // ========================================================
    {
        BDD f = (BDD::prime(v1) | BDD::prime(v2)) & BDD::prime(v3);
        SvgParams params;
        params.var_name_map[v1] = "A";
        params.var_name_map[v2] = "B";
        params.var_name_map[v3] = "C";
        save("example06_bdd_var_names.svg", f.save_svg(params));
    }

    // ========================================================
    // Example 7: ZDD — power set of {v1, v2, v3}
    // ========================================================
    {
        ZDD z = ZDD::power_set(v3);
        save("example07_zdd_power_set.svg", z.save_svg());
    }

    // ========================================================
    // Example 8: ZDD — singleton {v2} union singleton {v3}
    // ========================================================
    {
        ZDD z = ZDD::singleton(v2) + ZDD::singleton(v3);
        save("example08_zdd_union.svg", z.save_svg());
    }

    // ========================================================
    // Example 9: QDD — quasi-reduced from BDD (x1 & x2)
    //   Every level is present (no skip nodes)
    // ========================================================
    {
        BDD f = BDD::prime(v1) & BDD::prime(v2) & BDD::prime(v3);
        QDD q = f.to_qdd();
        save("example09_qdd.svg", q.save_svg());
    }

    // ========================================================
    // Example 10: BDD — 5 variables, majority function
    //   maj(x1..x5) = 1 iff 3 or more variables are true
    // ========================================================
    {
        BDD x[6];
        x[1] = BDD::prime(v1);
        x[2] = BDD::prime(v2);
        x[3] = BDD::prime(v3);
        x[4] = BDD::prime(v4);
        x[5] = BDD::prime(v5);

        // Majority: at least 3 out of 5
        BDD maj;
        for (int a = 1; a <= 5; ++a)
            for (int b = a+1; b <= 5; ++b)
                for (int c = b+1; c <= 5; ++c)
                    maj = maj | (x[a] & x[b] & x[c]);

        SvgParams params;
        params.var_name_map[v1] = "x1";
        params.var_name_map[v2] = "x2";
        params.var_name_map[v3] = "x3";
        params.var_name_map[v4] = "x4";
        params.var_name_map[v5] = "x5";
        save("example10_bdd_majority5.svg", maj.save_svg(params));
    }

    // ========================================================
    // Example 11: PiDD — permutations of S3
    //   All 6 permutations of {1, 2, 3}
    // ========================================================
    {
        PiDD_NewVar();  // element 1
        PiDD_NewVar();  // element 2
        PiDD_NewVar();  // element 3

        // Build S3: start from identity, apply all transpositions
        PiDD id(1);
        PiDD s12 = id.Swap(1, 2);
        PiDD s13 = id.Swap(1, 3);
        PiDD s23 = id.Swap(2, 3);
        PiDD s3 = id + s12 + s13 + s23
                     + s12.Swap(1, 3) + s13.Swap(1, 2);
        SvgParams params;
        params.var_name_map = PiDD::svg_var_name_map();
        save("example11_pidd_s3.svg", s3.save_svg(params));
    }

    // ========================================================
    // Example 12: SeqBDD — set of sequences {"ab", "ac", "b"}
    //   Using variables as symbols: v1=a, v2=b, v3=c
    // ========================================================
    {
        // "ab" = push(v1) then push(v2): sequence [v1, v2]
        SeqBDD ab = SeqBDD(1).push(static_cast<int>(v2)).push(static_cast<int>(v1));
        // "ac" = sequence [v1, v3]
        SeqBDD ac = SeqBDD(1).push(static_cast<int>(v3)).push(static_cast<int>(v1));
        // "b" = sequence [v2]
        SeqBDD b = SeqBDD(1).push(static_cast<int>(v2));

        SeqBDD seq = ab + ac + b;

        SvgParams params;
        params.var_name_map[v1] = "a";
        params.var_name_map[v2] = "b";
        params.var_name_map[v3] = "c";
        save("example12_seqbdd_sequences.svg", seq.save_svg(params));
    }

    // ========================================================
    // Example 13: RotPiDD — rotational permutations
    //   Identity + LeftRot(2,1) + LeftRot(3,1)
    // ========================================================
    {
        RotPiDD_NewVar();  // element 1
        RotPiDD_NewVar();  // element 2
        RotPiDD_NewVar();  // element 3

        RotPiDD id(1);
        RotPiDD r21 = id.LeftRot(2, 1);   // cyclic shift (1,2)
        RotPiDD r31 = id.LeftRot(3, 1);   // cyclic shift (1,2,3)
        RotPiDD perms = id + r21 + r31;
        SvgParams params;
        params.var_name_map = RotPiDD::svg_var_name_map();
        save("example13_rotpidd_rotations.svg", perms.save_svg(params));
    }

    // ========================================================
    // PiDD examples with 10+ nodes (examples 14-18)
    // Extend to S5: need elements 4 and 5
    // ========================================================
    PiDD_NewVar();  // element 4
    PiDD_NewVar();  // element 5

    // Helper: generate S_n from identity by repeated transposition closure.
    // Explicit -> PiDD return type for strict C++11 conformance (return-type
    // deduction for multi-statement lambdas is a C++14 feature).
    auto make_sn = [](int n) -> PiDD {
        PiDD id(1);
        PiDD all = id;
        for (int round = 0; round < 10; ++round) {
            PiDD prev = all;
            for (int x = 2; x <= n; ++x) {
                all = all + prev.Swap(1, x);
            }
        }
        return all;
    };

    SvgParams pidd_params;
    pidd_params.var_name_map = PiDD::svg_var_name_map();

    // ========================================================
    // Example 14: PiDD — S5 (all 120 permutations of {1,..,5})
    // ========================================================
    {
        PiDD s5 = make_sn(5);

        save("example14_pidd_s5.svg", s5.save_svg(pidd_params));
    }

    // ========================================================
    // Example 15: PiDD — A5 (even permutations of S5, 60 perms)
    // ========================================================
    {
        PiDD s5 = make_sn(5);
        PiDD a5 = s5.Even();

        save("example15_pidd_a5.svg", a5.save_svg(pidd_params));
    }

    // ========================================================
    // Example 16: PiDD — odd permutations of S5 (60 perms)
    // ========================================================
    {
        PiDD s5 = make_sn(5);
        PiDD odd5 = s5.Odd();

        save("example16_pidd_odd_s5.svg", odd5.save_svg(pidd_params));
    }

    // ========================================================
    // Example 17: PiDD — S5 with SwapBound(3)
    //   Permutations reachable with at most 3 transpositions
    // ========================================================
    {
        PiDD s5 = make_sn(5);
        PiDD bounded = s5.SwapBound(3);

        save("example17_pidd_swapbound3.svg", bounded.save_svg(pidd_params));
    }

    // ========================================================
    // Example 18: PiDD — S5 with SwapBound(2)
    //   Permutations of {1,..,5} reachable with at most 2
    //   transpositions (id + single transpositions + double)
    // ========================================================
    {
        PiDD s5 = make_sn(5);
        PiDD bounded = s5.SwapBound(2);

        save("example18_pidd_s5_swapbound2.svg", bounded.save_svg(pidd_params));
    }

    // ========================================================
    // Example 19: PiDD — 100 random permutations of {1,..,5}
    //   Each generated by 5-10 random transpositions
    // ========================================================
    {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> swap_count_dist(5, 10);
        std::uniform_int_distribution<int> elem_dist(1, 5);

        PiDD result(0);  // empty set
        for (int i = 0; i < 100; ++i) {
            PiDD perm(1);  // identity
            int nswaps = swap_count_dist(rng);
            for (int s = 0; s < nswaps; ++s) {
                int a = elem_dist(rng);
                int b = elem_dist(rng);
                while (b == a) b = elem_dist(rng);
                perm = perm.Swap(a, b);
            }
            result = result + perm;
        }
        SvgParams params = pidd_params;
        params.skip_unused_levels = true;
        save("example19_pidd_random100.svg", result.save_svg(params));
    }

    // ========================================================
    // Example 20: MTBDD — temperature function
    //   f(x1,x2) = 40 if x2, else (25 if x1, else 10)
    // ========================================================
    {
        auto cold = MTBDD<double>::terminal(10.0);
        auto warm = MTBDD<double>::terminal(25.0);
        auto hot  = MTBDD<double>::terminal(40.0);
        auto inner = MTBDD<double>::ite(v1, warm, cold);
        auto mt = MTBDD<double>::ite(v2, hot, inner);
        SvgParams params;
        params.var_name_map[v1] = "rain";
        params.var_name_map[v2] = "sun";
        save("example20_mtbdd_temperature.svg", mt.save_svg(params));
    }

    // ========================================================
    // Example 21: MTZDD — integer weights
    //   Three terminals: 0, 1, 5
    // ========================================================
    {
        auto t0 = MTZDD<int64_t>::terminal(0);
        auto t1 = MTZDD<int64_t>::terminal(1);
        auto t5 = MTZDD<int64_t>::terminal(5);
        auto inner = MTZDD<int64_t>::ite(v1, t5, t1);
        auto mt = MTZDD<int64_t>::ite(v2, inner, t0);
        SvgParams params;
        params.var_name_map[v1] = "x";
        params.var_name_map[v2] = "y";
        save("example21_mtzdd_weights.svg", mt.save_svg(params));
    }

    // ========================================================
    // Example 22: MVBDD — k=3, two variables
    //   (mv1 == 1) OR (mv2 == 2)
    // ========================================================
    {
        MVBDD base(3);
        bddvar mv1 = base.new_var();
        bddvar mv2 = base.new_var();
        auto lit1 = MVBDD::singleton(base, mv1, 1);
        auto lit2 = MVBDD::singleton(base, mv2, 2);
        auto f = lit1 | lit2;
        SvgParams params;
        params.var_name_map[mv1] = "color";
        params.var_name_map[mv2] = "shape";
        params.draw_edge_labels = true;
        save("example22_mvbdd_k3.svg", f.save_svg(params));
    }

    // ========================================================
    // Example 23: MVZDD — k=3, singleton family
    //   Family containing only {mv1=1}
    // ========================================================
    {
        MVZDD base(3);
        bddvar mv1 = base.new_var();
        auto s = MVZDD::singleton(base, mv1, 1);
        SvgParams params;
        params.var_name_map[mv1] = "x";
        params.draw_edge_labels = true;
        save("example23_mvzdd_k3.svg", s.save_svg(params));
    }

    // ========================================================
    // Example 24: MVBDD — k=4, one variable with edge labels
    // ========================================================
    {
        MVBDD base(4);
        bddvar mv1 = base.new_var();
        auto lit = MVBDD::singleton(base, mv1, 3);
        SvgParams params;
        params.var_name_map[mv1] = "dir";
        params.draw_edge_labels = true;
        save("example24_mvbdd_k4.svg", lit.save_svg(params));
    }

    std::cout << "Done. 24 SVG files generated." << std::endl;
    return 0;
}
