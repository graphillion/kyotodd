Infrastructure Functions
========================

Initialization
--------------

.. doxygenfunction:: bddinit
.. doxygenfunction:: BDD_Init

Variable Management
-------------------

.. doxygenfunction:: bddnewvar
.. doxygenfunction:: BDD_NewVar
.. doxygenfunction:: bddnewvaroflev
.. doxygenfunction:: bddlevofvar
.. doxygenfunction:: bddvaroflev
.. doxygenfunction:: bddvarused

Node Queries
------------

.. doxygenfunction:: bddtop
.. doxygenfunction:: bddcopy
.. doxygenfunction:: bddfree
.. doxygenfunction:: bddused
.. doxygenfunction:: bddsize
.. doxygenfunction:: bddvsize(bddp *p, int lim)
.. doxygenfunction:: bddvsize(const std::vector< bddp > &v)

Node Creation
-------------

.. doxygenfunction:: getnode
.. doxygenfunction:: getznode
.. doxygenfunction:: bddprime
.. doxygenfunction:: BDD_ID
.. doxygenfunction:: BDDvar

Garbage Collection
------------------

.. doxygenfunction:: bddgc
.. doxygenfunction:: bddgc_protect
.. doxygenfunction:: bddgc_unprotect
.. doxygenfunction:: bddgc_setthreshold
.. doxygenfunction:: bddgc_getthreshold
.. doxygenfunction:: bddlive

Deprecated
----------

.. doxygenfunction:: bddisbdd
.. doxygenfunction:: bddiszbdd
