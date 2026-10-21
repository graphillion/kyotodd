BDD Operations
==============

Logical Operations
------------------

.. doxygenfunction:: bddnot
.. doxygenfunction:: bddand(bddp f, bddp g)
.. doxygenfunction:: bddand(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddor(bddp f, bddp g)
.. doxygenfunction:: bddor(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddxor(bddp f, bddp g)
.. doxygenfunction:: bddxor(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddnand(bddp f, bddp g)
.. doxygenfunction:: bddnand(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddnor(bddp f, bddp g)
.. doxygenfunction:: bddnor(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddxnor(bddp f, bddp g)
.. doxygenfunction:: bddxnor(bddp f, bddp g, BddExecMode mode)

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

.. doxygenfunction:: bddimply(bddp f, bddp g)
.. doxygenfunction:: bddimply(bddp f, bddp g, BddExecMode mode)

Support Set
-----------

.. doxygenfunction:: bddsupport
.. doxygenfunction:: bddsupport_vec

Quantification
--------------

.. doxygenfunction:: bddexist(bddp f, bddp g)
.. doxygenfunction:: bddexist(bddp f, const std::vector< bddvar > &vars)
.. doxygenfunction:: bddexist(bddp f, const std::vector< bddvar > &vars, BddExecMode mode)
.. doxygenfunction:: bddexistvar(bddp f, bddvar v)
.. doxygenfunction:: bddexistvar(bddp f, bddvar v, BddExecMode mode)
.. doxygenfunction:: bdduniv(bddp f, bddp g)
.. doxygenfunction:: bdduniv(bddp f, const std::vector< bddvar > &vars)
.. doxygenfunction:: bdduniv(bddp f, const std::vector< bddvar > &vars, BddExecMode mode)
.. doxygenfunction:: bddunivvar(bddp f, bddvar v)
.. doxygenfunction:: bddunivvar(bddp f, bddvar v, BddExecMode mode)

Variable Shifting
-----------------

.. doxygenfunction:: bddlshiftb(bddp f, bddvar shift)
.. doxygenfunction:: bddlshiftb(bddp f, bddvar shift, BddExecMode mode)
.. doxygenfunction:: bddrshiftb(bddp f, bddvar shift)
.. doxygenfunction:: bddrshiftb(bddp f, bddvar shift, BddExecMode mode)

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
