QDD Class
=========

QDD (Quasi-reduced Decision Diagram) does not apply the jump rule,
so every path from root to terminal visits every variable level exactly
once. Uses BDD complement edge semantics.

QDD Class
---------

.. py:class:: kyotodd.QDD(val=0)

   A Quasi-reduced Decision Diagram.

   :param int val: Initial value. 0 for false, 1 for true, negative for null.

   Static Constants
   ~~~~~~~~~~~~~~~~

   .. py:attribute:: false_
      :type: QDD

      Constant false (0-terminal) QDD.

   .. py:attribute:: true_
      :type: QDD

      Constant true (1-terminal) QDD.

   .. py:attribute:: null
      :type: QDD

      Null (error) QDD.

   Operators
   ~~~~~~~~~

   .. py:method:: __invert__()

      Complement (negate): ``~self``. O(1) via complement edge toggle.

      :rtype: QDD

   .. py:method:: __eq__(other)
   .. py:method:: __ne__(other)
   .. py:method:: __hash__()

   .. py:method:: __repr__()

      Return string representation: ``QDD(node_id=...)``.

   .. py:method:: __bool__()

      Always raises :exc:`TypeError`.

      QDD cannot be converted to bool. Use ``== QDD.false_`` or
      ``== QDD.true_`` instead.

      :raises TypeError: Always.

   Node Creation
   ~~~~~~~~~~~~~

   .. py:staticmethod:: getnode(var, lo, hi)

      Create a QDD node with level validation.

      Children must be at the expected level (var's level - 1).
      Does NOT apply the jump rule (lo == hi nodes are preserved).

      :param int var: Variable number.
      :param QDD lo: The low (0-edge) child.
      :param QDD hi: The high (1-edge) child.
      :return: The created QDD node.
      :rtype: QDD
      :raises ValueError: if children are not at the expected level.

   Child Accessors
   ~~~~~~~~~~~~~~~

   .. py:method:: child0()

      Get the 0-child (lo) with BDD complement edge resolution.

      :rtype: QDD

   .. py:method:: child1()

      Get the 1-child (hi) with BDD complement edge resolution.

      :rtype: QDD

   .. py:method:: child(child)

      Get child by index (0 or 1) with complement edge resolution.

      :param int child: 0 or 1.
      :rtype: QDD

   .. py:method:: raw_child0()
   .. py:method:: raw_child1()
   .. py:method:: raw_child(child)

      Get raw children without complement resolution.

   Conversion
   ~~~~~~~~~~

   .. py:method:: to_bdd()

      Convert QDD to canonical BDD by applying jump rule.

      :rtype: BDD

   .. py:method:: to_zdd()

      Convert QDD to canonical ZDD.

      :rtype: ZDD

   Operation Cache
   ~~~~~~~~~~~~~~~

   .. py:staticmethod:: cache_get(op, f, g)

      Read a 2-operand cache entry.

      :param int op: Operation code (0-255).
      :param QDD f: First operand QDD.
      :param QDD g: Second operand QDD.
      :return: The cached QDD result, or ``QDD.null`` on miss.
      :rtype: QDD

   .. py:staticmethod:: cache_put(op, f, g, result)

      Write a 2-operand cache entry.

      :param int op: Operation code (0-255).
      :param QDD f: First operand QDD.
      :param QDD g: Second operand QDD.
      :param QDD result: The result QDD to cache.

   Properties
   ~~~~~~~~~~

   .. py:property:: node_id
      :type: int

      The raw node ID of this QDD.

   .. py:property:: is_terminal
      :type: bool

      True if this is a terminal node.

   .. py:property:: is_one
      :type: bool

      True if this is the 1-terminal.

   .. py:property:: is_zero
      :type: bool

      True if this is the 0-terminal.

   .. py:property:: top_var
      :type: int

      The top (root) variable number of this QDD.

   .. py:property:: raw_size
      :type: int

      The number of nodes in the DAG of this QDD.

   Binary I/O
   ~~~~~~~~~~

   .. py:method:: export_binary_str()

      Export this QDD in binary format to a bytes object.

      :rtype: bytes

   .. py:staticmethod:: import_binary_str(data, ignore_type=False)

      Import a QDD from binary format bytes.

      :param bytes data: Binary data.
      :param bool ignore_type: If True, skip dd_type validation.
      :rtype: QDD

   .. py:method:: export_binary_file(path)

      Export this QDD in binary format to a file.

      :param str path: Output file path.

   .. py:staticmethod:: import_binary_file(path, ignore_type=False)

      Import a QDD from a binary format file.

      :param str path: Input file path.
      :param bool ignore_type: If True, skip dd_type validation.
      :rtype: QDD

   .. py:staticmethod:: export_binary_multi_str(qdds)

      Export multiple QDDs in binary format to a bytes object.

      :param list[QDD] qdds: QDDs to export.
      :rtype: bytes

   .. py:staticmethod:: import_binary_multi_str(data, ignore_type=False)

      Import multiple QDDs from binary format bytes.

      :param bytes data: Binary data.
      :param bool ignore_type: If True, skip dd_type validation.
      :rtype: list[QDD]

   .. py:staticmethod:: export_binary_multi_file(qdds, path)

      Export multiple QDDs in binary format to a file.

      :param list[QDD] qdds: QDDs to export.
      :param str path: File path to write to.

   .. py:staticmethod:: import_binary_multi_file(path, ignore_type=False)

      Import multiple QDDs from a binary format file.

      :param str path: File path to read from.
      :param bool ignore_type: If True, skip dd_type validation.
      :rtype: list[QDD]

Example
-------

.. code-block:: python

   import kyotodd

   kyotodd.init()
   kyotodd.new_var()  # variable 1
   kyotodd.new_var()  # variable 2

   # Create a BDD and convert to QDD
   x1 = kyotodd.BDD.prime(1)
   q = x1.to_qdd()

   # QDD visits every level — has more nodes than BDD
   assert q.raw_size >= x1.raw_size

   # Convert back to BDD
   b = q.to_bdd()
   assert b == x1
