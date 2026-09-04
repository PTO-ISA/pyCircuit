# PYC primitive registries

This directory separates language semantics from replaceable backend
implementations:

- `semantic_registry.json` is the stable, vendor-neutral PYC contract.
- `library/verilog/rtl_catalog.json` contains qualified implementation choices.

Python and canonical PYC may reference only semantic IDs.  Vendor module,
parameter, port, source, digest, provenance, and license data enter IR only in
the Verilog-only `pyc-select-rtl-primitives` pass.
