import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


class BuildConfigurationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.repo_root = Path(__file__).resolve().parents[3]

    def test_production_configure_does_not_require_lit(self):
        mlir_dir = os.environ.get("MLIR_DIR")
        if not mlir_dir:
            self.skipTest("MLIR_DIR is required for the configure regression test")

        cmake = shutil.which("cmake")
        ninja = shutil.which("ninja")
        self.assertIsNotNone(cmake)
        self.assertIsNotNone(ninja)

        ignored_lit_dirs = {str(self.repo_root / ".venv" / "bin")}
        for executable in ("lit", "llvm-lit"):
            executable_path = shutil.which(executable)
            if executable_path:
                ignored_lit_dirs.add(str(Path(executable_path).resolve().parent))

        with tempfile.TemporaryDirectory() as temp_dir:
            result = subprocess.run(
                [
                    cmake,
                    "-S",
                    str(self.repo_root),
                    "-B",
                    str(Path(temp_dir) / "build"),
                    "-G",
                    "Ninja",
                    f"-DCMAKE_MAKE_PROGRAM={ninja}",
                    f"-DMLIR_DIR={mlir_dir}",
                    "-DBUILD_TESTING=OFF",
                    f"-DCMAKE_IGNORE_PATH={';'.join(sorted(ignored_lit_dirs))}",
                ],
                capture_output=True,
                env=os.environ,
                text=True,
            )

        self.assertEqual(
            result.returncode,
            0,
            msg=f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}",
        )

    def test_standalone_lit_requires_explicit_tool_paths(self):
        lit = shutil.which("lit") or str(Path(sys.executable).with_name("lit"))
        self.assertTrue(Path(lit).is_file(), f"lit executable not found: {lit}")

        environment = os.environ.copy()
        environment.pop("ACIR_TEST_EXEC_ROOT", None)
        environment.pop("ACIR_TOOLS_DIR", None)
        environment.pop("LLVM_TOOLS_DIR", None)
        result = subprocess.run(
            [lit, "-sv", str(self.repo_root / "test" / "ACIR" / "dialect-smoke.mlir")],
            capture_output=True,
            env=environment,
            text=True,
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ACIR_TEST_EXEC_ROOT", result.stderr)
        self.assertIn("ACIR_TOOLS_DIR", result.stderr)
        self.assertIn("LLVM_TOOLS_DIR", result.stderr)

    def test_binding_unit_target_configures_with_portable_gtest_only(self):
        cmake = shutil.which("cmake")
        ninja = shutil.which("ninja")
        self.assertIsNotNone(cmake)
        self.assertIsNotNone(ninja)

        binding_source = (self.repo_root / "unittests" / "Bindings").as_posix()
        support_targets = "\n".join(
            f"add_library({target} INTERFACE)"
            for target in (
                "ACIRBindings",
                "ACIRDialect",
                "ACIRTransforms",
                "ACSimDialect",
                "LLVMSupport",
                "MLIRFuncDialect",
                "MLIRIndexDialect",
                "MLIRParser",
                "MLIRSCFDialect",
            )
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            source_dir = Path(temp_dir) / "source"
            build_dir = Path(temp_dir) / "build"
            gtest_dir = source_dir / "gtest-only"
            gtest_dir.mkdir(parents=True)
            (gtest_dir / "GTestConfig.cmake").write_text(
                "add_library(GTest::gtest_main INTERFACE IMPORTED)\n"
                "set(GTest_FOUND TRUE)\n",
                encoding="utf-8",
            )
            (source_dir / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(BindingUnitConfigure LANGUAGES CXX)\n"
                "enable_testing()\n"
                f"{support_targets}\n"
                f'add_subdirectory("{binding_source}" bindings)\n',
                encoding="utf-8",
            )

            result = subprocess.run(
                [
                    cmake,
                    "-S",
                    str(source_dir),
                    "-B",
                    str(build_dir),
                    "-G",
                    "Ninja",
                    f"-DCMAKE_MAKE_PROGRAM={ninja}",
                    f"-DGTest_DIR={gtest_dir}",
                ],
                capture_output=True,
                text=True,
            )

        self.assertEqual(
            result.returncode,
            0,
            msg=f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}",
        )


if __name__ == "__main__":
    unittest.main()
