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


def _cmake(sources: list[str], rtl_sources: list[str], *, target: str) -> str:
    text = """cmake_minimum_required(VERSION 3.25)
project(PycircuitDesign LANGUAGES C CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
add_executable(pycircuit_system runner_main.cpp)
target_include_directories(pycircuit_system PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pycircuit_system PRIVATE pycircuit::pyc6_runtime)
"""
    if target == "cpp":
        rows = "".join("  " + _cmake_quote(path) + "\n" for path in sources)
        text += "add_library(pycircuit_dut SHARED model_api.cpp\n" + rows + ")\n"
        text += "target_sources(pycircuit_system PRIVATE\n" + rows + ")\n"
        text += """target_include_directories(pycircuit_dut PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pycircuit_dut PRIVATE pycircuit::pyc6_runtime)
target_compile_definitions(pycircuit_dut PRIVATE AGENTIC_MODEL_BUILD)
set_target_properties(pycircuit_dut PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
"""
    else:
        text += r"""find_package(verilator REQUIRED)
# Some Verilator packages serialize JSON lists as unquoted CMake text.
# Preserve each element while configuring this target, then restore their helper.
file(READ "${verilator_CONFIG}" _pyc_verilator_config)
string(FIND "${_pyc_verilator_config}" "function(json_get_list " _pyc_list_start)
if(_pyc_list_start GREATER_EQUAL 0)
  string(SUBSTRING "${_pyc_verilator_config}" ${_pyc_list_start} -1 _pyc_list_tail)
  string(FIND "${_pyc_list_tail}" "endfunction()" _pyc_list_end)
  if(_pyc_list_end LESS 0)
    message(FATAL_ERROR "Cannot preserve Verilator's JSON list helper")
  endif()
  math(EXPR _pyc_list_length "${_pyc_list_end} + 13")
  string(SUBSTRING "${_pyc_list_tail}" 0 ${_pyc_list_length} _pyc_original_list)
  function(json_get_list RET JSON SECTION VARIABLE)
    string(JSON _length ERROR_VARIABLE _status LENGTH "${JSON}" ${SECTION} ${VARIABLE})
    if(NOT "${_status}" STREQUAL "NOTFOUND" OR _length EQUAL 0)
      set(${RET} "" PARENT_SCOPE)
      return()
    endif()
    math(EXPR _last "${_length} - 1")
    set(_arguments)
    foreach(_index RANGE ${_last})
      string(JSON _value GET "${JSON}" ${SECTION} ${VARIABLE} ${_index})
      string(REPLACE "\\" "\\\\" _value "${_value}")
      string(REPLACE "\"" "\\\"" _value "${_value}")
      string(REPLACE "$" "\\$" _value "${_value}")
      string(REPLACE ";" "\\;" _value "${_value}")
      list(APPEND _arguments "\"${_value}\"")
    endforeach()
    list(JOIN _arguments " " _serialized)
    set(${RET} "${_serialized}" PARENT_SCOPE)
  endfunction()
endif()
target_compile_definitions(pycircuit_system PRIVATE PYCIRCUIT_RTL_RUNNER VL_TIME_CONTEXT)
verilate(pycircuit_system SOURCES
"""
        text += "".join("  " + _cmake_quote(path) + "\n" for path in rtl_sources)
        text += """  TOP_MODULE PycircuitRunnerBridge PREFIX VPycircuitRunnerBridge
  VERILATOR_ARGS --Wno-fatal)
if(DEFINED _pyc_original_list)
  cmake_language(EVAL CODE "${_pyc_original_list}")
endif()
"""
    return text


def _target_files(payload: dict, target: str):
    """Copy structured native outputs; never parse or split generated source."""
    runner = payload["runner"]
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

    add("runner_metadata.hpp", runner["metadata_header"], "runtime-glue")
    add("runner_main.cpp", runner["main_source"], "runtime-glue")
    if target == "cpp":
        add("dut.h", runner["abi_header"], "header")
        add("model_api.cpp", runner["abi_source"], "runtime-glue")
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
        add("runner_bridge.sv", runner["rtl_bridge"], "runtime-glue")
        add("rtl_system.hpp", runner["rtl_adapter"], "runtime-glue")
        for row in payload["rtl_source_groups"]:
            add(row["path"], row["text"], "rtl")
            group(row["source"]).append(row["path"])
            rtl_sources.append(row["path"])
        rtl_sources += ["design_top.sv", "runner_bridge.sv"]
    for row in payload["source_maps"]:
        add(row["path"], row["text"], "source-map")
        group(row["source"]).append(row["path"])
    add("CMakeLists.txt", _cmake(sources, rtl_sources, target=target), "cmake")
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
    *, final: str | Path, target: str, output: str | Path, replace: bool = False
):
    if target not in {"cpp", "verilog"}:
        raise _DriverError("emit target must be cpp or verilog")
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
                emitted = subprocess.run(
                    [str(helper), str(saved), "--target", target],
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
