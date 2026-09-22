"""The multi-unit AC package flow links and emits a structured bundle.

Issue #180 records that no in-repo test exercises the multi-unit
``--header-output`` flow, so the cross-unit contract was never run end to end.
The flow is:

1. compile each module source with ``acc.py -c <module>.py -o <unit>.ac
   --header-output <unit header>.ac``;
2. compile the root source with ``acc.py -c <root>.py -o core.ac`` and publish
   the compiler interface units with ``acc.py -c <root>.py --unit interfaces``;
3. link the directory with the native ``acc -c <package> -verify``; and
4. emit the structured C++ bundle with ``acc -c <package> -emit-cpp-bundle``.

Linking compares the importing unit's ``ac.module.import`` schema with the
provider's published schema byte for byte, so this test also pins the three
frontend properties that make the comparison hold: the declaration owns the
nominal declaration inventory its ports need, the implementation publishes the
declared interface skeleton, and every nominal declaration names the Python file
that owns it.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from jsonschema import Draft202012Validator

REPOSITORY = Path(__file__).resolve().parents[4]
SCHEMA = REPOSITORY / "schemas" / "agentic-circuit" / "source-map.schema.json"

MANIFEST = """[project]
name = "multi-unit-fixture"
version = "0.1.0"
architecture = "source/core.py"
system = "probe"

[providers]
standard_library = ["ac"]

[build]
profile = "fast"
compiler = "c++"
standard_library = "libc++"
component_roots = ["components"]
protocol_roots = ["protocols"]
build_root = "build"
instrumentation_layers = []

[run]
trace_roots = ["traces"]
inputs = {}

[diagnostics]
format = "text"
"""

CHILD_A = """
import agentic_circuit as ac


@ac.struct
class In:
    a: ac.u8


@ac.struct
class Mid:
    v: ac.u8


@ac.rule
def form_mid(x: In) -> Mid:
    return Mid(v=x.a)


@ac.module_decl(source="source/child_a.py")
def child_a(x: In) -> Mid:
    ...


child_a_ref = child_a


@ac.module(declaration=child_a_ref)
def child_a(x: In) -> Mid:
    mid = form_mid(x)
    return mid
"""

CHILD_B = """
import agentic_circuit as ac

from source.child_a import Mid


@ac.struct
class Out:
    v: ac.u8


@ac.rule
def form_out(m: Mid) -> Out:
    return Out(v=m.v)


@ac.module_decl(source="source/child_b.py")
def child_b(m: Mid) -> Out:
    ...


child_b_ref = child_b


@ac.module(declaration=child_b_ref)
def child_b(m: Mid) -> Out:
    out = form_out(m)
    return out
"""

CORE = """
import agentic_circuit as ac

from source.child_a import In, child_a
from source.child_b import Out, child_b


@ac.system
def probe(x: In) -> Out:
    mid = child_a(x)
    out = child_b(mid)
    return out
