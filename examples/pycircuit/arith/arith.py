from __future__ import annotations

from pycircuit import (
    Circuit,
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    const,
    ct,
    module,
    spec,
    u,
)


@spec.valueclass
class LaneCfg:
    lanes: int
    lane_width: int


@const
def _derive_cfg(m: Circuit, *, lanes: int, lane_width: int) -> LaneCfg:
    _ = m
    return LaneCfg(lanes=max(1, int(lanes)), lane_width=max(1, int(lane_width)))


@const
def _acc_width(m: Circuit, cfg: LaneCfg) -> int:
    _ = m
    return int(cfg.lane_width) + ct.clog2(int(cfg.lanes))


@const
def _lane_mask(m: Circuit, *, width: int) -> int:
    _ = m
    w = max(1, int(width))
    return ct.bitmask(w)


def build(
    m: CycleAwareCircuit, domain: CycleAwareDomain, lanes: int = 8, lane_width: int = 16
) -> None:
    cfg = _derive_cfg(m, lanes=lanes, lane_width=lane_width)
    acc_w = _acc_width(m, cfg)
    lane_mask = _lane_mask(m, width=int(cfg.lane_width))

    a = m.input("a", width=acc_w)
    b = m.input("b", width=acc_w)

    m.output("sum", a + b)
    m.output("lane_mask", u(int(cfg.lane_width), lane_mask))
    m.output("acc_width", u(max(1, ct.clog2(256)), acc_w))


build.__pycircuit_name__ = "arith"


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="arith", eager=True, lanes=8, lane_width=16
        ).emit_mlir()
    )
