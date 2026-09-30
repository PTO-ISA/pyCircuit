#!/usr/bin/env python3
"""Private final-snapshot to runnable preview transport; not public C3 emit."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [
    str(ROOT / "python/pycircuit/src"),
]

from pycircuit._driver import _validate_published_program  # noqa: E402
from pycircuit._generated_bundle import (  # noqa: E402
    _relative_file_path,
    _validate_generated_bundle,
)
from pycircuit._native_verify import verify_program_owner  # noqa: E402
from pycircuit._publication import (  # noqa: E402
    _paths_for,
    _publication_owner_generated,
    _publish_directory,
    _read_journal,
    _read_published,
    _require_initialized_control,
    _validate_control,
)
from pycircuit._publication_fs import _PublicationFileSystem  # noqa: E402
from pycircuit._source_capture import _read_stable_file_bytes  # noqa: E402


def final_snapshot(path: Path) -> bytes:
    filesystem = _PublicationFileSystem()
    path = filesystem.absolute(path)
    filesystem.assert_no_symlink_chain(path)
    paths = _paths_for(path, filesystem)
    while filesystem.kind(paths.control) is None:
        try:
            filesystem.require_kind(path, "file")
            snapshot = _read_stable_file_bytes(path)
        except (OSError, ValueError, RuntimeError):
            if filesystem.kind(paths.control) is None:
                raise
            break
        # A publisher may have bootstrapped management during the read.
        # Discard that snapshot and enter the managed recovery/lock path.
        if filesystem.kind(paths.control) is None:
            return snapshot
    filesystem.require_kind(paths.control, "directory")
    filesystem.require_kind(paths.lock, "file")
    with filesystem.lock(paths.lock, shared=True):
        _validate_control(paths, filesystem, cleanup_temporary=False)
        journal = _read_journal(paths, filesystem, cleanup_temporary=False)
        owner = journal["owner"] if journal else verify_program_owner(path)
    if owner["kind"] != "program":
        raise ValueError("preview input is not a managed final design")
    return _read_published(
        path,
        owner=owner,
        stable_validate=_validate_published_program,
        recovery_validate=_validate_published_program,
        read=_read_stable_file_bytes,
        filesystem=filesystem,
    )


def cmake_text(sources: list[str], *, rtl: bool) -> str:
    def quote(value: str) -> str:
        return (
            '"'
            + value.replace("\\", "\\\\")
            .replace('"', '\\"')
            .replace("$", "\\$")
            .replace(";", "\\;")
            + '"'
        )

    text = """cmake_minimum_required(VERSION 3.25)
project(PycircuitMigrationModel LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT IS_DIRECTORY "${PYCIRCUIT_RUNTIME_ROOT}/include/gfsim")
  message(FATAL_ERROR "PYCIRCUIT_RUNTIME_ROOT must name this checkout's runtime")
endif()
add_library(pyc6_runtime STATIC
  "${PYCIRCUIT_RUNTIME_ROOT}/model_input.cpp"
  "${PYCIRCUIT_RUNTIME_ROOT}/sim_executor.cpp"
  "${PYCIRCUIT_RUNTIME_ROOT}/system_runner.cpp")
target_include_directories(pyc6_runtime PUBLIC "${PYCIRCUIT_RUNTIME_ROOT}/include")
set_target_properties(pyc6_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
function(configure_model target consumer)
add_executable(${target} "${consumer}"
"""
    text += "".join(f"  {quote(source)}\n" for source in sources)
    text += """)
target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${target} PRIVATE pyc6_runtime)
"""
    if rtl:
        text += """find_package(verilator REQUIRED)
target_compile_definitions(${target} PRIVATE PYCIRCUIT_RTL_RUNNER VL_TIME_CONTEXT)
verilate(${target} SOURCES hardware.sv runner_bridge.sv
  TOP_MODULE PycircuitRunnerBridge PREFIX VPycircuitRunnerBridge
  VERILATOR_ARGS --Wno-fatal)
"""
    text += """endfunction()
configure_model(pycircuit_system runner_main.cpp)
if(PYCIRCUIT_TEST_CONSUMER)
  configure_model(m4_reset_replay "${PYCIRCUIT_TEST_CONSUMER}")
endif()
"""
    if not rtl:
        text += "add_library(pycircuit_dut SHARED model_api.cpp\n"
        text += "".join(f"  {quote(source)}\n" for source in sources)
        text += """)
