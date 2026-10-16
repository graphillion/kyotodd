BDD Operations
==============

Logical Operations
------------------

.. doxygenfunction:: bddnot
.. doxygenfunction:: bddand(bddp f, bddp g)
.. doxygenfunction:: bddand(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddor
.. doxygenfunction:: bddxor(bddp f, bddp g)
.. doxygenfunction:: bddxor(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddnand
.. doxygenfunction:: bddnor
.. doxygenfunction:: bddxnor

Cofactoring
-----------

.. doxygenfunction:: bddat0(bddp f, bddvar v)
.. doxygenfunction:: bddat0(bddp f, bddvar v, BddExecMode mode)
.. doxygenfunction:: bddat1(bddp f, bddvar v)
.. doxygenfunction:: bddat1(bddp f, bddvar v, BddExecMode mode)
.. doxygenfunction:: bddcofactor(bddp f, bddp g)
.. doxygenfunction:: bddcofactor(bddp f, bddp g, BddExecMode mode)

If-Then-Else
-------------

.. doxygenfunction:: bddite(bddp f, bddp g, bddp h)
.. doxygenfunction:: bddite(bddp f, bddp g, bddp h, BddExecMode mode)

Implication
-----------

.. doxygenfunction:: bddimply

Support Set
-----------

.. doxygenfunction:: bddsupport
.. doxygenfunction:: bddsupport_vec

Quantification
--------------

.. doxygenfunction:: bddexist(bddp f, bddp g)
.. doxygenfunction:: bddexist(bddp f, const std::vector< bddvar > &vars)
.. doxygenfunction:: bddexistvar
.. doxygenfunction:: bdduniv(bddp f, bddp g)
.. doxygenfunction:: bdduniv(bddp f, const std::vector< bddvar > &vars)
.. doxygenfunction:: bddunivvar

Variable Shifting
-----------------

.. doxygenfunction:: bddlshiftb
.. doxygenfunction:: bddrshiftb

Variable Swapping
-----------------

.. doxygenfunction:: bddswap(bddp f, bddvar v1, bddvar v2)
.. doxygenfunction:: bddswap(bddp f, bddvar v1, bddvar v2, BddExecMode mode)

Smoothing
---------

.. doxygenfunction:: bddsmooth(bddp f, bddvar v)
.. doxygenfunction:: bddsmooth(bddp f, bddvar v, BddExecMode mode)

Spreading
---------

.. doxygenfunction:: bddspread(bddp f, int k)
.. doxygenfunction:: bddspread(bddp f, int k, BddExecMode mode)

Satisfiability Counting
-----------------------

.. doxygenfunction:: bddcount(bddp f, bddvar n)
.. doxygenfunction:: bddexactcount(bddp f, bddvar n)
.. doxygenfunction:: bddexactcount(bddp f, bddvar n, CountMemoMap &memo)
