I/O Functions
=============

Sapporo Format
--------------

.. doxygenfunction:: bdd_export_sapporo(FILE *strm, bddp f)
.. doxygenfunction:: bdd_export_sapporo(std::ostream &strm, bddp f)
.. doxygenfunction:: bdd_import_sapporo(FILE *strm)
.. doxygenfunction:: bdd_import_sapporo(std::istream &strm)
.. doxygenfunction:: zdd_export_sapporo(FILE *strm, bddp f)
.. doxygenfunction:: zdd_export_sapporo(std::ostream &strm, bddp f)
.. doxygenfunction:: zdd_import_sapporo(FILE *strm)
.. doxygenfunction:: zdd_import_sapporo(std::istream &strm)

Binary Format (BDD)
-------------------

.. doxygenfunction:: bdd_export_binary(FILE *strm, bddp f)
.. doxygenfunction:: bdd_export_binary(std::ostream &strm, bddp f)
.. doxygenfunction:: bdd_import_binary(FILE *strm, bool ignore_type)
.. doxygenfunction:: bdd_import_binary(std::istream &strm, bool ignore_type)

Binary Format (ZDD)
-------------------

.. doxygenfunction:: zdd_export_binary(FILE *strm, bddp f)
.. doxygenfunction:: zdd_export_binary(std::ostream &strm, bddp f)
.. doxygenfunction:: zdd_import_binary(FILE *strm, bool ignore_type)
.. doxygenfunction:: zdd_import_binary(std::istream &strm, bool ignore_type)

Binary Format (QDD)
-------------------

.. doxygenfunction:: qdd_export_binary(FILE *strm, bddp f)
.. doxygenfunction:: qdd_export_binary(std::ostream &strm, bddp f)
.. doxygenfunction:: qdd_import_binary(FILE *strm, bool ignore_type)
.. doxygenfunction:: qdd_import_binary(std::istream &strm, bool ignore_type)

Binary Format (UnreducedDD)
---------------------------

.. doxygenfunction:: unreduced_export_binary(FILE *strm, bddp f)
.. doxygenfunction:: unreduced_export_binary(std::ostream &strm, bddp f)
.. doxygenfunction:: unreduced_import_binary(FILE *strm)
.. doxygenfunction:: unreduced_import_binary(std::istream &strm)

Multi-root Binary Format (BDD)
------------------------------

.. doxygenfunction:: bdd_export_binary_multi(FILE *strm, const bddp *roots, size_t n)
.. doxygenfunction:: bdd_export_binary_multi(std::ostream &strm, const bddp *roots, size_t n)
.. doxygenfunction:: bdd_import_binary_multi(FILE *strm, bool ignore_type)
.. doxygenfunction:: bdd_import_binary_multi(std::istream &strm, bool ignore_type)

Multi-root Binary Format (ZDD)
------------------------------

.. doxygenfunction:: zdd_export_binary_multi(FILE *strm, const bddp *roots, size_t n)
.. doxygenfunction:: zdd_export_binary_multi(std::ostream &strm, const bddp *roots, size_t n)
.. doxygenfunction:: zdd_import_binary_multi(FILE *strm, bool ignore_type)
.. doxygenfunction:: zdd_import_binary_multi(std::istream &strm, bool ignore_type)

Multi-root Binary Format (QDD)
------------------------------

.. doxygenfunction:: qdd_export_binary_multi(FILE *strm, const bddp *roots, size_t n)
.. doxygenfunction:: qdd_export_binary_multi(std::ostream &strm, const bddp *roots, size_t n)
.. doxygenfunction:: qdd_import_binary_multi(FILE *strm, bool ignore_type)
.. doxygenfunction:: qdd_import_binary_multi(std::istream &strm, bool ignore_type)

Graphillion Format
------------------

.. doxygenfunction:: zdd_export_graphillion(FILE *strm, bddp f, int offset)
.. doxygenfunction:: zdd_export_graphillion(std::ostream &strm, bddp f, int offset)
.. doxygenfunction:: zdd_import_graphillion(FILE *strm, int offset)
.. doxygenfunction:: zdd_import_graphillion(std::istream &strm, int offset)

Graphviz DOT Format
-------------------

.. doxygenfunction:: bdd_save_graphviz(FILE *strm, bddp f, GraphvizMode mode)
.. doxygenfunction:: bdd_save_graphviz(std::ostream &strm, bddp f, GraphvizMode mode)
.. doxygenfunction:: zdd_save_graphviz(FILE *strm, bddp f, GraphvizMode mode)
.. doxygenfunction:: zdd_save_graphviz(std::ostream &strm, bddp f, GraphvizMode mode)

Legacy Sapporo Format
---------------------

.. doxygenfunction:: bddexport(FILE *strm, const bddp *p, int lim)
.. doxygenfunction:: bddexport(FILE *strm, const std::vector< bddp > &v)
.. doxygenfunction:: bddexport(std::ostream &strm, const bddp *p, int lim)
.. doxygenfunction:: bddexport(std::ostream &strm, const std::vector< bddp > &v)
.. doxygenfunction:: bddimport(FILE *strm, bddp *p, int lim)
.. doxygenfunction:: bddimport(FILE *strm, std::vector< bddp > &v)
.. doxygenfunction:: bddimport(std::istream &strm, bddp *p, int lim)
.. doxygenfunction:: bddimport(std::istream &strm, std::vector< bddp > &v)
.. doxygenfunction:: bddimportz(FILE *strm, bddp *p, int lim)
.. doxygenfunction:: bddimportz(FILE *strm, std::vector< bddp > &v)
.. doxygenfunction:: bddimportz(std::istream &strm, bddp *p, int lim)
.. doxygenfunction:: bddimportz(std::istream &strm, std::vector< bddp > &v)
.. doxygenfunction:: ZDD_Import(FILE *strm)
.. doxygenfunction:: ZDD_Import(FILE *strm, std::vector< ZDD > &v)
.. doxygenfunction:: ZDD_Import(std::istream &strm)
.. doxygenfunction:: ZDD_Import(std::istream &strm, std::vector< ZDD > &v)

Knuth Format (Deprecated)
-------------------------

.. doxygenfunction:: bdd_export_knuth(FILE *strm, bddp f, bool is_hex, int offset)
.. doxygenfunction:: bdd_export_knuth(std::ostream &strm, bddp f, bool is_hex, int offset)
.. doxygenfunction:: bdd_import_knuth(FILE *strm, bool is_hex, int offset)
.. doxygenfunction:: bdd_import_knuth(std::istream &strm, bool is_hex, int offset)
.. doxygenfunction:: zdd_export_knuth(FILE *strm, bddp f, bool is_hex, int offset)
.. doxygenfunction:: zdd_export_knuth(std::ostream &strm, bddp f, bool is_hex, int offset)
.. doxygenfunction:: zdd_import_knuth(FILE *strm, bool is_hex, int offset)
.. doxygenfunction:: zdd_import_knuth(std::istream &strm, bool is_hex, int offset)

Debug Dump
----------

.. doxygenfunction:: bdddump
.. doxygenfunction:: bddvdump
