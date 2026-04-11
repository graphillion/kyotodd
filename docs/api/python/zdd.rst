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

   .. py:method:: intersec(other)

      Intersection with another family.

      Equivalent to ``self & other``.

      :param ZDD other: The other family.
      :return: The intersection of this family and *other*.
      :rtype: ZDD

   .. py:method:: __xor__(other)

      Symmetric difference: ``self ^ other``.

      :param ZDD other: Right operand.
      :rtype: ZDD

   .. py:method:: __invert__()

      Complement: toggle empty set membership (``~self``).

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

   .. py:method:: __lshift__(s)

      Left shift: increase variable numbers by *s*. ``self << s``.

      :param int s: Number of positions to shift.
      :rtype: ZDD

   .. py:method:: __rshift__(s)

      Right shift: decrease variable numbers by *s*. ``self >> s``.

      :param int s: Number of positions to shift.
      :rtype: ZDD

   .. py:method:: __iadd__(other)
                   __isub__(other)
                   __iand__(other)
                   __ixor__(other)
                   __imul__(other)
                   __itruediv__(other)
                   __imod__(other)
                   __ilshift__(s)
                   __irshift__(s)

      In-place variants of the above operators.

   .. py:method:: __eq__(other)

      Equality comparison by node ID.

   .. py:method:: __ne__(other)

      Inequality comparison by node ID.

   .. py:method:: __hash__()

      Hash based on node ID.

   .. py:method:: __repr__()

      Return string representation: ``ZDD(node_id=...)``.

   .. py:method:: __bool__()

      Always raises :exc:`TypeError`.

      ZDD cannot be converted to bool. Use ``== ZDD.empty`` or
      ``== ZDD.single`` instead.

      :raises TypeError: Always.

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

      Select sets containing variable *v*, keeping *v* in the result.

      :param int v: Variable number.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: onset0(v)

      Select sets containing variable *v*, with *v* removed from each set (1-cofactor).

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

   .. py:method:: joint_join(g)

      Joint join.

      For each pair (A, B) where A is in this family and B is in *g*
      with A & B non-empty, include A | B in the result.
      Pairs with no overlap are excluded.

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

   .. py:method:: meet(other)

      Meet operation (intersection of all element pairs).

      For each pair of sets (one from this family, one from *other*),
      compute their intersection and collect all results.

      :param ZDD other: Another ZDD family.
      :return: The resulting ZDD.
      :rtype: ZDD

   .. py:method:: product(g)

      Cross product for disjoint variable sets.

      Like join (``*``), but more efficient when the two families operate on
      completely disjoint variable sets. Each pair (A, B) produces A ∪ B.

      :param ZDD g: The other family (must use disjoint variables).
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

   .. py:method:: choose(k)

      Filter to sets of exactly *k* elements.

      :param int k: Exact number of elements per set.
      :return: A ZDD containing only sets of size *k*.
      :rtype: ZDD

   .. py:method:: size_le(k)

      Filter to sets with at most *k* elements.

      :param int k: Maximum number of elements.
      :return: A ZDD containing only sets of size ≤ *k*.
      :rtype: ZDD

   .. py:method:: size_ge(k)

      Filter to sets with at least *k* elements.

      :param int k: Minimum number of elements.
      :return: A ZDD containing only sets of size ≥ *k*.
      :rtype: ZDD

   .. py:method:: supersets_of(s)

      Extract all sets that are supersets of *s*.

      :param list[int] s: A list of variable numbers.
      :return: A ZDD containing all sets S in the family such that s ⊆ S.
      :rtype: ZDD

   .. py:method:: subsets_of(s)

      Extract all sets that are subsets of *s*.

      :param list[int] s: A list of variable numbers.
      :return: A ZDD containing all sets S in the family such that S ⊆ s.
      :rtype: ZDD

   .. py:method:: project(vars)

      Remove specified variables from all sets (existential projection).

      :param list[int] vars: Variables to remove.
      :return: A ZDD with the specified variables projected out.
      :rtype: ZDD

   .. py:method:: coalesce(v1, v2)

      Merge variable *v2* into *v1*.

      Every set containing *v2* will have *v2* replaced by *v1*.
      Variable *v1* survives, *v2* is eliminated.

      :param int v1: The surviving variable number.
      :param int v2: The variable to merge into *v1*.
      :return: A ZDD with *v2* merged into *v1*.
      :rtype: ZDD

   .. py:method:: flatten()

      Union of all sets as a single-set ZDD.

      Collects all elements that appear in any set and returns
      a family containing only their union.

      :return: A ZDD containing a single set (the union of all sets).
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

   Variable Analysis
   -----------------

   .. py:method:: always()

      Find elements common to ALL sets in the family.

      :return: A ZDD family of singletons, one for each always-present variable.
              For example, ``{{1,2,3},{1,2}}.always()`` returns ``{{1},{2}}``.
      :rtype: ZDD

   .. py:method:: permit_sym(n)

      Symmetric permit: keep sets with at most *n* elements.

      :param int n: Maximum number of elements.
      :return: A ZDD containing only sets with <= *n* elements.
      :rtype: ZDD

   .. py:method:: swap(v1, v2)

      Swap two variables in the family.

      :param int v1: First variable number.
      :param int v2: Second variable number.
      :return: A ZDD with *v1* and *v2* swapped.
      :rtype: ZDD

   .. py:method:: imply_chk(v1, v2)

      Check if *v1* implies *v2* in the family.

      :param int v1: First variable number.
      :param int v2: Second variable number.
      :return: 1 if every set containing *v1* also contains *v2*, 0 otherwise.
      :rtype: int

   .. py:method:: coimply_chk(v1, v2)

      Check co-implication between *v1* and *v2* in the family.

      :param int v1: First variable number.
      :param int v2: Second variable number.
      :return: 1 if co-implication holds, 0 otherwise.
      :rtype: int

   .. py:method:: sym_chk(v1, v2)

      Check if two variables are symmetric in the family.

      :param int v1: First variable number.
      :param int v2: Second variable number.
      :return: 1 if symmetric, 0 if not.
      :rtype: int

   .. py:method:: imply_set(v)

      Find all variables implied by *v* in the family.

      :param int v: Variable number.
      :return: A ZDD (family of singletons) of variables that *v* implies.
      :rtype: ZDD

   .. py:method:: sym_grp()

      Find all symmetry groups (size >= 2) in the family.

      :return: A ZDD family where each set is a symmetry group.
      :rtype: ZDD

   .. py:method:: sym_grp_naive()

      Find all symmetry groups (naive method, includes size 1).

      :return: A ZDD family where each set is a symmetry group.
      :rtype: ZDD

   .. py:method:: sym_set(v)

      Find all variables symmetric with *v* in the family.

      :param int v: Variable number.
      :return: A ZDD (single set) of variables symmetric with *v*.
      :rtype: ZDD

   .. py:method:: coimply_set(v)

      Find all variables in co-implication relation with *v*.

      :param int v: Variable number.
      :return: A ZDD (single set) of variables co-implied by *v*.
      :rtype: ZDD

   .. py:method:: divisor()

      Find a non-trivial divisor of the family (as polynomial).

      :return: A ZDD representing a divisor.
      :rtype: ZDD

   .. py:method:: support_vars()

      Return all variable numbers that appear in the ZDD, sorted by level.

      :return: A list of variable numbers.
      :rtype: list[int]

   Counting
   --------

   .. py:property:: exact_count
      :type: int

      The number of sets in the family (arbitrary precision Python int).

   .. py:method:: count()

      Count the number of sets in the family (floating-point).

      :return: The number of sets as a float.
      :rtype: float

   .. py:method:: has_empty()

      Check if the empty set is a member of the family.

      :return: True if the family contains the empty set.
      :rtype: bool

   Membership Queries
   ------------------

   .. py:method:: contains(s)

      Check if a set is a member of the family.

      :param list[int] s: A list of variable numbers representing the set.
      :return: True if *s* is in the family.
      :rtype: bool

   .. py:method:: is_subset_family(g)

      Check if this family is a subset of another family (F ⊆ G).

      Uses early termination for efficiency.

      :param ZDD g: The other family.
      :return: True if every set in this family is also in *g*.
      :rtype: bool

   .. py:method:: is_disjoint(g)

      Check if this family and another family are disjoint (F ∩ G = ∅).

      Uses early termination for efficiency.

      :param ZDD g: The other family.
      :return: True if the families share no common sets.
      :rtype: bool

   .. py:method:: count_intersec(g)

      Count the sets in the intersection without building it.

      Computes \|F ∩ G\| without materializing the intersection ZDD.

      :param ZDD g: The other family.
      :return: The number of sets in the intersection.
      :rtype: int

   .. py:method:: jaccard_index(g)

      Compute the Jaccard similarity index.

      J(F, G) = \|F ∩ G\| / \|F ∪ G\|. Returns 1.0 when both families are empty.

      :param ZDD g: The other family.
      :return: The Jaccard index (0.0 to 1.0).
      :rtype: float

   .. py:method:: hamming_distance(g)

      Compute the Hamming distance (symmetric difference size).

      \|F △ G\| = \|F\| + \|G\| - 2\|F ∩ G\|.

      :param ZDD g: The other family.
      :return: The number of sets in the symmetric difference.
      :rtype: int

   .. py:method:: overlap_coefficient(g)

      Compute the overlap coefficient.

      O(F, G) = \|F ∩ G\| / min(\|F\|, \|G\|). Returns 1.0 when both families are empty.

      :param ZDD g: The other family.
      :return: The overlap coefficient (0.0 to 1.0).
      :rtype: float

   Enumeration and Sampling
   ------------------------

   .. py:method:: enumerate()

      Enumerate all sets in the family.

      :return: A list of sets, each a list of variable numbers.
      :rtype: list[list[int]]

   .. py:method:: uniform_sample(seed=0)

      Sample a set uniformly at random from the family.

      :param int seed: Random seed (default: 0).
      :return: A list of variable numbers in the sampled set.
      :rtype: list[int]

   .. py:method:: exact_count_with_memo(memo)

      Count the number of sets using a memo for caching.

      :param ZddCountMemo memo: A ZddCountMemo object for caching.
      :rtype: int

   .. py:method:: uniform_sample_with_memo(memo, seed=0)

      Sample a set using a memo for caching.

      :param ZddCountMemo memo: A ZddCountMemo object for caching.
      :param int seed: Random seed (default: 0).
      :return: A list of variable numbers in the sampled set.
      :rtype: list[int]

   .. py:method:: sample_k(k, seed=0)

      Sample *k* sets uniformly at random from the family.

      Uses hypergeometric distribution at each node for exact uniformity.

      :param int k: Number of sets to sample.
      :param int seed: Random seed (default: 0).
      :return: A ZDD containing the sampled sets.
      :rtype: ZDD

   .. py:method:: sample_k_with_memo(k, memo, seed=0)

      Sample *k* sets using a memo for caching counts.

      :param int k: Number of sets to sample.
      :param ZddCountMemo memo: A ZddCountMemo object for caching.
      :param int seed: Random seed (default: 0).
      :return: A ZDD containing the sampled sets.
      :rtype: ZDD

   .. py:method:: random_subset(p, seed=0)

      Include each set independently with probability *p*.

      :param float p: Inclusion probability (0.0 to 1.0).
      :param int seed: Random seed (default: 0).
      :return: A ZDD containing the randomly selected sets.
      :rtype: ZDD

   .. py:method:: weighted_sample(weights, mode, seed=0)

      Sample one set proportional to aggregated weight.

      The weight of each set is computed by aggregating element weights
      according to *mode* (Sum or Product).

      :param list[float] weights: Weight vector indexed by variable number.
      :param WeightMode mode: Aggregation mode (``WeightMode.Sum`` or ``WeightMode.Product``).
      :param int seed: Random seed (default: 0).
      :return: A list of variable numbers in the sampled set.
      :rtype: list[int]

   .. py:method:: weighted_sample_with_memo(weights, mode, memo, seed=0)

      Weighted sample using a memo for caching precomputed values.

      :param list[float] weights: Weight vector indexed by variable number.
      :param WeightMode mode: Aggregation mode.
      :param WeightedSampleMemo memo: A memo object for caching.
      :param int seed: Random seed (default: 0).
      :return: A list of variable numbers in the sampled set.
      :rtype: list[int]

   .. py:method:: boltzmann_sample(weights, beta, seed=0)

      Sample from a Boltzmann distribution P(S) ∝ exp(-β · Σ w[v]).

      :param list[float] weights: Weight vector indexed by variable number.
      :param float beta: Inverse temperature parameter.
      :param int seed: Random seed (default: 0).
      :return: A list of variable numbers in the sampled set.
      :rtype: list[int]

   .. py:method:: boltzmann_sample_with_memo(weights, beta, memo, seed=0)

      Boltzmann sample using a memo for caching.

      :param list[float] weights: Weight vector indexed by variable number.
      :param float beta: Inverse temperature parameter.
      :param WeightedSampleMemo memo: A memo object for caching.
      :param int seed: Random seed (default: 0).
      :return: A list of variable numbers in the sampled set.
      :rtype: list[int]

   .. py:staticmethod:: boltzmann_weights(weights, beta)

      Transform weights for Boltzmann sampling.

      Computes ``exp(-beta * w)`` for each weight.

      :param list[float] weights: Original weight vector.
      :param float beta: Inverse temperature parameter.
      :return: Transformed weight vector.
      :rtype: list[float]

   Constructors
   ------------

   .. py:staticmethod:: singleton(v)

      Create the ZDD ``{{v}}`` (a family with one singleton set).

      :param int v: Variable number.
      :rtype: ZDD

   .. py:staticmethod:: single_set(vars)

      Create the ZDD ``{{v1, v2, ...}}`` (a family with one set).

      :param list[int] vars: List of variable numbers.
      :rtype: ZDD

   .. py:staticmethod:: power_set(n)

      Create the power set of ``{1, ..., n}``.

      :param int n: Universe size.
      :rtype: ZDD

   .. py:staticmethod:: power_set_vars(vars)

      Create the power set of the given variables.

      :param list[int] vars: List of variable numbers.
      :rtype: ZDD

   .. py:staticmethod:: from_sets(sets)

      Construct a ZDD from a list of sets.

      :param list[list[int]] sets: List of sets, each a list of variable numbers.
      :rtype: ZDD

   .. py:staticmethod:: combination(n, k)

      Create the ZDD of all *k*-element subsets of ``{1, ..., n}``.

      :param int n: Universe size.
      :param int k: Subset size.
      :rtype: ZDD

   .. py:staticmethod:: random_family(n, seed=0)

      Generate a uniformly random family over ``{1, ..., n}``.

      Selects one of the ``2^(2^n)`` possible families uniformly at random.

      :param int n: Universe size.
      :param int seed: Random seed (default: 0).
      :rtype: ZDD

   Node Operations
   ---------------

   .. py:staticmethod:: getnode(var, lo, hi)

      Create a ZDD node with the given variable and children.

      Applies ZDD reduction rules (zero-suppression, complement normalization).

      :param int var: Variable number.
      :param ZDD lo: The low (0-edge) child.
      :param ZDD hi: The high (1-edge) child.
      :rtype: ZDD

   .. py:method:: child0()

      Get the 0-child (lo) with complement edge resolution (ZDD semantics).

      :rtype: ZDD

   .. py:method:: child1()

      Get the 1-child (hi) with complement edge resolution (ZDD semantics).

      :rtype: ZDD

   .. py:method:: child(child)

      Get the child by index (0 or 1) with complement edge resolution.

      :param int child: Child index (0 or 1).
      :rtype: ZDD

   .. py:method:: raw_child0()

      Get the raw 0-child (lo) without complement resolution.

      :rtype: ZDD

   .. py:method:: raw_child1()

      Get the raw 1-child (hi) without complement resolution.

      :rtype: ZDD

   .. py:method:: raw_child(child)

      Get the raw child by index (0 or 1) without complement resolution.

      :param int child: Child index (0 or 1).
      :rtype: ZDD

   .. py:staticmethod:: shared_size(zdds)

      Count the total number of shared nodes across multiple ZDDs.

      :param list[ZDD] zdds: List of ZDD objects.
      :return: The number of distinct nodes (with complement sharing).
      :rtype: int

   .. py:staticmethod:: shared_plain_size(zdds)

      Count the total number of nodes across multiple ZDDs without complement sharing.

      :param list[ZDD] zdds: List of ZDD objects.
      :rtype: int

   Operation Cache
   ---------------

   .. py:staticmethod:: cache_get(op, f, g)

      Read a 2-operand cache entry.

      :param int op: Operation code (0-255).
      :param ZDD f: First operand ZDD.
      :param ZDD g: Second operand ZDD.
      :return: The cached ZDD result, or ``ZDD.null`` on miss.
      :rtype: ZDD

   .. py:staticmethod:: cache_put(op, f, g, result)

      Write a 2-operand cache entry.

      :param int op: Operation code (0-255).
      :param ZDD f: First operand ZDD.
      :param ZDD g: Second operand ZDD.
      :param ZDD result: The result ZDD to cache.

   Summary
   -------

   .. py:method:: print()

      Print ZDD statistics (ID, Var, Size, Card, Lit, Len) and return as string.

      :rtype: str

   Conversion
   ----------

   .. py:method:: to_qdd()

      Convert to a Quasi-reduced Decision Diagram (QDD).

      :rtype: QDD

   .. py:method:: to_bdd(n=0)

      Convert this ZDD (family) to a BDD (characteristic function).

      :param int n: Number of variables (default: 0, uses ``var_count()``).
      :rtype: BDD

   String Formatting
   -----------------

   .. py:method:: print_sets(delim1="},{", delim2=",", var_name_map=...)

      Format the family of sets as a string with custom delimiters.

      Special cases: null ZDD returns ``'N'``, empty ZDD returns ``'E'``.

      :param str delim1: Delimiter between sets (default: ``'},{'``).
      :param str delim2: Delimiter between elements (default: ``','``).
      :param list[str] var_name_map: List indexed by variable number for display names.
      :rtype: str

   .. py:method:: to_str()

      Format the family of sets in default format.

      Each set is enclosed in braces, elements separated by commas.
      Example: ``'{},{1},{2},{2,1}'``.
      Special cases: null ZDD returns ``'N'``, empty ZDD returns ``'E'``.

      :rtype: str

   .. py:method:: to_cnf(var_name_map=[])

      Convert to a CNF (Conjunctive Normal Form) string.

      Each set becomes an OR-clause, and clauses are joined by AND.
      Example: ``'(1 | 2) & (2 | 3)'``.

      :param list[str] var_name_map: Optional variable name mapping.
      :return: CNF string representation.
      :rtype: str

   .. py:method:: to_dnf(var_name_map=[])

      Convert to a DNF (Disjunctive Normal Form) string.

      Each set becomes an AND-term, and terms are joined by OR.
      Example: ``'(1 & 2) | (2 & 3)'``.

      :param list[str] var_name_map: Optional variable name mapping.
      :return: DNF string representation.
      :rtype: str

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

   Binary I/O
   ^^^^^^^^^^

   .. py:method:: export_binary_str()

      Export this ZDD in binary format to a bytes object.

      :rtype: bytes

   .. py:staticmethod:: import_binary_str(data, ignore_type=False)

      Import a ZDD from binary format bytes.

      :param bytes data: Binary data.
      :param bool ignore_type: If True, skip type checking.
      :rtype: ZDD

   .. py:method:: export_binary_file(path)

      Export this ZDD in binary format to a file.

      :param str path: File path to write to.

   .. py:staticmethod:: import_binary_file(path, ignore_type=False)

      Import a ZDD from a binary format file.

      :param str path: File path to read from.
      :param bool ignore_type: If True, skip type checking.
      :rtype: ZDD

   .. py:staticmethod:: export_binary_multi_str(zdds)

      Export multiple ZDDs in binary format to a bytes object.

      :param list[ZDD] zdds: List of ZDD objects.
      :rtype: bytes

   .. py:staticmethod:: import_binary_multi_str(data, ignore_type=False)

      Import multiple ZDDs from binary format bytes.

      :param bytes data: Binary data.
      :param bool ignore_type: If True, skip type checking.
      :rtype: list[ZDD]

   .. py:staticmethod:: export_binary_multi_file(zdds, path)

      Export multiple ZDDs in binary format to a file.

      :param list[ZDD] zdds: List of ZDD objects.
      :param str path: File path to write to.

   .. py:staticmethod:: import_binary_multi_file(path, ignore_type=False)

      Import multiple ZDDs from a binary format file.

      :param str path: File path to read from.
      :param bool ignore_type: If True, skip type checking.
      :rtype: list[ZDD]

   Sapporo I/O
   ^^^^^^^^^^^

   .. py:method:: export_sapporo_str()

      Export this ZDD in Sapporo format to a string.

      :rtype: str

   .. py:staticmethod:: import_sapporo_str(s)

      Import a ZDD from a Sapporo format string.

      :param str s: Sapporo format string.
      :rtype: ZDD

   .. py:method:: export_sapporo_file(path)

      Export this ZDD in Sapporo format to a file.

      :param str path: File path to write to.

   .. py:staticmethod:: import_sapporo_file(path)

      Import a ZDD from a Sapporo format file.

      :param str path: File path to read from.
      :rtype: ZDD

   Knuth I/O (deprecated)
   ^^^^^^^^^^^^^^^^^^^^^^

   .. deprecated::
      Use Sapporo or binary I/O instead.

   .. py:method:: export_knuth_str(is_hex=False, offset=0)

      Export this ZDD in Knuth format to a string.

      :param bool is_hex: If True, use hexadecimal node IDs.
      :param int offset: Variable number offset (default: 0).
      :rtype: str

   .. py:staticmethod:: import_knuth_str(s, is_hex=False, offset=0)

      Import a ZDD from a Knuth format string.

      :param str s: The Knuth format string.
      :param bool is_hex: If True, use hexadecimal node IDs.
      :param int offset: Variable number offset (default: 0).
      :rtype: ZDD

   .. py:method:: export_knuth_file(path, is_hex=False, offset=0)

      Export this ZDD in Knuth format to a file.

      :param str path: File path to write to.
      :param bool is_hex: If True, use hexadecimal node IDs.
      :param int offset: Variable number offset (default: 0).

   .. py:staticmethod:: import_knuth_file(path, is_hex=False, offset=0)

      Import a ZDD from a Knuth format file.

      :param str path: File path to read from.
      :param bool is_hex: If True, use hexadecimal node IDs.
      :param int offset: Variable number offset (default: 0).
      :rtype: ZDD

   Graphillion I/O
   ^^^^^^^^^^^^^^^

   .. py:method:: export_graphillion_str(offset=0)

      Export this ZDD in Graphillion format to a string.

      :param int offset: Variable number offset (default: 0).
      :rtype: str

   .. py:staticmethod:: import_graphillion_str(s, offset=0)

      Import a ZDD from a Graphillion format string.

      :param str s: Graphillion format string.
      :param int offset: Variable number offset (default: 0).
      :rtype: ZDD

   .. py:method:: export_graphillion_file(path, offset=0)

      Export this ZDD in Graphillion format to a file.

      :param str path: File path to write to.
      :param int offset: Variable number offset (default: 0).

   .. py:staticmethod:: import_graphillion_file(path, offset=0)

      Import a ZDD from a Graphillion format file.

      :param str path: File path to read from.
      :param int offset: Variable number offset (default: 0).
      :rtype: ZDD

   Graphviz
   ^^^^^^^^

   .. py:method:: save_graphviz_str(raw=False)

      Export this ZDD as a Graphviz DOT string.

      :param bool raw: If True, show physical DAG with complement markers.
                       If False (default), expand complement edges.
      :rtype: str

   .. py:method:: save_graphviz_file(path, raw=False)

      Export this ZDD as a Graphviz DOT file.

      :param str path: File path to write to.
      :param bool raw: If True, show physical DAG with complement markers.
                       If False (default), expand complement edges.

   SVG
   ^^^

   .. py:method:: save_svg_str(raw=False, draw_zero=True)

      Export this ZDD as an SVG string.

      :param bool raw: If True, show physical DAG with complement markers (Knuth-style).
                       If False (default), expand complement edges.
      :param bool draw_zero: If True (default), draw the 0-terminal node.
      :return: SVG image as a string.
      :rtype: str

   .. py:method:: save_svg_file(path, raw=False, draw_zero=True)

      Export this ZDD as an SVG file.

      :param str path: File path to write to.
      :param bool raw: If True, show physical DAG with complement markers (Knuth-style).
                       If False (default), expand complement edges.
      :param bool draw_zero: If True (default), draw the 0-terminal node.

   Properties
   ----------

   .. py:property:: node_id
      :type: int

      The raw node ID of this ZDD.

   .. py:property:: raw_size
      :type: int

      The number of nodes in the DAG of this ZDD.

   .. py:property:: plain_size
      :type: int

      The number of nodes without complement edge sharing.

   .. py:property:: top_var
      :type: int

      The top (root) variable number of this ZDD.

   .. py:property:: lit
      :type: int

      The total literal count across all sets in the family.

   .. py:property:: max_set_size
      :type: int

      The maximum set size in the family.

   .. py:property:: min_set_size
      :type: int

      The minimum set size in the family.

   .. py:property:: is_poly
      :type: int

      1 if the family has >= 2 sets, 0 otherwise.

   .. py:property:: support
      :type: ZDD

      The support set as a ZDD.

   .. py:property:: is_terminal
      :type: bool

      True if this is a terminal node.

   .. py:property:: is_one
      :type: bool

      True if this is the 1-terminal (unit family containing the empty set).

   .. py:property:: is_zero
      :type: bool

      True if this is the 0-terminal (empty family).

   Statistics
   ----------

   .. py:method:: max_size()

      Return the maximum set size in the family.

      Equivalent to the ``max_set_size`` property.

      :rtype: int

   .. py:method:: profile()

      Compute the set size distribution (arbitrary precision).

      Returns a list where ``result[k]`` is the number of sets of size *k*.

      :return: Set size distribution.
      :rtype: list[int]

   .. py:method:: profile_double()

      Compute the set size distribution (floating-point).

      Returns a list where ``result[k]`` is the number of sets of size *k*.

      :return: Set size distribution.
      :rtype: list[float]

   .. py:method:: element_frequency()

      Compute the frequency of each variable across all sets.

      Returns a list where ``result[v]`` is the number of sets containing
      variable *v*.

      :return: Per-variable frequency counts (arbitrary precision).
      :rtype: list[int]

   .. py:method:: average_size()

      Compute the average number of elements per set.

      :return: The mean set size.
      :rtype: float

   .. py:method:: variance_size()

      Compute the variance of set sizes.

      :return: The variance of the set size distribution.
      :rtype: float

   .. py:method:: median_size()

      Compute the median set size.

      :return: The median set size.
      :rtype: float

   .. py:method:: entropy()

      Compute the Shannon entropy based on element frequencies.

      :return: The entropy value.
      :rtype: float

   Weight Operations
   -----------------

   .. py:method:: get_sum(weights)

      Compute the total weight sum over all sets in the family.

      For each set *S* in the family, computes Σ weights[v] for v ∈ S,
      then returns the total of all such sums.

      :param list[int] weights: List of integer weights indexed by variable number.
                                 Size must be > top variable number of the ZDD.
      :rtype: int

   .. py:method:: min_weight(weights)

      Find the minimum total weight among all sets.

      :param list[int] weights: Weight vector indexed by variable number.
      :return: The minimum weight.
      :rtype: int

   .. py:method:: max_weight(weights)

      Find the maximum total weight among all sets.

      :param list[int] weights: Weight vector indexed by variable number.
      :return: The maximum weight.
      :rtype: int

   .. py:method:: min_weight_set(weights)

      Find a set with the minimum total weight.

      :param list[int] weights: Weight vector indexed by variable number.
      :return: A list of variable numbers in the lightest set.
      :rtype: list[int]

   .. py:method:: max_weight_set(weights)

      Find a set with the maximum total weight.

      :param list[int] weights: Weight vector indexed by variable number.
      :return: A list of variable numbers in the heaviest set.
      :rtype: list[int]

   Iteration
   ---------

   .. py:method:: iter_min_weight(weights)

      Iterate over sets in ascending weight order.

      :param list[int] weights: List of integer weights indexed by variable number.
                                 Size must be > top variable number of the ZDD.
      :return: An iterator yielding (weight, set) pairs.
      :rtype: Iterator[Tuple[int, List[int]]]

   .. py:method:: iter_max_weight(weights)

      Iterate over sets in descending weight order.

      :param list[int] weights: List of integer weights indexed by variable number.
                                 Size must be > top variable number of the ZDD.
      :return: An iterator yielding (weight, set) pairs.
      :rtype: Iterator[Tuple[int, List[int]]]

   .. py:method:: iter_rank()

      Iterate over sets in structure order (same as rank/unrank).

      Structure order: empty set first (if present), then at each node
      hi-edge sets before lo-edge sets.

      :return: An iterator yielding sorted lists of variable numbers.
      :rtype: Iterator[List[int]]

   .. py:method:: iter_random(seed=0)

      Iterate over sets in uniformly random order without replacement.

      Uses a hybrid strategy: rejection sampling when few sets have been
      sampled, direct sampling from the remaining family otherwise.

      :param int seed: Random seed (default: 0).
      :return: An iterator yielding sorted lists of variable numbers.
      :rtype: Iterator[List[int]]

   Ranking and Unranking
   ---------------------

   .. py:method:: rank(s)

      Rank a set in the family.

      Returns the 0-based index of set *s* in the ZDD's structure-based
      ordering (empty set first, then hi-edge before lo-edge).

      :param list[int] s: A list of variable numbers representing the set.
      :return: The rank (int), or -1 if the set is not in the family.
      :rtype: int
      :raises OverflowError: If the rank exceeds int64 range.

   .. py:method:: exact_rank(s)

      Rank a set in the family (arbitrary precision).

      :param list[int] s: A list of variable numbers representing the set.
      :return: The rank (int), or -1 if the set is not in the family.
      :rtype: int

   .. py:method:: exact_rank_with_memo(s, memo)

      Rank a set using a memo for caching.

      The memo must have been created for this ZDD.

      :param list[int] s: A list of variable numbers representing the set.
      :param ZddCountMemo memo: A memo object created for this ZDD.
      :return: The rank (int), or -1 if the set is not in the family.
      :rtype: int
      :raises ValueError: If the memo was created for a different ZDD.

   .. py:method:: unrank(order)

      Retrieve the set at a given index in the family.

      :param int order: The 0-based index.
      :return: A sorted list of variable numbers.
      :rtype: list[int]
      :raises IndexError: If order is out of range.

   .. py:method:: exact_unrank(order)

      Retrieve the set at a given index (arbitrary precision).

      :param int order: The 0-based index (arbitrary precision).
      :return: A sorted list of variable numbers.
      :rtype: list[int]
      :raises IndexError: If order is out of range.

   .. py:method:: exact_unrank_with_memo(order, memo)

      Retrieve the set at a given index using a memo.

      The memo must have been created for this ZDD.

      :param int order: The 0-based index (arbitrary precision).
      :param ZddCountMemo memo: A memo object created for this ZDD.
      :return: A sorted list of variable numbers.
      :rtype: list[int]
      :raises ValueError: If the memo was created for a different ZDD.
      :raises IndexError: If order is out of range.

   Cost-Bounded Filtering
   ----------------------

   .. py:method:: cost_bound_le(weights, b)

      Extract all sets whose total cost is at most *b*.

      Returns a ZDD representing {X ∈ F | Cost(X) ≤ b}, where
      Cost(X) = Σ weights[v] for v ∈ X. Uses the BkTrk-IntervalMemo
      algorithm internally.

      :param list[int] weights: Cost vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int b: Cost bound. Sets with total cost ≤ b are included.
      :return: A ZDD containing all cost-bounded sets.
      :rtype: ZDD
      :raises ValueError: If the ZDD is null or weights is too small.

   .. py:method:: cost_bound_le_with_memo(weights, b, memo)

      Extract cost-bounded sets (≤ b), reusing a memo for efficiency.

      The memo caches intermediate results using the interval-memoizing
      technique. When calling ``cost_bound_le`` repeatedly with different
      bounds on the same ZDD and weights, passing a
      :py:class:`CostBoundMemo` can be significantly faster.

      :param list[int] weights: Cost vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int b: Cost bound. Sets with total cost ≤ b are included.
      :param CostBoundMemo memo: Memo object for caching across calls.
      :return: A ZDD containing all cost-bounded sets.
      :rtype: ZDD
      :raises ValueError: If the ZDD is null, weights is too small,
                          or a different weights vector was used with this memo.

   .. py:method:: cost_bound_ge(weights, b)

      Extract all sets whose total cost is at least *b*.

      Returns a ZDD representing {X ∈ F | Cost(X) ≥ b}.
      Implemented by negating weights and calling ``cost_bound_le``.

      :param list[int] weights: Cost vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int b: Cost bound. Sets with total cost ≥ b are included.
      :return: A ZDD containing all sets with cost ≥ b.
      :rtype: ZDD
      :raises ValueError: If the ZDD is null or weights is too small.

   .. py:method:: cost_bound_ge_with_memo(weights, b, memo)

      Extract sets with cost ≥ b, reusing a memo for efficiency.

      :param list[int] weights: Cost vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int b: Cost bound. Sets with total cost ≥ b are included.
      :param CostBoundMemo memo: Memo object for caching across calls.
      :return: A ZDD containing all sets with cost ≥ b.
      :rtype: ZDD
      :raises ValueError: If the ZDD is null, weights is too small,
                          or a different weights vector was used with this memo.

   .. py:method:: cost_bound_eq(weights, b)

      Extract all sets whose total cost is exactly *b*.

      Returns a ZDD representing {X ∈ F | Cost(X) = b}.
      Computed as ``cost_bound_le(b) - cost_bound_le(b - 1)``.

      :param list[int] weights: Cost vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int b: Cost bound. Sets with total cost = b are included.
      :return: A ZDD containing all sets with cost = b.
      :rtype: ZDD
      :raises ValueError: If the ZDD is null or weights is too small.

   .. py:method:: cost_bound_eq_with_memo(weights, b, memo)

      Extract sets with cost = b, reusing a memo for efficiency.

      :param list[int] weights: Cost vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int b: Cost bound. Sets with total cost = b are included.
      :param CostBoundMemo memo: Memo object for caching across calls.
      :return: A ZDD containing all sets with cost = b.
      :rtype: ZDD
      :raises ValueError: If the ZDD is null, weights is too small,
                          or a different weights vector was used with this memo.

   .. py:method:: cost_bound_range(weights, lo, hi)

      Extract all sets whose total cost is in the range [*lo*, *hi*].

      Returns a ZDD representing {X ∈ F | lo ≤ Cost(X) ≤ hi}.

      :param list[int] weights: Cost vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int lo: Lower cost bound (inclusive).
      :param int hi: Upper cost bound (inclusive).
      :return: A ZDD containing all sets with cost in [lo, hi].
      :rtype: ZDD
      :raises ValueError: If the ZDD is null or weights is too small.

   .. py:method:: cost_bound_range_with_memo(weights, lo, hi, memo)

      Extract sets with cost in [lo, hi], reusing a memo for efficiency.

      :param list[int] weights: Cost vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int lo: Lower cost bound (inclusive).
      :param int hi: Upper cost bound (inclusive).
      :param CostBoundMemo memo: Memo object for caching across calls.
      :return: A ZDD containing all sets with cost in [lo, hi].
      :rtype: ZDD
      :raises ValueError: If the ZDD is null, weights is too small,
                          or a different weights vector was used with this memo.

   First-k and K-lightest/heaviest Extraction
   -------------------------------------------

   .. py:method:: get_k_sets(k)

      Return the first *k* sets in ZDD structure order.

      Structure order: empty set (if present) at index 0, then
      hi-edge sets before lo-edge sets at each node. This is the
      same ordering used by :py:meth:`rank` and :py:meth:`unrank`.

      :param int k: Number of sets to extract (must be >= 0).
      :return: A ZDD containing the first *k* sets.
      :rtype: ZDD
      :raises ValueError: If *k* is negative.

   .. py:method:: exact_get_k_sets(k)

      Return the first *k* sets (arbitrary precision *k*).

      :param int k: Number of sets to extract (arbitrary precision, >= 0).
      :return: A ZDD containing the first *k* sets.
      :rtype: ZDD
      :raises ValueError: If *k* is negative.

   .. py:method:: exact_get_k_sets_with_memo(k, memo)

      Return the first *k* sets, reusing a memo for caching counts.

      :param int k: Number of sets to extract (arbitrary precision, >= 0).
      :param ZddCountMemo memo: Memo object for caching counts.
      :return: A ZDD containing the first *k* sets.
      :rtype: ZDD
      :raises ValueError: If *k* is negative or memo root mismatch.

   .. py:method:: get_k_lightest(k, weights, strict=0)

      Return the *k* sets with the smallest total weight.

      Uses binary search on cost bounds combined with
      :py:meth:`cost_bound_le`. Tie-breaking at the boundary cost
      tier is controlled by *strict*:

      - ``0``: exactly *k* sets (pick from tier by structure order).
      - ``<0``: fewer than *k* (all strictly lighter than boundary).
      - ``>0``: more than *k* (includes full boundary tier).

      :param int k: Number of sets to extract (must be >= 0).
      :param list[int] weights: Weight vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int strict: Tie-breaking mode (default 0).
      :return: A ZDD containing the selected sets.
      :rtype: ZDD
      :raises ValueError: If *k* is negative or weights too small.

   .. py:method:: exact_get_k_lightest(k, weights, strict=0)

      Return the *k* lightest sets (arbitrary precision *k*).

      :param int k: Number of sets (arbitrary precision, >= 0).
      :param list[int] weights: Weight vector indexed by variable number.
      :param int strict: Tie-breaking mode (default 0).
      :return: A ZDD containing the selected sets.
      :rtype: ZDD

   .. py:method:: get_k_heaviest(k, weights, strict=0)

      Return the *k* sets with the largest total weight.

      Implemented as ``f - f.get_k_lightest(|F| - k, weights, -strict)``.

      :param int k: Number of sets to extract (must be >= 0).
      :param list[int] weights: Weight vector indexed by variable number.
                                 Size must be > ``var_used()``.
      :param int strict: Tie-breaking mode (default 0).
      :return: A ZDD containing the selected sets.
      :rtype: ZDD
      :raises ValueError: If *k* is negative or weights too small.

   .. py:method:: exact_get_k_heaviest(k, weights, strict=0)

      Return the *k* heaviest sets (arbitrary precision *k*).

      :param int k: Number of sets (arbitrary precision, >= 0).
      :param list[int] weights: Weight vector indexed by variable number.
      :param int strict: Tie-breaking mode (default 0).
      :return: A ZDD containing the selected sets.
      :rtype: ZDD
