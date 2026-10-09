"""Independent C11 and ctypes acceptance for generated Model API shared libraries."""

from __future__ import annotations

import ctypes
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest
import test_cpp_source_parts as cpp
import test_final_value_declarations as declarations
import test_source_preview_workflow as source_preview

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
CONSUMER_C = ROOT / "tests/integration/model-abi/consumer.c"
_STATUS_OK = 0
_STATUS_INVALID_ARGUMENT = 1
_STATUS_ABI_MISMATCH = 2
_STATUS_INVALID_STATE = 3
_STATUS_RUNTIME_FAILURE = 4
_STEP_RUNNING = 0
_STEP_QUIESCENT = 1
_STEP_TERMINATED = 2
_STEP_FAILED = 3
_CONFIG = {
    "deadlock_window": None,
    "max_domain_cycles": {},
    "max_ticks": 3,
    "schema": "pycircuit-model-config",
    "version": "1",
}


def _build_dut(build: Path, design: str, native: Path) -> tuple[Path, Path]:
    source_preview._configure_preview(build, native=native, design=design)
    source_preview._checked(
        [
            "cmake",
            "--build",
            str(build),
            "--target",
            "preview-artifacts",
            "--parallel",
            "4",
        ]
    )
    artifacts = build / "artifacts/cpp"
    assert (artifacts / "dut.h").is_file()
    assert (artifacts / "model_api.cpp").is_file()
    model_build = build / "model-cpp"
    source_preview._checked(
        [
            "cmake",
            "-S",
            str(artifacts),
            "-B",
            str(model_build),
            "-G",
            "Ninja",
            "-DPYCIRCUIT_RUNTIME_ROOT=" + str(ROOT / "simulator/gfsim"),
        ]
    )
    source_preview._checked(
        [
            "cmake",
            "--build",
            str(model_build),
            "--target",
            "pycircuit_dut",
            "--parallel",
            "4",
        ]
    )
    patterns = (
        "libpycircuit_dut.dylib",
        "libpycircuit_dut.so",
        "pycircuit_dut.dll",
    )
    libraries = [
        model_build / name for name in patterns if (model_build / name).is_file()
    ]
    assert len(libraries) == 1, f"expected one generated shared library: {libraries}"
    return artifacts, libraries[0]


@pytest.fixture(scope="module")
def generated_duts(
    tmp_path_factory: pytest.TempPathFactory,
    request: pytest.FixtureRequest,
) -> dict[str, dict[str, Path]]:
    monkeypatch = pytest.MonkeyPatch()
    request.addfinalizer(monkeypatch.undo)
    native = source_preview._native_build()
    monkeypatch.setenv("PYCIRCUIT_NATIVE_BUILD", str(native))
    native = source_preview._require_preview_tools()
    monkeypatch.setenv(
        "PYCIRCUIT_SOURCE_COMPILER", str(native / "bin/pycircuit-source-unit")
    )
    monkeypatch.setenv("PYCIRCUIT_LINKER", str(native / "bin/pycircuit-link"))
    monkeypatch.setenv(
        "PYCIRCUIT_EMITTER",
        str(native / "bin/pycircuit-emit"),
    )
    compiler = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
    if compiler is None:
        pytest.fail("source compiler Model API acceptance requires a C compiler")
    root = tmp_path_factory.mktemp("source-model-api")
    result: dict[str, dict[str, Path]] = {}
    for design in ("design_top", "hold_top", "zero_rule_top", "failure_top"):
        artifacts, library = _build_dut(root / design, design, native)
        consumer = root / f"{design}-c-consumer"
        command = [
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I",
            str(artifacts),
            "-I",
            str(ROOT / "include"),
            str(CONSUMER_C),
            str(library),
            "-Wl,-rpath," + str(library.parent),
            "-o",
            str(consumer),
        ]
        compiled = subprocess.run(
            command, text=True, capture_output=True, check=False, timeout=120
        )
        assert (
            compiled.returncode == 0
        ), f"C consumer compile failed: {command!r}\n{compiled.stderr}"
        result[design] = {
            "artifacts": artifacts,
            "library": library,
            "consumer": consumer,
        }
    return result


