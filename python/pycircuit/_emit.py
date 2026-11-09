"""Publish native target artifacts from one locked, verified final snapshot."""

from __future__ import annotations

import json
import subprocess
import tempfile
from collections.abc import Mapping
from pathlib import Path

from ._driver import _DriverError, _validate_published_program
from ._generated_bundle import _relative_file_path
from ._native_verify import native_helper, verify_program_owner
from ._publication import (
    _paths_conflict,
    _paths_for,
    _publication_lock_set,
    _publication_owner_generated,
    _PublicationInput,
    _PublicationOutput,
    _publish_directory,
    _read_journal,
    _require_initialized_control,
    _validate_control,
)
from ._publication_fs import _PublicationFileSystem
from ._source_capture import _read_stable_file_bytes
from ._source_map import _validate_emitted_bundle

_VERILATOR_POLICY_PATH = Path(__file__).with_name("verilator_policy.json")
_STRUCTURAL_CPP_MAX_TU_BYTES = 1024 * 1024


def _verilator_policy() -> tuple[dict[str, object], str]:
    """Load the one versioned policy copied into every Verilog bundle."""
    try:
        text = _VERILATOR_POLICY_PATH.read_text(encoding="utf-8")
        policy = json.loads(text)
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise _DriverError("installed Verilator build policy is unavailable") from error
    expected = {
        "schema",
        "output_split_statements",
        "output_split_cfuncs",
        "output_groups",
        "max_generated_cpp_bytes",
        "max_generated_header_bytes",
        "max_text_symbol_bytes",
    }
    if (
        type(policy) is not dict
        or set(policy) != expected
        or policy.get("schema") != "pycircuit-verilator-build-policy-v1"
        or any(
            type(policy.get(name)) is not int or policy[name] <= 0
            for name in (
                "output_split_statements",
                "output_split_cfuncs",
                "max_generated_cpp_bytes",
                "max_generated_header_bytes",
                "max_text_symbol_bytes",
            )
        )
        or type(policy.get("output_groups")) is not int
        or policy["output_groups"] != 0
    ):
        raise _DriverError("installed Verilator build policy is invalid")
    return policy, json.dumps(policy, indent=2) + "\n"


def _managed_owner(path: Path, fs: _PublicationFileSystem) -> Mapping[str, object]:
    paths = _paths_for(path, fs)
    _require_initialized_control(paths, fs)
    with fs.lock(paths.lock, shared=True):
        _validate_control(paths, fs, cleanup_temporary=False)
        journal = _read_journal(paths, fs, cleanup_temporary=False)
        owner = journal["owner"] if journal else verify_program_owner(path)
    if owner["kind"] != "program":
        raise _DriverError("emit input is not a final design")
    return owner


def _cmake_quote(value: str) -> str:
    return (
        '"'
        + value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("$", "\\$")
        .replace(";", "\\;")
        + '"'
    )


