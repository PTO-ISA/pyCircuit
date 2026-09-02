import hashlib
import importlib.util
import json
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CONTRACT_EPOCH = "0.4"
LLVM_LOCK = {
    "release": "22.1.8",
    "upstream_commit": "ca7933e47d3a3451d81e72ac174dcb5aa28b59d1",
    "source_url": (
        "https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.8/"
        "llvm-project-22.1.8.src.tar.xz"
    ),
    "source_sha256": "922f1817a0df7b1489272d18134ee0087a8b068828f87ac63b9861b1a9965888",
    "local_prefix": "/opt/homebrew/opt/llvm",
    "supported_host_triples": ["arm64-apple-darwin", "x86_64-linux-gnu"],
    "package_version_policy": "exact",
}
GOVERNANCE_FILES = {
    "LICENSE",
    "CONTRIBUTING.md",
    "CODE_OF_CONDUCT.md",
    "SECURITY.md",
    "SUPPORT.md",
    "CHANGELOG.md",
}


def load_contract_checker():
    path = ROOT / "scripts/check-contracts.py"
    spec = importlib.util.spec_from_file_location("contract_checker", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def initialize_markdown_fixture(files):
    temporary_directory = tempfile.TemporaryDirectory()
    root = Path(temporary_directory.name)
    subprocess.run(["git", "init", "-q"], cwd=root, check=True)
    for relative_path, content in files.items():
        path = root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
    subprocess.run(["git", "add", "."], cwd=root, check=True)
    return temporary_directory, root


def assert_exact_ci_cache_commands(test_case, workflow):
    fingerprint_step = re.search(
        r"      - name: Compute LLVM build fingerprint.*?(?=\n      - name:)",
        workflow,
        re.DOTALL,
    ).group()
    verification_step = re.search(
        r"      - name: Verify exact LLVM build outputs.*?(?=\n      - name:)",
        workflow,
        re.DOTALL,
    ).group()

    for command in (
        "pwd -P",
        "command -v c++",
        "c++ --version",
        "cmake --version",
        "ninja --version",
        "uname -m",
        "ldd --version 2>&1 | head -n 1",
    ):
        test_case.assertRegex(
            fingerprint_step,
            rf"(?m)^\s+{re.escape(command)}$",
        )

    exact_verification_commands = (
        '.cache/llvm-build/bin/mlir-opt --version | grep -F "LLVM version ${LLVM_RELEASE}"',
        'grep -F "set(PACKAGE_VERSION \\"${LLVM_RELEASE}\\")" .cache/llvm-build/lib/cmake/llvm/LLVMConfigVersion.cmake',
        'grep -F "set(PACKAGE_VERSION \\"${LLVM_RELEASE}\\")" .cache/llvm-build/lib/cmake/mlir/MLIRConfigVersion.cmake',
        'grep -F "CMAKE_HOME_DIRECTORY:INTERNAL=${GITHUB_WORKSPACE}/.cache/llvm-project-${LLVM_RELEASE}.src/llvm" .cache/llvm-build/CMakeCache.txt',
        'grep -F "CMAKE_CACHEFILE_DIR:INTERNAL=${GITHUB_WORKSPACE}/.cache/llvm-build" .cache/llvm-build/CMakeCache.txt',
    )
    for command in exact_verification_commands:
        test_case.assertRegex(
            verification_step,
            rf"(?m)^\s+{re.escape(command)}$",
        )


class RepositoryContractsTest(unittest.TestCase):
    def test_release_layout_and_ndf_profile_are_closed(self):
        for script in ("check-release-layout.py", "check-ndf.py"):
            with self.subTest(script=script):
                completed = subprocess.run(
                    [sys.executable, ROOT / "scripts" / script],
                    cwd=ROOT,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, completed.returncode, completed.stderr)

    def test_high_level_block_spec_is_closed_and_provider_complete(self):
        from jsonschema import Draft202012Validator

        schema = json.loads((ROOT / "schemas/block-spec.schema.json").read_text())
        catalog = json.loads((ROOT / "schemas/blocks.json").read_text())
        Draft202012Validator(schema).validate(catalog)
        blocks = catalog["blocks"]
        self.assertEqual(
            sorted(block["name"] for block in blocks),
            [block["name"] for block in blocks],
        )
        self.assertEqual(
            [
                "barrier",
                "compute",
                "engine",
                "fork",
                "merge",
                "pipeline",
                "reorder",
                "route",
                "schedule",
                "table",
                "table_masked_write",
                "table_read",
                "table_write",
            ],
            [block["name"] for block in blocks],
        )
        for block in blocks:
            expected_operation = f'ac.{block["name"].replace("_", ".")}'
            if block["name"] == "table_masked_write":
                expected_operation = "ac.table.masked_write"
            self.assertEqual(expected_operation, block["operation"])
            self.assertTrue(block["providers"]["cpp"]["optimized"])
            if block["name"].startswith("table"):
                self.assertTrue(block["providers"]["cpp"]["available"])
                self.assertFalse(block["providers"]["verilog"]["available"])
                self.assertFalse(block["providers"]["verilog"]["optimized"])
            else:
                self.assertTrue(block["providers"]["verilog"]["optimized"])
            self.assertTrue(block["refinement_observations"])
            self.assertTrue(
                all(port["ownership"] == "borrowed_simqueue" for port in block["ports"])
            )
        lambda_blocks = [block["name"] for block in blocks if block["lambda_regions"]]
        self.assertEqual(
            ["compute", "table_masked_write", "table_read", "table_write"],
            lambda_blocks,
        )

    def test_official_opcode_catalog_is_closed_backend_complete_and_standard(self):
        from jsonschema import Draft202012Validator

        schema = json.loads(
            (ROOT / "schemas/opcode-catalog.schema.json").read_text()
        )
        catalog = json.loads((ROOT / "schemas/opcodes.json").read_text())
        Draft202012Validator(schema).validate(catalog)
        expected = {
            "ac.barrier",
            "ac.broadcast",
            "ac.credit",
            "ac.dependency",
            "ac.expect",
            "ac.feedback",
            "ac.fork",
            "ac.merge",
            "ac.memory.instance",
            "ac.memory.request",
            "ac.observe",
            "ac.reorder",
            "ac.route",
            "ac.scope",
            "ac.select",
            "ac.sink",
            "ac.slot",
            "ac.source",
            "ac.table",
            "ac.table.choose",
            "ac.table.match",
            "ac.table.masked_write",
            "ac.table.read",
            "ac.table.write",
            "ac.transform",
        }
        operations = [entry["operation"] for entry in catalog["entries"]]
        self.assertEqual(sorted(expected), operations)
        self.assertEqual(len(operations), len(set(operations)))
        forbidden = {"decode", "dispatch", "rename", "retire"}
        for entry in catalog["entries"]:
            operation_kind = entry["operation"].removeprefix("ac.").replace(".", "_")
            self.assertEqual(entry["kind"], operation_kind)
            self.assertTrue(entry["gfsim"]["available"])
            provisional = entry["operation"].startswith("ac.table") or entry[
                "operation"
            ] == "ac.slot"
            if entry["role"] == "design" and not provisional:
                self.assertTrue(entry["pyc"]["available"])
            if provisional:
                self.assertFalse(entry["pyc"]["available"])
            self.assertTrue(entry["gfsim"]["realization"])
            self.assertTrue(entry["pyc"]["realization"])
            self.assertTrue(entry["refinement_observations"])
            self.assertNotIn(entry["kind"], forbidden)
            maximum_inputs = entry["inputs"]["max"]
            maximum_outputs = entry["outputs"]["max"]
            if maximum_inputs is not None:
                self.assertGreaterEqual(maximum_inputs, entry["inputs"]["min"])
            if maximum_outputs is not None:
                self.assertGreaterEqual(maximum_outputs, entry["outputs"]["min"])
        roles = {entry["operation"]: entry["role"] for entry in catalog["entries"]}
        self.assertEqual("observation", roles["ac.observe"])
        self.assertEqual("verification", roles["ac.expect"])
        self.assertTrue(
            all(
                role == "design"
                for operation, role in roles.items()
                if operation not in {"ac.observe", "ac.expect"}
            )
        )

    def test_diagnostic_explanation_catalog_is_closed_and_versioned(self):
        path = ROOT / "resources/diagnostics.json"
        self.assertTrue(path.is_file())
        document = json.loads(path.read_text())
        self.assertEqual(
            {"schema", "version", "contract_epoch", "entries"}, set(document)
        )
        self.assertEqual("agentic-circuit-diagnostic-catalog", document["schema"])
        self.assertEqual("0.1", document["version"])
        self.assertEqual(CONTRACT_EPOCH, document["contract_epoch"])
        codes = []
        for entry in document["entries"]:
            self.assertEqual(
                {"code", "title", "rule", "causes", "examples", "repairs"},
                set(entry),
            )
            self.assertRegex(
                entry["code"],
                r"^AC(PY|ELAB|IR-[A-Z]+|LOWER|BUILD|TRACE|RUN)-[A-Z0-9-]+$",
            )
            for key in ("causes", "examples", "repairs"):
                self.assertTrue(entry[key])
                self.assertTrue(
                    all(isinstance(item, str) and item for item in entry[key])
                )
            codes.append(entry["code"])
        self.assertEqual(sorted(codes), codes)
        self.assertEqual(len(codes), len(set(codes)))

    def test_minimal_acpy_fixture_matches_the_public_schema(self):
        self.assertIsNotNone(importlib.util.find_spec("jsonschema"))
        from jsonschema.validators import Draft202012Validator

        schema = json.loads((ROOT / "schemas/acpy.schema.json").read_text())
        fixture = json.loads(
            (
                ROOT
                / "tests/python_frontend/fixtures/acpy/minimal.acpy.json"
            ).read_text()
        )

        Draft202012Validator(schema).validate(fixture)

    def test_ci_runs_address_and_undefined_sanitizer_presets(self):
        workflow = (ROOT / ".github/workflows/ci.yml").read_text()
        sanitizer_job = re.search(r"(?ms)^  sanitizers:\n.*\Z", workflow)
        self.assertIsNotNone(sanitizer_job, "CI lacks sanitizer job legs")
        job = sanitizer_job.group()
        self.assertIn("needs: contracts-and-configure", job)
        self.assertIn('preset: ["asan-llvm22", "ubsan-llvm22"]', job)
        self.assertIn("fail-on-cache-miss: true", job)
        self.assertIn('cmake --preset "${{ matrix.preset }}"', job)
        self.assertIn('cmake --build --preset "${{ matrix.preset }}"', job)
        self.assertIn("--target GfsimTests CodeGenTests", job)
        self.assertIn('ctest --test-dir "build/${{ matrix.preset }}"', job)
        self.assertIn("--output-on-failure -R '^(GfsimTests|CodeGenTests)$'", job)
        self.assertIn("ASAN_OPTIONS", job)
        self.assertIn("UBSAN_OPTIONS", job)
        self.assertIn("halt_on_error=1", job)

    def test_ci_runs_frontend_on_every_supported_python_minor(self):
        workflow = (ROOT / ".github/workflows/ci.yml").read_text()
        frontend_job = re.search(
            r"(?ms)^  python-frontend:\n.*?(?=^  [a-z][a-z0-9-]*:\n|\Z)",
            workflow,
        )
        self.assertIsNotNone(frontend_job, "CI lacks the Python frontend matrix")
        job = frontend_job.group()
        self.assertIn('python-version: ["3.11", "3.12", "3.13"]', job)
        self.assertIn("python-version: ${{ matrix.python-version }}", job)
        self.assertIn("python -m unittest discover -s tests/python_frontend -v", job)
        self.assertIn(
            "PYTHONHASHSEED=1 python -m unittest "
            "tests.python_frontend.test_determinism -v",
            job,
        )
        self.assertIn(
            "PYTHONHASHSEED=99 python -m unittest "
            "tests.python_frontend.test_determinism -v",
            job,
        )

    def test_ci_caches_verified_llvm_sources_and_exact_build_outputs(self):
        workflow = (ROOT / ".github/workflows/ci.yml").read_text()
        assert_exact_ci_cache_commands(self, workflow)
        source_cache_step = re.search(
            r"      - name: Cache content-addressed LLVM source archive.*?(?=\n      - name:)",
            workflow,
            re.DOTALL,
        ).group()
        build_cache_restore_step = re.search(
            r"      - name: Restore exact LLVM build outputs.*?(?=\n      - name:)",
            workflow,
            re.DOTALL,
        ).group()
        build_cache_save_step = re.search(
            r"      - name: Save exact LLVM build outputs.*?(?=\n      - name:)",
            workflow,
            re.DOTALL,
        ).group()
        fingerprint_step = re.search(
            r"      - name: Compute LLVM build fingerprint.*?(?=\n      - name:)",
            workflow,
            re.DOTALL,
        ).group()
        verification_step = re.search(
            r"      - name: Fetch and verify locked LLVM source.*?(?=\n      - name:)",
            workflow,
            re.DOTALL,
        ).group()
        build_step = re.search(
            r"      - name: Build locked MLIR package.*?(?=\n      - name:)",
            workflow,
            re.DOTALL,
        ).group()

        self.assertRegex(
            source_cache_step,
            r"(?m)^\s+path: \.cache/llvm-project-22\.1\.8\.src\.tar\.xz$",
        )
        self.assertEqual(1, source_cache_step.count(".cache/"), source_cache_step)
        self.assertNotRegex(verification_step, r"(?m)^\s+if:")
        self.assertIn("sha256sum --check --strict", verification_step)
        expected_build_cache_key = (
            "key: llvm-build-${{ runner.os }}-${{ runner.arch }}-"
            "${{ env.LLVM_SOURCE_SHA256 }}-"
            "${{ hashFiles('toolchains/llvm.lock.json', "
            "'scripts/ci-build-llvm.sh') }}-"
            "${{ steps.llvm-build-fingerprint.outputs.value }}"
        )
        self.assertIn(
            "uses: actions/cache/restore@"
            "5a3ec84eff668545956fd18022155c47e93e2684",
            build_cache_restore_step,
        )
        self.assertIn("path: .cache/llvm-build", build_cache_restore_step)
        self.assertIn(expected_build_cache_key, build_cache_restore_step)
        self.assertIn(
            "uses: actions/cache/save@"
            "5a3ec84eff668545956fd18022155c47e93e2684",
            build_cache_save_step,
        )
        self.assertIn("path: .cache/llvm-build", build_cache_save_step)
        self.assertIn(expected_build_cache_key, build_cache_save_step)
        # The hour-long LLVM build must be cached even when a later test step
        # fails, but a partially built tree must never be cached.
        self.assertIn("if: always()", build_cache_save_step)
        self.assertIn(
            "steps.llvm-build-cache.outputs.cache-hit != 'true'",
            build_cache_save_step,
        )
        self.assertIn(
            "steps.llvm-build.conclusion == 'success'",
            build_cache_save_step,
        )
        for fingerprint_input in (
            "pwd -P",
            "command -v c++",
            "c++ --version",
            "cmake --version",
            "ninja --version",
            "ldd --version",
        ):
            self.assertIn(fingerprint_input, fingerprint_step)
        self.assertIn("steps.llvm-build-cache.outputs.cache-hit != 'true'", build_step)

        native_dependencies = re.search(
            r"      - name: Install native test dependencies.*?(?=\n      - name:)",
            workflow,
            re.DOTALL,
        ).group()
        self.assertRegex(
            native_dependencies,
            r"sudo apt-get install --yes --no-install-recommends libgtest-dev",
        )

        build_verification = re.search(
            r"      - name: Verify exact LLVM build outputs.*?(?=\n      - name:)",
            workflow,
            re.DOTALL,
        ).group()
        self.assertNotRegex(build_verification, r"(?m)^\s+if:")
        self.assertIn("test -x .cache/llvm-build/bin/mlir-opt", build_verification)
        self.assertIn(
            "test -f .cache/llvm-build/lib/cmake/llvm/LLVMConfigVersion.cmake",
            build_verification,
        )
        self.assertIn(
            "test -f .cache/llvm-build/lib/cmake/mlir/MLIRConfigVersion.cmake",
            build_verification,
        )
        self.assertIn(
            'mlir-opt --version | grep -F "LLVM version ${LLVM_RELEASE}"',
            build_verification,
        )
        self.assertEqual(
            2,
            build_verification.count(
                'grep -F "set(PACKAGE_VERSION \\"${LLVM_RELEASE}\\")"'
            ),
            build_verification,
        )
        self.assertIn("CMAKE_HOME_DIRECTORY:INTERNAL=${GITHUB_WORKSPACE}", build_verification)
        self.assertIn("CMAKE_CACHEFILE_DIR:INTERNAL=${GITHUB_WORKSPACE}", build_verification)

    def test_ci_cache_contract_rejects_inert_or_misdirected_checks(self):
        workflow = (ROOT / ".github/workflows/ci.yml").read_text()
        mutations = (
            ("            c++ --version", "            echo 'c++ --version'"),
            (
                'grep -F "set(PACKAGE_VERSION \\"${LLVM_RELEASE}\\")" .cache/llvm-build/lib/cmake/mlir/MLIRConfigVersion.cmake',
                'grep -F "set(PACKAGE_VERSION \\"${LLVM_RELEASE}\\")" .cache/llvm-build/lib/cmake/llvm/LLVMConfigVersion.cmake',
            ),
            (
                "CMAKE_HOME_DIRECTORY:INTERNAL=${GITHUB_WORKSPACE}/.cache/llvm-project-${LLVM_RELEASE}.src/llvm",
                "CMAKE_HOME_DIRECTORY:INTERNAL=${GITHUB_WORKSPACE}/wrong-source",
            ),
            (
                "CMAKE_CACHEFILE_DIR:INTERNAL=${GITHUB_WORKSPACE}/.cache/llvm-build",
                "CMAKE_CACHEFILE_DIR:INTERNAL=${GITHUB_WORKSPACE}/wrong-build",
            ),
        )
        for original, replacement in mutations:
            with self.subTest(original=original):
                self.assertIn(original, workflow)
                mutated = workflow.replace(original, replacement, 1)
                with self.assertRaises(AssertionError):
                    assert_exact_ci_cache_commands(self, mutated)

    def test_cmake_package_versions_require_exact_match(self):
        cmake = (ROOT / "CMakeLists.txt").read_text()
        package_version = re.search(
            r"write_basic_package_version_file\(.*?\n\)", cmake, re.DOTALL
        ).group()
        self.assertIn("COMPATIBILITY ExactVersion", package_version)

    def test_governance_files_are_present_and_nonempty(self):
        missing = sorted(
            path for path in GOVERNANCE_FILES if not (ROOT / path).is_file()
        )
        empty = sorted(
            path
            for path in GOVERNANCE_FILES
            if (ROOT / path).is_file() and not (ROOT / path).read_text().strip()
        )
        self.assertEqual([], missing, f"missing governance files: {missing}")
        self.assertEqual([], empty, f"empty governance files: {empty}")

    def test_all_machine_contracts_use_the_exact_epoch(self):
        schema_epochs = {}
        for path in sorted((ROOT / "schemas").glob("*.schema.json")):
            document = json.loads(path.read_text())
            properties = document.get("properties", {})
            epoch = properties.get("contract_epoch", {}).get("const")
            schema_epochs[path.name] = epoch

        pyproject = (ROOT / "pyproject.toml").read_text()
        declared_epoch = re.search(
            r'^contract-epoch\s*=\s*"([^"]+)"\s*$', pyproject, re.MULTILINE
        )

        self.assertEqual(12, len(schema_epochs))
        self.assertEqual(
            {CONTRACT_EPOCH}, set(schema_epochs.values()), schema_epochs
        )
        self.assertIsNotNone(declared_epoch, "pyproject.toml lacks contract-epoch")
        self.assertEqual(CONTRACT_EPOCH, declared_epoch.group(1))

    def test_all_json_schemas_compile_as_draft_2020_12(self):
        self.assertIsNotNone(
            importlib.util.find_spec("jsonschema"),
            "install the locked development requirements before running contracts",
        )
        from jsonschema.validators import Draft202012Validator

        checked = []
        for path in sorted((ROOT / "schemas").glob("*.schema.json")):
            document = json.loads(path.read_text())
            self.assertEqual(
                "https://json-schema.org/draft/2020-12/schema",
                document.get("$schema"),
                path.name,
            )
            Draft202012Validator.check_schema(document)
            checked.append(path.name)
        self.assertEqual(12, len(checked), checked)

    def test_trace_source_decoder_uses_the_runtime_decoder_concept(self):
        record = json.loads((ROOT / "schemas/stdlib/TraceSource.json").read_text())
        parameters = {
            parameter["name"]: parameter for parameter in record["static_parameters"]
        }

        self.assertEqual(
            "gfsim::TraceDecoder<Decoder,Transaction>",
            parameters["Decoder"]["constraint"],
        )

    def test_process_state_plan_schema_is_closed_and_accepts_exact_baseline(self):
        self.assertIsNotNone(importlib.util.find_spec("jsonschema"))
        from jsonschema import ValidationError
        from jsonschema.validators import Draft202012Validator

        schema = json.loads(
            (ROOT / "schemas/acir-process-state-plan.schema.json").read_text()
        )
        validator = Draft202012Validator(schema)
        occurrence = {
            "call_sites": [],
            "iteration_vector": [],
            "kind": "original",
            "operation_path": "@Top::@workload/r0/b0/o0",
        }
        descriptor = {
            "cpp": "acir::generated::impl_wake_next_delta_043ae4e869cdd2b9059e1696f276b6844179f19aa6a52872ad0ac2d273a4c550",
            "effect": "stateful",
            "fingerprint": "sha256:043ae4e869cdd2b9059e1696f276b6844179f19aa6a52872ad0ac2d273a4c550",
            "inputs": [],
            "kind": "implementation",
            "ordinal": 0,
            "payload": {"wake_kind": "next_delta", "wake_type": "@acir_wake_next_delta"},
            "results": ["@acir_wake_next_delta"],
            "role": "wake_next_delta",
            "source_paths": [],
            "symbol": "@acir_impl_wake_next_delta_043ae4e869cdd2b9059e1696f276b6844179f19aa6a52872ad0ac2d273a4c550",
        }
        fixture = {
            "callees": [descriptor],
            "contract_epoch": "0.4",
            "processes": [{
                "blocks": [{"actions": [], "cost": 2, "edge": {"kind": "suspend", "transition": 0}, "frames": [], "loads": [], "ordinal": 0, "path": "@Top::@workload/plan/pc/entry/b00000000", "pc": 0}],
                "captures": [], "definition_key": "@Top::@workload", "entry_pc": 0,
                "fairness_work": 2, "live_slots": [], "pc_bit_width": 1,
                "pcs": [{"blocks": [0], "entry_path": "@Top::@workload/plan/pc/entry/b00000000", "name": "entry", "ordinal": 0}],
                "transitions": [{"iteration_vector": [], "loads": [], "ordinal": 0, "source_pc": 0, "stores": [], "target_pc": 0, "wake": 0}],
                "wakes": [{"callee": 0, "iteration_vector": [], "kind": "next_delta", "occurrence": occurrence, "operation_path": "@Top::@workload/r0/b0/o0", "ordinal": 0, "sources": [], "target": "", "type_key": "@acir_wake_next_delta"}],
            }],
            "schema": "acir-process-state-plan-0.1",
            "value_types": [],
        }
        validator.validate(fixture)

        def validate_definition(name, value):
            Draft202012Validator({
                "$schema": schema["$schema"],
                "$ref": f"#/$defs/{name}",
                "$defs": schema["$defs"],
            }).validate(value)

        loop_occurrence = {"anchor": occurrence, "kind": "synthetic", "phase": "condition"}
        wrapper_occurrence = {"anchor": occurrence, "direction": "wrap", "kind": "synthetic", "slot": 0, "transition": 0}
        constant_occurrence = {"anchor": occurrence, "constant": 7, "kind": "synthetic"}
        for arm in (occurrence, loop_occurrence, wrapper_occurrence, constant_occurrence):
            validate_definition("occurrence", arm)

        coordinate = {"index": 0, "kind": "result", "owner_path": "@Top::@workload/r0/b0/o0"}
        planned_arms = (
            {"coordinate": coordinate, "kind": "original", "occurrence": occurrence, "path": "@Top::@workload/r0/b0/o0/r0"},
            {"capture": 0, "kind": "capture"},
            {"kind": "live_slot", "slot": 0},
            {"coordinate": coordinate, "kind": "synthetic", "occurrence": loop_occurrence},
            {"kind": "constant", "value": "0"},
        )
        for arm in planned_arms:
            validate_definition("planned_value", {**arm, "type": "i32"})

        binding = {"from": {**planned_arms[-1], "type": "i32"}, "to": {**planned_arms[-1], "type": "i32"}}
        edge_arms = (
            {"condition": {**planned_arms[-1], "type": "i1"}, "false_bindings": [], "false_block": 1, "kind": "branch", "true_bindings": [binding], "true_block": 0},
            {"bindings": [binding], "kind": "local_continue", "target_block": 0},
            {"kind": "suspend", "transition": 0},
            {"kind": "terminate", "status": "success"},
        )
        for arm in edge_arms:
            validate_definition("edge", arm)
        for kind, phases in (("entry", ("entry",)), ("scf.if", ("then", "else", "merge")), ("scf.for", ("header", "body", "exit")), ("scf.while", ("before", "after", "exit"))):
            for phase in phases:
                validate_definition("frame", {"bindings": [], "kind": kind, "operation_path": "@Top::@workload/r0/b0/o0", "phase": phase})

        action_base = {"cost": 1, "iteration_vector": [], "kind": "original", "occurrence": occurrence, "operands": [], "ordinal": 0, "result_types": [], "results": []}
        for emission in ("inline", "invoke", "wrap", "unwrap"):
            validate_definition("action", {**action_base, "callee": 0, "emission": emission})
        validate_definition("action", {**action_base, "emission": "copy_scalar", "scalar_op": {"attributes": [], "name": "arith.constant", "properties": "{}"}})
        validate_definition("action", {**action_base, "cost": 0, "emission": "forward_only"})
        validate_definition("live_slot", {"member_values": [], "name": "live00000000", "ordinal": 0, "storage_type": 0, "type": "i32"})
        validate_definition("live_slot", {"member_values": [], "name": "live00000000", "ordinal": 0, "storage_type": 0, "type": "i32", "unwrap_callee": 1, "wrap_callee": 0})

        payloads = {
            "record_create": {"fields": [{"name": "x", "type_key": "mlir:i32"}], "record_type": "mlir:!ac.record"},
            "record_get": {"field": "x", "record": "mlir:!ac.record", "result": "mlir:i32"},
            "record_with": {"field": "x", "record": "mlir:!ac.record", "value": "mlir:i32"},
            "packet_serialize": {"bytes": 4, "packet": "@packet", "packet_type": "mlir:!ac.packet"},
            "packet_deserialize": {"bytes": 4, "packet": "@packet", "packet_type": "mlir:!ac.packet"},
            "trace_decode": {"entry": "mlir:i32", "result": "mlir:i64", "source": "trace"},
            "queue_try_send": {"element": "mlir:i32", "queue": "@queue"},
            "queue_try_recv": {"element": "mlir:i32", "queue": "@queue"},
            "event_schedule": {"delay": "mlir:i64", "target": "@event", "value": "mlir:i32"},
            "trace_open": {"source": "trace"}, "trace_next": {"entry": "mlir:i32", "source": "trace"},
            "trace_eof": {"source": "trace"}, "trace_position": {"source": "trace"},
            "contract_require": {"message": "required"}, "contract_ensure": {"message": "ensured"}, "contract_assert": {"message": "asserted"},
            "probe": {"kind": "counter", "result": "mlir:i64", "target": "@probe"},
            "stat_add": {"stat": "@stat", "value_type": "mlir:i64"},
            "wake_condition": {"wake_kind": "condition", "wake_type": "@acir_wake_condition"},
            "wake_resource": {"wake_kind": "resource", "wake_type": "@acir_wake_resource"},
            "wake_event_queue": {"wake_kind": "event_queue", "wake_type": "@acir_wake_event_queue"},
            "wake_next_delta": {"wake_kind": "next_delta", "wake_type": "@acir_wake_next_delta"},
            "scalar_wrap": {"direction": "wrap", "scalar": "mlir:i32", "value_type": "storage:value:" + "0" * 64},
            "scalar_unwrap": {"direction": "unwrap", "scalar": "mlir:i32", "value_type": "storage:value:" + "0" * 64},
        }
        pure_roles = {"record_create", "record_get", "record_with", "packet_serialize", "packet_deserialize", "trace_decode", "scalar_wrap", "scalar_unwrap"}
        for ordinal, (role, payload) in enumerate(payloads.items()):
            candidate = {**descriptor, "ordinal": ordinal, "role": role, "payload": payload, "effect": "pure" if role in pure_roles else "stateful"}
            if role == "wake_next_delta":
                candidate.update(inputs=[], results=["@acir_wake_next_delta"])
            validate_definition("callee", candidate)

        value_identity = "0" * 64
        value_base = {"acir_type": "i32", "cpp": "acir::generated::value_" + value_identity, "fingerprint": "sha256:" + value_identity, "ordinal": 0, "symbol": "@acir_value_" + value_identity}
        validate_definition("value_type", {**value_base, "kind": "value", "payload": {"encoding": "i32", "members": [], "width_bits": 32}})
        validate_definition("value_type", {**value_base, "kind": "packet", "payload": {"bytes": 4, "encoding": "array<4xi8>", "members": [], "width_bits": 32}})

        mismatched_role = json.loads(json.dumps(fixture))
        mismatched_role["callees"][0]["role"] = "record_create"
        with self.assertRaises(ValidationError):
            validator.validate(mismatched_role)

        mismatched_effect = json.loads(json.dumps(fixture))
        mismatched_effect["callees"][0].update({
            "role": "record_create",
            "payload": {"fields": [], "record_type": "mlir:i32"},
            "results": ["mlir:i32"],
        })
        with self.assertRaises(ValidationError):
            validator.validate(mismatched_effect)

        unpaired_wrapper = json.loads(json.dumps(fixture))
        unpaired_wrapper["processes"][0]["live_slots"] = [{
            "member_values": [],
            "name": "live00000000",
            "ordinal": 0,
            "storage_type": 0,
            "type": "i32",
            "wrap_callee": 0,
        }]
        with self.assertRaises(ValidationError):
            validator.validate(unpaired_wrapper)

        missing_scalar_op = json.loads(json.dumps(fixture))
        missing_scalar_op["processes"][0]["blocks"][0]["actions"] = [{
            "cost": 1,
            "emission": "copy_scalar",
            "iteration_vector": [],
            "kind": "original",
            "occurrence": occurrence,
            "operands": [],
            "ordinal": 0,
            "result_types": [],
            "results": [],
        }]
        with self.assertRaises(ValidationError):
            validator.validate(missing_scalar_op)

        branch = schema["$defs"]["edge_branch"]
        self.assertEqual(
            {"condition", "false_bindings", "false_block", "kind", "true_bindings", "true_block"},
            set(branch["properties"]),
        )
        self.assertEqual(set(branch["properties"]), set(branch["required"]))

        object_definitions = [schema]
        object_definitions.extend(
            value for value in schema["$defs"].values()
            if isinstance(value, dict) and value.get("type") == "object"
        )
        for object_schema in object_definitions:
            self.assertIs(False, object_schema.get("additionalProperties"))

        def reject_unknown(path):
            mutated = json.loads(json.dumps(fixture))
            node = mutated
            for key in path:
                node = node[key]
            node["unknown"] = True
            with self.assertRaises(ValidationError):
                validator.validate(mutated)

        for path in (
            (), ("callees", 0), ("callees", 0, "payload"),
            ("processes", 0), ("processes", 0, "blocks", 0),
            ("processes", 0, "blocks", 0, "edge"),
            ("processes", 0, "pcs", 0),
            ("processes", 0, "transitions", 0),
            ("processes", 0, "wakes", 0),
            ("processes", 0, "wakes", 0, "occurrence"),
        ):
            reject_unknown(path)

    def test_acsim_binding_schema_is_closed_and_accepts_only_lock_records(self):
        self.assertIsNotNone(
            importlib.util.find_spec("jsonschema"),
            "install the locked development requirements before running contracts",
        )
        from jsonschema import ValidationError
        from jsonschema.validators import Draft202012Validator

        schema = json.loads(
            (ROOT / "schemas/acsim-binding.schema.json").read_text()
        )
        registry = json.loads(
            (ROOT / "test/Bindings/Inputs/leaf-fast.json").read_text()
        )
        candidate = registry["candidates"][0]
        record = candidate["record"]
        validator = Draft202012Validator(schema)
        validator.validate(record)
        unit_record = json.loads(json.dumps(record))
        unit_record["parameters"][0]["value"] = {"unit": "cycles", "value": 4}
        unit_record["construction"]["arguments"][0] = {
            "unit": "cycles",
            "value": 4,
        }
        validator.validate(unit_record)

        stateful_duplicate_name = json.loads(json.dumps(record))
        stateful_duplicate_name["effect"] = "stateful"
        stateful_duplicate_name["cpp"]["entry_points"].update(
            {"pure": "", "work": "gfsim::work", "xfer": "gfsim::xfer"}
        )
        stateful_duplicate_name["ownership"] = {
            "kind": "unique",
            "placement": "member_or_array",
        }
        stateful_duplicate_name["activation_sources"] = [
            {"kind": "ac.Clock", "name": "wake"},
            {"kind": "ac.Reset", "name": "wake"},
        ]
        # Draft 2020-12 cannot state unique-by-name. The schema accepts this
        # structurally; BindingRecord semantic validation rejects it.
        validator.validate(stateful_duplicate_name)
        identical_activation = json.loads(json.dumps(stateful_duplicate_name))
        identical_activation["activation_sources"][1] = dict(
            identical_activation["activation_sources"][0]
        )
        with self.assertRaises(ValidationError):
            validator.validate(identical_activation)

        def assert_closed_objects(fragment, path="#"):
            if isinstance(fragment, list):
                for index, value in enumerate(fragment):
                    assert_closed_objects(value, f"{path}/{index}")
                return
            if not isinstance(fragment, dict):
                return
            if fragment.get("type") == "object":
                self.assertIs(
                    False,
                    fragment.get("additionalProperties"),
                    f"open object schema at {path}",
                )
            for key, value in fragment.items():
                assert_closed_objects(value, f"{path}/{key}")

        assert_closed_objects(schema)

        mutations = []
        unknown = dict(record)
        unknown["emitter_callback"] = "emitLeaf"
        mutations.append(unknown)
        unavailable = dict(record)
        unavailable["availability"] = "unavailable"
        mutations.append(unavailable)
        wrong_epoch = dict(record)
        wrong_epoch["contract_epoch"] = "0.1"
        mutations.append(wrong_epoch)
        wrong_schema = dict(record)
        wrong_schema["binding_schema"] = "acsim-binding-0.2"
        mutations.append(wrong_schema)
        wrong_cardinality = dict(record)
        wrong_cardinality["ports"] = [dict(record["ports"][0])]
        wrong_cardinality["ports"][0]["cardinality"] = "many"
        mutations.append(wrong_cardinality)
        raw_parameter_type = json.loads(json.dumps(record))
        raw_parameter_type["parameters"][0]["cpp_type"] = "int; emit()"
        mutations.append(raw_parameter_type)
        invalid_static_key = json.loads(json.dumps(unit_record))
        invalid_static_key["parameters"][0]["value"] = {"not-valid": 4}
        mutations.append(invalid_static_key)
        for mutation in mutations:
            with self.assertRaises(ValidationError):
                validator.validate(mutation)

    def test_markdown_local_links_resolve(self):
        broken = []
        link_pattern = re.compile(r"(?<!!)\[[^\]]+\]\(([^)]+)\)")
        markdown_files = [ROOT / "README.md", *sorted((ROOT / "docs/spec").glob("*.md"))]
        for path in markdown_files:
            for target in link_pattern.findall(path.read_text()):
                destination = target.split("#", 1)[0]
                if not destination or re.match(r"^[a-z][a-z0-9+.-]*:", destination):
                    continue
                if not (path.parent / destination).resolve().exists():
                    broken.append(f"{path.relative_to(ROOT)} -> {target}")
        self.assertEqual([], broken, f"broken Markdown links: {broken}")

    def test_checker_scans_tracked_markdown_and_image_targets(self):
        temporary_directory, root = initialize_markdown_fixture(
            {
                "README.md": "# Fixture\n",
                "nested/guide.md": "# Guide\n\n![missing](missing.png)\n",
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        checker = load_contract_checker()
        checker.ROOT = root
        errors = []

        checker.check_links(errors)

        self.assertTrue(errors, "tracked Markdown image target was not checked")
        self.assertIn("nested/guide.md -> missing.png", errors[0])

    def test_checker_rejects_missing_markdown_fragments(self):
        temporary_directory, root = initialize_markdown_fixture(
            {
                "README.md": "# Fixture\n\n[section](guide.md#missing-heading)\n",
                "guide.md": "# Existing Heading\n",
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        checker = load_contract_checker()
        checker.ROOT = root
        errors = []

        checker.check_links(errors)

        self.assertTrue(errors, "missing Markdown fragment was not checked")
        self.assertIn("guide.md#missing-heading", errors[0])

    def test_checker_ignores_links_in_fenced_and_indented_code(self):
        temporary_directory, root = initialize_markdown_fixture(
            {
                "README.md": (
                    "# Fixture\n\n"
                    "```markdown\n"
                    "[fenced](missing-fenced.txt)\n"
                    "![fenced image](missing-fenced.png)\n"
                    "```\n\n"
                    "    [indented](missing-indented.txt)\n"
                ),
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        checker = load_contract_checker()
        checker.ROOT = root
        errors = []

        checker.check_links(errors)

        self.assertEqual([], errors, f"code example links were checked: {errors}")

    def test_code_example_headings_do_not_satisfy_fragments(self):
        temporary_directory, root = initialize_markdown_fixture(
            {
                "README.md": "# Fixture\n\n[section](guide.md#example-heading)\n",
                "guide.md": (
                    "# Guide\n\n"
                    "```markdown\n"
                    "# Example Heading\n"
                    "```\n"
                ),
            }
        )
        self.addCleanup(temporary_directory.cleanup)
        checker = load_contract_checker()
        checker.ROOT = root
        errors = []

        checker.check_links(errors)

        self.assertTrue(errors, "code example heading satisfied a real fragment")
        self.assertIn("guide.md#example-heading", errors[0])

    def test_repository_has_no_placeholder_markers(self):
        offenders = []
        marker = re.compile(r"\b(?:TODO|TBD|FIXME|XXX)\b")
        checked = [ROOT / "README.md"]
        checked.extend(ROOT / path for path in GOVERNANCE_FILES)
        for path in checked:
            if path.is_file() and marker.search(path.read_text()):
                offenders.append(str(path.relative_to(ROOT)))
        readme = (ROOT / "README.md").read_text()
        if "proposed Python" in readme or "No implementation contract is approved yet" in readme:
            offenders.append("README.md (stale specification-phase placeholder)")
        self.assertEqual([], offenders, f"placeholder content: {offenders}")

    def test_davincioo_reference_snapshot_is_provenance_locked(self):
        reference = ROOT / "references/davincioo-gfsim"
        source = json.loads((reference / "SOURCE.json").read_text())
        self.assertEqual(
            "agentic-circuit-reference-source@0.1", source.get("schema")
        )
        self.assertEqual(
            "https://github.com/hengliao1972/DavinciOO.git",
            source.get("repository"),
        )
        self.assertEqual(
            "a542b9cf705096288c615575be222b974b570a18",
            source.get("commit"),
        )
        self.assertEqual("model", source.get("subtree"))
        self.assertEqual("unresolved", source.get("license_status"))

        manifest = {}
        for line in (reference / "UPSTREAM_FILES.sha256").read_text().splitlines():
            digest, separator, path = line.partition("  ")
            self.assertEqual("  ", separator)
            self.assertRegex(digest, r"^[0-9a-f]{64}$")
            self.assertNotIn(path, manifest)
            manifest[path] = digest

        upstream = reference / "upstream"
        actual = {
            path.relative_to(reference).as_posix()
            for path in upstream.rglob("*")
            if path.is_file()
        }
        self.assertEqual(actual, set(manifest))
        for relative, expected in manifest.items():
            digest = hashlib.sha256((reference / relative).read_bytes()).hexdigest()
            self.assertEqual(expected, digest, relative)

    def test_llvm_lock_is_exact_and_complete(self):
        lock = json.loads((ROOT / "toolchains/llvm.lock.json").read_text())
        self.assertEqual(1, lock.get("lock_version"))
        self.assertEqual(LLVM_LOCK, lock.get("llvm"))

    def test_read_only_checker_accepts_the_repository(self):
        before = subprocess.run(
            ["git", "status", "--porcelain=v1"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        ).stdout
        result = subprocess.run(
            [sys.executable, "scripts/check-contracts.py"],
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
        self.assertEqual(before, after, "contract checker modified the repository")


if __name__ == "__main__":
    unittest.main()
