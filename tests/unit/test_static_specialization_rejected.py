"""Decision 0267 removed caller-inferred module specialization.

A valueclass default parameter or a build keyword argument is no longer a
specialization key: the emitter must fail closed and ask for an explicit
source-owned finite-family declaration instead of silently cloning the module.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
FRONTEND_PATH = ":".join(
    [
        str(ROOT / "python/semantic-core/src"),
        str(ROOT / "python/pycircuit/src"),
        str(ROOT),
    ]
)

VALUECLASS_DESIGN = """
from __future__ import annotations

from pycircuit import Circuit, module
from pycircuit.spec import valueclass


@valueclass
class Cfg:
    ways: int = 4
    mode: str = "a"


@module
def build(m: Circuit, cfg: Cfg = Cfg()) -> None:
    _ = cfg
    x = m.input("x", width=8)
    m.output("y", x)


build.__pycircuit_name__ = "caller_inferred"
"""

BUILD_ARGUMENT_DESIGN = """
from __future__ import annotations

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    module,
)


@module
def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    cd = domain.clock_domain
    x = m.input("x", width=8)
    r = m.out("r", domain=cd, width=8, init=0)
    r.set(x)
    m.output("y", r)


build.__pycircuit_name__ = "caller_inferred_build"


if __name__ == "__main__":
    print(compile_cycle_aware(build, name="caller_inferred_build", lanes=8).emit_mlir())
"""


def _emit(tmp_path: Path, source: str, name: str) -> subprocess.CompletedProcess[str]:
    design = tmp_path / name
    design.write_text(source, encoding="utf-8")
    environment = {
        "PATH": "/usr/bin:/bin",
        "PYTHONPATH": FRONTEND_PATH,
    }
    return subprocess.run(
        [
            sys.executable,
            "-m",
            "pycircuit.cli",
            "emit",
            str(design),
            "-o",
            str(tmp_path / "out.pyc"),
        ],
        text=True,
        capture_output=True,
        env=environment,
        check=False,
    )


def test_valueclass_default_parameter_fails_closed(tmp_path: Path) -> None:
    completed = _emit(tmp_path, VALUECLASS_DESIGN, "valueclass.py")

    assert completed.returncode != 0
    combined = completed.stdout + completed.stderr
    assert "caller-inferred specialization is forbidden" in combined
    assert "cfg" in combined


def test_build_keyword_argument_fails_closed(tmp_path: Path) -> None:
    design = tmp_path / "build_argument.py"
    design.write_text(BUILD_ARGUMENT_DESIGN, encoding="utf-8")
    completed = subprocess.run(
        [sys.executable, str(design)],
        text=True,
        capture_output=True,
        env={"PATH": "/usr/bin:/bin", "PYTHONPATH": FRONTEND_PATH},
        check=False,
    )

    assert completed.returncode != 0
    combined = completed.stdout + completed.stderr
    assert "caller-inferred specialization is forbidden" in combined
    assert "lanes" in combined
