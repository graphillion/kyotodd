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
        assert BDD.false_.size == 0

    def test_size_true(self):
        assert BDD.true_.size == 0

    def test_size_var(self):
        kyotodd.newvar()
        x = BDD.var(1)
        assert x.size == 1

    def test_top_var(self):
        kyotodd.newvar()
        kyotodd.newvar()
        x = BDD.var(2)
        assert x.top_var == 2
