"""Public source compiler compile/link/emit flow from a saved final artifact."""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest
import test_source_preview_workflow as preview

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests/integration/pycircuit/public-driver"
NATIVE_DEFAULT = ROOT / ".pycircuit_out/source-root"
INSTALL_DEFAULT = ROOT / ".pycircuit_out/source-candidate-install"


def _native() -> Path:
    return Path(os.environ.get("PYCIRCUIT_NATIVE_BUILD", NATIVE_DEFAULT)).resolve()


def _install() -> Path:
    return Path(os.environ.get("PYCIRCUIT_COMPILER_INSTALL", INSTALL_DEFAULT)).resolve()


def _environment() -> dict[str, str]:
    native = _native()
    environment = os.environ.copy()
    environment.update(
        {
            "PYCIRCUIT_NATIVE_BUILD": str(native),
            "PYCIRCUIT_SOURCE_COMPILER": str(native / "bin/pycircuit-source-unit"),
            "PYCIRCUIT_LINKER": str(native / "bin/pycircuit-link"),
            "PYCIRCUIT_EMITTER": str(native / "bin/pycircuit-emit"),
        }
    )
    package_root = str(ROOT / "python/pycircuit/src")
    environment["PYTHONPATH"] = os.pathsep.join(
        item for item in (package_root, environment.get("PYTHONPATH", "")) if item
    )
    return environment


