import enum
from typing import IO, Iterator, List, Tuple, Union, overload


class BddCountMemo:
    """Memo for BDD exact counting.

    Caches exact_count results so they can be reused across
    multiple exact_count and uniform_sample calls on the same BDD.
    """

    def __init__(self, bdd: "BDD", n: int) -> None:
        """Create a BDD count memo.

        Args:
            bdd: The BDD to count.
            n: Number of variables.
        """
        ...

    @property
    def stored(self) -> bool:
        """Whether the memo has been populated."""
        ...


class ZddCountMemo:
    """Memo for ZDD exact counting.

    Caches exact_count results so they can be reused across
    multiple exact_count and uniform_sample calls on the same ZDD.
    """

    def __init__(self, zdd: "ZDD") -> None:
        """Create a ZDD count memo.

        Args:
            zdd: The ZDD to count.
        """
        ...

    @property
    def stored(self) -> bool:
        """Whether the memo has been populated."""
        ...


class CostBoundMemo:
    """Interval-memoization table for cost-bounded enumeration.

    Caches intermediate results using the interval-memoizing technique
    (BkTrk-IntervalMemo) so that repeated cost_bound_le/cost_bound_ge/cost_bound_eq
    calls with different bounds on the same ZDD and weights are efficient.

    A single CostBoundMemo must only be used with one weights vector.
    Passing a different weights vector raises ValueError.
    """

    def __init__(self) -> None:
        """Create an empty cost-bound memo."""
        ...

    def clear(self) -> None:
        """Clear all cached entries. The weights binding is preserved."""
        ...


class WeightMode(enum.Enum):
    """Weight aggregation mode for weighted sampling."""

    Sum = ...
    """w(S) = sum of weights. Empty set weight = 0."""
    Product = ...
    """w(S) = product of weights. Empty set weight = 1."""


class WeightedSampleMemo:
    """Memo for weighted sampling.

    Caches precomputed weight values for weighted_sample calls.
    Create with a ZDD, weights vector, and WeightMode.
    """

    def __init__(self, zdd: "ZDD", weights: List[float], mode: WeightMode) -> None:
        """Create a weighted sample memo.

        Args:
            zdd: The ZDD to sample from.
            weights: Weight vector indexed by variable number.
            mode: Weight aggregation mode (Sum or Product).
        """
        ...

    @property
    def stored(self) -> bool:
        """Whether the memo has been populated."""
        ...


