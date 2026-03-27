"""Tests for PiDD Python bindings."""

import pytest
import kyotodd


@pytest.fixture(autouse=True)
def reset_pidd():
    """Re-initialize the BDD library and PiDD state before each test."""
    # PiDD globals must be reset manually since they are not part of bddinit
    import kyotodd._core as _core
    # Force finalize ignoring live objects, then reinit
    try:
        kyotodd.finalize()
    except RuntimeError:
        pass
    kyotodd.init(1024)
    yield


def _setup_pidd(n):
    """Create n PiDD variables."""
    for _ in range(n):
        kyotodd.pidd_newvar()


class TestPiDDNewVar:
    def test_basic(self):
        assert kyotodd.pidd_var_used() == 0
        v1 = kyotodd.pidd_newvar()
        assert v1 == 1
        v2 = kyotodd.pidd_newvar()
        assert v2 == 2
        assert kyotodd.pidd_var_used() == 2


class TestPiDDConstruction:
    def test_empty(self):
        _setup_pidd(3)
        p = kyotodd.PiDD(0)
        assert p.exact_count == 0

    def test_identity(self):
        _setup_pidd(3)
        p = kyotodd.PiDD(1)
        assert p.exact_count == 1

    def test_repr(self):
        _setup_pidd(3)
        p = kyotodd.PiDD(1)
        assert "PiDD: id=" in repr(p)


class TestPiDDSetOperations:
    def test_union(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = a.swap(2, 1)
        c = a + b
        assert c.exact_count == 2

    def test_intersection(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = a.swap(2, 1)
        c = (a + b) & a
        assert c.exact_count == 1
        assert c == a

    def test_difference(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = a.swap(2, 1)
        c = (a + b) - a
        assert c.exact_count == 1
        assert c == b


class TestPiDDSwap:
    def test_swap_identity(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        assert a.swap(1, 1) == a

    def test_swap_involution(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = a.swap(2, 1)
        c = b.swap(2, 1)
        assert c == a


class TestPiDDComposition:
    def test_identity_composition(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = a.swap(3, 1)
        assert a * b == b
        assert b * a == b

    def test_composition(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        s12 = a.swap(2, 1)
        s23 = a.swap(3, 2)
        comp = s23 * s12
        assert comp.exact_count == 1


class TestPiDDOddEven:
    def test_identity_is_even(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        assert a.odd().exact_count == 0
        assert a.even().exact_count == 1

    def test_single_swap_is_odd(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1).swap(2, 1)
        assert a.odd().exact_count == 1
        assert a.even().exact_count == 0


class TestPiDDCofact:
    def test_cofact_basic(self):
        _setup_pidd(3)
        # identity + swap(2,1) = {[1,2,3], [2,1,3]}
        a = kyotodd.PiDD(1) + kyotodd.PiDD(1).swap(2, 1)
        # cofact(2, 1): perms with position 2 = value 1 → {[2,1,3]}
        cf = a.cofact(2, 1)
        assert cf.exact_count == 1


class TestPiDDProperties:
    def test_properties(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1).swap(3, 1)
        assert a.size >= 1
        assert a.exact_count == 1
        assert isinstance(a.zdd, kyotodd.ZDD)


class TestPiDDEquality:
    def test_eq(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = kyotodd.PiDD(1)
        assert a == b

    def test_ne(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = a.swap(2, 1)
        assert a != b

    def test_hash(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = kyotodd.PiDD(1)
        assert hash(a) == hash(b)


class TestPiDDExactCountPrecision:
    def test_exact_count_matches_zdd(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1) + kyotodd.PiDD(1).swap(2, 1)
        assert a.exact_count == a.zdd.exact_count

    def test_exact_count_is_python_int(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        assert isinstance(a.exact_count, int)


class TestPiDDFromZDD:
    def test_roundtrip(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1).swap(2, 1)
        z = a.zdd
        b = kyotodd.PiDD(z)
        assert a == b

    def test_identity_roundtrip(self):
        _setup_pidd(3)
        a = kyotodd.PiDD(1)
        b = kyotodd.PiDD(a.zdd)
        assert a == b
