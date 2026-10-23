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
        assert "RotPiDD: id=" in repr(r)


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
        # value 1 precedes value 2: [1,2,3], [1,3,2], [3,1,2] → 3 perms
        ordered = all_perms.order(1, 2)
        assert ordered.exact_count == 3
        assert sorted(ordered.to_perms()) == [[1, 2, 3], [1, 3, 2], [3, 1, 2]]

    def test_order_is_precedence_not_value_comparison(self):
        from kyotodd import Permutation, RotPiDD
        _setup_rotpidd(3)
        all_perms = RotPiDD(0)
        for p in [[1,2,3], [1,3,2], [2,1,3], [2,3,1], [3,1,2], [3,2,1]]:
            all_perms += kyotodd.rotpidd_from_perm(p)
        # [2,3,1] has pi(1) < pi(2) but lists 2 before 1; [3,1,2] is the opposite.
        assert all_perms.order(1, 2).to_permutations() != [Permutation([2, 3, 1])]
        kept = set(all_perms.order(1, 2).to_permutations())
        assert Permutation([3, 1, 2]) in kept
        assert Permutation([2, 3, 1]) not in kept
        # pi(a) < pi(b) is obtained through the inverse.
        by_value = all_perms.inverse().order(1, 2).inverse()
        assert sorted(by_value.to_perms()) == [[1, 2, 3], [1, 3, 2], [2, 3, 1]]


class TestRotPiDDCofact:
    def test_cofact(self):
        _setup_rotpidd(3)
        all_perms = kyotodd.RotPiDD(0)
        for p in [[1,2,3], [1,3,2], [2,1,3], [2,3,1], [3,1,2], [3,2,1]]:
            all_perms += kyotodd.rotpidd_from_perm(p)
        # cofact(1, 2): perms where position 1 has value 2 → 2 perms
        cf = all_perms.cofact(1, 2)
        assert cf.exact_count == 2

    def test_cofact_matches_brute_force(self):
        import itertools
        from kyotodd import Permutation, RotPiDD
        _setup_rotpidd(4)
        perms = [list(p) for p in itertools.permutations([1, 2, 3, 4])]
        all_perms = RotPiDD(0)
        for p in perms:
            all_perms += kyotodd.rotpidd_from_perm(p)
        for u in range(1, 5):
            for v in range(1, 5):
                # delete position u and value v, shift the rest down
                ref = RotPiDD(0)
                for p in perms:
                    if p[u - 1] != v:
                        continue
                    q = [x - 1 if x > v else x for i, x in enumerate(p) if i + 1 != u]
                    ref += RotPiDD.from_permutation(Permutation(q))
                assert all_perms.cofact(u, v) == ref
                assert all_perms.cofact(u, v).exact_count == 6

    def test_cofact_single_permutation(self):
        from kyotodd import Permutation, RotPiDD
        _setup_rotpidd(5)
        b = RotPiDD.from_permutation(Permutation([1, 5, 3, 2, 4]))
        assert b.cofact(2, 5).to_permutations() == [Permutation([1, 3, 2, 4])]
        assert b.cofact(3, 3).to_permutations() == [Permutation([1, 4, 2, 3])]
        assert b.cofact(5, 4).to_permutations() == [Permutation([1, 4, 3, 2])]
        assert b.cofact(2, 3) == RotPiDD(0)
        assert b.cofact(3, 5) == RotPiDD(0)


