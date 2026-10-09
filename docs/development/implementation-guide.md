# Implementing a design with the current source profile

Use ordinary @module definitions, explicit child instances and nested stateless
@rule functions. Python capture reads syntax without executing the design.
The [language reference](../reference/language.md) specifies supported expressions
and the [frontend guide](agent-frontend-guide.md) provides a complete example.

## Define modules and storage

Declare typed input arguments and a named output-type mapping. A source may
contain several module definitions. Declare children structurally, bind each
complete input signature in a registered rule, and return wires or pure computed
values. Standard dff/dffe/memory leaves own storage; nonlocal persistent writes
belong to the retired frontend. Work reads current state and Xfer commits.

Use explicit masks for modular arithmetic. Integer arithmetic and selection
preserve full intermediate width; only proven declaration boundaries narrow.
Boolean and Integer are distinct, even at one bit. Complete source systems,
collections/records, mathematical comparisons and dynamic checks still require
additional implementation; do not infer authoring support from a hand-written
IR or runtime component.

## Compile, link and emit

```sh
pycircuit compile -c src/child.py --source-root src \
  --package-prefix demo -o /absolute/build/units/child
pycircuit compile -c src/top.py --source-root src \
  --package-prefix demo -I /absolute/build/units/child \
  -o /absolute/build/units/top
pycircuit link /absolute/build/units/child /absolute/build/units/top \
  --top demo.top.Top -o /absolute/build/top.ac
pycircuit emit /absolute/build/top.ac --target cpp -o /absolute/build/cpp
pycircuit emit /absolute/build/top.ac --target verilog -o /absolute/build/verilog
```

Each source is a separate producer. Parent compilation consumes published
interfaces; link receives the explicit complete unit closure. Both emitters use
the same verified final artifact. Use canonical output paths without symlink
components, and keep artifacts outside the source tree.

## Build and run the generated model

Generated CMake exports source-owned pycircuit_modules. The typed C++ pyc_dut
provides drive/sample around the shared SystemRunner; a host driver supplies
complete inputs and clock levels. One Step is a Work/Xfer sampling epoch, and
sample returns the successful Work's old-Q snapshot. Use a finite configuration;
events are silent unless explicitly requested. The port C ABI remains deferred.

The installed PycircuitExamples.cmake helper runs these public commands for
per-design examples. [module_loop](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/module_loop/README.md)
demonstrates two source units; wire_ops, arith and
[counter](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/counter/README.md) preserve their original
hardware behavior. Their independent
native/RTL oracles are part of the design folders. Configure with an installed
CMAKE_PREFIX_PATH, build, then run CTest with --no-tests=error.

## Verify the actual scope

Select focused gates from the changed contract, then the applicable integration
checks. Preserve invalid-input and output-publication protection, state/clock
transaction behavior, Runtime-only/no-LLVM consumption and explicit source-unit
ownership. Source presence or a planned command is not a passing result. Bind
commands, outcomes, reviews and exact candidate hashes under docs/gates/logs.
The [testing guide](testing-and-gates.md) describes the evidence contract; the
historical source-unit cutover page records its original accepted slice rather than current
feature coverage.
