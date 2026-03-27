import pytest
import kyotodd


class TestQDDBasic:
    def test_construct_false(self):
        q = kyotodd.QDD(0)
        assert q.is_zero
        assert q.is_terminal
        assert not q.is_one

    def test_construct_true(self):
        q = kyotodd.QDD(1)
        assert q.is_one
        assert q.is_terminal
        assert not q.is_zero

    def test_construct_null(self):
        q = kyotodd.QDD(-1)
        assert q == kyotodd.QDD.null

    def test_default_construct(self):
        q = kyotodd.QDD()
        assert q.is_zero

    def test_static_constants(self):
        assert kyotodd.QDD.false_.is_zero
        assert kyotodd.QDD.true_.is_one


class TestQDDOperators:
    def test_equality(self):
        q0a = kyotodd.QDD(0)
        q0b = kyotodd.QDD(0)
        assert q0a == q0b

    def test_inequality(self):
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        assert q0 != q1

    def test_complement(self):
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        assert ~q0 == q1
        assert ~q1 == q0
        assert ~~q0 == q0

    def test_hash(self):
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        s = {q0, q1}
        assert len(s) == 2

    def test_repr(self):
        q = kyotodd.QDD(0)
        assert "QDD: id=" in repr(q)

    def test_bool_raises(self):
        q = kyotodd.QDD(0)
        with pytest.raises(TypeError):
            bool(q)


class TestQDDGetnode:
    def test_simple_node(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        node = kyotodd.QDD.getnode(v1, q0, q1)
        assert not node.is_terminal
        assert node.top_var == v1

    def test_two_level(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        lo = kyotodd.QDD.getnode(v1, q0, q1)
        hi = kyotodd.QDD.getnode(v1, q1, q0)
        node = kyotodd.QDD.getnode(v2, lo, hi)
        assert node.top_var == v2

    def test_lo_eq_hi_preserved(self):
        """QDD does NOT apply jump rule: lo == hi nodes are preserved."""
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        node = kyotodd.QDD.getnode(v1, q0, q0)
        assert not node.is_terminal
        assert node.top_var == v1

    def test_invalid_level_raises(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        # Children at level 0 (terminal), but v2 expects children at level 1
        with pytest.raises(ValueError):
            kyotodd.QDD.getnode(v2, q0, q1)


class TestQDDChildAccessors:
    def test_child0_child1(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        node = kyotodd.QDD.getnode(v1, q0, q1)
        assert node.child0() == q0
        assert node.child1() == q1

    def test_child_by_index(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        node = kyotodd.QDD.getnode(v1, q0, q1)
        assert node.child(0) == q0
        assert node.child(1) == q1

    def test_raw_child(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        node = kyotodd.QDD.getnode(v1, q0, q1)
        assert node.raw_child0() == q0
        assert node.raw_child1() == q1
        assert node.raw_child(0) == q0
        assert node.raw_child(1) == q1


class TestQDDConversion:
    def test_to_bdd(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        node = kyotodd.QDD.getnode(v1, q0, q1)
        bdd = node.to_bdd()
        assert isinstance(bdd, kyotodd.BDD)
        assert bdd.top_var == v1

    def test_to_zdd(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        node = kyotodd.QDD.getnode(v1, q0, q1)
        zdd = node.to_zdd()
        assert isinstance(zdd, kyotodd.ZDD)

    def test_terminal_to_bdd(self):
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        assert q0.to_bdd() == kyotodd.BDD(0)
        assert q1.to_bdd() == kyotodd.BDD(1)

    def test_roundtrip_qdd_bdd_qdd(self):
        """QDD -> BDD -> QDD should preserve semantics."""
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        qdd = kyotodd.QDD.getnode(v1, q0, q1)
        bdd = qdd.to_bdd()
        # BDD -> UnreducedDD -> reduce_as_qdd for roundtrip
        u = kyotodd.UnreducedDD(bdd)
        qdd2 = u.reduce_as_qdd()
        assert qdd == qdd2


class TestQDDProperties:
    def test_node_id(self):
        q = kyotodd.QDD(0)
        assert isinstance(q.node_id, int)

    def test_raw_size_terminal(self):
        q = kyotodd.QDD(0)
        assert q.raw_size == 0

    def test_raw_size_node(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        node = kyotodd.QDD.getnode(v1, q0, q1)
        assert node.raw_size >= 1
