from __future__ import annotations

import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from agentic_circuit._capture_worker import CaptureWorkerRequest, run_capture_worker
from agentic_circuit._commands.model import (
    SdkIdentity,
    _cmake_sources_for,
    _freeze,
    _validate_generated_inventory,
)
from jsonschema import Draft202012Validator

REPOSITORY = Path(__file__).resolve().parents[4]
BUILD = REPOSITORY / ".pycircuit_out/acir/dev-llvm22"
MODEL_PLAN_SCHEMA = REPOSITORY / "schemas/agentic-circuit/model-plan.schema.json"
MODEL_MANIFEST_SCHEMA = (
    REPOSITORY / "schemas/agentic-circuit/model-manifest.schema.json"
)
SOURCE_MAP_SCHEMA = REPOSITORY / "schemas/agentic-circuit/source-map.schema.json"
EMITTED_COST_SCHEMA = REPOSITORY / "schemas/agentic-circuit/emitted-cost.schema.json"
MODEL_CONSUMER = REPOSITORY / "tests/integration/agentic-circuit/model-install-consumer"
FIXTURE_SOURCE_REVISION = "a" * 40
PLAN_CORE_FILES = (
    "frozen.ac.mlir",
    "model-plan.json",
    "model-sources.cmake",
    "model.d",
    "queuegraph.json",
)
CORE_GENERATED_FILES = (
    "include/generated/model.h",
    "share/generated/cost-report.json",
    "share/generated/source-map.json",
    "src/generated/model.cpp",
    "src/generated/queuegraph.cpp",
)


def sha256(data: bytes) -> str:
    return "sha256:" + hashlib.sha256(data).hexdigest()


def file_kind(path: Path) -> str:
    value = path.as_posix()
    if value.startswith("bin/"):
        return "tool"
    if value.startswith("include/"):
        return "header"
    if value.startswith("lib/cmake/"):
        return "cmake"
    if value.startswith("python/wheelhouse/"):
        return "wheel"
    if value.startswith("share/pycircuit/schemas/"):
        return "schema"
    if value.startswith("share/pycircuit/licenses/"):
        return "license"
    if value.startswith("lib/") and path.suffix in {".a", ".dylib", ".so"}:
        return "library"
    return "metadata"


def platform_contract() -> tuple[dict[str, object], dict[str, object]]:
    if platform.system() == "Darwin":
        return (
            {
                "architecture": "arm64",
                "cxx_abi": "Apple libc++",
                "cxx_compiler": "AppleClang 21",
                "cxx_standard": "20",
                "host_triple": "arm64-apple-darwin",
                "id": "macos-arm64",
                "minimum_libc": None,
                "minimum_os": "macOS 15",
                "os": "macos",
                "python": "3.11",
                "runner": "macos-15",
            },
            {"kind": "system", "name": "libc++", "sha256": None, "version": "21"},
        )
    return (
        {
            "architecture": "x86_64",
            "cxx_abi": "libstdc++ CXX11 ABI",
            "cxx_compiler": "clang++ 22.1.8",
            "cxx_standard": "20",
            "host_triple": "x86_64-linux-gnu",
            "id": "linux-x86_64",
            "minimum_libc": "glibc 2.39",
            "minimum_os": "Ubuntu 24.04",
            "os": "linux",
            "python": "3.11",
            "runner": "ubuntu-24.04",
        },
        {"kind": "system", "name": "glibc", "sha256": None, "version": "2.39"},
    )


