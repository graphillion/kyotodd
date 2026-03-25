/**
 * SVG Export Examples for KyotoDD
 *
 * Generates 10 SVG files demonstrating various BDD/ZDD/QDD/UnreducedDD
 * visualization features including Expanded/Raw modes, custom parameters,
 * and variable name maps.
 */
#include "bdd.h"
#include "qdd.h"
#include "unreduced_dd.h"
#include <iostream>
#include <fstream>
#include <string>

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

    std::cout << "Done. 10 SVG files generated." << std::endl;
    return 0;
}
