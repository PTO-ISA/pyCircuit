"""Publish native target artifacts from one locked, verified final snapshot."""

from __future__ import annotations

import json
import subprocess
import tempfile
from collections.abc import Mapping
from pathlib import Path

from ._driver import _DriverError, _validate_linked, _validate_published_program
from ._generated_bundle import _execution_metadata, _relative_file_path
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


def _managed_owner(
    path: Path, fs: _PublicationFileSystem, *, linked: bool = False
) -> Mapping[str, object]:
    paths = _paths_for(path, fs)
    _require_initialized_control(paths, fs)
    with fs.lock(paths.lock, shared=True):
        _validate_control(paths, fs, cleanup_temporary=False)
        journal = _read_journal(paths, fs, cleanup_temporary=False)
        owner = (
            journal["owner"] if journal else verify_program_owner(path, linked=linked)
        )
    if owner["kind"] != ("linked" if linked else "program"):
        raise _DriverError("emit input has the wrong artifact kind for this target")
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
    needs_four_state: bool = False,
) -> str:
    if simulation and target == "verilog":
        tools = (
            "find_program(PYCIRCUIT_IVERILOG iverilog REQUIRED)\n"
            "find_program(PYCIRCUIT_VVP vvp REQUIRED)\n"
            if needs_four_state
            else "find_program(PYCIRCUIT_VERILATOR verilator REQUIRED)\n"
        )
        arguments = (
            '--iverilog "${PYCIRCUIT_IVERILOG}" --vvp "${PYCIRCUIT_VVP}"'
            if needs_four_state
            else '--verilator "${PYCIRCUIT_VERILATOR}"'
        )
        return (
            """cmake_minimum_required(VERSION 3.25)
project(PycircuitSimulation LANGUAGES C CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
"""
            + tools
            + """set(PYCIRCUIT_SIM "${CMAKE_CURRENT_BINARY_DIR}/bin/pycircuit_sim${CMAKE_EXECUTABLE_SUFFIX}")
file(GLOB_RECURSE PYCIRCUIT_RTL CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.v" "${CMAKE_CURRENT_SOURCE_DIR}/*.sv")
file(GLOB PYCIRCUIT_PRIMITIVES CONFIGURE_DEPENDS "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}/verilog/*.v")
add_custom_command(OUTPUT "${PYCIRCUIT_SIM}"
  COMMAND "${Python3_EXECUTABLE}" "${PYCIRCUIT_CMAKE_DIR}/verify_example.py"
    --compile-system "${CMAKE_CURRENT_SOURCE_DIR}" --output "${PYCIRCUIT_SIM}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}" """
            + arguments
            + """
  BYPRODUCTS "${PYCIRCUIT_SIM}.backend.json"
  DEPENDS ${PYCIRCUIT_RTL} ${PYCIRCUIT_PRIMITIVES}
    "${CMAKE_CURRENT_SOURCE_DIR}/generated.json" "${PYCIRCUIT_CMAKE_DIR}/verify_example.py"
  VERBATIM)
add_custom_target(pycircuit_sim DEPENDS "${PYCIRCUIT_SIM}")
"""
        )
    if target == "t-gfsim":
        return """cmake_minimum_required(VERSION 3.25)
project(PycircuitTransactionalSimulation LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
add_executable(pycircuit_sim simulation_main.cpp)
target_compile_features(pycircuit_sim PRIVATE cxx_std_20)
set_target_properties(pycircuit_sim PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
target_link_libraries(pycircuit_sim PRIVATE pycircuit::pyc6_runtime)
"""
    text = """cmake_minimum_required(VERSION 3.25)
project(PycircuitModules LANGUAGES C CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
add_library(pycircuit_modules STATIC
"""
    if target == "cpp":
        text += "".join("  " + _cmake_quote(path) + "\n" for path in sources)
    text += """ )
target_include_directories(pycircuit_modules PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pycircuit_modules PUBLIC pycircuit::pyc6_runtime)
"""
    if target == "verilog":
        text += "find_package(verilator REQUIRED)\n"
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
        text += "  PREFIX Vpycircuit_modules\n  VERILATOR_ARGS --Wno-fatal)\n"
    if simulation and target == "cpp":
        text += (
            "add_executable(pycircuit_sim simulation_main.cpp)\n"
            "target_compile_features(pycircuit_sim PRIVATE cxx_std_20)\n"
            'set_target_properties(pycircuit_sim PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")\n'
            "target_link_libraries(pycircuit_sim PRIVATE pycircuit_modules)\n"
        )
    return text