def install_sdk(prefix: Path) -> None:
    toolchain = os.environ.get("AC_GATE_TOOLCHAIN_ROOT")
    if toolchain is not None:
        shutil.copytree(toolchain, prefix, symlinks=True)
    else:
        completed = subprocess.run(
            ["cmake", "--install", str(BUILD), "--prefix", str(prefix)],
            cwd=REPOSITORY,
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode != 0:
            raise AssertionError(completed.stderr)
    schema_root = prefix / "share/pycircuit/schemas"
    license_root = prefix / "share/pycircuit/licenses"
    wheelhouse = prefix / "python/wheelhouse"
    schema_root.mkdir(parents=True, exist_ok=True)
    license_root.mkdir(parents=True, exist_ok=True)
    wheelhouse.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(MODEL_PLAN_SCHEMA, schema_root / "model-plan.schema.json")
    shutil.copyfile(MODEL_MANIFEST_SCHEMA, schema_root / "model-manifest.schema.json")
    shutil.copyfile(SOURCE_MAP_SCHEMA, schema_root / "source-map.schema.json")
    shutil.copyfile(EMITTED_COST_SCHEMA, schema_root / "emitted-cost.schema.json")
    shutil.copyfile(REPOSITORY / "LICENSE", license_root / "LICENSE")
    (wheelhouse / "agentic_circuit-0.1.0-py3-none-any.whl").write_bytes(b"wheel")

    files = []
    manifest_path = prefix / "share/pycircuit/sdk-manifest.json"
    for path in sorted(
        prefix.rglob("*"), key=lambda item: item.relative_to(prefix).as_posix()
    ):
        if not path.is_file() or path == manifest_path:
            continue
        data = path.read_bytes()
        relative = path.relative_to(prefix)
        files.append(
            {
                "path": relative.as_posix(),
                "kind": file_kind(relative),
                "sha256": sha256(data),
                "size": len(data),
            }
        )
    platform_value, dependency = platform_contract()
    manifest = {
        "schema": "pycircuit-sdk-platform-manifest",
        "version": "1",
        "contract_epoch": "0.5",
        "product_version": "6.1.0",
        "source_revision": FIXTURE_SOURCE_REVISION,
        "platform": platform_value,
        "distributions": {
            "agentic-circuit": "0.1.0",
            "pycircuit-hisi": "6.1.0",
            "pycircuit-semantic-core": "6.1.0",
        },
        "abi": {
            "acpy_epoch": "0.5",
            "acir_contract": "0.5",
            "sdk_manifest": "1",
            "release_index": "1",
            "model_plan": "1",
            "model_manifest": "1",
            "generator_abi": "1",
            "runtime_abi": "1",
            "consumer_lock": "1",
        },
        "capabilities": [
            "cycle-aware-signal",
            "pyc-cpp",
            "pyc-verilog",
            "agentic-model-plan",
            "agentic-model-emit-cpp",
            "gfsim-runtime-v1",
        ],
        "runtime_dependencies": [dependency],
        "self_path": "share/pycircuit/sdk-manifest.json",
        "files": files,
    }
    manifest_path.write_text(
        json.dumps(manifest, sort_keys=True, separators=(",", ":")) + "\n"
    )


def write_source(root: Path, *, delay_marker: Path | None = None) -> None:
    package = root / "model"
    package.mkdir(parents=True)
    (package / "__init__.py").write_text("")
    (package / "contracts.py").write_text(
        "import agentic_circuit as ac\n\n"
        "@ac.struct\n"
        "class Entry:\n"
        "    sequence: ac.u4\n"
        "    value: ac.u16\n"
        "    done: bool\n"
    )
    delay = (
        f"open({str(delay_marker)!r}, 'w').write('ready')\n"
        "for _delay in range(20_000_000):\n"
        "    pass\n"
        if delay_marker is not None
        else ""
    )
    (package / "top.py").write_text(
        delay + "import agentic_circuit as ac\n"
        "from .contracts import Entry\n\n"
        "@ac.rule\n"
        "def complete(entry):\n"
        "    return entry.with_fields(done=True)\n\n"
        "@ac.system\n"
        "def rob(entries: ac.const[int]) -> None:\n"
        "    issued = ac.source(Entry, depth=4, latency=1)\n"
        "    completed = complete(issued)\n"
        "    retired = ac.reorder(completed, by=Entry.sequence, "
        "entries=entries, start=0)\n"
        "    ac.sink(retired)\n"
    )
    (root / "model.toml").write_text('version = "1"\n\n[static]\nentries = 8\n')


def write_structured_source(root: Path) -> None:
    write_source(root)
    (root / "model/top.py").write_text(
        "import agentic_circuit as ac\n"
        "from .contracts import Entry\n\n"
        "@ac.module\n"
        "def complete(entry: Entry) -> Entry:\n"
        "    return entry.with_fields(done=True)\n\n"
        "@ac.system\n"
        "def rob(entry: Entry, *, entries: ac.const[int]) -> Entry:\n"
        "    ac.static_assert(entries > 0)\n"
        "    completed = complete(entry)\n"
        "    return completed\n"
    )


def plan_command(
    prefix: Path,
    source: Path,
    output: Path,
    *,
    config: Path | None = None,
    extra: tuple[str, ...] = (),
) -> tuple[list[str], dict[str, str]]:
    environment = os.environ.copy()
    environment.pop("PYTHONPATH", None)
    return (
        [
            sys.executable,
            str(prefix / "bin/agentic-circuit"),
            "model",
            "plan",
            "--sdk-root",
            str(prefix),
            "--source-root",
            str(source),
            "--entry",
            "model.top:rob",
            "--config",
            str(config) if config is not None else "model.toml",
            "--out-dir",
            str(output),
            "--json",
            *extra,
        ],
        environment,
    )


def run_plan(
    prefix: Path,
    source: Path,
    output: Path,
    *,
    config: Path | None = None,
    extra: tuple[str, ...] = (),
    hash_seed: int = 0,
) -> subprocess.CompletedProcess[str]:
    command, environment = plan_command(
        prefix, source, output, config=config, extra=extra
    )
    environment["PYTHONHASHSEED"] = str(hash_seed)
    return subprocess.run(
        command,
        cwd=source,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def emit_command(
    prefix: Path, plan: Path, output: Path
) -> tuple[list[str], dict[str, str]]:
    environment = os.environ.copy()
    environment.pop("PYTHONPATH", None)
    return (
        [
            sys.executable,
            str(prefix / "bin/agentic-circuit"),
            "model",
            "emit-cpp",
            "--sdk-root",
            str(prefix),
            "--plan",
            str(plan / "model-plan.json"),
            "--out-dir",
            str(output),
            "--manifest",
            str(output / "model-manifest.json"),
            "--depfile",
            str(output / "model.d"),
            "--json",
        ],
        environment,
    )


def run_emit(
    prefix: Path, plan: Path, output: Path
) -> subprocess.CompletedProcess[str]:
    command, environment = emit_command(prefix, plan, output)
    return subprocess.run(
        command,
        cwd=plan,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def compile_emitted_model(
    prefix: Path, output: Path
) -> subprocess.CompletedProcess[str]:
    build = output.parent / "model-consumer-build"
    configured = subprocess.run(
        [
            "cmake",
            "-S",
            str(MODEL_CONSUMER),
            "-B",
            str(build),
            f"-DCMAKE_PREFIX_PATH={prefix}",
            f"-DAGENTIC_MODEL_ROOT={output}",
            "-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=TRUE",
            "-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=TRUE",
        ],
        cwd=output.parent,
        text=True,
        capture_output=True,
        check=False,
    )
    if configured.returncode != 0:
        return configured
    built = subprocess.run(
        ["cmake", "--build", str(build)],
        cwd=output.parent,
        text=True,
        capture_output=True,
        check=False,
    )
    if built.returncode != 0:
        return built
    plugin = next(
        (
            path
            for path in build.iterdir()
            if path.name in {"libmodel-plugin.dylib", "libmodel-plugin.so"}
        ),
        None,
    )
    if plugin is None:
        return subprocess.CompletedProcess(
            args=("find-model-plugin",),
            returncode=1,
            stdout="",
            stderr="model plugin was not built",
        )
    symbols = subprocess.run(
        ["nm", "-gU", str(plugin)]
        if platform.system() == "Darwin"
        else ["nm", "-D", "--defined-only", str(plugin)],
        cwd=build,
        text=True,
        capture_output=True,
        check=False,
    )
    defined = tuple(line for line in symbols.stdout.splitlines() if line.strip())
    if (
        symbols.returncode != 0
        or len(defined) != 1
        or not defined[0].endswith("agentic_model_query_v1")
    ):
        return subprocess.CompletedProcess(
            args=symbols.args,
            returncode=1,
            stdout=symbols.stdout,
            stderr=symbols.stderr or "model plugin exported more than the query symbol",
        )
    return subprocess.run(
        [str(build / "model-consumer")],
        cwd=output.parent,
        text=True,
        capture_output=True,
        check=False,
    )


class ModelArtifactInventoryTest(unittest.TestCase):
    def test_dynamic_inventory_is_closed_and_drives_cmake(self) -> None:
        inventory = tuple(
            sorted(
                (
                    *CORE_GENERATED_FILES,
                    "include/generated/modules/Module_Child.h",
                    "include/generated/modules/queuegraph_helpers.h",
                    "include/generated/modules/queuegraph_types.h",
                    "include/generated/types/Entry.h",
                    "src/generated/helpers/queuegraph_helpers.cpp",
                    "src/generated/modules/Module_Child.cpp",
                )
            )
        )

        _validate_generated_inventory(inventory)
        cmake = _cmake_sources_for(inventory).decode()
        for path in inventory:
            if path.endswith((".cpp", ".h")):
                self.assertIn(f'  "{path}"', cmake)

    def test_dynamic_inventory_rejects_unplanned_shapes(self) -> None:
        with self.assertRaisesRegex(ValueError, "unexpected generated file path"):
            _validate_generated_inventory(
                tuple(sorted((*CORE_GENERATED_FILES, "src/generated/extra.txt")))
            )
        with self.assertRaisesRegex(ValueError, "headers and sources do not match"):
            _validate_generated_inventory(
                tuple(
                    sorted(
                        (
                            *CORE_GENERATED_FILES,
                            "include/generated/modules/Module_Child.h",
                            "include/generated/modules/queuegraph_types.h",
                        )
                    )
                )
            )

    def test_freeze_preserves_per_module_acir_diagnostics(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            native = Path(temporary) / "_native.so"
            native.write_bytes(b"native")
            sdk = SdkIdentity(
                root=Path(temporary),
                manifest_sha256=sha256(b"manifest"),
                source_revision=FIXTURE_SOURCE_REVISION,
                queue_plan_tool=Path(temporary) / "acir-queue-plan",
                queue_cxxgen_tool=Path(temporary) / "acir-queue-cxxgen",
                queue_cxxgen_sha256=sha256(b"generator"),
                queue_cxxgen_size=len(b"generator"),
                native_extension=native,
                native_sha256=sha256(b"native"),
                native_size=len(b"native"),
            )
            result = SimpleNamespace(
                diagnostics=(),
                artifacts=(
                    SimpleNamespace(path="frozen.ac.mlir", data=b"frozen"),
                    SimpleNamespace(path="modules/Child.ac.mlir", data=b"child"),
                ),
            )
            with (
                mock.patch(
                    "agentic_circuit._commands.model.run_native_compiler",
                    return_value=result,
                ),
                mock.patch(
                    "agentic_circuit._commands.model.native_extension_path",
                    return_value=native,
                ),
            ):
                frozen, modules = _freeze(b"input", sdk)

        self.assertEqual(b"frozen", frozen)
        self.assertEqual({"modules/Child.ac.mlir": b"child"}, modules)


@unittest.skipUnless(
    sys.version_info[:2] == (3, 11), "SDK model-plan profile requires Python 3.11"
)
class ModelPlanCommandTest(unittest.TestCase):
    def test_structured_plan_and_emit_publish_exact_multi_tu_inventory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "sdk"
            source = root / "source"
            plan_root = root / "plan"
            output = root / "generated"
            install_sdk(prefix)
            write_structured_source(source)

            planned = run_plan(prefix, source, plan_root)
            emitted = run_emit(prefix, plan_root, output)

            self.assertEqual(0, planned.returncode, planned.stdout)
            self.assertEqual(0, emitted.returncode, emitted.stdout)
            plan = json.loads((plan_root / "model-plan.json").read_bytes())
            outputs = plan["outputs"]
            self.assertTrue(
                any(path.startswith("include/generated/types/") for path in outputs)
            )
            self.assertTrue(
                any(path.startswith("include/generated/modules/") for path in outputs)
            )
            self.assertTrue(
                any(path.startswith("src/generated/modules/") for path in outputs)
            )
            self.assertTrue(
                any(path.startswith("src/generated/helpers/") for path in outputs)
            )
            self.assertEqual(
                ("modules/Top.ac.mlir", "modules/complete.ac.mlir"),
                tuple(
                    path.relative_to(plan_root).as_posix()
                    for path in sorted((plan_root / "modules").glob("*.ac.mlir"))
                ),
            )
            manifest = json.loads((output / "model-manifest.json").read_bytes())
            self.assertEqual(
                outputs, [item["path"] for item in manifest["generated_files"]]
            )
            cmake = (output / "model-sources.cmake").read_text()
            depfile = (output / "model.d").read_text()
            for path in outputs:
                if path.endswith((".h", ".cpp")):
                    self.assertIn(f'  "{path}"', cmake)
                self.assertIn(str(output / path), depfile)

    def test_installed_plan_is_schema_valid_and_root_independent(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "sdk"
            left = root / "left"
            right = root / "right"
            install_sdk(prefix)
            write_source(left)
            write_source(right)
            left_result = run_plan(prefix, left, root / "left-plan", hash_seed=1)
            right_result = run_plan(prefix, right, root / "right-plan", hash_seed=777)
            left_files = {
                path.relative_to(root / "left-plan").as_posix(): path.read_bytes()
                for path in (root / "left-plan").rglob("*")
                if path.is_file()
            }
            right_files = {
                path.relative_to(root / "right-plan").as_posix(): path.read_bytes()
                for path in (root / "right-plan").rglob("*")
                if path.is_file()
            }
            first_directory_inode = (root / "left-plan").stat().st_ino
            repeated = run_plan(prefix, left, root / "left-plan")
            repeated_directory_inode = (root / "left-plan").stat().st_ino
            repeated_files = {
                path.relative_to(root / "left-plan").as_posix(): path.read_bytes()
                for path in (root / "left-plan").rglob("*")
                if path.is_file()
            }
            (left / "model/top.py").write_text(
                "raise RuntimeError('must not import')\n"
            )
            (right / "model/top.py").write_text(
                "raise RuntimeError('must not import')\n"
            )
            left_emit = run_emit(prefix, root / "left-plan", root / "left-generated")
            right_emit = run_emit(prefix, root / "right-plan", root / "right-generated")
            emitted_mtimes = {
                path.relative_to(
                    root / "left-generated"
                ).as_posix(): path.stat().st_mtime_ns
                for path in (root / "left-generated").rglob("*")
                if path.is_file()
            }
            time.sleep(0.01)
            repeated_emit = run_emit(
                prefix, root / "left-plan", root / "left-generated"
            )
            repeated_emit_mtimes = {
                path.relative_to(
                    root / "left-generated"
                ).as_posix(): path.stat().st_mtime_ns
                for path in (root / "left-generated").rglob("*")
                if path.is_file()
            }
            left_generated = {
                path.relative_to(root / "left-generated").as_posix(): path.read_bytes()
                for path in (root / "left-generated").rglob("*")
                if path.is_file()
            }
            right_generated = {
                path.relative_to(root / "right-generated").as_posix(): path.read_bytes()
                for path in (root / "right-generated").rglob("*")
                if path.is_file()
            }
            runtime = compile_emitted_model(prefix, root / "left-generated")

        self.assertEqual(0, left_result.returncode, left_result.stdout)
        self.assertEqual(0, right_result.returncode, right_result.stdout)
        self.assertEqual(0, repeated.returncode, repeated.stdout)
        self.assertEqual(0, left_emit.returncode, left_emit.stdout)
        self.assertEqual(0, right_emit.returncode, right_emit.stdout)
        self.assertEqual(0, repeated_emit.returncode, repeated_emit.stdout)
        self.assertEqual(emitted_mtimes, repeated_emit_mtimes)
        self.assertEqual(0, runtime.returncode, runtime.stderr)
        self.assertNotEqual(first_directory_inode, repeated_directory_inode)
        self.assertEqual(left_files, repeated_files)
        self.assertEqual(tuple(sorted(left_files)), tuple(sorted(right_files)))
        self.assertTrue(set(PLAN_CORE_FILES).issubset(left_files))
        for name in set(left_files) - {"model.d"}:
            self.assertEqual(left_files[name], right_files[name], name)
        self.assertNotEqual(left_files["model.d"], right_files["model.d"])
        plan = json.loads(left_files["model-plan.json"])
        self.assertEqual(
            set(json.loads(MODEL_PLAN_SCHEMA.read_text())["required"]), set(plan)
        )
        self.assertEqual(
            ("config", "import", "contract", "entry"),
            tuple(item["role"] for item in plan["inputs"]),
        )
        self.assertEqual(sorted(plan["outputs"]), plan["outputs"])
        self.assertTrue(set(CORE_GENERATED_FILES).issubset(plan["outputs"]))
        self.assertEqual(
            plan["specialization"],
            json.loads(left_files["queuegraph.json"])["specialization"],
        )
        self.assertNotIn(str(left), left_files["model-plan.json"].decode())
        self.assertNotIn(str(prefix), left_files["model-plan.json"].decode())
        for name in set(left_files) - {"model.d"}:
            self.assertNotIn(str(left).encode(), left_files[name], name)
            self.assertNotIn(str(prefix).encode(), left_files[name], name)
        expected_emit_files = tuple(
            sorted(
                (
                    *plan["outputs"],
                    "model-manifest.json",
                    "model-sources.cmake",
                    "model.d",
                )
            )
        )
        self.assertEqual(expected_emit_files, tuple(sorted(left_generated)))
        self.assertEqual(expected_emit_files, tuple(sorted(right_generated)))
        for name in set(expected_emit_files) - {"model.d"}:
            self.assertEqual(left_generated[name], right_generated[name], name)
        manifest = json.loads(left_generated["model-manifest.json"])
        self.assertEqual(
            set(json.loads(MODEL_MANIFEST_SCHEMA.read_text())["required"]),
            set(manifest),
        )
        self.assertEqual(
            [item["path"] for item in manifest["generated_files"]],
            list(plan["outputs"]),
        )
        cmake_sources = left_generated["model-sources.cmake"].decode()
        for path in plan["outputs"]:
            if path.endswith((".cpp", ".h")):
                self.assertIn(f'  "{path}"', cmake_sources)
        depfile = left_generated["model.d"].decode()
        for path in plan["outputs"]:
            self.assertIn(str(root / "left-generated" / path), depfile)
        source_map_bytes = left_generated["share/generated/source-map.json"]
        source_map = json.loads(source_map_bytes)
        Draft202012Validator(json.loads(SOURCE_MAP_SCHEMA.read_text())).validate(
            source_map
        )
        self.assertEqual("agentic-circuit-source-map", source_map["schema"])
        self.assertIsInstance(source_map["module_specializations"], list)
        self.assertIsInstance(source_map["state_owners"], list)
        self.assertEqual(
            sha256(source_map_bytes),
            manifest["source_map"]["sha256"],
        )
        cost_report_bytes = left_generated["share/generated/cost-report.json"]
        cost_report = json.loads(cost_report_bytes)
        Draft202012Validator(json.loads(EMITTED_COST_SCHEMA.read_text())).validate(
            cost_report
        )
        self.assertEqual("agentic-circuit-emitted-cost", cost_report["schema"])
        self.assertEqual(
            plan["specialization"], cost_report["identity"]["specialization"]
        )
        self.assertEqual(
            sha256(left_files["queuegraph.json"]),
            cost_report["identity"]["queuegraph_sha256"],
        )
        self.assertEqual(
            sha256(source_map_bytes),
            cost_report["identity"]["source_map_sha256"],
        )
        self.assertEqual(
            sha256(cost_report_bytes),
            manifest["cost_report"]["sha256"],
        )

    def test_emit_rejects_tamper_and_cleans_only_manifest_owned_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "sdk"
            source = root / "source"
            plan = root / "plan"
            output = root / "generated"
            install_sdk(prefix)
            write_source(source)
            planned = run_plan(prefix, source, plan)
            self.assertEqual(0, planned.returncode, planned.stdout)
            emitted = run_emit(prefix, plan, output)
            self.assertEqual(0, emitted.returncode, emitted.stdout)

            manifest_path = output / "model-manifest.json"
            manifest = json.loads(manifest_path.read_bytes())
            obsolete = output / "src/generated/obsolete.cpp"
            obsolete.write_text("obsolete\n")
            manifest["generated_files"].append(
                {"path": "src/generated/obsolete.cpp", "sha256": sha256(b"obsolete\n")}
            )
            manifest_path.write_text(
                json.dumps(manifest, sort_keys=True, separators=(",", ":")) + "\n"
            )
            replaced = run_emit(prefix, plan, output)
            self.assertEqual(0, replaced.returncode, replaced.stdout)
            self.assertFalse(obsolete.exists())

            unowned = output / "keep.txt"
            unowned.write_text("keep\n")
            refused = run_emit(prefix, plan, output)
            self.assertEqual(2, refused.returncode, refused.stdout)
            self.assertEqual(
                "ACSDK-EMIT-OUTPUT-001", json.loads(refused.stdout)["code"]
            )
            self.assertEqual("keep\n", unowned.read_text())
            unowned.unlink()

            def output_bytes() -> dict[str, bytes]:
                return {
                    path.relative_to(output).as_posix(): path.read_bytes()
                    for path in output.rglob("*")
                    if path.is_file()
                }

            preserved = output_bytes()

            frozen = plan / "frozen.ac.mlir"
            frozen_bytes = frozen.read_bytes()
            frozen.write_bytes(frozen_bytes + b"tampered")
            rejected_hash = run_emit(prefix, plan, output)
            self.assertEqual(2, rejected_hash.returncode, rejected_hash.stdout)
            self.assertEqual(
                "ACSDK-EMIT-HASH-001", json.loads(rejected_hash.stdout)["code"]
            )
            self.assertEqual(preserved, output_bytes())
            frozen.write_bytes(frozen_bytes)

            queuegraph = plan / "queuegraph.json"
            queuegraph_bytes = queuegraph.read_bytes()
            queuegraph.write_bytes(queuegraph_bytes + b"tampered")
            rejected_queuegraph = run_emit(prefix, plan, output)
            self.assertEqual(
                2, rejected_queuegraph.returncode, rejected_queuegraph.stdout
            )
            self.assertEqual(
                "ACSDK-EMIT-HASH-001",
                json.loads(rejected_queuegraph.stdout)["code"],
            )
            self.assertEqual(preserved, output_bytes())
            queuegraph.write_bytes(queuegraph_bytes)

            cmake = plan / "model-sources.cmake"
            cmake_bytes = cmake.read_bytes()
            cmake.write_bytes(cmake_bytes + b"# tampered\n")
            rejected_cmake = run_emit(prefix, plan, output)
            self.assertEqual(2, rejected_cmake.returncode, rejected_cmake.stdout)
            self.assertEqual(
                "ACSDK-EMIT-HASH-001", json.loads(rejected_cmake.stdout)["code"]
            )
            self.assertEqual(preserved, output_bytes())
            cmake.write_bytes(cmake_bytes)

            plan_path = plan / "model-plan.json"
            plan_bytes = plan_path.read_bytes()
            plan_document = json.loads(plan_bytes)
            plan_document["sdk"]["runtime_abi"] = "2"
            plan_path.write_text(
                json.dumps(plan_document, sort_keys=True, separators=(",", ":")) + "\n"
            )
            rejected_plan = run_emit(prefix, plan, output)
            self.assertEqual(2, rejected_plan.returncode, rejected_plan.stdout)
            self.assertEqual(
                "ACSDK-EMIT-SDK-001", json.loads(rejected_plan.stdout)["code"]
            )
            self.assertEqual(preserved, output_bytes())
            plan_path.write_bytes(plan_bytes)

            cxxgen = prefix / "bin/acir-queue-cxxgen"
            cxxgen_bytes = cxxgen.read_bytes()
            cxxgen.write_bytes(cxxgen_bytes + b"tampered")
            cxxgen.chmod(0o755)
            rejected_generator = run_emit(prefix, plan, output)
            self.assertEqual(
                2, rejected_generator.returncode, rejected_generator.stdout
            )
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-003",
                json.loads(rejected_generator.stdout)["code"],
            )
            self.assertEqual(preserved, output_bytes())
            cxxgen.write_bytes(cxxgen_bytes)
            cxxgen.chmod(0o755)

            first_command, first_environment = emit_command(prefix, plan, output)
            second_command, second_environment = emit_command(prefix, plan, output)
            first = subprocess.Popen(
                first_command,
                cwd=plan,
                env=first_environment,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            second = subprocess.Popen(
                second_command,
                cwd=plan,
                env=second_environment,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            first_stdout, first_stderr = first.communicate(timeout=30)
            second_stdout, second_stderr = second.communicate(timeout=30)
            self.assertEqual(0, first.returncode, first_stderr or first_stdout)
            self.assertEqual(0, second.returncode, second_stderr or second_stdout)
            manifest = json.loads((output / "model-manifest.json").read_bytes())
            expected = tuple(
                sorted(
                    [item["path"] for item in manifest["generated_files"]]
                    + ["model-manifest.json", "model-sources.cmake", "model.d"]
                )
            )
            self.assertEqual(
                expected,
                tuple(
                    sorted(
                        path.relative_to(output).as_posix()
                        for path in output.rglob("*")
                        if path.is_file()
                    )
                ),
            )

    def test_plan_failures_publish_nothing_and_preserve_stale_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "sdk"
            source = root / "source"
            install_sdk(prefix)
            write_source(source)

            missing_sdk = subprocess.run(
                [sys.executable, str(prefix / "bin/agentic-circuit"), "model", "plan"],
                cwd=source,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(2, missing_sdk.returncode)

            outside = root / "outside.toml"
            outside.write_text('version = "1"\n\n[static]\nentries = 8\n')
            escaped = run_plan(prefix, source, root / "escaped-plan", config=outside)
            self.assertEqual(2, escaped.returncode, escaped.stdout)
            self.assertEqual(
                "ACSDK-PLAN-CONFIG-001", json.loads(escaped.stdout)["code"]
            )
            self.assertFalse((root / "escaped-plan").exists())

            stale = root / "stale-plan"
            stale.mkdir()
            (stale / "keep.txt").write_text("keep\n")
            rejected_stale = run_plan(prefix, source, stale)
            self.assertEqual(2, rejected_stale.returncode, rejected_stale.stdout)
            self.assertEqual(
                "ACSDK-PLAN-OUTPUT-001", json.loads(rejected_stale.stdout)["code"]
            )
            self.assertEqual("keep\n", (stale / "keep.txt").read_text())
            self.assertEqual(
                ("keep.txt",), tuple(path.name for path in stale.iterdir())
            )

            redirected = root / "redirected-plan"
            redirected.symlink_to(stale, target_is_directory=True)
            rejected_redirect = run_plan(prefix, source, redirected)
            self.assertEqual(2, rejected_redirect.returncode, rejected_redirect.stdout)
            self.assertEqual(
                "ACSDK-PLAN-OUTPUT-001",
                json.loads(rejected_redirect.stdout)["code"],
            )

            manifest_path = prefix / "share/pycircuit/sdk-manifest.json"
            manifest_bytes = manifest_path.read_bytes()
            manifest = json.loads(manifest_bytes)
            manifest["product_version"] = "6.0.1"
            manifest_path.write_text(json.dumps(manifest))
            mismatch = run_plan(prefix, source, root / "mismatch-plan")
            self.assertEqual(2, mismatch.returncode, mismatch.stdout)
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-001", json.loads(mismatch.stdout)["code"]
            )
            self.assertFalse((root / "mismatch-plan").exists())
            manifest_path.write_bytes(manifest_bytes)

            other_prefix = root / "other-sdk"
            shutil.copytree(prefix, other_prefix, symlinks=True, copy_function=os.link)
            wrong_root_command, wrong_root_environment = plan_command(
                other_prefix, source, root / "wrong-root-plan"
            )
            wrong_root_command[1] = str(prefix / "bin/agentic-circuit")
            wrong_root = subprocess.run(
                wrong_root_command,
                cwd=source,
                env=wrong_root_environment,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(2, wrong_root.returncode, wrong_root.stdout)
            self.assertEqual(
                "ACSDK-PLAN-ROOT-002", json.loads(wrong_root.stdout)["code"]
            )
            self.assertFalse((root / "wrong-root-plan").exists())

            malformed = json.loads(manifest_bytes)
            malformed["capabilities"] = 1
            manifest_path.write_text(json.dumps(malformed))
            invalid = run_plan(prefix, source, root / "invalid-manifest-plan")
            self.assertEqual(2, invalid.returncode, invalid.stdout)
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-001", json.loads(invalid.stdout)["code"]
            )
            manifest_path.write_bytes(manifest_bytes)

            wrong_platform = json.loads(manifest_bytes)
            wrong_platform["platform"]["id"] = "wrong-platform"
            manifest_path.write_text(json.dumps(wrong_platform))
            platform_mismatch = run_plan(
                prefix, source, root / "platform-mismatch-plan"
            )
            self.assertEqual(2, platform_mismatch.returncode, platform_mismatch.stdout)
            self.assertEqual(
                "ACSDK-PLAN-PLATFORM-001",
                json.loads(platform_mismatch.stdout)["code"],
            )
            manifest_path.write_bytes(manifest_bytes)

            wrong_abi = json.loads(manifest_bytes)
            wrong_abi["abi"]["generator_abi"] = "2"
            manifest_path.write_text(json.dumps(wrong_abi))
            abi_mismatch = run_plan(prefix, source, root / "abi-mismatch-plan")
            self.assertEqual(2, abi_mismatch.returncode, abi_mismatch.stdout)
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-001",
                json.loads(abi_mismatch.stdout)["code"],
            )
            manifest_path.write_bytes(manifest_bytes)

            launcher = prefix / "bin/agentic-circuit"
            launcher_bytes = launcher.read_bytes()
            launcher.write_bytes(launcher_bytes + b"\n# tampered\n")
            launcher.chmod(0o755)
            bad_launcher = run_plan(prefix, source, root / "bad-launcher-plan")
            self.assertEqual(2, bad_launcher.returncode, bad_launcher.stdout)
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-003", json.loads(bad_launcher.stdout)["code"]
            )
            launcher.write_bytes(launcher_bytes)
            launcher.chmod(0o755)

            native_root = (
                prefix
                / "lib"
                / f"python{sys.version_info.major}.{sys.version_info.minor}"
                / "site-packages/agentic_circuit"
            )
            native = next(
                path
                for path in native_root.glob("_native*")
                if path.is_file() and path.suffix in {".so", ".dylib"}
            )
            native_bytes = native.read_bytes()
            native.write_bytes(native_bytes + b"tampered")
            bad_native = run_plan(prefix, source, root / "bad-native-plan")
            self.assertEqual(2, bad_native.returncode, bad_native.stdout)
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-003", json.loads(bad_native.stdout)["code"]
            )
            native.write_bytes(native_bytes)

            schema = prefix / "share/pycircuit/schemas/model-plan.schema.json"
            schema_bytes = schema.read_bytes()
            schema.write_bytes(schema_bytes + b"tampered")
            bad_schema = run_plan(prefix, source, root / "bad-schema-plan")
            self.assertEqual(2, bad_schema.returncode, bad_schema.stdout)
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-003", json.loads(bad_schema.stdout)["code"]
            )
            schema.write_bytes(schema_bytes)

            tool = prefix / "bin/acir-queue-plan"
            missing_tool = tool.with_suffix(".missing")
            tool.rename(missing_tool)
            absent = run_plan(prefix, source, root / "missing-tool-plan")
            self.assertEqual(2, absent.returncode, absent.stdout)
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-002", json.loads(absent.stdout)["code"]
            )
            missing_tool.rename(tool)

            marker = root / "config-toctou.ready"
            config_race_source = root / "config-race-source"
            write_source(config_race_source, delay_marker=marker)
            command, environment = plan_command(
                prefix, config_race_source, root / "config-race-plan"
            )
            config_race = subprocess.Popen(
                command,
                cwd=config_race_source,
                env=environment,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            for _ in range(100):
                if marker.exists():
                    break
                time.sleep(0.05)
            self.assertTrue(marker.exists())
            (config_race_source / "model.toml").write_text(
                'version = "1"\n\n[static]\nentries = 9\n'
            )
            config_stdout, config_stderr = config_race.communicate(timeout=30)
            self.assertEqual(2, config_race.returncode, config_stderr)
            self.assertEqual("ACSDK-PLAN-CONFIG-002", json.loads(config_stdout)["code"])
            self.assertFalse((root / "config-race-plan").exists())

            marker = root / "source-toctou.ready"
            source_race_root = root / "source-race-source"
            write_source(source_race_root, delay_marker=marker)
            command, environment = plan_command(
                prefix, source_race_root, root / "source-race-plan"
            )
            source_race = subprocess.Popen(
                command,
                cwd=source_race_root,
                env=environment,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            for _ in range(100):
                if marker.exists():
                    break
                time.sleep(0.05)
            self.assertTrue(marker.exists())
            (source_race_root / "model/contracts.py").write_text(
                "import agentic_circuit as ac\n\n@ac.struct\nclass Entry:\n"
                "    sequence: ac.u5\n    value: ac.u16\n    done: bool\n"
            )
            source_stdout, source_stderr = source_race.communicate(timeout=30)
            self.assertEqual(2, source_race.returncode, source_stderr)
            self.assertEqual("ACSDK-PLAN-SOURCE-002", json.loads(source_stdout)["code"])
            self.assertFalse((root / "source-race-plan").exists())

            outside_source = root / "outside_contract.py"
            outside_source.write_text(
                "import agentic_circuit as ac\n@ac.struct\nclass Escaped:\n"
                "    value: ac.u8\n"
            )
            contract = source / "model/contracts.py"
            contract.unlink()
            contract.symlink_to(outside_source)
            escaped_import = run_plan(prefix, source, root / "escaped-import-plan")
            self.assertEqual(2, escaped_import.returncode, escaped_import.stdout)
            self.assertEqual(
                "ACSDK-PLAN-SOURCE-001", json.loads(escaped_import.stdout)["code"]
            )
            self.assertFalse((root / "escaped-import-plan").exists())

            tool_bytes = tool.read_bytes()
            tool.write_bytes(tool_bytes + b"tampered")
            tool.chmod(0o755)
            tampered = run_plan(prefix, source, root / "tampered-plan")
            self.assertEqual(2, tampered.returncode, tampered.stdout)
            self.assertEqual(
                "ACSDK-PLAN-MANIFEST-003", json.loads(tampered.stdout)["code"]
            )
            self.assertFalse((root / "tampered-plan").exists())


class ModelCaptureWorkerTest(unittest.TestCase):
    @unittest.skipIf(
        sys.version_info[:2] == (3, 11), "requires a non-SDK Python profile"
    )
    def test_non_python311_profile_is_rejected(self) -> None:
        from agentic_circuit._commands.model import _validate_platform
        from agentic_circuit._workspace import UserInputError

        platform_value, _ = platform_contract()
        with self.assertRaises(UserInputError) as caught:
            _validate_platform(platform_value)

        self.assertEqual("ACSDK-PLAN-PLATFORM-001", caught.exception.diagnostic.code)

    def test_jit_capture_timeout_is_bounded(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            entry = root / "hanging.py"
            entry.write_text("while True:\n    pass\n")
            result = run_capture_worker(
                CaptureWorkerRequest(
                    python=sys.executable,
                    workspace=root,
                    entry=entry,
                    system="top",
                    static_arguments=(),
                    component_roots=(),
                    private_output=root / "capture",
                    timeout=0.1,
                    jit_source_closure=True,
                    module_name="hanging",
                )
            )

        self.assertEqual(1, len(result.diagnostics))
        self.assertEqual("ACPY-CAPTURE-002", result.diagnostics[0].code)


if __name__ == "__main__":
    unittest.main()
