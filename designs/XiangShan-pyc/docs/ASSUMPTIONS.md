# Design Assumptions — XiangShan-pyc

This file records all design assumptions, inferred decisions, and known
conflicts encountered during the PyCircuit V6 reimplementation.

## General assumptions

| ID | Assumption | Rationale |
|----|-----------|-----------|
| A-001 | All modules use a single clock domain initially | KunMingHu reference is single-clock; multi-clock can be added later |
| A-002 | Reset is synchronous, active-high after `pyc_reset_active()` | Matches PyCircuit V6 convention |
| A-003 | Parameters are hardcoded to KunMingHu defaults first | Per plan: parameterization deferred until core logic is correct |
| A-004 | XLEN = 64, VLEN = 128, ELEN = 64 | KunMingHu default RV64GCV configuration |
| A-005 | FetchBlockSize = 64 bytes (32 RVC instructions or 16 RVI) | Default from `FrontendParameters` |
| A-006 | DecodeWidth = RenameWidth = CommitWidth = 8 | Default from `XSCoreParameters` |
| A-007 | Valid/Ready handshake uses explicit `_valid`/`_ready`/`_bits` port triples | PyCircuit V6 has no built-in Decoupled; we use explicit ports |

## Clock / reset

| ID | Assumption | Source |
|----|-----------|--------|
| A-010 | Every `build_*` function receives `(m, domain)` and derives `clk`/`rst` from `domain.clock_domain` | PyCircuit V6 convention |
| A-011 | No gated clocks in initial implementation | Simplification; can add `EnableClockGate` later |

## Bus protocols

| ID | Assumption | Source |
|----|-----------|--------|
| A-020 | TileLink channel widths follow CoupledL2 defaults | Extracted from reference `Parameters.scala` |
| A-021 | AXI4 used only at SoC boundary (XSTop outward) | XiangShan architecture |

## Known conflicts / open questions

| ID | Description | Status |
|----|------------|--------|
| C-001 | Some figures referenced in XiangShan-doc are missing (e.g. `frontend.png`, `nanhu-memblock.png`) | Non-blocking: use text descriptions |
