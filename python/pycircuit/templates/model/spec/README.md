# Project specification workflow

The L0/L1 design input, L2 implementation specification and L3 acceptance criteria
are NDF 0.2 drafts. They describe the supplied telemetry starter. Change them with
the implementation, independently derived vectors and verification owners.
Rename `telemetry-model` in ndf.yaml and document owners for a new project;
choose a stable clause prefix before publishing the first revision.

Build the external NDF CLI at the exact revision in ndf.lock using its locked
Cargo workflow, and place its checkout at `.pycircuit_out/ndf`. pyCircuit does
not fetch it automatically. From the project root, use a new output directory:

```sh
pycircuit spec-check . --ndf .pycircuit_out/ndf/target/release/ndf \
  --output .pycircuit_out/spec-check
python3 verification/verify.py --build-dir ../.pycircuit_out/telemetry-verification
```

`spec-check` retains standard NDF build/check/diff/coverage/trace evidence and
resolves `implements`/`verifies` comments to actual Python definitions.
Traceability PASS does not approve the draft or establish behavioral correctness.
With all requirements draft, native active-L1 coverage is N/A. The separate
verification command checks installed generated C++ and RTL against supplied
vectors; it needs PYC_TOOLCHAIN_ROOT and the tools reported by `pycircuit doctor --rtl`.
Extend the acceptance suite for the design before claiming complete conformance.
