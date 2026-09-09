from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[4]
BUILD = REPOSITORY / ".pycircuit_out" / "acir" / "dev-llvm22"
FIXTURE = Path(__file__).parent / "fixtures" / "inspect"
RUNTIME_CONSUMER = (
    REPOSITORY / "tests/integration/agentic-circuit/runtime-install-consumer"
)


def install_to(prefix: Path) -> subprocess.CompletedProcess[str]:
    toolchain_root = os.environ.get("AC_GATE_TOOLCHAIN_ROOT")
    if toolchain_root:
        shutil.copytree(Path(toolchain_root), prefix, symlinks=True)
        return subprocess.CompletedProcess(
            args=("copytree", toolchain_root, os.fspath(prefix)),
            returncode=0,
            stdout="",
            stderr="",
        )
    return subprocess.run(
        ["cmake", "--install", str(BUILD), "--prefix", str(prefix)],
        cwd=REPOSITORY,
        text=True,
        capture_output=True,
        check=False,
    )


def run_installed(
    prefix: Path, *arguments: str, cwd: Path
) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment.pop("PYTHONPATH", None)
    return subprocess.run(
        [str(prefix / "bin/agentic-circuit"), *arguments],
        cwd=cwd,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def cmake_cache_path(name: str) -> Path:
    for line in (BUILD / "CMakeCache.txt").read_text().splitlines():
        if line.startswith(f"{name}:") and "=" in line:
            return Path(line.split("=", 1)[1])
    raise AssertionError(f"{name} is missing from {BUILD / 'CMakeCache.txt'}")


class InstallationTest(unittest.TestCase):
    def test_runtime_component_builds_without_llvm_or_mlir_packages(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "prefix"
            build = root / "build"
            installed = install_to(prefix)
            self.assertEqual(0, installed.returncode, installed.stderr)
            runtime_targets = (
                prefix
                / "lib/cmake/AgenticCircuit/AgenticCircuitRuntimeTargets.cmake"
            ).read_text()
            for forbidden in ("LLVM", "MLIR", "ACIRBindings", "GfsimTooling"):
                self.assertNotIn(forbidden, runtime_targets)
            runtime_headers = prefix / "include/gfsim"
            self.assertFalse((runtime_headers / "tooling").exists())
            self.assertFalse((prefix / "include/agentic-circuit-tooling").exists())
            for header in runtime_headers.rglob("*.h"):
                text = header.read_text()
                self.assertNotIn('#include "acir/', text, header)
                self.assertNotIn('#include "llvm/', text, header)
                self.assertNotIn("#include <llvm/", text, header)

            configured = subprocess.run(
                [
                    "cmake",
                    "-S",
                    str(RUNTIME_CONSUMER),
                    "-B",
                    str(build),
                    f"-DCMAKE_PREFIX_PATH={prefix}",
                    "-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=TRUE",
                    "-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=TRUE",
                ],
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, configured.returncode, configured.stderr)
            built = subprocess.run(
                ["cmake", "--build", str(build)],
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, built.returncode, built.stderr)
            executed = subprocess.run(
                [str(build / "runtime-consumer")],
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, executed.returncode, executed.stderr)

    def test_unknown_cmake_component_fails_with_supported_components(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "prefix"
            source = root / "consumer"
            source.mkdir()
            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.25)\n"
                "project(UnknownAgenticCircuitComponent LANGUAGES CXX)\n"
                "find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED "
                "COMPONENTS Unknown)\n",
                encoding="utf-8",
            )
            installed = install_to(prefix)
            self.assertEqual(0, installed.returncode, installed.stderr)
            configured = subprocess.run(
                [
                    "cmake",
                    "-S",
                    str(source),
                    "-B",
                    str(root / "build"),
                    f"-DCMAKE_PREFIX_PATH={prefix}",
                ],
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(0, configured.returncode)
            self.assertIn(
                "Unknown AgenticCircuit component 'Unknown'", configured.stderr
            )
            self.assertIn("Runtime and CompilerDev", configured.stderr)

    def test_default_component_rejects_missing_compiler_dev_cli(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "prefix"
            source = root / "consumer"
            source.mkdir()
            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.25)\n"
                "project(DefaultAgenticCircuitComponent LANGUAGES C CXX)\n"
                "find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED)\n",
                encoding="utf-8",
            )
            installed = install_to(prefix)
            self.assertEqual(0, installed.returncode, installed.stderr)
            executable = prefix / "bin/agentic-circuit"
            executable.rename(executable.with_suffix(".missing"))

            configured = subprocess.run(
                [
                    "cmake",
                    "-S",
                    str(source),
                    "-B",
                    str(root / "build"),
                    f"-DCMAKE_PREFIX_PATH={prefix}",
                    f"-DLLVM_DIR={cmake_cache_path('LLVM_DIR')}",
                    f"-DMLIR_DIR={cmake_cache_path('MLIR_DIR')}",
                ],
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(0, configured.returncode)
            self.assertIn("Agentic Circuit executable is missing", configured.stderr)

    def test_installed_prefix_runs_without_source_checkout(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            prefix = root / "prefix"
            project = root / "project"
            unrelated = root / "outside"
            shutil.copytree(FIXTURE, project)
            unrelated.mkdir()
            installed = install_to(prefix)
            self.assertEqual(0, installed.returncode, installed.stderr)
            installed_bins = {path.name for path in (prefix / "bin").iterdir()}

            doctor = run_installed(prefix, "doctor", "--json", cwd=unrelated)
            capabilities = run_installed(
                prefix, "schema", "capabilities", "--json", cwd=unrelated
            )
            opcode = run_installed(
                prefix,
                "schema",
                "opcode",
                "ac.reorder",
                "--json",
                cwd=unrelated,
            )
            checked = run_installed(
                prefix,
                "check",
                "architecture.py",
                "--project",
                str(project / "agentic-circuit.toml"),
                "--json",
                cwd=unrelated,
            )
            initialized = run_installed(
                prefix, "init", str(root / "initialized"), "--json", cwd=unrelated
            )
            built = run_installed(
                prefix,
                "build",
                "architecture.py",
                "--project",
                str(project / "agentic-circuit.toml"),
                "--output-dir",
                str(project / "build/main"),
                "--json",
                cwd=unrelated,
            )

        self.assertEqual(0, doctor.returncode, doctor.stderr)
        self.assertEqual(0, capabilities.returncode, capabilities.stderr)
        self.assertEqual(
            "agentic-circuit-capabilities",
            json.loads(capabilities.stdout)["schema"],
        )
        self.assertEqual(0, opcode.returncode, opcode.stderr)
        self.assertEqual("ac.reorder", json.loads(opcode.stdout)["operation"])
        self.assertEqual(0, checked.returncode, checked.stderr)
        self.assertEqual("passed", json.loads(checked.stdout)["status"])
        self.assertEqual(0, initialized.returncode, initialized.stderr)
        self.assertEqual(0, built.returncode, built.stderr)
        self.assertNotIn("pack-perfetto-trace.py", installed_bins)


if __name__ == "__main__":
    unittest.main()