target_include_directories(pycircuit_dut PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pycircuit_dut PRIVATE pyc6_runtime)
target_compile_definitions(pycircuit_dut PRIVATE AGENTIC_MODEL_BUILD)
set_target_properties(pycircuit_dut PROPERTIES
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
"""
    return text


def preview_files(payload: dict, *, rtl: bool):
    runner = payload["runner"]
    files = {
        "runner_metadata.hpp": (runner["metadata_header"], "runtime-glue"),
        "runner_main.cpp": (runner["main_source"], "runtime-glue"),
    }
    groups = []
    sources = []
    if rtl:
        files.update(
            {
                "hardware.sv": (runner["rtl_hardware"], "rtl"),
                "runner_bridge.sv": (runner["rtl_bridge"], "runtime-glue"),
                "rtl_system.hpp": (runner["rtl_adapter"], "runtime-glue"),
            }
        )
        # Private aggregate simulation input; never falsely assign it to root.
    else:
        files["dut.h"] = (runner["abi_header"], "header")
        files["model_api.cpp"] = (runner["abi_source"], "runtime-glue")
        files["pycircuit_support.hpp"] = (payload["support_header"], "runtime-glue")
        files["pycircuit_system.hpp"] = (payload["system_header"], "runtime-glue")
        for group in payload["source_groups"]:
            members = [group["header_path"]]
            for path, content, role in (
                (group["header_path"], group["header"], "header"),
                (group["source_path"], group["implementation"], "source"),
            ):
                if path is None:
                    if content is not None:
                        raise ValueError("header-only group has implementation text")
                    continue
                _relative_file_path(path)
                if path in files or type(content) is not str:
                    raise ValueError("native source group has duplicate/invalid files")
                files[path] = (content, role)
            if group["source_path"] is not None:
                members.append(group["source_path"])
                sources.append(group["source_path"])
            groups.append({"source": group["source"], "files": members})
    files["CMakeLists.txt"] = (cmake_text(sources, rtl=rtl), "cmake")
    for path, (contents, _role) in files.items():
        _relative_file_path(path)
        if type(contents) is not str:
            raise ValueError("native output is not text")
    receipt = {
        "kind": "pycircuit-generated",
        "target": "verilog" if rtl else "cpp",
        "entry": payload["entry"],
        "entry_source": payload["entry_source"],
        "files": [{"path": path, "role": files[path][1]} for path in sorted(files)],
        "source_groups": groups,
    }
    return files, receipt


def materialize(final: Path, output: Path, helper: Path) -> None:
    snapshot = final_snapshot(final)
    with tempfile.TemporaryDirectory(prefix="pycircuit-preview-") as temporary:
        saved = Path(temporary) / "design_top.ac"
        saved.write_bytes(snapshot)
        emitted = subprocess.run(
            [str(helper), str(saved), "--runner"],
            capture_output=True,
            text=True,
            check=False,
        )
        if emitted.returncode:
            raise ValueError(emitted.stderr.strip() or "native preview emission failed")
        payload = json.loads(emitted.stdout)
    # Complete all native generation/path admission before any publication.
    artifacts = [preview_files(payload, rtl=rtl) for rtl in (False, True)]
    filesystem = _PublicationFileSystem()
    output = filesystem.absolute(output)
    filesystem.assert_no_symlink_chain(output)
    for target in ("cpp", "verilog"):
        destination = output / target
        if destination.exists() or destination.is_symlink():
            _require_initialized_control(
                _paths_for(destination, filesystem), filesystem
            )
    output.mkdir(parents=True, exist_ok=True)
    for target, (files, receipt) in zip(("cpp", "verilog"), artifacts, strict=True):
        owner = _publication_owner_generated(
            **payload["entry_source"],
            definition=payload["entry"]["definition"],
            target=target,
        )

        def build(stage: Path, files=files, receipt=receipt) -> None:
            for relative, (content, _role) in files.items():
                path = stage / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding="utf-8")
            (stage / "generated.json").write_text(
                json.dumps(receipt, ensure_ascii=False, separators=(",", ":")) + "\n",
                encoding="utf-8",
            )

        _publish_directory(
            output / target,
            owner=owner,
            build=build,
            validate=_validate_generated_bundle,
            replace=True,
        )


def main() -> int:
    if len(sys.argv) != 4:
        print(
            "usage: materialize_m4_preview.py FINAL OUTPUT NATIVE_HELPER",
            file=sys.stderr,
        )
        return 2
    try:
        materialize(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]))
    except Exception as error:
        print(f"migration preview: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
