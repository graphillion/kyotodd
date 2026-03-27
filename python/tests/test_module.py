import pytest
import kyotodd


def test_init_explicit():
    kyotodd.init()


def test_init_with_params():
    kyotodd.init(node_count=2048, node_max=100000)


def test_auto_init_via_new_var():
    """new_var() should work without explicit init (auto-init)."""
    kyotodd.init()  # reset first
    v = kyotodd.new_var()
    assert v == 1


def test_new_var_sequential():
    v1 = kyotodd.new_var()
    v2 = kyotodd.new_var()
    v3 = kyotodd.new_var()
    assert v1 == 1
    assert v2 == 2
    assert v3 == 3


def test_var_count():
    assert kyotodd.var_count() == 0
    kyotodd.new_var()
    assert kyotodd.var_count() == 1
    kyotodd.new_var()
    assert kyotodd.var_count() == 2


def test_new_var_of_level():
    v = kyotodd.new_var_of_level(1)
    assert v >= 1


def test_to_level():
    v = kyotodd.new_var()
    lev = kyotodd.to_level(v)
    assert lev == v  # default mapping: var == level


def test_to_var():
    v = kyotodd.new_var()
    lev = kyotodd.to_level(v)
    assert kyotodd.to_var(lev) == v


def test_node_count():
    count = kyotodd.node_count()
    assert count >= 0


def test_gc():
    kyotodd.new_var()
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
    kyotodd.new_var()
    x = BDD.var(1)
    with pytest.raises(ValueError):
        x.at0(0)  # var 0 is invalid
    with pytest.raises(ValueError):
        x.at1(999999)  # var out of range


def test_finalize_with_live_objects_raises():
    from kyotodd import BDD
    kyotodd.new_var()
    x = BDD.var(1)
    with pytest.raises(RuntimeError):
        kyotodd.finalize()
    del x


def test_reinit_with_live_objects_raises():
    from kyotodd import BDD
    kyotodd.new_var()
    x = BDD.var(1)
    with pytest.raises(RuntimeError):
        kyotodd.init(256)
    del x


def test_node_max_exhaustion_raises():
    from kyotodd import BDD
    kyotodd.init(4, 4)
    v1 = kyotodd.new_var()
    v2 = kyotodd.new_var()
    v3 = kyotodd.new_var()
    v4 = kyotodd.new_var()
    with pytest.raises(OverflowError):
        a = BDD.var(v1) & BDD.var(v2) & BDD.var(v3) & BDD.var(v4)
        b = BDD.var(v1) | BDD.var(v2) | BDD.var(v3) | BDD.var(v4)
        c = a ^ b
