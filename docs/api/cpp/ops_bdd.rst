BDD Operations
==============

Logical Operations
------------------

.. doxygenfunction:: bddnot
.. doxygenfunction:: bddand
.. doxygenfunction:: bddor
.. doxygenfunction:: bddxor
.. doxygenfunction:: bddnand
.. doxygenfunction:: bddnor
.. doxygenfunction:: bddxnor

Cofactoring
-----------

.. doxygenfunction:: bddat0
.. doxygenfunction:: bddat1
.. doxygenfunction:: bddcofactor

If-Then-Else
-------------

.. doxygenfunction:: bddite

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

.. doxygenfunction:: bddswap

Smoothing
---------

.. doxygenfunction:: bddsmooth

Spreading
---------

.. doxygenfunction:: bddspread
