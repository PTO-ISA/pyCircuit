# Nested Enum payload

A `Packet` contains a nested `Header`, a 17-bit payload and a one-bit `matched`
field. `Mode` is a nominal two-bit Enum with IDLE, RUN and WAIT members.

The ordinary typed `classify` helper copies the packet, sets the copy's mode to
RUN, then compares the original mode with WAIT. Hardware records are values:
updating the copy does not change the saved input. Opcode and payload pass
through unchanged. `matched` is payload data, separate from Queue validity.

One `transfer` Rule pops an input, calls the helper, and pushes the result
atomically. The compiler supplies availability/capacity conditions. Two depth-one,
latency-one queues preserve the two-stage transport and full replacement policy:
E0 captures a token, E1 transforms it, and E2 is its earliest consumption.

The external `valid/data/take` handshake is a boundary protocol. Enum carriers
with undeclared codes or X/Z remain data; this design adds no membership filter.
Unchanged fields preserve their value/known/Z bits. The matched comparison can be
unknown when no known input bit establishes equality or inequality.

The 184-cycle System covers ordinary payloads and flow control. Independent
Module and Icarus tests additionally cover invalid Enum codes, X/Z fields,
backpressure, reset and retirement history.

```sh
cmake -S examples/enum_payload_pipeline -B /absolute/build/enum_payload_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/enum_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/enum_payload_pipeline --output-on-failure --no-tests=error
```

[Generated excerpts and verification inputs](GENERATED.md).
