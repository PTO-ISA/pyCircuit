# BaseJump priority-encode ACIR → PYC IR → Verilog demo

Run the complete flow from the repository root (the script uses the checked-in
Python frontend and the single-queue native backend):

```text
python examples/agentic-circuit/blocks/priority_encode_runtime/run_demo.py
```

The command writes its generated ACIR, canonical PYC IR, C++/gfsim source,
Verilog and gate report under
`.pycircuit_out/examples/priority_encode_runtime/`.  Set
`PYCIRCUIT_DEMO_OUT` to choose another disposable output directory.  The ACIR
operation carries the primitive declaration and provenance attributes; the
canonical PYC operation carries the same IDs.  The Verilog backend embeds the
wrapper and its BaseJump dependency closure, so the generated file is
self-contained for Verilator and Yosys.

The packed result is `{valid,index}`: bit 3 is `valid`, bits 2:0 are the
selected input position.  `lo_to_hi=true` selects the lowest set bit.

## Parameterized configurations

The frontend accepts compile-time `lo_to_hi` and infers the result width from
the annotated input payload.  Two configurations can be generated and gated
with:

```text
python examples/agentic-circuit/blocks/priority_encode_runtime/run_variants.py
```

The checked-in example contains:

| configuration | input | direction | result | selected bit |
| --- | ---: | --- | ---: | ---: |
| `w8_lo_to_hi` | 8 bits | low → high | 4 bits | bit 3 of `0x28` |
| `w16_hi_to_lo` | 16 bits | high → low | 5 bits | bit 5 of `0x28` |

Each configuration gets its own ACIR, PYC IR, gfsim C++, generated Verilog and
Verilator/Yosys report under
`.pycircuit_out/examples/priority_encode_runtime/variants/<configuration>/`.