def _cmake(
    sources: list[str],
    rtl_sources: list[str],
    *,
    target: str,
    standard_sources: list[str],
    root_rtl_name: str = "",
    simulation: bool = False,
    verilator_policy: Mapping[str, object] | None = None,
) -> str:
    if simulation and target == "verilog":
        return """cmake_minimum_required(VERSION 3.25)
project(PycircuitSimulation LANGUAGES C CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(PYCIRCUIT_VERILATOR verilator REQUIRED)
set(PYCIRCUIT_SIM "${CMAKE_CURRENT_BINARY_DIR}/bin/pycircuit_sim${CMAKE_EXECUTABLE_SUFFIX}")
set(PYCIRCUIT_RTL_STAGE "${CMAKE_CURRENT_BINARY_DIR}/verilator")
file(GLOB_RECURSE PYCIRCUIT_RTL CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.v" "${CMAKE_CURRENT_SOURCE_DIR}/*.sv")
file(GLOB PYCIRCUIT_PRIMITIVES CONFIGURE_DEPENDS "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}/verilog/*.v")
add_custom_command(OUTPUT "${PYCIRCUIT_SIM}"
  COMMAND "${Python3_EXECUTABLE}" "${PYCIRCUIT_CMAKE_DIR}/verify_example.py"
    --compile-system "${CMAKE_CURRENT_SOURCE_DIR}" --output "${PYCIRCUIT_SIM}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}" --verilator "${PYCIRCUIT_VERILATOR}"
    --build-dir "${PYCIRCUIT_RTL_STAGE}"
  DEPENDS ${PYCIRCUIT_RTL} ${PYCIRCUIT_PRIMITIVES}
    "${CMAKE_CURRENT_SOURCE_DIR}/generated.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/verilator_policy.json"
    "${PYCIRCUIT_CMAKE_DIR}/verify_example.py"
    "${PYCIRCUIT_CMAKE_DIR}/verilator_policy.json"
  VERBATIM)
add_custom_target(pycircuit_sim DEPENDS "${PYCIRCUIT_SIM}")
"""
    text = """cmake_minimum_required(VERSION 3.25)
project(PycircuitModules LANGUAGES C CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
add_library(pycircuit_modules STATIC
"""
    if target == "structural-cpp":
        text += "".join("  " + _cmake_quote(path) + "\n" for path in sources)
    text += """ )
target_include_directories(pycircuit_modules PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pycircuit_modules PUBLIC pycircuit::pyc6_runtime)
"""
    if target == "structural-cpp":
        # Every generated shard includes the same (potentially large) concrete
        # module declaration. Build it once per compiler configuration instead
        # of reparsing it independently in every parallel translation unit.
        text += 'target_precompile_headers(pycircuit_modules PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/design.hpp")\n'
    if target == "verilog":
        if verilator_policy is None:
            raise _DriverError("Verilog bundle has no Verilator build policy")
        split = int(verilator_policy["output_split_statements"])
        cfuncs = int(verilator_policy["output_split_cfuncs"])
        groups = int(verilator_policy["output_groups"])
        text += "find_package(Python3 REQUIRED COMPONENTS Interpreter)\n"
        text += "find_package(verilator REQUIRED)\n"
        text += 'set(PYCIRCUIT_VERILATOR_DIR "${CMAKE_CURRENT_BINARY_DIR}/verilator")\n'
        text += (
            "set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS\n"
            '  "${CMAKE_CURRENT_SOURCE_DIR}/generated.json"\n'
            '  "${CMAKE_CURRENT_SOURCE_DIR}/verilator_policy.json")\n'
        )
        text += (
            'execute_process(COMMAND "${Python3_EXECUTABLE}" '
            '"${PYCIRCUIT_CMAKE_DIR}/verify_example.py" '
            '--check-verilator-policy "${CMAKE_CURRENT_SOURCE_DIR}" '
            "RESULT_VARIABLE PYCIRCUIT_POLICY_STATUS)\n"
            "if(NOT PYCIRCUIT_POLICY_STATUS EQUAL 0)\n"
            '  message(FATAL_ERROR "generated Verilator policy validation failed")\n'
            "endif()\n"
        )
        text += "verilate(pycircuit_modules SOURCES\n"
        for path in standard_sources:
            relative = _relative_file_path(path)[0]
            if not relative.startswith("include/verilog/"):
                raise _DriverError("native standard RTL path is outside Runtime")
            # The prefix is a CMake variable, not data from the source program.
            suffix = relative.removeprefix("include/")
            quoted = _cmake_quote(suffix)[1:-1]
            text += '  "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}/' + quoted + '"\n'
        text += "".join("  " + _cmake_quote(path) + "\n" for path in rtl_sources)
        text += "  TOP_MODULE " + _cmake_quote(root_rtl_name) + "\n"
        text += (
            '  PREFIX Vpycircuit_modules\n  DIRECTORY "${PYCIRCUIT_VERILATOR_DIR}"\n'
        )
        text += (
            "  VERILATOR_ARGS --Wno-fatal"
            f" --output-split {split}"
            f" --output-split-cfuncs {cfuncs}"
            f" --output-groups {groups})\n"
        )
        text += (
            'execute_process(COMMAND "${Python3_EXECUTABLE}" '
            '"${PYCIRCUIT_CMAKE_DIR}/verify_example.py" '
            '--check-verilator-generated "${CMAKE_CURRENT_SOURCE_DIR}" '
            '--mdir "${PYCIRCUIT_VERILATOR_DIR}" '
            "RESULT_VARIABLE PYCIRCUIT_GENERATED_STATUS)\n"
            "if(NOT PYCIRCUIT_GENERATED_STATUS EQUAL 0)\n"
            '  message(FATAL_ERROR "generated Verilator C++ validation failed")\n'
            "endif()\n"
            "add_custom_command(TARGET pycircuit_modules PRE_LINK\n"
            '  COMMAND "${Python3_EXECUTABLE}" '
            '"${PYCIRCUIT_CMAKE_DIR}/verify_example.py"\n'
            '    --check-verilator-generated "${CMAKE_CURRENT_SOURCE_DIR}"\n'
            '    --mdir "${PYCIRCUIT_VERILATOR_DIR}" VERBATIM)\n'
            "add_custom_command(TARGET pycircuit_modules POST_BUILD\n"
            '  COMMAND "${Python3_EXECUTABLE}" '
            '"${PYCIRCUIT_CMAKE_DIR}/verify_example.py"\n'
            '    --check-verilator-symbols "${CMAKE_CURRENT_SOURCE_DIR}"\n'
            '    --binary "$<TARGET_FILE:pycircuit_modules>" VERBATIM)\n'
        )
    if simulation and target == "structural-cpp":
        text += (
            "add_executable(pycircuit_sim simulation_main.cpp)\n"
            "target_compile_features(pycircuit_sim PRIVATE cxx_std_20)\n"
            'set_target_properties(pycircuit_sim PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")\n'
            "target_link_libraries(pycircuit_sim PRIVATE pycircuit_modules)\n"
        )
    return text


