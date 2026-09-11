from __future__ import annotations

from pycircuit import (
    Circuit,
    ConnectorStruct,
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    const,
    module,
    spec,
    u,
)


@const
def _pipe_struct(m: Circuit, *, width: int):
    _ = m
    base = (
        spec.struct("pipe")
        .field("payload.data", width=width)
        .field("ctrl.valid", width=1)
        .field("ctrl.ready", width=1)
        .build()
    )
    return (
        base.remove_field("ctrl.ready")
        .rename_field("payload.data", "word")
        .select_fields(["payload.word", "ctrl.valid"])
    )


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, *, width: int = 32):
    cd = domain.clock_domain
    clk = cd.clk
    rst = cd.rst

    s = _pipe_struct(m, width=width)
    in_b = m.inputs(s, prefix="in_")

    st0 = m.state(s, clk=cd.clk, rst=cd.rst, prefix="st0_")
    m.connect(st0, in_b)

    st1_in = st0.flatten()
    st1_in["payload.word"] = (st0["payload.word"].read() + u(width, 1))[0:width]

    st1 = m.state(s, clk=cd.clk, rst=cd.rst, prefix="st1_")
    m.connect(st1, ConnectorStruct(st1_in, spec=s))

    m.outputs(s, st1, prefix="out_")


build.__pycircuit_name__ = "pipeline_builder"
if __name__ == "__main__":
    print(compile_cycle_aware(build, name="pipeline_builder", width=32).emit_mlir())
