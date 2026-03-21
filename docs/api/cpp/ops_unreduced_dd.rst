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

      Convert a BDD to an UnreducedDD with complement expansion.
      Recursively expands all complement edges using BDD semantics
      (~(var,lo,hi) = (var,~lo,~hi)). The resulting UnreducedDD has
      no complement edges.

   .. cpp:function:: explicit UnreducedDD(const ZDD& zdd)

      Convert a ZDD to an UnreducedDD with complement expansion.
      Uses ZDD complement semantics (~(var,lo,hi) = (var,~lo,hi)).

   .. cpp:function:: explicit UnreducedDD(const QDD& qdd)

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

      Reduce to a canonical BDD. Applies BDD complement semantics and
      BDD reduction rules (jump rule: lo == hi => return lo).

      :return: The canonical BDD.
      :throws std\:\:invalid_argument: If any node has a bddnull child.

   .. cpp:function:: ZDD reduce_as_zdd() const

      Reduce to a canonical ZDD. Applies ZDD complement semantics and
      ZDD reduction rules (zero-suppression: hi == bddempty => return lo).

      :return: The canonical ZDD.
      :throws std\:\:invalid_argument: If any node has a bddnull child.

   .. cpp:function:: QDD reduce_as_qdd() const

      Reduce to a canonical QDD. Equivalent to ``reduce_as_bdd().to_qdd()``.

      :return: The canonical QDD.
      :throws std\:\:invalid_argument: If any node has a bddnull child.

   **Binary I/O**

   .. cpp:function:: void export_binary(FILE* strm) const
   .. cpp:function:: void export_binary(std::ostream& strm) const

      Export this UnreducedDD in binary format.

   .. cpp:function:: static UnreducedDD import_binary(FILE* strm)
   .. cpp:function:: static UnreducedDD import_binary(std::istream& strm)

      Import an UnreducedDD from binary format.
