# U01 transport portability

The pure unit lane contains no subprocess call, `MLIR_OPT` lookup, toolchain lookup, or `mlir-opt` path. It passed 13/13 with `MLIR_OPT` and `PYC_TOOLCHAIN_ROOT` unset and `PATH=/usr/bin:/bin`.

The configured system lane resolves `mlir-opt` from `MLIR_OPT`, then `PYC_TOOLCHAIN_ROOT/bin`, then `PATH`. On this candidate it ran explicitly with `/opt/homebrew/opt/llvm/bin/mlir-opt`, confirmed LLVM 22.1.8, parsed four representative transports, and passed 4/4 with zero skips.

Pure coverage includes one-file/root capture, no sibling scan/import/model execution, no old semantic-compiler call, independent `Decimal` oracle for a 6021-digit decimal generated from a huge hexadecimal literal without changing Python's global digit limit, distinct bool/integer/float/complex/bytes/ellipsis tags, MLIR byte escaping, LF/CRLF normalization, U+2028/U+0085 physical-line spans, inherited helper-node spans, and malformed located-node rejection.

The broader unit run reported 265 passed and one unrelated current-candidate failure: `tests/unit/test_primitive_catalog.py::test_acir_semantic_registry_separates_semantics_from_implementations` cannot find `ac.reservation_set` after the concurrent native U01 ODS reduction. This is retained as a native candidate integration failure, not hidden or attributed to transport portability.

This evidence proves transport serialization and upstream parser acceptance only. It does not prove the native Packet schema, source-unit importer, header registry, linking, or C3 publication.
