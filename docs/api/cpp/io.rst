I/O Functions
=============

Export
------

.. doxygenfunction:: bddexport(FILE *strm, const bddp *p, int lim)
.. doxygenfunction:: bddexport(FILE *strm, const std::vector< bddp > &v)
.. doxygenfunction:: bddexport(std::ostream &strm, const bddp *p, int lim)
.. doxygenfunction:: bddexport(std::ostream &strm, const std::vector< bddp > &v)

Import (BDD)
------------

.. doxygenfunction:: bddimport(FILE *strm, bddp *p, int lim)
.. doxygenfunction:: bddimport(FILE *strm, std::vector< bddp > &v)
.. doxygenfunction:: bddimport(std::istream &strm, bddp *p, int lim)
.. doxygenfunction:: bddimport(std::istream &strm, std::vector< bddp > &v)

Import (ZDD)
------------

.. doxygenfunction:: bddimportz(FILE *strm, bddp *p, int lim)
.. doxygenfunction:: bddimportz(FILE *strm, std::vector< bddp > &v)
.. doxygenfunction:: bddimportz(std::istream &strm, bddp *p, int lim)
.. doxygenfunction:: bddimportz(std::istream &strm, std::vector< bddp > &v)

Import (ZDD objects)
--------------------

.. doxygenfunction:: ZDD_Import(FILE *strm)
.. doxygenfunction:: ZDD_Import(FILE *strm, std::vector< ZDD > &v)
.. doxygenfunction:: ZDD_Import(std::istream &strm)
.. doxygenfunction:: ZDD_Import(std::istream &strm, std::vector< ZDD > &v)

Debug Dump
----------

.. doxygenfunction:: bdddump
.. doxygenfunction:: bddvdump
