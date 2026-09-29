"""N0-U1: masked owned-register next assignment conformance and runtime oracle.

Covers ``state = (state + 1) & 255`` on an owned ``range(256)`` register
initialised to 254. The packet asserts that the lowered source keeps the mask
witness, the integer-boundary proof, the assignment's next-role use and the
exact data/enable-to-target yield binding; that redirecting or dropping those
witnesses is rejected; and that the emitted hardware counts every one of the
256 reachable states with the two-phase hold before each commit edge.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import pytest
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]

TYPES = """\
from typing import Annotated

Word = Annotated[int, range(256)]
"""

COUNTER = """\
from pycircuit import module, rule
from .types import Word

@module
def Counter():
    state: Word = 254

    @rule
    def tick():
        nonlocal state
        state = (state + 1) & 255

    tick()
"""


@dataclass(frozen=True)
class SourceUnit:
    source: Path
    body: Path
    header: Path


def _tool(env_name: str, name: str) -> Path:
    configured = os.environ.get(env_name)
    if configured and Path(configured).is_file():
        return Path(configured).resolve()
    found = shutil.which(name)
    if not found:
        raise AssertionError(
            f"set {env_name} or put {name} on PATH for the N0-U1 packet"
        )
    return Path(found).resolve()


def _source_harness() -> Path:
    return _tool("ACIR_SOURCE_UNIT_HARNESS", "acir-source-unit-harness")


def _design_harness() -> Path:
    return _tool("ACIR_DESIGN_HARNESS", "acir-design-harness")


def _compile(
    source: Path,
    *,
    source_root: Path,
    output_dir: Path,
    headers: tuple[Path, ...] = (),
    lower_numeric: bool = False,
) -> SourceUnit:
    output_dir.mkdir(parents=True, exist_ok=True)
    capture = _capture_source_file(source, source_root=source_root)
    transport = output_dir / f"{source.stem}.transport.mlir"
    body = output_dir / f"{source.stem}.body.mlir"
    header = output_dir / f"{source.stem}.interface.mlir"
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    command = [
        str(_source_harness()),
        "--capture", str(transport),
        "--package", "demo",
        "--path", source.relative_to(source_root).as_posix(),
    ]
    for dependency in headers:
        command.extend(("--header", str(dependency)))
    if lower_numeric:
        command.append("--lower-numeric")
    command.extend(("--body-out", str(body), "--interface-out", str(header)))
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    assert completed.returncode == 0, completed.stderr
    return SourceUnit(source, body, header)


def _compile_counter(tmp_path: Path) -> tuple[SourceUnit, SourceUnit]:
    source_root = tmp_path / "source"
    source_root.mkdir()
    (source_root / "types.py").write_text(TYPES, encoding="utf-8")
    (source_root / "counter.py").write_text(COUNTER, encoding="utf-8")
    types = _compile(
        source_root / "types.py", source_root=source_root,
        output_dir=tmp_path / "units/types",
    )
    counter = _compile(
        source_root / "counter.py", source_root=source_root,
        output_dir=tmp_path / "units/counter", headers=(types.header,),
        lower_numeric=True,
    )
    return types, counter


def _link(
    units: list[SourceUnit], output: Path, *, top: str = "demo.counter.Counter"
) -> subprocess.CompletedProcess[str]:
    command = [str(_design_harness())]
    for unit in units:
        command.extend(("--body", str(unit.body), "--header", str(unit.header)))
    command.extend(("--top", top, "--target", "final", "--output", str(output)))
    return subprocess.run(command, text=True, capture_output=True, check=False)


def _emit(design: Path, target: str, output: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(_design_harness()), "--design", str(design),
         "--target", target, "--output", str(output)],
        text=True, capture_output=True, check=False,
    )


def _linked_counter(tmp_path: Path) -> tuple[SourceUnit, Path]:
    types, counter = _compile_counter(tmp_path)
    design = tmp_path / "counter.ac"
    completed = _link([types, counter], design)
    assert completed.returncode == 0, completed.stderr
    return counter, design


def test_masked_next_retains_mask_boundary_use_and_yield_witnesses(
    tmp_path: Path,
) -> None:
    counter, design = _linked_counter(tmp_path)
    body_text = counter.body.read_text()
    design_text = design.read_text()

    # The source-level reset image is the literal 254 on a range(256) register.
    assert "ac.reg" in body_text
    assert 'ac.initial_value = {kind = "scalar"' in body_text
    assert "254" in body_text
    assert '"unsigned"' in body_text
    assert "upper = #ac.math_int<256>" in body_text

    # Every witness the packet requires must survive into the linked final
    # artifact, not only exist in the intermediate source body. The reset image
    # is re-encoded there, so its value is proved by the runtime oracle below
    # rather than by a literal match.
    for label, text in (("source body", body_text), ("final design", design_text)):
        # the mask witness survives lowering instead of being folded away
        assert "arith.andi" in text, label
        # the boundary proof keeps its check binding and the range check
        assert "ac.numeric.proof" in text, label
        assert "checks = [{id = " in text, label
        assert 'kind = "range"' in text, label
        # the assignment's next-role use and the exact data/enable yield binding
        assert "ac.value.use" in text, label
        assert 'role = "next"' in text, label
        assert 'kind = "next_scalar"' in text, label
        assert "ac.yield_bindings" in text, label
        assert "data_operand" in text and "enable_operand" in text, label

    assert "ac.initial_value" in design_text


def test_masked_next_rtl_keeps_the_reset_image_and_the_full_mask(
    tmp_path: Path,
) -> None:
    _, design = _linked_counter(tmp_path)
    rtl = tmp_path / "counter.sv"
    completed = _emit(design, "verilog", rtl)
    assert completed.returncode == 0, completed.stderr
    text = rtl.read_text()
    assert re.search(r"initial0\s*=\s*8'd254", text), text[:600]
    assert re.search(r"8'd255", text), text[:600]
    assert re.search(r"d0\s*=\s*", text), text[:600]
    assert "q0_e" in text


def test_masked_next_counts_every_reachable_state_in_hardware(
    tmp_path: Path,
) -> None:
    """Without observations the design's state is read through a testbench
    hierarchical probe, so the oracle needs no product interface change."""
    iverilog = _tool("IVERILOG", "iverilog")
    vvp = _tool("VVP", "vvp")
    _, design = _linked_counter(tmp_path)
    rtl = tmp_path / "counter.sv"
    assert _emit(design, "verilog", rtl).returncode == 0

    testbench = tmp_path / "counter_tb.sv"
    testbench.write_text(
        """\
