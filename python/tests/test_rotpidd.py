"""Tests for RotPiDD Python bindings."""

import pytest
import kyotodd


@pytest.fixture(autouse=True)
def reset_rotpidd():
    """Re-initialize the BDD library and RotPiDD state before each test."""
    try:
        kyotodd.finalize()
    except RuntimeError:
        pass
    kyotodd.init(1024)
    yield


def _setup_rotpidd(n):
    """Create n RotPiDD variables."""
    for _ in range(n):
        kyotodd.rotpidd_newvar()


class TestRotPiDDNewVar:
    def test_basic(self):
        assert kyotodd.rotpidd_var_used() == 0
        v1 = kyotodd.rotpidd_newvar()
        assert v1 == 1
        v2 = kyotodd.rotpidd_newvar()
        assert v2 == 2
        assert kyotodd.rotpidd_var_used() == 2


class TestRotPiDDConstruction:
    def test_empty(self):
        _setup_rotpidd(3)
        r = kyotodd.RotPiDD(0)
        assert r.exact_count == 0

    def test_identity(self):
        _setup_rotpidd(3)
        r = kyotodd.RotPiDD(1)
        assert r.exact_count == 1

    def test_repr(self):
        _setup_rotpidd(3)
        r = kyotodd.RotPiDD(1)
        assert "node_id=" in repr(r)


class TestRotPiDDFromPerm:
    def test_single_perm(self):
        _setup_rotpidd(4)
        r = kyotodd.rotpidd_from_perm([2, 3, 1, 4])
        assert r.exact_count == 1
        perms = r.to_perms()
        assert len(perms) == 1
        assert perms[0] == [2, 3, 1, 4]

    def test_identity_perm(self):
        _setup_rotpidd(3)
        r = kyotodd.rotpidd_from_perm([1, 2, 3])
        assert r == kyotodd.RotPiDD(1)


class TestRotPiDDSetOperations:
    def test_union(self):
        _setup_rotpidd(3)
        a = kyotodd.rotpidd_from_perm([1, 2, 3])
        b = kyotodd.rotpidd_from_perm([2, 1, 3])
        c = a + b
        assert c.exact_count == 2

    def test_intersection(self):
        _setup_rotpidd(3)
        a = kyotodd.rotpidd_from_perm([1, 2, 3])
        b = kyotodd.rotpidd_from_perm([2, 1, 3])
        u = a + b
        assert (u & a).exact_count == 1
        assert (u & a) == a

    def test_difference(self):
        _setup_rotpidd(3)
        a = kyotodd.rotpidd_from_perm([1, 2, 3])
        b = kyotodd.rotpidd_from_perm([2, 1, 3])
        u = a + b
        assert (u - a) == b

    def test_inplace(self):
        _setup_rotpidd(3)
        a = kyotodd.rotpidd_from_perm([1, 2, 3])
        b = kyotodd.rotpidd_from_perm([2, 1, 3])
        a += b
        assert a.exact_count == 2


class TestRotPiDDLeftRot:
    def test_trivial(self):
        _setup_rotpidd(3)
        r = kyotodd.RotPiDD(1)
        assert r.left_rot(2, 2) == r

    def test_left_rot_involution(self):
        _setup_rotpidd(3)
        r = kyotodd.RotPiDD(1)
        s = r.left_rot(2, 1)
        assert s.left_rot(2, 1) == r


class TestRotPiDDComposition:
    def test_identity_composition(self):
        _setup_rotpidd(3)
        identity = kyotodd.RotPiDD(1)
        p = kyotodd.rotpidd_from_perm([2, 3, 1])
        assert identity * p == p
        assert p * identity == p

    def test_inverse_composition(self):
        _setup_rotpidd(3)
        # [2,3,1] and [3,1,2] are inverses
        p1 = kyotodd.rotpidd_from_perm([2, 3, 1])
        p2 = kyotodd.rotpidd_from_perm([3, 1, 2])
        identity = kyotodd.RotPiDD(1)
        assert (p2 * p1) == identity

    def test_set_composition(self):
        _setup_rotpidd(3)
        a = kyotodd.rotpidd_from_perm([1, 2, 3]) + kyotodd.rotpidd_from_perm([2, 1, 3])
        b = kyotodd.rotpidd_from_perm([1, 2, 3]) + kyotodd.rotpidd_from_perm([1, 3, 2])
        comp = b * a
        assert comp.exact_count == 4


