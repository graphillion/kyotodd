Permutation Class
=================

``kyotodd.Permutation`` is a plain value class (not a decision diagram) that
holds a single permutation in one-line notation. It is the element type that
:class:`kyotodd.PiDD` and :class:`kyotodd.RotPiDD` convert to and from through
``from_permutation()`` and ``to_permutations()``.

Conventions
-----------

- **One-line notation, 1-based.** ``Permutation([2, 3, 1])`` maps
  1 -> 2, 2 -> 3 and 3 -> 1. This is the notation used by
  ``rotpidd_from_perm()`` and ``RotPiDD.to_perms()``.
- **Finite support.** Positions beyond ``len(p)`` are fixed points, so
  ``p(i) == i`` for ``i > len(p)``. ``==``, ``<`` and ``hash()`` ignore
  trailing fixed points: ``Permutation([2, 1]) == Permutation([2, 1, 3])``.
- **Composition applies the left operand first.** ``(p * q)(i) == q(p(i))``.
  This is the convention of ``PiDD.__mul__`` and ``RotPiDD.__mul__``, so
  ``PiDD.from_permutation(p) * PiDD.from_permutation(q) ==
  PiDD.from_permutation(p * q)`` (and likewise for RotPiDD).

Example
-------

.. code-block:: python

   import kyotodd
   from kyotodd import Permutation, PiDD

   kyotodd.init()
   for _ in range(3):
       kyotodd.pidd_newvar()

   p = Permutation([2, 3, 1])
   q = Permutation([1, 3, 2])
   pq = p * q                      # Permutation([3, 2, 1]): q(p(1)) = q(2) = 3, ...

   f = PiDD.from_permutation(p) * PiDD.from_permutation(q)
   assert f == PiDD.from_permutation(pq)
   assert f.to_permutations() == [pq]
   assert pq.is_odd() and pq.inverse() == pq

Permutation Class
-----------------

.. py:class:: kyotodd.Permutation(values=None)

   A single permutation in one-line notation (see the conventions above).

   :param list[int] values: The images ``[p(1), ..., p(n)]``, a
                            rearrangement of ``1..n``. Omit it for the
                            identity permutation of size 0.
   :raises ValueError: If ``values`` is not a rearrangement of ``1..n``.

   .. py:staticmethod:: identity(n)

      The identity permutation of size ``n``.

      :param int n: Number of positions (``n >= 0``).
      :rtype: Permutation

   .. py:method:: __len__()

      The number of stored positions ``n``.

      :rtype: int

   .. py:method:: __call__(i)

      The image ``p(i)`` of the 1-based position ``i``; ``i`` itself when
      ``i > len(p)``.

      :param int i: A position ``>= 1``.
      :rtype: int
      :raises IndexError: If ``i < 1``.

   .. py:method:: to_list()

      The one-line notation as a list ``[p(1), ..., p(n)]``.

      :rtype: list[int]

   .. py:method:: inverse()

      The inverse permutation (same size).

      :rtype: Permutation

   .. py:method:: __mul__(other)

      Composition ``self * other``: apply ``self`` first, then ``other``;
      ``(self * other)(i) == other(self(i))``. The result has size
      ``max(len(self), len(other))``.

      :param Permutation other: Right operand.
      :rtype: Permutation

   .. py:method:: is_even()
                  is_odd()

      Parity: whether the permutation is a product of an even (odd) number
      of transpositions.

      :rtype: bool

   .. py:method:: __eq__(other)
                  __ne__(other)

      Equality as permutations; trailing fixed points are ignored.

   .. py:method:: __lt__(other)

      Lexicographic order of ``p(1), p(2), ...`` (the shorter operand is
      extended with fixed points), so permutations can be sorted.

   .. py:method:: __hash__()

      Hash consistent with ``==``; permutations can be dictionary keys and
      set members.

   .. py:method:: __repr__()

      ``Permutation([2, 3, 1])``.

   .. py:method:: __str__()

      One-line notation, e.g. ``"[2 3 1]"``.
