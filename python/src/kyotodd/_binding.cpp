#include <pybind11/pybind11.h>
#include "bdd.h"

namespace py = pybind11;

static bool g_initialized = false;

PYBIND11_MODULE(_core, m) {
    m.doc() = "KyotoDD BDD/ZDD library";

    m.def("init", [](uint64_t node_count, uint64_t node_max) {
        bddinit(node_count, node_max);
        g_initialized = true;
    }, py::arg("node_count") = 256, py::arg("node_max") = UINT64_MAX,
       "Initialize the BDD library.");
}
