QDD Operations
==============

QDD (Quasi-reduced Decision Diagram) does not apply the jump rule,
so every path from root to terminal visits every variable level exactly
once. Uses BDD complement edge semantics.

Header: ``#include "qdd.h"``

QDD Class
---------

.. cpp:class:: QDD : public DDBase

   A Quasi-reduced Decision Diagram.

   **Constructors**

   .. cpp:function:: QDD()

      Default constructor. Creates a false (0-terminal) QDD.

   .. cpp:function:: explicit QDD(int val)

      Construct from integer. 0 = false, 1 = true, negative = null.

   .. cpp:function:: QDD(const QDD& other)

      Copy constructor.

   .. cpp:function:: QDD(QDD&& other)

      Move constructor.

   **Static Factory Functions**

   .. cpp:function:: static QDD zero()

      Return a false (0-terminal) QDD.

   .. cpp:function:: static QDD one()

      Return a true (1-terminal) QDD.

   **Node Creation**

   .. cpp:function:: static bddp getnode(bddvar var, bddp lo, bddp hi)

      Create a QDD node with level validation.

      Children must be at the expected level (var's level - 1).
      Does NOT apply the jump rule (lo == hi nodes are preserved).
      Uses BDD complement edge normalization.

      :param var: Variable number.
      :param lo: The low (0-edge) child node ID.
      :param hi: The high (1-edge) child node ID.
      :return: The node ID for the (var, lo, hi) triple.
      :throws std\:\:invalid_argument: if children are not at the expected level.

   .. cpp:function:: static QDD getnode(bddvar var, const QDD& lo, const QDD& hi)

      Create a QDD node with level validation (class type version).

   .. cpp:function:: static bddp getnode_raw(bddvar var, bddp lo, bddp hi)

      Advanced: no error checking. Use with extreme caution.

   **Operators**

   .. cpp:function:: QDD operator~() const

      Complement (negate). O(1) via complement edge toggle.

   .. cpp:function:: bool operator==(const QDD& o) const
   .. cpp:function:: bool operator!=(const QDD& o) const

   **Child Accessors (static bddp versions)**

   .. cpp:function:: static bddp child0(bddp f)

      Get the 0-child (lo) with BDD complement edge resolution.

   .. cpp:function:: static bddp child1(bddp f)

      Get the 1-child (hi) with BDD complement edge resolution.

   .. cpp:function:: static bddp child(bddp f, int child)

      Get child by index (0 or 1) with BDD complement edge resolution.

   **Child Accessors (member versions)**

   .. cpp:function:: QDD raw_child0() const
   .. cpp:function:: QDD raw_child1() const
   .. cpp:function:: QDD raw_child(int child) const

      Get the raw children without complement resolution.

   .. cpp:function:: QDD child0() const
   .. cpp:function:: QDD child1() const
   .. cpp:function:: QDD child(int child) const

      Get children with complement resolution.

   **Conversion**

   .. cpp:function:: BDD to_bdd() const

      Convert QDD to canonical BDD by applying jump rule.

   .. cpp:function:: ZDD to_zdd() const

      Convert QDD to canonical ZDD.

   **Binary I/O**

   .. cpp:function:: void export_binary(FILE* strm) const
   .. cpp:function:: void export_binary(std::ostream& strm) const

      Export this QDD in binary format.

   .. cpp:function:: static QDD import_binary(FILE* strm, bool ignore_type = false)
   .. cpp:function:: static QDD import_binary(std::istream& strm, bool ignore_type = false)

      Import a QDD from binary format.

   .. cpp:function:: static void export_binary_multi(FILE* strm, const std::vector<QDD>& qdds)
   .. cpp:function:: static void export_binary_multi(std::ostream& strm, const std::vector<QDD>& qdds)

      Export multiple QDDs in binary format.

   .. cpp:function:: static std::vector<QDD> import_binary_multi(FILE* strm, bool ignore_type = false)
   .. cpp:function:: static std::vector<QDD> import_binary_multi(std::istream& strm, bool ignore_type = false)

      Import multiple QDDs from binary format.

   **Cache**

   .. cpp:function:: static QDD cache_get(uint8_t op, const QDD& f, const QDD& g)

      Read 2-operand cache and return as QDD. Returns QDD::Null on miss.

   .. cpp:function:: static void cache_put(uint8_t op, const QDD& f, const QDD& g, const QDD& result)

      Write 2-operand cache entry.

   **Constants**

   .. cpp:member:: static const QDD False

      Constant false QDD.

   .. cpp:member:: static const QDD True

      Constant true QDD.

   .. cpp:member:: static const QDD Null

      Null (error) QDD.

Free Functions
--------------

.. cpp:function:: QDD QDD_ID(bddp p)

   Wrap a raw bddp as a QDD. Validates that the node is reduced.

   :throws std\:\:invalid_argument: if the node is not reduced.
