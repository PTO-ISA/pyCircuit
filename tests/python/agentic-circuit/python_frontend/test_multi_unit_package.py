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
GOLDENS = REPOSITORY / "tests" / "goldens" / "agentic-circuit" / "source-map"
MANIFEST_SCHEMA = (
    REPOSITORY / "schemas" / "agentic-circuit" / "module-manifest.schema.json"
)
MANIFEST_GOLDENS = (
    REPOSITORY / "tests" / "goldens" / "agentic-circuit" / "module-manifest"
)

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

# A source unit is imported by the capture worker, so its dependent annotations
# have to stay lazy.
SPECIALIZED_STAGE = """
from __future__ import annotations

import agentic_circuit as ac


@ac.module_decl(
    source="source/stage.py",
    parameters=(
        ac.static_parameter("lanes", ac.static_int(width=4, signed=False)),
    ),
    finite_cases=(
        ac.case(("lanes", 2)),
        ac.case(("lanes", 4)),
    ),
)
def stage(
    value: ac.Queue[ac.u8, lanes, 2],
) -> ac.Queue[ac.u8, lanes, 2]:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(
    value: ac.Queue[ac.u8, lanes, 2],
) -> ac.Queue[ac.u8, lanes, 2]:
    return value
"""

SPECIALIZED_CORE = """
from __future__ import annotations

import agentic_circuit as ac

from source.stage import stage


@ac.system
def probe(
    low: ac.Queue[ac.u8, 2, 2],
    high: ac.Queue[ac.u8, 4, 2],
) -> tuple[ac.Queue[ac.u8, 2, 2], ac.Queue[ac.u8, 4, 2]]:
    first = stage(low, static=ac.case(("lanes", 2)))
    second = stage(high, static=ac.case(("lanes", 4)))
    return first, second
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

    def _build_package(self, modules: tuple[str, ...] = ("child_a", "child_b")) -> Path:
        acc = _repository_tool("acc")
        assert acc is not None
        package = self.root / "package"
        (package / "interfaces" / "source").mkdir(parents=True)
        (package / "interfaces" / "_compiler").mkdir(parents=True)

        for name in modules:
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

        manifest = json.loads(
            (bundle / "share" / "generated" / "module-manifest.json").read_text(
                encoding="utf-8"
            )
        )
        Draft202012Validator(
            json.loads(MANIFEST_SCHEMA.read_text(encoding="utf-8"))
        ).validate(manifest)
        self.assertEqual("Top", manifest["definition"])
        self.assertEqual(
            ["Top", "child_a", "child_b"],
            sorted(module["symbol"] for module in manifest["modules"]),
        )
        self.assertEqual(
            ["child_a", "child_b"],
            [instance["definition"] for instance in manifest["instances"]],
        )
        # Interface ports stay ordered inputs-then-outputs, which is the order
        # the case signature materializes against.
        for module in manifest["modules"]:
            directions = [port["direction"] for port in module["interface"]]
            self.assertGreaterEqual(len(directions), 2)
            self.assertEqual(
                sorted(directions, key=lambda item: 0 if item == "input" else 1),
                directions,
            )
        self.assertEqual(
            ["child_a_0", "child_b_1"],
            [instance["name"] for instance in document["module_instances"]],
        )
        # Publication keeps canonical provenance and the capture keeps the
        # original call site, not the flattened text position.
        core_lines = (self.root / "source" / "core.py").read_text(
            encoding="utf-8"
        ).splitlines()

        def call_line(needle: str) -> int:
            return next(
                index + 1
                for index, text in enumerate(core_lines)
                if needle in text
            )

        self.assertEqual(
            [
                {
                    "origins": [
                        {
                            "frames": [
                                {
                                    "column": 11,
                                    "file": "source/core.py",
                                    "kind": "statement",
                                    "line": call_line("= child_a("),
                                }
                            ]
                        }
                    ]
                },
                {
                    "origins": [
                        {
                            "frames": [
                                {
                                    "column": 11,
                                    "file": "source/core.py",
                                    "kind": "statement",
                                    "line": call_line("= child_b("),
                                }
                            ]
                        }
                    ]
                },
            ],
            [
                instance["source_provenance"]
                for instance in document["module_instances"]
            ],
        )

    def test_multi_unit_source_map_matches_golden(self) -> None:
        """Item V05: the module construct is pinned by a source-map golden."""

        self._require_native_flow()
        self._write("source/child_a.py", CHILD_A)
        self._write("source/child_b.py", CHILD_B)
        self._write("source/core.py", CORE)

        bundle = self._build_package()

        document = (bundle / "share" / "generated" / "source-map.json").read_bytes()
        Draft202012Validator(
            json.loads(SCHEMA.read_text(encoding="utf-8"))
        ).validate(json.loads(document))
        self.assertEqual(
            (GOLDENS / "module.json").read_bytes(),
            document,
            "module source-map golden drifted",
        )

    def test_specialized_family_source_map_matches_golden(self) -> None:
        """Item V05: the specialization construct is pinned by a golden."""

        self._require_native_flow()
        self._write("source/stage.py", SPECIALIZED_STAGE)
        self._write("source/core.py", SPECIALIZED_CORE)

        bundle = self._build_package(("stage",))

        document = (bundle / "share" / "generated" / "source-map.json").read_bytes()
        Draft202012Validator(
            json.loads(SCHEMA.read_text(encoding="utf-8"))
        ).validate(json.loads(document))
        self.assertEqual(
            (GOLDENS / "specialization.json").read_bytes(),
            document,
            "specialization source-map golden drifted",
        )
        # Two placements of one definition are distinguishable only by their
        # ordered typed static arguments, which is the manifest identity the
        # issue asks for.
        instances = json.loads(document)["module_instances"]
        self.assertEqual(["stage", "stage"], [item["definition"] for item in instances])
        self.assertEqual(
            [
                ("stage_0", [("lanes", "2 : i4")]),
                ("stage_1", [("lanes", "4 : i4")]),
            ],
            [
                (
                    item["name"],
                    [
                        (argument["name"], argument["value"].split(", ")[-1][:-2])
                        for argument in item["static_arguments"]
                    ],
                )
                for item in instances
            ],
        )

        manifest = (
            bundle / "share" / "generated" / "module-manifest.json"
        ).read_bytes()
        Draft202012Validator(
            json.loads(MANIFEST_SCHEMA.read_text(encoding="utf-8"))
        ).validate(json.loads(manifest))
        self.assertEqual(
            (MANIFEST_GOLDENS / "specialization.json").read_bytes(),
            manifest,
            "module manifest golden drifted",
        )
        stage = next(
            module
            for module in json.loads(manifest)["modules"]
            if module["symbol"] == "stage"
        )
        self.assertEqual(
            [
                ("lanes", "true", "#ac.static_type<#ac.static_int_type<4, false>>"),
            ],
            [
                (parameter["name"], str(parameter["required"]).lower(), parameter["type"])
                for parameter in stage["parameters"]
            ],
        )
        self.assertEqual(2, len(stage["declared_cases"]))
        self.assertEqual(2, len(stage["cases"]))
        self.assertTrue(
            all(module_case["signature"].startswith("(!ac.queue<i8")
                for module_case in stage["cases"])
        )

        # The published unit carries every declared case, so the golden covers a
        # specialization rather than a single concrete module.
        unit = (self.root / "package" / "sources_stage.ac").read_text(encoding="utf-8")
        self.assertEqual(2, unit.count("ac.module.case arguments"))
        self.assertIn('#ac.static_argument<"lanes"', unit)
        self.assertIn("2 : i4", unit)
        self.assertIn("4 : i4", unit)

    def test_lower_sources_returns_the_bundle_and_manifest(self) -> None:
        """The Python API drives the flow and returns the published manifest."""

        self._require_native_flow()
        self._write("source/child_a.py", CHILD_A)
        self._write("source/child_b.py", CHILD_B)
        self._write("source/core.py", CORE)
        acc = _repository_tool("acc")
        assert acc is not None

        from agentic_circuit import bundle

        output = self.root / "api-bundle"
        result = bundle.lower_sources(
            self.root / "source" / "core.py", output=output, acc=acc
        )

        self.assertEqual(output, result.bundle)
        self.assertIsNone(result.package)
        self.assertEqual(
            [
                "CMakeLists.txt",
                "include/generated/dut.h",
                "include/generated/interfaces/Top_interface.hpp",
                "include/generated/model.h",
                "include/generated/modules/child_a.hpp",
                "include/generated/modules/child_b.hpp",
                "include/generated/modules/queuegraph_helpers.hpp",
                "share/generated/cost-report.json",
                "share/generated/module-manifest.json",
                "share/generated/source-map.json",
                "src/generated/helpers/queuegraph_helpers.cpp",
                "src/generated/model.cpp",
                "src/generated/modules/child_a.cpp",
                "src/generated/modules/child_b.cpp",
                "src/generated/queuegraph.cpp",
            ],
            list(result.files),
        )
        self.assertEqual("Top", result.manifest["definition"])
        self.assertEqual(
            ["child_a", "child_b"],
            [item["definition"] for item in result.manifest["instances"]],
        )

        retained = self.root / "api-package"
        second = bundle.lower_sources(
            self.root / "source" / "core.py",
            output=self.root / "api-bundle-retained",
            acc=acc,
            package=retained,
        )
        self.assertEqual(retained, second.package)
        # The retained package keeps the canonical logical paths: one source
        # unit per Python source and one interface header beside it.
        self.assertTrue((retained / "core.ac").is_file())
        self.assertTrue((retained / "sources" / "source" / "child_a.ac").is_file())
        self.assertTrue((retained / "sources" / "source" / "child_b.ac").is_file())
        self.assertTrue(
            (retained / "interfaces" / "source" / "child_a.ac").is_file()
        )
        self.assertTrue(
            (retained / "interfaces" / "_compiler" / "layouts.ac").is_file()
        )

    def test_published_source_unit_keeps_canonical_provenance(self) -> None:
        self._require_native_flow()
        self._write("source/child_a.py", CHILD_A)
        self._write("source/child_b.py", CHILD_B)
        self._write("source/core.py", CORE)

        self._build_package()

        unit = (
            self.root / "package" / "sources_child_a.ac"
        ).read_text(encoding="utf-8")
        child_lines = (self.root / "source" / "child_a.py").read_text(
            encoding="utf-8"
        ).splitlines()
        rule_line = next(
            index + 1
            for index, text in enumerate(child_lines)
            if "return Mid(" in text
        )

        # A published unit carries the canonical attribute, not MLIR locations.
        self.assertIn("ac.source_provenance = [", unit)
        self.assertNotIn("loc(\"source/child_a.py\"", unit)
        self.assertIn(
            "{column = 18 : i64, file = \"source/child_a.py\", "
            f"kind = \"statement\", line = {rule_line} : i64}}",
            unit,
        )

    def test_imported_pure_helper_is_inlined_before_source_publication(self) -> None:
        self._require_native_flow()
        self._write(
            "source/arithmetic.py",
            """import agentic_circuit as ac

def add_one(value: ac.u8) -> ac.u8:
    return value + 1
""",
        )
        self._write(
            "source/child_a.py",
            CHILD_A.replace(
                "import agentic_circuit as ac",
                "import agentic_circuit as ac\nfrom source.arithmetic import add_one",
            ).replace("return Mid(v=x.a)", "return Mid(v=add_one(x.a))"),
        )
        output = self.root / "child_a.ac"
        self._compile(
            ["-c", str(self.root / "source" / "child_a.py"),
             "-o", str(output), "--quiet"]
        )
        unit = output.read_text(encoding="utf-8")
        self.assertNotIn("func.func private @add_one", unit)
        self.assertNotIn("func.call @add_one", unit)
        self.assertIn('file = "source/arithmetic.py"', unit)

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