@pytest.mark.parametrize(
    ("design", "mode"),
    [
        ("design_top", "design"),
        ("hold_top", "hold"),
        ("zero_rule_top", "zero"),
        ("failure_top", "failure"),
    ],
)
def test_generated_library_links_and_behaves_for_an_independent_c_consumer(
    generated_duts: dict[str, dict[str, Path]], design: str, mode: str
) -> None:
    executable = generated_duts[design]["consumer"]
    result = subprocess.run(
        [str(executable), mode], text=True, capture_output=True, check=False, timeout=30
    )
    assert result.returncode == 0, (
        f"C consumer failed for {mode} (return={result.returncode}): "
        f"{result.stdout}{result.stderr}"
    )


class _Buffer(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.POINTER(ctypes.c_uint8)),
        ("size", ctypes.c_uint64),
    ]


class _StepResult(ctypes.Structure):
    _fields_ = [
        ("struct_size", ctypes.c_uint32),
        ("state", ctypes.c_int32),
        ("epoch_time", ctypes.c_uint64),
        ("epoch_delta", ctypes.c_uint32),
        ("reserved", ctypes.c_uint32),
    ]


_Create = ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.POINTER(ctypes.c_void_p))
_Destroy = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
_Configure = ctypes.CFUNCTYPE(
    ctypes.c_int32, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint64
)
_Reset = ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_void_p)
_Step = ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_void_p, ctypes.POINTER(_StepResult))
_ReadBuffer = ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_void_p, ctypes.POINTER(_Buffer))


class _ModelApi(ctypes.Structure):
    _fields_ = [
        ("struct_size", ctypes.c_uint32),
        ("abi_version", ctypes.c_uint32),
        ("create", _Create),
        ("destroy", _Destroy),
        ("configure_json", _Configure),
        ("reset", _Reset),
        ("step", _Step),
        ("statistics_json", _ReadBuffer),
        ("last_error", _ReadBuffer),
    ]


def _api(library_path: Path) -> tuple[Any, _ModelApi]:
    library = ctypes.CDLL(str(library_path))
    query = library.pycircuit_model_query_v1
    query.argtypes = []
    query.restype = ctypes.POINTER(_ModelApi)
    pointer = query()
    assert pointer
    api = pointer.contents
    assert api.struct_size == 64 and api.abi_version == 1
    assert ctypes.sizeof(_ModelApi) == 64
    assert ctypes.sizeof(_Buffer) == 16
    assert ctypes.sizeof(_StepResult) == 24
    assert all(
        bool(getattr(api, name))
        for name in (
            "create",
            "destroy",
            "configure_json",
            "reset",
            "step",
            "statistics_json",
            "last_error",
        )
    )
    return library, api


def _buffer_bytes(api_call: Any, model: ctypes.c_void_p) -> bytes:
    buffer = _Buffer()
    assert api_call(model, ctypes.byref(buffer)) == _STATUS_OK
    return ctypes.string_at(buffer.data, buffer.size) if buffer.size else b""


def _config_bytes() -> tuple[ctypes.Array[ctypes.c_char], int]:
    raw = json.dumps(_CONFIG, separators=(",", ":")).encode("utf-8")
    return ctypes.create_string_buffer(raw), len(raw)


def test_ctypes_resolves_c_symbol_layout_lifecycle_and_borrowed_buffers(
    generated_duts: dict[str, dict[str, Path]],
) -> None:
    library, api = _api(generated_duts["design_top"]["library"])
    assert library is not None  # Retain the CDLL while its callbacks are used.
    assert api.create(None) == _STATUS_INVALID_ARGUMENT
    api.destroy(None)

    model = ctypes.c_void_p()
    assert api.create(ctypes.byref(model)) == _STATUS_OK and model.value
    assert api.step(model, None) == _STATUS_INVALID_ARGUMENT
    config, length = _config_bytes()
    config_pointer = ctypes.cast(config, ctypes.POINTER(ctypes.c_uint8))
    assert api.configure_json(model, config_pointer, length) == _STATUS_OK
    assert api.reset(model) == _STATUS_OK

    result = _StepResult(ctypes.sizeof(_StepResult))
    assert api.step(model, ctypes.byref(result)) == _STATUS_OK
    assert result.state == _STEP_RUNNING and result.epoch_time == 1
    copied_statistics = _buffer_bytes(api.statistics_json, model)
    copied_error = _buffer_bytes(api.last_error, model)
    assert copied_error == b""
    assert api.step(model, ctypes.byref(result)) == _STATUS_OK
    assert result.state == _STEP_RUNNING and result.epoch_time == 2
    assert b'"name":"cycles"' in copied_statistics
    assert b'"name":"cycles"' in _buffer_bytes(api.statistics_json, model)
    assert api.reset(model) == _STATUS_OK
    api.destroy(model)


