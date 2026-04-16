Memo Classes
============

.. py:class:: BddCountMemo(bdd, n)

   Memo for BDD exact counting.

   Caches ``exact_count`` results so they can be reused across
   multiple ``exact_count_with_memo`` and ``uniform_sample_with_memo``
   calls on the same BDD.

   :param BDD bdd: The BDD to count.
   :param int n: Number of variables.

   .. py:property:: stored
      :type: bool

      Whether the memo has been populated.

.. py:class:: ZddCountMemo(zdd)

   Memo for ZDD exact counting.

   Caches ``exact_count`` results so they can be reused across
   multiple ``exact_count_with_memo`` and ``uniform_sample_with_memo``
   calls on the same ZDD.

   :param ZDD zdd: The ZDD to count.

   .. py:property:: stored
      :type: bool

      Whether the memo has been populated.

.. py:class:: CostBoundMemo()

   Interval-memoization table for cost-bounded enumeration.

   Caches intermediate results using the interval-memoizing technique
   (BkTrk-IntervalMemo) so that repeated :py:meth:`ZDD.cost_bound_le_with_memo` /
   :py:meth:`ZDD.cost_bound_ge_with_memo` / :py:meth:`ZDD.cost_bound_eq_with_memo`
   calls with different bounds on the same ZDD and weights are efficient.

   A single ``CostBoundMemo`` must only be used with one weights vector.
   Passing a different weights vector raises ``ValueError``.

   .. py:method:: clear()

      Clear all cached entries. The weights binding is preserved.

.. py:class:: WeightMode

   Weight aggregation mode for :py:meth:`ZDD.weighted_sample` and
   :py:class:`WeightedSampleMemo`.

   .. py:attribute:: Sum

      ``w(S) = sum of weights``. Empty set weight = 0.

   .. py:attribute:: Product

      ``w(S) = product of weights``. Empty set weight = 1.

.. py:class:: WeightedSampleMemo(zdd, weights, mode)

   Memo for weighted sampling.

   Caches precomputed weight values for
   :py:meth:`ZDD.weighted_sample_with_memo` and
   :py:meth:`ZDD.boltzmann_sample_with_memo` calls.

   :param ZDD zdd: The ZDD to sample from.
   :param list[float] weights: Weight vector indexed by variable number.
   :param WeightMode mode: Weight aggregation mode (``WeightMode.Sum`` or ``WeightMode.Product``).

   .. py:property:: stored
      :type: bool

      Whether the memo has been populated.
