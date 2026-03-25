"""Tests for MTBDDFloat, MTBDDInt, MTZDDFloat, MTZDDInt Python bindings."""

import pytest
import kyotodd as kdd


# ================================================================
#  MTBDDFloat
# ================================================================

class TestMTBDDFloat:
    def test_default_construction(self):
        t = kdd.MTBDDFloat()
        assert t.is_terminal
        assert t.is_zero
        assert t.terminal_value() == 0.0

    def test_terminal(self):
        t = kdd.MTBDDFloat.terminal(3.14)
        assert t.is_terminal
        assert t.terminal_value() == pytest.approx(3.14)

    def test_terminal_value_error(self):
        v = kdd.new_var()
        hi = kdd.MTBDDFloat.terminal(1.0)
        lo = kdd.MTBDDFloat.terminal(0.0)
        node = kdd.MTBDDFloat.ite(v, hi, lo)
        with pytest.raises(RuntimeError):
            node.terminal_value()

    def test_ite_node(self):
        v = kdd.new_var()
        hi = kdd.MTBDDFloat.terminal(1.0)
        lo = kdd.MTBDDFloat.terminal(0.0)
        node = kdd.MTBDDFloat.ite(v, hi, lo)
        assert not node.is_terminal
        assert node.top_var == v

    def test_evaluate(self):
        v = kdd.new_var()
        hi = kdd.MTBDDFloat.terminal(10.0)
        lo = kdd.MTBDDFloat.terminal(20.0)
        node = kdd.MTBDDFloat.ite(v, hi, lo)
        assert node.evaluate([0, 1]) == pytest.approx(10.0)
        assert node.evaluate([0, 0]) == pytest.approx(20.0)

    def test_from_bdd(self):
        v = kdd.new_var()
        bdd = kdd.BDD.var(v)
        mt = kdd.MTBDDFloat.from_bdd(bdd, 0.0, 1.0)
        assert mt.evaluate([0, 1]) == pytest.approx(1.0)
        assert mt.evaluate([0, 0]) == pytest.approx(0.0)

    def test_from_bdd_custom_values(self):
        v = kdd.new_var()
        bdd = kdd.BDD.var(v)
        mt = kdd.MTBDDFloat.from_bdd(bdd, -5.0, 5.0)
        assert mt.evaluate([0, 1]) == pytest.approx(5.0)
        assert mt.evaluate([0, 0]) == pytest.approx(-5.0)

    def test_addition(self):
        v = kdd.new_var()
        a = kdd.MTBDDFloat.ite(v, kdd.MTBDDFloat.terminal(3.0),
                                  kdd.MTBDDFloat.terminal(1.0))
        b = kdd.MTBDDFloat.ite(v, kdd.MTBDDFloat.terminal(2.0),
                                  kdd.MTBDDFloat.terminal(4.0))
        c = a + b
        assert c.evaluate([0, 1]) == pytest.approx(5.0)
        assert c.evaluate([0, 0]) == pytest.approx(5.0)

    def test_subtraction(self):
        a = kdd.MTBDDFloat.terminal(10.0)
        b = kdd.MTBDDFloat.terminal(3.0)
        c = a - b
        assert c.terminal_value() == pytest.approx(7.0)

    def test_multiplication(self):
        a = kdd.MTBDDFloat.terminal(3.0)
        b = kdd.MTBDDFloat.terminal(4.0)
        c = a * b
        assert c.terminal_value() == pytest.approx(12.0)

    def test_inplace_ops(self):
        a = kdd.MTBDDFloat.terminal(5.0)
        b = kdd.MTBDDFloat.terminal(3.0)
        a += b
        assert a.terminal_value() == pytest.approx(8.0)
        a -= b
        assert a.terminal_value() == pytest.approx(5.0)
        a *= b
        assert a.terminal_value() == pytest.approx(15.0)

    def test_min_max(self):
        a = kdd.MTBDDFloat.terminal(3.0)
        b = kdd.MTBDDFloat.terminal(5.0)
        assert kdd.MTBDDFloat.min(a, b).terminal_value() == pytest.approx(3.0)
        assert kdd.MTBDDFloat.max(a, b).terminal_value() == pytest.approx(5.0)

    def test_ite_cond(self):
        v = kdd.new_var()
        cond = kdd.MTBDDFloat.ite(v, kdd.MTBDDFloat.terminal(1.0),
                                     kdd.MTBDDFloat.terminal(0.0))
        then_val = kdd.MTBDDFloat.terminal(100.0)
        else_val = kdd.MTBDDFloat.terminal(200.0)
        result = cond.ite_cond(then_val, else_val)
        assert result.evaluate([0, 1]) == pytest.approx(100.0)
        assert result.evaluate([0, 0]) == pytest.approx(200.0)

    def test_equality(self):
        a = kdd.MTBDDFloat.terminal(3.0)
        b = kdd.MTBDDFloat.terminal(3.0)
        c = kdd.MTBDDFloat.terminal(4.0)
        assert a == b
        assert a != c
        assert hash(a) == hash(b)

    def test_repr(self):
        t = kdd.MTBDDFloat.terminal(1.0)
        r = repr(t)
        assert "MTBDDFloat" in r

    def test_bool_raises(self):
        t = kdd.MTBDDFloat()
        with pytest.raises(TypeError):
            bool(t)

    def test_is_one(self):
        t1 = kdd.MTBDDFloat.terminal(1.0)
        t0 = kdd.MTBDDFloat.terminal(0.0)
        assert t1.is_one
        assert not t0.is_one

    def test_raw_size(self):
        t = kdd.MTBDDFloat.terminal(1.0)
        assert t.raw_size >= 0

    def test_zero_terminal(self):
        z = kdd.MTBDDFloat.zero_terminal()
        assert z.is_zero