class BDD:
    """A Binary Decision Diagram representing a Boolean function.

    Each BDD object holds a root node ID and is automatically
    protected from garbage collection during its lifetime.
    """

    false_: BDD
    """Constant false BDD."""
    true_: BDD
    """Constant true BDD."""
    null: BDD
    """Null (error) BDD."""

    def __init__(self, val: int = 0) -> None:
        """Construct a BDD from an integer value.

        Args:
            val: 0 for false, 1 for true, negative for null.
        """
        ...

    @staticmethod
    def var(v: int) -> BDD:
        """Create a BDD representing the given variable.

        Args:
            v: Variable number (1-based). If the variable has not been created yet, it is automatically created up to v.

        Returns:
            A BDD for the variable.
        """
        ...

    @staticmethod
    def ite(f: BDD, g: BDD, h: BDD) -> BDD:
        """If-then-else: (f & g) | (~f & h).

        Args:
            f: Condition BDD.
            g: Then-branch BDD.
            h: Else-branch BDD.

        Returns:
            The resulting BDD.
        """
        ...

    @staticmethod
    def import_str(s: str) -> BDD:
        """Import a BDD from a string.

        Args:
            s: The serialized BDD string.

        Returns:
            The reconstructed BDD.

        Raises:
            RuntimeError: If import fails.
        """
        ...

    @staticmethod
    def import_file(stream: IO) -> BDD:
        """Import a BDD from a file.

        Args:
            stream: A readable stream.

        Returns:
            The reconstructed BDD.

        Raises:
            RuntimeError: If import fails or file cannot be opened.
        """
        ...

    def __and__(self, other: BDD) -> BDD:
        """Logical AND: self & other."""
        ...
    def __or__(self, other: BDD) -> BDD:
        """Logical OR: self | other."""
        ...
    def __xor__(self, other: BDD) -> BDD:
        """Logical XOR: self ^ other."""
        ...
    def __invert__(self) -> BDD:
        """Logical NOT: ~self."""
        ...
    def __lshift__(self, shift: int) -> BDD:
        """Left shift: rename variable i to i+shift."""
        ...
    def __rshift__(self, shift: int) -> BDD:
        """Right shift: rename variable i to i-shift."""
        ...
    def __iand__(self, other: BDD) -> BDD:
        """In-place logical AND."""
        ...
    def __ior__(self, other: BDD) -> BDD:
        """In-place logical OR."""
        ...
    def __ixor__(self, other: BDD) -> BDD:
        """In-place logical XOR."""
        ...
    def __ilshift__(self, shift: int) -> BDD:
        """In-place left shift."""
        ...
    def __irshift__(self, shift: int) -> BDD:
        """In-place right shift."""
        ...
    def __eq__(self, other: object) -> bool:
        """Equality comparison by node ID."""
        ...
    def __ne__(self, other: object) -> bool:
        """Inequality comparison by node ID."""
        ...
    def __hash__(self) -> int:
        """Hash based on node ID."""
        ...
    def __repr__(self) -> str:
        """Return string representation: 'BDD: id=...'."""
        ...

    def nand(self, other: BDD) -> BDD:
        """Logical NAND: ~(self & other).

        Args:
            other: The other BDD.

        Returns:
            The resulting BDD.
        """
        ...

    def nor(self, other: BDD) -> BDD:
        """Logical NOR: ~(self | other).

        Args:
            other: The other BDD.

        Returns:
            The resulting BDD.
        """
        ...

    def xnor(self, other: BDD) -> BDD:
        """Logical XNOR: ~(self ^ other).

        Args:
            other: The other BDD.

        Returns:
            The resulting BDD.
        """
        ...

    def at0(self, v: int) -> BDD:
        """Cofactor: restrict variable v to 0.

        Args:
            v: Variable number.

        Returns:
            The resulting BDD.
        """
        ...

    def at1(self, v: int) -> BDD:
        """Cofactor: restrict variable v to 1.

        Args:
            v: Variable number.

        Returns:
            The resulting BDD.
        """
        ...

    def cofactor(self, g: BDD) -> BDD:
        """Generalized cofactor (constrain) by BDD g.

        Args:
            g: The constraining BDD (must not be false).

        Returns:
            The generalized cofactor.
        """
        ...

    def support(self) -> BDD:
        """Return the support set as a BDD (disjunction of variables)."""
        ...

    def support_vec(self) -> List[int]:
        """Return the support set as a list of variable numbers."""
        ...

    def imply(self, other: BDD) -> int:
        """Check implication: whether self implies other.

        Args:
            other: The BDD to check against.

        Returns:
            1 if self implies other, 0 otherwise.
        """
        ...

    def exist(self, cube_or_vars_or_var: Union[BDD, List[int], int]) -> BDD:
        """Existential quantification.

        Can be called with a BDD cube, a list of variable numbers,
        or a single variable number.

        Args:
            cube_or_vars_or_var: Variables to quantify out.

        Returns:
            The resulting BDD.
        """
        ...

    def univ(self, cube_or_vars_or_var: Union[BDD, List[int], int]) -> BDD:
        """Universal quantification.

        Can be called with a BDD cube, a list of variable numbers,
        or a single variable number.

        Args:
            cube_or_vars_or_var: Variables to quantify.

        Returns:
            The resulting BDD.
        """
        ...

    def swap(self, v1: int, v2: int) -> BDD:
        """Swap variables v1 and v2 in the BDD.

        Args:
            v1: First variable number.
            v2: Second variable number.

        Returns:
            The BDD with v1 and v2 swapped.
        """
        ...

    def smooth(self, v: int) -> BDD:
        """Smooth (existential quantification) of variable v.

        Args:
            v: Variable number to quantify out.

        Returns:
            The resulting BDD.
        """
        ...

    def spread(self, k: int) -> BDD:
        """Spread variable values to neighboring k levels.

        Args:
            k: Number of levels to spread (must be >= 0).

        Returns:
            The resulting BDD.
        """
        ...

    def export_str(self) -> str:
        """Export this BDD to a string representation."""
        ...

    def export_file(self, stream: IO) -> None:
        """Export this BDD to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def export_multi_str(bdds: List[BDD]) -> str:
        """Export multiple BDDs to a string.

        Args:
            bdds: List of BDD objects.

        Returns:
            The serialized string.
        """
        ...

    @staticmethod
    def import_multi_str(s: str) -> List[BDD]:
        """Import multiple BDDs from a string.

        Args:
            s: The serialized string.

        Returns:
            A list of BDD objects.
        """
        ...

    @staticmethod
    def export_multi_file(bdds: List[BDD], stream: IO) -> None:
        """Export multiple BDDs to a file.

        Args:
            bdds: List of BDD objects.
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_multi_file(stream: IO) -> List[BDD]:
        """Import multiple BDDs from a file.

        Args:
            stream: A readable stream.

        Returns:
            A list of BDD objects.
        """
        ...

    @property
    def raw_size(self) -> int:
        """The number of nodes in the DAG of this BDD."""
        ...
    @property
    def plain_size(self) -> int:
        """The number of nodes without complement edge sharing."""
        ...
    @property
    def top_var(self) -> int:
        """The top (root) variable number of this BDD."""
        ...
    @property
    def node_id(self) -> int:
        """The raw node ID of this BDD."""
        ...
    @property
    def is_terminal(self) -> bool:
        """True if this is a terminal node."""
        ...
    @property
    def is_one(self) -> bool:
        """True if this is the 1-terminal (true)."""
        ...
    @property
    def is_zero(self) -> bool:
        """True if this is the 0-terminal (false)."""
        ...

    def to_qdd(self) -> QDD:
        """Convert to a Quasi-reduced Decision Diagram (QDD).

        Returns:
            The QDD representation.
        """
        ...

    def count(self, n: int) -> float:
        """Count the number of satisfying assignments (floating-point).

        Args:
            n: Number of variables in the Boolean function.

        Returns:
            The number of satisfying assignments as a float.
        """
        ...

    def exact_count(self, n: int) -> int:
        """Count the number of satisfying assignments (arbitrary precision).

        Args:
            n: Number of variables in the Boolean function.

        Returns:
            The number of satisfying assignments as a Python int.
        """
        ...

    def uniform_sample(self, n: int, seed: int = 0) -> List[int]:
        """Sample a satisfying assignment uniformly at random.

        Args:
            n: Number of variables in the Boolean function.
            seed: Random seed (default: 0).

        Returns:
            A list of variable numbers set to 1 in the sampled assignment.
        """
        ...

    def exact_count_with_memo(self, n: int, memo: BddCountMemo) -> int:
        """Count satisfying assignments using a memo for caching.

        Args:
            n: Number of variables in the Boolean function.
            memo: A BddCountMemo object for caching.

        Returns:
            The number of satisfying assignments as a Python int.
        """
        ...

    def uniform_sample_with_memo(self, n: int, memo: BddCountMemo, seed: int = 0) -> List[int]:
        """Sample a satisfying assignment using a memo for caching.

        Args:
            n: Number of variables in the Boolean function.
            memo: A BddCountMemo object for caching.
            seed: Random seed (default: 0).

        Returns:
            A list of variable numbers set to 1 in the sampled assignment.
        """
        ...

    @staticmethod
    def prime(v: int) -> BDD:
        """Create a BDD for the positive literal of variable v.

        Args:
            v: Variable number.

        Returns:
            A BDD representing the variable v.
        """
        ...

    @staticmethod
    def prime_not(v: int) -> BDD:
        """Create a BDD for the negative literal of variable v.

        Args:
            v: Variable number.

        Returns:
            A BDD representing NOT v.
        """
        ...

    @staticmethod
    def cube(lits: List[int]) -> BDD:
        """Create a BDD for the conjunction (AND) of literals.

        Uses DIMACS convention: positive int = variable,
        negative int = negated variable.

        Args:
            lits: List of literals (e.g. [1, -2, 3] means x1 & ~x2 & x3).

        Returns:
            A BDD representing the cube.
        """
        ...

    @staticmethod
    def clause(lits: List[int]) -> BDD:
        """Create a BDD for the disjunction (OR) of literals.

        Uses DIMACS convention: positive int = variable,
        negative int = negated variable.

        Args:
            lits: List of literals (e.g. [1, -2, 3] means x1 | ~x2 | x3).

        Returns:
            A BDD representing the clause.
        """
        ...

    @staticmethod
    def getnode(var: int, lo: BDD, hi: BDD) -> BDD:
        """Create a BDD node with the given variable and children.

        Applies BDD reduction rules (jump rule, complement normalization).

        Args:
            var: Variable number.
            lo: The low (0-edge) child.
            hi: The high (1-edge) child.

        Returns:
            The created BDD node.
        """
        ...

    @staticmethod
    def shared_size(bdds: List[BDD]) -> int:
        """Count the total number of shared nodes across multiple BDDs.

        Args:
            bdds: List of BDD objects.

        Returns:
            The number of distinct nodes (with complement sharing).
        """
        ...

    @staticmethod
    def shared_plain_size(bdds: List[BDD]) -> int:
        """Count the total number of nodes across multiple BDDs without complement sharing.

        Args:
            bdds: List of BDD objects.

        Returns:
            The number of nodes.
        """
        ...

    @staticmethod
    def cache_get(op: int, f: BDD, g: BDD) -> BDD:
        """Read a 2-operand cache entry.

        Args:
            op: Operation code (0-255).
            f: First operand BDD.
            g: Second operand BDD.

        Returns:
            The cached BDD result, or BDD.null on miss.
        """
        ...

    @staticmethod
    def cache_put(op: int, f: BDD, g: BDD, result: BDD) -> None:
        """Write a 2-operand cache entry.

        Args:
            op: Operation code (0-255).
            f: First operand BDD.
            g: Second operand BDD.
            result: The result BDD to cache.
        """
        ...

    def child0(self) -> BDD:
        """Get the 0-child (lo) with complement edge resolution."""
        ...

    def child1(self) -> BDD:
        """Get the 1-child (hi) with complement edge resolution."""
        ...

    def child(self, child: int) -> BDD:
        """Get the child by index (0 or 1) with complement edge resolution."""
        ...

    def raw_child0(self) -> BDD:
        """Get the raw 0-child (lo) without complement resolution."""
        ...

    def raw_child1(self) -> BDD:
        """Get the raw 1-child (hi) without complement resolution."""
        ...

    def raw_child(self, child: int) -> BDD:
        """Get the raw child by index (0 or 1) without complement resolution."""
        ...

    def export_binary_str(self) -> bytes:
        """Export this BDD in binary format to a bytes object."""
        ...

    @staticmethod
    def import_binary_str(data: bytes, ignore_type: bool = False) -> BDD:
        """Import a BDD from binary format bytes."""
        ...

    def export_binary_file(self, stream: IO) -> None:
        """Export this BDD in binary format to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_binary_file(stream: IO, ignore_type: bool = False) -> BDD:
        """Import a BDD from a binary format file.

        Args:
            stream: A readable stream.
            ignore_type: If True, skip dd_type validation.

        Returns:
            The reconstructed BDD.
        """
        ...

    @staticmethod
    def export_binary_multi_str(bdds: List[BDD]) -> bytes:
        """Export multiple BDDs in binary format to a bytes object.

        Args:
            bdds: List of BDD objects.
        """
        ...

    @staticmethod
    def import_binary_multi_str(data: bytes, ignore_type: bool = False) -> List[BDD]:
        """Import multiple BDDs from binary format bytes."""
        ...

    @staticmethod
    def export_binary_multi_file(bdds: List[BDD], stream: IO) -> None:
        """Export multiple BDDs in binary format to a file.

        Args:
            bdds: List of BDD objects.
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_binary_multi_file(stream: IO, ignore_type: bool = False) -> List[BDD]:
        """Import multiple BDDs from a binary format file.

        Args:
            stream: A readable stream.
            ignore_type: If True, skip dd_type validation.
        """
        ...

    def export_sapporo_str(self) -> str:
        """Export this BDD in Sapporo format to a string."""
        ...

    @staticmethod
    def import_sapporo_str(s: str) -> BDD:
        """Import a BDD from a Sapporo format string."""
        ...

    def export_sapporo_file(self, stream: IO) -> None:
        """Export this BDD in Sapporo format to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_sapporo_file(stream: IO) -> BDD:
        """Import a BDD from a Sapporo format file.

        Args:
            stream: A readable stream.
        """
        ...

    def export_knuth_str(self, is_hex: bool = False, offset: int = 0) -> str:
        """Export this BDD in Knuth format to a string (deprecated).

        Args:
            is_hex: If True, use hexadecimal node IDs.
            offset: Variable number offset (default: 0).

        Returns:
            A Knuth format string.

        .. deprecated:: Use export_sapporo_str() or export_binary_str() instead.
        """
        ...

    @staticmethod
    def import_knuth_str(s: str, is_hex: bool = False, offset: int = 0) -> BDD:
        """Import a BDD from a Knuth format string (deprecated).

        Args:
            s: The Knuth format string.
            is_hex: If True, use hexadecimal node IDs.
            offset: Variable number offset (default: 0).

        Returns:
            The reconstructed BDD.

        .. deprecated:: Use import_sapporo_str() or import_binary_str() instead.
        """
        ...

    def export_knuth_file(self, stream: IO, is_hex: bool = False, offset: int = 0) -> None:
        """Export this BDD in Knuth format to a file (deprecated).

        Args:
            stream: A writable stream.
            is_hex: If True, use hexadecimal node IDs.
            offset: Variable number offset (default: 0).

        .. deprecated:: Use export_sapporo_file() or export_binary_file() instead.
        """
        ...

    @staticmethod
    def import_knuth_file(stream: IO, is_hex: bool = False, offset: int = 0) -> BDD:
        """Import a BDD from a Knuth format file (deprecated).

        Args:
            stream: A readable stream.
            is_hex: If True, use hexadecimal node IDs.
            offset: Variable number offset (default: 0).

        Returns:
            The reconstructed BDD.

        .. deprecated:: Use import_sapporo_file() or import_binary_file() instead.
        """
        ...

    def save_graphviz_str(self, raw: bool = False) -> str:
        """Export this BDD as a Graphviz DOT string.

        Args:
            raw: If True, show physical DAG with complement markers.
                 If False (default), expand complement edges into full nodes.

        Returns:
            A DOT format string.
        """
        ...

    def save_graphviz_file(self, stream: IO, raw: bool = False) -> None:
        """Export this BDD as a Graphviz DOT file.

        Args:
            stream: A writable stream.
            raw: If True, show physical DAG with complement markers.
                 If False (default), expand complement edges into full nodes.
        """
        ...

    def print(self) -> str:
        """Print BDD summary (ID, Var, Level, Size) and return as string."""
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


class ZDD:
    """A Zero-suppressed Decision Diagram representing a family of sets.

    Each ZDD object holds a root node ID and is automatically
    protected from garbage collection during its lifetime.
    """

    empty: ZDD
    """Empty family (no sets)."""
    single: ZDD
    """Unit family containing only the empty set."""
    null: ZDD
    """Null (error) ZDD."""

    def __init__(self, val: int = 0) -> None:
        """Construct a ZDD from an integer value.

        Args:
            val: 0 for empty family, 1 for unit family, negative for null.
        """
        ...

    @staticmethod
    def import_str(s: str) -> ZDD:
        """Import a ZDD from a string.

        Args:
            s: The serialized ZDD string.

        Returns:
            The reconstructed ZDD.

        Raises:
            RuntimeError: If import fails.
        """
        ...

    @staticmethod
    def import_file(stream: IO) -> ZDD:
        """Import a ZDD from a file.

        Args:
            stream: A readable stream.

        Returns:
            The reconstructed ZDD.

        Raises:
            RuntimeError: If import fails or file cannot be opened.
        """
        ...

    def __add__(self, other: ZDD) -> ZDD:
        """Union: self + other."""
        ...
    def __sub__(self, other: ZDD) -> ZDD:
        """Subtraction (set difference): self - other."""
        ...
    def __and__(self, other: ZDD) -> ZDD:
        """Intersection: self & other."""
        ...
    def __xor__(self, other: ZDD) -> ZDD:
        """Symmetric difference: self ^ other."""
        ...
    def __invert__(self) -> ZDD:
        """Complement: toggle empty set membership (~self)."""
        ...
    def __mul__(self, other: ZDD) -> ZDD:
        """Join (cross product with union): self * other."""
        ...
    def __truediv__(self, other: ZDD) -> ZDD:
        """Division (quotient): self / other."""
        ...
    def __mod__(self, other: ZDD) -> ZDD:
        """Remainder: self % other."""
        ...
    def __iadd__(self, other: ZDD) -> ZDD:
        """In-place union."""
        ...
    def __isub__(self, other: ZDD) -> ZDD:
        """In-place subtraction."""
        ...
    def __iand__(self, other: ZDD) -> ZDD:
        """In-place intersection."""
        ...
    def __ixor__(self, other: ZDD) -> ZDD:
        """In-place symmetric difference."""
        ...
    def __imul__(self, other: ZDD) -> ZDD:
        """In-place join."""
        ...
    def __itruediv__(self, other: ZDD) -> ZDD:
        """In-place division."""
        ...
    def __imod__(self, other: ZDD) -> ZDD:
        """In-place remainder."""
        ...
    def __lshift__(self, s: int) -> ZDD:
        """Left shift: increase variable numbers by s."""
        ...
    def __rshift__(self, s: int) -> ZDD:
        """Right shift: decrease variable numbers by s."""
        ...
    def __ilshift__(self, s: int) -> ZDD:
        """In-place left shift."""
        ...
    def __irshift__(self, s: int) -> ZDD:
        """In-place right shift."""
        ...
    def __eq__(self, other: object) -> bool:
        """Equality comparison by node ID."""
        ...
    def __ne__(self, other: object) -> bool:
        """Inequality comparison by node ID."""
        ...
    def __hash__(self) -> int:
        """Hash based on node ID."""
        ...
    def __repr__(self) -> str:
        """Return string representation: 'ZDD: id=...'."""
        ...

    def __bool__(self) -> bool:
        """Always raises TypeError.

        ZDD cannot be converted to bool.
        Use ``== ZDD.empty`` or ``== ZDD.single`` instead.

        Raises:
            TypeError: Always.
        """
        ...

    def change(self, v: int) -> ZDD:
        """Toggle membership of variable v in all sets.

        Args:
            v: Variable number.

        Returns:
            The resulting ZDD.
        """
        ...

    def offset(self, v: int) -> ZDD:
        """Select sets NOT containing variable v.

        Args:
            v: Variable number.

        Returns:
            The resulting ZDD.
        """
        ...

    def onset(self, v: int) -> ZDD:
        """Select sets containing variable v (v is kept in the result).

        Args:
            v: Variable number.

        Returns:
            The resulting ZDD.
        """
        ...

    def onset0(self, v: int) -> ZDD:
        """Select sets containing variable v, then remove v (1-cofactor).

        Args:
            v: Variable number.

        Returns:
            The resulting ZDD.
        """
        ...

    def meet(self, other: ZDD) -> ZDD:
        """Meet operation (intersection of all element pairs).

        For each pair of sets (one from this family, one from other),
        compute their intersection and collect all results.

        Args:
            other: Another ZDD family.

        Returns:
            The resulting ZDD.
        """
        ...

    def maximal(self) -> ZDD:
        """Extract maximal sets (no proper superset in the family)."""
        ...

    def minimal(self) -> ZDD:
        """Extract minimal sets (no proper subset in the family)."""
        ...

    def minhit(self) -> ZDD:
        """Compute minimum hitting sets.

        A hitting set intersects every set in the family.
        """
        ...

    def closure(self) -> ZDD:
        """Compute the downward closure.

        Returns all subsets of sets in the family.
        """
        ...

    def restrict(self, g: ZDD) -> ZDD:
        """Restrict to sets that are subsets of some set in g.

        Args:
            g: The constraining family.

        Returns:
            The restricted ZDD.
        """
        ...

    def permit(self, g: ZDD) -> ZDD:
        """Keep sets whose elements are all permitted by g.

        Args:
            g: The permitting family.

        Returns:
            The resulting ZDD.
        """
        ...

    def remove_supersets(self, g: "ZDD") -> "ZDD":
        """Remove sets that are supersets of some set in g.

        Args:
            g: The constraining family.

        Returns:
            The resulting ZDD.
        """
        ...

    def remove_subsets(self, g: "ZDD") -> "ZDD":
        """Remove sets that are subsets of some set in g.

        Args:
            g: The constraining family.

        Returns:
            The resulting ZDD.
        """
        ...

    def disjoin(self, g: ZDD) -> ZDD:
        """Disjoint product of two families.

        For each pair (A, B) where A is in this family and B is in g,
        if A and B are disjoint, include A | B in the result.

        Args:
            g: The other family.

        Returns:
            The resulting ZDD.
        """
        ...

    def joint_join(self, g: ZDD) -> ZDD:
        """Joint join of two families.

        For each pair (A, B) with A & B non-empty,
        include A | B in the result.
        Pairs with no overlap are excluded.

        Args:
            g: The other family.

        Returns:
            The resulting ZDD.
        """
        ...

    def delta(self, g: ZDD) -> ZDD:
        """Delta operation (symmetric difference of elements within pairs).

        Args:
            g: The other family.

        Returns:
            The resulting ZDD.
        """
        ...

    def meet(self, other: ZDD) -> ZDD:
        """Meet operation (intersection of all element pairs).

        For each pair of sets (one from this family, one from other),
        compute their intersection and collect all results.

        Args:
            other: Another ZDD family.

        Returns:
            The resulting ZDD.
        """
        ...

    def always(self) -> ZDD:
        """Find elements common to ALL sets in the family.

        Returns a family of singletons, one for each always-present variable.
        For example, ``{{1,2,3},{1,2}}.always()`` returns ``{{1},{2}}``.
        """
        ...

    def permit_sym(self, n: int) -> ZDD:
        """Symmetric permit: keep sets with at most n elements.

        Args:
            n: Maximum number of elements.

        Returns:
            A ZDD containing only sets with <= n elements.
        """
        ...

    def swap(self, v1: int, v2: int) -> ZDD:
        """Swap two variables in the family.

        Args:
            v1: First variable number.
            v2: Second variable number.

        Returns:
            A ZDD with v1 and v2 swapped.
        """
        ...

    def imply_chk(self, v1: int, v2: int) -> int:
        """Check if v1 implies v2 in the family.

        Args:
            v1: First variable number.
            v2: Second variable number.

        Returns:
            1 if every set containing v1 also contains v2, 0 otherwise.
        """
        ...

    def coimply_chk(self, v1: int, v2: int) -> int:
        """Check co-implication between v1 and v2 in the family.

        Args:
            v1: First variable number.
            v2: Second variable number.

        Returns:
            1 if co-implication holds, 0 otherwise.
        """
        ...

    def sym_chk(self, v1: int, v2: int) -> int:
        """Check if two variables are symmetric in the family.

        Args:
            v1: First variable number.
            v2: Second variable number.

        Returns:
            1 if symmetric, 0 if not.
        """
        ...

    def imply_set(self, v: int) -> ZDD:
        """Find all variables implied by v in the family.

        Args:
            v: Variable number.

        Returns:
            A ZDD (family of singletons) of variables that v implies.
        """
        ...

    def sym_grp(self) -> ZDD:
        """Find all symmetry groups (size >= 2) in the family."""
        ...

    def sym_grp_naive(self) -> ZDD:
        """Find all symmetry groups (naive method, includes size 1)."""
        ...

    def sym_set(self, v: int) -> ZDD:
        """Find all variables symmetric with v in the family.

        Args:
            v: Variable number.

        Returns:
            A ZDD (single set) of variables symmetric with v.
        """
        ...

    def coimply_set(self, v: int) -> ZDD:
        """Find all variables in co-implication relation with v.

        Args:
            v: Variable number.

        Returns:
            A ZDD (single set) of variables co-implied by v.
        """
        ...

    def divisor(self) -> ZDD:
        """Find a non-trivial divisor of the family (as polynomial)."""
        ...

    def print_sets(self, delim1: str = "},{", delim2: str = ",",
                   var_name_map: List[str] = ...) -> str:
        """Print the family of sets as a string with custom delimiters.

        When called with default arguments, sets are separated by '},{'
        and elements by ','. The output contains no outer braces;
        for example: '};{1};{2};{2,1'.

        Special cases:
            - null ZDD: returns 'N'
            - empty ZDD: returns 'E'

        Args:
            delim1: Delimiter between sets (default: '},{').
            delim2: Delimiter between elements within a set (default: ',').
            var_name_map: List indexed by variable number for display names.

        Returns:
            The formatted string.
        """
        ...

    def to_str(self) -> str:
        """Print the family of sets in default format.

        Each set is enclosed in braces, elements separated by commas.
        Example: '{},{1},{2},{2,1}'

        Special cases:
            - null ZDD: returns 'N'
            - empty ZDD: returns 'E'

        Returns:
            The formatted string.
        """
        ...

    def export_str(self) -> str:
        """Export this ZDD to a string representation."""
        ...

    def export_file(self, stream: IO) -> None:
        """Export this ZDD to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def export_multi_str(zdds: List["ZDD"]) -> str:
        """Export multiple ZDDs to a string.

        Args:
            zdds: List of ZDD objects.

        Returns:
            The serialized string.
        """
        ...

    @staticmethod
    def import_multi_str(s: str) -> List["ZDD"]:
        """Import multiple ZDDs from a string.

        Args:
            s: The serialized string.

        Returns:
            A list of ZDD objects.
        """
        ...

    @staticmethod
    def export_multi_file(zdds: List["ZDD"], stream: IO) -> None:
        """Export multiple ZDDs to a file.

        Args:
            zdds: List of ZDD objects.
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_multi_file(stream: IO) -> List["ZDD"]:
        """Import multiple ZDDs from a file.

        Args:
            stream: A readable stream.

        Returns:
            A list of ZDD objects.
        """
        ...

    @property
    def exact_count(self) -> int:
        """The number of sets in the family (arbitrary precision Python int)."""
        ...
    @property
    def top_var(self) -> int:
        """The top (root) variable number of this ZDD."""
        ...
    @property
    def node_id(self) -> int:
        """The raw node ID of this ZDD."""
        ...
    @property
    def raw_size(self) -> int:
        """The number of nodes in the DAG of this ZDD."""
        ...
    @property
    def plain_size(self) -> int:
        """The number of nodes without complement edge sharing."""
        ...
    @property
    def lit(self) -> int:
        """The total literal count across all sets in the family."""
        ...
    @property
    def max_set_size(self) -> int:
        """The maximum set size in the family."""
        ...
    @property
    def is_poly(self) -> int:
        """1 if the family has >= 2 sets, 0 otherwise."""
        ...
    @property
    def support(self) -> ZDD:
        """The support set as a ZDD."""
        ...
    @property
    def is_terminal(self) -> bool:
        """True if this is a terminal node."""
        ...
    @property
    def is_one(self) -> bool:
        """True if this is the 1-terminal ({empty set})."""
        ...
    @property
    def is_zero(self) -> bool:
        """True if this is the 0-terminal (empty family)."""
        ...

    def to_qdd(self) -> QDD:
        """Convert to a Quasi-reduced Decision Diagram (QDD).

        Returns:
            The QDD representation.
        """
        ...

    def count(self) -> float:
        """Count the number of sets in the family (floating-point).

        Returns:
            The number of sets as a float.
        """
        ...

    def uniform_sample(self, seed: int = 0) -> List[int]:
        """Sample a set uniformly at random from the family.

        Args:
            seed: Random seed (default: 0).

        Returns:
            A list of variable numbers in the sampled set.
        """
        ...

    def exact_count_with_memo(self, memo: ZddCountMemo) -> int:
        """Count the number of sets using a memo for caching.

        Args:
            memo: A ZddCountMemo object for caching.

        Returns:
            The number of sets as a Python int.
        """
        ...

    def uniform_sample_with_memo(self, memo: ZddCountMemo, seed: int = 0) -> List[int]:
        """Sample a set using a memo for caching.

        Args:
            memo: A ZddCountMemo object for caching.
            seed: Random seed (default: 0).

        Returns:
            A list of variable numbers in the sampled set.
        """
        ...

    def sample_k(self, k: int, seed: int = 0) -> "ZDD":
        """Sample k sets uniformly at random from the family.

        Args:
            k: Number of sets to sample.
            seed: Random seed (default: 0).

        Returns:
            A ZDD containing the sampled sets.
        """
        ...

    def sample_k_with_memo(self, k: int, memo: ZddCountMemo, seed: int = 0) -> "ZDD":
        """Sample k sets using a memo for caching counts.

        Args:
            k: Number of sets to sample.
            memo: A ZddCountMemo object for caching.
            seed: Random seed (default: 0).

        Returns:
            A ZDD containing the sampled sets.
        """
        ...

    def random_subset(self, p: float, seed: int = 0) -> "ZDD":
        """Include each set independently with probability p.

        Args:
            p: Inclusion probability (0.0 to 1.0).
            seed: Random seed (default: 0).

        Returns:
            A ZDD containing the randomly selected sets.
        """
        ...

    def weighted_sample(self, weights: List[float], mode: WeightMode, seed: int = 0) -> List[int]:
        """Sample a set proportional to its weight.

        Args:
            weights: Weight vector indexed by variable number (index 0 unused).
            mode: WeightMode.Sum or WeightMode.Product.
            seed: Random seed (default: 0).

        Returns:
            A list of variable numbers in the sampled set.
        """
        ...

    def weighted_sample_with_memo(self, weights: List[float], mode: WeightMode, memo: WeightedSampleMemo, seed: int = 0) -> List[int]:
        """Sample a set proportional to its weight, using a memo.

        Args:
            weights: Weight vector indexed by variable number.
            mode: WeightMode.Sum or WeightMode.Product.
            memo: A WeightedSampleMemo for caching.
            seed: Random seed (default: 0).

        Returns:
            A list of variable numbers in the sampled set.
        """
        ...

    def boltzmann_sample(self, weights: List[float], beta: float, seed: int = 0) -> List[int]:
        """Sample from a Boltzmann distribution P(S) proportional to exp(-beta * sum(weights)).

        Args:
            weights: Weight vector indexed by variable number.
            beta: Inverse temperature parameter.
            seed: Random seed (default: 0).

        Returns:
            A list of variable numbers in the sampled set.
        """
        ...

    def boltzmann_sample_with_memo(self, weights: List[float], beta: float, memo: WeightedSampleMemo, seed: int = 0) -> List[int]:
        """Sample from Boltzmann distribution using a memo.

        Args:
            weights: Weight vector indexed by variable number.
            beta: Inverse temperature parameter.
            memo: A WeightedSampleMemo (Product mode, transformed weights).
            seed: Random seed (default: 0).

        Returns:
            A list of variable numbers in the sampled set.
        """
        ...

    @staticmethod
    def boltzmann_weights(weights: List[float], beta: float) -> List[float]:
        """Transform weights for Boltzmann sampling.

        Computes exp(-beta * w) for each weight.

        Args:
            weights: Original weight vector.
            beta: Inverse temperature parameter.

        Returns:
            Transformed weight vector for use with WeightedSampleMemo(Product mode).
        """
        ...

    def enumerate(self) -> List[List[int]]:
        """Enumerate all sets in the family.

        Returns:
            A list of sets, each a list of variable numbers.
        """
        ...

    def has_empty(self) -> bool:
        """Check if the empty set is a member of the family.

        Returns:
            True if the family contains the empty set.
        """
        ...

    def contains(self, s: List[int]) -> bool:
        """Check if a set is a member of the family.

        Args:
            s: A list of variable numbers representing the set.

        Returns:
            True if the set is in the family.
        """
        ...

    def choose(self, k: int) -> ZDD:
        """Filter to sets of exactly k elements.

        Args:
            k: Required number of elements.

        Returns:
            A ZDD containing only sets with exactly k elements.
        """
        ...

    def profile(self) -> List[int]:
        """Return the set size distribution (arbitrary precision).

        Returns:
            A list where profile[i] is the number of sets with exactly
            i elements. The list length is max_set_size + 1.
        """
        ...

    def profile_double(self) -> List[float]:
        """Return the set size distribution (floating-point).

        Returns:
            A list where profile[i] is the number of sets with exactly
            i elements (float).
        """
        ...

    def min_weight(self, weights: List[int]) -> int:
        """Find the minimum weight sum among all sets in the family.

        Args:
            weights: A list of integer weights indexed by variable number.
                     Size must be > the top variable number of the ZDD.

        Returns:
            The minimum weight sum.

        Raises:
            ValueError: If the ZDD is null, the family is empty, or weights is too small.
        """
        ...

    def max_weight(self, weights: List[int]) -> int:
        """Find the maximum weight sum among all sets in the family.

        Args:
            weights: A list of integer weights indexed by variable number.
                     Size must be > the top variable number of the ZDD.

        Returns:
            The maximum weight sum.

        Raises:
            ValueError: If the ZDD is null, the family is empty, or weights is too small.
        """
        ...

    def min_weight_set(self, weights: List[int]) -> List[int]:
        """Find a set with the minimum weight sum.

        Args:
            weights: A list of integer weights indexed by variable number.
                     Size must be > the top variable number of the ZDD.

        Returns:
            A list of variable numbers forming a set with minimum weight sum.

        Raises:
            ValueError: If the ZDD is null, the family is empty, or weights is too small.
        """
        ...

    def max_weight_set(self, weights: List[int]) -> List[int]:
        """Find a set with the maximum weight sum.

        Args:
            weights: A list of integer weights indexed by variable number.
                     Size must be > the top variable number of the ZDD.

        Returns:
            A list of variable numbers forming a set with maximum weight sum.

        Raises:
            ValueError: If the ZDD is null, the family is empty, or weights is too small.
        """
        ...

    def iter_min_weight(self, weights: List[int]) -> Iterator[Tuple[int, List[int]]]:
        """Iterate over sets in ascending weight order.

        Args:
            weights: A list of integer weights indexed by variable number.
                     Size must be > the top variable number of the ZDD.

        Returns:
            An iterator yielding (weight, set) pairs.
        """
        ...

    def iter_max_weight(self, weights: List[int]) -> Iterator[Tuple[int, List[int]]]:
        """Iterate over sets in descending weight order.

        Args:
            weights: A list of integer weights indexed by variable number.
                     Size must be > the top variable number of the ZDD.

        Returns:
            An iterator yielding (weight, set) pairs.
        """
        ...

    def iter_rank(self) -> Iterator[List[int]]:
        """Iterate over sets in structure order (same as rank/unrank).

        Structure order: empty set first (if present), then at each node
        hi-edge sets before lo-edge sets.

        Returns:
            An iterator yielding sorted lists of variable numbers.
        """
        ...

    def iter_random(self, seed: int = 0) -> Iterator[List[int]]:
        """Iterate over sets in uniformly random order without replacement.

        Uses a hybrid strategy: rejection sampling when few sets have been
        sampled, direct sampling from the remaining family otherwise.

        Args:
            seed: Random seed (default: 0).

        Returns:
            An iterator yielding sorted lists of variable numbers.
        """
        ...

    def rank(self, s: List[int]) -> int:
        """Rank a set in the family.

        Returns the 0-based index of set s in the ZDD's structure-based
        ordering (empty set first, then hi-edge before lo-edge).

        Args:
            s: A list of variable numbers representing the set.

        Returns:
            The rank (int), or -1 if the set is not in the family.

        Raises:
            OverflowError: If the rank exceeds int64 range.
        """
        ...

    def exact_rank(self, s: List[int]) -> int:
        """Rank a set in the family (arbitrary precision).

        Args:
            s: A list of variable numbers representing the set.

        Returns:
            The rank (int), or -1 if the set is not in the family.
        """
        ...

    def exact_rank_with_memo(self, s: List[int], memo: ZddCountMemo) -> int:
        """Rank a set using a memo for caching.

        The memo must have been created for this ZDD.

        Args:
            s: A list of variable numbers representing the set.
            memo: A ZddCountMemo object created for this ZDD.

        Returns:
            The rank (int), or -1 if the set is not in the family.

        Raises:
            ValueError: If the memo was created for a different ZDD.
        """
        ...

    def unrank(self, order: int) -> List[int]:
        """Retrieve the set at a given index in the family.

        Args:
            order: The 0-based index.

        Returns:
            A sorted list of variable numbers.

        Raises:
            IndexError: If order is out of range.
        """
        ...

    def exact_unrank(self, order: int) -> List[int]:
        """Retrieve the set at a given index (arbitrary precision).

        Args:
            order: The 0-based index (arbitrary precision).

        Returns:
            A sorted list of variable numbers.

        Raises:
            IndexError: If order is out of range.
        """
        ...

    def exact_unrank_with_memo(self, order: int, memo: ZddCountMemo) -> List[int]:
        """Retrieve the set at a given index using a memo.

        The memo must have been created for this ZDD.

        Args:
            order: The 0-based index (arbitrary precision).
            memo: A ZddCountMemo object created for this ZDD.

        Returns:
            A sorted list of variable numbers.

        Raises:
            ValueError: If the memo was created for a different ZDD.
            IndexError: If order is out of range.
        """
        ...

    def get_sum(self, weights: List[int]) -> int:
        """Compute the total weight sum over all sets in the family.

        For each set S in the family, computes sum of weights[v] for v in S,
        then returns the total of all such sums (arbitrary precision).

        Args:
            weights: A list of integer weights indexed by variable number.
                     Size must be > the top variable number of the ZDD.

        Returns:
            The total weight sum (int, arbitrary precision).

        Raises:
            ValueError: If the ZDD is null or weights is too small.
        """
        ...

    def cost_bound_le(self, weights: List[int], b: int) -> ZDD:
        """Extract all sets whose total cost is at most b.

        Returns a ZDD representing {X in F | Cost(X) <= b}, where
        Cost(X) = sum of weights[v] for v in X.

        Uses the BkTrk-IntervalMemo algorithm internally.

        Args:
            weights: A list of integer costs indexed by variable number.
                     Size must be > the number of variables (var_used()).
            b: Cost bound. Sets with total cost <= b are included.

        Returns:
            A ZDD containing all cost-bounded sets.

        Raises:
            ValueError: If the ZDD is null or weights is too small.
        """
        ...

    def cost_bound_le_with_memo(self, weights: List[int], b: int, memo: CostBoundMemo) -> ZDD:
        """Extract cost-bounded sets (<= b), reusing a memo for efficiency.

        The memo caches intermediate results using interval-memoizing.
        When calling cost_bound_le repeatedly with different bounds on
        the same ZDD and weights, passing a CostBoundMemo can be
        significantly faster.

        Args:
            weights: A list of integer costs indexed by variable number.
                     Size must be > the number of variables (var_used()).
            b: Cost bound. Sets with total cost <= b are included.
            memo: A CostBoundMemo object for caching across calls.

        Returns:
            A ZDD containing all cost-bounded sets.

        Raises:
            ValueError: If the ZDD is null, weights is too small,
                        or a different weights vector was used with this memo.
        """
        ...

    def cost_bound_ge(self, weights: List[int], b: int) -> ZDD:
        """Extract all sets whose total cost is at least b.

        Returns a ZDD representing {X in F | Cost(X) >= b}, where
        Cost(X) = sum of weights[v] for v in X.

        Implemented by negating weights and calling cost_bound_le.

        Args:
            weights: A list of integer costs indexed by variable number.
                     Size must be > the number of variables (var_used()).
            b: Cost bound. Sets with total cost >= b are included.

        Returns:
            A ZDD containing all sets with cost >= b.

        Raises:
            ValueError: If the ZDD is null or weights is too small.
        """
        ...

    def cost_bound_ge_with_memo(self, weights: List[int], b: int, memo: CostBoundMemo) -> ZDD:
        """Extract sets with cost >= b, reusing a memo for efficiency.

        Args:
            weights: A list of integer costs indexed by variable number.
                     Size must be > the number of variables (var_used()).
            b: Cost bound. Sets with total cost >= b are included.
            memo: A CostBoundMemo object for caching across calls.

        Returns:
            A ZDD containing all sets with cost >= b.

        Raises:
            ValueError: If the ZDD is null, weights is too small,
                        or a different weights vector was used with this memo.
        """
        ...

    def cost_bound_eq(self, weights: List[int], b: int) -> ZDD:
        """Extract all sets whose total cost is exactly b.

        Returns a ZDD representing {X in F | Cost(X) = b}.
        Computed as cost_bound_le(b) - cost_bound_le(b - 1).

        Args:
            weights: A list of integer costs indexed by variable number.
                     Size must be > the number of variables (var_used()).
            b: Cost bound. Sets with total cost exactly b are included.

        Returns:
            A ZDD containing all sets with cost == b.

        Raises:
            ValueError: If the ZDD is null or weights is too small.
        """
        ...

    def cost_bound_eq_with_memo(self, weights: List[int], b: int, memo: CostBoundMemo) -> ZDD:
        """Extract sets with cost == b, reusing a memo for efficiency.

        Args:
            weights: A list of integer costs indexed by variable number.
                     Size must be > the number of variables (var_used()).
            b: Cost bound. Sets with total cost exactly b are included.
            memo: A CostBoundMemo object for caching across calls.

        Returns:
            A ZDD containing all sets with cost == b.

        Raises:
            ValueError: If the ZDD is null, weights is too small,
                        or a different weights vector was used with this memo.
        """
        ...

    def size_le(self, k: int) -> ZDD:
        """Extract all sets with at most k elements.

        Returns a ZDD representing {X in F | |X| <= k}.

        Args:
            k: Maximum set size.

        Returns:
            A ZDD containing all sets with size <= k.

        Raises:
            ValueError: If the ZDD is null.
        """
        ...

    def size_ge(self, k: int) -> ZDD:
        """Extract all sets with at least k elements.

        Returns a ZDD representing {X in F | |X| >= k}.

        Args:
            k: Minimum set size.

        Returns:
            A ZDD containing all sets with size >= k.

        Raises:
            ValueError: If the ZDD is null.
        """
        ...

    @staticmethod
    def singleton(v: int) -> ZDD:
        """Create the ZDD {{v}} (a family with one singleton set).

        Args:
            v: Variable number.

        Returns:
            A ZDD representing {{v}}.
        """
        ...

    @staticmethod
    def single_set(vars: List[int]) -> ZDD:
        """Create the ZDD {{v1, v2, ...}} (a family with one set).

        Args:
            vars: List of variable numbers.

        Returns:
            A ZDD representing the single set.
        """
        ...

    @staticmethod
    def power_set(n: int) -> ZDD:
        """Create the power set of {1, ..., n}.

        Args:
            n: Universe size.

        Returns:
            A ZDD representing 2^{1,...,n}.
        """
        ...

    @staticmethod
    def power_set_vars(vars: List[int]) -> ZDD:
        """Create the power set of the given variables.

        Args:
            vars: List of variable numbers.

        Returns:
            A ZDD representing the power set.
        """
        ...

    @staticmethod
    def from_sets(sets: List[List[int]]) -> ZDD:
        """Construct a ZDD from a list of sets.

        Args:
            sets: List of sets, each a list of variable numbers.

        Returns:
            A ZDD representing the family of sets.
        """
        ...

    @staticmethod
    def combination(n: int, k: int) -> ZDD:
        """Create the ZDD of all k-element subsets of {1, ..., n}.

        Args:
            n: Universe size.
            k: Subset size.

        Returns:
            A ZDD representing C(n,k).
        """
        ...

    @staticmethod
    def random_family(n: int, seed: int = 0) -> ZDD:
        """Generate a uniformly random family over {1, ..., n}.

        Selects one of the 2^(2^n) possible families uniformly at random.

        Args:
            n: Universe size.
            seed: Random seed (default: 0).

        Returns:
            A ZDD representing the randomly chosen family.
        """
        ...

    @staticmethod
    def getnode(var: int, lo: ZDD, hi: ZDD) -> ZDD:
        """Create a ZDD node with the given variable and children.

        Applies ZDD reduction rules (zero-suppression, complement normalization).

        Args:
            var: Variable number.
            lo: The low (0-edge) child.
            hi: The high (1-edge) child.

        Returns:
            The created ZDD node.
        """
        ...

    @staticmethod
    def shared_size(zdds: List[ZDD]) -> int:
        """Count the total number of shared nodes across multiple ZDDs.

        Args:
            zdds: List of ZDD objects.

        Returns:
            The number of distinct nodes (with complement sharing).
        """
        ...

    @staticmethod
    def shared_plain_size(zdds: List[ZDD]) -> int:
        """Count the total number of nodes across multiple ZDDs without complement sharing.

        Args:
            zdds: List of ZDD objects.

        Returns:
            The number of nodes.
        """
        ...

    @staticmethod
    def cache_get(op: int, f: ZDD, g: ZDD) -> ZDD:
        """Read a 2-operand cache entry.

        Args:
            op: Operation code (0-255).
            f: First operand ZDD.
            g: Second operand ZDD.

        Returns:
            The cached ZDD result, or ZDD.null on miss.
        """
        ...

    @staticmethod
    def cache_put(op: int, f: ZDD, g: ZDD, result: ZDD) -> None:
        """Write a 2-operand cache entry.

        Args:
            op: Operation code (0-255).
            f: First operand ZDD.
            g: Second operand ZDD.
            result: The result ZDD to cache.
        """
        ...

    def child0(self) -> ZDD:
        """Get the 0-child (lo) with complement edge resolution (ZDD semantics)."""
        ...

    def child1(self) -> ZDD:
        """Get the 1-child (hi) with complement edge resolution (ZDD semantics)."""
        ...

    def child(self, child: int) -> ZDD:
        """Get the child by index (0 or 1) with complement edge resolution (ZDD semantics)."""
        ...

    def raw_child0(self) -> ZDD:
        """Get the raw 0-child (lo) without complement resolution."""
        ...

    def raw_child1(self) -> ZDD:
        """Get the raw 1-child (hi) without complement resolution."""
        ...

    def raw_child(self, child: int) -> ZDD:
        """Get the raw child by index (0 or 1) without complement resolution."""
        ...

    def export_binary_str(self) -> bytes:
        """Export this ZDD in binary format to a bytes object."""
        ...

    @staticmethod
    def import_binary_str(data: bytes, ignore_type: bool = False) -> ZDD:
        """Import a ZDD from binary format bytes."""
        ...

    def export_binary_file(self, stream: IO) -> None:
        """Export this ZDD in binary format to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_binary_file(stream: IO, ignore_type: bool = False) -> ZDD:
        """Import a ZDD from a binary format file.

        Args:
            stream: A readable stream.
            ignore_type: If True, skip dd_type validation.
        """
        ...

    @staticmethod
    def export_binary_multi_str(zdds: List[ZDD]) -> bytes:
        """Export multiple ZDDs in binary format to a bytes object.

        Args:
            zdds: List of ZDD objects.
        """
        ...

    @staticmethod
    def import_binary_multi_str(data: bytes, ignore_type: bool = False) -> List[ZDD]:
        """Import multiple ZDDs from binary format bytes."""
        ...

    @staticmethod
    def export_binary_multi_file(zdds: List[ZDD], stream: IO) -> None:
        """Export multiple ZDDs in binary format to a file.

        Args:
            zdds: List of ZDD objects.
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_binary_multi_file(stream: IO, ignore_type: bool = False) -> List[ZDD]:
        """Import multiple ZDDs from a binary format file.

        Args:
            stream: A readable stream.
            ignore_type: If True, skip dd_type validation.
        """
        ...

    def export_sapporo_str(self) -> str:
        """Export this ZDD in Sapporo format to a string."""
        ...

    @staticmethod
    def import_sapporo_str(s: str) -> ZDD:
        """Import a ZDD from a Sapporo format string."""
        ...

    def export_sapporo_file(self, stream: IO) -> None:
        """Export this ZDD in Sapporo format to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_sapporo_file(stream: IO) -> ZDD:
        """Import a ZDD from a Sapporo format file.

        Args:
            stream: A readable stream.
        """
        ...

    def export_knuth_str(self, is_hex: bool = False, offset: int = 0) -> str:
        """Export this ZDD in Knuth format to a string (deprecated).

        Args:
            is_hex: If True, use hexadecimal node IDs.
            offset: Variable number offset (default: 0).

        Returns:
            A Knuth format string.

        .. deprecated:: Use export_sapporo_str() or export_binary_str() instead.
        """
        ...

    @staticmethod
    def import_knuth_str(s: str, is_hex: bool = False, offset: int = 0) -> ZDD:
        """Import a ZDD from a Knuth format string (deprecated).

        Args:
            s: The Knuth format string.
            is_hex: If True, use hexadecimal node IDs.
            offset: Variable number offset (default: 0).

        Returns:
            The reconstructed ZDD.

        .. deprecated:: Use import_sapporo_str() or import_binary_str() instead.
        """
        ...

    def export_knuth_file(self, stream: IO, is_hex: bool = False, offset: int = 0) -> None:
        """Export this ZDD in Knuth format to a file (deprecated).

        Args:
            stream: A writable stream.
            is_hex: If True, use hexadecimal node IDs.
            offset: Variable number offset (default: 0).

        .. deprecated:: Use export_sapporo_file() or export_binary_file() instead.
        """
        ...

    @staticmethod
    def import_knuth_file(stream: IO, is_hex: bool = False, offset: int = 0) -> ZDD:
        """Import a ZDD from a Knuth format file (deprecated).

        Args:
            stream: A readable stream.
            is_hex: If True, use hexadecimal node IDs.
            offset: Variable number offset (default: 0).

        Returns:
            The reconstructed ZDD.

        .. deprecated:: Use import_sapporo_file() or import_binary_file() instead.
        """
        ...

    def export_graphillion_str(self, offset: int = 0) -> str:
        """Export this ZDD in Graphillion format to a string.

        Args:
            offset: Variable number offset (default: 0).

        Returns:
            A Graphillion format string.
        """
        ...

    @staticmethod
    def import_graphillion_str(s: str, offset: int = 0) -> ZDD:
        """Import a ZDD from a Graphillion format string.

        Args:
            s: The Graphillion format string.
            offset: Variable number offset (default: 0).

        Returns:
            The reconstructed ZDD.
        """
        ...

    def export_graphillion_file(self, stream: IO, offset: int = 0) -> None:
        """Export this ZDD in Graphillion format to a file.

        Args:
            stream: A writable stream.
            offset: Variable number offset (default: 0).
        """
        ...

    @staticmethod
    def import_graphillion_file(stream: IO, offset: int = 0) -> ZDD:
        """Import a ZDD from a Graphillion format file.

        Args:
            stream: A readable stream.
            offset: Variable number offset (default: 0).

        Returns:
            The reconstructed ZDD.
        """
        ...

    def save_graphviz_str(self, raw: bool = False) -> str:
        """Export this ZDD as a Graphviz DOT string.

        Args:
            raw: If True, show physical DAG with complement markers.
                 If False (default), expand complement edges into full nodes.

        Returns:
            A DOT format string.
        """
        ...

    def save_graphviz_file(self, stream: IO, raw: bool = False) -> None:
        """Export this ZDD as a Graphviz DOT file.

        Args:
            stream: A writable stream.
            raw: If True, show physical DAG with complement markers.
                 If False (default), expand complement edges into full nodes.
        """
        ...

    def print(self) -> str:
        """Print ZDD statistics (ID, Var, Size, Card, Lit, Len) and return as string."""
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


