from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
CHECKER = ROOT / "flows/tools/check_generated_rtl.py"


def run_checker(
    tmp_path: Path, text: str, *extra: str
) -> subprocess.CompletedProcess[str]:
    rtl = tmp_path / "generated.sv"
    rtl.write_text(text, encoding="utf-8")
    return subprocess.run(
        (sys.executable, CHECKER, rtl, *extra),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def test_generated_rtl_audit_accepts_explicit_readable_structure(
    tmp_path: Path,
) -> None:
    checked = run_checker(
        tmp_path,
        """module checked_unit (input clk, input value, output result);
wire result_value;
assign result_value = 1'd1;
child_unit child (
  .clk(clk),
  .value(value),
  .result(result_value)
);
assign result = result_value;
endmodule
""",
    )
    assert checked.returncode == 0, checked.stdout + checked.stderr


@pytest.mark.parametrize(
    ("text", "message"),
    [
        (
            "module BadName (output result); assign result = 1'd0; endmodule\n",
            "not readable lower-snake",
        ),
        (
            "module bad_literal (output result);\nassign result = 1;\nendmodule\n",
            "unsized value literal",
        ),
        (
            """module bad_port (input a, input b, output result);
child_unit child (
  .value(a & b),
  .result(result)
);
endmodule
""",
            "contains an expression",
        ),
        (
            "module bad_ascii (output result); // café\nassign result = 1'd0;\nendmodule\n",
            "is not ASCII",
        ),
        (
            "module dead_net (output result);\nwire unused;\nassign result = 1'd0;\nendmodule\n",
            "has dead internal nets",
        ),
        (
            "module z_last ();\nendmodule\nmodule a_first ();\nendmodule\n",
            "modules are not deterministically ordered",
        ),
    ],
)
def test_generated_rtl_audit_rejects_malformed_output(
    tmp_path: Path, text: str, message: str
) -> None:
    checked = run_checker(tmp_path, text)
    assert checked.returncode == 1
    assert message in checked.stdout


def test_runtime_obligation_does_not_authorize_synthesis(tmp_path: Path) -> None:
    checked = run_checker(
        tmp_path,
        """module checked_unit (input clk, input condition);
obligation_range: assert property (@(posedge clk) (condition));
obligation_range_coverage: cover property (@(posedge clk) (condition));
endmodule
""",
        "--require-synthesis-admissible",
    )
    assert checked.returncode == 1
    assert "runtime-checked obligations cannot authorize synthesis" in checked.stdout