# ================================================================
#  MTBDDInt
# ================================================================

class TestMTBDDInt:
    def test_default_construction(self):
        t = kdd.MTBDDInt()
        assert t.is_terminal
        assert t.is_zero
        assert t.terminal_value() == 0

    def test_terminal(self):
        t = kdd.MTBDDInt.terminal(42)
        assert t.terminal_value() == 42

    def test_ite_node(self):
        v = kdd.new_var()
        hi = kdd.MTBDDInt.terminal(10)
        lo = kdd.MTBDDInt.terminal(20)
        node = kdd.MTBDDInt.ite(v, hi, lo)
        assert not node.is_terminal

    def test_evaluate(self):
        v = kdd.new_var()
        hi = kdd.MTBDDInt.terminal(10)
        lo = kdd.MTBDDInt.terminal(20)
        node = kdd.MTBDDInt.ite(v, hi, lo)
        assert node.evaluate([0, 1]) == 10
        assert node.evaluate([0, 0]) == 20

    def test_from_bdd(self):
        v = kdd.new_var()
        bdd = kdd.BDD.var(v)
        mt = kdd.MTBDDInt.from_bdd(bdd, 0, 1)
        assert mt.evaluate([0, 1]) == 1
        assert mt.evaluate([0, 0]) == 0

    def test_arithmetic(self):
        a = kdd.MTBDDInt.terminal(10)
        b = kdd.MTBDDInt.terminal(3)
        assert (a + b).terminal_value() == 13
        assert (a - b).terminal_value() == 7
        assert (a * b).terminal_value() == 30

    def test_min_max(self):
        a = kdd.MTBDDInt.terminal(3)
        b = kdd.MTBDDInt.terminal(7)
        assert kdd.MTBDDInt.min(a, b).terminal_value() == 3
        assert kdd.MTBDDInt.max(a, b).terminal_value() == 7

    def test_ite_cond(self):
        v = kdd.new_var()
        cond = kdd.MTBDDInt.ite(v, kdd.MTBDDInt.terminal(1),
                                   kdd.MTBDDInt.terminal(0))
        then_val = kdd.MTBDDInt.terminal(100)
        else_val = kdd.MTBDDInt.terminal(200)
        result = cond.ite_cond(then_val, else_val)
        assert result.evaluate([0, 1]) == 100
        assert result.evaluate([0, 0]) == 200

    def test_equality(self):
        a = kdd.MTBDDInt.terminal(42)
        b = kdd.MTBDDInt.terminal(42)
        c = kdd.MTBDDInt.terminal(43)
        assert a == b
        assert a != c

    def test_repr(self):
        t = kdd.MTBDDInt.terminal(1)
        assert "MTBDDInt" in repr(t)

    def test_negative_values(self):
        a = kdd.MTBDDInt.terminal(-5)
        b = kdd.MTBDDInt.terminal(3)
        assert (a + b).terminal_value() == -2
        assert (a * b).terminal_value() == -15


# ================================================================
#  MTZDDFloat
# ================================================================

