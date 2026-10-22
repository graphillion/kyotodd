RotPiDD Operations
==================

RotPiDD (Rotational Permutation Decision Diagram) represents sets of
permutations on a ZDD, using left rotations LeftRot(x, y) as decomposition
variables. This is an alternative to PiDD, which uses adjacent transpositions.

The left rotation LeftRot(u, v) (with u > v) cyclically shifts positions
v, v+1, ..., u to the left by one.

Header: ``#include "rotpidd.h"``

Variable Management
-------------------

.. cpp:function:: int RotPiDD_NewVar()

   Extend the permutation size by 1.

   Creates the necessary BDD variables for the new rotations involving
   the new element. Call *n* times for permutations of size *n*.

   :return: The new permutation size (1-indexed).

.. cpp:function:: int RotPiDD_VarUsed()

   Return the current permutation size.

   :return: The number of RotPiDD variables created so far.

Level Conversion Macros
-----------------------

.. c:macro:: RotPiDD_X_Lev(lev)

   Return the x coordinate for a given BDD level.

.. c:macro:: RotPiDD_Y_Lev(lev)

   Return the y coordinate for a given BDD level.

.. c:macro:: RotPiDD_Lev_XY(x, y)

   Return the BDD level for coordinates (x, y).

.. c:macro:: RotPiDD_Y_YUV(y, u, v)

   Return v if y == u, u if y == v, otherwise y.
   Used in cofactor and rotation algebra.

.. c:macro:: RotPiDD_U_XYU(x, y, u)

   Return y if x == u, otherwise u.
   Used in cofactor computation.

Conversion Functions
--------------------

.. cpp:function:: static RotPiDD RotPiDD::VECtoRotPiDD(std::vector<int> v)
                  static RotPiDD RotPiDD::from_vector(std::vector<int> v)
                  static RotPiDD RotPiDD::from_vector(std::vector<int> v, BddExecMode mode)

   Convert a permutation vector to a RotPiDD containing that single
   permutation. The input is automatically normalized to [1..n].

   :param v: Permutation vector (values can be arbitrary integers).
   :return: A RotPiDD encoding the permutation.

.. cpp:function:: static void RotPiDD::normalizePerm(std::vector<int>& v)

   Normalize a permutation vector to [1..n] preserving relative order.

   :param v: Vector to normalize (modified in-place).

.. cpp:function:: static RotPiDD RotPiDD::from_permutation(const Permutation& p)
                  static RotPiDD RotPiDD::from_permutation(const Permutation& p, BddExecMode mode)

   Convert a :cpp:class:`Permutation` to a RotPiDD containing it. Equivalent
   to ``from_vector(p.to_vector())``; ``mode`` is forwarded to the
   ``left_rot`` calls. The variables for the moved positions must already
   exist (``RotPiDD_NewVar()``).

   :param p: The permutation (one-line notation).
   :return: A RotPiDD containing the single permutation.
   :throws std::invalid_argument: if a moved position exceeds
      ``RotPiDD_VarUsed()``.

RotPiDD Class
-------------