def init(node_count: int = 256, node_max: int = ...) -> None:
    """Initialize the BDD library.

    Args:
        node_count: Initial number of node slots to allocate.
        node_max: Maximum number of node slots allowed.
    """
    ...

def finalize() -> None:
    """Finalize the BDD library and release all resources.

    Raises RuntimeError if called while BDD or ZDD objects exist.
    Safe to call multiple times when no objects are alive.
    """
    ...

def new_var() -> int:
    """Create a new variable and return its variable number."""
    ...

def new_var_of_level(lev: int) -> int:
    """Create a new variable at the specified level.

    Args:
        lev: The level to insert the new variable at.

    Returns:
        The variable number of the newly created variable.
    """
    ...

def to_level(var: int) -> int:
    """Convert a variable number to its level.

    Args:
        var: Variable number.
    """
    ...

def to_var(level: int) -> int:
    """Convert a level to its variable number.

    Args:
        level: Level number.
    """
    ...

def var_count() -> int:
    """Return the number of variables created so far."""
    ...

def gc() -> None:
    """Manually invoke garbage collection."""
    ...

def live_nodes() -> int:
    """Return the number of live (non-garbage) nodes."""
    ...

def gc_set_threshold(threshold: float) -> None:
    """Set the GC threshold.

    Args:
        threshold: A value between 0.0 and 1.0 (default: 0.9).
    """
    ...

