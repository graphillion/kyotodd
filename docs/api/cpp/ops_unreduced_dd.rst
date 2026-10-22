UnreducedDD Operations
======================

UnreducedDD is a type-agnostic unreduced Decision Diagram that does NOT
apply any reduction rules at node creation time. Complement edges are
stored raw and only gain meaning when ``reduce_as_bdd()``,
``reduce_as_zdd()``, or ``reduce_as_qdd()`` is called.

Header: ``#include "unreduced_dd.h"``

UnreducedDD Class
-----------------

.. cpp:class:: UnreducedDD : public DDBase

   A type-agnostic unreduced Decision Diagram.

   **Constructors**

   .. cpp:function:: UnreducedDD()

      Default constructor. Creates a 0-terminal.

   .. cpp:function:: explicit UnreducedDD(int val)

      Construct from integer. 0 = 0-terminal, 1 = 1-terminal, negative = null.

   .. cpp:function:: UnreducedDD(const UnreducedDD& other)

      Copy constructor.

   .. cpp:function:: UnreducedDD(UnreducedDD&& other)

      Move constructor.

   .. cpp:function:: explicit UnreducedDD(const BDD& bdd)
                     UnreducedDD(const BDD& bdd, BddExecMode mode)

      Convert a BDD to an UnreducedDD with complement expansion.
      Recursively expands all complement edges using BDD semantics
      (~(var,lo,hi) = (var,~lo,~hi)). The resulting UnreducedDD has
      no complement edges. The two-argument form selects the execution
      mode (``Auto`` / ``Recursive`` / ``Iterative``).

   .. cpp:function:: explicit UnreducedDD(const ZDD& zdd)
                     UnreducedDD(const ZDD& zdd, BddExecMode mode)

      Convert a ZDD to an UnreducedDD with complement expansion.
      Uses ZDD complement semantics (~(var,lo,hi) = (var,~lo,hi)).

   .. cpp:function:: explicit UnreducedDD(const QDD& qdd)
                     UnreducedDD(const QDD& qdd, BddExecMode mode)

      Convert a QDD to an UnreducedDD with complement expansion.
      Uses BDD complement semantics.

   **Static Factory Functions**

   .. cpp:function:: static UnreducedDD zero()

      Return a 0-terminal UnreducedDD.

   .. cpp:function:: static UnreducedDD one()

      Return a 1-terminal UnreducedDD.

   **Raw Wrap (complement edges preserved)**

   .. cpp:function:: static UnreducedDD wrap_raw(const BDD& bdd)

      Wrap a BDD's bddp directly without complement expansion.
      Only use ``reduce_as_bdd()`` on the result.

   .. cpp:function:: static UnreducedDD wrap_raw(const ZDD& zdd)

      Wrap a ZDD's bddp directly without complement expansion.
      Only use ``reduce_as_zdd()`` on the result.

   .. cpp:function:: static UnreducedDD wrap_raw(const QDD& qdd)

      Wrap a QDD's bddp directly without complement expansion.
      Only use ``reduce_as_qdd()`` on the result.

   **Node Creation**

   .. cpp:function:: static UnreducedDD getnode(bddvar var, const UnreducedDD& lo, const UnreducedDD& hi)

      Create an unreduced DD node.

      Always allocates a new node. No complement normalization, no
      reduction rules, no unique table insertion.

      :param var: Variable number.
      :param lo: The low (0-edge) child.
      :param hi: The high (1-edge) child.
      :return: The created UnreducedDD node.

   .. cpp:function:: static bddp getnode_raw(bddvar var, bddp lo, bddp hi)

      Create an unreduced node from raw bddp values.

   **Child Accessors (raw only)**

   .. cpp:function:: UnreducedDD raw_child0() const
   .. cpp:function:: UnreducedDD raw_child1() const
   .. cpp:function:: UnreducedDD raw_child(int child) const

      Get the raw children without complement interpretation.

   **Child Mutation (top-down construction)**

   .. cpp:function:: void set_child0(const UnreducedDD& child)

      Set the 0-child (lo) of this unreduced node.

      Only valid on unreduced, non-terminal, non-complemented nodes.

   .. cpp:function:: void set_child1(const UnreducedDD& child)

      Set the 1-child (hi) of this unreduced node.

      Only valid on unreduced, non-terminal, non-complemented nodes.

   **Operators**

   .. cpp:function:: UnreducedDD operator~() const

      Toggle complement bit (bit 0). O(1). The complement bit has no
      semantics in UnreducedDD; it is only interpreted at reduce time.

   .. cpp:function:: bool operator==(const UnreducedDD& o) const
   .. cpp:function:: bool operator!=(const UnreducedDD& o) const
   .. cpp:function:: bool operator<(const UnreducedDD& o) const

   **Query**

   .. cpp:function:: bool is_reduced() const

      Check if this DD is fully reduced (canonical). Returns false for
      bddnull, true for terminals, and checks the reduced flag for
      non-terminal nodes.

   **Reduce**

   .. cpp:function:: BDD reduce_as_bdd() const
                     BDD reduce_as_bdd(BddExecMode mode) const

      Reduce to a canonical BDD. Applies BDD complement semantics and
      BDD reduction rules (jump rule: lo == hi => return lo). The second
      overload selects the execution mode.

      :return: The canonical BDD.
      :throws std\:\:invalid_argument: If any node has a bddnull child.

   .. cpp:function:: ZDD reduce_as_zdd() const
                     ZDD reduce_as_zdd(BddExecMode mode) const

      Reduce to a canonical ZDD. Applies ZDD complement semantics and
      ZDD reduction rules (zero-suppression: hi == bddempty => return lo).
      The second overload selects the execution mode.

      :return: The canonical ZDD.
      :throws std\:\:invalid_argument: If any node has a bddnull child.

   .. cpp:function:: QDD reduce_as_qdd() const
                     QDD reduce_as_qdd(BddExecMode mode) const

      Reduce to a canonical QDD. Equivalent to ``reduce_as_bdd().to_qdd()``
      (the mode, when given, is forwarded to both steps).

      :return: The canonical QDD.
      :throws std\:\:invalid_argument: If any node has a bddnull child.

   **Binary I/O**

   .. cpp:function:: void export_binary(FILE* strm) const
   .. cpp:function:: void export_binary(std::ostream& strm) const

      Export this UnreducedDD in binary format.

   .. cpp:function:: static UnreducedDD import_binary(FILE* strm)
   .. cpp:function:: static UnreducedDD import_binary(std::istream& strm)

      Import an UnreducedDD from binary format.

