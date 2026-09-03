from __future__ import annotations

import json
from collections.abc import Mapping
from dataclasses import dataclass
from typing import Any

from .probe import TbProbes
from .tb import Tb


@dataclass(frozen=True)
class TestbenchProgram:
    """Serializable host-side testbench payload embedded in `.pyc`."""

    top_symbol: str
    top_header: str
    in_raw: tuple[str, ...]
    in_tys: tuple[str, ...]
    out_raw: tuple[str, ...]
    out_tys: tuple[str, ...]
    clocks: tuple[dict[str, Any], ...]
    reset: dict[str, Any] | None
    drives: tuple[dict[str, Any], ...]
    expects: tuple[dict[str, Any], ...]
    prints: tuple[dict[str, Any], ...]
    random_streams: tuple[dict[str, Any], ...]
    timeout_cycles: int
    finish_cycle: int | None
    sva_asserts: tuple[dict[str, Any], ...]
    probes: tuple[dict[str, Any], ...] = ()

    def as_dict(self) -> dict[str, Any]:
        return {
            "version": 1,
            "top_symbol": self.top_symbol,
            "top_header": self.top_header,
            "ports": {
                "inputs": [
                    {"name": n, "ty": t} for n, t in zip(self.in_raw, self.in_tys)
                ],
                "outputs": [
                    {"name": n, "ty": t} for n, t in zip(self.out_raw, self.out_tys)
                ],
            },
            "clocks": list(self.clocks),
            "reset": self.reset,
            "drives": list(self.drives),
            "expects": list(self.expects),
            "prints": list(self.prints),
            "random_streams": list(self.random_streams),
            "timeout_cycles": int(self.timeout_cycles),
            "finish_cycle": (
                None if self.finish_cycle is None else int(self.finish_cycle)
            ),
            "sva_asserts": list(self.sva_asserts),
            "probes": list(self.probes),
        }

    def as_json(self) -> str:
        return json.dumps(
            self.as_dict(), sort_keys=True, separators=(",", ":"), ensure_ascii=False
        )


def testbench_payload_from_tb(
    *,
    top_symbol: str,
    in_raw: list[str],
    in_tys: list[str],
    out_raw: list[str],
    out_tys: list[str],
    tb: Tb,
    probes: TbProbes | None = None,
) -> TestbenchProgram:
    probe_rows: tuple[dict[str, Any], ...] = ()
    if probes is not None:
        probe_rows = tuple(
            {
                "canonical_path": handle.path,
                "source_path": handle.source_path,
                "at": handle.at,
                "tags": dict(handle.tags),
                "width_bits": int(handle.width_bits),
                "ty": handle.ty,
            }
            for handle in probes.glob("**")
        )
    return TestbenchProgram(
        top_symbol=str(top_symbol),
        top_header=f"{top_symbol}.hpp",
        in_raw=tuple(str(x) for x in in_raw),
        in_tys=tuple(str(x) for x in in_tys),
        out_raw=tuple(str(x) for x in out_raw),
        out_tys=tuple(str(x) for x in out_tys),
        clocks=tuple(
            {
                "port": str(c.port),
                "half_period_steps": int(c.half_period_steps),
                "phase_steps": int(c.phase_steps),
                "start_high": bool(c.start_high),
            }
            for c in tb.clocks
        ),
        reset=(
            None
            if tb.reset_spec is None
            else {
                "port": str(tb.reset_spec.port),
                "cycles_asserted": int(tb.reset_spec.cycles_asserted),
                "cycles_deasserted": int(tb.reset_spec.cycles_deasserted),
            }
        ),
        drives=tuple(
            {"port": str(d.port), "value": int(d.value), "at": int(d.at)}
            for d in tb.drives
        ),
        expects=tuple(
            {
                "port": str(e.port),
                "value": int(e.value),
                "at": int(e.at),
                "phase": str(e.phase),
                "msg": (None if e.msg is None else str(e.msg)),
            }
            for e in tb.expects
        ),
        prints=tuple(
            {
                "fmt": str(p.fmt),
                "ports": [str(x) for x in p.ports],
                "at": (None if p.at is None else int(p.at)),
                "start": (None if p.start is None else int(p.start)),
                "every": (None if p.every is None else int(p.every)),
            }
            for p in tb.prints
        ),
        random_streams=tuple(
            {
                "port": str(r.port),
                "seed": int(r.seed),
                "start": int(r.start),
                "every": int(r.every),
            }
            for r in tb.random_streams
        ),
        timeout_cycles=int(tb.timeout_cycles),
        finish_cycle=(None if tb.finish_cycle is None else int(tb.finish_cycle)),
        sva_asserts=tuple(
            {
                "expr": str(a.expr),
                "clock": str(a.clock),
                "reset": (None if a.reset is None else str(a.reset)),
                "name": (None if a.name is None else str(a.name)),
                "msg": (None if a.msg is None else str(a.msg)),
            }
            for a in tb.sva_asserts
        ),
        probes=probe_rows,
    )


def emit_testbench_pyc(
    *,
    payload: Mapping[str, Any],
    tb_name: str,
    frontend_contract: str,
) -> str:
    payload_json = json.dumps(
        dict(payload), sort_keys=True, separators=(",", ":"), ensure_ascii=False
    )
    name_esc = json.dumps(str(tb_name), ensure_ascii=False)
    payload_esc = json.dumps(payload_json, ensure_ascii=False)
    # Intentionally no func bodies for testbench payload files.
    return (
        f'module attributes {{pyc.top = @{tb_name}, pyc.frontend.contract = "{frontend_contract}", '
        f"pyc.tb.name = {name_esc}, pyc.tb.payload = {payload_esc}}} {{\n"
        "}\n"
    )
