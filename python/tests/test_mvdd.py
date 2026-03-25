"""Tests for MVBDD, MVZDD, MVDDVarTable, and MVDDVarInfo Python bindings."""

import pytest
import kyotodd as kdd


# ================================================================
#  MVDDVarTable
# ================================================================

class TestMVDDVarTable:
    def test_construction(self):
        table = kdd.MVDDVarTable(3)
        assert table.k == 3
        assert table.mvdd_var_count == 0

    def test_invalid_k(self):
        with pytest.raises(ValueError):
            kdd.MVDDVarTable(1)

    def test_register_var(self):
        table = kdd.MVDDVarTable(3)
        v1 = kdd.new_var()
        v2 = kdd.new_var()
        mv = table.register_var([v1, v2])
        assert mv == 1
        assert table.mvdd_var_count == 1

    def test_dd_vars_of(self):
        table = kdd.MVDDVarTable(3)
        v1 = kdd.new_var()
        v2 = kdd.new_var()
        mv = table.register_var([v1, v2])
        assert table.dd_vars_of(mv) == [v1, v2]

    def test_mvdd_var_of(self):
        table = kdd.MVDDVarTable(3)
        v1 = kdd.new_var()
        v2 = kdd.new_var()
        mv = table.register_var([v1, v2])
        assert table.mvdd_var_of(v1) == mv
        assert table.mvdd_var_of(v2) == mv
        assert table.mvdd_var_of(999) == 0

    def test_dd_var_index(self):
        table = kdd.MVDDVarTable(3)
        v1 = kdd.new_var()
        v2 = kdd.new_var()
        table.register_var([v1, v2])
        assert table.dd_var_index(v1) == 0
        assert table.dd_var_index(v2) == 1
        assert table.dd_var_index(999) == -1

    def test_var_info(self):
        table = kdd.MVDDVarTable(3)
        v1 = kdd.new_var()
        v2 = kdd.new_var()
        mv = table.register_var([v1, v2])
        info = table.var_info(mv)
        assert info.mvdd_var == mv
        assert info.k == 3
        assert info.dd_vars == [v1, v2]

    def test_get_top_dd_var(self):
        table = kdd.MVDDVarTable(3)
        v1 = kdd.new_var()
        v2 = kdd.new_var()
        mv = table.register_var([v1, v2])
        assert table.get_top_dd_var(mv) == v1

    def test_repr(self):
        table = kdd.MVDDVarTable(3)
        r = repr(table)
        assert "MVDDVarTable" in r
        assert "k=3" in r

    def test_var_info_repr(self):
        table = kdd.MVDDVarTable(3)
        v1 = kdd.new_var()
        v2 = kdd.new_var()
        mv = table.register_var([v1, v2])
        info = table.var_info(mv)
        r = repr(info)
        assert "MVDDVarInfo" in r


# ================================================================
#  MVBDD
# ================================================================