def gc_get_threshold() -> float:
    """Get the current GC threshold."""
    ...

def node_count() -> int:
    """Return the number of used node slots (including dead nodes awaiting GC)."""
    ...

def new_vars(n: int, reverse: bool = False) -> List[int]:
    """Create multiple new variables at once.

    Args:
        n: Number of variables to create.
        reverse: If True, return variable numbers in reverse order.

    Returns:
        A list of the newly created variable numbers.
    """
    ...

def gc_rootcount() -> int:
    """Return the number of registered GC root pointers."""
    ...

def top_level() -> int:
    """Return the maximum level number (equal to the number of variables)."""
    ...



# ================================================================
# PiDD
# ================================================================

def pidd_newvar() -> int:
    """Create a new PiDD variable (extend permutation size by 1).

    Returns:
        The new permutation size.
    """
    ...

def pidd_var_used() -> int:
    """Return the current PiDD permutation size."""
    ...


class PiDD:
    """A Permutation Decision Diagram based on adjacent transpositions.

    Represents a set of permutations using a ZDD, where each variable
    corresponds to an adjacent transposition Swap(x, y).
    """

    def __init__(self, val: int = 0) -> None:
        """Construct a PiDD from an integer value.

        Args:
            val: 0 for empty set, 1 for {identity}, negative for null.
        """
        ...

    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __repr__(self) -> str: ...

    def __and__(self, other: PiDD) -> PiDD:
        """Intersection: self & other."""
        ...
    def __add__(self, other: PiDD) -> PiDD:
        """Union: self + other."""
        ...
    def __sub__(self, other: PiDD) -> PiDD:
        """Difference: self - other."""
        ...
    def __mul__(self, other: PiDD) -> PiDD:
        """Composition: self * other."""
        ...
    def __truediv__(self, other: PiDD) -> PiDD:
        """Division: self / other."""
        ...
    def __mod__(self, other: PiDD) -> PiDD:
        """Remainder: self % other."""
        ...
    def __iand__(self, other: PiDD) -> PiDD:
        """In-place intersection."""
        ...
    def __iadd__(self, other: PiDD) -> PiDD:
        """In-place union."""
        ...
    def __isub__(self, other: PiDD) -> PiDD:
        """In-place difference."""
        ...
    def __imul__(self, other: PiDD) -> PiDD:
        """In-place composition."""
        ...

    def swap(self, u: int, v: int) -> PiDD:
        """Apply transposition Swap(u, v) to all permutations.

        Args:
            u: First position.
            v: Second position.

        Returns:
            A new PiDD with the transposition applied.
        """
        ...

    def cofact(self, u: int, v: int) -> PiDD:
        """Extract permutations where position u has value v.

        Args:
            u: Position to check.
            v: Required value at position u.

        Returns:
            A PiDD of sub-permutations satisfying the condition.
        """
        ...

    def odd(self) -> PiDD:
        """Extract odd permutations from the set."""
        ...

    def even(self) -> PiDD:
        """Extract even permutations from the set."""
        ...

    def swap_bound(self, n: int) -> PiDD:
        """Apply symmetric constraint (PermitSym) to the internal ZDD.

        Args:
            n: Constraint parameter.

        Returns:
            A PiDD with the constraint applied.
        """
        ...

    @property
    def top_x(self) -> int:
        """The x coordinate of the top variable."""
        ...
    @property
    def top_y(self) -> int:
        """The y coordinate of the top variable."""
        ...
    @property
    def top_lev(self) -> int:
        """The BDD level of the top variable."""
        ...
    @property
    def size(self) -> int:
        """The number of nodes in the internal ZDD."""
        ...
    @property
    def exact_count(self) -> int:
        """The number of permutations in the set."""
        ...
    @property
    def zdd(self) -> ZDD:
        """The internal ZDD representation."""
        ...

    def __truediv__(self, other: PiDD) -> PiDD:
        """Division: self / other."""
        ...
    def __mod__(self, other: PiDD) -> PiDD:
        """Remainder: self % other."""
        ...
    def __itruediv__(self, other: PiDD) -> PiDD:
        """In-place division."""
        ...
    def __imod__(self, other: PiDD) -> PiDD:
        """In-place remainder."""
        ...

    def print(self) -> str:
        """Print all permutations and return as a string."""
        ...

    def enum(self) -> str:
        """Enumerate all permutations in compact form and return as a string."""
        ...

    def enum2(self) -> str:
        """Enumerate all permutations in expanded form and return as a string."""
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


