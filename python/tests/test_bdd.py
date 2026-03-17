import pytest
import kyotodd
from kyotodd import BDD


class TestBDDConstruction:
    def test_false(self):
        b = BDD(0)
        assert b == BDD.false_

    def test_true(self):
        b = BDD(1)
        assert b == BDD.true_

    def test_null(self):
        b = BDD(-1)
        assert b == BDD.null

    def test_default_is_false(self):
        b = BDD()
        assert b == BDD.false_

    def test_var(self):
        kyotodd.newvar()
        x = BDD.var(1)
        assert x != BDD.false_
        assert x != BDD.true_


class TestBDDConstants:
    def test_false_is_not_true(self):
        assert BDD.false_ != BDD.true_

    def test_false_is_not_null(self):
        assert BDD.false_ != BDD.null

    def test_true_is_not_null(self):
        assert BDD.true_ != BDD.null


class TestBDDEquality:
    def test_eq_same(self):
        kyotodd.newvar()
        x = BDD.var(1)
        y = BDD.var(1)
        assert x == y

    def test_ne_different(self):
        kyotodd.newvar()
        kyotodd.newvar()
        x = BDD.var(1)
        y = BDD.var(2)
        assert x != y


class TestBDDHash:
    def test_hashable(self):
        kyotodd.newvar()
        x = BDD.var(1)
        h = hash(x)
        assert isinstance(h, int)

    def test_equal_objects_same_hash(self):
        kyotodd.newvar()
        x = BDD.var(1)
        y = BDD.var(1)
        assert hash(x) == hash(y)

    def test_usable_in_set(self):
        kyotodd.newvar()
        kyotodd.newvar()
        x = BDD.var(1)
        y = BDD.var(2)
        s = {x, y, BDD.var(1)}
        assert len(s) == 2

    def test_usable_as_dict_key(self):
        kyotodd.newvar()
        x = BDD.var(1)
        d = {x: "hello"}
        assert d[BDD.var(1)] == "hello"


class TestBDDRepr:
    def test_repr_false(self):
        r = repr(BDD.false_)
        assert "BDD(node_id=" in r

    def test_repr_var(self):
        kyotodd.newvar()
        x = BDD.var(1)
        r = repr(x)
        assert r.startswith("BDD(node_id=")
        assert r.endswith(")")


class TestBDDBool:
    def test_bool_raises(self):
        with pytest.raises(TypeError):
            if BDD.false_:
                pass


class TestBDDProperties:
    def test_node_id(self):
        nid = BDD.false_.node_id
        assert isinstance(nid, int)

    def test_size_false(self):
        assert BDD.false_.raw_size == 0

    def test_size_true(self):
        assert BDD.true_.raw_size == 0

    def test_size_var(self):
        kyotodd.newvar()
        x = BDD.var(1)
        assert x.raw_size == 1

    def test_top_var(self):
        kyotodd.newvar()
        kyotodd.newvar()
        x = BDD.var(2)
        assert x.top_var == 2


class TestBDDOperators:
    def _make_vars(self):
        kyotodd.newvar()
        kyotodd.newvar()
        return BDD.var(1), BDD.var(2)

    def test_and(self):
        x, y = self._make_vars()
        z = x & y
        assert z != BDD.false_
        assert z != x
        assert z != y

    def test_or(self):
        x, y = self._make_vars()
        z = x | y
        assert z != BDD.false_
        assert z != x
        assert z != y

    def test_xor(self):
        x, y = self._make_vars()
        z = x ^ y
        assert z != BDD.false_

    def test_not(self):
        x, _ = self._make_vars()
        nx = ~x
        assert nx != x
        # x & ~x == false
        assert (x & nx) == BDD.false_
        # x | ~x == true
        assert (x | nx) == BDD.true_

    def test_double_not(self):
        x, _ = self._make_vars()
        assert ~~x == x

    def test_lshift(self):
        x, _ = self._make_vars()
        shifted = x << 1
        assert shifted != x
        assert shifted.top_var == x.top_var + 1

    def test_rshift(self):
        _, y = self._make_vars()
        shifted = y >> 1
        assert shifted != y
        assert shifted.top_var == y.top_var - 1

    def test_and_identity(self):
        x, _ = self._make_vars()
        assert (x & BDD.true_) == x
        assert (x & BDD.false_) == BDD.false_

    def test_or_identity(self):
        x, _ = self._make_vars()
        assert (x | BDD.false_) == x
        assert (x | BDD.true_) == BDD.true_

    def test_xor_self(self):
        x, _ = self._make_vars()
        assert (x ^ x) == BDD.false_

    def test_de_morgan(self):
        x, y = self._make_vars()
        assert ~(x & y) == (~x | ~y)
        assert ~(x | y) == (~x & ~y)


class TestBDDCompoundAssignment:
    def _make_vars(self):
        kyotodd.newvar()
        kyotodd.newvar()
        return BDD.var(1), BDD.var(2)

    def test_iand(self):
        x, y = self._make_vars()
        expected = x & y
        x &= y
        assert x == expected

    def test_ior(self):
        x, y = self._make_vars()
        expected = x | y
        x |= y
        assert x == expected

    def test_ixor(self):
        x, y = self._make_vars()
        expected = x ^ y
        x ^= y
        assert x == expected

    def test_ilshift(self):
        x, _ = self._make_vars()
        expected = x << 1
        x <<= 1
        assert x == expected

    def test_irshift(self):
        _, y = self._make_vars()
        expected = y >> 1
        y >>= 1
        assert y == expected