class TestMVBDD:
    def test_construction_k(self):
        m = kdd.MVBDD(3)
        assert m.k == 3
        assert m.is_zero

    def test_construction_true(self):
        m = kdd.MVBDD(3, True)
        assert m.is_one

    def test_zero_one_factories(self):
        table = kdd.MVDDVarTable(3)
        z = kdd.MVBDD.zero(table)
        o = kdd.MVBDD.one(table)
        assert z.is_zero
        assert o.is_one
        assert z != o

    def test_new_var(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        assert v1 == 1
        v2 = m.new_var()
        assert v2 == 2

    def test_singleton(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 0)
        assert not s.is_terminal
        s1 = kdd.MVBDD.singleton(m, v1, 1)
        s2 = kdd.MVBDD.singleton(m, v1, 2)
        assert s != s1
        assert s1 != s2

    def test_child(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 1)
        # child(1) should be true (the matching value)
        c1 = s.child(1)
        assert c1.is_one
        # child(0) and child(2) should be false
        c0 = s.child(0)
        assert c0.is_zero
        c2 = s.child(2)
        assert c2.is_zero

    def test_evaluate(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 1)
        assert s.evaluate([1]) is True
        assert s.evaluate([0]) is False
        assert s.evaluate([2]) is False

    def test_boolean_ops(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s0 = kdd.MVBDD.singleton(m, v1, 0)
        s1 = kdd.MVBDD.singleton(m, v1, 1)
        # OR: should be true for value 0 or 1
        result = s0 | s1
        assert result.evaluate([0]) is True
        assert result.evaluate([1]) is True
        assert result.evaluate([2]) is False
        # AND: disjoint literals, always false
        result = s0 & s1
        assert result.evaluate([0]) is False
        assert result.evaluate([1]) is False

    def test_not(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 1)
        ns = ~s
        assert ns.evaluate([1]) is False
        assert ns.evaluate([0]) is True

    def test_xor(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s0 = kdd.MVBDD.singleton(m, v1, 0)
        s1 = kdd.MVBDD.singleton(m, v1, 1)
        result = s0 ^ s1
        assert result.evaluate([0]) is True
        assert result.evaluate([1]) is True
        assert result.evaluate([2]) is False

    def test_inplace_ops(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s0 = kdd.MVBDD.singleton(m, v1, 0)
        s1 = kdd.MVBDD.singleton(m, v1, 1)
        r = kdd.MVBDD.zero(m.var_table)
        r |= s0
        r |= s1
        assert r.evaluate([0]) is True
        assert r.evaluate([1]) is True

    def test_to_bdd(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 1)
        bdd = s.to_bdd()
        assert isinstance(bdd, kdd.BDD)

    def test_from_bdd(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 1)
        bdd = s.to_bdd()
        recovered = kdd.MVBDD.from_bdd(m, bdd)
        assert recovered == s

    def test_ite(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        t = kdd.MVBDD.one(m.var_table)
        f = kdd.MVBDD.zero(m.var_table)
        # ite: v1==0 -> true, v1==1 -> false, v1==2 -> true
        result = kdd.MVBDD.ite(m, v1, [t, f, t])
        assert result.evaluate([0]) is True
        assert result.evaluate([1]) is False
        assert result.evaluate([2]) is True

    def test_top_var(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 1)
        assert s.top_var == v1

    def test_terminal_top_var(self):
        m = kdd.MVBDD(3)
        assert m.top_var == 0

    def test_node_count(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 1)
        assert s.mvbdd_node_count >= 1
        assert s.size >= 1

    def test_equality(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s1 = kdd.MVBDD.singleton(m, v1, 1)
        s2 = kdd.MVBDD.singleton(m, v1, 1)
        assert s1 == s2
        assert hash(s1) == hash(s2)

    def test_repr(self):
        m = kdd.MVBDD(3)
        r = repr(m)
        assert "MVBDD" in r
        assert "k=3" in r

    def test_bool_raises(self):
        m = kdd.MVBDD(3)
        with pytest.raises(TypeError):
            bool(m)

    def test_var_table(self):
        m = kdd.MVBDD(3)
        t = m.var_table
        assert isinstance(t, kdd.MVDDVarTable)
        assert t.k == 3

    def test_construction_with_table_bdd(self):
        m = kdd.MVBDD(3)
        v1 = m.new_var()
        s = kdd.MVBDD.singleton(m, v1, 1)
        bdd = s.to_bdd()
        m2 = kdd.MVBDD(m.var_table, bdd)
        assert m2 == s


# ================================================================
#  MVZDD
# ================================================================

class TestMVZDD:
    def test_construction_empty(self):
        z = kdd.MVZDD(3)
        assert z.k == 3
        assert z.is_zero

    def test_construction_one(self):
        z = kdd.MVZDD(3, True)
        assert z.is_one

    def test_zero_one_factories(self):
        table = kdd.MVDDVarTable(3)
        z = kdd.MVZDD.zero(table)
        o = kdd.MVZDD.one(table)
        assert z.is_zero
        assert o.is_one

    def test_new_var(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        assert v1 == 1

    def test_singleton(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        assert s.count == 1.0

    def test_child(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        c1 = s.child(1)
        assert c1.is_one
        c0 = s.child(0)
        assert c0.is_zero

    def test_evaluate(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        assert s.evaluate([1]) is True
        assert s.evaluate([0]) is False
        assert s.evaluate([2]) is False

    def test_union(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s0 = kdd.MVZDD.singleton(z, v1, 0)
        s1 = kdd.MVZDD.singleton(z, v1, 1)
        result = s0 + s1
        assert result.count == 2.0
        assert result.evaluate([0]) is True
        assert result.evaluate([1]) is True
        assert result.evaluate([2]) is False

    def test_intersection(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s0 = kdd.MVZDD.singleton(z, v1, 0)
        s1 = kdd.MVZDD.singleton(z, v1, 1)
        result = s0 & s1
        assert result.count == 0.0

    def test_difference(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s0 = kdd.MVZDD.singleton(z, v1, 0)
        s1 = kdd.MVZDD.singleton(z, v1, 1)
        union = s0 + s1
        result = union - s0
        assert result == s1

    def test_inplace_ops(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s0 = kdd.MVZDD.singleton(z, v1, 0)
        s1 = kdd.MVZDD.singleton(z, v1, 1)
        r = kdd.MVZDD.zero(z.var_table)
        r += s0
        r += s1
        assert r.count == 2.0

    def test_enumerate(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s0 = kdd.MVZDD.singleton(z, v1, 0)
        s1 = kdd.MVZDD.singleton(z, v1, 1)
        union = s0 + s1
        sets = union.enumerate()
        assert len(sets) == 2
        assert sorted(sets) == [[0], [1]]

    def test_exact_count(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s0 = kdd.MVZDD.singleton(z, v1, 0)
        s1 = kdd.MVZDD.singleton(z, v1, 1)
        s2 = kdd.MVZDD.singleton(z, v1, 2)
        union = s0 + s1 + s2
        assert union.exact_count == 3

    def test_to_str(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        s_str = z.to_str()
        assert isinstance(s_str, str)

    def test_str(self):
        z = kdd.MVZDD(3)
        s = str(z)
        assert isinstance(s, str)

    def test_print_sets(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        result = s.print_sets()
        assert isinstance(result, str)

    def test_to_zdd(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        zdd = s.to_zdd()
        assert isinstance(zdd, kdd.ZDD)

    def test_from_zdd(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        zdd = s.to_zdd()
        recovered = kdd.MVZDD.from_zdd(z, zdd)
        assert recovered == s

    def test_ite(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        t = kdd.MVZDD.one(z.var_table)
        f = kdd.MVZDD.zero(z.var_table)
        # v1==0 -> one, v1==1 -> zero, v1==2 -> one
        result = kdd.MVZDD.ite(z, v1, [t, f, t])
        assert result.evaluate([0]) is True
        assert result.evaluate([1]) is False
        assert result.evaluate([2]) is True

    def test_top_var(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        assert s.top_var == v1

    def test_node_count(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        assert s.mvzdd_node_count >= 1
        assert s.size >= 1

    def test_equality(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s1 = kdd.MVZDD.singleton(z, v1, 1)
        s2 = kdd.MVZDD.singleton(z, v1, 1)
        assert s1 == s2
        assert hash(s1) == hash(s2)

    def test_repr(self):
        z = kdd.MVZDD(3)
        r = repr(z)
        assert "MVZDD" in r
        assert "k=3" in r

    def test_bool_raises(self):
        z = kdd.MVZDD(3)
        with pytest.raises(TypeError):
            bool(z)

    def test_var_table(self):
        z = kdd.MVZDD(3)
        t = z.var_table
        assert isinstance(t, kdd.MVDDVarTable)

    def test_construction_with_table_zdd(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        s = kdd.MVZDD.singleton(z, v1, 1)
        zdd = s.to_zdd()
        z2 = kdd.MVZDD(z.var_table, zdd)
        assert z2 == s

    def test_two_variables(self):
        z = kdd.MVZDD(3)
        v1 = z.new_var()
        v2 = z.new_var()
        s1 = kdd.MVZDD.singleton(z, v1, 1)
        s2 = kdd.MVZDD.singleton(z, v2, 2)
        union = s1 + s2
        assert union.count == 2.0
        assert union.evaluate([1, 0]) is True
        assert union.evaluate([0, 2]) is True
        assert union.evaluate([0, 0]) is False
