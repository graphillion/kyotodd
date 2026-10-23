RotPiDD Class
=============

RotPiDD (Rotational Permutation Decision Diagram) represents sets of
permutations using ZDDs, where each variable corresponds to a left
rotation LeftRot(x, y). This is an alternative to PiDD, which uses
adjacent transpositions.

The left rotation LeftRot(u, v) (with u > v) cyclically shifts positions
v, v+1, ..., u to the left: the element at position v moves to position u,
v+1 moves to v, v+2 moves to v+1, and so on.

Setup Functions
---------------

.. py:function:: kyotodd.rotpidd_newvar()

   Create a new RotPiDD variable, extending the permutation size by 1.

   Each call adds BDD variables for the new rotations involving the new
   element. Call this *n* times to work with permutations of size *n*.

   :return: The new permutation size.
   :rtype: int

.. py:function:: kyotodd.rotpidd_var_used()

   Return the current RotPiDD permutation size.

   :return: The number of RotPiDD variables created so far.
   :rtype: int

.. py:function:: kyotodd.rotpidd_from_perm(perm)

   Create a RotPiDD containing a single permutation from a vector.

   The input vector is automatically normalized to [1..n] preserving
   the relative order of elements.

   :param list[int] perm: A list of integers representing a permutation.
   :return: A RotPiDD containing the single permutation.
   :rtype: RotPiDD

RotPiDD Class
-------------