# ================================================================
# RotPiDD
# ================================================================

def rotpidd_newvar() -> int:
    """Create a new RotPiDD variable (extend permutation size by 1).

    Returns:
        The new permutation size.
    """
    ...

def rotpidd_var_used() -> int:
    """Return the current RotPiDD permutation size."""
    ...

def rotpidd_from_perm(perm: List[int]) -> RotPiDD:
    """Create a RotPiDD from a permutation vector.

    The vector is automatically normalized to [1..n].

    Args:
        perm: A list of integers representing a permutation.

    Returns:
        A RotPiDD containing the single permutation.
    """
    ...


class RotPiDD:
    """A Rotational Permutation Decision Diagram.

    Represents a set of permutations using a ZDD, where each variable
    corresponds to a left rotation LeftRot(x, y).
    """

    def __init__(self, val: int = 0) -> None:
        """Construct a RotPiDD from an integer value.

        Args:
            val: 0 for empty set, 1 for {identity}, negative for null.
        """
        ...

    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...
    def __repr__(self) -> str: ...

    def __and__(self, other: RotPiDD) -> RotPiDD:
        """Intersection: self & other."""
        ...
    def __add__(self, other: RotPiDD) -> RotPiDD:
        """Union: self + other."""
        ...
    def __sub__(self, other: RotPiDD) -> RotPiDD:
        """Difference: self - other."""
        ...
    def __mul__(self, other: RotPiDD) -> RotPiDD:
        """Composition: self * other."""
        ...
    def __iand__(self, other: RotPiDD) -> RotPiDD:
        """In-place intersection."""
        ...
    def __iadd__(self, other: RotPiDD) -> RotPiDD:
        """In-place union."""
        ...
    def __isub__(self, other: RotPiDD) -> RotPiDD:
        """In-place difference."""
        ...
    def __imul__(self, other: RotPiDD) -> RotPiDD:
        """In-place composition."""
        ...

    def left_rot(self, u: int, v: int) -> RotPiDD:
        """Apply left rotation LeftRot(u, v) to all permutations.

        Left-rotates positions v, v+1, ..., u cyclically.

        Args:
            u: Upper position.
            v: Lower position.

        Returns:
            A new RotPiDD with the rotation applied.
        """
        ...

    def swap(self, a: int, b: int) -> RotPiDD:
        """Swap positions a and b in all permutations.

        Args:
            a: First position.
            b: Second position.

        Returns:
            A new RotPiDD with the swap applied.
        """
        ...

    def reverse(self, l: int, r: int) -> RotPiDD:
        """Reverse positions l..r in all permutations.

        Args:
            l: Left position.
            r: Right position.

        Returns:
            A new RotPiDD with the reversal applied.
        """
        ...

    def cofact(self, u: int, v: int) -> RotPiDD:
        """Extract permutations where position u has value v.

        Args:
            u: Position to check.
            v: Required value at position u.

        Returns:
            A RotPiDD of sub-permutations satisfying the condition.
        """
        ...

    def odd(self) -> RotPiDD:
        """Extract odd permutations from the set."""
        ...

    def even(self) -> RotPiDD:
        """Extract even permutations from the set."""
        ...

    def rot_bound(self, n: int) -> RotPiDD:
        """Apply symmetric constraint (PermitSym) to the internal ZDD.

        Args:
            n: Constraint parameter.

        Returns:
            A RotPiDD with the constraint applied.
        """
        ...

    def order(self, a: int, b: int) -> RotPiDD:
        """Extract permutations where pi(a) < pi(b).

        Args:
            a: First position.
            b: Second position.

        Returns:
            A RotPiDD containing only permutations satisfying the order.
        """
        ...

    def inverse(self) -> RotPiDD:
        """Compute the inverse of each permutation in the set."""
        ...

    def insert(self, p: int, v: int) -> RotPiDD:
        """Insert value v at position p in each permutation.

        Args:
            p: Insertion position.
            v: Value to insert.

        Returns:
            A RotPiDD with the element inserted.
        """
        ...

    def remove_max(self, k: int) -> RotPiDD:
        """Remove variables with size >= k from each permutation.

        Args:
            k: Threshold.

        Returns:
            A RotPiDD with large variables removed.
        """
        ...

    def normalize(self, k: int) -> RotPiDD:
        """Remove variables with x > k by projecting them out.

        Args:
            k: Upper bound for retained variables.

        Returns:
            A normalized RotPiDD.
        """
        ...

    def extract_one(self) -> RotPiDD:
        """Extract a single permutation from the set."""
        ...

    def to_perms(self) -> List[List[int]]:
        """Convert to a list of permutation vectors.

        Returns:
            A list of lists, each representing a permutation as [1..n].
        """
        ...

    @property
    def top_x(self) -> int:
        """The x coordinate of the top variable."""
        ...
    @property
    def top_y(self) -> int:
        """The y coordinate of the top variable."""
        ...
    @property
    def top_lev(self) -> int:
        """The BDD level of the top variable."""
        ...
    @property
    def size(self) -> int:
        """The number of nodes in the internal ZDD."""
        ...
    @property
    def exact_count(self) -> int:
        """The number of permutations in the set."""
        ...
    @property
    def zdd(self) -> ZDD:
        """The internal ZDD representation."""
        ...

    def print(self) -> str:
        """Print all permutations and return as a string."""
        ...

    def enum(self) -> str:
        """Enumerate all permutations in compact form and return as a string."""
        ...

    def enum2(self) -> str:
        """Enumerate all permutations in expanded form and return as a string."""
        ...

    def contradiction_maximization(
        self,
        n: int,
        w: List[List[int]],
    ) -> int:
        """Run contradiction maximization algorithm.

        Args:
            n: Permutation size.
            w: Weight matrix (n+1 x n+1, 1-indexed).

        Returns:
            The maximum contradiction value.
        """
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