def _build_host_fault_library(
    source_artifacts: Path, output_root: Path, fault: str
) -> Path:
    """Build a test-only copy with a deterministic host-side construction fault."""
    copied_artifacts = output_root / "artifacts/cpp"
    shutil.copytree(source_artifacts, copied_artifacts)
    system_header = copied_artifacts / "pycircuit_system.hpp"
    original = system_header.read_text(encoding="utf-8")
    if fault == "build-failure":
        pattern = re.compile(r"return\s+root_\.FreezeObjects\(nullptr,\s*\"root\"\);")
        matches = list(pattern.finditer(original))
        assert len(matches) == 1, f"expected one FinalizeBuild root freeze: {matches}"
        mutated = pattern.sub("return false;", original, count=1)
    elif fault == "constructor-exception":
        pattern = re.compile(r'(FinalSystem\(\)\s*:\s*root_\(nullptr,\s*"root"\)\s*\{)')
        matches = list(pattern.finditer(original))
        assert len(matches) == 1, f"expected one FinalSystem constructor: {matches}"
        mutated = pattern.sub(r"\1\n    throw 17;", original, count=1)
        assert "throw 17;" in mutated
    else:
        raise AssertionError(f"unknown host fault: {fault}")
    assert mutated != original
    system_header.write_text(mutated, encoding="utf-8")

    model_build = output_root / "model-cpp"
    source_preview._checked(
        [
            "cmake",
            "-S",
            str(copied_artifacts),
            "-B",
            str(model_build),
            "-G",
            "Ninja",
            "-DPYCIRCUIT_RUNTIME_ROOT=" + str(ROOT / "simulator/gfsim"),
        ]
    )
    source_preview._checked(
        [
            "cmake",
            "--build",
            str(model_build),
            "--target",
            "pycircuit_dut",
            "--parallel",
            "4",
        ]
    )
    patterns = (
        "libpycircuit_dut.dylib",
        "libpycircuit_dut.so",
        "pycircuit_dut.dll",
    )
    libraries = [
        model_build / name for name in patterns if (model_build / name).is_file()
    ]
    assert len(libraries) == 1, f"expected one generated shared library: {libraries}"
    return libraries[0]


@pytest.mark.parametrize("fault", ["build-failure", "constructor-exception"])
def test_create_contains_host_build_failure_and_constructor_exception(
    generated_duts: dict[str, dict[str, Path]], tmp_path: Path, fault: str
) -> None:
    """Faults alter only copied compiler output; they are not user circuits."""
    library = _build_host_fault_library(
        generated_duts["design_top"]["artifacts"], tmp_path / fault, fault
    )
    probe = r"""
import ctypes
import sys

class Api(ctypes.Structure):
    _fields_ = [
        ("struct_size", ctypes.c_uint32),
        ("abi_version", ctypes.c_uint32),
        ("create", ctypes.c_void_p),
        ("destroy", ctypes.c_void_p),
        ("configure_json", ctypes.c_void_p),
        ("reset", ctypes.c_void_p),
        ("step", ctypes.c_void_p),
        ("statistics_json", ctypes.c_void_p),
        ("last_error", ctypes.c_void_p),
    ]

library = ctypes.CDLL(sys.argv[1])
query = library.pycircuit_model_query_v1
query.argtypes = []
query.restype = ctypes.POINTER(Api)
api = query().contents
create = ctypes.CFUNCTYPE(
    ctypes.c_int32, ctypes.POINTER(ctypes.c_void_p)
)(api.create)
destroy = ctypes.CFUNCTYPE(None, ctypes.c_void_p)(api.destroy)
output = ctypes.c_void_p(1)
status = create(ctypes.byref(output))
assert status == 4, status
assert output.value is None, output.value
destroy(None)
"""
    completed = subprocess.run(
        [sys.executable, "-c", probe, str(library)],
        text=True,
        capture_output=True,
        check=False,
        timeout=30,
    )
    assert completed.returncode == 0, (
        f"fault containment subprocess failed ({completed.returncode}): "
        f"{completed.stdout}{completed.stderr}"
    )