class TestRotPiDDInsert:
    @staticmethod
    def _insert(w, p, v):
        """Insert value v at position p: positions >= p and values >= v shift up."""
        w = list(w) + list(range(len(w) + 1, max(len(w), p - 1, v - 1) + 1))
        r = []
        for i, x in enumerate(w):
            if i + 1 == p:
                r.append(v)
            r.append(x + 1 if x >= v else x)
        if p == len(w) + 1:
            r.append(v)
        return r

    def test_insert_single_permutation(self):
        from kyotodd import Permutation, RotPiDD
        _setup_rotpidd(5)
        a = RotPiDD.from_permutation(Permutation([2, 1, 3]))
        assert a.insert(2, 4).to_permutations() == [Permutation([2, 4, 1, 3])]
        assert RotPiDD(1).insert(1, 4).to_permutations() == [Permutation([4, 1, 2, 3])]
        assert RotPiDD(1).insert(4, 1).to_permutations() == [Permutation([2, 3, 4, 1])]
        b = RotPiDD.from_permutation(Permutation([1, 3, 2]))
        assert b.insert(1, 4).to_permutations() == [Permutation([4, 1, 3, 2])]
        # cofact(p, v) undoes insert(p, v)
        for p in range(1, 5):
            for v in range(1, 5):
                assert b.insert(p, v).cofact(p, v) == b

    def test_insert_matches_brute_force(self):
        import itertools
        from kyotodd import Permutation, RotPiDD
        _setup_rotpidd(5)
        perms = [list(p) for p in itertools.permutations([1, 2, 3, 4])]
        all_perms = RotPiDD(0)
        for p in perms:
            all_perms += kyotodd.rotpidd_from_perm(p)
        for p in range(1, 6):
            for v in range(1, 6):
                ref = RotPiDD(0)
                for w in perms:
                    ref += RotPiDD.from_permutation(Permutation(self._insert(w, p, v)))
                assert all_perms.insert(p, v) == ref
                assert all_perms.insert(p, v).exact_count == 24
                assert all_perms.insert(p, v).cofact(p, v) == all_perms

    def test_insert_builds_symmetric_group(self):
        from kyotodd import RotPiDD
        _setup_rotpidd(6)
        all_perms = RotPiDD(1)
        fact = 1
        for n in range(1, 6):
            nxt = RotPiDD(0)
            for p in range(1, n + 2):
                nxt += all_perms.insert(p, n + 1)
            fact *= n + 1
            assert nxt.exact_count == fact
            all_perms = nxt

    def test_insert_out_of_range(self):
        import itertools
        from kyotodd import RotPiDD
        _setup_rotpidd(3)
        with pytest.raises(ValueError):
            RotPiDD(1).insert(0, 1)
        with pytest.raises(ValueError):
            RotPiDD(1).insert(1, 4)
        # S_3 uses all three elements, so inserting inside needs a fourth one.
        s3 = RotPiDD(0)
        for p in itertools.permutations([1, 2, 3]):
            s3 += kyotodd.rotpidd_from_perm(list(p))
        with pytest.raises(ValueError):
            s3.insert(1, 1)


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


class TestRotPiDDExactCountPrecision:
    def test_exact_count_matches_zdd(self):
        _setup_rotpidd(3)
        all_perms = kyotodd.RotPiDD(0)
        for p in [[1,2,3], [1,3,2], [2,1,3], [2,3,1], [3,1,2], [3,2,1]]:
            all_perms += kyotodd.rotpidd_from_perm(p)
        assert all_perms.exact_count == all_perms.zdd.exact_count

    def test_exact_count_is_python_int(self):
        _setup_rotpidd(3)
        r = kyotodd.RotPiDD(1)
        assert isinstance(r.exact_count, int)


class TestRotPiDDFromZDD:
    def test_roundtrip(self):
        _setup_rotpidd(3)
        a = kyotodd.rotpidd_from_perm([2, 3, 1])
        z = a.zdd
        b = kyotodd.RotPiDD(z)
        assert a == b

    def test_identity_roundtrip(self):
        _setup_rotpidd(3)
        a = kyotodd.RotPiDD(1)
        b = kyotodd.RotPiDD(a.zdd)
        assert a == b


class TestRotPiDDCount:
    def test_count_matches_exact_count(self):
        _setup_rotpidd(3)
        s = (kyotodd.rotpidd_from_perm([2, 3, 1])
             + kyotodd.rotpidd_from_perm([1, 3, 2])
             + kyotodd.RotPiDD(1))
        assert s.exact_count == 3
        assert s.count() == 3.0
        assert isinstance(s.count(), float)

    def test_count_terminals(self):
        _setup_rotpidd(3)
        assert kyotodd.RotPiDD(0).count() == 0.0
        assert kyotodd.RotPiDD(1).count() == 1.0
