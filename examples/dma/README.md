# DMA between two memories

This example copies a complete typed request from a DRAM controller into an SRAM
controller. The DRAM read response carries both addresses and the tag into the
SRAM write. The copy response reports the old destination value, and a separate
readback confirms the new value.

## Design

`Dram`, `Sram`, and `Dma` are ordinary hierarchical Modules. Each controller
owns its memory, controller state, and response Queues. The parent Rule performs
the cross-Module operation
`sram.enqueue_copy(dram.finish_read())`; the DRAM release and SRAM Queue push
therefore form one atomic transaction. `finish_read` and `enqueue_copy` are
callable Methods; controller service and parent forwarding Rules activate from
their declarations. A shared response Method computes the current DRAM response
for both the output callback and completion operations, so no callback captures
another local Rule's result.

The design retains seven Queues and two independent 16×u16 synchronous memories:

| Controller | Operation delay | Priority |
| --- | ---: | --- |
| DRAM | 3 rising edges | copy read before seed write |
| SRAM | 2 rising edges | check read before copy write |

All Queue resource admission is compiler owned. The source saves committed
`busy`, `remaining`, and `reader` snapshots for the controller algorithm; it does
not reproduce Queue full/empty guards in the Rule bodies. A release edge cannot
also accept another memory request.

## Short System scenario

`bench.py` is a 42-cycle teaching scenario with literal independent expected
values. It performs two complete sequences:

1. seed DRAM address5 with `0x1234`, copy it to SRAM address3, and read it back;
2. seed DRAM address9 with `0xbeef`, copy it to the same SRAM address3, and verify
   that the copy response reports the previous `0x1234` before readback reports
   `0xbeef`.

The first copy response and first readback response are deliberately stalled and
then released. Assertions check request acceptance, response enqueue events,
response validity, tags, the zero initial RAM values, the nonzero old SRAM value,
and both final readbacks. Expected values are authored literals; they are not
computed from DUT outputs.

One sequential System Rule checks the current scenario, logs the visible
response state, and advances the phase. The 42-cycle System passes generated C++
workers1/2 and Verilator together with the standalone Module oracle.

The earlier 582-cycle Table bench is preserved byte-for-byte at
`tests/fixtures/historical_examples/system_benches/dma/bench.py`. It is a
historical regression fixture, not a passing result for this short scenario and
not part of the recommended example flow. Passing the short System does not claim
that the retired 582-cycle transaction plan now fits the compiler's fixed limits.

## Independent Module verification

The retained handwritten Module driver remains the full physical oracle. Native
workers1/2 and Verilator cover 1,167 Work samples and581 rising edges across both
RAMs and all seven Queues, including capacity, priority, long stalls, concurrent
service, reset, Discard/retry, and unknown-control rejection. Of83 accepted
inputs,70 retire and13 are canceled by reset. That oracle and the DUT source are
unchanged by the short System rewrite.

The Module oracle is intentionally stronger than the tutorial System. The short
System demonstrates the end-to-end function without duplicating the saturation,
reset, fault, and conservation matrices already owned by the Module test.

## Build and run

```sh
cmake -S examples/dma -B /absolute/build/dma -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/dma --parallel 4
ctest --test-dir /absolute/build/dma --output-on-failure --no-tests=error
```

The public flow independently compiles `dma.py` and `bench.py`, links their
explicit closure, and emits C++ and Verilog from the same verified final ACIR.
