import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def load_coverage_checker():
    path = ROOT / "scripts/check-ir-coverage.py"
    spec = importlib.util.spec_from_file_location("coverage_checker", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


ACIR_MANIFEST = """schema: acir-ir-inventory
contract_epoch: "0.4"
dialect: acir
operations:
  - ac.system
types:
  - struct
"""

ACSIM_MANIFEST = """schema: acir-ir-inventory
contract_epoch: "0.4"
dialect: acsim
operations:
  - acsim.model
types:
  - value
"""

FIXTURE_FILES = {
    "contracts/acir.yaml": ACIR_MANIFEST,
    "contracts/acsim.yaml": ACSIM_MANIFEST,
    "include/acir/Dialect/ACIR/ACIROps.td": (
        'def ACIR_SystemOp : ACIR_Op<"system", [Symbol]> {\n}\n'
    ),
    "include/acir/Dialect/ACIR/ACIRTypes.td": (
        'def ACIR_StructType : ACIR_LayoutNamedType<"Struct", "struct">;\n'
    ),
    "include/acir/Dialect/ACSim/ACSimOps.td": (
        'def ACSim_ModelOp : ACSim_Op<"model", [Symbol]> {\n}\n'
    ),
    "include/acir/Dialect/ACSim/ACSimTypes.td": (
        'def ACSim_ValueType : ACSim_SymbolType<"Value", "value">;\n'
    ),
    "lib/Dialect/ACIR/ACIRTypes.cpp": (
        "void initialize() {\n"
        "  addTypes<\n"
        '#include "acir/Dialect/ACIR/ACIRTypes.cpp.inc"\n'
        "      >();\n"
        "  addOperations<\n"
        '#include "acir/Dialect/ACIR/ACIROps.cpp.inc"\n'
        "      >();\n"
        "}\n"
    ),
    "lib/Dialect/ACSim/ACSimTypes.cpp": (
        "void initialize() {\n"
        "  addTypes<\n"
        '#include "acir/Dialect/ACSim/ACSimTypes.cpp.inc"\n'
        "      >();\n"
        "  addOperations<\n"
        '#include "acir/Dialect/ACSim/ACSimOps.cpp.inc"\n'
        "      >();\n"
        "}\n"
    ),
    "test/ACIR/system-valid.mlir": "ac.system @soc root @Top {}\n!ac.struct<@s>\n",
    "test/ACIR/system-invalid.mlir": "ac.system\n!ac.struct<@s>\n",
    "test/ACSim/model-valid.mlir": "acsim.model @m {}\n!acsim.value<@v>\n",
    "test/ACSim/model-invalid.mlir": "acsim.model\n!acsim.value<@v>\n",
}


def initialize_coverage_fixture(overrides=None, removals=()):
    temporary_directory = tempfile.TemporaryDirectory()
    root = Path(temporary_directory.name)
    files = dict(FIXTURE_FILES)
    if overrides:
        files.update(overrides)
    for relative_path, content in files.items():
        path = root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
    for relative_path in removals:
        (root / relative_path).unlink()
    checker = load_coverage_checker()
    errors = []
    surface = checker.check_ods_surface(root, errors)
    if not errors:
        ledger = root / checker.LEDGER_PATH
        ledger.parent.mkdir(parents=True, exist_ok=True)
        ledger.write_text(checker.render_ledger(root, surface))
    return temporary_directory, root


class IRCoverageTest(unittest.TestCase):
    def check_fixture(self, root):
        checker = load_coverage_checker()
        return checker.run_checks(root)

    def test_read_only_checker_accepts_the_repository(self):
        before = subprocess.run(
            ["git", "status", "--porcelain=v1"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        ).stdout
        result = subprocess.run(
            [sys.executable, "scripts/check-ir-coverage.py"],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        after = subprocess.run(
            ["git", "status", "--porcelain=v1"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        ).stdout
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertEqual(before, after, "coverage checker modified the repository")

    def test_minimal_fixture_passes(self):
        temporary_directory, root = initialize_coverage_fixture()
        self.addCleanup(temporary_directory.cleanup)
        self.assertEqual([], self.check_fixture(root))

    def test_manifest_missing_operation_is_reported(self):
        temporary_directory, root = initialize_coverage_fixture(
            overrides={
                "contracts/acir.yaml": ACIR_MANIFEST.replace(
                    "operations:\n  - ac.system\n", "operations: []\n"
                )
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        errors = self.check_fixture(root)
        self.assertTrue(
            any("implementation-only public alias" in error for error in errors),
            errors,
        )

    def test_manifest_extra_operation_needs_source_symbol(self):
        temporary_directory, root = initialize_coverage_fixture(
            overrides={
                "contracts/acir.yaml": ACIR_MANIFEST.replace(
                    "  - ac.system\n", "  - ac.system\n  - ac.ghost\n"
                )
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        errors = self.check_fixture(root)
        self.assertTrue(
            any("ac.ghost" in error and "no source symbol" in error for error in errors),
            errors,
        )

    def test_stale_epoch_is_rejected(self):
        temporary_directory, root = initialize_coverage_fixture(
            overrides={
                "contracts/acsim.yaml": ACSIM_MANIFEST.replace(
                    '"0.4"', '"0.0"'
                )
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        errors = self.check_fixture(root)
        self.assertTrue(
            any("stale contract_epoch" in error for error in errors), errors
        )

    def test_missing_negative_coverage_is_reported(self):
        temporary_directory, root = initialize_coverage_fixture(
            removals=("test/ACIR/system-invalid.mlir",)
        )
        self.addCleanup(temporary_directory.cleanup)
        errors = self.check_fixture(root)
        self.assertTrue(
            any(
                "ac.system" in error and "no negative lit test coverage" in error
                for error in errors
            ),
            errors,
        )
        self.assertTrue(
            any(
                "!ac.struct" in error and "no negative lit test coverage" in error
                for error in errors
            ),
            errors,
        )

    def test_missing_positive_coverage_is_reported(self):
        temporary_directory, root = initialize_coverage_fixture(
            removals=("test/ACSim/model-valid.mlir",)
        )
        self.addCleanup(temporary_directory.cleanup)
        errors = self.check_fixture(root)
        self.assertTrue(
            any(
                "acsim.model" in error and "no positive lit test coverage" in error
                for error in errors
            ),
            errors,
        )

    def test_substring_coverage_does_not_count(self):
        # ac.systemx must not satisfy ac.system coverage.
        temporary_directory, root = initialize_coverage_fixture(
            overrides={
                "test/ACIR/system-valid.mlir": "ac.systemx @soc {}\n!ac.struct<@s>\n"
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        errors = self.check_fixture(root)
        self.assertTrue(
            any(
                "ac.system" in error and "no positive lit test coverage" in error
                for error in errors
            ),
            errors,
        )

    def test_missing_registration_table_is_reported(self):
        temporary_directory, root = initialize_coverage_fixture(
            overrides={
                "lib/Dialect/ACSim/ACSimTypes.cpp": "void initialize() {}\n"
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        errors = self.check_fixture(root)
        self.assertTrue(
            any("does not register the generated" in error for error in errors),
            errors,
        )

    def test_stale_ledger_is_rejected(self):
        temporary_directory, root = initialize_coverage_fixture()
        self.addCleanup(temporary_directory.cleanup)
        ledger = root / "docs/spec/50-verification/ir-coverage.md"
        ledger.write_text(ledger.read_text() + "\nstale edit\n")
        errors = self.check_fixture(root)
        self.assertTrue(any("is stale" in error for error in errors), errors)

    def test_missing_ledger_is_reported(self):
        temporary_directory, root = initialize_coverage_fixture()
        self.addCleanup(temporary_directory.cleanup)
        (root / "docs/spec/50-verification/ir-coverage.md").unlink()
        errors = self.check_fixture(root)
        self.assertTrue(
            any("missing coverage ledger" in error for error in errors), errors
        )


if __name__ == "__main__":
    unittest.main()
