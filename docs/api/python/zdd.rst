ZDD Class
=========

.. py:class:: kyotodd.ZDD(val=0)

   A Zero-suppressed Decision Diagram representing a family of sets.

   Each ZDD object holds a root node ID and is automatically protected
   from garbage collection during its lifetime.

   :param int val: Initial value. 0 for empty family, 1 for unit family {empty set},
                   negative for null.

   Constants
   ---------

   .. py:attribute:: empty
      :type: ZDD

      Empty family (no sets) (class attribute).

   .. py:attribute:: single
      :type: ZDD

      Unit family containing only the empty set (class attribute).

   .. py:attribute:: null
      :type: ZDD

      Null (error) ZDD (class attribute).

   Set Operators
   -------------

   .. py:method:: __add__(other)

      Union: ``self + other``.

      :param ZDD other: Right operand.
      :rtype: ZDD

   .. py:method:: __sub__(other)

      Subtraction (set difference): ``self - other``.

      :param ZDD other: Right operand.
      :rtype: ZDD

   .. py:method:: __and__(other)

      Intersection: ``self & other``.

      :param ZDD other: Right operand.
      :rtype: ZDD

   .. py:method:: __xor__(other)

      Symmetric difference: ``self ^ other``.

      :param ZDD other: Right operand.
      :rtype: ZDD

   Algebraic Operators
   -------------------

   .. py:method:: __mul__(other)

      Join (cross product with union of elements): ``self * other``.

      :param ZDD other: Right operand.
      :rtype: ZDD

   .. py:method:: __truediv__(other)

      Division (quotient): ``self / other``.

      :param ZDD other: Right operand.
      :rtype: ZDD

   .. py:method:: __mod__(other)

      Remainder: ``self % other``.

      :param ZDD other: Right operand.
      :rtype: ZDD

   .. py:method:: __iadd__(other)
                   __isub__(other)
                   __iand__(other)
                   __ixor__(other)
                   __imul__(other)
                   __itruediv__(other)
                   __imod__(other)

      In-place variants of the above operators.

   .. py:method:: __eq__(other)

      Equality comparison by node ID.

   .. py:method:: __ne__(other)

      Inequality comparison by node ID.

   .. py:method:: __hash__()

      Hash based on node ID.

   .. py:method:: __repr__()

      Return string representation: ``ZDD(node_id=...)``.

   Selection Operations
   --------------------

   .. py:method:: change(v)

      Toggle membership of variable *v* in all sets.

      For each set S in the family, if *v* is in S then remove it,
      otherwise add it.

      :param int v: Variable number.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: offset(v)

      Select sets NOT containing variable *v*.

      :param int v: Variable number.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: onset(v)

      Select sets containing variable *v*, then remove *v*.

      :param int v: Variable number.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: onset0(v)

      Select sets NOT containing variable *v* (same as offset).

      :param int v: Variable number.
      :return: The resulting ZDD.
      :rtype: ZDD

   Cross-Product Operations
   ------------------------

   .. py:method:: disjoin(g)

      Disjoint product.

      For each pair (A, B) where A is in this family and B is in *g*,
      if A and B are disjoint, include A | B in the result.

      :param ZDD g: The other family.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: jointjoin(g)

      Joint join.

      For each pair (A, B) where A is in this family and B is in *g*,
      include A | B in the result (regardless of overlap).

      :param ZDD g: The other family.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: delta(g)

      Delta operation (symmetric difference of elements within pairs).

      For each pair (A, B) where A is in this family and B is in *g*,
      include the symmetric difference of A and B in the result.

      :param ZDD g: The other family.
      :return: The resulting ZDD.
      :rtype: ZDD

   Filtering Operations
   --------------------

   .. py:method:: restrict(g)

      Restrict to sets that are subsets of some set in *g*.

      :param ZDD g: The constraining family.
      :return: The restricted ZDD.
      :rtype: ZDD

   .. py:method:: permit(g)

      Keep sets whose elements are all permitted by *g*.

      :param ZDD g: The permitting family.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: nonsup(g)

      Remove sets that are supersets of some set in *g*.

      :param ZDD g: The constraining family.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: nonsub(g)

      Remove sets that are subsets of some set in *g*.

      :param ZDD g: The constraining family.
      :return: The resulting ZDD.
      :rtype: ZDD

   Unary Operations
   ----------------

   .. py:method:: maximal()

      Extract maximal sets (no proper superset in the family).

      :return: A ZDD containing only the maximal sets.
      :rtype: ZDD

   .. py:method:: minimal()

      Extract minimal sets (no proper subset in the family).

      :return: A ZDD containing only the minimal sets.
      :rtype: ZDD

   .. py:method:: minhit()

      Compute minimum hitting sets.

      A hitting set intersects every set in the family. Returns the
      family of all inclusion-minimal hitting sets.

      :return: A ZDD of minimal hitting sets.
      :rtype: ZDD

   .. py:method:: closure()

      Compute the downward closure.

      Returns all subsets of sets in the family.

      :return: A ZDD representing the downward closure.
      :rtype: ZDD

   Counting
   --------

   .. py:property:: card
      :type: int

      The number of sets in the family (cardinality).

   I/O
   ---

   .. py:method:: export_str()

      Export this ZDD to a string representation.

      :return: The serialized ZDD.
      :rtype: str

   .. py:method:: export_file(path)

      Export this ZDD to a file.

      :param str path: File path to write to.

   .. py:staticmethod:: import_str(s)

      Import a ZDD from a string.

      :param str s: The serialized ZDD string.
      :return: The reconstructed ZDD.
      :rtype: ZDD
      :raises RuntimeError: If import fails.

   .. py:staticmethod:: import_file(path)

      Import a ZDD from a file.

      :param str path: File path to read from.
      :return: The reconstructed ZDD.
      :rtype: ZDD
      :raises RuntimeError: If import fails or file cannot be opened.

   Properties
   ----------

   .. py:property:: node_id
      :type: int

      The raw node ID of this ZDD.

   .. py:property:: top_var
      :type: int

      The top (root) variable number of this ZDD.
