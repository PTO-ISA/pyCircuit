# Zybo Z7-20 bring-up (minimal LED blink)

This folder adds a tiny, known-good Vivado build that uses the checked-in
pyCircuit-generated `Counter` RTL to blink the Zybo Z7-20 user LEDs.

## What it builds

- Top: `platforms/zybo_z7_20/rtl/zybo_counter_top.sv`
- Generated design: `.pycircuit_out/examples/counter/counter.v`
- Constraints: `platforms/zybo_z7_20/constraints/zybo_z7_20_minimal.xdc`
- Vivado script: `platforms/zybo_z7_20/vivado/build_zybo_counter.tcl`

Behavior on hardware:

- Set `sw[0]=1` to enable counting.
- `led[3:0]` increments about ~2 times per second.
- Hold `btn[0]` to reset (synchronous reset after 2FF sync).

## Build (Windows)

From repo root:

```powershell
vivado -mode batch -source platforms/zybo_z7_20/vivado/build_zybo_counter.tcl
```

Optional (program automatically after build):

```powershell
$env:PYC_PROGRAM=1
vivado -mode batch -source platforms/zybo_z7_20/vivado/build_zybo_counter.tcl
```

The bitstream is written to:

- `.pycircuit_out/platforms/zybo_z7_20/vivado/counter/zybo_counter_top.bit`

## LinxISA CPU bring-up demo (UART + exit MMIO)

This repo also includes a Zybo top that instantiates the pyCircuit-generated
`linx_cpu_pyc` bring-up core and routes UART bytes to a simple PL UART TX.

- Top: `platforms/zybo_z7_20/rtl/zybo_linx_cpu_top.sv`
- Constraints: `platforms/zybo_z7_20/constraints/zybo_z7_20_linx_cpu.xdc`
- Vivado script: `platforms/zybo_z7_20/vivado/build_zybo_linx_cpu.tcl`
- Windows helper: `integrations/linx/flows/tools/windows/zybo_z7_20_linx_cpu_flow.ps1`

Build/program:

```powershell
powershell -ExecutionPolicy Bypass -File integrations\\linx\\flows\\tools\\windows\\zybo_z7_20_linx_cpu_flow.ps1 -Program
```

## Linx PS/PL platform (AXI-Lite monitor-driven bring-up)

For bring-up that uses a Zynq PS app to load programs and stream UART output,
use the PS/PL platform wrappers (in-order + OOO):

- In-order wrapper: `platforms/zybo_z7_20/rtl/linx_platform_inorder_axi.sv`
- OOO wrapper: `platforms/zybo_z7_20/rtl/linx_platform_ooo_axi.sv`
- AXI regs + UART FIFO: `platforms/zybo_z7_20/rtl/linx_platform_regs_axi.sv`
- Constraints: `platforms/zybo_z7_20/constraints/zybo_z7_20_leds_only.xdc`
- Vivado scripts:
  - `platforms/zybo_z7_20/vivado/build_zybo_linx_platform_inorder.tcl`
  - `platforms/zybo_z7_20/vivado/build_zybo_linx_platform_ooo.tcl`
- Windows helper: `integrations/linx/flows/tools/windows/zybo_z7_20_linx_platform_flow.ps1`

Build/program:

```powershell
powershell -ExecutionPolicy Bypass -File integrations\\linx\\flows\\tools\\windows\\zybo_z7_20_linx_platform_flow.ps1 -Core InOrder -Program
```
