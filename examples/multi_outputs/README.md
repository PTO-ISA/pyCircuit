# Independent module outputs

`Decode` returns `tuple[Header, Payload, ac.u1]`. These are three independent
outputs of one module instance. The two Struct outputs carry different nominal
types; `valid` is a scalar output. The caller unpacks them directly:

```python
header, payload, valid = Decode(word, enable)
tag, = HeaderConsumer(header)
data, = PayloadConsumer(payload)
```

The consumers are separate child instances. Each tuple position becomes one
`ac.instance` result; no tuple operation, wrapper Struct, register or additional
cycle is introduced. A singleton output tuple uses a trailing comma. A genuine
Struct payload remains one result, even though it has fields.

This example deliberately sends Header and Payload to separate consumers. When
several fields instead travel together to one downstream interface, prefer a
single named Struct output and matching Struct input, passed directly between
the modules. Tuple returns describe independent interfaces, not a requirement to
split every field into a port. See the [authoring standard](../STANDARD.md).

`GroupedOutputs` demonstrates that shared-interface case: `DecodePacket` returns
one `Decoded` Struct containing the header, payload and valid field, and
`PacketConsumer` receives that whole value as one input. The System checks both
forms against the same literal expected results.

`HeaderConsumer` increments the low four bits modulo 16. `PayloadConsumer`
complements the eight-bit word. `valid` forwards `enable` independently and does
not suppress data. All three paths are combinational.

## Run the generated System

```sh
pycircuit run examples/multi_outputs --target cpp --cycles 8
pycircuit run examples/multi_outputs --target verilog --cycles 8
```

`bench.py` contains eight literal input/expected-output cases and six source
assertions. Its phase counter supplies the stimulus; the DUT contains no state.
The compiler generates the clocks, reset and both simulation harnesses. The
System observes every case on a low/high pair: eight cycles, sixteen sampling
epochs. This example exercises known data; the compiler's direct-call regression
separately checks four-state transport and stateful multi-output modules.

To run the independent worker/backend comparison with an installed toolchain:

```sh
cmake -S examples/multi_outputs -B /absolute/build/multi_outputs -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/multi_outputs --parallel 2
ctest --test-dir /absolute/build/multi_outputs --output-on-failure --no-tests=error
```

The shared helper compiles `multi_outputs.py` and `bench.py` independently,
links their explicit closure, and emits both targets. The System test compares
C++ workers 1/2 with RTL, checks the full duration and source assertions, and
writes its execution/verification receipts in the build's `system-verification/`
directory. Generated outputs remain outside tracked source.
