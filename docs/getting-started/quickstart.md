# Quickstart

pyCircuit turns a small Python hardware description into C++ and Verilog through
one compiler flow. This complete counter is the smallest stateful example:

```python
from pycircuit import bits, log, rule, system


@rule
def increment(count):
    log("info", "count", count)
    count = count + 1


@system
def HelloCounter():
    count: bits[8] = 0
    increment(count)
```

`count` is persistent eight-bit storage. Its rule logs the current value and
proposes an increment modulo 256. Clock/reset and storage write enables are
compiler-owned, and `@system` selects a closed executable simulation.

## Run

After [installing pyCircuit](installation.md), the checked-in example runs with:

```sh
pycircuit run examples/hello_counter --cycles 5
pycircuit run examples/hello_counter --target verilog --cycles 5
```

The two commands generate their own simulation harnesses. No C++ driver, RTL
testbench or runtime configuration needs to be authored.

## Compile and emit

After [installing pyCircuit](installation.md), save the source as
`hello_counter.py` and run:

```bash
mkdir -p .pycircuit_out/hello_counter/units
pycircuit compile -c hello_counter.py --source-root . \
  --package-prefix hello -o .pycircuit_out/hello_counter/units/hello_counter
pycircuit link .pycircuit_out/hello_counter/units/hello_counter \
  --top hello.hello_counter.HelloCounter \
  -o .pycircuit_out/hello_counter/hello_counter.ac
pycircuit emit .pycircuit_out/hello_counter/hello_counter.ac \
  --target cpp -o .pycircuit_out/hello_counter/cpp
pycircuit emit .pycircuit_out/hello_counter/hello_counter.ac \
  --target verilog -o .pycircuit_out/hello_counter/verilog
```

Compilation publishes one independently reusable source unit. Linking verifies
the complete design, and both emit commands consume that same verified artifact.
Generated files stay in `.pycircuit_out`.

For the complete generated C++ and RTL run, continue with the
[hello counter tutorial](tutorial.md).
