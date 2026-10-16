ZDD Operations
==============

Selection Operations
--------------------

.. doxygenfunction:: bddoffset(bddp f, bddvar var)
.. doxygenfunction:: bddoffset(bddp f, bddvar var, BddExecMode mode)
.. doxygenfunction:: bddonset(bddp f, bddvar var)
.. doxygenfunction:: bddonset(bddp f, bddvar var, BddExecMode mode)
.. doxygenfunction:: bddonset0(bddp f, bddvar var)
.. doxygenfunction:: bddonset0(bddp f, bddvar var, BddExecMode mode)
.. doxygenfunction:: bddchange(bddp f, bddvar var)
.. doxygenfunction:: bddchange(bddp f, bddvar var, BddExecMode mode)

Set Operations
--------------

.. doxygenfunction:: bddunion(bddp f, bddp g)
.. doxygenfunction:: bddunion(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddintersec(bddp f, bddp g)
.. doxygenfunction:: bddintersec(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddsubtract(bddp f, bddp g)
.. doxygenfunction:: bddsubtract(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddsymdiff(bddp f, bddp g)
.. doxygenfunction:: bddsymdiff(bddp f, bddp g, BddExecMode mode)

Algebraic Operations
--------------------

.. doxygenfunction:: bdddiv(bddp f, bddp g)
.. doxygenfunction:: bdddiv(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddremainder

Cross-Product Operations
------------------------

.. doxygenfunction:: bddjoin(bddp f, bddp g)
.. doxygenfunction:: bddjoin(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddmeet(bddp f, bddp g)
.. doxygenfunction:: bddmeet(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bdddelta(bddp f, bddp g)
.. doxygenfunction:: bdddelta(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bdddisjoin(bddp f, bddp g)
.. doxygenfunction:: bdddisjoin(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddjointjoin(bddp f, bddp g)
.. doxygenfunction:: bddjointjoin(bddp f, bddp g, BddExecMode mode)

Filtering Operations
--------------------

.. doxygenfunction:: bddrestrict(bddp f, bddp g)
.. doxygenfunction:: bddrestrict(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddpermit(bddp f, bddp g)
.. doxygenfunction:: bddpermit(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddremove_supersets(bddp f, bddp g)
.. doxygenfunction:: bddremove_supersets(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddremove_subsets(bddp f, bddp g)
.. doxygenfunction:: bddremove_subsets(bddp f, bddp g, BddExecMode mode)

Unary Operations
----------------

.. doxygenfunction:: bddmaximal(bddp f)
.. doxygenfunction:: bddmaximal(bddp f, BddExecMode mode)
.. doxygenfunction:: bddminimal(bddp f)
.. doxygenfunction:: bddminimal(bddp f, BddExecMode mode)
.. doxygenfunction:: bddminhit(bddp f)
.. doxygenfunction:: bddminhit(bddp f, BddExecMode mode)
.. doxygenfunction:: bddclosure(bddp f)
.. doxygenfunction:: bddclosure(bddp f, BddExecMode mode)

Push
----

.. doxygenfunction:: bddpush

Variable Shifting
-----------------

.. doxygenfunction:: bddlshiftz
.. doxygenfunction:: bddrshiftz

Counting
--------

.. doxygenfunction:: bddcard

.. note::

   ``bddcard`` is not available in the Python API.
   Use ``ZDD.exact_count`` (arbitrary precision) or ``ZDD.count()`` (float) instead.

.. doxygenfunction:: bddcount(bddp f)
.. doxygenfunction:: bddlit
.. doxygenfunction:: bddlen
.. doxygenfunction:: bddexactcount(bddp f)
.. doxygenfunction:: bddexactcount(bddp f, CountMemoMap &memo)
.. doxygenfunction:: bddcardmp16
.. doxygenfunction:: bddhasempty

Weight Functions
----------------

.. doxygenfunction:: bddweightsum(bddp f, const std::vector<int> &weights)
.. doxygenfunction:: bddweightsum(bddp f, const std::vector<int> &weights, BddExecMode mode)

Cost-Bound Filtering
--------------------

.. doxygenfunction:: bddcostbound_le(bddp f, const std::vector<int> &weights, long long b, CostBoundMemo &memo)
.. doxygenfunction:: bddcostbound_le(bddp f, const std::vector<int> &weights, long long b, CostBoundMemo &memo, BddExecMode mode)
.. doxygenfunction:: bddcostbound_ge(bddp f, const std::vector<int> &weights, long long b, CostBoundMemo &memo)
.. doxygenfunction:: bddcostbound_ge(bddp f, const std::vector<int> &weights, long long b, CostBoundMemo &memo, BddExecMode mode)

Ranking / Unranking
-------------------

.. doxygenfunction:: bddrank
.. doxygenfunction:: bddexactrank(bddp f, const std::vector<bddvar>& s)
.. doxygenfunction:: bddexactrank(bddp f, const std::vector<bddvar>& s, CountMemoMap& memo)
.. doxygenfunction:: bddunrank
.. doxygenfunction:: bddexactunrank(bddp f, const bigint::BigInt& order)
.. doxygenfunction:: bddexactunrank(bddp f, const bigint::BigInt& order, CountMemoMap& memo)

Random Generation
-----------------

.. doxygenfunction:: ZDD_Random

Variable Analysis
-----------------

.. doxygenfunction:: bddispoly
.. doxygenfunction:: bddswapz
.. doxygenfunction:: bddimplychk
.. doxygenfunction:: bddcoimplychk
.. doxygenfunction:: bddpermitsym(bddp f, int n)
.. doxygenfunction:: bddpermitsym(bddp f, int n, BddExecMode mode)
.. doxygenfunction:: bddalways(bddp f)
.. doxygenfunction:: bddalways(bddp f, BddExecMode mode)
.. doxygenfunction:: bddsymchk(bddp f, bddvar v1, bddvar v2)
.. doxygenfunction:: bddsymchk(bddp f, bddvar v1, bddvar v2, BddExecMode mode)
.. doxygenfunction:: bddimplyset
.. doxygenfunction:: bddsymgrp
.. doxygenfunction:: bddsymgrpnaive
.. doxygenfunction:: bddsymset(bddp f, bddvar v)
.. doxygenfunction:: bddsymset(bddp f, bddvar v, BddExecMode mode)
.. doxygenfunction:: bddcoimplyset(bddp f, bddvar v)
.. doxygenfunction:: bddcoimplyset(bddp f, bddvar v, BddExecMode mode)
.. doxygenfunction:: bdddivisor

LCM Algorithms
--------------

.. doxygenfunction:: ZDD_LCM_A
.. doxygenfunction:: ZDD_LCM_C
.. doxygenfunction:: ZDD_LCM_M
