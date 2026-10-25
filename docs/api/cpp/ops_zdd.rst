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
.. doxygenfunction:: bddremainder(bddp f, bddp g)
.. doxygenfunction:: bddremainder(bddp f, bddp g, BddExecMode mode)

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
.. doxygenfunction:: bddflatten(bddp f)
.. doxygenfunction:: bddflatten(bddp f, BddExecMode mode)

Push
----

.. doxygenfunction:: bddpush

Variable Shifting
-----------------

.. doxygenfunction:: bddlshiftz(bddp f, bddvar shift)
.. doxygenfunction:: bddlshiftz(bddp f, bddvar shift, BddExecMode mode)
.. doxygenfunction:: bddrshiftz(bddp f, bddvar shift)
.. doxygenfunction:: bddrshiftz(bddp f, bddvar shift, BddExecMode mode)

Counting
--------

.. doxygenfunction:: bddcard

.. note::

   ``bddcard`` is deprecated, and so are its member wrappers ``ZDD::card()``
   and ``ZDD::Card()``: the result saturates at 2^39 - 1, and there is no
   iterative implementation, so a ZDD deeper than ``BDD_RecurLimit`` throws
   ``std::overflow_error``. Use ``ZDD::exact_count()`` (arbitrary precision)
   or ``ZDD::count()`` (double) instead.

   ``bddcard`` is not available in the Python API.
   Use ``ZDD.exact_count`` (arbitrary precision) or ``ZDD.count()`` (float) instead.

.. doxygenfunction:: bddcount(bddp f)
.. doxygenfunction:: bddlit(bddp f)
.. doxygenfunction:: bddlit(bddp f, BddExecMode mode)
.. doxygenfunction:: bddlen(bddp f)
.. doxygenfunction:: bddlen(bddp f, BddExecMode mode)
.. doxygenfunction:: bddmaxsize(bddp f)
.. doxygenfunction:: bddmaxsize(bddp f, BddExecMode mode)
.. doxygenfunction:: bddexactcount(bddp f)
.. doxygenfunction:: bddexactcount(bddp f, CountMemoMap &memo)
.. doxygenfunction:: bddcardmp16
.. doxygenfunction:: bddhasempty

Similarity Measures
-------------------

