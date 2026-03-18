"""Tests for SeqBDD Python bindings."""

import pytest
import kyotodd


@pytest.fixture(autouse=True)
def reset():
    """Re-initialize the BDD library before each test."""
    try:
        kyotodd.finalize()
    except RuntimeError:
        pass
    kyotodd.init(1024)
    yield


def _vars(n):
    """Create n BDD variables and return their numbers."""
    return [kyotodd.newvar() for _ in range(n)]


class TestSeqBDDConstruction:
    def test_empty(self):
        s = kyotodd.SeqBDD(0)
        assert s.card == 0

    def test_epsilon(self):
        s = kyotodd.SeqBDD(1)
        assert s.card == 1

    def test_default(self):
        s = kyotodd.SeqBDD()
        assert s.card == 0

    def test_repr(self):
        s = kyotodd.SeqBDD(1)
        assert repr(s) == "SeqBDD(card=1)"

    def test_equality(self):
        a = kyotodd.SeqBDD(0)
        b = kyotodd.SeqBDD(0)
        c = kyotodd.SeqBDD(1)
        assert a == b
        assert a != c

    def test_hash(self):
        a = kyotodd.SeqBDD(0)
        b = kyotodd.SeqBDD(0)
        assert hash(a) == hash(b)

    def test_bool_raises(self):
        s = kyotodd.SeqBDD(0)
        with pytest.raises(TypeError):
            bool(s)


class TestSeqBDDFromList:
    def test_single_element(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        assert s.card == 1
        assert s.top == v1

    def test_multiple_elements(self):
        v1, v2, v3 = _vars(3)
        s = kyotodd.SeqBDD.from_list([v1, v2, v3])
        assert s.card == 1
        assert s.len == 3

    def test_empty_list(self):
        s = kyotodd.SeqBDD.from_list([])
        assert s == kyotodd.SeqBDD(1)  # epsilon

    def test_repeated_variable(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1, v1])
        assert s.card == 1
        assert s.len == 2


class TestSeqBDDPush:
    def test_push_on_epsilon(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD(1).push(v1)
        assert s.card == 1
        assert s.top == v1

    def test_push_on_empty(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD(0).push(v1)
        assert s == kyotodd.SeqBDD(0)


class TestSeqBDDSetOps:
    def test_union(self):
        v1, v2 = _vars(2)
        a = kyotodd.SeqBDD.from_list([v1])
        b = kyotodd.SeqBDD.from_list([v2])
        u = a + b
        assert u.card == 2

    def test_intersection(self):
        v1, v2 = _vars(2)
        a = kyotodd.SeqBDD.from_list([v1])
        b = kyotodd.SeqBDD.from_list([v2])
        u = a + b
        assert (u & a) == a

    def test_difference(self):
        v1, v2 = _vars(2)
        a = kyotodd.SeqBDD.from_list([v1])
        b = kyotodd.SeqBDD.from_list([v2])
        u = a + b
        assert (u - a) == b

    def test_inplace_union(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1])
        s += kyotodd.SeqBDD.from_list([v2])
        assert s.card == 2


class TestSeqBDDOnSetOffSet:
    def test_on_set0_matching(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1, v2])
        r = s.on_set0(v1)
        assert r == kyotodd.SeqBDD.from_list([v2])

    def test_on_set0_non_matching(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1, v2])
        r = s.on_set0(v2)
        assert r == kyotodd.SeqBDD(0)

    def test_on_set(self):
        v1, v2 = _vars(2)
        a = kyotodd.SeqBDD.from_list([v1])
        b = kyotodd.SeqBDD.from_list([v2])
        s = a + b
        assert s.on_set(v1) == a

    def test_off_set(self):
        v1, v2 = _vars(2)
        a = kyotodd.SeqBDD.from_list([v1])
        b = kyotodd.SeqBDD.from_list([v2])
        s = a + b
        assert s.off_set(v1) == b


