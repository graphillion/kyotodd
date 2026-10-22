"""Tests for the Permutation class and its PiDD / RotPiDD interop."""

import itertools
import random

import pytest
import kyotodd
from kyotodd import Permutation, PiDD, RotPiDD


class TestPermutationConstruction:
    def test_from_list(self):
        p = Permutation([2, 3, 1])
        assert len(p) == 3
        assert p.to_list() == [2, 3, 1]

    def test_default_is_empty_identity(self):
        p = Permutation()
        assert len(p) == 0
        assert p.to_list() == []
        assert p == Permutation.identity(0)
        assert p == Permutation.identity(4)

    def test_identity(self):
        assert Permutation.identity(3).to_list() == [1, 2, 3]
        with pytest.raises(ValueError):
            Permutation.identity(-1)

    @pytest.mark.parametrize("bad", [[0, 1], [1, 3], [1, 1], [2, 2, 1], [-1, 2]])
    def test_rejects_non_permutations(self, bad):
        with pytest.raises(ValueError):
            Permutation(bad)

    def test_repr_and_str(self):
        p = Permutation([2, 3, 1])
        assert repr(p) == "Permutation([2, 3, 1])"
        assert str(p) == "[2 3 1]"
        assert str(Permutation()) == "[]"


class TestPermutationImage:
    def test_call(self):
        p = Permutation([2, 3, 1])
        assert (p(1), p(2), p(3)) == (2, 3, 1)

    def test_fixed_points_beyond_size(self):
        p = Permutation([2, 1])
        assert p(3) == 3
        assert p(100) == 100

    def test_rejects_non_positive(self):
        p = Permutation([2, 1])
        with pytest.raises(IndexError):
            p(0)


class TestPermutationAlgebra:
    def test_mul_applies_left_first(self):
        p = Permutation([2, 3, 1])
        q = Permutation([1, 3, 2])
        pq = p * q
        assert pq.to_list() == [3, 2, 1]
        assert all(pq(i) == q(p(i)) for i in range(1, 4))
        assert (q * p).to_list() == [2, 1, 3]

    def test_mul_extends_shorter_operand(self):
        p = Permutation([2, 1])
        q = Permutation([1, 2, 4, 3])
        assert (p * q).to_list() == [2, 1, 4, 3]
        assert (p * Permutation()) == p

    def test_inverse(self):
        p = Permutation([3, 1, 4, 2])
        assert p.inverse().to_list() == [2, 4, 1, 3]
        assert p * p.inverse() == Permutation.identity(4)
        assert p.inverse() * p == Permutation.identity(4)

    def test_parity(self):
        assert Permutation().is_even()
        assert Permutation([2, 1]).is_odd()
        assert Permutation([2, 3, 1]).is_even()
        assert Permutation([2, 3, 4, 1]).is_odd()
        rng = random.Random(5)
        for _ in range(20):
            a = list(range(1, 7)); rng.shuffle(a)
            b = list(range(1, 7)); rng.shuffle(b)
            pa, pb = Permutation(a), Permutation(b)
            assert (pa * pb).is_even() == (pa.is_even() == pb.is_even())

    def test_equality_ignores_trailing_fixed_points(self):
        assert Permutation([2, 1]) == Permutation([2, 1, 3])
        assert Permutation([2, 1]) != Permutation([2, 1, 4, 3])
        assert hash(Permutation([2, 1])) == hash(Permutation([2, 1, 3]))
        assert len({Permutation([2, 1]), Permutation([2, 1, 3]), Permutation([1, 2])}) == 2

    def test_ordering(self):
        assert Permutation([1, 2, 3]) < Permutation([1, 3, 2])
        assert not (Permutation([2, 1]) < Permutation([2, 1, 3]))
        perms = sorted(Permutation(list(p)) for p in itertools.permutations([1, 2, 3]))
        assert [p.to_list() for p in perms] == [list(p) for p in itertools.permutations([1, 2, 3])]


class TestPiDDInterop:
    def test_round_trip(self):
        for _ in range(4):
            kyotodd.pidd_newvar()
        for v in itertools.permutations([1, 2, 3, 4]):
            p = Permutation(list(v))
            d = PiDD.from_permutation(p)
            assert d.exact_count == 1
            perms = d.to_permutations()
            assert perms == [p]
            assert perms[0].to_list() == list(v)

    def test_shorter_than_var_count(self):
        for _ in range(5):
            kyotodd.pidd_newvar()
        d = PiDD.from_permutation(Permutation([2, 1]))
        assert d == PiDD(1).swap(2, 1)
        perms = d.to_permutations()
        assert len(perms[0]) == 5
        assert perms[0] == Permutation([2, 1])
        assert PiDD.from_permutation(Permutation()) == PiDD(1)

    def test_too_large_raises(self):
        for _ in range(3):
            kyotodd.pidd_newvar()
        with pytest.raises(ValueError):
            PiDD.from_permutation(Permutation([2, 3, 4, 1]))

    def test_set_conversion(self):
        for _ in range(3):
            kyotodd.pidd_newvar()
        allp = PiDD(0)
        for v in itertools.permutations([1, 2, 3]):
            allp += PiDD.from_permutation(Permutation(list(v)))
        assert allp.exact_count == 6
        perms = allp.to_permutations()
        assert len(perms) == 6
        assert len(set(perms)) == 6
        assert PiDD(0).to_permutations() == []

    def test_mul_matches_permutation_mul(self):
        for _ in range(5):
            kyotodd.pidd_newvar()
        rng = random.Random(11)
        for _ in range(20):
            a = list(range(1, 6)); rng.shuffle(a)
            b = list(range(1, 6)); rng.shuffle(b)
            pa, pb = Permutation(a), Permutation(b)
            A, B = PiDD.from_permutation(pa), PiDD.from_permutation(pb)
            assert A * B == PiDD.from_permutation(pa * pb)
            assert (A.odd() != PiDD(0)) == pa.is_odd()


class TestRotPiDDInterop:
    def test_round_trip(self):
        for _ in range(4):
            kyotodd.rotpidd_newvar()
        for v in itertools.permutations([1, 2, 3, 4]):
            p = Permutation(list(v))
            d = RotPiDD.from_permutation(p)
            assert d == kyotodd.rotpidd_from_perm(list(v))
            perms = d.to_permutations()
            assert perms == [p]
            assert [q.to_list() for q in perms] == d.to_perms()

    def test_too_large_raises(self):
        for _ in range(3):
            kyotodd.rotpidd_newvar()
        with pytest.raises(ValueError):
            RotPiDD.from_permutation(Permutation([2, 3, 4, 1]))

    def test_mul_and_inverse_match_permutation(self):
        for _ in range(5):
            kyotodd.rotpidd_newvar()
        rng = random.Random(12)
        for _ in range(20):
            a = list(range(1, 6)); rng.shuffle(a)
            b = list(range(1, 6)); rng.shuffle(b)
            pa, pb = Permutation(a), Permutation(b)
            A, B = RotPiDD.from_permutation(pa), RotPiDD.from_permutation(pb)
            assert A * B == RotPiDD.from_permutation(pa * pb)
            assert A.inverse() == RotPiDD.from_permutation(pa.inverse())
            assert (A.odd() != RotPiDD(0)) == pa.is_odd()
        assert RotPiDD(0).to_permutations() == []
