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

    def test_card_empty(self):
        assert ZDD.empty.card == 0

    def test_card_single(self):
        assert ZDD.single.card == 1

    def test_card_family(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}
        assert u.card == 2


class TestZDDMethods:
    def _setup(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        return _make_singleton(1), _make_singleton(2), _make_singleton(3)

    def test_offset(self):
        a, b, _ = self._setup()
        ab = a * b  # {{1,2}}
        u = ab + b  # {{1,2}, {2}}
        # offset(1): sets NOT containing 1 → {{2}}
        assert u.offset(1) == b
        # {{1,2}} has no sets without 1 → empty
        assert ab.offset(1) == ZDD.empty

    def test_onset(self):
        a, b, _ = self._setup()
        ab = a * b  # {{1,2}}
        u = ab + b  # {{1,2}, {2}}
        # onset(1): sets containing 1, keep var → {{1,2}}
        assert u.onset(1) == ab

    def test_onset0(self):
        a, b, _ = self._setup()
        ab = a * b  # {{1,2}}
        # onset0(1): sets containing 1, remove var 1 → {{2}}
        assert ab.onset0(1) == b

    def test_change(self):
        _, _, _ = self._setup()
        s = ZDD.single  # {{}}
        # change(1): toggle var 1 → {{1}}
        s1 = s.change(1)
        assert s1 == _make_singleton(1)
        # change(1) again: toggle back → {{}}
        assert s1.change(1) == ZDD.single

    def test_maximal(self):
        a, b, _ = self._setup()
        ab = a * b  # {{1,2}}
        u = ab + a  # {{1,2}, {1}}
        # maximal: {{1,2}} (remove {1} since {1} ⊂ {1,2})
        assert u.maximal() == ab

    def test_minimal(self):
        a, b, _ = self._setup()
        ab = a * b  # {{1,2}}
        u = ab + a  # {{1,2}, {1}}
        # minimal: {{1}} (remove {1,2} since {1} ⊂ {1,2})
        assert u.minimal() == a

    def test_closure(self):
        a, b, _ = self._setup()
        # closure = pairwise intersection closure
        # closure({{1}, {2}}) = {{1}, {2}, {}} since {1}∩{2} = {}
        u = a + b  # {{1}, {2}}
        c = u.closure()
        assert c.card == 3  # {{1}, {2}, {}}

    def test_minhit(self):
        a, b, _ = self._setup()
        u = a + b  # {{1}, {2}}
        # minimum hitting set: must hit both {1} and {2} → {{1,2}}
        h = u.minhit()
        assert h == (a * b)

    def test_restrict(self):
        a, b, c = self._setup()
        ab = a * b   # {{1,2}}
        ac = a * c   # {{1,3}}
        u = ab + ac  # {{1,2}, {1,3}}
        # restrict by {{1,2}}: keep sets that are subsets of {1,2}
        assert u.restrict(ab) == ab

    def test_permit(self):
        a, b, c = self._setup()
        ab = a * b   # {{1,2}}
        # permit(F, G): keep sets A in F where A ⊆ some B in G
        # permit({{1}, {2}, {1,2}}, {{1,2}}) = {{1}, {2}, {1,2}}
        f = a + b + ab
        assert f.permit(ab) == f
        # permit({{1,2,3}}, {{1,2}}) = empty ({1,2,3} ⊄ {1,2})
        abc = a * b * c
        assert abc.permit(ab) == ZDD.empty

    def test_nonsup(self):
        a, b, _ = self._setup()
        ab = a * b  # {{1,2}}
        u = ab + a  # {{1,2}, {1}}
        # nonsup(a): remove supersets of {1} → {} (both are supersets of {1})
        assert u.nonsup(a) == ZDD.empty

    def test_nonsub(self):
        a, b, _ = self._setup()
        ab = a * b  # {{1,2}}
        u = ab + a  # {{1,2}, {1}}
        # nonsub(ab): remove subsets of {1,2} → {} (both are subsets of {1,2})
        assert u.nonsub(ab) == ZDD.empty

    def test_disjoin(self):
        a, b, _ = self._setup()
        # disjoin: product restricted to disjoint pairs
        # {{1}} disjoin {{2}} = {{1,2}} (disjoint)
        assert a.disjoin(b) == (a * b)
        # {{1}} disjoin {{1}} = {} (not disjoint)
        assert a.disjoin(a) == ZDD.empty

    def test_delta(self):
        a, b, _ = self._setup()
        # delta: symmetric difference of each pair
        # {{1}} delta {{2}} = {{1,2}} (sym diff of {1} and {2})
        d = a.delta(b)
        assert d != ZDD.empty