def _target_files(payload: dict, target: str):
    """Copy structured native outputs; never parse or split generated source."""
    files: dict[str, tuple[str, str]] = {}
    groups: dict[tuple[str, str], dict] = {}
    sources: list[str] = []
    rtl_sources: list[str] = []

    def add(path, text, role):
        _relative_file_path(path)
        if path in files or type(text) is not str:
            raise _DriverError("native emission contains duplicate or invalid files")
        files[path] = (text, role)

    def group(owner):
        key = (owner["package"], owner["path"])
        if key not in groups:
            groups[key] = {"source": owner, "files": []}
        return groups[key]["files"]

    if target == "structural-cpp":
        add("design.hpp", payload["structural_cpp_header"], "header")
        for row in payload["structural_cpp_groups"]:
            add(row["path"], row["text"], "source")
            group(row["source"]).append(row["path"])
            sources.append(row["path"])
        add(
            "manifest.json",
            json.dumps(payload["manifest"], ensure_ascii=False, separators=(",", ":"))
            + "\n",
            "runtime-glue",
        )
    else:
        policy, policy_text = _verilator_policy()
        add("design_top.sv", payload["rtl_core"], "rtl")
        add("verilator_policy.json", policy_text, "runtime-glue")
        # The package with shared packed types must precede source groups.
        rtl_sources.append("design_top.sv")
        for row in payload["rtl_source_groups"]:
            add(row["path"], row["text"], "rtl")
            group(row["source"]).append(row["path"])
            rtl_sources.append(row["path"])
        if "port_availability" in payload:
            add(
                "ports.json",
                json.dumps(payload["port_availability"], separators=(",", ":")) + "\n",
                "runtime-glue",
            )
    simulation = bool(
        payload.get("simulation_main")
        if target == "structural-cpp"
        else payload.get("simulation_top")
    )
    if simulation:
        count = payload.get("source_check_count")
        if type(count) is not int or count < 0:
            raise _DriverError("system emission requires a verified source-check count")
        add(
            "simulation_verification.json",
            json.dumps(
                {
                    "entry": payload["entry"],
                    "entry_source": payload["entry_source"],
                    "source_checks": count,
                },
                separators=(",", ":"),
            )
            + "\n",
            "runtime-glue",
        )
        if target == "structural-cpp":
            add("simulation_main.cpp", payload["simulation_main"], "runtime-glue")
            add("simulation_config.json", payload["simulation_config"], "runtime-glue")
        else:
            add("simulation_top.sv", payload["simulation_top"], "runtime-glue")
    if target == "structural-cpp" and any(
        path.endswith(".cpp")
        and len(text.encode("utf-8")) > _STRUCTURAL_CPP_MAX_TU_BYTES
        for path, (text, _role) in files.items()
    ):
        raise _DriverError(
            "native structural C++ translation unit exceeds the 1 MiB limit"
        )
    for row in payload.get("source_maps", []):
        add(row["path"], row["text"], "source-map")
        group(row["source"]).append(row["path"])
    add(
        "CMakeLists.txt",
        _cmake(
            sources,
            rtl_sources,
            target=target,
            standard_sources=payload.get("rtl_standard_sources", []),
            root_rtl_name=payload.get("root_rtl_name", ""),
            simulation=simulation,
            verilator_policy=policy if target == "verilog" else None,
        ),
        "cmake",
    )
    for row in groups.values():
        row["files"].sort(key=lambda path: path.encode("utf-8"))
    receipt = {
        "kind": "pycircuit-generated",
        "target": target,
        "entry": payload["entry"],
        "entry_source": payload["entry_source"],
        "files": [{"path": path, "role": files[path][1]} for path in sorted(files)],
        "source_groups": [groups[key] for key in sorted(groups)],
    }
    return files, receipt


