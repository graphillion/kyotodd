#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <sstream>
#include <fstream>
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

    // Exception translation
    py::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p) std::rethrow_exception(p);
        } catch (const std::overflow_error& e) {
            PyErr_SetString(PyExc_MemoryError, e.what());
        } catch (const std::invalid_argument& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
        } catch (const std::out_of_range& e) {
            PyErr_SetString(PyExc_IndexError, e.what());
        }
    });

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

    // BDD class
    py::class_<BDD>(m, "BDD")
        .def(py::init([](int val) {
            ensure_init();
            return BDD(val);
        }), py::arg("val") = 0)

        .def_static("var", [](bddvar v) -> BDD {
            ensure_init();
            return BDDvar(v);
        }, py::arg("v"),
           "Create a BDD representing the given variable.")

        .def_property_readonly_static("false_", [](py::object) -> BDD {
            return BDD::False;
        })
        .def_property_readonly_static("true_", [](py::object) -> BDD {
            return BDD::True;
        })
        .def_property_readonly_static("null", [](py::object) -> BDD {
            return BDD::Null;
        })

        .def("__eq__", [](const BDD& a, const BDD& b) { return a == b; })
        .def("__ne__", [](const BDD& a, const BDD& b) { return a != b; })
        .def("__hash__", [](const BDD& a) {
            return std::hash<uint64_t>()(a.root);
        })
        .def("__repr__", [](const BDD& a) {
            return "BDD(node_id=" + std::to_string(a.root) + ")";
        })
        .def("__bool__", [](const BDD&) -> bool {
            throw py::type_error(
                "BDD cannot be converted to bool. "
                "Use == BDD.false_ or == BDD.true_ instead.");
        })

        // Operators
        .def("__and__",    [](const BDD& a, const BDD& b) { return a & b; })
        .def("__or__",     [](const BDD& a, const BDD& b) { return a | b; })
        .def("__xor__",    [](const BDD& a, const BDD& b) { return a ^ b; })
        .def("__invert__", [](const BDD& a) { return ~a; })
        .def("__lshift__", [](const BDD& a, bddvar s) { return a << s; })
        .def("__rshift__", [](const BDD& a, bddvar s) { return a >> s; })
        .def("__iand__",   [](BDD& a, const BDD& b) -> BDD& { a &= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__ior__",    [](BDD& a, const BDD& b) -> BDD& { a |= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__ixor__",   [](BDD& a, const BDD& b) -> BDD& { a ^= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__ilshift__",[](BDD& a, bddvar s) -> BDD& { a <<= s; return a; },
             py::return_value_policy::reference_internal)
        .def("__irshift__",[](BDD& a, bddvar s) -> BDD& { a >>= s; return a; },
             py::return_value_policy::reference_internal)

        // Methods
        .def("at0", &BDD::At0, py::arg("v"),
             "Cofactor: restrict variable v to 0.")
        .def("at1", &BDD::At1, py::arg("v"),
             "Cofactor: restrict variable v to 1.")
        .def("cofactor", &BDD::Cofactor, py::arg("g"),
             "Generalized cofactor by BDD g.")
        .def("support", &BDD::Support,
             "Return the support set as a BDD (conjunction of variables).")
        .def("support_vec", &BDD::SupportVec,
             "Return the support set as a list of variable numbers.")
        .def("imply", &BDD::Imply, py::arg("other"),
             "Check implication: return 1 if self => other, 0 otherwise.")

        // Quantification (lambdas to avoid overload_cast requiring C++14)
        .def("exist", [](const BDD& f, const BDD& cube) { return f.Exist(cube); },
             py::arg("cube"), "Existential quantification by cube BDD.")
        .def("exist", [](const BDD& f, const std::vector<bddvar>& vars) { return f.Exist(vars); },
             py::arg("vars"), "Existential quantification by variable list.")
        .def("exist", [](const BDD& f, bddvar var) { return f.Exist(var); },
             py::arg("var"), "Existential quantification of a single variable.")
        .def("univ", [](const BDD& f, const BDD& cube) { return f.Univ(cube); },
             py::arg("cube"), "Universal quantification by cube BDD.")
        .def("univ", [](const BDD& f, const std::vector<bddvar>& vars) { return f.Univ(vars); },
             py::arg("vars"), "Universal quantification by variable list.")
        .def("univ", [](const BDD& f, bddvar var) { return f.Univ(var); },
             py::arg("var"), "Universal quantification of a single variable.")

        // Static methods
        .def_static("ite", &BDD::Ite, py::arg("f"), py::arg("g"), py::arg("h"),
             "If-then-else: returns (f & g) | (~f & h).")

        // I/O
        .def("export_str", [](const BDD& b) -> std::string {
            std::ostringstream oss;
            bddp p = b.root;
            bddexport(oss, &p, 1);
            return oss.str();
        }, "Export BDD to a string.")
        .def_static("import_str", [](const std::string& s) -> BDD {
            ensure_init();
            std::istringstream iss(s);
            bddp p;
            int ret = bddimport(iss, &p, 1);
            if (ret < 1) throw std::runtime_error("BDD import failed");
            return BDD_ID(p);
        }, py::arg("s"), "Import a BDD from a string.")
        .def("export_file", [](const BDD& b, const std::string& path) {
            std::ofstream ofs(path);
            if (!ofs) throw std::runtime_error("Cannot open file: " + path);
            bddp p = b.root;
            bddexport(ofs, &p, 1);
        }, py::arg("path"), "Export BDD to a file.")
        .def_static("import_file", [](const std::string& path) -> BDD {
            ensure_init();
            std::ifstream ifs(path);
            if (!ifs) throw std::runtime_error("Cannot open file: " + path);
            bddp p;
            int ret = bddimport(ifs, &p, 1);
            if (ret < 1) throw std::runtime_error("BDD import failed");
            return BDD_ID(p);
        }, py::arg("path"), "Import a BDD from a file.")

        .def_property_readonly("node_id", [](const BDD& b) { return b.root; })
        .def_property_readonly("size", &BDD::Size)
        .def_property_readonly("top_var", [](const BDD& b) -> bddvar {
            return bddtop(b.root);
        })
    ;

    // ZDD class
    py::class_<ZDD>(m, "ZDD")
        .def(py::init([](int val) {
            ensure_init();
            return ZDD(val);
        }), py::arg("val") = 0)

        .def_property_readonly_static("empty", [](py::object) -> ZDD {
            return ZDD::Empty;
        })
        .def_property_readonly_static("single", [](py::object) -> ZDD {
            return ZDD::Single;
        })
        .def_property_readonly_static("null", [](py::object) -> ZDD {
            return ZDD::Null;
        })

        .def("__eq__", [](const ZDD& a, const ZDD& b) { return a == b; })
        .def("__ne__", [](const ZDD& a, const ZDD& b) { return a != b; })
        .def("__hash__", [](const ZDD& a) {
            return std::hash<uint64_t>()(a.root);
        })
        .def("__repr__", [](const ZDD& a) {
            return "ZDD(node_id=" + std::to_string(a.root) + ")";
        })

        // Operators
        .def("__add__",      [](const ZDD& a, const ZDD& b) { return a + b; })
        .def("__sub__",      [](const ZDD& a, const ZDD& b) { return a - b; })
        .def("__and__",      [](const ZDD& a, const ZDD& b) { return a & b; })
        .def("__xor__",      [](const ZDD& a, const ZDD& b) { return a ^ b; })
        .def("__mul__",      [](const ZDD& a, const ZDD& b) { return a * b; })
        .def("__truediv__",  [](const ZDD& a, const ZDD& b) { return a / b; })
        .def("__mod__",      [](const ZDD& a, const ZDD& b) { return a % b; })
        .def("__iadd__",     [](ZDD& a, const ZDD& b) -> ZDD& { a += b; return a; },
             py::return_value_policy::reference_internal)
        .def("__isub__",     [](ZDD& a, const ZDD& b) -> ZDD& { a -= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__iand__",     [](ZDD& a, const ZDD& b) -> ZDD& { a &= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__ixor__",     [](ZDD& a, const ZDD& b) -> ZDD& { a ^= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__imul__",     [](ZDD& a, const ZDD& b) -> ZDD& { a *= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__itruediv__", [](ZDD& a, const ZDD& b) -> ZDD& { a /= b; return a; },
             py::return_value_policy::reference_internal)
        .def("__imod__",     [](ZDD& a, const ZDD& b) -> ZDD& { a %= b; return a; },
             py::return_value_policy::reference_internal)

        // Methods
        .def("change", &ZDD::Change, py::arg("v"),
             "Toggle membership of variable v.")
        .def("offset", &ZDD::Offset, py::arg("v"),
             "Remove variable v from all sets.")
        .def("onset", &ZDD::OnSet, py::arg("v"),
             "Select sets containing variable v, then remove v.")
        .def("onset0", &ZDD::OnSet0, py::arg("v"),
             "Select sets not containing variable v.")
        .def("maximal", &ZDD::Maximal,
             "Return maximal sets (no proper superset in the family).")
        .def("minimal", &ZDD::Minimal,
             "Return minimal sets (no proper subset in the family).")
        .def("minhit", &ZDD::Minhit,
             "Return minimum hitting sets.")
        .def("closure", &ZDD::Closure,
             "Return the downward closure.")
        .def("restrict", &ZDD::Restrict, py::arg("g"),
             "Restrict to sets that are subsets of some set in g.")
        .def("permit", &ZDD::Permit, py::arg("g"),
             "Permit: keep sets whose elements are permitted by g.")
        .def("nonsup", &ZDD::Nonsup, py::arg("g"),
             "Remove sets that are supersets of some set in g.")
        .def("nonsub", &ZDD::Nonsub, py::arg("g"),
             "Remove sets that are subsets of some set in g.")
        .def("disjoin", &ZDD::Disjoin, py::arg("g"),
             "Disjoint product.")
        .def("jointjoin", &ZDD::Jointjoin, py::arg("g"),
             "Joint join.")
        .def("delta", &ZDD::Delta, py::arg("g"),
             "Delta operation.")

        // I/O
        .def("export_str", [](const ZDD& z) -> std::string {
            std::ostringstream oss;
            bddp p = z.root;
            bddexport(oss, &p, 1);
            return oss.str();
        }, "Export ZDD to a string.")
        .def_static("import_str", [](const std::string& s) -> ZDD {
            ensure_init();
            std::istringstream iss(s);
            bddp p;
            int ret = bddimportz(iss, &p, 1);
            if (ret < 1) throw std::runtime_error("ZDD import failed");
            ZDD z(0);
            z.root = p;
            return z;
        }, py::arg("s"), "Import a ZDD from a string.")
        .def("export_file", [](const ZDD& z, const std::string& path) {
            std::ofstream ofs(path);
            if (!ofs) throw std::runtime_error("Cannot open file: " + path);
            bddp p = z.root;
            bddexport(ofs, &p, 1);
        }, py::arg("path"), "Export ZDD to a file.")
        .def_static("import_file", [](const std::string& path) -> ZDD {
            ensure_init();
            std::ifstream ifs(path);
            if (!ifs) throw std::runtime_error("Cannot open file: " + path);
            bddp p;
            int ret = bddimportz(ifs, &p, 1);
            if (ret < 1) throw std::runtime_error("ZDD import failed");
            ZDD z(0);
            z.root = p;
            return z;
        }, py::arg("path"), "Import a ZDD from a file.")

        .def_property_readonly("card", &ZDD::Card)
        .def_property_readonly("node_id", [](const ZDD& z) { return z.root; })
        .def_property_readonly("top_var", [](const ZDD& z) -> bddvar {
            return bddtop(z.root);
        })
    ;
}
