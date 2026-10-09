# Simulation

Generated C++ sources build as `pycircuit_modules` and link the shared Runtime.
Runtime-only consumers do not require LLVM; CompilerDev requires LLVM/MLIR
22.1.8. The generated typed `pyc_dut` exposes inputs and outputs to a host driver.

The host uses the shared SystemRunner, owns stimuli and explicit clock/reset
levels, and supplies a finite run limit. A Step samples the design during Work
and then commits successful proposals in Xfer. `sample()` returns that Work
result; it does not reevaluate outputs after the commit. The following Work
sees the new state. Failed whole-system checks discard pending state and clock
history before any commit.

C++ and Verilog use the same final IR, source ownership and standard storage
semantics. Verilator validates supported two-state RTL behavior. Native
four-state checks and applicable Icarus tests provide separate X/Z evidence.
Independent reference models live under tests, outside the DUT and compiler.

Start with the [counter tutorial](../getting-started/tutorial.md). Use
[testing and gates](../development/testing-and-gates.md) for the API/example
entrypoints and nightly matrices.