"""


def _repository_tool(name: str) -> Path | None:
    for candidate in (
        REPOSITORY / ".pycircuit_out/toolchain/build/bin" / name,
        REPOSITORY / ".pycircuit_out/acir/dev-llvm22/bin" / name,
    ):
        if candidate.is_file():
            return candidate
    return None


def _native_extension_available() -> bool:
    try:
        from agentic_circuit import _native  # noqa: F401
    except ImportError:
        return False
    return True


class MultiUnitPackageTest(unittest.TestCase):
    """Issue #180: the multi-unit flow links into a structured bundle."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        (self.root / "source").mkdir()
        (self.root / "source" / "__init__.py").write_text("", encoding="utf-8")
        (self.root / "agentic-circuit.toml").write_text(MANIFEST, encoding="utf-8")

    def _write(self, relative: str, text: str) -> Path:
        path = self.root / relative
        path.write_text(text, encoding="utf-8")
        return path

    def _compile(self, arguments: list[str]) -> None:
        completed = subprocess.run(
            [sys.executable, "-m", "agentic_circuit._acc_py", *arguments],
            text=True,
            capture_output=True,
            check=False,
            cwd=self.root,
            env={**os.environ, "PYTHONHASHSEED": "0"},
        )
        self.assertEqual(
            0,
            completed.returncode,
            f"acc.py failed: {completed.stdout}\n{completed.stderr}",
        )

    def _build_package(self) -> Path:
        acc = _repository_tool("acc")
        assert acc is not None
        package = self.root / "package"
        (package / "interfaces" / "source").mkdir(parents=True)
        (package / "interfaces" / "_compiler").mkdir(parents=True)

        for name in ("child_a", "child_b"):
            self._compile(
                [
                    "-c",
                    str(self.root / "source" / f"{name}.py"),
                    "-o",
                    str(package / f"sources_{name}.ac"),
                    "--header-output",
                    str(package / "interfaces" / "source" / f"{name}.ac"),
                    "--quiet",
                ]
            )
        self._compile(
            [
                "-c",
                str(self.root / "source" / "core.py"),
                "-o",
                str(package / "core.ac"),
                "--quiet",
            ]
        )
        interfaces = self.root / "interfaces"
        self._compile(
            [
                "-c",
                str(self.root / "source" / "core.py"),
                "--unit",
                "interfaces",
                "-o",
                str(interfaces),
                "--quiet",
            ]
        )
        for path in sorted((interfaces / "_compiler").glob("*.ac")):
            shutil.copy(path, package / "interfaces" / "_compiler" / path.name)

        verified = subprocess.run(
            [str(acc), "-c", str(package), "-verify"],
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, verified.returncode, verified.stderr)

        bundle = self.root / "bundle"
        emitted = subprocess.run(
            [str(acc), "-c", str(package), "-emit-cpp-bundle", "-o", str(bundle)],
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, emitted.returncode, emitted.stderr)
        return bundle

    def _require_native_flow(self) -> None:
        if _repository_tool("acc") is None:
            self.skipTest("the native acc compiler is not built")
        if not _native_extension_available():
            self.skipTest("the Agentic Circuit native extension is unavailable")

    def test_multi_unit_hierarchy_links_into_a_structured_bundle(self) -> None:
        self._require_native_flow()
        self._write("source/child_a.py", CHILD_A)
        self._write("source/child_b.py", CHILD_B)
        self._write("source/core.py", CORE)

        bundle = self._build_package()

        expected = (
            "CMakeLists.txt",
            "include/generated/dut.h",
            "include/generated/model.h",
            "include/generated/modules/child_a.hpp",
            "include/generated/modules/child_b.hpp",
            "share/generated/cost-report.json",
            "share/generated/source-map.json",
            "src/generated/model.cpp",
            "src/generated/modules/child_a.cpp",
            "src/generated/modules/child_b.cpp",
            "src/generated/queuegraph.cpp",
        )
        for relative in expected:
            self.assertTrue(
                (bundle / relative).is_file(),
                f"bundle is missing {relative}: "
                f"{sorted(str(p.relative_to(bundle)) for p in bundle.rglob('*'))}",
            )

    def test_multi_unit_hierarchy_reports_its_module_instances(self) -> None:
        self._require_native_flow()
        self._write("source/child_a.py", CHILD_A)
        self._write("source/child_b.py", CHILD_B)
        self._write("source/core.py", CORE)

        bundle = self._build_package()

        document = json.loads(
            (bundle / "share" / "generated" / "source-map.json").read_text(
                encoding="utf-8"
            )
        )
        Draft202012Validator(
            json.loads(SCHEMA.read_text(encoding="utf-8"))
        ).validate(document)

        self.assertEqual("Top", document["definition"])
        self.assertEqual("probe", document["system"])
        self.assertEqual(
            ["child_a", "child_b"],
            [instance["definition"] for instance in document["module_instances"]],
        )
        self.assertEqual(
            ["child_a_0", "child_b_1"],
            [instance["name"] for instance in document["module_instances"]],
        )

    def test_each_source_owns_its_nominals_and_import_header(self) -> None:
        """A source header carries its own nominals and its own import."""

        self._require_native_flow()
        self._write("source/child_a.py", CHILD_A)
        self._write("source/child_b.py", CHILD_B)
        self._write("source/core.py", CORE)

        self._build_package()

        header_a = (
            self.root / "package" / "interfaces" / "source" / "child_a.ac"
        ).read_text(encoding="utf-8")
        header_b = (
            self.root / "package" / "interfaces" / "source" / "child_b.ac"
        ).read_text(encoding="utf-8")

        # The declaring file owns each nominal, so shared types are declared once.
        self.assertIn(
            'ac.struct @In fields [{name = "a", type = i8}] '
            '{ac.source_file = "source/child_a.py"}',
            header_a,
        )
        self.assertIn(
            'ac.struct @Mid fields [{name = "v", type = i8}] '
            '{ac.source_file = "source/child_a.py"}',
            header_a,
        )
        self.assertIn(
            'ac.struct @Out fields [{name = "v", type = i8}] '
            '{ac.source_file = "source/child_b.py"}',
            header_b,
        )
        self.assertNotIn("ac.struct @In", header_b)
        self.assertNotIn("ac.struct @Mid", header_b)

        # The import header carries the nominals the declared ports need, so the
        # linker compares it against the provider's schema successfully.
        self.assertIn('ac.module.import @child_a', header_a)
        self.assertIn('ac.module.import @child_b', header_b)
        self.assertIn(
            ']>, <"source/child_a.py", "source/child_a.py">, [@In, @Mid]>',
            header_a,
        )
        self.assertIn(
            ']>, <"source/child_b.py", "source/child_b.py">, [@Mid, @Out]>',
            header_b,
        )


if __name__ == "__main__":
    unittest.main()
