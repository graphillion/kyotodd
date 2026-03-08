import pytest
import kyotodd


def test_init_explicit():
    kyotodd.init()


def test_init_with_params():
    kyotodd.init(node_count=2048, node_max=100000)


def test_auto_init_via_newvar():
    """newvar() should work without explicit init (auto-init)."""
    kyotodd.init()  # reset first
    v = kyotodd.newvar()
    assert v == 1


def test_newvar_sequential():
    v1 = kyotodd.newvar()
    v2 = kyotodd.newvar()
    v3 = kyotodd.newvar()
    assert v1 == 1
    assert v2 == 2
    assert v3 == 3


def test_var_count():
    assert kyotodd.var_count() == 0
    kyotodd.newvar()
    assert kyotodd.var_count() == 1
    kyotodd.newvar()
    assert kyotodd.var_count() == 2


def test_newvar_of_level():
    v = kyotodd.newvar_of_level(1)
    assert v >= 1


def test_level_of_var():
    v = kyotodd.newvar()
    lev = kyotodd.level_of_var(v)
    assert lev == v  # default mapping: var == level


def test_var_of_level():
    v = kyotodd.newvar()
    lev = kyotodd.level_of_var(v)
    assert kyotodd.var_of_level(lev) == v


def test_node_count():
    count = kyotodd.node_count()
    assert count >= 0


def test_gc():
    kyotodd.newvar()
    kyotodd.gc()  # should not raise


def test_gc_threshold():
    kyotodd.gc_set_threshold(0.8)
    assert kyotodd.gc_get_threshold() == pytest.approx(0.8)
    kyotodd.gc_set_threshold(0.9)
    assert kyotodd.gc_get_threshold() == pytest.approx(0.9)


def test_live_nodes():
    count = kyotodd.live_nodes()
    assert count >= 0


def test_invalid_var_raises_valueerror():
    from kyotodd import BDD
    kyotodd.newvar()
    x = BDD.var(1)
    with pytest.raises(ValueError):
        x.at0(0)  # var 0 is invalid
    with pytest.raises(ValueError):
        x.at1(999999)  # var out of range