def _cli(*args: str, expected: int = 0) -> subprocess.CompletedProcess[str]:
    command = [sys.executable, "-m", "pycircuit.cli", *args]
    result = subprocess.run(
        command,
        cwd=ROOT,
        env=_environment(),
        text=True,
        capture_output=True,
        check=False,
        timeout=900,
    )
    assert result.returncode == expected, (
        f"command returned {result.returncode}, expected {expected}: {command!r}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    return result


def _compile_link(source: Path, output: Path, package: str) -> Path:
    units = output / "units"
    units.mkdir(parents=True, exist_ok=True)
    by_name = {
        name: units / Path(name).stem
        for name in ("types.py", "counter.py", "design_top.py")
    }
    for name in ("types.py", "counter.py", "design_top.py"):
        command = [
            "compile",
            "-c",
            str(source / name),
            "--source-root",
            str(source),
            "--package-prefix",
            package,
            "-o",
            str(by_name[name]),
        ]
        if name == "counter.py":
            command.extend(("-I", str(by_name["types.py"])))
        elif name == "design_top.py":
            command.extend(
                ("-I", str(by_name["types.py"]), "-I", str(by_name["counter.py"]))
            )
        _cli(*command)
    final = output / "final.ac"
    _cli(
        "link",
        *(str(by_name[name]) for name in ("types.py", "counter.py", "design_top.py")),
        "--top",
        f"{package}.design_top.DesignTop",
        "-o",
        str(final),
    )
    return final


def _tree(path: Path) -> dict[str, bytes]:
    return {
        item.relative_to(path).as_posix(): item.read_bytes()
        for item in path.rglob("*")
        if item.is_file()
    }


def _receipt(path: Path) -> dict[str, Any]:
    return json.loads((path / "generated.json").read_text(encoding="utf-8"))


def _assert_source_maps_match(cpp: Path, rtl: Path) -> None:
    left, right = _receipt(cpp), _receipt(rtl)
    left_groups = {
        tuple(sorted(group["source"].items())): group for group in left["source_groups"]
    }
    right_groups = {
        tuple(sorted(group["source"].items())): group
        for group in right["source_groups"]
    }
    assert left_groups.keys() == right_groups.keys()
    assert {key[1][1] for key in left_groups} == {
        "counter.py",
        "design_top.py",
        "types.py",
    }

    def maps(output: Path, groups: dict) -> dict[tuple, dict[str, Any]]:
        files = {row["path"]: row["role"] for row in _receipt(output)["files"]}
        result = {}
        for owner, group in groups.items():
            maps_in_group = [
                name for name in group["files"] if files[name] == "source-map"
            ]
            assert len(maps_in_group) == 1
            map_path = maps_in_group[0]
            value = json.loads((output / map_path).read_text(encoding="utf-8"))
            assert value["source"] == group["source"]
            assert value["generated_files"] == sorted(
                (name for name in group["files"] if name != map_path),
                key=lambda name: name.encode("utf-8"),
            )
            assert [row["path"] for row in _receipt(output)["files"]].count(
                map_path
            ) == 1
            result[owner] = value
        return result

    cpp_maps = maps(cpp, left_groups)
    rtl_maps = maps(rtl, right_groups)
    assert cpp_maps.keys() == rtl_maps.keys()
    for owner in cpp_maps:
        assert cpp_maps[owner]["source"] == rtl_maps[owner]["source"]
        assert cpp_maps[owner]["origins"] == rtl_maps[owner]["origins"]
    # The declaration source owns no generated RTL hardware, but still has its
    # final-IR inventory and source-map group.
    types_owner = next(key for key in right_groups if key[1][1] == "types.py")
    assert rtl_maps[types_owner]["generated_files"] == []


def _emit(
    final: Path, target: str, output: Path, *, replace: bool = False, expected: int = 0
):
    args = ["emit", str(final), "--target", target, "-o", str(output)]
    if replace:
        args.append("--replace")
    return _cli(*args, expected=expected)


def _exported_dut_symbols(library: Path) -> set[str]:
    if sys.platform == "darwin":
        command = ["nm", "-gU", str(library)]
    elif sys.platform == "win32":
        dumpbin = shutil.which("dumpbin")
        if dumpbin is None:
            pytest.fail(
                "Windows DUT export check requires dumpbin from a developer shell"
            )
        command = [dumpbin, "/nologo", "/exports", str(library)]
    else:
        command = ["nm", "-D", "--defined-only", str(library)]
    result = subprocess.run(
        command, text=True, capture_output=True, check=False, timeout=30
    )
    assert result.returncode == 0, f"{command!r}\n{result.stdout}\n{result.stderr}"
    symbols: set[str] = set()
    for line in result.stdout.splitlines():
        fields = line.split()
        if not fields:
            continue
        if sys.platform == "win32":
            if len(fields) < 4 or not fields[0].isdigit() or not fields[1].isdigit():
                continue
            name = fields[3]
        else:
            name = fields[-1]
        name = name.lstrip("_").split("@", 1)[0]
        if name:
            symbols.add(name)
    return symbols


def test_public_emit_uses_saved_final_for_both_targets_and_runs_installed_runtime(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    shutil.copytree(FIXTURE / "src", source)
    shutil.copytree(FIXTURE / "configs", tmp_path / "configs")
    workspace = tmp_path / "published"
    final = _compile_link(source, workspace, "source_public")

    # Emit in new CLI processes after the Python source and every source-unit
    # body/header have disappeared; the final artifact is the complete input.
    shutil.rmtree(source)
    shutil.rmtree(workspace / "units")
    cpp, rtl = tmp_path / "generated-cpp", tmp_path / "generated-verilog"
    _emit(final, "cpp", cpp)
    _emit(final, "verilog", rtl)
    _assert_source_maps_match(cpp, rtl)

    install = _install()
    if not (install / "share/pycircuit/cmake/pycircuitConfig.cmake").is_file():
        pytest.fail(
            f"source compiler generated CMake needs the installed Runtime package at {install}"
        )
    runners: dict[str, Path] = {}
    for target, generated in (("cpp", cpp), ("verilog", rtl)):
        build = tmp_path / f"build-{target}"
        command = [
            "cmake",
            "-S",
            str(generated),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_PREFIX_PATH=" + str(install),
        ]
        configured = subprocess.run(
            command, text=True, capture_output=True, check=False, timeout=180
        )
        assert configured.returncode == 0, f"{configured.stdout}\n{configured.stderr}"
        built = subprocess.run(
            ["cmake", "--build", str(build), "--parallel", "4"],
            text=True,
            capture_output=True,
            check=False,
            timeout=900,
        )
        assert built.returncode == 0, f"{built.stdout}\n{built.stderr}"
        runner = build / "pycircuit_system"
        if os.name == "nt":
            runner = runner.with_suffix(".exe")
        assert runner.is_file()
        runners[target] = runner
        if target == "cpp":
            library_names = (
                "pycircuit_dut.dll",
                "libpycircuit_dut.dylib",
                "libpycircuit_dut.so",
            )
            libraries = [
                build / name for name in library_names if (build / name).is_file()
            ]
            assert (
                len(libraries) == 1
            ), f"generated CPP DUT library is missing or ambiguous: {libraries}"
            assert _exported_dut_symbols(libraries[0]) == {"pycircuit_model_query_v1"}

    for target, runner in runners.items():
        events = tmp_path / f"{target}-events.jsonl"
        run = subprocess.run(
            [
                str(runner),
                "--config",
                str(FIXTURE / "configs/three-ticks.json"),
                "--events",
                str(events),
            ],
            text=True,
            capture_output=True,
            check=False,
            timeout=120,
        )
        assert run.returncode == 0, f"{target}: {run.stdout}\n{run.stderr}"
        # This oracle contains explicit expected event values and epochs; it
        # does not infer correctness from cross-backend agreement.
        preview._oracle().assert_design_top_run(events.read_bytes())


def test_emit_rejections_preserve_managed_and_unmanaged_outputs(tmp_path: Path) -> None:
    source = tmp_path / "source"
    shutil.copytree(FIXTURE / "src", source)
    final = _compile_link(source, tmp_path / "primary", "source_public")
    cpp = tmp_path / "managed-cpp"
    _emit(final, "cpp", cpp)
    baseline = _tree(cpp)

    corrupt = tmp_path / "invalid-final.ac"
    corrupt.write_text("this is not verified final MLIR\n", encoding="utf-8")
    _emit(corrupt, "cpp", cpp, replace=True, expected=1)
    assert _tree(cpp) == baseline

    # The same generated owner includes the target in its publication identity.
    _emit(final, "verilog", cpp, replace=True, expected=1)
    assert _tree(cpp) == baseline

    alternate = tmp_path / "alternate-source"
    shutil.copytree(FIXTURE / "src", alternate)
    other_final = _compile_link(alternate, tmp_path / "alternate", "source_other")
    _emit(other_final, "cpp", cpp, replace=True, expected=1)
    assert _tree(cpp) == baseline

    unmanaged = tmp_path / "unmanaged"
    unmanaged.mkdir()
    (unmanaged / "keep.txt").write_text("user data", encoding="utf-8")
    unmanaged_before = _tree(unmanaged)
    _emit(final, "cpp", unmanaged, replace=True, expected=1)
    assert _tree(unmanaged) == unmanaged_before

    target = tmp_path / "symlink-target"
    target.mkdir()
    (target / "keep.txt").write_text("linked user data", encoding="utf-8")
    symlink = tmp_path / "symlink-output"
    symlink.symlink_to(target, target_is_directory=True)
    _emit(final, "cpp", symlink, replace=True, expected=1)
    assert symlink.is_symlink()
    assert _tree(target) == {"keep.txt": b"linked user data"}


def test_emit_rejects_a_non_replace_unmanaged_destination(tmp_path: Path) -> None:
    source = tmp_path / "source"
    shutil.copytree(FIXTURE / "src", source)
    final = _compile_link(source, tmp_path / "published", "source_public")
    destination = tmp_path / "existing"
    destination.mkdir()
    marker = destination / "keep.txt"
    marker.write_text("leave intact", encoding="utf-8")
    before = hashlib.sha256(marker.read_bytes()).digest()

    _emit(final, "cpp", destination, expected=1)

    assert hashlib.sha256(marker.read_bytes()).digest() == before


def test_emit_rejects_nested_output_without_poisoning_final_publication_control(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source"
    shutil.copytree(FIXTURE / "src", source)
    final = _compile_link(source, tmp_path / "published", "source_public")
    final_control = final.parent / f".{final.name}.pycircuit-publication"
    before = _tree(final_control)
    nested_output = final_control / "evil"

    _emit(final, "cpp", nested_output, expected=1)

    assert _tree(final_control) == before
    assert not nested_output.exists()
    assert not (final_control / ".evil.pycircuit-publication").exists()

    normal_output = tmp_path / "normal-generated"
    _emit(final, "cpp", normal_output)
    assert (normal_output / "generated.json").is_file()
