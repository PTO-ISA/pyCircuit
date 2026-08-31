# Testbench

`@testbench` lets pyCircuit keep host/device simulation intent in the same frontend flow:

- frontend emits a TB `.pyc` payload (JSON encoded in module attrs)
- backend (`pycc`) lowers that payload to C++ or SystemVerilog testbench text

Observation points (pyCircuit 6):

- `phase="pre"` samples at **TICK-OBS** (after combinational settle, before state commit).
- `phase="post"` samples at **XFER-OBS** (after state commit).

## Authoring

Write a module `build` and a decorated testbench:

```python
from pycircuit import Circuit, Tb, module, testbench

@module
def build(m: Circuit):
    ...

@testbench
def tb(t: Tb):
    t.clock("clk")
    t.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    t.timeout(100)
    t.drive("in_valid", 0, at=0)
    t.expect("out_valid", 0, at=0, phase="pre")
    t.finish(at=10)
```

`pycircuit build` expects `tb` to be decorated with `@testbench`.

## Tb API (selected)

- `t.clock(port, half_period_steps=..., phase_steps=..., start_high=...)`
- `t.reset(port, cycles_asserted=..., cycles_deasserted=...)`
- `t.drive(port, value, at=cycle)`
- `t.expect(port, value, at=cycle, phase="pre"|"post", msg=None)`
- `t.timeout(cycles)`
- `t.finish(at=cycle)`
- `t.print(fmt, at=cycle, ports=[...])`
- `t.print_every(fmt, start=0, every=1, ports=[...])`
- `t.sva_assert(expr, clock=..., reset=..., name=..., msg=...)`

## Sidecar schedule mode

The default testbench path keeps the existing inline C++ schedule behavior.
For long or dense schedules, `--tb-schedule-mode sidecar` can externalize the drive/expect schedule into a sidecar container and keep the generated C++ runner shape stable.

```bash
pycc <design.py> --tb-schedule-mode sidecar
```

The current sidecar PR scope is limited to drive frames, expect events, port metadata, string metadata, and compact periodic drive patterns.
It does not include any generated workload API.
