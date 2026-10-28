"""Uninitialized State is X before a write and after reset, on both targets."""

import sys
from pathlib import Path

from transactional_test_support import SourceTest

fixture = SourceTest(sys.argv[1:], None, "uninitialized")
target = sys.argv[-1]
(fixture.sources / "design.py").write_text(
    """from enum import Enum
import pycircuit as ac

@ac.encoding(width=2)
class Mode(Enum):
    IDLE = 0
    ACTIVE = 2

@ac.struct
class Header:
    mode: Mode
    data: ac.bits[80]

@ac.module
def Top() -> tuple[ac.u8, Mode, Header]:
    saved: ac.u8
    mode: Mode
    header: Header
    written: ac.u1 = 0

    @ac.rule(guard=lambda: written == 0)
    def initialize() -> ac.u8:
        mode = Mode.ACTIVE
        header = Header(mode=Mode.ACTIVE, data=ac.bits[80](37))
        written = 1
        return 37

    saved = initialize()
    return saved, mode, header
"""
)
fixture.compile_source("design")
final = fixture.scratch / "uninitialized.ac"
linked = fixture.link_saved(("design",), "uninitialized.design.Top", final)
# The output binds the declared State; neither export nor Rule registration
# allocates a Queue. Omitted initialization remains explicit in saved ACIR.
allocations = [
    line for line in linked.read_text().splitlines() if "ac.resource.alloc" in line
]
assert len(allocations) == 4
assert all("!ac.state<" in line for line in allocations)
for name in ("saved", "mode", "header"):
    declaration = [line for line in allocations if f'"{name}"' in line]
    assert len(declaration) == 1
    assert " initial(" not in declaration[0]
executable = fixture.scratch / "uninitialized.exe"
if target == "cpp":
    bundle = fixture.scratch / "cpp"
    fixture.cli("emit", final, "--target", "cpp", "-o", bundle)
    runtime = Path(fixture.emitter).parent.parent / "runtime/libpyc6_runtime.a"
    fixture.run(
        [
            fixture.cxx,
            "-std=c++20",
            "-pthread",
            f"-I{fixture.repo / 'include'}",
            f"-I{bundle}",
            *bundle.rglob("*.cpp"),
            Path(__file__).with_suffix(".cpp"),
            runtime,
            "-o",
            executable,
        ]
    )
    for workers in (1, 2):
        fixture.run([executable, workers])
else:
    rtl = fixture.scratch / "uninitialized.sv"
    rtl.write_text(
        fixture.run(
            [fixture.emitter, final, "--target", "verilog", "--format", "source"]
        ).stdout
    )
    fixture.run(
        [
            "iverilog",
            "-g2012",
            "-s",
            "tb",
            "-o",
            executable,
            rtl,
            fixture.repo / "include/verilog/dff.v",
            fixture.repo / "include/verilog/dffe.v",
            Path(__file__).with_suffix(".sv"),
        ]
    )
    fixture.run(["vvp", executable])
print(f"uninitialized State {target}: PASS")