.. doxygenfunction:: bddjaccardindex(bddp f, bddp g)
.. doxygenfunction:: bddjaccardindex(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddhammingdist(bddp f, bddp g)
.. doxygenfunction:: bddhammingdist(bddp f, bddp g, BddExecMode mode)
.. doxygenfunction:: bddoverlapcoeff(bddp f, bddp g)
.. doxygenfunction:: bddoverlapcoeff(bddp f, bddp g, BddExecMode mode)

Weight Functions
----------------

.. doxygenfunction:: bddweightsum(bddp f, const std::vector<int> &weights)
.. doxygenfunction:: bddweightsum(bddp f, const std::vector<int> &weights, BddExecMode mode)
.. doxygenfunction:: bddminweightset(bddp f, const std::vector<int> &weights)
.. doxygenfunction:: bddminweightset(bddp f, const std::vector<int> &weights, BddExecMode mode)
.. doxygenfunction:: bddmaxweightset(bddp f, const std::vector<int> &weights)
.. doxygenfunction:: bddmaxweightset(bddp f, const std::vector<int> &weights, BddExecMode mode)

Cost-Bound Filtering
--------------------

.. doxygenfunction:: bddcostbound_le(bddp f, const std::vector<int> &weights, long long b, CostBoundMemo &memo)
.. doxygenfunction:: bddcostbound_le(bddp f, const std::vector<int> &weights, long long b, CostBoundMemo &memo, BddExecMode mode)
.. doxygenfunction:: bddcostbound_ge(bddp f, const std::vector<int> &weights, long long b, CostBoundMemo &memo)
.. doxygenfunction:: bddcostbound_ge(bddp f, const std::vector<int> &weights, long long b, CostBoundMemo &memo, BddExecMode mode)

Weighted Selection
------------------

.. doxygenfunction:: bddgetklightest(bddp f, int64_t k, const std::vector<int> &weights, int strict)
.. doxygenfunction:: bddgetklightest(bddp f, int64_t k, const std::vector<int> &weights, int strict, BddExecMode mode)
.. doxygenfunction:: bddgetklightest(bddp f, const bigint::BigInt &k, const std::vector<int> &weights, int strict)
.. doxygenfunction:: bddgetklightest(bddp f, const bigint::BigInt &k, const std::vector<int> &weights, int strict, BddExecMode mode)
.. doxygenfunction:: bddgetkheaviest(bddp f, int64_t k, const std::vector<int> &weights, int strict)
.. doxygenfunction:: bddgetkheaviest(bddp f, int64_t k, const std::vector<int> &weights, int strict, BddExecMode mode)
.. doxygenfunction:: bddgetkheaviest(bddp f, const bigint::BigInt &k, const std::vector<int> &weights, int strict)
.. doxygenfunction:: bddgetkheaviest(bddp f, const bigint::BigInt &k, const std::vector<int> &weights, int strict, BddExecMode mode)

Ranking / Unranking
-------------------

.. doxygenfunction:: bddrank(bddp f, const std::vector<bddvar>& s)
.. doxygenfunction:: bddrank(bddp f, const std::vector<bddvar>& s, BddExecMode mode)
.. doxygenfunction:: bddexactrank(bddp f, const std::vector<bddvar>& s)
.. doxygenfunction:: bddexactrank(bddp f, const std::vector<bddvar>& s, BddExecMode mode)
.. doxygenfunction:: bddexactrank(bddp f, const std::vector<bddvar>& s, CountMemoMap& memo)
.. doxygenfunction:: bddexactrank(bddp f, const std::vector<bddvar>& s, CountMemoMap& memo, BddExecMode mode)
.. doxygenfunction:: bddunrank(bddp f, int64_t order)
.. doxygenfunction:: bddunrank(bddp f, int64_t order, BddExecMode mode)
.. doxygenfunction:: bddexactunrank(bddp f, const bigint::BigInt& order)
.. doxygenfunction:: bddexactunrank(bddp f, const bigint::BigInt& order, BddExecMode mode)
.. doxygenfunction:: bddexactunrank(bddp f, const bigint::BigInt& order, CountMemoMap& memo)
.. doxygenfunction:: bddexactunrank(bddp f, const bigint::BigInt& order, CountMemoMap& memo, BddExecMode mode)
.. doxygenfunction:: bddgetksets(bddp f, int64_t k)
.. doxygenfunction:: bddgetksets(bddp f, int64_t k, BddExecMode mode)
.. doxygenfunction:: bddgetksets(bddp f, const bigint::BigInt& k)
.. doxygenfunction:: bddgetksets(bddp f, const bigint::BigInt& k, BddExecMode mode)
.. doxygenfunction:: bddgetksets(bddp f, const bigint::BigInt& k, CountMemoMap& memo)
.. doxygenfunction:: bddgetksets(bddp f, const bigint::BigInt& k, CountMemoMap& memo, BddExecMode mode)

Random Generation
-----------------

.. doxygenfunction:: ZDD_Random

Variable Analysis
-----------------

.. doxygenfunction:: bddispoly
.. doxygenfunction:: bddswapz(bddp f, bddvar v1, bddvar v2)
.. doxygenfunction:: bddswapz(bddp f, bddvar v1, bddvar v2, BddExecMode mode)
.. doxygenfunction:: bddpermutez(bddp f, const std::vector< bddvar > &from, const std::vector< bddvar > &to)
.. doxygenfunction:: bddpermutez(bddp f, const std::vector< bddvar > &from, const std::vector< bddvar > &to, BddExecMode mode)
.. doxygenfunction:: bddcoalesce(bddp f, bddvar v1, bddvar v2)
.. doxygenfunction:: bddcoalesce(bddp f, bddvar v1, bddvar v2, BddExecMode mode)
.. doxygenfunction:: bddimplychk
.. doxygenfunction:: bddcoimplychk
.. doxygenfunction:: bddpermitsym(bddp f, int n)
.. doxygenfunction:: bddpermitsym(bddp f, int n, BddExecMode mode)
.. doxygenfunction:: bddalways(bddp f)
.. doxygenfunction:: bddalways(bddp f, BddExecMode mode)
.. doxygenfunction:: bddsymchk(bddp f, bddvar v1, bddvar v2)
.. doxygenfunction:: bddsymchk(bddp f, bddvar v1, bddvar v2, BddExecMode mode)
.. doxygenfunction:: bddimplyset(bddp f, bddvar v)
.. doxygenfunction:: bddimplyset(bddp f, bddvar v, BddExecMode mode)
.. doxygenfunction:: bddsymgrp(bddp f)
.. doxygenfunction:: bddsymgrp(bddp f, BddExecMode mode)
.. doxygenfunction:: bddsymgrpnaive(bddp f)
.. doxygenfunction:: bddsymgrpnaive(bddp f, BddExecMode mode)
.. doxygenfunction:: bddsymset(bddp f, bddvar v)
.. doxygenfunction:: bddsymset(bddp f, bddvar v, BddExecMode mode)
.. doxygenfunction:: bddcoimplyset(bddp f, bddvar v)
.. doxygenfunction:: bddcoimplyset(bddp f, bddvar v, BddExecMode mode)
.. doxygenfunction:: bdddivisor(bddp f)
.. doxygenfunction:: bdddivisor(bddp f, BddExecMode mode)

LCM Algorithms
--------------

.. doxygenfunction:: ZDD_LCM_A
.. doxygenfunction:: ZDD_LCM_C
.. doxygenfunction:: ZDD_LCM_M
