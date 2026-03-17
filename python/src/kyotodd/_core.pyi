from typing import List, Union

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
            v: Variable number (must have been created with newvar()).

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
    def import_file(path: str) -> BDD:
        """Import a BDD from a file.

        Args:
            path: File path to read from.

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
        """Return string representation: BDD(node_id=...)."""
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

    def export_file(self, path: str) -> None:
        """Export this BDD to a file.

        Args:
            path: File path to write to.
        """
        ...

    @property
    def size(self) -> int:
        """The number of nodes in the DAG of this BDD."""
        ...
    @property
    def top_var(self) -> int:
        """The top (root) variable number of this BDD."""
        ...
    @property
    def node_id(self) -> int:
        """The raw node ID of this BDD."""
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
    def import_file(path: str) -> ZDD:
        """Import a ZDD from a file.

        Args:
            path: File path to read from.

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
        """Return string representation: ZDD(node_id=...)."""
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
        """Select sets containing variable v, then remove v.

        Args:
            v: Variable number.

        Returns:
            The resulting ZDD.
        """
        ...

    def onset0(self, v: int) -> ZDD:
        """Select sets NOT containing variable v (same as offset).

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

    def nonsup(self, g: ZDD) -> ZDD:
        """Remove sets that are supersets of some set in g.

        Args:
            g: The constraining family.

        Returns:
            The resulting ZDD.
        """
        ...

    def nonsub(self, g: ZDD) -> ZDD:
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

    def jointjoin(self, g: ZDD) -> ZDD:
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

    def export_str(self) -> str:
        """Export this ZDD to a string representation."""
        ...

    def export_file(self, path: str) -> None:
        """Export this ZDD to a file.

        Args:
            path: File path to write to.
        """
        ...

    @property
    def card(self) -> int:
        """The number of sets in the family (cardinality)."""
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
    def size(self) -> int:
        """The number of nodes in the DAG of this ZDD."""
        ...
    @property
    def lit(self) -> int:
        """The total literal count across all sets in the family."""
        ...
    @property
    def len(self) -> int:
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


def init(node_count: int = 256, node_max: int = ...) -> None:
    """Initialize the BDD library.

    Args:
        node_count: Initial number of node slots to allocate.
        node_max: Maximum number of node slots allowed.
    """
    ...

def newvar() -> int:
    """Create a new variable and return its variable number."""
    ...

def newvar_of_level(lev: int) -> int:
    """Create a new variable at the specified level.

    Args:
        lev: The level to insert the new variable at.

    Returns:
        The variable number of the newly created variable.
    """
    ...

def level_of_var(var: int) -> int:
    """Return the level of the given variable.

    Args:
        var: Variable number.
    """
    ...

def var_of_level(level: int) -> int:
    """Return the variable at the given level.

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
    """Return the total number of nodes currently allocated."""
    ...

def zdd_random(lev: int, density: int = 50) -> ZDD:
    """Generate a random ZDD over the lowest lev levels.

    Args:
        lev: Number of variable levels to use.
        density: Probability (0-100) for each terminal to be 1 (default: 50).

    Returns:
        A random ZDD.
    """
    ...
