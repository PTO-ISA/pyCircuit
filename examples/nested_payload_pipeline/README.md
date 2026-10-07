# Nested payload mode update

## Payload behavior

Packet contains Header(opcode6, mode3) followed by payload17, totaling26 bits.
The rule copies the input and sets only `result.header.mode = item.header.mode + 1`.
Mode wraps modulo8, including 7 -> 0; opcode and payload remain unchanged.
The new source declares Header before Packet and uses ordinary immutable record
copy/update rather than the retired with_fields API. The physical record layout
and every original field are preserved.

`NestedPayloadPipeline(valid, data, take)` returns typed ready/valid/data:
26 payload bits and 28 total result bits. Original native UInt arithmetic was
two-state. Historical RTL and current fixed-bit arithmetic produce X throughout
the arithmetic result if an operand contains X/Z; Z does not survive addition.
For the nested root, untouched opcode/payload preserve their complete raw planes
and unknown copied fields do not contaminate a known mode increment. Tests must
not constrain the value plane of an unknown computed result.

## Timing and ownership

Exactly two depth1/latency1 queues retain the original typed input and rule
result. Explicit `downstream_pop` preserves full replacement; head-read latency
is zero and empty flow is disabled. The returned sink adds no third queue.
E0 captures input, E1 commits the transformed token, and E2 is its earliest
consumption. All Work outputs use old committed state. Held clocks do not
transfer; stalls fill at most two slots; rising reset clears both queues.
Clock/reset and proposals remain absent from Python authoring.

## Build and verification

```sh
cmake -S examples/nested_payload_pipeline -B /absolute/build/nested_payload_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/nested_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/nested_payload_pipeline --output-on-failure --no-tests=error
```

The two CTest gates execute actual generated native workers 1/2, Verilator and
genuine Icarus. They check 1121 known Work rows and 281 native/Icarus four-state
Work rows, including wraparound, zero/all-X tokens, stalls/full replacement,
changed offers, active held high/low clocks, reset and drain. Scalar cases cover
all256 byte values; nested cases cover all64 opcode values and all8 modes plus
walking input bits. Per-bit and dense X/Z patterns distinguish computed unknown
arithmetic from exact copied value/known/Z planes. All runners are finite.