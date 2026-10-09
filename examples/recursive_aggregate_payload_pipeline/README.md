# Recursive aggregate payload update

The 13-bit Packet preserves the original ordered fields: Tagged(Mode1, value3),
Nested(Header(code3), value3), a real two-element Table of nominal Mode values,
and flag1. The update sets the tagged Mode to RUN, increments header.code modulo
8, swaps the two Modes, and derives flag from the original first Mode being IDLE.
Tagged.value and Nested.value remain unchanged. Ordered nominal Structs retain
the historical tuple positions without claiming a general tuple authoring API.

Two depth-one queues use downstream_pop replacement. E0 captures, E1 transfers
the transformation, and E2 consumes it. All Work reads use old committed state.
Held clocks cannot transfer; a rising reset discards both occupied tokens.

Both original execution owners at revision
b3e52fb22061350d61b23089c8bed36913650245 use packed 2962 → packed 7125:
tagged(IDLE,5) → (RUN,5), nested(code6,2) → (code7,2), Modes(IDLE,RUN) →
(RUN,IDLE), flag0 → flag1. The native test_queue_codegen.py history takes five
Work/Xfer ticks and retires exactly one result. The test_pyc_backend.py history
uses ten cycles, reset at 0, injection at 1, take always high, and the sole
post-edge output at cycle 2. The active sampler reevaluates at the low clock
level after the rising commit to expose that state without another transfer.

New independent coverage spans all eight header codes, all eight nested peer
values and all four Mode pairs, with varied tagged values/Modes and incoming
flags. Walking bits, header wrap, changed stalled offers, full replacement,
held clocks, reset discard and drain supplement the originals. The host and
Verilator check 615 known Work rows; native workers 1/2 and genuine Icarus
check 209 four-state Work rows. Per-bit and dense X/Z patterns distinguish exact
copied value/known/Z planes from the computed header and comparison flag.
Header arithmetic containing X/Z becomes all X; an unknown original Mode 0 makes
the flag X. Only those computed unknown latent values are unconstrained.

The separate bench.py system has 48 literal stimulus/expected phases derived from
an independent two-slot model. It preserves the original payload and adds known
wrap/stall/drain checks. Physical clocks, reset and four-state scenarios remain
in the host and RTL test assets.

```sh
cmake -S examples/recursive_aggregate_payload_pipeline -B /absolute/build/recursive_aggregate_payload_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/recursive_aggregate_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/recursive_aggregate_payload_pipeline --output-on-failure --no-tests=error
```

These assets specify required checks; execution evidence is recorded separately.
