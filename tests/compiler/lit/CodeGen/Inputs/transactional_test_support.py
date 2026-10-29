"""Shared source-unit compilation and generated-System execution plumbing."""

import json
import os
import shutil
import subprocess
import sys
from collections import Counter
from contextlib import contextmanager
from pathlib import Path


class SourceTest:
    def __init__(self, arguments, example, package):
        repo, compiler, linker, emitter, cxx, opt, scratch = arguments[:7]
        self.repo, self.scratch = Path(repo).resolve(), Path(scratch).resolve()
        self.emitter, self.cxx, self.opt = emitter, cxx, opt
        self.package = package
        self.sequence = 0
        if self.scratch.exists():
            shutil.rmtree(self.scratch)
        self.scratch.mkdir(parents=True)
        self.sources = self.scratch / "sources"
        if example is None:
            self.sources.mkdir()
        else:
            shutil.copytree(self.repo / "examples" / example, self.sources)
        self.env = dict(
            os.environ,
            PYTHONPATH=str(self.repo / "python"),
            PYCIRCUIT_SOURCE_COMPILER=compiler,
            PYCIRCUIT_LINKER=linker,
            PYCIRCUIT_EMITTER=emitter,
        )

    def run(self, command, *, fails=False):
        self.sequence += 1
        command = [str(arg) for arg in command]
        result = subprocess.run(
            command, env=self.env, capture_output=True, text=True, timeout=240
        )
        prefix = self.scratch / f"{self.sequence:02d}"
        prefix.with_suffix(".command.json").write_text(json.dumps(command))
        prefix.with_suffix(".stdout").write_text(result.stdout)
        prefix.with_suffix(".stderr").write_text(result.stderr)
        prefix.with_suffix(".status").write_text(str(result.returncode) + "\n")
        assert (result.returncode != 0) == fails, (
            command,
            result.returncode,
            result.stdout,
            result.stderr,
        )
        return result

    def cli(self, *arguments, fails=False):
        return self.run(
            [sys.executable, "-m", "pycircuit.cli", *arguments], fails=fails
        )

    def compile_source(
        self, name, *interfaces, output=None, replace=False, fails=False
    ):
        arguments = [
            "compile",
            "-c",
            self.sources / f"{name}.py",
            "--source-root",
            self.sources,
            "--package-prefix",
            self.package,
            "-o",
            self.scratch / (output or name),
        ]
        for interface in interfaces:
            arguments.extend(("-I", self.scratch / interface))
        if replace:
            arguments.append("--replace")
        return self.cli(*arguments, fails=fails)

    def artifact(self, unit, kind):
        receipt = json.loads((self.scratch / unit / "unit.json").read_text())
        return self.scratch / unit / receipt["files"][kind]

    def link_saved(self, units, top, final):
        """Resume a portable Linked artifact with no source-unit inputs left."""
        linked = final.with_suffix(".linked.ac")
        self.cli(
            "link",
            *(self.scratch / unit for unit in units),
            "--top",
            top,
            "--target",
            "linked",
            "-o",
            linked,
        )
        if self.sources.exists():
            shutil.rmtree(self.sources)
        for unit in units:
            shutil.rmtree(self.scratch / unit)
        portable = final.with_suffix(".portable.ac")
        shutil.copyfile(linked, portable)
        self.cli("link", "--from-linked", portable, "-o", final)
        return portable

    @contextmanager
    def interfaces_only(self, *units):
        hidden = []
        try:
            for unit in units:
                body = self.artifact(unit, "body")
                saved = self.scratch / f"{unit}-hidden.ac"
                body.rename(saved)
                hidden.append((body, saved))
            yield
        finally:
            for body, saved in hidden:
                saved.rename(body)

    def build_system(self, final, target, *, name=None):
        bundle = self.scratch / (name or target)
        self.cli("emit", final, "--target", target, "-o", bundle)
        generated = json.loads((bundle / "generated.json").read_text())
        executable = self.scratch / f"system-{name or target}.exe"
        if target in ("cpp", "t-gfsim"):
            runtime = Path(self.emitter).parent.parent / "runtime/libpyc6_runtime.a"
            if not runtime.is_file():
                runtime = Path(self.emitter).parent.parent / "lib/libpyc6_runtime.a"
            self.run(
                [
                    self.cxx,
                    "-std=c++20",
                    "-pthread",
                    f"-I{self.repo / 'include'}",
                    f"-I{bundle}",
                    *(
                        bundle / row["path"]
                        for row in generated["files"]
                        if row["path"].endswith(".cpp")
                    ),
                    runtime,
                    "-o",
                    executable,
                ]
            )
        else:
            self.run(
                [
                    sys.executable,
                    self.repo / "cmake/verify_example.py",
                    "--compile-system",
                    bundle,
                    "--output",
                    executable,
                    "--include",
                    self.repo / "include",
                ]
            )
        return executable, generated

    def build_module(self, final, target, cpp_adapter, rtl_adapter):
        """Build an ordinary exported Module with test-owned public-port stimulus."""
        executable = self.scratch / f"module-{target}.exe"
        if target == "cpp":
            bundle = self.scratch / "cpp"
            self.cli("emit", final, "--target", target, "-o", bundle)
            generated = json.loads((bundle / "generated.json").read_text())
            runtime = Path(self.emitter).parent.parent / "runtime/libpyc6_runtime.a"
            if not runtime.is_file():
                runtime = Path(self.emitter).parent.parent / "lib/libpyc6_runtime.a"
            self.run(
                [
                    self.cxx,
                    "-std=c++20",
                    "-pthread",
                    f"-I{self.repo / 'include'}",
                    f"-I{bundle}",
                    cpp_adapter,
                    *(
                        bundle / row["path"]
                        for row in generated["files"]
                        if row["path"].endswith(".cpp")
                    ),
                    runtime,
                    "-o",
                    executable,
                ]
            )
        else:
            rtl = self.scratch / "module.sv"
            rtl.write_text(
                self.run(
                    [self.emitter, final, "--target", "verilog", "--format", "source"]
                ).stdout
            )
            self.run(
                [
                    "iverilog",
                    "-g2012",
                    "-s",
                    "tb",
                    "-o",
                    executable,
                    rtl,
                    self.repo / "include/verilog/dff.v",
                    self.repo / "include/verilog/dffe.v",
                    rtl_adapter,
                ]
            )
        return executable

    def build_runtime_consumer(self, target):
        """Build an emitted bundle against the installed Runtime without LLVM."""
        portable = self.scratch / "portable-generated"
        shutil.copytree(self.scratch / target, portable)
        build = self.scratch / "runtime-only-build"
        self.run(
            [
                "cmake",
                "-S",
                portable,
                "-B",
                build,
                f"-DCMAKE_PREFIX_PATH={os.environ['PYC_TOOLCHAIN_ROOT']}",
                "-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=TRUE",
                "-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=TRUE",
                f"-DCMAKE_CXX_COMPILER={self.cxx}",
            ]
        )
        self.run(
            ["cmake", "--build", build, "--target", "pycircuit_sim", "--parallel", 2]
        )
        return build / "bin/pycircuit_sim"


def log_history(result, *, successful=True):
    """Decode public System logs without assuming a design or execution order.

    Known payloads are integers; X/Z payloads retain (value, known, z) masks.
    """
    rows = [
        json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")
    ]
    if successful:
        assert any(row.get("kind") == "result" for row in rows), rows
    history = Counter()
    for row in rows:
        if row.get("kind") != "log":
            continue
        values = []
        for value in row["values"]:
            if value["kind"] == "bits4":
                bits = tuple(int(value[key], 16) for key in ("value", "known", "z"))
                values.append(
                    bits[0]
                    if bits[1] == (1 << value["width"]) - 1 and bits[2] == 0
                    else bits
                )
            else:
                values.append(int(value["value"]))
        history[
            (int(row["evaluation_epoch"]), row["spec"]["event"], tuple(values))
        ] += 1
    return history
