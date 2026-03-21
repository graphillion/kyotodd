UnreducedDD Class
=================

UnreducedDD is a type-agnostic unreduced Decision Diagram that does NOT
apply any reduction rules at node creation time. Complement edges are
stored raw and only gain meaning when ``reduce_as_bdd()``,
``reduce_as_zdd()``, or ``reduce_as_qdd()`` is called.

UnreducedDD Class
-----------------

.. py:class:: kyotodd.UnreducedDD(val=0)

   A type-agnostic unreduced Decision Diagram.

   :param int val: Initial value. 0 for 0-terminal, 1 for 1-terminal,
                   negative for null.

   **Complement expansion constructors:**

   .. py:method:: __init__(bdd)

      Convert a BDD to an UnreducedDD with complement expansion.
      Recursively expands all complement edges using BDD semantics.

      :param BDD bdd: Source BDD.

   .. py:method:: __init__(zdd)

      Convert a ZDD to an UnreducedDD with complement expansion.
      Recursively expands all complement edges using ZDD semantics.

      :param ZDD zdd: Source ZDD.

   .. py:method:: __init__(qdd)

      Convert a QDD to an UnreducedDD with complement expansion.
      Uses BDD complement semantics.

      :param QDD qdd: Source QDD.

   Operators
   ~~~~~~~~~

   .. py:method:: __invert__()

      Toggle complement bit (bit 0). O(1). The complement bit has no
      semantics in UnreducedDD; it is only interpreted at reduce time.

      :rtype: UnreducedDD

   .. py:method:: __eq__(other)
   .. py:method:: __ne__(other)
   .. py:method:: __lt__(other)
   .. py:method:: __hash__()

   Node Creation
   ~~~~~~~~~~~~~

   .. py:staticmethod:: getnode(var, lo, hi)

      Create an unreduced DD node.

      Always allocates a new node. No complement normalization, no
      reduction rules, no unique table insertion.

      :param int var: Variable number.
      :param UnreducedDD lo: The low (0-edge) child.
      :param UnreducedDD hi: The high (1-edge) child.
      :return: The created UnreducedDD node.
      :rtype: UnreducedDD

   Raw Wrap
   ~~~~~~~~

   .. py:staticmethod:: wrap_raw_bdd(bdd)

      Wrap a BDD's bddp directly without complement expansion.
      Only use ``reduce_as_bdd()`` on the result.

      :param BDD bdd: Source BDD.
      :rtype: UnreducedDD

   .. py:staticmethod:: wrap_raw_zdd(zdd)

      Wrap a ZDD's bddp directly without complement expansion.
      Only use ``reduce_as_zdd()`` on the result.

      :param ZDD zdd: Source ZDD.
      :rtype: UnreducedDD

   .. py:staticmethod:: wrap_raw_qdd(qdd)

      Wrap a QDD's bddp directly without complement expansion.
      Only use ``reduce_as_qdd()`` on the result.

      :param QDD qdd: Source QDD.
      :rtype: UnreducedDD

   Child Accessors
   ~~~~~~~~~~~~~~~

   .. py:method:: raw_child0()
   .. py:method:: raw_child1()
   .. py:method:: raw_child(child)

      Get the raw children without complement interpretation.
      UnreducedDD does not provide complement-resolving child accessors.

   Child Mutation (Top-down Construction)
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

   .. py:method:: set_child0(child)

      Set the 0-child (lo) of this unreduced node.

      Only valid on unreduced, non-terminal, non-complemented nodes.

      :param UnreducedDD child: New 0-child.

   .. py:method:: set_child1(child)

      Set the 1-child (hi) of this unreduced node.

      Only valid on unreduced, non-terminal, non-complemented nodes.

      :param UnreducedDD child: New 1-child.

   Reduce
   ~~~~~~

   .. py:method:: reduce_as_bdd()

      Reduce to a canonical BDD. Applies BDD complement semantics and
      jump rule (lo == hi => return lo).

      :return: The canonical BDD.
      :rtype: BDD
      :raises ValueError: If any node has a null child.

   .. py:method:: reduce_as_zdd()

      Reduce to a canonical ZDD. Applies ZDD complement semantics and
      zero-suppression rule (hi == empty => return lo).

      :return: The canonical ZDD.
      :rtype: ZDD
      :raises ValueError: If any node has a null child.

   .. py:method:: reduce_as_qdd()

      Reduce to a canonical QDD. Equivalent to ``reduce_as_bdd().to_qdd()``.

      :return: The canonical QDD.
      :rtype: QDD
      :raises ValueError: If any node has a null child.

   Properties
   ~~~~~~~~~~

   .. py:property:: node_id
      :type: int

      The raw node ID of this UnreducedDD.

   .. py:property:: is_terminal
      :type: bool

      True if this is a terminal node.

   .. py:property:: is_one
      :type: bool

      True if this is the 1-terminal.

   .. py:property:: is_zero
      :type: bool

      True if this is the 0-terminal.

   .. py:property:: is_reduced
      :type: bool

      True if this DD is fully reduced (canonical).

   .. py:property:: top_var
      :type: int

      The top (root) variable number.

   .. py:property:: raw_size
      :type: int

      The number of nodes in the DAG.

   Binary I/O
   ~~~~~~~~~~

   .. py:method:: export_binary_str()

      Export this UnreducedDD in binary format to a bytes object.

      :rtype: bytes

   .. py:staticmethod:: import_binary_str(data)

      Import an UnreducedDD from binary format bytes.

      :param bytes data: Binary data.
      :rtype: UnreducedDD

   .. py:method:: export_binary_file(path)

      Export this UnreducedDD in binary format to a file.

      :param str path: Output file path.

   .. py:staticmethod:: import_binary_file(path)

      Import an UnreducedDD from a binary format file.

      :param str path: Input file path.
      :rtype: UnreducedDD

Example
-------

.. code-block:: python

   import kyotodd

   kyotodd.init()
   kyotodd.new_var()  # variable 1
   kyotodd.new_var()  # variable 2

   # Build unreduced DD from scratch
   t = kyotodd.UnreducedDD(1)  # 1-terminal
   f = kyotodd.UnreducedDD(0)  # 0-terminal

   # Create nodes without any reduction
   n1 = kyotodd.UnreducedDD.getnode(1, t, f)
   n2 = kyotodd.UnreducedDD.getnode(2, n1, n1)  # lo == hi: no jump rule

   # Reduce as BDD (jump rule eliminates n2)
   bdd = n2.reduce_as_bdd()

   # Reduce as ZDD (zero-suppression rule)
   zdd = n2.reduce_as_zdd()