def _target_files(payload: dict, target: str):
    """Copy structured native outputs; never parse or split generated source."""
    sample_rows, needs_four_state = _execution_metadata(payload)
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

    if target == "t-gfsim":
        add("pycircuit_model.hpp", payload["model_header"], "header")
        group(payload["entry_source"]).append("pycircuit_model.hpp")
    elif target == "cpp":
        add("pycircuit_support.hpp", payload["support_header"], "runtime-glue")
        add("pycircuit_system.hpp", payload["system_header"], "runtime-glue")
        for row in payload["source_groups"]:
            members = group(row["source"])
            add(row["header_path"], row["header"], "header")
            members.append(row["header_path"])
            if row["source_path"] is not None:
                add(row["source_path"], row["implementation"], "source")
                members.append(row["source_path"])
                sources.append(row["source_path"])
            elif row["implementation"] is not None:
                raise _DriverError("header-only source contains implementation text")
    else:
        add("design_top.sv", payload["rtl_core"], "rtl")
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
        if target in {"cpp", "t-gfsim"}
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
                    "system_samples": sample_rows,
                    "needs_four_state": needs_four_state,
                    "reset_semantics": payload.get("reset_semantics", "strict"),
                },
                separators=(",", ":"),
            )
            + "\n",
            "runtime-glue",
        )
        if target in {"cpp", "t-gfsim"}:
            add("simulation_main.cpp", payload["simulation_main"], "runtime-glue")
            add("simulation_config.json", payload["simulation_config"], "runtime-glue")
        else:
            add("simulation_top.sv", payload["simulation_top"], "runtime-glue")
    for row in payload["source_maps"]:
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
            needs_four_state=needs_four_state,
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
        "reset_semantics": payload.get("reset_semantics", "strict"),
        "system_samples": sample_rows,
        "needs_four_state": needs_four_state,
    }
    if target == "verilog":
        receipt["root_rtl_name"] = payload["root_rtl_name"]
        receipt["rtl_mode"] = payload.get("rtl_mode", "managed")
    return files, receipt


def emit_command(
    *,
    final: str | Path,
    target: str,
    output: str | Path,
    replace: bool = False,
    rtl_mode: str = "managed",
):
    if target not in {"cpp", "verilog", "t-gfsim"}:
        raise _DriverError("emit target must be cpp, verilog or t-gfsim")
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
    control = _paths_for(source, fs).control
    with tempfile.TemporaryDirectory(prefix="pycircuit-emit-") as temporary:
        saved = Path(temporary) / "design_top.ac"
        while True:
            inputs = []
            snapshot = None
            if fs.kind(control) is not None:
                owner = _managed_owner(source, fs, linked=target == "t-gfsim")
                validate_input = (
                    _validate_linked
                    if target == "t-gfsim"
                    else _validate_published_program
                )
                inputs.append(
                    _PublicationInput(
                        source,
                        owner,
                        validate_input,
                        validate_input,
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
                emitted = subprocess.run(
                    [str(helper), str(saved), "--target", target]
                    + (["--rtl-mode", rtl_mode] if target == "verilog" else []),
                    capture_output=True,
                    text=True,
                    check=False,
                )
                if emitted.returncode:
                    detail = " ".join((emitted.stderr or emitted.stdout).split())
                    raise _DriverError(f"native emit rejected final design: {detail}")
                payload = json.loads(emitted.stdout)
                files, receipt = _target_files(payload, target)
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