def emit_command(
    *,
    final: str | Path,
    target: str,
    output: str | Path,
    replace: bool = False,
    rtl_mode: str = "managed",
):
    if target not in {"verilog", "structural-cpp"}:
        raise _DriverError("unsupported publication target")
    if rtl_mode not in {"managed", "physical"}:
        raise _DriverError("RTL mode must be managed or physical")
    if target != "verilog" and rtl_mode != "managed":
        raise _DriverError("physical RTL mode requires the verilog target")
    fs = _PublicationFileSystem()
    source, destination = fs.absolute(final), fs.absolute(output)
    fs.assert_no_symlink_chain(source)
    fs.assert_no_symlink_chain(destination)
    if _paths_conflict(source, destination):
        raise _DriverError("emit input and output paths overlap")
    if destination.exists() or destination.is_symlink():
        _require_initialized_control(_paths_for(destination, fs), fs)
    helper = native_helper("emit")
    optimizer = native_helper("opt")
    control = _paths_for(source, fs).control
    with tempfile.TemporaryDirectory(prefix="pycircuit-emit-") as temporary:
        saved = Path(temporary) / "design_top.ac"
        while True:
            inputs = []
            snapshot = None
            if fs.kind(control) is not None:
                owner = _managed_owner(source, fs)
                inputs.append(
                    _PublicationInput(
                        source,
                        owner,
                        _validate_published_program,
                        _validate_published_program,
                    )
                )
            else:
                try:
                    fs.require_kind(source, "file")
                    snapshot = _read_stable_file_bytes(source)
                except (OSError, ValueError, RuntimeError):
                    if fs.kind(control) is not None:
                        continue
                    raise
                if fs.kind(control) is not None:
                    continue
            with _publication_lock_set(
                inputs=inputs,
                outputs=[_PublicationOutput(destination, _validate_emitted_bundle)],
                filesystem=fs,
            ) as locks:
                if inputs:
                    snapshot = locks.snapshot(source, _read_stable_file_bytes)
                elif fs.kind(control) is not None:
                    continue
                saved.write_bytes(snapshot)
                hwir = Path(temporary) / "design_top.hwir.mlir"
                if b"hwir.contract" in snapshot:
                    hwir.write_bytes(snapshot)
                else:
                    lowered = subprocess.run(
                        [
                            str(optimizer),
                            str(saved),
                            "--ac-final-to-hwir",
                            "-o",
                            str(hwir),
                        ],
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                    if lowered.returncode:
                        detail = " ".join((lowered.stderr or lowered.stdout).split())
                        raise _DriverError(
                            f"native HWIR conversion rejected final design: {detail}"
                        )
                emitted = subprocess.run(
                    [
                        str(helper),
                        str(hwir),
                        "--target",
                        target,
                        *(["--rtl-mode", rtl_mode] if target == "verilog" else []),
                    ],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                if emitted.returncode:
                    detail = " ".join((emitted.stderr or emitted.stdout).split())
                    raise _DriverError(f"native emit rejected final design: {detail}")
                payload = json.loads(emitted.stdout)
                files, receipt = _target_files(payload, target)
                receipt["ir_contract"] = payload.get("ir_contract", "hwir-v2")
                if target == "verilog":
                    receipt["rtl_mode"] = payload.get("rtl_mode", rtl_mode)
                owner = _publication_owner_generated(
                    **payload["entry_source"],
                    definition=payload["entry"]["definition"],
                    target=target,
                )

                def build(stage: Path, files=files, receipt=receipt):
                    for relative, (text, _role) in files.items():
                        path = stage / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_text(text, encoding="utf-8")
                    (stage / "generated.json").write_text(
                        json.dumps(receipt, ensure_ascii=False, separators=(",", ":"))
                        + "\n",
                        encoding="utf-8",
                    )

                return _publish_directory(
                    destination,
                    owner=owner,
                    build=build,
                    validate=_validate_emitted_bundle,
                    replace=replace,
                    filesystem=fs,
                    locks=locks,
                )