.. cpp:class:: RotPiDD

   A set of permutations represented on a ZDD using left rotations.

   **Constructors**

   .. cpp:function:: RotPiDD()

      Default constructor. Creates an empty set.

   .. cpp:function:: explicit RotPiDD(int a)

      Construct from integer. 0 = empty set, 1 = {identity}, -1 = null.

   .. cpp:function:: RotPiDD(const RotPiDD& f)

      Copy constructor.

   .. cpp:function:: RotPiDD(RotPiDD&& f)

      Move constructor.

   .. cpp:function:: explicit RotPiDD(const ZDD& zbdd)

      Construct from a ZDD directly.

   **Set Operations (inline friends)**

   .. cpp:function:: friend RotPiDD operator&(const RotPiDD& p, const RotPiDD& q)

      Intersection.

   .. cpp:function:: friend RotPiDD operator+(const RotPiDD& p, const RotPiDD& q)

      Union.

   .. cpp:function:: friend RotPiDD operator-(const RotPiDD& p, const RotPiDD& q)

      Difference.

   .. cpp:function:: friend RotPiDD operator*(const RotPiDD& p, const RotPiDD& q)

      Composition. Computes { pi . sigma | sigma in p, pi in q }.
      Decomposes on q's top variable and applies LeftRot recursively.

   .. cpp:function:: RotPiDD multiply(const RotPiDD& q) const
                     RotPiDD multiply(const RotPiDD& q, BddExecMode mode) const

      Named form of ``operator*`` (composition). The second overload
      selects the execution mode (``Auto`` / ``Recursive`` / ``Iterative``);
      ``operator*`` uses ``Auto``.

   **Compound Assignment**

   .. cpp:function:: RotPiDD& operator&=(const RotPiDD& q)
                     RotPiDD& operator+=(const RotPiDD& q)
                     RotPiDD& operator-=(const RotPiDD& q)
                     RotPiDD& operator*=(const RotPiDD& q)

      In-place variants of the above operators.

   **Comparison**

   .. cpp:function:: friend bool operator==(const RotPiDD& p, const RotPiDD& q)

      Equality comparison.

   .. cpp:function:: friend bool operator!=(const RotPiDD& p, const RotPiDD& q)

      Inequality comparison.

   **Core Operations**

   .. cpp:function:: RotPiDD LeftRot(int u, int v) const

      Apply left rotation LeftRot(u, v) to each permutation in the set.

      Cyclically shifts positions v, v+1, ..., u to the left. The four
      cases of the recursive decomposition handle the relationship
      between the rotation interval [v..u] and the current decomposition
      variable's interval [y..x].

      :param u: Upper position (1-indexed).
      :param v: Lower position (1-indexed).
      :return: A RotPiDD with the rotation applied.

   .. cpp:function:: RotPiDD left_rot(int u, int v, BddExecMode mode) const

      Same as ``left_rot(u, v)`` with an explicit execution mode. ``Auto``
      picks the recursive implementation when the internal ZDD's level
      is at most ``BDD_RecurLimit`` and the explicit-stack implementation
      otherwise; ``Recursive`` / ``Iterative`` force one of them. Every
      ``BddExecMode`` overload below follows the same rule.

   .. cpp:function:: RotPiDD Swap(int a, int b) const

      Swap positions *a* and *b* in all permutations.

      Implemented by composing left rotations.

      :param a: First position.
      :param b: Second position.

   .. cpp:function:: RotPiDD swap(int a, int b, BddExecMode mode) const

      Same as ``swap(a, b)``; the mode is forwarded to the underlying
      ``left_rot`` / ``multiply`` calls.

   .. cpp:function:: RotPiDD Reverse(int l, int r) const

      Reverse the subarray at positions *l* through *r* in all permutations.

      :param l: Left position.
      :param r: Right position.

   .. cpp:function:: RotPiDD reverse(int l, int r, BddExecMode mode) const

      Same as ``reverse(l, r)``; the mode is forwarded to the underlying
      ``left_rot`` / ``multiply`` calls.

   .. cpp:function:: RotPiDD Cofact(int u, int v) const

      Extract permutations where sigma(u) = v, returning sub-permutations
      with position u removed. Each selected permutation becomes a
      permutation on one fewer element: position *u* and value *v* are
      deleted, and positions above *u* and values above *v* shift down by
      one. For the top variable (x, y) of a RotPiDD this is exactly its
      1-child. Example: ``{[1 5 3 2 4]}.cofact(2, 5) == {[1 3 2 4]}``.

      :param u: Position to check.
      :param v: Required value.

   .. cpp:function:: RotPiDD cofact(int u, int v, BddExecMode mode) const

      Same as ``cofact(u, v)`` with an explicit execution mode.

   **Parity**

   .. cpp:function:: RotPiDD Odd() const

      Extract odd permutations from the set.

   .. cpp:function:: RotPiDD odd(BddExecMode mode) const

      Same as ``odd()`` with an explicit execution mode.

   .. cpp:function:: RotPiDD Even() const

      Extract even permutations. Computed as ``*this - Odd()``.

   .. cpp:function:: RotPiDD even(BddExecMode mode) const

      Same as ``even()`` with an explicit execution mode (forwarded to
      ``odd``).

   **Filtering**

   .. cpp:function:: RotPiDD RotBound(int n) const

      Apply symmetric constraint via ``ZDD::PermitSym(n)``.

   .. cpp:function:: RotPiDD Order(int a, int b) const

      Extract permutations where pi(a) < pi(b).

      :param a: First position.
      :param b: Second position.

   .. cpp:function:: RotPiDD order(int a, int b, BddExecMode mode) const

      Same as ``order(a, b)`` with an explicit execution mode.

   **Transformations**

   .. cpp:function:: RotPiDD Inverse() const

      Compute the inverse of each permutation.
      Returns { sigma^{-1} | sigma in the set }.

   .. cpp:function:: RotPiDD inverse(BddExecMode mode) const

      Same as ``inverse()`` with an explicit execution mode.

   .. cpp:function:: RotPiDD Insert(int p, int v) const

      Insert value *v* at position *p* in each permutation.

      :param p: Insertion position.
      :param v: Value to insert.

   .. cpp:function:: RotPiDD insert(int p, int v, BddExecMode mode) const

      Same as ``insert(p, v)`` with an explicit execution mode.

   .. cpp:function:: RotPiDD RemoveMax(int k) const

      Remove variables with x >= *k*, projecting down to sub-permutations.

      :param k: Threshold.

   .. cpp:function:: RotPiDD remove_max(int k, BddExecMode mode) const

      Same as ``remove_max(k)`` with an explicit execution mode.

   .. cpp:function:: RotPiDD normalizeRotPiDD(int k) const

      Remove variables with x > *k* by discarding rotation labels.

      :param k: Upper bound for retained variables.

   .. cpp:function:: RotPiDD normalizeRotPiDD(int k, BddExecMode mode) const

      Same as ``normalizeRotPiDD(k)`` with an explicit execution mode.

   **Query Methods**

   .. cpp:function:: int TopX() const

      The x coordinate of the top variable.

   .. cpp:function:: int TopY() const

      The y coordinate of the top variable.

   .. cpp:function:: int TopLev() const

      The BDD level of the top variable.

   .. cpp:function:: uint64_t Size() const

      The number of ZDD nodes.

   .. cpp:function:: uint64_t card() const
                     uint64_t Card() const

      The number of permutations in the set.

      Deprecated: use ``count()`` or ``exact_count()`` instead. Both forms
      wrap the obsolete recursive ``bddcard``, so the result saturates at
      2^39 - 1 and a set deeper than ``BDD_RecurLimit`` throws
      ``std::overflow_error``.

      .. note:: Not available in the Python API. Use ``RotPiDD.exact_count`` instead.

   .. cpp:function:: double count() const

      The number of permutations in the set as a ``double`` approximation.
      Unlike ``card()`` it never saturates, and it works on sets deeper
      than ``BDD_RecurLimit``.

   .. cpp:function:: bigint::BigInt exact_count() const

      The number of permutations in the set with arbitrary precision. Also
      safe on sets deeper than ``BDD_RecurLimit``.

   .. cpp:function:: ZDD GetZDD() const

      Return the internal ZDD.

   **Extraction and Conversion**

   .. cpp:function:: RotPiDD Extract_One()

      Extract a single permutation from the set.

   .. cpp:function:: RotPiDD extract_one(BddExecMode mode) const

      Same as ``extract_one()`` with an explicit execution mode.

   .. cpp:function:: std::vector<std::vector<int>> RotPiDDToVectorOfPerms() const

      Convert to a vector of permutation vectors.

   .. cpp:function:: std::vector<std::vector<int>> to_vector_of_perms(BddExecMode mode) const

      Same as ``to_vector_of_perms()`` with an explicit execution mode.

   .. cpp:function:: std::vector<Permutation> to_permutations() const
                     std::vector<Permutation> to_permutations(BddExecMode mode) const

      Convert to a list of :cpp:class:`Permutation` objects, in the same
      order as ``to_vector_of_perms()``. Each Permutation has size
      ``RotPiDD_VarUsed()``.

      ``RotPiDD::from_permutation(p) * RotPiDD::from_permutation(q) ==
      RotPiDD::from_permutation(p * q)``: both ``operator*`` apply the left
      operand first.

   **Output**

   .. cpp:function:: void Print() const

      Print the internal ZDD structure to stdout.

   .. cpp:function:: void Enum() const

      Enumerate all permutations in vector notation
      (e.g., ``[2 3 1]``).

   .. cpp:function:: void enumerate(BddExecMode mode) const

      Same as ``enumerate()`` with an explicit execution mode.

   .. cpp:function:: void Enum2() const

      Enumerate all permutations in rotation notation
      (e.g., ``(3:1)(2:1)``).

   .. cpp:function:: void enumerate2(BddExecMode mode) const

      Same as ``enumerate2()`` with an explicit execution mode.

   **Optimization**

   .. cpp:function:: long long int contradictionMaximization(\
         unsigned long long int used_set, \
         std::vector<int>& unused_list, \
         int n, \
         std::unordered_map<std::pair<bddp, unsigned long long int>, long long int, hash_func>& hash, \
         const std::vector<std::vector<int>>& w) const

      Find the maximum weight permutation using memoized recursion over
      the ZDD structure.

      :param used_set: Bitmask of used elements.
      :param unused_list: List of unused elements (modified during recursion).
      :param n: Current element count.
      :param hash: Memoization table.
      :param w: Weight matrix. w[i][j] is the weight between elements i and j.
      :return: The maximum weight achievable.

   .. cpp:type:: contmax_hash

      The memoization table type used by ``contradictionMaximization``
      (``std::unordered_map<std::pair<bddp, unsigned long long int>, long long int, hash_func>``).

   .. cpp:function:: long long int contradictionMaximization(\
         unsigned long long int used_set, \
         std::vector<int>& unused_list, \
         int n, \
         contmax_hash& hash, \
         const std::vector<std::vector<int>>& w, \
         BddExecMode mode) const

      Same as above with an explicit execution mode.
