# Exact-width bit types

These examples show the `ac.u1` through `ac.u64` unsigned circuit values,
same-width bit operations, and composition into `@ac.struct` payloads.

- [`bit_widths.py`](bit_widths.py) uses non-power-of-two fields and preserves
  their exact widths through ACIR, gfsim, and PYC lowering.
