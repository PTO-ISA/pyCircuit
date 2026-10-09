"""Prove the current CompilerDev SDK keeps working after its prefix moves."""

from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path

import pytest
from test_public_emit import _assert_counter_trace

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests/integration/pycircuit/sdk-relocation"
DEFAULT_NATIVE_BUILD = ROOT / ".pycircuit_out/source-root"
SOURCES = ("types.py", "counter.py", "design_top.py")


def _native_build() -> Path:
    return Path(
        os.environ.get("PYCIRCUIT_NATIVE_BUILD", DEFAULT_NATIVE_BUILD)
    ).resolve()


def _checked(
    command: list[str],
    *,
    cwd: Path,
    env: dict[str, str],
    timeout: int = 900,
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout,
    )
    assert result.returncode == 0, (
        f"command failed ({result.returncode}): {command!r}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    return result


def _clean_environment(
    original_prefix: Path, *, host_tools: Path, venv_bin: Path
) -> dict[str, str]:
    environment = os.environ.copy()
    for name in tuple(environment):
        if (
            name in {"PYTHONPATH", "PYTHONHOME", "PYTHONNOUSERSITE"}
            or name
            in {
                "CMAKE_PREFIX_PATH",
                "CMAKE_TOOLCHAIN_FILE",
                "CPATH",
                "C_INCLUDE_PATH",
                "CPLUS_INCLUDE_PATH",
                "OBJC_INCLUDE_PATH",
                "LIBRARY_PATH",
                "CC",
                "CXX",
                "CPP",
                "CFLAGS",
                "CXXFLAGS",
                "LDFLAGS",
                "SDKROOT",
                "MACOSX_DEPLOYMENT_TARGET",
            }
            or name.startswith(
                ("PYTHON", "PYC", "ACIR", "LLVM", "MLIR", "DYLD_", "LD_LIBRARY_PATH")
            )
        ):
            environment.pop(name, None)

    original = str(original_prefix.resolve())
    environment["PATH"] = os.pathsep.join(
        (str(venv_bin), str(host_tools), "/usr/bin", "/bin", "/usr/sbin", "/sbin")
    )
    environment["PYTHONNOUSERSITE"] = "1"
    assert all(original not in entry for entry in environment["PATH"].split(os.pathsep))
    for name in (
        "PYTHONPATH",
        "CMAKE_PREFIX_PATH",
        "LLVM_DIR",
        "MLIR_DIR",
        "DYLD_LIBRARY_PATH",
        "LD_LIBRARY_PATH",
    ):
        assert name not in environment
    return environment


def _install_compiler_sdk(original_prefix: Path, env: dict[str, str]) -> None:
    native_build = _native_build()
    cache = native_build / "CMakeCache.txt"
    metadata = native_build / "toolchain-metadata.json"
    if not cache.is_file() or not metadata.is_file():
        pytest.fail(
            f"build the current checkout with CompilerDev first: {native_build}"
        )
    identity = json.loads(metadata.read_text(encoding="utf-8"))
    assert (
        identity["git_sha"]
        == _checked(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, env=env, timeout=30
        ).stdout.strip()
    )
    assert identity["llvm_version"] == "22.1.8"
    assert identity["platform"] == platform.system()
    assert identity["arch"] == platform.machine()

    _checked(
        ["cmake", "--install", str(native_build), "--prefix", str(original_prefix)],
        cwd=ROOT,
        env=env,
    )
    assert (original_prefix / "bin/pycircuit").is_file()
    assert (original_prefix / "share/pycircuit/cmake/pycircuitConfig.cmake").is_file()


def _assert_macos_native_dependencies_relocated(
    prefix: Path, old_prefix: Path, env: dict[str, str]
) -> None:
    native_build = _native_build()
    if sys.platform != "darwin":
        return
    otool = shutil.which("otool")
    assert otool, "macOS relocation test requires otool"

    file_command = shutil.which("file")
    assert file_command, "macOS relocation test requires file"
    native_files = [
        *sorted((prefix / "bin").iterdir()),
        *sorted((prefix / "lib").glob("*.dylib")),
    ]
    inspected = 0
    for binary in native_files:
        if not binary.is_file():
            continue
        file_description = _checked(
            [file_command, str(binary)], cwd=prefix, env=env, timeout=30
        ).stdout
        if "Mach-O" not in file_description:
            continue
        inspected += 1
        dependencies = _checked(
            [otool, "-L", str(binary)], cwd=prefix, env=env, timeout=30
        ).stdout
        assert str(old_prefix) not in dependencies, dependencies
        assert str(native_build) not in dependencies, dependencies
        assert str(ROOT) not in dependencies, dependencies
        own_install_name: str | None = None
        if binary.suffix == ".dylib":
            install_names = _checked(
                [otool, "-D", str(binary)], cwd=prefix, env=env, timeout=30
            ).stdout.splitlines()[1:]
            assert len(install_names) == 1, install_names
            own_install_name = install_names[0].strip()
            assert str(old_prefix) not in own_install_name
            assert str(native_build) not in own_install_name
            assert str(ROOT) not in own_install_name
            assert own_install_name.startswith(
                ("@rpath/", "@loader_path/", "@executable_path/")
            ), f"dylib has a non-relocatable install name: {binary}: {own_install_name}"

        rpaths = _checked(
            [otool, "-l", str(binary)], cwd=prefix, env=env, timeout=30
        ).stdout
        assert str(old_prefix) not in rpaths, rpaths
        assert str(native_build) not in rpaths, rpaths
        assert str(ROOT) not in rpaths, rpaths

        binary_rpaths: list[str] = []
        load_command = _checked(
            [otool, "-l", str(binary)], cwd=prefix, env=env, timeout=30
        ).stdout
        lines = load_command.splitlines()
        for index, line in enumerate(lines[:-1]):
            if line.strip() == "cmd LC_RPATH":
                path_line = lines[index + 2].strip()
                assert path_line.startswith("path "), path_line
                binary_rpaths.append(
                    path_line.removeprefix("path ").split(" (offset", 1)[0]
                )
        for rpath in binary_rpaths:
            if rpath.startswith(("/usr/lib", "/System/Library")):
                continue
            resolved_rpath = _resolve_loader_path(rpath, binary, prefix)
            assert resolved_rpath.is_relative_to(
                prefix.resolve()
            ), f"non-system runtime search path escapes relocated SDK: {binary}: {rpath}"

        for line in dependencies.splitlines()[1:]:
            dependency = line.strip().split(" (compatibility version", 1)[0]
            if not dependency or dependency == own_install_name:
                continue
            if dependency.startswith(("/usr/lib/", "/System/Library/")):
                continue
            if dependency.startswith("@rpath/"):
                relative = dependency.removeprefix("@rpath/")
                candidates = [
                    _resolve_loader_path(f"{rpath}/{relative}", binary, prefix)
                    for rpath in binary_rpaths
                ]
                assert any(
                    candidate.is_relative_to(prefix.resolve()) and candidate.is_file()
                    for candidate in candidates
                ), f"@rpath dependency is absent from relocated SDK: {binary}: {dependency}"
            elif dependency.startswith(("@loader_path/", "@executable_path/")):
                candidate = _resolve_loader_path(dependency, binary, prefix)
                assert (
                    candidate.is_relative_to(prefix.resolve()) and candidate.is_file()
                ), f"relative dependency is absent from relocated SDK: {binary}: {dependency}"
            else:
                pytest.fail(
                    f"non-system dependency is not relocatable: {binary}: {dependency}"
                )
    assert inspected > 0


def _resolve_loader_path(value: str, binary: Path, prefix: Path) -> Path:
    if value.startswith("@loader_path/"):
        relative = value.removeprefix("@loader_path/")
        return (binary.parent / relative).resolve()
    if value.startswith("@executable_path/"):
        relative = value.removeprefix("@executable_path/")
        return (binary.parent / relative).resolve()
    if value.startswith("@rpath/"):
        raise ValueError("@rpath must be expanded using the binary's LC_RPATH entries")
    return Path(value).resolve()


def _compile_and_link(
    driver: Path, source: Path, work: Path, env: dict[str, str]
) -> Path:
    units = work / "units"
    units.mkdir(parents=True)
    by_name = {name: units / Path(name).stem for name in SOURCES}
    for name in SOURCES:
        command = [
            str(driver),
            "compile",
            "-c",
            str(source / name),
            "--source-root",
            str(source),
            "--package-prefix",
            "measurement_relocated",
            "-o",
            str(by_name[name]),
        ]
        if name == "counter.py":
            command.extend(("-I", str(by_name["types.py"])))
        elif name == "design_top.py":
            command.extend(
                ("-I", str(by_name["types.py"]), "-I", str(by_name["counter.py"]))
            )
        _checked(command, cwd=work, env=env)

    final = work / "design_top.ac"
    _checked(
        [
            str(driver),
            "link",
            *(str(by_name[name]) for name in SOURCES),
            "--top",
            "measurement_relocated.design_top.DesignTop",
            "-o",
            str(final),
        ],
        cwd=work,
        env=env,
    )
    return final


def _assert_events(payload: bytes) -> None:
    _assert_counter_trace(
        payload,
        children={"root/__pyc_call_0": (7, 99)},
        event="relocation_tick",
        report="relocation_count",
    )


def test_compiler_sdk_compiles_links_emits_and_runs_after_prefix_move(
    tmp_path: Path,
) -> None:
    cmake = shutil.which("cmake")
    ninja = shutil.which("ninja")
    assert cmake and ninja, "relocated SDK test requires CMake and Ninja"

    original_prefix = tmp_path / "sdk-original"
    moved_prefix = tmp_path / "SDK moved ü"
    isolated_venv = tmp_path / "isolated-python"
    import venv

    venv.EnvBuilder(with_pip=False, system_site_packages=False).create(isolated_venv)
    venv_bin = isolated_venv / "bin"
    if sys.platform == "darwin":
        # Homebrew/uv Python builds resolve libpython relative to the venv
        # executable; keep the runtime library available without adding any
        # site packages to this isolated environment.
        library_name = Path(str(sysconfig.get_config_var("LDLIBRARY")))
        framework = sysconfig.get_config_var("PYTHONFRAMEWORK")
        if framework:
            framework_prefix = Path(
                str(sysconfig.get_config_var("PYTHONFRAMEWORKPREFIX"))
            )
            version = str(sysconfig.get_config_var("VERSION"))
            python_library = (
                framework_prefix
                / f"{framework}.framework"
                / "Versions"
                / version
                / str(framework)
            )
        else:
            python_library = (
                Path(str(sysconfig.get_config_var("LIBDIR"))) / library_name
            )
        assert python_library.is_file(), python_library
        venv_library = isolated_venv / "lib" / library_name
        venv_library.parent.mkdir(parents=True, exist_ok=True)
        venv_library.symlink_to(python_library)
    host_tools = tmp_path / "host-tools"
    host_tools.mkdir()
    host_tool_paths = {
        "cmake": cmake,
        "ninja": ninja,
        "git": shutil.which("git"),
        "otool": shutil.which("otool"),
        "file": shutil.which("file"),
        "verilator": shutil.which("verilator"),
        "make": shutil.which("make"),
    }
    for tool in ("clang", "clang++", "cc", "c++", "xcrun"):
        host_tool_paths[tool] = shutil.which(tool)
    for name, path in host_tool_paths.items():
        if path:
            (host_tools / name).symlink_to(path)
    (host_tools / "python3").symlink_to(venv_bin / "python3")
    env = _clean_environment(original_prefix, host_tools=host_tools, venv_bin=venv_bin)
    import_probe = subprocess.run(
        [str(venv_bin / "python3"), "-c", "import pycircuit"],
        cwd=tmp_path,
        env=env,
        text=True,
        capture_output=True,
        check=False,
        timeout=30,
    )
    assert import_probe.returncode != 0
    assert "ModuleNotFoundError" in import_probe.stderr
    _install_compiler_sdk(original_prefix, env)
    original_prefix.rename(moved_prefix)
    assert not original_prefix.exists()

    driver = moved_prefix / "bin/pycircuit"
    assert driver.is_file()
    _assert_macos_native_dependencies_relocated(moved_prefix, original_prefix, env)
    _checked([str(driver), "--help"], cwd=tmp_path, env=env, timeout=60)

    source = tmp_path / "independent-source"
    shutil.copytree(FIXTURE, source)
    work = tmp_path / "external-build-inputs"
    work.mkdir()
    final = _compile_and_link(driver, source, work, env)
    assert final.is_file()

    shutil.rmtree(source)
    shutil.rmtree(work / "units")
    cpp = tmp_path / "generated-cpp"
    verilog = tmp_path / "generated-verilog"
    for target, output in (("cpp", cpp), ("verilog", verilog)):
        _checked(
            [str(driver), "emit", str(final), "--target", target, "-o", str(output)],
            cwd=tmp_path,
            env=env,
        )
        assert (output / "CMakeLists.txt").is_file()

    runners: list[tuple[str, Path]] = []
    for target, output in (("cpp", cpp), ("verilog", verilog)):
        build = tmp_path / f"runtime-only-{target}-build"
        _checked(
            [
                cmake,
                "-S",
                str(output),
                "-B",
                str(build),
                "-G",
                "Ninja",
                "-DCMAKE_PREFIX_PATH=" + str(moved_prefix),
                "-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=TRUE",
                "-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=TRUE",
                "-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF",
                "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF",
            ],
            cwd=tmp_path,
            env=env,
            timeout=180,
        )
        cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
        assert "CMAKE_DISABLE_FIND_PACKAGE_LLVM:UNINITIALIZED=TRUE" in cache
        assert "CMAKE_DISABLE_FIND_PACKAGE_MLIR:UNINITIALIZED=TRUE" in cache
        assert not any(
            line.startswith(("LLVM_DIR:", "MLIR_DIR:")) for line in cache.splitlines()
        )
        package_dir = next(
            (
                line.split("=", 1)[1]
                for line in cache.splitlines()
                if line.startswith("pycircuit_DIR:")
            ),
            None,
        )
        assert package_dir is not None
        assert Path(package_dir).resolve().is_relative_to(moved_prefix.resolve())
        generated_graph = (build / "build.ninja").read_text(encoding="utf-8")
        assert str(moved_prefix) in generated_graph
        if target == "cpp":
            assert "libpyc6_runtime" in generated_graph
        else:
            assert "include/verilog" in generated_graph
            assert "verify_example.py" in generated_graph
        assert str(original_prefix) not in generated_graph
        assert str(_native_build()) not in generated_graph
        _checked(
            [
                cmake,
                "--build",
                str(build),
                "--target",
                "pycircuit_sim",
                "--parallel",
                "4",
            ],
            cwd=tmp_path,
            env=env,
        )
        runner = build / "bin/pycircuit_sim"
        if os.name == "nt":
            runner = runner.with_suffix(".exe")
        assert runner.is_file()
        runners.append((target, runner))

    config = tmp_path / "three-ticks.json"
    shutil.copyfile(FIXTURE / "three-ticks.json", config)
    for index, (target, runner) in enumerate(runners):
        events = tmp_path / f"events-{index}.jsonl"
        arguments = ["--cycles", "3"] if target == "cpp" else ["+cycles=3"]
        run = _checked(
            [str(runner), *arguments],
            cwd=tmp_path,
            env=env,
            timeout=120,
        )
        events.write_text(run.stdout, encoding="utf-8")
        _assert_events(events.read_bytes())
