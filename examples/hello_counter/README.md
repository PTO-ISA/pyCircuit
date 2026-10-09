# Hello counter

This source system owns an eight-bit counter and logs its current value.
pyCircuit infers storage, clock, reset and write enables, then generates the
complete native and Verilog simulation artifacts.

With an installed toolchain:

```sh
pycircuit run examples/hello_counter --cycles 5
pycircuit run examples/hello_counter --target verilog --cycles 5
```

Use `--toolchain /path/to/install` when the toolchain is not bundled with the
Python package. Build artifacts stay in `.pycircuit_out/run/hello_counter/`.
The generated binary is `simulation/cpp/bin/pycircuit_sim` (or
`simulation/verilog/bin/pycircuit_sim` for Verilator).

No authored C++ driver, RTL testbench or runtime config is required. Each cycle
has a low and high sampling epoch; the current-value log is therefore
`0, 0, 1, 1, 2, 2, 3, 3, 4, 4` for five cycles. Logs are source observations;
independent framework oracles are separate tests.
