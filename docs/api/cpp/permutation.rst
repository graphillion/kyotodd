Permutation
===========

``Permutation`` is a plain value class (not a decision diagram) that holds a
single permutation in one-line notation. It is the element type that PiDD and
RotPiDD convert to and from: see :cpp:func:`PiDD::from_permutation`,
:cpp:func:`PiDD::to_permutations`, :cpp:func:`RotPiDD::from_permutation` and
:cpp:func:`RotPiDD::to_permutations`.

Header: ``#include "permutation.h"`` (also included by ``pidd.h`` and
``rotpidd.h``)

Conventions
-----------

- **One-line notation, 1-based.** ``Permutation({2, 3, 1})`` is the permutation
  with pi(1) = 2, pi(2) = 3 and pi(3) = 1. This is the notation printed by
  ``PiDD::enumerate()`` and used by ``RotPiDD::from_vector()`` /
  ``RotPiDD::to_vector_of_perms()``.
- **Finite support.** Every position beyond ``size()`` is a fixed point, so
  ``p(i) == i`` for ``i > p.size()``. Equality, ordering and ``hash()`` ignore
  trailing fixed points: ``Permutation({2, 1}) == Permutation({2, 1, 3})``.
  This mirrors PiDD / RotPiDD, where ``PiDD(1)`` is the identity whatever the
  current variable count is.
- **Composition applies the left operand first.** ``(p * q)(i) == q(p(i))``.
  This is the convention of ``PiDD::operator*`` and ``RotPiDD::operator*``,
  so ``PiDD::from_permutation(p) * PiDD::from_permutation(q) ==
  PiDD::from_permutation(p * q)`` (and likewise for RotPiDD).

Example
-------

.. code-block:: cpp

   #include "pidd.h"

   using namespace kyotodd;

   BDD_Init(1024);
   for (int i = 0; i < 3; ++i) PiDD_NewVar();

   Permutation p({2, 3, 1});
   Permutation q({1, 3, 2});
   Permutation pq = p * q;             // [3 2 1]: q(p(1)) = q(2) = 3, ...

   PiDD f = PiDD::from_permutation(p) * PiDD::from_permutation(q);
   assert(f == PiDD::from_permutation(pq));
   std::vector<Permutation> perms = f.to_permutations();   // { [3 2 1] }
   assert(perms[0] == pq && perms[0].is_odd());

Permutation Class
-----------------

.. cpp:class:: Permutation

   A single permutation in one-line notation (see the conventions above).

   **Constructors**

   .. cpp:function:: Permutation()

      The identity permutation of size 0.

   .. cpp:function:: explicit Permutation(const std::vector<int>& v)

      Construct from one-line notation: ``v[0] = pi(1)``, ...,
      ``v[n-1] = pi(n)``.

      :param v: A rearrangement of 1..n.
      :throws std::invalid_argument: if ``v`` is not a rearrangement of 1..n.

   .. cpp:function:: static Permutation identity(int n)

      The identity permutation of size ``n``.

      :throws std::invalid_argument: if ``n < 0``.

   **Query Methods**

   .. cpp:function:: int size() const

      The number of stored positions n.

   .. cpp:function:: int operator()(int i) const

      The image pi(i) of the 1-based position ``i``; ``i`` itself when
      ``i > size()``.

      :throws std::out_of_range: if ``i < 1``.

   .. cpp:function:: const std::vector<int>& to_vector() const

      The stored one-line notation (``v[i-1] = pi(i)``).

   .. cpp:function:: std::string to_string() const

      The one-line notation as text, e.g. ``"[2 3 1]"`` (``"[]"`` for size 0).

   .. cpp:function:: bool is_even() const
                     bool is_odd() const

      Parity: whether the permutation is a product of an even (odd) number
      of transpositions.

   .. cpp:function:: std::size_t hash() const

      A hash value consistent with ``operator==`` (trailing fixed points are
      ignored).

   **Algebra**

   .. cpp:function:: Permutation inverse() const

      The inverse permutation (same size).

   .. cpp:function:: Permutation operator*(const Permutation& q) const
                     Permutation& operator*=(const Permutation& q)

      Composition: apply ``*this`` first, then ``q``; ``(p * q)(i) == q(p(i))``.
      The result has size ``max(size(), q.size())``.

Free Functions
--------------

.. cpp:function:: bool operator==(const Permutation& a, const Permutation& b)
                  bool operator!=(const Permutation& a, const Permutation& b)

   Equality as permutations; trailing fixed points are ignored.

.. cpp:function:: bool operator<(const Permutation& a, const Permutation& b)

   Lexicographic order of pi(1), pi(2), ... (the shorter operand is extended
   with fixed points). Makes ``Permutation`` usable in ``std::set`` and
   ``std::sort``.

.. cpp:function:: std::ostream& operator<<(std::ostream& os, const Permutation& p)

   Print ``p.to_string()``.
