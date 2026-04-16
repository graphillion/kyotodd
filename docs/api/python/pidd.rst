PiDD Class
==========

PiDD (Permutation Decision Diagram) represents sets of permutations using
ZDDs, where each variable corresponds to an adjacent transposition
Swap(x, y).

Setup Functions
---------------

.. py:function:: kyotodd.pidd_newvar()

   Create a new PiDD variable, extending the permutation size by 1.

   Each call adds BDD variables for the new transpositions involving
   the new element. Call this *n* times to work with permutations
   of size *n*.

   :return: The new permutation size.
   :rtype: int

.. py:function:: kyotodd.pidd_var_used()

   Return the current PiDD permutation size.

   :return: The number of PiDD variables created so far.
   :rtype: int

PiDD Class
----------

.. py:class:: kyotodd.PiDD(val=0)

   A Permutation Decision Diagram based on adjacent transpositions.

   Represents a set of permutations compactly using a ZDD. Each path
   in the ZDD encodes a permutation as a composition of adjacent
   transpositions Swap(x, y).

   :param int val: Initial value. 0 for empty set, 1 for the set
                   containing only the identity permutation, negative
                   for null.

   Set Operators
   ~~~~~~~~~~~~~

   .. py:method:: __add__(other)

      Union: ``self + other``.

      :param PiDD other: Right operand.
      :rtype: PiDD

   .. py:method:: __sub__(other)

      Difference: ``self - other``.

      :param PiDD other: Right operand.
      :rtype: PiDD

   .. py:method:: __and__(other)

      Intersection: ``self & other``.

      :param PiDD other: Right operand.
      :rtype: PiDD

   .. py:method:: __mul__(other)

      Composition: ``self * other``.

      Computes { pi . sigma | pi in self, sigma in other }.

      :param PiDD other: Right operand.
      :rtype: PiDD

   .. py:method:: __truediv__(other)

      Division: ``self / other``.

      :param PiDD other: Right operand.
      :rtype: PiDD

   .. py:method:: __mod__(other)

      Remainder: ``self % other``.

      :param PiDD other: Right operand.
      :rtype: PiDD

   .. py:method:: __iadd__(other)
                   __isub__(other)
                   __iand__(other)
                   __imul__(other)
                   __itruediv__(other)
                   __imod__(other)

      In-place variants of the above operators.

   .. py:method:: __eq__(other)

      Equality comparison.

   .. py:method:: __ne__(other)

      Inequality comparison.

   .. py:method:: __hash__()

      Hash based on internal ZDD node ID.

   .. py:method:: __repr__()

      Return string representation: ``PiDD: id=...``.

   .. py:method:: __bool__()

      Always raises :exc:`TypeError`.

      PiDD cannot be converted to bool.

      :raises TypeError: Always.

   Core Operations
   ~~~~~~~~~~~~~~~

   .. py:method:: swap(u, v)

      Apply transposition Swap(u, v) to all permutations in the set.

      Composes the transposition that exchanges positions *u* and *v*
      from the left onto every permutation in the set.

      :param int u: First position.
      :param int v: Second position.
      :return: A new PiDD with the transposition applied.
      :rtype: PiDD

   .. py:method:: cofact(u, v)

      Extract permutations where position *u* has value *v*.

      Returns the sub-permutations (with position *u* removed) of all
      permutations sigma in the set such that sigma(u) = v.

      :param int u: Position to check.
      :param int v: Required value at position *u*.
      :return: A PiDD of sub-permutations satisfying the condition.
      :rtype: PiDD

   .. py:method:: odd()

      Extract odd permutations from the set.

      :return: A PiDD containing only the odd permutations.
      :rtype: PiDD

   .. py:method:: even()

      Extract even permutations from the set.

      :return: A PiDD containing only the even permutations.
      :rtype: PiDD

   .. py:method:: swap_bound(n)

      Apply symmetric constraint (PermitSym) to the internal ZDD.

      :param int n: Constraint parameter.
      :return: A PiDD with the constraint applied.
      :rtype: PiDD

   Properties
   ~~~~~~~~~~

   .. py:property:: exact_count
      :type: int

      The number of permutations in the set.

   .. py:property:: size
      :type: int

      The number of nodes in the internal ZDD.

   .. py:property:: top_x
      :type: int

      The x coordinate of the top variable.

   .. py:property:: top_y
      :type: int

      The y coordinate of the top variable.

   .. py:property:: top_lev
      :type: int

      The BDD level of the top variable.

   .. py:property:: zdd
      :type: ZDD

      The internal ZDD representation.

   Display
   ~~~~~~~

   .. py:method:: print()

      Print PiDD statistics and return as string.

      :rtype: str

   .. py:method:: enum()

      Enumerate all permutations in vector notation and return as string.

      :rtype: str

   .. py:method:: enum2()

      Enumerate all permutations in transposition notation and return as string.

      :rtype: str

Example
-------

.. code-block:: python

   import kyotodd

   kyotodd.init()

   # Set up permutations of size 4
   for _ in range(4):
       kyotodd.pidd_newvar()

   # Identity permutation
   e = kyotodd.PiDD(1)

   # Apply transposition (swap positions 1 and 2)
   s12 = e.swap(2, 1)

   # Apply transposition (swap positions 2 and 3)
   s23 = e.swap(3, 2)

   # Compose: s23 * s12 = apply s12 first, then s23
   composed = s23 * s12

   # Union: set of two permutations
   pair = s12 + s23
   assert pair.exact_count == 2

   # Parity filtering
   assert s12.odd().exact_count == 1   # single swap is odd
   assert e.even().exact_count == 1    # identity is even
