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


def test_cache_ratio():
    assert kyotodd.cache_get_ratio() == 1
    size = kyotodd.cache_size()
    assert size > 0 and size & (size - 1) == 0  # power of 2
    assert kyotodd.cache_set_ratio(2) == 1
    assert kyotodd.cache_get_ratio() == 2
    assert kyotodd.cache_size() == max(1, size // 2)
    assert kyotodd.cache_set_ratio(0) == 2  # freeze
    assert kyotodd.cache_get_ratio() == 0
    with pytest.raises(ValueError):
        kyotodd.cache_set_ratio(-1)
    assert kyotodd.cache_set_ratio(1) == 0
    assert kyotodd.cache_size() == size


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


class TestImportLimit:
    def teardown_method(self):
        kyotodd.set_import_limit(10_000_000, 1_000_000, 1_000_000)

    def test_defaults(self):
        lim = kyotodd.get_import_limit()
        assert lim == {"max_nodes": 10_000_000, "max_levels": 1_000_000,
                       "max_outputs": 1_000_000}

    def test_set_all_and_unlimited(self):
        kyotodd.set_import_limit(123, 45, 6)
        assert kyotodd.get_import_limit() == {"max_nodes": 123, "max_levels": 45,
                                              "max_outputs": 6}
        kyotodd.set_import_limit(0, 0, 0)
        assert kyotodd.get_import_limit() == {"max_nodes": 0, "max_levels": 0,
                                              "max_outputs": 0}

    def test_omitted_arguments_keep_current(self):
        kyotodd.set_import_limit(max_levels=7)
        lim = kyotodd.get_import_limit()
        assert lim["max_nodes"] == 10_000_000
        assert lim["max_levels"] == 7
        assert lim["max_outputs"] == 1_000_000

    def test_limit_applies_to_import(self):
        from kyotodd import ZDD
        kyotodd.new_vars(3)
        z = ZDD.single
        for v in (1, 2, 3):
            z = z.change(v)
        data = z.export_binary_str()
        kyotodd.set_import_limit(max_nodes=2)
        with pytest.raises(RuntimeError):
            ZDD.import_binary_str(data)
        kyotodd.set_import_limit(max_nodes=0)
        assert ZDD.import_binary_str(data) == z
