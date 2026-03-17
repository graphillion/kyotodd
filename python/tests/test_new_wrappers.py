import pytest
import kyotodd
from kyotodd import BDD, ZDD


def _make_singleton(var):
    """Create a ZDD representing the family {{var}}."""
    z = ZDD(1)  # {{}}
    return z.change(var)


# ===== BDD new methods =====

class TestBDDSwap:
    def test_swap_basic(self):
        kyotodd.newvar()
        kyotodd.newvar()
        x = BDD.var(1)
        y = BDD.var(2)
        # swap(x, 1, 2) should give y
        assert x.swap(1, 2) == y

    def test_swap_and(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        x, y, z = BDD.var(1), BDD.var(2), BDD.var(3)
        f = x & y  # f(1,2)
        g = f.swap(1, 3)  # f(3,2) = z & y
        assert g == (z & y)

    def test_swap_identity(self):
        kyotodd.newvar()
        x = BDD.var(1)
        # swap(1, 1) is identity
        assert x.swap(1, 1) == x


class TestBDDSmooth:
    def test_smooth_basic(self):
        kyotodd.newvar()
        kyotodd.newvar()
        x, y = BDD.var(1), BDD.var(2)
        f = x & y
        # smooth(x, 1) = exist(x, 1) = (0 & y) | (1 & y) = y
        assert f.smooth(1) == y

    def test_smooth_equals_exist(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        x, y, z = BDD.var(1), BDD.var(2), BDD.var(3)
        f = (x & y) | z
        assert f.smooth(1) == f.exist(1)


class TestBDDSpread:
    def test_spread_zero(self):
        kyotodd.newvar()
        x = BDD.var(1)
        # spread(0) should be identity
        assert x.spread(0) == x

    def test_spread_positive(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        x = BDD.var(1)
        # spread(1) should expand the variable
        result = x.spread(1)
        assert result != BDD.false_


# ===== ZDD new methods =====

class TestZDDBool:
    def test_bool_raises(self):
        with pytest.raises(TypeError):
            if ZDD.empty:
                pass


class TestZDDShift:
    def test_lshift(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        shifted = a << 1
        assert shifted.top_var == 2

    def test_rshift(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(2)
        shifted = a >> 1
        assert shifted.top_var == 1

    def test_ilshift(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        expected = a << 1
        a <<= 1
        assert a == expected

    def test_irshift(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(2)
        expected = a >> 1
        a >>= 1
        assert a == expected


class TestZDDNewProperties:
    def test_size_empty(self):
        assert ZDD.empty.size == 0

    def test_size_single(self):
        assert ZDD.single.size == 0

    def test_size_family(self):
        kyotodd.newvar()
        a = _make_singleton(1)
        assert a.size == 1

    def test_lit_empty(self):
        assert ZDD.empty.lit == 0

    def test_lit_single(self):
        # {{}} has 0 literals
        assert ZDD.single.lit == 0

    def test_lit_family(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        # {{1}, {2}}: total literals = 1 + 1 = 2
        u = a + b
        assert u.lit == 2

    def test_exact_count_empty(self):
        assert ZDD.empty.exact_count == 0

    def test_exact_count_single(self):
        assert ZDD.single.exact_count == 1

    def test_exact_count_family(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b
        assert u.exact_count == 2

    def test_exact_count_type(self):
        # exact_count should return a Python int
        assert isinstance(ZDD.single.exact_count, int)

    def test_is_poly_empty(self):
        assert ZDD.empty.is_poly == 0

    def test_is_poly_single(self):
        assert ZDD.single.is_poly == 0

    def test_is_poly_family(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b
        assert u.is_poly == 1

    def test_support(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b  # {{1,2}}
        s = ab.support
        assert s != ZDD.empty


class TestZDDSwap:
    def test_swap(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        assert a.swap(1, 2) == b


class TestZDDAlways:
    def test_always_single_set(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b  # {{1,2}}
        # always({{1,2}}): both 1 and 2 are in every set → {{1},{2}}
        result = ab.always()
        assert result.card == 2
        assert result != ZDD.empty

    def test_always_two_sets(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b   # {{1,2}}
        u = ab + a   # {{1,2}, {1}}
        # always: element common to all sets = {1}
        assert u.always() == a


class TestZDDPermitSym:
    def test_permit_sym(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b  # {{1,2}}
        u = ab + a  # {{1,2}, {1}}
        # permit_sym(1): keep sets with at most 1 element
        result = u.permit_sym(1)
        assert result == a


class TestZDDImplyChk:
    def test_imply_chk(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b  # {{1,2}}: var 1 always appears with var 2
        assert ab.imply_chk(1, 2) == 1

    def test_imply_chk_false(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}: var 1 does not imply var 2
        assert u.imply_chk(1, 2) == 0


class TestZDDCoImplyChk:
    def test_coimply_chk(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b  # {{1,2}}
        # In {{1,2}}, both always appear together → co-implication
        assert ab.coimply_chk(1, 2) == 1


class TestZDDSymChk:
    def test_sym_chk_symmetric(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}: symmetric in 1 and 2
        assert u.sym_chk(1, 2) == 1

    def test_sym_chk_not_symmetric(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b  # {{1,2}}
        u = ab + a  # {{1,2}, {1}}: not symmetric
        assert u.sym_chk(1, 2) == 0


class TestZDDImplySet:
    def test_imply_set(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b  # {{1,2}}
        # var 1 implies var 2 in {{1,2}} → result should contain {{2}}
        result = ab.imply_set(1)
        assert result != ZDD.empty
        # {{2}} should be a subset of the imply set
        assert (b & result) == b

    def test_imply_set_no_implication(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}: var 1 does not always appear with var 2
        result = u.imply_set(1)
        # var 2 is NOT implied by var 1 (var 1 appears without var 2)
        assert (b & result) == ZDD.empty


class TestZDDSymGrp:
    def test_sym_grp(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}: 1 and 2 are symmetric
        result = u.sym_grp()
        assert result != ZDD.empty
        # Result should contain {{1,2}} as a symmetry group
        ab = a * b
        assert (ab & result) == ab

    def test_sym_grp_not_symmetric(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b
        u = ab + a  # {{1,2}, {1}}: not symmetric
        result = u.sym_grp()
        # {1,2} is not a symmetry group
        assert (ab & result) == ZDD.empty


class TestZDDSymGrpNaive:
    def test_sym_grp_naive(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}: 1 and 2 are symmetric
        result = u.sym_grp_naive()
        assert result != ZDD.empty
        # Should agree with sym_grp
        assert result == u.sym_grp()


class TestZDDSymSet:
    def test_sym_set(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}: symmetric
        result = u.sym_set(1)
        assert result != ZDD.empty
        # var 2 is symmetric with var 1, so {{2}} should be in the result
        assert (b & result) == b

    def test_sym_set_not_symmetric(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b
        u = ab + a  # {{1,2}, {1}}: not symmetric
        result = u.sym_set(1)
        # var 2 is not symmetric with var 1
        assert (b & result) == ZDD.empty


class TestZDDCoImplySet:
    def test_coimply_set(self):
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        ab = a * b  # {{1,2}}
        result = ab.coimply_set(1)
        assert result != ZDD.empty
        # var 2 co-implies var 1 (they always appear together)
        assert (b & result) == b

    def test_coimply_set_consistent_with_chk(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        c = _make_singleton(3)
        f = a * b + a * c  # {{1,2},{1,3}}
        result = f.coimply_set(1)
        # coimply_chk(1,2)=0 and coimply_chk(1,3)=0 since 2 and 3 don't always co-occur with 1
        assert f.coimply_chk(1, 2) == 0
        assert f.coimply_chk(1, 3) == 0
        # coimply_set should not include vars that fail coimply_chk
        assert (b & result) == ZDD.empty
        assert (c & result) == ZDD.empty


class TestZDDDivisor:
    def test_divisor(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        a = _make_singleton(1)
        b = _make_singleton(2)
        c = _make_singleton(3)
        # (a+b)*c = {{1,3},{2,3}} has a non-trivial divisor
        f = (a + b) * c
        d = f.divisor()
        assert d != ZDD.empty
        # The divisor should actually divide f
        assert f % d == ZDD.empty


# ===== Module-level functions =====

class TestZDDRandom:
    def test_zdd_random(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        r = kyotodd.zdd_random(3)
        # result is a valid ZDD
        assert r != ZDD.null

    def test_zdd_random_density_0(self):
        kyotodd.newvar()
        kyotodd.newvar()
        r = kyotodd.zdd_random(2, 0)
        assert r == ZDD.empty

    def test_zdd_random_density_100(self):
        kyotodd.newvar()
        kyotodd.newvar()
        r = kyotodd.zdd_random(2, 100)
        assert r != ZDD.empty
