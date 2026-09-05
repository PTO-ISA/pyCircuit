# Qualified PYC population-count evidence

Date: 2026-09-05

Decision: 0164

Scope: vendor-neutral `pyc.popcount`, structural and Cycle-Aware Python,
Agentic ACIR/QueueGraph lowering, C++ and gfsim reference semantics, and
Verilog-only selection of repository-owned BSD-3-Clause RTL.

## Results

| Lane | Result |
| --- | --- |
| pyCircuit/Agentic repository unit lane | PASS: 20 tests |
| Primitive selection/verifier/C++/RTL/manifest system tests | PASS: 11 tests |
| Standalone ACIR/ACSim lit, including Agentic popcount-to-PYC | PASS: 140 tests |
| QueueGraph code-generation C++ tests | PASS: 96 tests |
| gfsim exact-width helper and SimQueue block | PASS: 251 tests total |
| Decision status | PASS: 164 rows, 0 deferred |
| Repository contracts, API hygiene, and MkDocs strict | PASS |

## Proved contracts

- `pyc.popcount` returns `max(1,ceil(log2(N+1)))` bits and carries no
  implementation identity in canonical PYC.
- Structural and Cycle-Aware pyCircuit APIs infer parameters from the input
  type and preserve the Cycle-Aware input cycle.
- Agentic `ac.var.popcount` lowers to one semantic PYC op; QueueGraph C++ uses
  the shared typed gfsim reference, and `Popcount<Width>` is a SimQueue block.
- Agentic direct JIT includes the same typed gfsim helper and executes a
  representative 13-bit value through the generated queue model. The canonical
  Agentic ACIR→PYC→pycc path performs catalog selection and emits a closed
  Verilog source set that passes Verilator; the compatibility Verilog emitter
  no longer hard-codes a popcount implementation.
- The Verilog selection pass binds `WIDTH` and `COUNT_WIDTH`, verifies the BSD
  source digest, records the implementation/binding manifest, and emits the
  minimal source closure.
- Widths 1, 4, 13, and 64 pass RTL/reference checks. The PYC C++ and selected
  RTL paths agree on zero, mixed, and all-ones inputs.
- The qualified RTL uses an explicitly padded balanced adder tree, and the
  direct semantic Verilog emitter constructs the same balanced expression.
  Structural regression checks reject the former accumulator chain. Yosys was
  unavailable locally, so no synthesis-depth report is claimed.
- PR #29's Solderpad-licensed BaseJump source is not imported or relicensed.
