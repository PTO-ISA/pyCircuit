from __future__ import annotations

import importlib.util
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parents[4]
FIXTURES = Path(__file__).resolve().parent / "fixtures" / "lowering"
ZERO_DIGEST = "sha256:" + "0" * 64


def load_fixture(path: Path):
    spec = importlib.util.spec_from_file_location(f"lowering_{path.stem}", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import lowering fixture {path}")
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


def registry(*, source_binding: str | None = "input"):
    from agentic_circuit._schemas import (
        ComponentSchema,
        PortSchema,
        ResultSchema,
        SchemaRegistry,
    )

    refine = ComponentSchema(
        identity="test.Refine",
        fingerprint=ZERO_DIGEST,
        ports=(PortSchema("input", "flow", "i32", "in", "consumer", 1),),
        results=(ResultSchema("output", "i32", source_binding),),
        parameters=(),
        availability="available",
        effect_kind="stateful",
    )
    return SchemaRegistry({refine.identity: refine})


def elaborate(path: Path, workspace: Path, *, schemas=None):
    from agentic_circuit._frontend import CaptureRequest, elaborate_frontend

    loaded = load_fixture(path)
    return elaborate_frontend(
        CaptureRequest(entry=path, workspace=workspace, system="main"),
        vars(loaded),
        schemas or registry(),
    )


def golden_bytes(name: str) -> bytes:
    return (FIXTURES / name).read_bytes().rstrip(b"\n")


def golden_text(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def acir_opt() -> Path | None:
    roots = [REPOSITORY / "build"]
    if REPOSITORY.parent.name == ".worktrees":
        roots.append(REPOSITORY.parent.parent / "build")
    for root in roots:
        candidate = root / "dev-llvm22" / "bin" / "acir-opt"
        if candidate.is_file():
            return candidate
    return None


class AcirLoweringTest(unittest.TestCase):
    def test_hierarchy_matches_goldens(self) -> None:
        result = elaborate(FIXTURES / "hierarchy.py", FIXTURES)

        self.assertEqual((), result.diagnostics)
        self.assertIsNotNone(result.document)
        self.assertIsNotNone(result.acir)
        assert result.document is not None and result.acir is not None
        self.assertEqual(
            golden_bytes("hierarchy.acpy.json"), result.document.canonical_bytes()
        )
        self.assertEqual(golden_text("hierarchy.ac.mlir"), result.acir)
        self.assertNotIn("ac.connect", result.acir)

    def test_process_matches_golden_and_native_verifier(self) -> None:
        result = elaborate(FIXTURES / "process.py", FIXTURES)

        self.assertEqual((), result.diagnostics)
        self.assertEqual(golden_text("process.ac.mlir"), result.acir)
        verifier = acir_opt()
        if verifier is None:
            self.skipTest("acir-opt is not built")
        completed = subprocess.run(
            (str(verifier), "-o", "/dev/null", "-"),
            input=result.acir,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)

    def test_equivalent_roots_emit_identical_bytes(self) -> None:
        with (
            tempfile.TemporaryDirectory() as first_root,
            tempfile.TemporaryDirectory() as second_root,
        ):
            first_path = Path(first_root) / "hierarchy.py"
            second_path = Path(second_root) / "hierarchy.py"
            shutil.copyfile(FIXTURES / "hierarchy.py", first_path)
            shutil.copyfile(FIXTURES / "hierarchy.py", second_path)

            first = elaborate(first_path, Path(first_root))
            second = elaborate(second_path, Path(second_root))

        assert first.document is not None and second.document is not None
        self.assertEqual(
            first.document.canonical_bytes(), second.document.canonical_bytes()
        )
        self.assertEqual(first.acir, second.acir)

    def test_lowering_failure_does_not_publish_partial_artifacts(self) -> None:
        result = elaborate(
            FIXTURES / "hierarchy.py",
            FIXTURES,
            schemas=registry(source_binding=None),
        )

        self.assertIsNone(result.document)
        self.assertIsNone(result.acir)
        self.assertEqual(
            ("ACPY-VERIFY-001",), tuple(d.code for d in result.diagnostics)
        )


if __name__ == "__main__":
    unittest.main()
