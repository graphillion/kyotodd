import pytest
import kyotodd
from kyotodd import BDD, ZDD


class TestBDDExportImportStr:
    def test_roundtrip_false(self):
        s = BDD.false_.export_str()
        assert BDD.import_str(s) == BDD.false_

    def test_roundtrip_true(self):
        s = BDD.true_.export_str()
        assert BDD.import_str(s) == BDD.true_

    def test_roundtrip_var(self):
        kyotodd.newvar()
        x = BDD.var(1)
        s = x.export_str()
        result = BDD.import_str(s)
        assert result == x

    def test_roundtrip_complex(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        x, y, z = BDD.var(1), BDD.var(2), BDD.var(3)
        f = (x & y) | (~x & z)
        s = f.export_str()
        result = BDD.import_str(s)
        assert result == f

    def test_export_str_not_empty(self):
        kyotodd.newvar()
        x = BDD.var(1)
        s = x.export_str()
        assert len(s) > 0


class TestBDDExportImportFile:
    def test_roundtrip_file(self, tmp_path):
        kyotodd.newvar()
        kyotodd.newvar()
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
        kyotodd.newvar()
        kyotodd.newvar()
        a = ZDD(1).change(1)  # {{1}}
        b = ZDD(1).change(2)  # {{2}}
        f = a + b  # {{1}, {2}}
        s = f.export_str()
        result = ZDD.import_str(s)
        assert result == f

    def test_roundtrip_complex(self):
        kyotodd.newvar()
        kyotodd.newvar()
        kyotodd.newvar()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        c = ZDD(1).change(3)
        f = (a * b) + c  # {{1,2}, {3}}
        s = f.export_str()
        result = ZDD.import_str(s)
        assert result == f


class TestZDDExportImportFile:
    def test_roundtrip_file(self, tmp_path):
        kyotodd.newvar()
        kyotodd.newvar()
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