# ================================================================
# QDD
# ================================================================

class QDD:
    """A Quasi-reduced Decision Diagram.

    QDD does not apply the jump rule (lo == hi nodes are preserved).
    Every path from root to terminal visits every variable level exactly once.
    Uses BDD complement edge semantics.
    """

    false_: QDD
    """Constant false QDD."""
    true_: QDD
    """Constant true QDD."""
    null: QDD
    """Null (error) QDD."""

    def __init__(self, val: int = 0) -> None:
        """Construct a QDD from an integer value.

        Args:
            val: 0 for false, 1 for true, negative for null.
        """
        ...

    def __eq__(self, other: object) -> bool:
        """Equality comparison by node ID."""
        ...
    def __ne__(self, other: object) -> bool:
        """Inequality comparison by node ID."""
        ...
    def __hash__(self) -> int:
        """Hash based on node ID."""
        ...
    def __repr__(self) -> str:
        """Return string representation: 'QDD: id=...'."""
        ...
    def __invert__(self) -> QDD:
        """Complement (negate): ~self."""
        ...

    @staticmethod
    def getnode(var: int, lo: QDD, hi: QDD) -> QDD:
        """Create a QDD node with level validation.

        Children must be at the expected level (var's level - 1).

        Args:
            var: Variable number.
            lo: The low (0-edge) child.
            hi: The high (1-edge) child.

        Returns:
            The created QDD node.
        """
        ...

    def child0(self) -> QDD:
        """Get the 0-child (lo) with complement resolution."""
        ...

    def child1(self) -> QDD:
        """Get the 1-child (hi) with complement resolution."""
        ...

    def child(self, child: int) -> QDD:
        """Get the child by index (0 or 1) with complement resolution."""
        ...

    def raw_child0(self) -> QDD:
        """Get the raw 0-child (lo) without complement resolution."""
        ...

    def raw_child1(self) -> QDD:
        """Get the raw 1-child (hi) without complement resolution."""
        ...

    def raw_child(self, child: int) -> QDD:
        """Get the raw child by index (0 or 1) without complement resolution."""
        ...

    def to_bdd(self) -> BDD:
        """Convert to a canonical BDD by applying jump rule."""
        ...

    def to_zdd(self) -> ZDD:
        """Convert to a canonical ZDD."""
        ...

    @staticmethod
    def cache_get(op: int, f: QDD, g: QDD) -> QDD:
        """Read a 2-operand cache entry.

        Args:
            op: Operation code (0-255).
            f: First operand QDD.
            g: Second operand QDD.

        Returns:
            The cached QDD result, or QDD.null on miss.
        """
        ...

    @staticmethod
    def cache_put(op: int, f: QDD, g: QDD, result: QDD) -> None:
        """Write a 2-operand cache entry.

        Args:
            op: Operation code (0-255).
            f: First operand QDD.
            g: Second operand QDD.
            result: The result QDD to cache.
        """
        ...

    @property
    def node_id(self) -> int:
        """The raw node ID of this QDD."""
        ...
    @property
    def is_terminal(self) -> bool:
        """True if this is a terminal node."""
        ...
    @property
    def is_one(self) -> bool:
        """True if this is the 1-terminal."""
        ...
    @property
    def is_zero(self) -> bool:
        """True if this is the 0-terminal."""
        ...
    @property
    def top_var(self) -> int:
        """The top (root) variable number of this QDD."""
        ...
    @property
    def raw_size(self) -> int:
        """The number of nodes in the DAG of this QDD."""
        ...

    def export_binary_str(self) -> bytes:
        """Export this QDD in binary format to a bytes object."""
        ...

    @staticmethod
    def import_binary_str(data: bytes, ignore_type: bool = False) -> QDD:
        """Import a QDD from binary format bytes."""
        ...

    def export_binary_file(self, stream: IO) -> None:
        """Export this QDD in binary format to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_binary_file(stream: IO, ignore_type: bool = False) -> QDD:
        """Import a QDD from a binary format file.

        Args:
            stream: A readable stream.
            ignore_type: If True, skip dd_type validation.
        """
        ...

    @staticmethod
    def export_binary_multi_str(qdds: List[QDD]) -> bytes:
        """Export multiple QDDs in binary format to a bytes object.

        Args:
            qdds: List of QDD objects.
        """
        ...

    @staticmethod
    def import_binary_multi_str(data: bytes, ignore_type: bool = False) -> List[QDD]:
        """Import multiple QDDs from binary format bytes."""
        ...

    @staticmethod
    def export_binary_multi_file(qdds: List[QDD], stream: IO) -> None:
        """Export multiple QDDs in binary format to a file.

        Args:
            qdds: List of QDD objects.
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_binary_multi_file(stream: IO, ignore_type: bool = False) -> List[QDD]:
        """Import multiple QDDs from a binary format file.

        Args:
            stream: A readable stream.
            ignore_type: If True, skip dd_type validation.
        """
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


# ================================================================
# UnreducedDD
# ================================================================

