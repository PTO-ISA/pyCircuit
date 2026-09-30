# Source-owned counter design

Each Python file has one compile producer. The root and child share register
references; repeated child instances own distinct hidden state. Rules propose
next values; the system commits once per successful cycle.

With the installed prefix in `CMAKE_PREFIX_PATH`:

```sh
cmake -S examples/pycircuit/counter -B .pycircuit_out/counter -G Ninja
cmake --build .pycircuit_out/counter -j 4
cmake -S .pycircuit_out/counter/cpp -B .pycircuit_out/counter/run -G Ninja
cmake --build .pycircuit_out/counter/run -j 4
.pycircuit_out/counter/run/pycircuit_system --config examples/pycircuit/counter/config.json --events -
```

Configure `verilog` instead of `cpp` to use the same final design with Verilator.
The example has a portless module root and empty static arguments. It does not
claim full testbench `@system` support.

Ninja clean removes the declared unit depfiles and generated payload inventory;
publication control locks remain. Rebuilding after clean uses the same public
commands. Keep model build directories outside the generated bundle directories.
Compiler/helper/Python/config files in a full prefix are build dependencies.
For an immutable wheel prefix whose sole Python package sits outside the prefix,
replace the wheel by reinstalling and reconfiguring the project.
