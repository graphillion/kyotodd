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