def _global_api_name_declaration(kind: str) -> str:
    owner = 'ac.source_owner = {package = "", path = "__init__.py"}'
    origin = (
        'ac.origin = {site = {definition = @"pycircuit_model_query_v1", '
        "ast_path = []}, expansion = []}"
    )
    if kind == "alias":
        return (
            '    "ac.type_alias"() <{sym_name = "pycircuit_model_query_v1", '
            'target = {kind = "bool", storage = i1}}> {'
            + owner
            + ", "
            + origin
            + ', ac.declaration_role = "definition"} : () -> () '
            'loc("__init__.py":1:1)\n'
        )
    assert kind == "constant"
    return (
        '    "ac.constant"() <{sym_name = "pycircuit_model_query_v1", '
        'type = {kind = "bool"}, value = {kind = "bool", value = true}}> {'
        + owner
        + ", "
        + origin
        + ', ac.declaration_role = "definition"} : () -> () '
        'loc("__init__.py":1:1)\n'
    )


@pytest.mark.parametrize("kind", ["alias", "constant"])
def test_global_declaration_cannot_shadow_exported_model_api_symbol(
    tmp_path: Path, kind: str
) -> None:
    final, _units = declarations._build(tmp_path / "global-declaration")
    variant = tmp_path / f"global-{kind}.ac"
    variant.write_text(
        declarations._insert_unit(
            final.read_text(encoding="utf-8"),
            "__init__.py",
            _global_api_name_declaration(kind),
            package="",
        ),
        encoding="utf-8",
    )

    declarations._reject_cpp_only(
        variant, r"pycircuit_model_query_v1|reserved|collision|scope|name"
    )


def _build_empty_package_module(
    directory: Path, *, path: str, source: str, definition: str, package: str = ""
) -> Path:
    source_root = directory / "source"
    source_root.mkdir(parents=True)
    source_path = source_root / path
    source_path.parent.mkdir(parents=True, exist_ok=True)
    source_path.write_text(source, encoding="utf-8")
    unit = cpp._compile_source(
        source_path,
        source_root=source_root,
        output_dir=directory / "unit",
        package=package,
    )
    final = directory / "model.ac"
    linked = cpp._link([unit], final, top=definition, role="design")
    assert linked.returncode == 0, linked.stderr
    return final


@pytest.mark.parametrize(
    ("source_path", "function", "definition"),
    [
        (
            "pycircuit_model_query_v1.py",
            "Root",
            "pycircuit_model_query_v1.Root",
        ),
        ("__init__.py", "PycircuitModelQueryV1", "PycircuitModelQueryV1"),
    ],
)
def test_global_source_namespace_or_family_cannot_claim_model_api_name(
    tmp_path: Path, source_path: str, function: str, definition: str
) -> None:
    source = (
        "from pycircuit import module\n\n"
        "@module\n"
        f"def {function}():\n"
        "    state: bool = True\n"
    )
    final = _build_empty_package_module(
        tmp_path,
        path=source_path,
        source=source,
        definition=definition,
    )

    declarations._reject_cpp_only(
        final, r"pycircuit_model_query_v1|reserved|collision|scope|name"
    )


def test_nested_model_api_spelling_remains_a_valid_cpp_namespace(
    tmp_path: Path,
) -> None:
    source = (
        "from pycircuit import module\n\n@module\ndef Root():\n    state: bool = True\n"
    )
    final = _build_empty_package_module(
        tmp_path,
        path="pycircuit_model_query_v1.py",
        source=source,
        definition="demo.pycircuit_model_query_v1.Root",
        package="demo",
    )
    cpp_output = tmp_path / "nested.cpp"
    emitted = cpp._design_harness()
    result = subprocess.run(
        [
            emitted,
            "--design",
            str(final),
            "--target",
            "cpp",
            "--role",
            "design",
            "--output",
            str(cpp_output),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
