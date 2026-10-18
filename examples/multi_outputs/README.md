# Independent scalar outputs and a shared structured interface

`Decode` returns `tuple[ac.u4, ac.u8, ac.u1]`: three independent values from one
Module instance. Each consumer takes and returns its scalar directly:

```python
tag, word, valid = Decode(word, enable)
tag = HeaderConsumer(tag)
data = PayloadConsumer(word)
```

The consumer calls create distinct child instances. No one-field Struct or
singleton tuple is needed. `HeaderConsumer` increments the four-bit tag modulo
16; `PayloadConsumer` complements the eight-bit word. `valid` forwards `enable`
independently and does not suppress data. These functions are combinational.

`GroupedOutputs` demonstrates a different connection: the tag, word and valid
fields travel together to one consumer. `DecodePacket` returns one `Decoded`
Struct with those three fields, and `PacketConsumer` accepts it as one argument.
The parent passes the value directly instead of unpacking and rebuilding it.

The same eight-case independent oracle checks both interface styles. Interface
grouping follows the connection's meaning: scalar for one value, Struct for
related fields sent together, tuple for independent outputs. See the
[authoring standard](../STANDARD.md).

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

Single values use direct scalar parameters and results. Only `Decoded` groups
multiple fields; no one-field Struct or singleton tuple is needed. The same
independent eight-case oracle checks both interface styles.