class TestRotPiDDSwap:
    def test_swap(self):
        _setup_rotpidd(3)
        r = kyotodd.RotPiDD(1)
        s = r.swap(1, 3)
        perms = s.to_perms()
        assert len(perms) == 1
        assert perms[0] == [3, 2, 1]


class TestRotPiDDReverse:
    def test_reverse(self):
        _setup_rotpidd(4)
        r = kyotodd.RotPiDD(1)
        s = r.reverse(1, 4)
        perms = s.to_perms()
        assert len(perms) == 1
        assert perms[0] == [4, 3, 2, 1]


class TestRotPiDDOddEven:
    def test_parity(self):
        _setup_rotpidd(3)
        # All 6 permutations of [1,2,3]
        all_perms = kyotodd.RotPiDD(0)
        for p in [[1,2,3], [1,3,2], [2,1,3], [2,3,1], [3,1,2], [3,2,1]]:
            all_perms += kyotodd.rotpidd_from_perm(p)
        assert all_perms.exact_count == 6
        assert all_perms.odd().exact_count == 3
        assert all_perms.even().exact_count == 3


class TestRotPiDDInverse:
    def test_inverse(self):
        _setup_rotpidd(4)
        p = kyotodd.rotpidd_from_perm([3, 1, 4, 2])
        inv = p.inverse()
        identity = kyotodd.RotPiDD(1)
        assert (p * inv) == identity


class TestRotPiDDOrder:
    def test_order(self):
        _setup_rotpidd(3)
        all_perms = kyotodd.RotPiDD(0)
        for p in [[1,2,3], [1,3,2], [2,1,3], [2,3,1], [3,1,2], [3,2,1]]:
            all_perms += kyotodd.rotpidd_from_perm(p)
        # pi(1) < pi(2): [1,2,3], [1,3,2], [2,3,1] → 3 perms
        ordered = all_perms.order(1, 2)
        assert ordered.exact_count == 3


class TestRotPiDDCofact:
    def test_cofact(self):
        _setup_rotpidd(3)
        all_perms = kyotodd.RotPiDD(0)
        for p in [[1,2,3], [1,3,2], [2,1,3], [2,3,1], [3,1,2], [3,2,1]]:
            all_perms += kyotodd.rotpidd_from_perm(p)
        # cofact(1, 2): perms where position 1 has value 2 → 2 perms
        cf = all_perms.cofact(1, 2)
        assert cf.exact_count == 2


class TestRotPiDDExtractOne:
    def test_extract_one(self):
        _setup_rotpidd(3)
        r = kyotodd.rotpidd_from_perm([2, 3, 1]) + kyotodd.rotpidd_from_perm([3, 1, 2])
        one = r.extract_one()
        assert one.exact_count == 1
        assert (one & r) == one


class TestRotPiDDToPerms:
    def test_roundtrip(self):
        _setup_rotpidd(3)
        perms_in = [[1,2,3], [2,3,1], [3,1,2]]
        r = kyotodd.RotPiDD(0)
        for p in perms_in:
            r += kyotodd.rotpidd_from_perm(p)
        perms_out = r.to_perms()
        assert len(perms_out) == 3
        assert set(map(tuple, perms_out)) == set(map(tuple, perms_in))


class TestRotPiDDProperties:
    def test_properties(self):
        _setup_rotpidd(3)
        r = kyotodd.rotpidd_from_perm([2, 3, 1])
        assert r.size >= 1
        assert r.exact_count == 1
        assert isinstance(r.zdd, kyotodd.ZDD)


class TestRotPiDDEquality:
    def test_eq(self):
        _setup_rotpidd(3)
        a = kyotodd.RotPiDD(1)
        b = kyotodd.RotPiDD(1)
        assert a == b

    def test_ne(self):
        _setup_rotpidd(3)
        a = kyotodd.RotPiDD(1)
        b = kyotodd.rotpidd_from_perm([2, 1, 3])
        assert a != b

    def test_hash(self):
        _setup_rotpidd(3)
        a = kyotodd.RotPiDD(1)
        b = kyotodd.RotPiDD(1)
        assert hash(a) == hash(b)
