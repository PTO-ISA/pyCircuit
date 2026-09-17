# Bitfield RTL Primitives

This directory splits the previous monolithic wrapping-bitfield unit into
smaller composable RTL blocks.

Recommended composition:

```text
value + bit_width + bit_offset
            |
            v
pyc_wrapping_field_normalize
      |              |
    field            mask
      |               |
      |         +-----+------+
      |         |            |
      |       CLEAR         SET
      |
      +--> dynamic_sign_extend   (BXS / signed extract)
      +--> popcount              (BCNT)
      +--> zero_count            (CLZ / CTZ)
      +--> reverse_bytes         (REV)

value + source + mask + width + offset
      |
      +--> bitfield_insert       (BFI)
```

Files:

- `pyc_wrapping_field_normalize.sv`
- `pyc_bitfield_clear.sv`
- `pyc_bitfield_set.sv`
- `pyc_bitfield_insert.sv`
- `pyc_reverse_bytes.sv`
- `pyc_dynamic_sign_extend.sv`
- `pyc_popcount_primitive.sv`
- `pyc_runtime_zero_count.sv`

Notes:

- `field` is packed into the low bits.
- `mask` marks selected positions in the original `value`.
- `pyc_runtime_zero_count` uses a runtime direction port so one physical
  datapath can implement both CLZ and CTZ.
- These are low-level RTL building blocks. Compiler/PYC semantic operations
  do not need to map 1:1 to every file.
