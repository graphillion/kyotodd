import pytest
import kyotodd
from kyotodd import BDD, ZDD, UnreducedDD, SeqBDD


class TestBDDExportImportStr:
    def test_roundtrip_false(self):
        s = BDD.false_.export_str()
        assert BDD.import_str(s) == BDD.false_

    def test_roundtrip_true(self):
        s = BDD.true_.export_str()
        assert BDD.import_str(s) == BDD.true_

    def test_roundtrip_var(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = x.export_str()
        result = BDD.import_str(s)
        assert result == x

    def test_roundtrip_complex(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        x, y, z = BDD.var(1), BDD.var(2), BDD.var(3)
        f = (x & y) | (~x & z)
        s = f.export_str()
        result = BDD.import_str(s)
        assert result == f

    def test_export_str_not_empty(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = x.export_str()
        assert len(s) > 0


class TestBDDExportImportFile:
    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        x, y = BDD.var(1), BDD.var(2)
        f = x & y
        path = str(tmp_path / "bdd.txt")
        f.export_file(path)
        result = BDD.import_file(path)
        assert result == f

    def test_import_nonexistent_file(self):
        with pytest.raises(RuntimeError):
            BDD.import_file("/nonexistent/path/file.txt")


class TestZDDExportImportStr:
    def test_roundtrip_empty(self):
        s = ZDD.empty.export_str()
        assert ZDD.import_str(s) == ZDD.empty

    def test_roundtrip_single(self):
        s = ZDD.single.export_str()
        assert ZDD.import_str(s) == ZDD.single

    def test_roundtrip_family(self):
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)  # {{1}}
        b = ZDD(1).change(2)  # {{2}}
        f = a + b  # {{1}, {2}}
        s = f.export_str()
        result = ZDD.import_str(s)
        assert result == f

    def test_roundtrip_complex(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        c = ZDD(1).change(3)
        f = (a * b) + c  # {{1,2}, {3}}
        s = f.export_str()
        result = ZDD.import_str(s)
        assert result == f


class TestZDDExportImportFile:
    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        f = a + b
        path = str(tmp_path / "zdd.txt")
        f.export_file(path)
        result = ZDD.import_file(path)
        assert result == f

    def test_import_nonexistent_file(self):
        with pytest.raises(RuntimeError):
            ZDD.import_file("/nonexistent/path/file.txt")


class TestBDDKnuthIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        x, y = BDD.var(1), BDD.var(2)
        f = x & y
        s = f.export_knuth_str()
        assert len(s) > 0
        result = BDD.import_knuth_str(s)
        assert result == f

    def test_roundtrip_str_hex(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = x.export_knuth_str(is_hex=True)
        result = BDD.import_knuth_str(s, is_hex=True)
        assert result == x

    def test_roundtrip_str_terminal(self):
        s = BDD.false_.export_knuth_str()
        assert BDD.import_knuth_str(s) == BDD.false_
        s = BDD.true_.export_knuth_str()
        assert BDD.import_knuth_str(s) == BDD.true_

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        x, y = BDD.var(1), BDD.var(2)
        f = x | y
        path = str(tmp_path / "bdd_knuth.txt")
        f.export_knuth_file(path)
        result = BDD.import_knuth_file(path)
        assert result == f

    def test_import_nonexistent_file(self):
        with pytest.raises(RuntimeError):
            BDD.import_knuth_file("/nonexistent/path/file.txt")


class TestZDDKnuthIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        f = a + b
        s = f.export_knuth_str()
        assert len(s) > 0
        result = ZDD.import_knuth_str(s)
        assert result == f

    def test_roundtrip_str_hex(self):
        kyotodd.new_var()
        a = ZDD(1).change(1)
        s = a.export_knuth_str(is_hex=True)
        result = ZDD.import_knuth_str(s, is_hex=True)
        assert result == a

    def test_roundtrip_str_terminal(self):
        s = ZDD.empty.export_knuth_str()
        assert ZDD.import_knuth_str(s) == ZDD.empty
        s = ZDD.single.export_knuth_str()
        assert ZDD.import_knuth_str(s) == ZDD.single

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        f = a + b
        path = str(tmp_path / "zdd_knuth.txt")
        f.export_knuth_file(path)
        result = ZDD.import_knuth_file(path)
        assert result == f

    def test_import_nonexistent_file(self):
        with pytest.raises(RuntimeError):
            ZDD.import_knuth_file("/nonexistent/path/file.txt")


class TestBDDPrint:
    def test_print_returns_string(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = x.print()
        assert isinstance(s, str)
        assert len(s) > 0

    def test_print_terminal(self):
        s = BDD.true_.print()
        assert isinstance(s, str)


class TestZDDPrint:
    def test_print_returns_string(self):
        kyotodd.new_var()
        a = ZDD(1).change(1)
        s = a.print()
        assert isinstance(s, str)
        assert len(s) > 0

    def test_print_terminal(self):
        s = ZDD.single.print()
        assert isinstance(s, str)


class TestUnreducedDDZeroOne:
    def test_zero(self):
        z = UnreducedDD.zero()
        assert z == UnreducedDD(0)
        assert z.is_zero

    def test_one(self):
        o = UnreducedDD.one()
        assert o == UnreducedDD(1)
        assert o.is_one


class TestSeqBDDSeqStr:
    def test_seq_str_equals_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        s = SeqBDD.from_list([1, 2])
        assert s.seq_str() == str(s)

    def test_seq_str_empty(self):
        s = SeqBDD(0)
        result = s.seq_str()
        assert isinstance(result, str)
