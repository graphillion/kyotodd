I/O Functions
=============

Import Limits
-------------

Every import function (Sapporo text, binary, Graphillion and Knuth) checks
the node, level and root counts declared in the file header against a set of
safety limits before allocating anything, so that a corrupted or malicious
file cannot trigger a huge allocation. The defaults are 10,000,000 nodes,
1,000,000 levels and 1,000,000 roots. To load a larger DD from a trusted
file, raise the limits or pass 0 to disable a limit entirely:

.. code-block:: cpp

   // Accept any number of nodes; keep the default level and root bounds.
   bdd_set_import_limit(0, BDD_IMPORT_DEFAULT_MAX_LEVELS,
                        BDD_IMPORT_DEFAULT_MAX_OUTPUTS);
   ZDD big = ZDD::import_binary(strm);

.. doxygenstruct:: kyotodd::ImportLimit
   :members:

.. doxygenfunction:: bdd_set_import_limit
.. doxygenfunction:: bdd_get_import_limit

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

.. doxygenfunction:: bdd_save_graphviz(FILE *strm, bddp f, DrawMode mode)
.. doxygenfunction:: bdd_save_graphviz(std::ostream &strm, bddp f, DrawMode mode)
.. doxygenfunction:: zdd_save_graphviz(FILE *strm, bddp f, DrawMode mode)
.. doxygenfunction:: zdd_save_graphviz(std::ostream &strm, bddp f, DrawMode mode)

SVG Format
----------

Self-contained SVG drawings of a decision diagram (also available as the
``save_svg`` member functions of :cpp:class:`kyotodd::BDD`,
:cpp:class:`kyotodd::ZDD`, ``QDD``, ``UnreducedDD``, ``MTBDD`` / ``MTZDD``
and ``MVBDD`` / ``MVZDD``). Nodes are circles labelled with the variable
number (or the name from ``var_name_map``), terminals are rectangles, 0-edges
are dashed and 1-edges solid. With ``DrawMode::Expanded`` (the default)
complement edges are expanded into separate nodes; with ``DrawMode::Raw``
the physical DAG is drawn and complement edges carry a hollow dot marker.

Layout notes:

- Levels without nodes take no row by default (``skip_unused_levels``);
  set it to ``false`` to reserve a row for every level.
- An arc is drawn as a straight segment when it keeps clear of every other
  node and arrives steeply at its target; otherwise it is routed
  orthogonally through the gaps between rows and the lanes between nodes.
- Labels that do not fit inside their node are drawn with a smaller font.
- Arrowheads, dot markers and outlines use ``node_stroke``; labels use
  ``font_family``.

.. doxygenstruct:: kyotodd::SvgParams
   :members:

.. doxygenfunction:: bdd_save_svg(const char *filename, bddp f, const SvgParams &params)
.. doxygenfunction:: bdd_save_svg(std::ostream &strm, bddp f, const SvgParams &params)
.. doxygenfunction:: bdd_save_svg(bddp f, const SvgParams &params)
.. doxygenfunction:: zdd_save_svg(const char *filename, bddp f, const SvgParams &params)
.. doxygenfunction:: zdd_save_svg(std::ostream &strm, bddp f, const SvgParams &params)
.. doxygenfunction:: zdd_save_svg(bddp f, const SvgParams &params)
.. doxygenfunction:: qdd_save_svg(const char *filename, bddp f, const SvgParams &params)
.. doxygenfunction:: qdd_save_svg(std::ostream &strm, bddp f, const SvgParams &params)
.. doxygenfunction:: qdd_save_svg(bddp f, const SvgParams &params)
.. doxygenfunction:: unreduced_save_svg(const char *filename, bddp f, const SvgParams &params)
.. doxygenfunction:: unreduced_save_svg(std::ostream &strm, bddp f, const SvgParams &params)
.. doxygenfunction:: unreduced_save_svg(bddp f, const SvgParams &params)
.. doxygenfunction:: mtbdd_save_svg(const char *filename, bddp f, const SvgParams &params)
.. doxygenfunction:: mtbdd_save_svg(std::ostream &strm, bddp f, const SvgParams &params)
.. doxygenfunction:: mtbdd_save_svg(bddp f, const SvgParams &params)
.. doxygenfunction:: mtzdd_save_svg(const char *filename, bddp f, const SvgParams &params)
.. doxygenfunction:: mtzdd_save_svg(std::ostream &strm, bddp f, const SvgParams &params)
.. doxygenfunction:: mtzdd_save_svg(bddp f, const SvgParams &params)
.. doxygenfunction:: mvbdd_save_svg(const char *filename, bddp f, const MVDDVarTable *table, const SvgParams &params)
.. doxygenfunction:: mvbdd_save_svg(std::ostream &strm, bddp f, const MVDDVarTable *table, const SvgParams &params)
.. doxygenfunction:: mvbdd_save_svg(bddp f, const MVDDVarTable *table, const SvgParams &params)
.. doxygenfunction:: mvzdd_save_svg(const char *filename, bddp f, const MVDDVarTable *table, const SvgParams &params)
.. doxygenfunction:: mvzdd_save_svg(std::ostream &strm, bddp f, const MVDDVarTable *table, const SvgParams &params)
.. doxygenfunction:: mvzdd_save_svg(bddp f, const MVDDVarTable *table, const SvgParams &params)

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

.. doxygenfunction:: bdddump(bddp f)
.. doxygenfunction:: bdddump(bddp f, BddExecMode mode)
.. doxygenfunction:: bddvdump