.. py:class:: kyotodd.RotPiDD(val=0)

   A Rotational Permutation Decision Diagram.

   Represents a set of permutations compactly using a ZDD. Each path
   in the ZDD encodes a permutation as a composition of left rotations
   LeftRot(x, y).

   :param int val: Initial value. 0 for empty set, 1 for the set
                   containing only the identity permutation, negative
                   for null.

   Set Operators
   ~~~~~~~~~~~~~

   .. py:method:: __add__(other)

      Union: ``self + other``.

      :param RotPiDD other: Right operand.
      :rtype: RotPiDD

   .. py:method:: __sub__(other)

      Difference: ``self - other``.

      :param RotPiDD other: Right operand.
      :rtype: RotPiDD

   .. py:method:: __and__(other)

      Intersection: ``self & other``.

      :param RotPiDD other: Right operand.
      :rtype: RotPiDD

   .. py:method:: __mul__(other)

      Composition: ``self * other``.

      Computes { pi . sigma | pi in self, sigma in other }.

      :param RotPiDD other: Right operand.
      :rtype: RotPiDD

   .. py:method:: __iadd__(other)
                   __isub__(other)
                   __iand__(other)
                   __imul__(other)

      In-place variants of the above operators.

   .. py:method:: __eq__(other)

      Equality comparison.

   .. py:method:: __ne__(other)

      Inequality comparison.

   .. py:method:: __hash__()

      Hash based on internal ZDD node ID.

   .. py:method:: __repr__()

      Return string representation: ``RotPiDD: id=...``.

   .. py:method:: __bool__()

      Always raises :exc:`TypeError`.

      RotPiDD cannot be converted to bool.

      :raises TypeError: Always.

   Core Operations
   ~~~~~~~~~~~~~~~

   .. py:method:: left_rot(u, v)

      Apply left rotation LeftRot(u, v) to all permutations in the set.

      Cyclically shifts positions v, v+1, ..., u to the left.
      If u < v, the arguments are swapped (always normalized to u > v).
      If u == v, the identity operation is applied.

      :param int u: Upper position.
      :param int v: Lower position.
      :return: A new RotPiDD with the rotation applied.
      :rtype: RotPiDD

   .. py:method:: swap(a, b)

      Swap positions *a* and *b* in all permutations.

      Implemented internally as a composition of left rotations.

      :param int a: First position.
      :param int b: Second position.
      :return: A new RotPiDD with the swap applied.
      :rtype: RotPiDD

   .. py:method:: reverse(l, r)

      Reverse positions *l* through *r* in all permutations.

      :param int l: Left (smaller) position.
      :param int r: Right (larger) position.
      :return: A new RotPiDD with the reversal applied.
      :rtype: RotPiDD

   .. py:method:: cofact(u, v)

      Extract permutations where position *u* has value *v*.

      Returns the sub-permutations (with position *u* removed) of all
      permutations sigma in the set such that sigma(u) = v. Each selected
      permutation becomes a permutation on one fewer element: position *u*
      and value *v* are deleted, and positions above *u* and values above
      *v* shift down by one. Example:
      ``{[1 5 3 2 4]}.cofact(2, 5) == {[1 3 2 4]}``.

      :param int u: Position to check.
      :param int v: Required value at position *u*.
      :return: A RotPiDD of sub-permutations satisfying the condition.
      :rtype: RotPiDD

   Parity
   ~~~~~~

   .. py:method:: odd()

      Extract odd permutations from the set.

      :return: A RotPiDD containing only the odd permutations.
      :rtype: RotPiDD

   .. py:method:: even()

      Extract even permutations from the set.

      :return: A RotPiDD containing only the even permutations.
      :rtype: RotPiDD

   Filtering
   ~~~~~~~~~

   .. py:method:: rot_bound(n)

      Apply symmetric constraint (PermitSym) to the internal ZDD.

      :param int n: Constraint parameter.
      :return: A RotPiDD with the constraint applied.
      :rtype: RotPiDD

   .. py:method:: order(a, b)

      Extract the permutations in which the value *a* precedes the value *b*.

      Keeps only those permutations whose one-line notation lists *a*
      before *b*, i.e. the position holding *a* is smaller than the
      position holding *b* (pi\ :sup:`-1`\ (a) < pi\ :sup:`-1`\ (b)).
      For topological orders this is the constraint "a before b". On all
      permutations of {1, 2, 3}, ``order(1, 2)`` keeps ``[1, 2, 3]``,
      ``[1, 3, 2]`` and ``[3, 1, 2]``.

      To keep the permutations with pi(a) < pi(b) instead (the value at
      position *a* smaller than the value at position *b*), use
      ``inverse().order(a, b).inverse()``.

      :param int a: Value that must come first.
      :param int b: Value that must come later.
      :return: A RotPiDD containing permutations satisfying the order.
      :rtype: RotPiDD

   Transformations
   ~~~~~~~~~~~~~~~

   .. py:method:: inverse()

      Compute the inverse of each permutation in the set.

      Returns { sigma^{-1} | sigma in self }.

      :return: A RotPiDD containing the inverse permutations.
      :rtype: RotPiDD

   .. py:method:: insert(p, v)

      Insert value *v* at position *p* in each permutation, shifting
      existing elements to make room: the result sigma has sigma(p) = v,
      the positions *p*, *p* + 1, ... move up by one, and the values *v*,
      *v* + 1, ... increase by one. This is the inverse of
      :py:meth:`cofact`: ``f.insert(p, v).cofact(p, v) == f``. A
      permutation on *n* elements becomes one on at most
      max(*n* + 1, *p*, *v*) elements, so enough RotPiDD variables must
      exist. Example: ``{[2 1 3]}.insert(2, 4) == {[2 4 1 3]}``; the union
      of ``f.insert(p, n + 1)`` over *p* = 1..*n* + 1 extends every
      permutation in ``f`` by the new element *n* + 1.

      :param int p: Insertion position (1-indexed).
      :param int v: Value to insert (1-indexed).
      :return: A RotPiDD with the element inserted.
      :rtype: RotPiDD
      :raises ValueError: if *p* or *v* is out of range, or if the result
         needs more elements than there are RotPiDD variables.

   .. py:method:: remove_max(k)

      Remove variables with x >= *k*, projecting each permutation
      down to a sub-permutation of size k-1.

      :param int k: Threshold.
      :return: A RotPiDD with large variables removed.
      :rtype: RotPiDD

   .. py:method:: normalize(k)

      Remove all variables with x > *k* by projecting them out.

      Unlike :py:meth:`remove_max`, this simply discards the rotation
      labels without adjusting indices.

      :param int k: Upper bound for retained variables.
      :return: A normalized RotPiDD.
      :rtype: RotPiDD

   Extraction
   ~~~~~~~~~~

   .. py:method:: extract_one()

      Extract a single permutation from the set.

      :return: A RotPiDD containing exactly one permutation.
      :rtype: RotPiDD

   Conversion
   ~~~~~~~~~~

   .. py:method:: to_perms()

      Convert to a list of permutation vectors.

      Each permutation is returned as a list of integers [1..n].

      :return: A list of permutation vectors.
      :rtype: list[list[int]]

   .. py:staticmethod:: from_permutation(perm)

      Create a RotPiDD containing a single permutation. Equivalent to
      ``rotpidd_from_perm(perm.to_list())``.

      :param Permutation perm: A :class:`kyotodd.Permutation` in one-line
                               notation. Every position it moves must be
                               ``<= rotpidd_var_used()``.
      :return: A RotPiDD containing exactly ``perm``.
      :rtype: RotPiDD
      :raises ValueError: If ``perm`` moves a position beyond
                          ``rotpidd_var_used()``.

   .. py:method:: to_permutations()

      Convert to a list of :class:`kyotodd.Permutation` objects, in the same
      order as :meth:`to_perms`. Each Permutation has size
      ``rotpidd_var_used()``.

      ``RotPiDD.from_permutation(p) * RotPiDD.from_permutation(q) ==
      RotPiDD.from_permutation(p * q)``: both ``*`` apply the left operand
      first.

      :return: A list of Permutation objects (empty for the empty set).
      :rtype: list[Permutation]

   .. py:method:: count()

      Count the number of permutations in the set (floating-point).

      :return: The number of permutations as a float.
      :rtype: float

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

      Print RotPiDD statistics and return as string.

      :rtype: str

   .. py:method:: enum()

      Enumerate all permutations in vector notation and return as string.

      :rtype: str

   .. py:method:: enum2()

      Enumerate all permutations in rotation notation and return as string.

      :rtype: str

   Optimization
   ~~~~~~~~~~~~

   .. py:method:: contradiction_maximization(n, w)

      Find the maximum weighted contradiction value using memoized
      recursion over the ZDD structure.

      :param int n: Permutation size.
      :param list[list[int]] w: Weight matrix of size (n+1) x (n+1).
      :return: The maximum contradiction value.
      :rtype: int

Example
-------

.. code-block:: python

   import kyotodd

   kyotodd.init()

   # Set up permutations of size 4
   for _ in range(4):
       kyotodd.rotpidd_newvar()

   # Create single permutations from vectors
   p1 = kyotodd.rotpidd_from_perm([2, 3, 1, 4])
   p2 = kyotodd.rotpidd_from_perm([4, 1, 2, 3])

   # Union of permutations
   both = p1 + p2
   assert both.exact_count == 2

   # Composition
   composed = p2 * p1
   print(composed.to_perms())  # [[4, 2, 3, 1]]

   # Inverse
   inv = p1.inverse()
   identity = kyotodd.RotPiDD(1)
   assert p1 * inv == identity

   # Build all 24 permutations of [1,2,3,4] and filter
   from itertools import permutations
   all24 = kyotodd.RotPiDD(0)
   for p in permutations(range(1, 5)):
       all24 += kyotodd.rotpidd_from_perm(list(p))
   assert all24.exact_count == 24

   # Even permutations (alternating group A4, order 12)
   a4 = all24.even()
   assert a4.exact_count == 12

   # Permutations in which the value 1 precedes the value 2
   ordered = all24.order(1, 2)
   assert ordered.exact_count == 12
