Infrastructure Functions
========================

Initialization
--------------

.. doxygenfunction:: bddinit
.. doxygenfunction:: BDD_Init
.. doxygenfunction:: bddfinal

Variable Management
-------------------

.. doxygenfunction:: bddnewvar()
.. doxygenfunction:: bddnewvar(int n)
.. doxygenfunction:: BDD_NewVar
.. doxygenfunction:: bddnewvaroflev
.. doxygenfunction:: bddlevofvar
.. doxygenfunction:: bddvaroflev
.. doxygenfunction:: bddvarused
.. doxygenfunction:: bddtoplev

Node Queries
------------

.. doxygenfunction:: bddtop
.. doxygenfunction:: bddcopy
.. doxygenfunction:: bddfree
.. doxygenfunction:: bddused
.. doxygenfunction:: bddsize
.. doxygenfunction:: bddvsize(bddp *p, size_t lim)
.. doxygenfunction:: bddvsize(const std::vector< bddp > &v)
.. doxygenfunction:: bddplainsize(bddp f, bool is_zdd)
.. doxygenfunction:: bddrawsize(const std::vector< bddp > &v)
.. doxygenfunction:: bddplainsize(const std::vector< bddp > &v, bool is_zdd)

Node Creation
-------------

.. doxygenfunction:: bddconst
.. doxygenfunction:: bddprime
.. doxygenfunction:: BDD_ID
.. doxygenfunction:: ZDD_ID
.. doxygenfunction:: BDDvar
.. doxygenfunction:: ZDD_Meet

Garbage Collection
------------------

.. doxygenfunction:: bddgc
.. doxygenfunction:: bddgc_protect
.. doxygenfunction:: bddgc_unprotect
.. doxygenfunction:: bddgc_setthreshold
.. doxygenfunction:: bddgc_getthreshold
.. doxygenfunction:: bddlive
.. doxygenfunction:: bddgc_rootcount

Validation
----------

.. doxygenfunction:: bdd_check_reduced

Deprecated
----------

.. doxygenfunction:: bddisbdd
.. doxygenfunction:: bddiszbdd
