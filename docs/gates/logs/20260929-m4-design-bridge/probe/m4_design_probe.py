"""Probe the design/testbench artifact boundary with the committed design harness.

Read-only experiment: it writes only under the scratch directory passed as argv[1].
It answers three questions with real Python sources and the real compiler:

  A. Can a DUT-only closure rooted at a @module *with typed ports* become a
     design artifact today?
  B. Can a DUT-only closure rooted at a *portless* @module become a design
     artifact today, and what does the artifact contain?
  C. What does the current M2 shape (root = @system carrying stimulus, phase,
     checks and report) put into its artifact?

Run:
  P=/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit
  PYTHONPATH="$P/python/pycircuit/src:$P/python/semantic-core/src:$P/python/agentic-circuit/src" \
    python3 m4_design_probe.py /tmp/m4-design-probe
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

P = Path("/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit")
BIN = P / ".pycircuit_out/w10-pm/build/bin"
SCRATCH = Path(sys.argv[1] if len(sys.argv) > 1 else "/tmp/m4-design-probe")

from pycircuit._source_capture import _capture_source_file  # noqa: E402
from pycircuit._source_transport import _emit_source_transport  # noqa: E402

TYPES = """\
from typing import Annotated

Word = Annotated[int, range(256)]
Phase = Annotated[int, range(8)]
"""

INCREMENT = """\
from pycircuit import module, rule
from .types import Word

@module
def Increment(enabled: bool, incoming: Word, outgoing: Word):
    @rule
    def update():
        nonlocal outgoing
        if enabled and incoming != 0:
            outgoing = (incoming + 1) & 255

    update()
"""

BLINKER = """\
from pycircuit import module, rule
from .types import Word

@module
def Blinker():
    phase: Word = 0
    level: Word = 0

    @rule
    def tick():
        nonlocal phase, level
        phase = (phase + 1) & 255
        if phase == 0:
            level = 1

    tick()
"""

TEST_INCREMENT = """\
from pycircuit import system, rule, log, report
from .types import Word, Phase
from .increment import Increment

@system
def TestIncrement():
    enabled: bool = True
    incoming: Word = 1
    outgoing: Word = 0
    phase: Phase = 0
    dut = Increment(enabled, incoming, outgoing)

    @rule
    def fixture():
        nonlocal enabled, incoming, phase
        if phase == 0:
            assert outgoing == 0, "reset output"
            incoming = 4
        elif phase == 1:
            assert outgoing == 2, "first input"
            incoming = 0
        elif phase == 2:
            assert outgoing == 5, "second input"
        elif phase == 3:
            assert outgoing == 5, "disabled input holds"
            enabled = False
        elif phase == 4:
            assert outgoing == 5, "final output"
            print("increment passed", outgoing)
            log("info", "test_complete", outgoing)
            report("completed", 1)
        if phase < 4:
            phase = phase + 1

    fixture()
"""


def _compile(source: Path, *, root: Path, out: Path, headers=()) -> tuple[Path, Path]:
    out.mkdir(parents=True, exist_ok=True)
    capture = _capture_source_file(source, source_root=root)
    transport = out / f"{source.stem}.transport.mlir"
    body = out / f"{source.stem}.body.mlir"
    header = out / f"{source.stem}.interface.mlir"
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    cmd = [str(BIN / "acir-source-unit-harness"), "--capture", str(transport),
           "--package", "demo", "--path", source.relative_to(root).as_posix()]
    for dep in headers:
        cmd += ["--header", str(dep)]
    cmd += ["--body-out", str(body), "--interface-out", str(header)]
    done = subprocess.run(cmd, text=True, capture_output=True)
    if done.returncode != 0:
        raise SystemExit(f"unit compile failed for {source}:\n{done.stderr}")
    return body, header


def _link(pairs, top: str, out: Path):
    cmd = [str(BIN / "acir-design-harness")]
    for body, header in pairs:
        cmd += ["--body", str(body), "--header", str(header)]
    cmd += ["--top", top, "--target", "final", "--output", str(out)]
    return subprocess.run(cmd, text=True, capture_output=True)


def _emit(design: Path, target: str, out: Path):
    return subprocess.run(
        [str(BIN / "acir-design-harness"), "--design", str(design),
         "--target", target, "--output", str(out)],
        text=True, capture_output=True)


def _scenario(name: str, root: Path, units: Path, sources, top: str, root_stem: str):
    src = SCRATCH / name / "source"
    src.mkdir(parents=True, exist_ok=True)
    compiled = {}
    for stem, text, deps in sources:
        (src / f"{stem}.py").write_text(text)
        headers = tuple(compiled[d][1] for d in deps)
        compiled[stem] = _compile(src / f"{stem}.py", root=src,
                                 out=units / name / stem, headers=headers)
    pairs = [compiled[stem] for stem, _, _ in sources]
    # The .ac artifact is named after the Python source file owning the root.
    design = SCRATCH / name / f"{root_stem}.ac"
    result = _link(pairs, top, design)
    print(f"--- {name}: root={top} link rc={result.returncode}")
    if result.stderr.strip():
        print("    stderr:", result.stderr.strip().replace("\n", "\n            ")[:500])
    if not design.exists():
        return
    text = design.read_text()
    print(f"    design bytes={len(text)}"
          f" ac.system={'ac.system' in text}"
          f" ac.expect={'ac.expect' in text}"
          f" ac.observe={'ac.observe' in text}"
          f" stimulus-name={'TestIncrement' in text}")
    for target, ext in (("cpp", "cpp"), ("verilog", "sv")):
        out = design.with_suffix(f".{ext}")
        emitted = _emit(design, target, out)
        if out.exists():
            body = out.read_text()
            print(f"    emit {target}: rc={emitted.returncode} bytes={len(body)}"
                  f" FinalModel={'FinalModel' in body}"
                  f" FinalModelSim={'FinalModelSim' in body}")
        else:
            print(f"    emit {target}: rc={emitted.returncode} no output"
                  f" stderr={emitted.stderr.strip()[:200]}")


def main() -> None:
    units = SCRATCH / "units"
    _scenario("A-ported-module-root", SCRATCH, units,
              [("types", TYPES, ()), ("increment", INCREMENT, ("types",))],
              "demo.increment.Increment", "increment")
    _scenario("B-portless-module-root", SCRATCH, units,
              [("types", TYPES, ()), ("blinker", BLINKER, ("types",))],
              "demo.blinker.Blinker", "blinker")
    _scenario("C-system-root", SCRATCH, units,
              [("types", TYPES, ()), ("increment", INCREMENT, ("types",)),
               ("test_increment", TEST_INCREMENT, ("types", "increment"))],
              "demo.test_increment.TestIncrement", "test_increment")


if __name__ == "__main__":
    main()