class UnreducedDD:
    """A type-agnostic unreduced Decision Diagram.

    Does NOT apply any reduction rules at node creation time.
    Complement edges are stored raw and only gain meaning
    when reduce_as_bdd(), reduce_as_zdd(), or reduce_as_qdd() is called.
    """

    @overload
    def __init__(self, val: int = 0) -> None:
        """Construct an UnreducedDD from an integer value.

        Args:
            val: 0 for 0-terminal, 1 for 1-terminal, negative for null.
        """
        ...
    @overload
    def __init__(self, bdd: BDD) -> None:
        """Convert a BDD to an UnreducedDD with complement expansion.

        Recursively expands all complement edges using BDD semantics.
        """
        ...
    @overload
    def __init__(self, zdd: ZDD) -> None:
        """Convert a ZDD to an UnreducedDD with complement expansion.

        Recursively expands all complement edges using ZDD semantics.
        """
        ...
    @overload
    def __init__(self, qdd: QDD) -> None:
        """Convert a QDD to an UnreducedDD with complement expansion.

        Uses BDD complement semantics.
        """
        ...

    def __eq__(self, other: object) -> bool:
        """Equality comparison by node ID (not semantic equality)."""
        ...
    def __ne__(self, other: object) -> bool:
        """Inequality comparison by node ID."""
        ...
    def __lt__(self, other: UnreducedDD) -> bool:
        """Less-than by node ID (for ordered containers)."""
        ...
    def __hash__(self) -> int:
        """Hash based on node ID."""
        ...
    def __repr__(self) -> str:
        """Return string representation: 'UnreducedDD: id=...'."""
        ...
    def __invert__(self) -> UnreducedDD:
        """Toggle complement bit (bit 0). O(1).

        The complement bit has no semantics in UnreducedDD;
        it is only interpreted at reduce time.
        """
        ...

    @staticmethod
    def zero() -> UnreducedDD:
        """Return a 0-terminal UnreducedDD."""
        ...

    @staticmethod
    def one() -> UnreducedDD:
        """Return a 1-terminal UnreducedDD."""
        ...

    @staticmethod
    def getnode(var: int, lo: UnreducedDD, hi: UnreducedDD) -> UnreducedDD:
        """Create an unreduced DD node.

        Always allocates a new node. No complement normalization,
        no reduction rules, no unique table insertion.

        Args:
            var: Variable number.
            lo: The low (0-edge) child.
            hi: The high (1-edge) child.

        Returns:
            The created UnreducedDD node.
        """
        ...

    @staticmethod
    def wrap_raw_bdd(bdd: BDD) -> UnreducedDD:
        """Wrap a BDD's bddp directly without complement expansion.

        Only use reduce_as_bdd() on the result.
        """
        ...

    @staticmethod
    def wrap_raw_zdd(zdd: ZDD) -> UnreducedDD:
        """Wrap a ZDD's bddp directly without complement expansion.

        Only use reduce_as_zdd() on the result.
        """
        ...

    @staticmethod
    def wrap_raw_qdd(qdd: QDD) -> UnreducedDD:
        """Wrap a QDD's bddp directly without complement expansion.

        Only use reduce_as_qdd() on the result.
        """
        ...

    def raw_child0(self) -> UnreducedDD:
        """Get the raw 0-child (lo) as an UnreducedDD."""
        ...

    def raw_child1(self) -> UnreducedDD:
        """Get the raw 1-child (hi) as an UnreducedDD."""
        ...

    def raw_child(self, child: int) -> UnreducedDD:
        """Get the raw child by index (0 or 1) as an UnreducedDD."""
        ...

    def set_child0(self, child: UnreducedDD) -> None:
        """Set the 0-child (lo) of this unreduced node.

        Only valid on unreduced, non-terminal, non-complemented nodes.
        """
        ...

    def set_child1(self, child: UnreducedDD) -> None:
        """Set the 1-child (hi) of this unreduced node.

        Only valid on unreduced, non-terminal, non-complemented nodes.
        """
        ...

    def reduce_as_bdd(self) -> BDD:
        """Reduce to a canonical BDD.

        Applies BDD complement semantics and jump rule.
        """
        ...

    def reduce_as_zdd(self) -> ZDD:
        """Reduce to a canonical ZDD.

        Applies ZDD complement semantics and zero-suppression rule.
        """
        ...

    def reduce_as_qdd(self) -> QDD:
        """Reduce to a canonical QDD.

        Equivalent to reduce_as_bdd().to_qdd().
        """
        ...

    @property
    def node_id(self) -> int:
        """The raw node ID of this UnreducedDD."""
        ...
    @property
    def is_terminal(self) -> bool:
        """True if this is a terminal node."""
        ...
    @property
    def is_one(self) -> bool:
        """True if this is the 1-terminal."""
        ...
    @property
    def is_zero(self) -> bool:
        """True if this is the 0-terminal."""
        ...
    @property
    def is_reduced(self) -> bool:
        """True if this DD is fully reduced (canonical)."""
        ...
    @property
    def top_var(self) -> int:
        """The top (root) variable number."""
        ...
    @property
    def raw_size(self) -> int:
        """The number of nodes in the DAG."""
        ...

    def export_binary_str(self) -> bytes:
        """Export this UnreducedDD in binary format to a bytes object."""
        ...

    @staticmethod
    def import_binary_str(data: bytes) -> UnreducedDD:
        """Import an UnreducedDD from binary format bytes."""
        ...

    def export_binary_file(self, stream: IO) -> None:
        """Export this UnreducedDD in binary format to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def import_binary_file(stream: IO) -> UnreducedDD:
        """Import an UnreducedDD from a binary format file.

        Args:
            stream: A readable stream.
        """
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


# ================================================================
# SeqBDD
# ================================================================

class SeqBDD:
    """A Sequence BDD representing sets of sequences.

    Uses ZDD internally to compactly represent ordered sequences
    where the same symbol may appear multiple times.
    """

    @overload
    def __init__(self, val: int = 0) -> None:
        """Construct a SeqBDD.

        Args:
            val: 0 for empty set, 1 for {epsilon}, negative for null.
        """
        ...
    @overload
    def __init__(self, zdd: ZDD) -> None:
        """Construct a SeqBDD from an existing ZDD.

        Args:
            zdd: A ZDD to interpret as a sequence set.
        """
        ...

    def __eq__(self, other: object) -> bool:
        """Equality comparison."""
        ...
    def __ne__(self, other: object) -> bool:
        """Inequality comparison."""
        ...
    def __hash__(self) -> int:
        """Hash based on internal ZDD node ID."""
        ...
    def __repr__(self) -> str:
        """Return string representation: 'SeqBDD: id=...'."""
        ...
    def __str__(self) -> str:
        """Return the sequence string representation."""
        ...

    def __and__(self, other: SeqBDD) -> SeqBDD:
        """Intersection: ``self & other``."""
        ...
    def __add__(self, other: SeqBDD) -> SeqBDD:
        """Union: ``self + other``."""
        ...
    def __sub__(self, other: SeqBDD) -> SeqBDD:
        """Difference: ``self - other``."""
        ...
    def __mul__(self, other: SeqBDD) -> SeqBDD:
        """Concatenation: ``self * other``."""
        ...
    def __truediv__(self, other: SeqBDD) -> SeqBDD:
        """Left quotient: ``self / other``."""
        ...
    def __mod__(self, other: SeqBDD) -> SeqBDD:
        """Left remainder: ``self % other``."""
        ...
    def __iand__(self, other: SeqBDD) -> SeqBDD:
        """In-place intersection."""
        ...
    def __iadd__(self, other: SeqBDD) -> SeqBDD:
        """In-place union."""
        ...
    def __isub__(self, other: SeqBDD) -> SeqBDD:
        """In-place difference."""
        ...
    def __imul__(self, other: SeqBDD) -> SeqBDD:
        """In-place concatenation."""
        ...
    def __itruediv__(self, other: SeqBDD) -> SeqBDD:
        """In-place left quotient."""
        ...
    def __imod__(self, other: SeqBDD) -> SeqBDD:
        """In-place left remainder."""
        ...

    def offset(self, v: int) -> SeqBDD:
        """Remove sequences starting with variable v.

        Args:
            v: Variable number.

        Returns:
            A SeqBDD without sequences starting with v.
        """
        ...

    def onset0(self, v: int) -> SeqBDD:
        """Extract suffixes of sequences starting with v.

        Selects sequences whose first symbol is v, then removes the leading v.

        Args:
            v: Variable number.

        Returns:
            A SeqBDD of suffixes after stripping the leading v.
        """
        ...

    def onset(self, v: int) -> SeqBDD:
        """Extract sequences starting with variable v.

        Args:
            v: Variable number.

        Returns:
            A SeqBDD of sequences that start with v.
        """
        ...

    def push(self, v: int) -> SeqBDD:
        """Prepend variable v to all sequences.

        Args:
            v: Variable number to prepend.

        Returns:
            A SeqBDD with v prepended to every sequence.
        """
        ...

    @property
    def top(self) -> int:
        """The variable number of the root node."""
        ...
    @property
    def size(self) -> int:
        """The number of nodes in the internal ZDD."""
        ...
    @property
    def exact_count(self) -> int:
        """The number of sequences in the set."""
        ...
    @property
    def lit(self) -> int:
        """Total symbol count across all sequences."""
        ...
    @property
    def len(self) -> int:
        """Length of the longest sequence."""
        ...
    @property
    def zdd(self) -> ZDD:
        """The internal ZDD representation."""
        ...

    def export_str(self) -> str:
        """Export the internal ZDD in Sapporo format to a string."""
        ...

    def export_file(self, stream: IO) -> None:
        """Export the internal ZDD in Sapporo format to a file.

        Args:
            stream: A writable stream.
        """
        ...

    @staticmethod
    def from_list(vars: List[int]) -> SeqBDD:
        """Create a SeqBDD representing a single sequence.

        Args:
            vars: List of variable numbers for the sequence.

        Returns:
            A SeqBDD containing the single sequence.
        """
        ...

    def print_seq(self) -> str:
        """Print all sequences in a human-readable format and return as string."""
        ...

    def seq_str(self) -> str:
        """Get all sequences as a string.

        Equivalent to str(self).

        Returns:
            A string representation of all sequences.
        """
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


class MVDDVarInfo:
    """Information about a single MVDD variable."""

    @property
    def mvdd_var(self) -> int:
        """MVDD variable number (1-indexed)."""
        ...

    @property
    def k(self) -> int:
        """Value domain size."""
        ...

    @property
    def dd_vars(self) -> List[int]:
        """Internal DD variable numbers (size k-1)."""
        ...


class MVDDVarTable:
    """Manages the mapping between MVDD variables and internal DD variables."""

    def __init__(self, k: int) -> None:
        """Create a variable table with domain size k.

        Args:
            k: Value domain size (>= 2, <= 65536).
        """
        ...

    @property
    def k(self) -> int:
        """Value domain size."""
        ...

    @property
    def mvdd_var_count(self) -> int:
        """Number of registered MVDD variables."""
        ...

    def register_var(self, dd_vars: List[int]) -> int:
        """Register a new MVDD variable with internal DD variables.

        Args:
            dd_vars: Internal DD variable numbers (must have size k-1).

        Returns:
            The new MVDD variable number (1-indexed).
        """
        ...

    def dd_vars_of(self, mv: int) -> List[int]:
        """Return internal DD variable numbers for MVDD variable mv."""
        ...

    def mvdd_var_of(self, dv: int) -> int:
        """Return the MVDD variable number for internal DD variable dv.

        Returns 0 if not found.
        """
        ...

    def dd_var_index(self, dv: int) -> int:
        """Return the index (0 to k-2) of DD variable dv within its MVDD variable.

        Returns -1 if not found.
        """
        ...

    def var_info(self, mv: int) -> MVDDVarInfo:
        """Return info for MVDD variable mv."""
        ...

    def get_top_dd_var(self, mv: int) -> int:
        """Return the first (lowest-level) DD variable for MVDD variable mv."""
        ...


class MVBDD:
    """Multi-Valued BDD.

    Represents a Boolean function over multi-valued variables
    (each taking values 0..k-1).
    """

    @overload
    def __init__(self, k: int, value: bool = False) -> None: ...
    @overload
    def __init__(self, table: MVDDVarTable, bdd: "BDD") -> None: ...

    @staticmethod
    def zero(table: MVDDVarTable) -> "MVBDD":
        """Constant false, sharing the given table."""
        ...

    @staticmethod
    def one(table: MVDDVarTable) -> "MVBDD":
        """Constant true, sharing the given table."""
        ...

    @staticmethod
    def singleton(base: "MVBDD", mv: int, value: int) -> "MVBDD":
        """Create a literal: MVDD variable mv equals value."""
        ...

    @staticmethod
    def ite(base: "MVBDD", mv: int, children: List["MVBDD"]) -> "MVBDD":
        """Build an MVBDD by specifying a child for each value of variable mv."""
        ...

    @staticmethod
    def from_bdd(base: "MVBDD", bdd: "BDD") -> "MVBDD":
        """Wrap a BDD as an MVBDD using base's variable table."""
        ...

    def new_var(self) -> int:
        """Create a new MVDD variable."""
        ...

    def child(self, value: int) -> "MVBDD":
        """Return the cofactor when the top MVDD variable takes the given value."""
        ...

    def evaluate(self, assignment: List[int]) -> bool:
        """Evaluate the Boolean function for the given MVDD assignment."""
        ...

    def to_bdd(self) -> "BDD":
        """Return the internal BDD."""
        ...

    @property
    def k(self) -> int:
        """Value domain size."""
        ...

    @property
    def var_table(self) -> MVDDVarTable:
        """Shared variable table."""
        ...

    @property
    def node_id(self) -> int:
        """Raw node ID."""
        ...

    @property
    def top_var(self) -> int:
        """Top MVDD variable number (0 for terminals)."""
        ...

    @property
    def is_zero(self) -> bool:
        """True if constant false."""
        ...

    @property
    def is_one(self) -> bool:
        """True if constant true."""
        ...

    @property
    def is_terminal(self) -> bool:
        """True if terminal node."""
        ...

    @property
    def mvbdd_node_count(self) -> int:
        """MVDD-level logical node count."""
        ...

    @property
    def size(self) -> int:
        """Internal BDD node count."""
        ...

    def __and__(self, other: "MVBDD") -> "MVBDD": ...
    def __or__(self, other: "MVBDD") -> "MVBDD": ...
    def __xor__(self, other: "MVBDD") -> "MVBDD": ...
    def __invert__(self) -> "MVBDD": ...
    def __iand__(self, other: "MVBDD") -> "MVBDD": ...
    def __ior__(self, other: "MVBDD") -> "MVBDD": ...
    def __ixor__(self, other: "MVBDD") -> "MVBDD": ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


class MVZDD:
    """Multi-Valued ZDD.

    Represents a family of multi-valued assignments
    (each variable takes values 0..k-1).
    """

    @overload
    def __init__(self, k: int, value: bool = False) -> None: ...
    @overload
    def __init__(self, table: MVDDVarTable, zdd: "ZDD") -> None: ...

    @staticmethod
    def zero(table: MVDDVarTable) -> "MVZDD":
        """Empty family, sharing the given table."""
        ...

    @staticmethod
    def one(table: MVDDVarTable) -> "MVZDD":
        """Family containing only the all-zero assignment."""
        ...

    @staticmethod
    def singleton(base: "MVZDD", mv: int, value: int) -> "MVZDD":
        """Create a singleton family: one assignment where mv=value, all others=0."""
        ...

    @staticmethod
    def ite(base: "MVZDD", mv: int, children: List["MVZDD"]) -> "MVZDD":
        """Build an MVZDD by specifying a child for each value of variable mv."""
        ...

    @staticmethod
    def from_zdd(base: "MVZDD", zdd: "ZDD") -> "MVZDD":
        """Wrap a ZDD as an MVZDD using base's variable table."""
        ...

    def new_var(self) -> int:
        """Create a new MVDD variable."""
        ...

    def child(self, value: int) -> "MVZDD":
        """Return the sub-family when the top MVDD variable takes the given value."""
        ...

    def evaluate(self, assignment: List[int]) -> bool:
        """Check if the given assignment is in the family."""
        ...

    def enumerate(self) -> List[List[int]]:
        """Enumerate all MVDD assignments in the family."""
        ...

    def to_str(self) -> str:
        """Return a string representation of all assignments."""
        ...

    @overload
    def print_sets(self) -> str: ...
    @overload
    def print_sets(self, var_names: List[str]) -> str: ...

    def to_zdd(self) -> "ZDD":
        """Return the internal ZDD."""
        ...

    @property
    def count(self) -> float:
        """Number of assignments (double)."""
        ...

    @property
    def exact_count(self) -> int:
        """Number of assignments (arbitrary precision)."""
        ...

    @property
    def k(self) -> int:
        """Value domain size."""
        ...

    @property
    def var_table(self) -> MVDDVarTable:
        """Shared variable table."""
        ...

    @property
    def node_id(self) -> int:
        """Raw node ID."""
        ...

    @property
    def top_var(self) -> int:
        """Top MVDD variable number (0 for terminals)."""
        ...

    @property
    def is_zero(self) -> bool:
        """True if empty family."""
        ...

    @property
    def is_one(self) -> bool:
        """True if family contains only the all-zero assignment."""
        ...

    @property
    def is_terminal(self) -> bool:
        """True if terminal node."""
        ...

    @property
    def mvzdd_node_count(self) -> int:
        """MVDD-level logical node count."""
        ...

    @property
    def size(self) -> int:
        """Internal ZDD node count."""
        ...

    def __add__(self, other: "MVZDD") -> "MVZDD": ...
    def __sub__(self, other: "MVZDD") -> "MVZDD": ...
    def __and__(self, other: "MVZDD") -> "MVZDD": ...
    def __iadd__(self, other: "MVZDD") -> "MVZDD": ...
    def __isub__(self, other: "MVZDD") -> "MVZDD": ...
    def __iand__(self, other: "MVZDD") -> "MVZDD": ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


