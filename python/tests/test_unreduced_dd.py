import pytest
import kyotodd


class TestUnreducedDDBasic:
    def test_construct_zero(self):
        u = kyotodd.UnreducedDD(0)
        assert u.is_zero
        assert u.is_terminal
        assert not u.is_one

    def test_construct_one(self):
        u = kyotodd.UnreducedDD(1)
        assert u.is_one
        assert u.is_terminal
        assert not u.is_zero

    def test_construct_null(self):
        u = kyotodd.UnreducedDD(-1)
        assert u.node_id == kyotodd.QDD.null.node_id  # bddnull

    def test_default_construct(self):
        u = kyotodd.UnreducedDD()
        assert u.is_zero


class TestUnreducedDDOperators:
    def test_equality(self):
        u0a = kyotodd.UnreducedDD(0)
        u0b = kyotodd.UnreducedDD(0)
        assert u0a == u0b

    def test_inequality(self):
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        assert u0 != u1

    def test_complement_toggle(self):
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        assert ~u0 == u1
        assert ~~u0 == u0

    def test_less_than(self):
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        # Just check it doesn't crash and returns bool
        result = u0 < u1 or u1 < u0 or u0 == u1
        assert result is True

    def test_hash(self):
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        s = {u0, u1}
        assert len(s) == 2

    def test_repr(self):
        u = kyotodd.UnreducedDD(0)
        assert "UnreducedDD(node_id=" in repr(u)

    def test_bool_raises(self):
        u = kyotodd.UnreducedDD(0)
        with pytest.raises(TypeError):
            bool(u)


class TestUnreducedDDGetnode:
    def test_simple_node(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        assert not node.is_terminal
        assert node.top_var == v1
        assert not node.is_reduced

    def test_no_reduction_applied(self):
        """UnreducedDD should NOT apply any reduction rules."""
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        # lo == hi should NOT be reduced away
        node = kyotodd.UnreducedDD.getnode(v1, u0, u0)
        assert not node.is_terminal
        assert node.top_var == v1

    def test_no_unique_table_sharing(self):
        """Each getnode call creates a distinct node."""
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        n1 = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        n2 = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        # Different allocations, different node IDs
        assert n1 != n2


class TestUnreducedDDChildAccessors:
    def test_raw_child0_child1(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        assert node.raw_child0() == u0
        assert node.raw_child1() == u1

    def test_raw_child_by_index(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        assert node.raw_child(0) == u0
        assert node.raw_child(1) == u1


class TestUnreducedDDSetChild:
    def test_set_child0(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        # Create node with null children
        node = kyotodd.UnreducedDD.getnode(v1, kyotodd.UnreducedDD(-1),
                                            kyotodd.UnreducedDD(-1))
        node.set_child0(u0)
        node.set_child1(u1)
        assert node.raw_child0() == u0
        assert node.raw_child1() == u1


class TestUnreducedDDReduce:
    def test_reduce_as_bdd(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        bdd = node.reduce_as_bdd()
        assert isinstance(bdd, kyotodd.BDD)
        assert bdd.top_var == v1

    def test_reduce_as_zdd(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        zdd = node.reduce_as_zdd()
        assert isinstance(zdd, kyotodd.ZDD)

    def test_reduce_as_qdd(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        qdd = node.reduce_as_qdd()
        assert isinstance(qdd, kyotodd.QDD)

    def test_reduce_bdd_jump_rule(self):
        """BDD reduction should apply jump rule: lo == hi => return lo."""
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        # lo == hi == 0-terminal
        node = kyotodd.UnreducedDD.getnode(v1, u0, u0)
        bdd = node.reduce_as_bdd()
        assert bdd == kyotodd.BDD(0)  # jump rule reduces to terminal

    def test_reduce_zdd_zero_suppression(self):
        """ZDD reduction should apply zero-suppression: hi == empty => return lo."""
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        # hi == 0-terminal (bddempty for ZDD)
        node = kyotodd.UnreducedDD.getnode(v1, u1, u0)
        zdd = node.reduce_as_zdd()
        # zero-suppression: hi is empty, so result is lo (= 1-terminal)
        assert zdd == kyotodd.ZDD(1)  # ZDD single = {{}} = 1-terminal

    def test_reduce_terminal(self):
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        assert u0.reduce_as_bdd() == kyotodd.BDD(0)
        assert u1.reduce_as_bdd() == kyotodd.BDD(1)


class TestUnreducedDDFromDD:
    def test_from_bdd(self):
        v1 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1)
        u = kyotodd.UnreducedDD(bdd)
        assert not u.is_terminal
        # Reduce back should give same BDD
        assert u.reduce_as_bdd() == bdd

    def test_from_zdd(self):
        v1 = kyotodd.new_var()
        zdd = kyotodd.ZDD(1)  # single = {{}}
        u = kyotodd.UnreducedDD(zdd)
        assert u.reduce_as_zdd() == zdd

    def test_from_qdd(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        qdd = kyotodd.QDD.getnode(v1, q0, q1)
        u = kyotodd.UnreducedDD(qdd)
        assert not u.is_terminal


class TestUnreducedDDWrapRaw:
    def test_wrap_raw_bdd(self):
        v1 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1)
        u = kyotodd.UnreducedDD.wrap_raw_bdd(bdd)
        assert u.reduce_as_bdd() == bdd

    def test_wrap_raw_zdd(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        zdd = node.reduce_as_zdd()
        u = kyotodd.UnreducedDD.wrap_raw_zdd(zdd)
        assert u.reduce_as_zdd() == zdd

    def test_wrap_raw_qdd(self):
        v1 = kyotodd.new_var()
        q0 = kyotodd.QDD(0)
        q1 = kyotodd.QDD(1)
        qdd = kyotodd.QDD.getnode(v1, q0, q1)
        u = kyotodd.UnreducedDD.wrap_raw_qdd(qdd)
        assert u.reduce_as_qdd() == qdd


class TestUnreducedDDProperties:
    def test_node_id(self):
        u = kyotodd.UnreducedDD(0)
        assert isinstance(u.node_id, int)

    def test_raw_size_terminal(self):
        u = kyotodd.UnreducedDD(0)
        assert u.raw_size == 0

    def test_raw_size_node(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        assert node.raw_size >= 1

    def test_is_reduced_terminal(self):
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        assert u0.is_reduced
        assert u1.is_reduced

    def test_is_reduced_unreduced_node(self):
        v1 = kyotodd.new_var()
        u0 = kyotodd.UnreducedDD(0)
        u1 = kyotodd.UnreducedDD(1)
        node = kyotodd.UnreducedDD.getnode(v1, u0, u1)
        assert not node.is_reduced