Construction from a TdZdd spec
------------------------------

Header: ``#include "unreduced_dd_tdzdd.h"`` (not included by ``bdd.h``).
Requires the `TdZdd <https://github.com/kunisura/TdZdd>`_ submodule
(``third_party/TdZdd``, fetched with ``git clone --recursive`` or
``git submodule update --init``). The header provides
``UnreducedDD::construct()`` (build a DD from a spec),
``UnreducedDD::zdd_subset()`` and ``ZDD::subset()`` (TdZdd's
``zddSubset``: restrict a family to the sets a spec accepts).

.. cpp:enum-class:: SkipPadding

   How ``UnreducedDD::construct()`` and ``UnreducedDD::zdd_subset()``
   treat edges whose child level is lower than (parent level - 1). TdZdd stores such edges as-is; their
   meaning (don't-care vs. must-be-0) is decided only when the DD is
   reduced as a BDD or a ZDD.

   .. cpp:enumerator:: None

      Store the skipping edge as-is (same as TdZdd's ``DdStructure``).

   .. cpp:enumerator:: Bdd

      Insert chain nodes with ``lo == hi == child`` (BDD don't-care).

   .. cpp:enumerator:: Zdd

      Insert chain nodes with ``lo == child``, ``hi == 0-terminal`` (ZDD).

.. cpp:function:: template<typename SPEC> static UnreducedDD UnreducedDD::construct(const tdzdd::DdSpecBase<SPEC, 2>& spec, SkipPadding pad = SkipPadding::None, int level_offset = 0)

   Build an UnreducedDD top-down from a TdZdd spec (``tdzdd::DdSpec``,
   ``StatelessDdSpec``, ``PodArrayDdSpec`` or ``HybridDdSpec``; binary
   specs only).

   The construction is breadth-first and level-by-level, equivalent to
   ``tdzdd::DdBuilder``, but nodes are allocated directly in KyotoDD's
   node array; no ``tdzdd::DdStructure`` is created. States at the same
   level are merged via the spec's ``hash_code`` / ``equal_to`` /
   ``merge_states`` (including TdZdd's handling of states that reach the
   1-terminal), so the result represents the same function as an
   unreduced ``DdStructure`` and reduces to the same BDD / ZDD. The
   node-level shape may differ: ``DdStructure`` lazily sweeps dead nodes
   (nodes whose children are all the 0-terminal, as produced by
   ``merge_states`` returning 1) once they exceed a fraction of the
   table, whereas ``construct()`` keeps every dead node; they disappear
   on reduction. States at a level are visited in the same order as
   ``tdzdd::DdBuilder`` (newest first), so even a ``merge_states`` whose
   outcome depends on which of two equal states is presented first gives
   the same result as TdZdd. Reduce it afterwards with :cpp:func:`reduce_as_bdd`,
   :cpp:func:`reduce_as_zdd` or :cpp:func:`reduce_as_qdd`.

   TdZdd level ``i`` is mapped to KyotoDD level ``i + level_offset``
   (variable ``bddvaroflev(i + level_offset)``). Missing variables are
   created automatically with ``bddnewvar()``.

   Edges to the 0-terminal are never padded. Edges to the 1-terminal are
   padded down to KyotoDD level 1, so with ``level_offset > 0`` the
   KyotoDD levels ``1 .. level_offset`` below the spec's range are
   covered too (``SkipPadding::Zdd`` forces those variables to 0,
   ``SkipPadding::Bdd`` makes them don't-care). The root is never padded
   above its own level. The implementation is an iterative level loop
   (no recursion), so there are no ``_rec`` / ``_iter`` variants and no
   ``BddExecMode`` argument.

   :param spec: The TdZdd spec.
   :param pad: How to treat level-skipping edges.
   :param level_offset: Non-negative offset added to every TdZdd level.
   :return: The constructed UnreducedDD (a terminal if the root is).
   :throws std\:\:invalid_argument: If ``level_offset`` is negative.
   :throws std\:\:logic_error: If the spec returns a child level that is
      not lower than the current level.

   Example (choose exactly ``k`` of ``n`` items, from the TdZdd user guide):

   .. code-block:: cpp

      #include "bdd.h"
      #include "unreduced_dd_tdzdd.h"

      class Combination : public tdzdd::DdSpec<Combination, int, 2> {
          int const n, k;
      public:
          Combination(int n, int k) : n(n), k(k) {}
          int getRoot(int& state) const { state = 0; return n; }
          int getChild(int& state, int level, int value) const {
              state += value;
              if (--level == 0) return (state == k) ? -1 : 0;
              if (state > k) return 0;
              if (state + level < k) return 0;
              return level;
          }
      };

      UnreducedDD u = UnreducedDD::construct(Combination(10, 4));
      ZDD z = u.reduce_as_zdd();      // z.exact_count() == 210

ZDD subsetting with a TdZdd spec
--------------------------------

.. cpp:function:: template<typename SPEC> UnreducedDD UnreducedDD::zdd_subset(const tdzdd::DdSpecBase<SPEC, 2>& spec, SkipPadding pad = SkipPadding::None, int level_offset = 0) const

   Port of TdZdd's ``DdStructure::zddSubset``: restrict this DD, read as
   a ZDD, to the sets the spec accepts, without building the spec's own
   ZDD. The result is unreduced; reduce it with :cpp:func:`reduce_as_zdd`.

   The construction is breadth-first and level-by-level, like
   :cpp:func:`construct`, but each pending state is a pair (input node,
   spec state). When the two sides sit at different levels, the 0-edge of
   the higher one is followed until the levels agree (ZDD semantics: a
   skipped level means the variable is 0), exactly as
   ``tdzdd::ZddSubsetter`` does. Only branches on which both sides reach
   the 1-terminal lead to the 1-terminal. Consequently, input variables
   outside the spec's level range (above the spec's root or below
   ``level_offset``) are forced to 0: sets containing them are dropped.

   This DD is read with ZDD complement semantics (a complement edge
   toggles the empty set) and may be unreduced (dead or duplicate nodes
   are fine), but must not contain ``bddnull`` children. The spec's
   level ``i`` corresponds to KyotoDD level ``i + level_offset``; no
   variables are created. ``merge_states`` and the 1-terminal state are
   handled as in :cpp:func:`construct`, but equal states are merged per
   input node, and the order in which equal states are presented is not
   identical to TdZdd's (which groups states by its internal node
   numbering).

   :param spec: The TdZdd spec.
   :param pad: How to treat level-skipping edges in the result.
   :param level_offset: Non-negative offset added to every TdZdd level.
   :return: The unreduced subset DD (a terminal if it collapses).
   :throws std\:\:invalid_argument: If ``level_offset`` is negative, or
      this DD is ``bddnull`` or contains ``bddnull`` children.
   :throws std\:\:logic_error: If the spec returns a child level that is
      not lower than the current level.

.. cpp:function:: template<typename SPEC> ZDD ZDD::subset(const tdzdd::DdSpecBase<SPEC, 2>& spec, int level_offset = 0) const

   Convenience wrapper (declared in ``zdd_class.h``, defined in
   ``unreduced_dd_tdzdd.h``): equivalent to
   ``UnreducedDD::wrap_raw(*this).zdd_subset(spec, SkipPadding::None,
   level_offset).reduce_as_zdd()``. The result equals the intersection of
   this family with the ZDD the spec describes, computed top-down in one
   pass.

   Example (sets of size 2 to 4 that contain item 3):

   .. code-block:: cpp

      // ChooseItem: accept exactly the sets that contain `item`.
      class ChooseItem : public tdzdd::DdSpec<ChooseItem, int, 2> {
          int const n, item;
      public:
          ChooseItem(int n, int item) : n(n), item(item) {}
          int getRoot(int& found) const { found = 0; return n; }
          int getChild(int& found, int level, int value) const {
              if (level == item && value) found = 1;
              if (--level == 0) return found ? -1 : 0;
              return level;
          }
      };

      ZDD fam = ZDD::combination(6, 2) + ZDD::combination(6, 3)
              + ZDD::combination(6, 4);
      ZDD with3 = fam.subset(ChooseItem(6, 3));   // same as fam.onset(3)
