# Ordered mask decoder

The fixed table contains three rules: mask 0xF0 with matches 0x10,
0x20 and 0x30, returning op 1, 2 or 3 and length 4. The default is op0/length0.
Later matching rules select over earlier results. This implementation retains that
ordered mask/equality/selection chain and the original four-bit op and three-bit
length outputs. The root has no public configuration parameters or state.

The source uses an `ac.u8` input and returns the nominal
`DecodeResult(op: ac.u4, len: ac.u3)` in one physical `result` packet.
Ordinary local names share the mask and match predicates. Pure conditional
expressions build each output's ordered selection chain before constructing
the result value. These values and local names allocate no storage.

```sh
cmake -S examples/decode_rules -B /absolute/build/decode-rules -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/decode-rules --parallel 4
ctest --test-dir /absolute/build/decode-rules --output-on-failure --no-tests=error
```

Immutable local names share existing SSA expressions; they do not allocate
registers or capture values at construction. The independent oracle preserves
0x10 -> op1/length4, checks all 256 instructions and both outputs, and uses
`PYC_DECODE_FOUR_STATE` for complete Icarus X/Z checks. Low-nibble X/Z bits are
masked away. Uncertain high matches use ordinary four-state equality and mux
merging, without inferring correlations between separate predicates.
See [the generated-output guide](GENERATED.md) for the captured source-owned
IR and actual generated C++ and RTL.

## Generated system usage

`bench.py` exports `example_decode_rules.bench.ExerciseDecodeRules`. Compile sources in
order `decode_rules.py`, `bench.py`, then link that system root and emit C++ or
Verilog through the public `pycircuit compile`, `link`, and `emit` commands.
Run `pycircuit run examples/decode_rules --target cpp --cycles 257` or select
`--target verilog`. Each managed cycle checks one original known-input row
in both sampling epochs; all 257 original rows are represented.

The original `driver.cpp`, `rtl_tb.sv`, configuration, and their independent
oracles remain intact. This source bench covers the complete known-input table;
host X/Z construction and recovery checks, where present, remain in those
retained native/RTL oracles and are not claimed by the generated system run.