class TestBDDMethods:
    def _make_vars(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        return BDD.var(1), BDD.var(2), BDD.var(3)

    def test_at0(self):
        x, y, _ = self._make_vars()
        f = x & y
        # f|_{x=0} = false & y = false
        assert f.at0(1) == BDD.false_

    def test_at1(self):
        x, y, _ = self._make_vars()
        f = x & y
        # f|_{x=1} = true & y = y
        assert f.at1(1) == y

    def test_at0_at1_reconstruct(self):
        """Shannon expansion: f = x & f|_{x=1} | ~x & f|_{x=0}"""
        x, y, z = self._make_vars()
        f = (x & y) | z
        assert f == (x & f.at1(1)) | (~x & f.at0(1))

    def test_cofactor(self):
        x, y, _ = self._make_vars()
        f = x & y
        # cofactor by x gives y
        assert f.cofactor(x) == y

    def test_support(self):
        x, y, z = self._make_vars()
        f = x & z  # support = {1, 3}
        s = f.support()
        assert s != BDD.false_
        assert s != BDD.true_

    def test_support_vec(self):
        x, y, z = self._make_vars()
        f = x & z  # support = {1, 3}
        sv = f.support_vec()
        assert sorted(sv) == [1, 3]

    def test_support_vec_single(self):
        x, _, _ = self._make_vars()
        assert x.support_vec() == [1]

    def test_support_vec_constant(self):
        assert BDD.false_.support_vec() == []
        assert BDD.true_.support_vec() == []

    def test_imply_true(self):
        x, y, _ = self._make_vars()
        f = x & y
        # (x & y) => x
        assert f.imply(x) == 1

    def test_imply_false(self):
        x, y, _ = self._make_vars()
        # x does not imply (x & y)
        assert x.imply(x & y) == 0

    def test_imply_self(self):
        x, _, _ = self._make_vars()
        assert x.imply(x) == 1

    def test_imply_false_implies_anything(self):
        x, _, _ = self._make_vars()
        assert BDD.false_.imply(x) == 1

    def test_size_complex(self):
        x, y, z = self._make_vars()
        f = (x & y) | z
        assert f.raw_size >= 2  # at least 2 nodes

    def test_top_var_and(self):
        x, y, _ = self._make_vars()
        f = x & y
        # higher level = closer to root; var 2 is at level 2
        assert f.top_var == 2


class TestBDDQuantification:
    def _make_vars(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        return BDD.var(1), BDD.var(2), BDD.var(3)

    def test_exist_single_var(self):
        x, y, _ = self._make_vars()
        f = x & y
        # exist x. (x & y) = y
        assert f.exist(1) == y

    def test_exist_var_list(self):
        x, y, z = self._make_vars()
        f = x & y & z
        # exist [x, y]. (x & y & z) = z
        assert f.exist([1, 2]) == z

    def test_exist_cube(self):
        x, y, z = self._make_vars()
        f = x & y & z
        # cube-based exist should match var-list-based exist
        result_list = f.exist([1, 2])
        result_cube = f.exist(x.support())  # single-var cube
        assert result_cube == f.exist(1)

    def test_exist_all_vars(self):
        x, y, _ = self._make_vars()
        f = x & y
        # exist [x, y]. (x & y) = true
        assert f.exist([1, 2]) == BDD.true_

    def test_exist_no_effect(self):
        x, y, _ = self._make_vars()
        f = x & y
        # exist z. (x & y) = x & y (z not in support)
        assert f.exist(3) == f

    def test_univ_single_var(self):
        x, y, _ = self._make_vars()
        f = x | y
        # forall x. (x | y) = y
        assert f.univ(1) == y

    def test_univ_var_list(self):
        x, y, _ = self._make_vars()
        f = x | y
        # forall [x, y]. (x | y) = false
        assert f.univ([1, 2]) == BDD.false_

    def test_univ_cube(self):
        x, y, _ = self._make_vars()
        f = x | y
        cube = x.support()
        # forall x. (x | y) = y
        assert f.univ(cube) == y

    def test_exist_univ_duality(self):
        """exist x. f  ==  ~(forall x. ~f)"""
        x, y, _ = self._make_vars()
        f = x & y
        assert f.exist(1) == ~((~f).univ(1))


class TestBDDIte:
    def _make_vars(self):
        kyotodd.newvar()
        kyotodd.newvar()
        return BDD.var(1), BDD.var(2)

    def test_ite_basic(self):
        x, y = self._make_vars()
        # ite(x, y, false) = x & y
        assert BDD.ite(x, y, BDD.false_) == (x & y)

    def test_ite_as_or(self):
        x, y = self._make_vars()
        # ite(x, true, y) = x | y
        assert BDD.ite(x, BDD.true_, y) == (x | y)

    def test_ite_as_not(self):
        x, _ = self._make_vars()
        # ite(x, false, true) = ~x
        assert BDD.ite(x, BDD.false_, BDD.true_) == ~x

    def test_ite_mux(self):
        x, y = self._make_vars()
        # ite(x, y, ~y) = x xnor y  (equiv: ~(x ^ y))
        result = BDD.ite(x, y, ~y)
        assert result == ~(x ^ y)
