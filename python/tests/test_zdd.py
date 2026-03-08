import kyotodd
from kyotodd import ZDD


def _make_singleton(var):
    """Create a ZDD representing the family {{var}}."""
    z = ZDD(1)  # {{}}
    return z.change(var)


class TestZDDConstruction:
    def test_empty(self):
        z = ZDD(0)
        assert z == ZDD.empty

    def test_single(self):
        z = ZDD(1)
        assert z == ZDD.single

    def test_null(self):
        z = ZDD(-1)
        assert z == ZDD.null

    def test_default_is_empty(self):
        z = ZDD()
        assert z == ZDD.empty


class TestZDDConstants:
    def test_empty_is_not_single(self):
        assert ZDD.empty != ZDD.single

    def test_empty_is_not_null(self):
        assert ZDD.empty != ZDD.null

    def test_single_is_not_null(self):
        assert ZDD.single != ZDD.null


class TestZDDEquality:
    def test_eq_same(self):
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(1)
        assert a == b

    def test_ne_different(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        assert a != b


class TestZDDHash:
    def test_hashable(self):
        kyotodd.newvar()
        a = _make_singleton(1)
        assert isinstance(hash(a), int)

    def test_equal_objects_same_hash(self):
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(1)
        assert hash(a) == hash(b)

    def test_usable_in_set(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        s = {a, b, _make_singleton(1)}
        assert len(s) == 2


class TestZDDRepr:
    def test_repr(self):
        r = repr(ZDD.empty)
        assert r.startswith("ZDD(node_id=")
        assert r.endswith(")")


class TestZDDOperators:
    def _setup(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        return _make_singleton(1), _make_singleton(2), _make_singleton(3)

    def test_union(self):
        a, b, _ = self._setup()
        u = a + b  # {{1}, {2}}
        assert u != a
        assert u != b
        assert u != ZDD.empty

    def test_subtract(self):
        a, b, _ = self._setup()
        u = a + b
        assert (u - a) == b
        assert (u - b) == a

    def test_intersect(self):
        a, b, _ = self._setup()
        u = a + b
        assert (u & a) == a
        assert (a & b) == ZDD.empty

    def test_symdiff(self):
        a, b, _ = self._setup()
        u = a + b
        assert (u ^ a) == b

    def test_join(self):
        a, b, _ = self._setup()
        # {{1}} * {{2}} = {{1,2}}
        j = a * b
        assert j != ZDD.empty
        assert j != a
        assert j != b

    def test_div(self):
        a, b, _ = self._setup()
        # {{1,2}} / {{1}} = {{2}}
        ab = a * b  # {{1,2}}
        assert (ab / a) == b

    def test_remainder(self):
        a, b, _ = self._setup()
        ab = a * b  # {{1,2}}
        u = ab + a  # {{1,2}, {1}}
        # u / a = {{2}, {}} (quotient), u % a = remainder
        q = u / a
        r = u % a
        assert u == (q * a) + r

    def test_union_identity(self):
        a, _, _ = self._setup()
        assert (a + ZDD.empty) == a

    def test_subtract_self(self):
        a, _, _ = self._setup()
        assert (a - a) == ZDD.empty

    def test_intersect_self(self):
        a, _, _ = self._setup()
        assert (a & a) == a


class TestZDDCompoundAssignment:
    def _setup(self):
        kyotodd.newvar()
        kyotodd.newvar()
        return _make_singleton(1), _make_singleton(2)

    def test_iadd(self):
        a, b = self._setup()
        expected = a + b
        a += b
        assert a == expected

    def test_isub(self):
        a, b = self._setup()
        u = a + b
        expected = u - a
        u -= a
        assert u == expected

    def test_iand(self):
        a, b = self._setup()
        u = a + b
        expected = u & a
        u &= a
        assert u == expected

    def test_ixor(self):
        a, b = self._setup()
        expected = a ^ b
        a ^= b
        assert a == expected

    def test_imul(self):
        a, b = self._setup()
        expected = a * b
        a *= b
        assert a == expected

    def test_itruediv(self):
        a, b = self._setup()
        ab = a * b
        expected = ab / a
        ab /= a
        assert ab == expected

    def test_imod(self):
        a, b = self._setup()
        ab = a * b
        u = ab + a
        expected = u % a
        u %= a
        assert u == expected


class TestZDDProperties:
    def test_node_id(self):
        assert isinstance(ZDD.empty.node_id, int)

    def test_top_var(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(2)
        assert a.top_var == 2