module tb;
  logic clk = 1'b0;
  logic reset = 1'b1;
  FinalModel dut(.clk(clk), .reset(reset));
  integer i;
  logic [7:0] expected;
  task automatic tick;
    begin #1 clk = 1'b1; #1 clk = 1'b0; #1; end
  endtask
  initial begin
    tick();
    if (dut.q0 !== 8'd254) begin
      $display("RESET_MISMATCH %0d", dut.q0);
      $finish;
    end
    reset = 1'b0;
    expected = 8'd254;
    for (i = 0; i < 256; i = i + 1) begin
      // the two-phase hold: before the commit edge the old Q is still visible
      if (dut.q0 !== expected) begin
        $display("HOLD_MISMATCH %0d %0d %0d", i, dut.q0, expected);
        $finish;
      end
      tick();
      expected = expected + 8'd1;
      if (dut.q0 !== expected) begin
        $display("STEP_MISMATCH %0d %0d %0d", i, dut.q0, expected);
        $finish;
      end
    end
    $display("MASKED_NEXT_OK %0d", dut.q0);
    $finish;
  end
endmodule
""",
        encoding="utf-8",
    )
    binary = tmp_path / "counter.vvp"
    compiled = subprocess.run(
        [str(iverilog), "-g2012", "-s", "tb", "-o", str(binary),
         str(rtl), str(testbench)],
        text=True, capture_output=True, check=False,
    )
    assert compiled.returncode == 0, compiled.stderr
    ran = subprocess.run([str(vvp), str(binary)], text=True,
                         capture_output=True, check=False)
    assert ran.returncode == 0, ran.stderr
    assert "MASKED_NEXT_OK 254" in ran.stdout, ran.stdout


@pytest.mark.parametrize(
    "needle,replacement,diagnostic",
    [
        (
            "data_operand = 0 : i32",
            "data_operand = 3 : i32",
            "U1 YieldBinding is not the exact scalar contribution",
        ),
        (
            "enable_operand = 1 : i32",
            "enable_operand = 0 : i32",
            "U1 YieldBinding is not the exact scalar contribution",
        ),
        (
            'role = "next"',
            'role = "use"',
            "U1 ValueUse does not bind the required boundary value",
        ),
        (
            'kind = "next_scalar"',
            'kind = "read"',
            "U1 RequiredUse is not the boundary value assignment",
        ),
        (
            'kind = "range"',
            'kind = "assert"',
            "U1 requires exact low_bits and boundary witnesses",
        ),
    ],
)
def test_masked_next_rejects_redirected_witnesses(
    tmp_path: Path, needle: str, replacement: str, diagnostic: str
) -> None:
    """Redirecting the value, the enable, the use role, the target kind or the
    check kind must fail closed with the U1 diagnostic and publish nothing."""
    _, design = _linked_counter(tmp_path)
    text = design.read_text()
    assert needle in text, needle
    mutated = tmp_path / "redirected.ac"
    mutated.write_text(text.replace(needle, replacement, 1), encoding="utf-8")
    output = tmp_path / "must-not-exist.sv"

    result = _emit(mutated, "verilog", output)

    assert result.returncode != 0, result.stdout
    assert diagnostic in result.stderr, result.stderr
    assert not output.exists()


@pytest.mark.parametrize("dropped", ['"ac.numeric.proof"', '"ac.expect"'])
def test_masked_next_rejects_dropped_obligations(
    tmp_path: Path, dropped: str
) -> None:
    """Dropping a proof or a check must leave the lowered inventory unclosed."""
    _, design = _linked_counter(tmp_path)
    lines = design.read_text().splitlines(keepends=True)
    kept = [line for line in lines if dropped not in line]
    assert len(kept) < len(lines), dropped
    mutated = tmp_path / "dropped.ac"
    mutated.write_text("".join(kept), encoding="utf-8")
    output = tmp_path / "must-not-exist.sv"

    result = _emit(mutated, "verilog", output)

    assert result.returncode != 0, result.stdout
    assert "U1 lowered inventory is not closed" in result.stderr, result.stderr
    assert not output.exists()
