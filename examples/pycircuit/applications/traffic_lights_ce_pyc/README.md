# Traffic Lights (pyCircuit)

A cycle-aware traffic lights controller based on the [Traffic-lights-ce](https://github.com/Starrynightzyq/Traffic-lights-ce) design.
It exposes BCD countdowns for East/West and North/South, plus discrete red/yellow/green lights.
The terminal emulator renders a simple 7-seg view and can load multiple stimulus patterns.

## Key files

- `traffic_lights_ce_pyc.py`: pyCircuit implementation of the FSM, countdowns, blink, and outputs.
- `traffic_lights_capi.cpp`: C API wrapper around the generated C++ model for ctypes.
- `emulate_traffic_lights.py`: terminal visualization; drives the DUT via the C API.
- `stimuli/*.py`: independent stimulus modules (driver logic separated from the DUT).
- `tb_traffic_lights_ce_pyc.py`: canonical compiler/simulation entrypoint.

## Ports

| Port | Dir | Width | Description |
|------|-----|-------|-------------|
| `clk` | in | 1 | System clock |
| `rst` | in | 1 | Synchronous reset |
| `go` | in | 1 | Run/pause (1=run, 0=freeze) |
| `emergency` | in | 1 | Emergency override (1=all red, BCD=88) |
| `ew_bcd` | out | 8 | East/West countdown BCD `{tens,ones}` |
| `ns_bcd` | out | 8 | North/South countdown BCD `{tens,ones}` |
| `ew_red` | out | 1 | East/West red |
| `ew_yellow` | out | 1 | East/West yellow (blink) |
| `ew_green` | out | 1 | East/West green |
| `ns_red` | out | 1 | North/South red |
| `ns_yellow` | out | 1 | North/South yellow (blink) |
| `ns_green` | out | 1 | North/South green |

## Source-owned configuration

This example is one zero-parameter module family. Its demonstration constants
are fixed in `traffic_lights_ce_pyc.py`; callers cannot create implicit
specializations through CLI parameters or environment variables.

| Constant | Value | Description |
|-----------|---------|-------------|
| `CLK_FREQ` | 4 | Demonstration clock frequency |
| `EW_GREEN_S` | 3 | East/West green interval |
| `EW_YELLOW_S` | 1 | East/West yellow interval |
| `NS_GREEN_S` | 2 | North/South green interval |
| `NS_YELLOW_S` | 1 | North/South yellow interval |

Derived durations:

- `EW_RED_S = NS_GREEN_S + NS_YELLOW_S`
- `NS_RED_S = EW_GREEN_S + EW_YELLOW_S`

## Build and Run

The source-owned demonstration timing is intentionally short for fast
visualization. The following sequence is verified end-to-end (including all
stimuli):

```bash
PYTHONPATH=python python3 -m pycircuit.cli emit \
  examples/pycircuit/applications/traffic_lights_ce_pyc/traffic_lights_ce_pyc.py \
  -o /tmp/traffic_lights_ce_pyc.pyc

$PYC_TOOLCHAIN_ROOT/bin/pycc /tmp/traffic_lights_ce_pyc.pyc \
  --emit=verilog --out-dir=.pycircuit_out/applications/traffic_lights_ce_pyc/generated

$PYC_TOOLCHAIN_ROOT/bin/pycc /tmp/traffic_lights_ce_pyc.pyc \
  --emit=cpp --out-dir=.pycircuit_out/applications/traffic_lights_ce_pyc/generated

c++ -std=c++20 -O2 -shared -fPIC \
  -I "$PYC_TOOLCHAIN_ROOT/include" \
  -I .pycircuit_out/applications/traffic_lights_ce_pyc/generated \
  .pycircuit_out/applications/traffic_lights_ce_pyc/generated/traffic_lights_ce_pyc.cpp \
  "$PYC_TOOLCHAIN_ROOT/lib/libpyc6_runtime.a" \
  -o .pycircuit_out/applications/traffic_lights_ce_pyc/libtraffic_lights_sim.dylib \
  examples/pycircuit/applications/traffic_lights_ce_pyc/traffic_lights_capi.cpp

python3 examples/pycircuit/applications/traffic_lights_ce_pyc/emulate_traffic_lights.py --stim basic
python3 examples/pycircuit/applications/traffic_lights_ce_pyc/emulate_traffic_lights.py --stim emergency_pulse
python3 examples/pycircuit/applications/traffic_lights_ce_pyc/emulate_traffic_lights.py --stim pause_resume
```

## Stimuli

Stimulus is loaded as an independent module, separate from the DUT.
Available modules live under `examples/pycircuit/applications/traffic_lights_ce_pyc/stimuli/`.

- `basic`: continuous run, no interruptions
- `emergency_pulse`: assert emergency for a window
- `pause_resume`: toggle `go` to pause/resume
