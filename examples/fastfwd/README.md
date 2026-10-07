# Four-lane packet forwarding

[Generated MLIR, C++ and RTL](GENERATED.md)

This example implements the stateless Fastfwd stub. Four lane inputs each
contain `valid: ac.u1`, `data: ac.bits[128]` and `control: ac.u5`; four engine
inputs contain the same valid/data pair. Every lane and engine forwards valid
and all 128 data bits in the same Work, including data whose valid bit is zero.
Lane control is unused. Backpressure, engine latency, datapath-valid and
128-bit datapath-data outputs are always known zero.

The source uses typed nested structs, recursive zero defaults and direct calls
to reusable `ForwardLane` and `ForwardEngine` modules. The top result has four
`Channel` fields, four `EngineResult` fields and a backpressure bit. No state or
clock is needed for this combinational behavior.

```sh
cmake -S examples/fastfwd -B /absolute/build/fastfwd -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/fastfwd --parallel 4
ctest --test-dir /absolute/build/fastfwd --output-on-failure --no-tests=error
```

The shared gate runs generated native models with one and two workers and an
independent RTL oracle. `PYC_FASTFWD_FOUR_STATE` enables genuine Icarus X/Z
checks; native checks also verify all value/known/Z planes and immediate known
recovery. The original public scalar port meanings map one-to-one to the fields
above; typed inputs/results group them for Python authoring.