class MTBDDFloat:
    """Multi-Terminal BDD with double (float) terminal values.

    Also known as ADD (Algebraic Decision Diagram).
    """

    def __init__(self) -> None:
        """Create a zero-terminal MTBDD."""
        ...

    @staticmethod
    def terminal(value: float) -> "MTBDDFloat":
        """Create a terminal node with the given value."""
        ...

    @staticmethod
    def ite(var: int, high: "MTBDDFloat", low: "MTBDDFloat") -> "MTBDDFloat":
        """Create a node: if var then high else low."""
        ...

    @staticmethod
    def from_bdd(bdd: "BDD", zero_val: float = 0.0, one_val: float = 1.0) -> "MTBDDFloat":
        """Convert a BDD to MTBDD."""
        ...

    @staticmethod
    def zero_terminal() -> "MTBDDFloat":
        """Return the zero-terminal MTBDD."""
        ...

    @staticmethod
    def min(a: "MTBDDFloat", b: "MTBDDFloat") -> "MTBDDFloat":
        """Element-wise minimum."""
        ...

    @staticmethod
    def max(a: "MTBDDFloat", b: "MTBDDFloat") -> "MTBDDFloat":
        """Element-wise maximum."""
        ...

    def terminal_value(self) -> float:
        """Return the terminal value."""
        ...

    def evaluate(self, assignment: List[int]) -> float:
        """Evaluate the MTBDD for the given assignment."""
        ...

    def ite_cond(self, then_case: "MTBDDFloat", else_case: "MTBDDFloat") -> "MTBDDFloat":
        """ITE: if this (condition) then then_case else else_case."""
        ...

    @property
    def node_id(self) -> int: ...
    @property
    def is_terminal(self) -> bool: ...
    @property
    def is_zero(self) -> bool: ...
    @property
    def is_one(self) -> bool: ...
    @property
    def top_var(self) -> int: ...
    @property
    def raw_size(self) -> int: ...

    def __add__(self, other: "MTBDDFloat") -> "MTBDDFloat": ...
    def __sub__(self, other: "MTBDDFloat") -> "MTBDDFloat": ...
    def __mul__(self, other: "MTBDDFloat") -> "MTBDDFloat": ...
    def __iadd__(self, other: "MTBDDFloat") -> "MTBDDFloat": ...
    def __isub__(self, other: "MTBDDFloat") -> "MTBDDFloat": ...
    def __imul__(self, other: "MTBDDFloat") -> "MTBDDFloat": ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...

    def save_svg_str(self, draw_zero: bool = True) -> str:
        """Export this MTBDD as an SVG string."""
        ...

    def save_svg_file(self, stream: IO, draw_zero: bool = True) -> None:
        """Export this MTBDD as SVG to a text stream."""
        ...

    def export_binary_bytes(self) -> bytes:
        """Export this MTBDD as binary bytes."""
        ...

    @staticmethod
    def import_binary_bytes(data: bytes) -> "MTBDDFloat":
        """Import an MTBDD from binary bytes."""
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


class MTBDDInt:
    """Multi-Terminal BDD with int64 terminal values."""

    def __init__(self) -> None:
        """Create a zero-terminal MTBDD."""
        ...

    @staticmethod
    def terminal(value: int) -> "MTBDDInt":
        """Create a terminal node with the given value."""
        ...

    @staticmethod
    def ite(var: int, high: "MTBDDInt", low: "MTBDDInt") -> "MTBDDInt":
        """Create a node: if var then high else low."""
        ...

    @staticmethod
    def from_bdd(bdd: "BDD", zero_val: int = 0, one_val: int = 1) -> "MTBDDInt":
        """Convert a BDD to MTBDD."""
        ...

    @staticmethod
    def zero_terminal() -> "MTBDDInt":
        """Return the zero-terminal MTBDD."""
        ...

    @staticmethod
    def min(a: "MTBDDInt", b: "MTBDDInt") -> "MTBDDInt":
        """Element-wise minimum."""
        ...

    @staticmethod
    def max(a: "MTBDDInt", b: "MTBDDInt") -> "MTBDDInt":
        """Element-wise maximum."""
        ...

    def terminal_value(self) -> int:
        """Return the terminal value."""
        ...

    def evaluate(self, assignment: List[int]) -> int:
        """Evaluate the MTBDD for the given assignment."""
        ...

    def ite_cond(self, then_case: "MTBDDInt", else_case: "MTBDDInt") -> "MTBDDInt":
        """ITE: if this (condition) then then_case else else_case."""
        ...

    @property
    def node_id(self) -> int: ...
    @property
    def is_terminal(self) -> bool: ...
    @property
    def is_zero(self) -> bool: ...
    @property
    def is_one(self) -> bool: ...
    @property
    def top_var(self) -> int: ...
    @property
    def raw_size(self) -> int: ...

    def __add__(self, other: "MTBDDInt") -> "MTBDDInt": ...
    def __sub__(self, other: "MTBDDInt") -> "MTBDDInt": ...
    def __mul__(self, other: "MTBDDInt") -> "MTBDDInt": ...
    def __iadd__(self, other: "MTBDDInt") -> "MTBDDInt": ...
    def __isub__(self, other: "MTBDDInt") -> "MTBDDInt": ...
    def __imul__(self, other: "MTBDDInt") -> "MTBDDInt": ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...

    def save_svg_str(self, draw_zero: bool = True) -> str:
        """Export this MTBDD as an SVG string."""
        ...

    def save_svg_file(self, stream: IO, draw_zero: bool = True) -> None:
        """Export this MTBDD as SVG to a text stream."""
        ...

    def export_binary_bytes(self) -> bytes:
        """Export this MTBDD as binary bytes."""
        ...

    @staticmethod
    def import_binary_bytes(data: bytes) -> "MTBDDInt":
        """Import an MTBDD from binary bytes."""
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


class MTZDDFloat:
    """Multi-Terminal ZDD with double (float) terminal values."""

    def __init__(self) -> None:
        """Create a zero-terminal MTZDD."""
        ...

    @staticmethod
    def terminal(value: float) -> "MTZDDFloat":
        """Create a terminal node with the given value."""
        ...

    @staticmethod
    def ite(var: int, high: "MTZDDFloat", low: "MTZDDFloat") -> "MTZDDFloat":
        """Create a node: if var then high else low."""
        ...

    @staticmethod
    def from_zdd(zdd: "ZDD", zero_val: float = 0.0, one_val: float = 1.0) -> "MTZDDFloat":
        """Convert a ZDD to MTZDD."""
        ...

    @staticmethod
    def zero_terminal() -> "MTZDDFloat":
        """Return the zero-terminal MTZDD."""
        ...

    @staticmethod
    def min(a: "MTZDDFloat", b: "MTZDDFloat") -> "MTZDDFloat":
        """Element-wise minimum."""
        ...

    @staticmethod
    def max(a: "MTZDDFloat", b: "MTZDDFloat") -> "MTZDDFloat":
        """Element-wise maximum."""
        ...

    def terminal_value(self) -> float:
        """Return the terminal value."""
        ...

    def evaluate(self, assignment: List[int]) -> float:
        """Evaluate the MTZDD for the given assignment."""
        ...

    def ite_cond(self, then_case: "MTZDDFloat", else_case: "MTZDDFloat") -> "MTZDDFloat":
        """ITE: if this (condition) then then_case else else_case."""
        ...

    @property
    def node_id(self) -> int: ...
    @property
    def is_terminal(self) -> bool: ...
    @property
    def is_zero(self) -> bool: ...
    @property
    def is_one(self) -> bool: ...
    @property
    def top_var(self) -> int: ...
    @property
    def raw_size(self) -> int: ...

    def __add__(self, other: "MTZDDFloat") -> "MTZDDFloat": ...
    def __sub__(self, other: "MTZDDFloat") -> "MTZDDFloat": ...
    def __mul__(self, other: "MTZDDFloat") -> "MTZDDFloat": ...
    def __iadd__(self, other: "MTZDDFloat") -> "MTZDDFloat": ...
    def __isub__(self, other: "MTZDDFloat") -> "MTZDDFloat": ...
    def __imul__(self, other: "MTZDDFloat") -> "MTZDDFloat": ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...

    def save_svg_str(self, draw_zero: bool = True) -> str:
        """Export this MTZDD as an SVG string."""
        ...

    def save_svg_file(self, stream: IO, draw_zero: bool = True) -> None:
        """Export this MTZDD as SVG to a text stream."""
        ...

    def export_binary_bytes(self) -> bytes:
        """Export this MTZDD as binary bytes."""
        ...

    @staticmethod
    def import_binary_bytes(data: bytes) -> "MTZDDFloat":
        """Import an MTZDD from binary bytes."""
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...


class MTZDDInt:
    """Multi-Terminal ZDD with int64 terminal values."""

    def __init__(self) -> None:
        """Create a zero-terminal MTZDD."""
        ...

    @staticmethod
    def terminal(value: int) -> "MTZDDInt":
        """Create a terminal node with the given value."""
        ...

    @staticmethod
    def ite(var: int, high: "MTZDDInt", low: "MTZDDInt") -> "MTZDDInt":
        """Create a node: if var then high else low."""
        ...

    @staticmethod
    def from_zdd(zdd: "ZDD", zero_val: int = 0, one_val: int = 1) -> "MTZDDInt":
        """Convert a ZDD to MTZDD."""
        ...

    @staticmethod
    def zero_terminal() -> "MTZDDInt":
        """Return the zero-terminal MTZDD."""
        ...

    @staticmethod
    def min(a: "MTZDDInt", b: "MTZDDInt") -> "MTZDDInt":
        """Element-wise minimum."""
        ...

    @staticmethod
    def max(a: "MTZDDInt", b: "MTZDDInt") -> "MTZDDInt":
        """Element-wise maximum."""
        ...

    def terminal_value(self) -> int:
        """Return the terminal value."""
        ...

    def evaluate(self, assignment: List[int]) -> int:
        """Evaluate the MTZDD for the given assignment."""
        ...

    def ite_cond(self, then_case: "MTZDDInt", else_case: "MTZDDInt") -> "MTZDDInt":
        """ITE: if this (condition) then then_case else else_case."""
        ...

    @property
    def node_id(self) -> int: ...
    @property
    def is_terminal(self) -> bool: ...
    @property
    def is_zero(self) -> bool: ...
    @property
    def is_one(self) -> bool: ...
    @property
    def top_var(self) -> int: ...
    @property
    def raw_size(self) -> int: ...

    def __add__(self, other: "MTZDDInt") -> "MTZDDInt": ...
    def __sub__(self, other: "MTZDDInt") -> "MTZDDInt": ...
    def __mul__(self, other: "MTZDDInt") -> "MTZDDInt": ...
    def __iadd__(self, other: "MTZDDInt") -> "MTZDDInt": ...
    def __isub__(self, other: "MTZDDInt") -> "MTZDDInt": ...
    def __imul__(self, other: "MTZDDInt") -> "MTZDDInt": ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    def __hash__(self) -> int: ...

    def save_svg_str(self, draw_zero: bool = True) -> str:
        """Export this MTZDD as an SVG string."""
        ...

    def save_svg_file(self, stream: IO, draw_zero: bool = True) -> None:
        """Export this MTZDD as SVG to a text stream."""
        ...

    def export_binary_bytes(self) -> bytes:
        """Export this MTZDD as binary bytes."""
        ...

    @staticmethod
    def import_binary_bytes(data: bytes) -> "MTZDDInt":
        """Import an MTZDD from binary bytes."""
        ...

    def show(self) -> None:
        """Display the DD as an SVG diagram."""
        ...
