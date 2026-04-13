SeqBDD Class
============

SeqBDD (Sequence BDD) represents sets of sequences using ZDDs,
where the same symbol may appear multiple times in a sequence.

SeqBDD Class
------------

.. py:class:: kyotodd.SeqBDD(val=0)

   A Sequence BDD representing a set of sequences.

   Uses ZDD internally to compactly represent ordered sequences
   where the same symbol may appear multiple times.

   :param int val: Initial value. 0 for empty set, 1 for the set
                   containing only the empty sequence (epsilon),
                   negative for null.

   Set Operators
   ~~~~~~~~~~~~~

   .. py:method:: __add__(other)

      Union: ``self + other``.

      :param SeqBDD other: Right operand.
      :rtype: SeqBDD

   .. py:method:: __sub__(other)

      Difference: ``self - other``.

      :param SeqBDD other: Right operand.
      :rtype: SeqBDD

   .. py:method:: __and__(other)

      Intersection: ``self & other``.

      :param SeqBDD other: Right operand.
      :rtype: SeqBDD

   .. py:method:: __mul__(other)

      Concatenation: ``self * other``.

      Computes the set of all sequences formed by appending a sequence
      from *other* to a sequence from *self*.

      :param SeqBDD other: Right operand.
      :rtype: SeqBDD

   .. py:method:: __truediv__(other)

      Left quotient: ``self / other``.

      Strips prefixes from *other* off matching sequences in *self*.

      :param SeqBDD other: Right operand.
      :rtype: SeqBDD
      :raises ValueError: If *other* is the empty set.

   .. py:method:: __mod__(other)

      Left remainder: ``self % other``.

      :param SeqBDD other: Right operand.
      :rtype: SeqBDD

   .. py:method:: __iadd__(other)
                   __isub__(other)
                   __iand__(other)
                   __imul__(other)
                   __itruediv__(other)
                   __imod__(other)

      In-place variants of the above operators.

   .. py:method:: __eq__(other)

      Equality comparison.

   .. py:method:: __ne__(other)

      Inequality comparison.

   .. py:method:: __hash__()

      Hash based on internal ZDD node ID.

   .. py:method:: __repr__()

      Return string representation: ``SeqBDD: id=...``.

   .. py:method:: __bool__()

      Always raises :exc:`TypeError`.

      SeqBDD cannot be converted to bool.

      :raises TypeError: Always.

   .. py:method:: __str__()

      Return human-readable representation of all sequences.
      Sequences are separated by ``" + "``. The empty sequence is
      shown as ``e``. Each symbol is displayed as its level value.

   Node Operations
   ~~~~~~~~~~~~~~~

   .. py:method:: offset(v)

      Remove all sequences whose first element is *v*.

      :param int v: Variable number.
      :return: A SeqBDD without sequences starting with *v*.
      :rtype: SeqBDD

   .. py:method:: onset0(v)

      Extract sequences starting with *v* and strip the leading *v*.

      :param int v: Variable number.
      :return: A SeqBDD of suffixes after removing the leading *v*.
      :rtype: SeqBDD

   .. py:method:: onset(v)

      Extract sequences starting with *v*, keeping *v*.

      :param int v: Variable number.
      :return: A SeqBDD of sequences that start with *v*.
      :rtype: SeqBDD

   .. py:method:: push(v)

      Prepend variable *v* to every sequence in the set.

      :param int v: Variable number to prepend.
      :return: A SeqBDD with *v* at the front of every sequence.
      :rtype: SeqBDD

   Properties
   ~~~~~~~~~~

   .. py:property:: top
      :type: int

      The variable number of the root node (0 for terminals).

   .. py:property:: exact_count
      :type: int

      The number of sequences in the set.

   .. py:property:: size
      :type: int

      The number of nodes in the internal ZDD.

   .. py:property:: lit
      :type: int

      Total symbol count across all sequences.

   .. py:property:: len
      :type: int

      Length of the longest sequence.

   .. py:property:: zdd
      :type: ZDD

      The internal ZDD representation.

   I/O
   ~~~

   .. py:method:: export_str()

      Export the internal ZDD in Sapporo format to a string.

      :rtype: str

   .. py:method:: export_file(path)

      Export the internal ZDD in Sapporo format to a file.

      :param str path: File path to write to.

   .. py:method:: print_seq()

      Print all sequences and return as string.

      :rtype: str

   .. py:method:: seq_str()

      Get all sequences as a string. Equivalent to ``str(self)``.

      :rtype: str

   Static Methods
   ~~~~~~~~~~~~~~

   .. py:staticmethod:: from_list(vars)

      Create a SeqBDD representing a single sequence.

      :param list[int] vars: Variable numbers for the sequence, in order.
      :return: A SeqBDD containing the single sequence.
      :rtype: SeqBDD

Example
-------

.. code-block:: python

   import kyotodd

   kyotodd.init()

   # Create 4 variables (symbols a, b, c, d)
   a = kyotodd.new_var()
   b = kyotodd.new_var()
   c = kyotodd.new_var()
   d = kyotodd.new_var()

   # Build single sequences
   ab = kyotodd.SeqBDD.from_list([a, b])
   bd = kyotodd.SeqBDD.from_list([b, d])
   sc = kyotodd.SeqBDD.from_list([c])

   # Union: set of sequences {ab, c}
   lhs = ab + sc

   # Union with epsilon: set {bd, epsilon}
   rhs = bd + kyotodd.SeqBDD(1)

   # Concatenation: {ab, c} * {bd, epsilon} = {ab, abbd, c, cbd}
   result = lhs * rhs
   assert result.exact_count == 4
   print(result)  # prints level values of sequences

   # Left quotient: strip prefix
   prefix = kyotodd.SeqBDD.from_list([a])
   quotient = result / prefix
   # quotient contains suffixes of sequences starting with 'a'

   # Remainder identity: f == p * (f / p) + (f % p)
   remainder = result % prefix
   assert result == prefix * quotient + remainder