class TestMTZDDFloat:
    def test_default_construction(self):
        t = kdd.MTZDDFloat()
        assert t.is_terminal
        assert t.is_zero

    def test_terminal(self):
        t = kdd.MTZDDFloat.terminal(2.71)
        assert t.terminal_value() == pytest.approx(2.71)

    def test_ite_node(self):
        v = kdd.new_var()
        hi = kdd.MTZDDFloat.terminal(1.0)
        lo = kdd.MTZDDFloat.terminal(0.0)
        node = kdd.MTZDDFloat.ite(v, hi, lo)
        assert not node.is_terminal

    def test_evaluate(self):
        v = kdd.new_var()
        hi = kdd.MTZDDFloat.terminal(10.0)
        lo = kdd.MTZDDFloat.terminal(20.0)
        node = kdd.MTZDDFloat.ite(v, hi, lo)
        # ZDD: var=1, assignment[1]=1 -> follow hi
        assert node.evaluate([0, 1]) == pytest.approx(10.0)
        assert node.evaluate([0, 0]) == pytest.approx(20.0)

    def test_from_zdd(self):
        v = kdd.new_var()
        zdd = kdd.ZDD.singleton(v)
        mt = kdd.MTZDDFloat.from_zdd(zdd, 0.0, 1.0)
        assert mt.evaluate([0, 1]) == pytest.approx(1.0)
        assert mt.evaluate([0, 0]) == pytest.approx(0.0)

    def test_arithmetic(self):
        a = kdd.MTZDDFloat.terminal(3.0)
        b = kdd.MTZDDFloat.terminal(4.0)
        assert (a + b).terminal_value() == pytest.approx(7.0)
        assert (a - b).terminal_value() == pytest.approx(-1.0)
        assert (a * b).terminal_value() == pytest.approx(12.0)

    def test_min_max(self):
        a = kdd.MTZDDFloat.terminal(3.0)
        b = kdd.MTZDDFloat.terminal(5.0)
        assert kdd.MTZDDFloat.min(a, b).terminal_value() == pytest.approx(3.0)
        assert kdd.MTZDDFloat.max(a, b).terminal_value() == pytest.approx(5.0)

    def test_ite_cond_terminal(self):
        # ITE with terminal condition
        cond = kdd.MTZDDFloat.terminal(1.0)  # non-zero → then
        then_val = kdd.MTZDDFloat.terminal(100.0)
        else_val = kdd.MTZDDFloat.terminal(200.0)
        result = cond.ite_cond(then_val, else_val)
        assert result.terminal_value() == pytest.approx(100.0)

        cond_z = kdd.MTZDDFloat.terminal(0.0)  # zero → else
        result_z = cond_z.ite_cond(then_val, else_val)
        assert result_z.terminal_value() == pytest.approx(200.0)

    def test_equality(self):
        a = kdd.MTZDDFloat.terminal(3.14)
        b = kdd.MTZDDFloat.terminal(3.14)
        c = kdd.MTZDDFloat.terminal(2.71)
        assert a == b
        assert a != c

    def test_repr(self):
        t = kdd.MTZDDFloat.terminal(1.0)
        assert "MTZDDFloat" in repr(t)

    def test_bool_raises(self):
        t = kdd.MTZDDFloat()
        with pytest.raises(TypeError):
            bool(t)

    def test_zero_terminal(self):
        z = kdd.MTZDDFloat.zero_terminal()
        assert z.is_zero


# ================================================================
#  MTZDDInt
# ================================================================

class TestMTZDDInt:
    def test_default_construction(self):
        t = kdd.MTZDDInt()
        assert t.is_terminal
        assert t.is_zero

    def test_terminal(self):
        t = kdd.MTZDDInt.terminal(100)
        assert t.terminal_value() == 100

    def test_ite_node(self):
        v = kdd.new_var()
        hi = kdd.MTZDDInt.terminal(10)
        lo = kdd.MTZDDInt.terminal(20)
        node = kdd.MTZDDInt.ite(v, hi, lo)
        assert not node.is_terminal

    def test_evaluate(self):
        v = kdd.new_var()
        hi = kdd.MTZDDInt.terminal(10)
        lo = kdd.MTZDDInt.terminal(20)
        node = kdd.MTZDDInt.ite(v, hi, lo)
        assert node.evaluate([0, 1]) == 10
        assert node.evaluate([0, 0]) == 20

    def test_from_zdd(self):
        v = kdd.new_var()
        zdd = kdd.ZDD.singleton(v)
        mt = kdd.MTZDDInt.from_zdd(zdd, 0, 1)
        assert mt.evaluate([0, 1]) == 1
        assert mt.evaluate([0, 0]) == 0

    def test_arithmetic(self):
        a = kdd.MTZDDInt.terminal(10)
        b = kdd.MTZDDInt.terminal(3)
        assert (a + b).terminal_value() == 13
        assert (a - b).terminal_value() == 7
        assert (a * b).terminal_value() == 30

    def test_min_max(self):
        a = kdd.MTZDDInt.terminal(3)
        b = kdd.MTZDDInt.terminal(7)
        assert kdd.MTZDDInt.min(a, b).terminal_value() == 3
        assert kdd.MTZDDInt.max(a, b).terminal_value() == 7

    def test_ite_cond_terminal(self):
        # ITE with terminal condition
        cond = kdd.MTZDDInt.terminal(1)  # non-zero → then
        then_val = kdd.MTZDDInt.terminal(100)
        else_val = kdd.MTZDDInt.terminal(200)
        result = cond.ite_cond(then_val, else_val)
        assert result.terminal_value() == 100

        cond_z = kdd.MTZDDInt.terminal(0)  # zero → else
        result_z = cond_z.ite_cond(then_val, else_val)
        assert result_z.terminal_value() == 200

    def test_equality(self):
        a = kdd.MTZDDInt.terminal(42)
        b = kdd.MTZDDInt.terminal(42)
        assert a == b

    def test_repr(self):
        t = kdd.MTZDDInt.terminal(1)
        assert "MTZDDInt" in repr(t)

    def test_negative_values(self):
        a = kdd.MTZDDInt.terminal(-5)
        b = kdd.MTZDDInt.terminal(3)
        assert (a + b).terminal_value() == -2

    def test_inplace_ops(self):
        a = kdd.MTZDDInt.terminal(5)
        b = kdd.MTZDDInt.terminal(3)
        a += b
        assert a.terminal_value() == 8
        a -= b
        assert a.terminal_value() == 5
        a *= b
        assert a.terminal_value() == 15