class TestSeqBDDConcatenation:
    def test_epsilon_left(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        assert kyotodd.SeqBDD(1) * s == s

    def test_epsilon_right(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        assert s * kyotodd.SeqBDD(1) == s

    def test_empty_left(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        assert kyotodd.SeqBDD(0) * s == kyotodd.SeqBDD(0)

    def test_two_singletons(self):
        v1, v2 = _vars(2)
        a = kyotodd.SeqBDD.from_list([v1])
        b = kyotodd.SeqBDD.from_list([v2])
        r = a * b
        assert r == kyotodd.SeqBDD.from_list([v1, v2])

    def test_spec_example(self):
        """(ab + c) * (bd + 1) = {ab, abbd, c, cbd}"""
        a, b, c, d = _vars(4)
        ab = kyotodd.SeqBDD.from_list([a, b])
        sc = kyotodd.SeqBDD.from_list([c])
        lhs = ab + sc

        bd = kyotodd.SeqBDD.from_list([b, d])
        rhs = bd + kyotodd.SeqBDD(1)

        result = lhs * rhs
        assert result.card == 4

        abbd = kyotodd.SeqBDD.from_list([a, b, b, d])
        cbd = kyotodd.SeqBDD.from_list([c, b, d])
        expected = ab + abbd + sc + cbd
        assert result == expected

    def test_inplace(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1])
        s *= kyotodd.SeqBDD.from_list([v2])
        assert s == kyotodd.SeqBDD.from_list([v1, v2])


class TestSeqBDDLeftQuotient:
    def test_by_epsilon(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        assert s / kyotodd.SeqBDD(1) == s

    def test_by_self(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1, v2])
        assert s / s == kyotodd.SeqBDD(1)

    def test_by_empty(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        with pytest.raises(ValueError):
            s / kyotodd.SeqBDD(0)

    def test_simple(self):
        v1, v2, v3 = _vars(3)
        s = kyotodd.SeqBDD.from_list([v1, v2, v3])
        prefix = kyotodd.SeqBDD.from_list([v1])
        assert s / prefix == kyotodd.SeqBDD.from_list([v2, v3])

    def test_no_match(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1])
        prefix = kyotodd.SeqBDD.from_list([v2])
        assert s / prefix == kyotodd.SeqBDD(0)

    def test_inplace(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1, v2])
        s /= kyotodd.SeqBDD.from_list([v1])
        assert s == kyotodd.SeqBDD.from_list([v2])


class TestSeqBDDRemainder:
    def test_by_epsilon(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        assert s % kyotodd.SeqBDD(1) == kyotodd.SeqBDD(0)

    def test_by_self(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        assert s % s == kyotodd.SeqBDD(0)

    def test_identity(self):
        """f == p * (f / p) + (f % p)"""
        v1, v2, v3 = _vars(3)
        f = kyotodd.SeqBDD.from_list([v1, v2, v3]) + kyotodd.SeqBDD.from_list([v2])
        p = kyotodd.SeqBDD.from_list([v1])
        assert f == p * (f / p) + (f % p)


class TestSeqBDDProperties:
    def test_card(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1]) + kyotodd.SeqBDD.from_list([v2])
        assert s.card == 2

    def test_len(self):
        v1, v2, v3 = _vars(3)
        s = kyotodd.SeqBDD.from_list([v1, v2, v3])
        assert s.len == 3

    def test_top(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1, v2])
        # top should be the highest-level variable
        assert s.top > 0

    def test_zdd(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        z = s.zdd
        assert isinstance(z, kyotodd.ZDD)
        assert z.card == 1


class TestSeqBDDStr:
    def test_empty(self):
        assert str(kyotodd.SeqBDD(0)) == "(empty)"

    def test_epsilon(self):
        assert str(kyotodd.SeqBDD(1)) == "e"

    def test_single_element(self):
        v1, = _vars(1)
        s = kyotodd.SeqBDD.from_list([v1])
        lev = kyotodd.level_of_var(v1)
        assert str(s) == str(lev)

    def test_multi_element(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1, v2])
        lev1 = kyotodd.level_of_var(v1)
        lev2 = kyotodd.level_of_var(v2)
        assert str(s) == f"{lev1} {lev2}"

    def test_multiple_sequences(self):
        v1, v2 = _vars(2)
        s = kyotodd.SeqBDD.from_list([v1]) + kyotodd.SeqBDD.from_list([v2])
        assert " + " in str(s)
