PiDD Operations
===============

PiDD (Permutation Decision Diagram) represents sets of permutations
on a ZDD, using adjacent transpositions Swap(x, y) as decomposition
variables.

Header: ``#include "pidd.h"``

Variable Management
-------------------

.. cpp:function:: int PiDD_NewVar()

   Extend the permutation size by 1.

   Creates the necessary BDD variables for the new transpositions
   involving the new element. Call *n* times for permutations of size *n*.

   :return: The new permutation size (1-indexed).

.. cpp:function:: int PiDD_VarUsed()

   Return the current permutation size.

   :return: The number of PiDD variables created so far.

Level Conversion Macros
-----------------------

.. c:macro:: PiDD_X_Lev(lev)

   Return the x coordinate for a given BDD level.

.. c:macro:: PiDD_Y_Lev(lev)

   Return the y coordinate for a given BDD level.

.. c:macro:: PiDD_Lev_XY(x, y)

   Return the BDD level for coordinates (x, y).

PiDD Class
----------

.. cpp:class:: PiDD

   A set of permutations represented on a ZDD using adjacent transpositions.

   **Constructors**

   .. cpp:function:: PiDD()

      Default constructor. Creates an empty set.

   .. cpp:function:: explicit PiDD(int a)

      Construct from integer. 0 = empty set, 1 = {identity}, -1 = null.

   .. cpp:function:: PiDD(const PiDD& f)

      Copy constructor.

   .. cpp:function:: explicit PiDD(const ZDD& zbdd)

      Construct from a ZDD directly.

   **Set Operations (inline friends)**

   .. cpp:function:: friend PiDD operator&(const PiDD& p, const PiDD& q)

      Intersection.

   .. cpp:function:: friend PiDD operator+(const PiDD& p, const PiDD& q)

      Union.

   .. cpp:function:: friend PiDD operator-(const PiDD& p, const PiDD& q)

      Difference.

   .. cpp:function:: friend PiDD operator*(const PiDD& p, const PiDD& q)

      Composition. Computes { pi . sigma | sigma in p, pi in q }.
      Decomposes on q's top variable and applies Swap recursively.

   .. cpp:function:: friend PiDD operator/(const PiDD& f, const PiDD& p)

      Division. Finds quotient q such that q * p is a subset of f.

   .. cpp:function:: friend PiDD operator%(const PiDD& f, const PiDD& p)

      Remainder: ``f - (f / p) * p``.

   **Core Operations**

   .. cpp:function:: PiDD Swap(int u, int v) const

      Apply transposition Swap(u, v) to each permutation in the set.

      Composes the transposition that exchanges positions *u* and *v*
      from the left. Uses recursive decomposition on ZDD variables with
      operation caching.

      :param u: First position (1-indexed).
      :param v: Second position (1-indexed).
      :return: A PiDD with the transposition applied.

   .. cpp:function:: PiDD Cofact(int u, int v) const

      Extract permutations where sigma(u) = v, returning sub-permutations
      with position u removed.

      :param u: Position to check.
      :param v: Required value.
      :return: A PiDD of sub-permutations.

   .. cpp:function:: PiDD Odd() const

      Extract odd permutations from the set.

   .. cpp:function:: PiDD Even() const

      Extract even permutations. Computed as ``*this - Odd()``.

   .. cpp:function:: PiDD SwapBound(int n) const

      Apply symmetric constraint via ``ZDD::PermitSym(n)``.

      :param n: Constraint parameter.

   **Query Methods**

   .. cpp:function:: int TopX() const

      The x coordinate of the top variable.

   .. cpp:function:: int TopY() const

      The y coordinate of the top variable.

   .. cpp:function:: int TopLev() const

      The BDD level of the top variable.

   .. cpp:function:: uint64_t Size() const

      The number of ZDD nodes.

   .. cpp:function:: uint64_t Card() const

      The number of permutations in the set.

   .. cpp:function:: ZDD GetZDD() const

      Return the internal ZDD.

   **Output**

   .. cpp:function:: void Print() const

      Print the internal ZDD structure to stdout.

   .. cpp:function:: void Enum() const

      Enumerate all permutations in vector notation (e.g., ``[2 1 3]``).

   .. cpp:function:: void Enum2() const

      Enumerate all permutations in transposition notation
      (e.g., ``(2:1)(3:2)``).
