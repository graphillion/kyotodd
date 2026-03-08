#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "bdd.h"

namespace py = pybind11;

static bool g_initialized = false;

static void ensure_init() {
    if (!g_initialized) {
        bddinit(256, UINT64_MAX);
        g_initialized = true;
    }
}

PYBIND11_MODULE(_core, m) {
    m.doc() = "KyotoDD BDD/ZDD library";

    // Initialization
    m.def("init", [](uint64_t node_count, uint64_t node_max) {
        bddinit(node_count, node_max);
        g_initialized = true;
    }, py::arg("node_count") = 256, py::arg("node_max") = UINT64_MAX,
       "Initialize the BDD library.");

    // Variable management
    m.def("newvar", []() -> bddvar {
        ensure_init();
        return bddnewvar();
    }, "Create a new variable and return its variable number.");

    m.def("newvar_of_level", [](bddvar lev) -> bddvar {
        ensure_init();
        return bddnewvaroflev(lev);
    }, py::arg("lev"),
       "Create a new variable at the specified level.");

    m.def("var_count", &bddvarused,
       "Return the number of variables created.");

    m.def("level_of_var", &bddlevofvar, py::arg("var"),
       "Return the level of the given variable.");

    m.def("var_of_level", &bddvaroflev, py::arg("level"),
       "Return the variable at the given level.");

    // Node stats
    m.def("node_count", &bddused,
       "Return the number of nodes in use.");

    // GC API
    m.def("gc", &bddgc,
       "Manually invoke garbage collection.");

    m.def("live_nodes", &bddlive,
       "Return the number of live nodes.");

    m.def("gc_set_threshold", &bddgc_setthreshold, py::arg("threshold"),
       "Set the GC threshold (0.0 to 1.0).");

    m.def("gc_get_threshold", &bddgc_getthreshold,
       "Get the current GC threshold.");
}
