"""Shared source-unit compilation and generated-System execution plumbing."""

import json
import os
import shutil
import subprocess
import sys
from contextlib import contextmanager
from pathlib import Path


class SourceTest:
    def __init__(self, arguments, example, package):
        repo, compiler, linker, emitter, cxx, opt, scratch, target, mode = arguments
        self.repo, self.scratch = Path(repo).resolve(), Path(scratch).resolve()
        self.emitter, self.cxx, self.opt = emitter, cxx, opt
        self.package = package
        self.sequence = 0
        if self.scratch.exists():
            shutil.rmtree(self.scratch)
        self.scratch.mkdir(parents=True)
        self.sources = self.scratch / "sources"
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

    def compile_source(self, name, *interfaces, output=None, fails=False):
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
        return self.cli(*arguments, fails=fails)

    def artifact(self, unit, kind):
        receipt = json.loads((self.scratch / unit / "unit.json").read_text())
        return self.scratch / unit / receipt["files"][kind]

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

    def build_system(self, final, target, *, reset_cycle=None):
        bundle = self.scratch / target
        self.cli("emit", final, "--target", target, "-o", bundle)
        generated = json.loads((bundle / "generated.json").read_text())
        # Reset-only test adapter: change physical input stimulus, retaining
        # every generated sampling/check/commit and observation publication step.
        if reset_cycle is not None:
            if target == "cpp":
                harness = bundle / "simulation_main.cpp"
                before = "self.inputs.pyc_7079635f636c6b = pyc_bit((epoch & 1u) != 0);\n    self.inputs.pyc_7079635f727374 = pyc_bit(false);"
                after = before.replace(
                    "pyc_bit(false)", f"pyc_bit(epoch / 2 == {reset_cycle})"
                )
            else:
                harness = bundle / "simulation_top.sv"
                before = "pyc_step(1'b0, 1'b0);\n      pyc_step(1'b1, 1'b0);"
                after = f"pyc_step(1'b0, pyc_cycle == {reset_cycle});\n      pyc_step(1'b1, pyc_cycle == {reset_cycle});"
            text = harness.read_text()
            assert text.count(before) == 1
            harness.write_text(text.replace(before, after))
        executable = self.scratch / f"system-{target}.exe"
        if target == "cpp":
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
