# DavinciOO contributor designs

Build DavinciOO modules here to exercise and improve pyCircuit. Decision 0222
authorizes this design program under `designs/`; compiler and primitive fixes
remain reusable framework changes. This directory contains planning contracts
and task cards plus the first implemented SPE.IEX and SPE.OOO pilots. It does not claim
that all 240 candidates are implemented.

| Pilot | Current evidence | Remaining promotion gate |
| --- | --- | --- |
| I1 | generated gfsim grant/retry/cancel/backpressure/reset/isolation matrix | H2/H1 integration; stateful PYC/RTL after #22 |
| I2 | generated gfsim operand/dependency/execute/release/cancel/backpressure/reset/isolation matrix | H2/H1 integration; stateful PYC/RTL after #22 |
| WBA | generated gfsim terminal/apply/retry/cancel/drain/backpressure/reset/isolation matrix | H2/H1 integration; stateful PYC/RTL after #22 |
| ROB | generated gfsim capacity/completion/durable-handoff/recovery/isolation matrix and offline replay | NDF traceability review; CMT/W2 and H2/H1 integration; stateful PYC/RTL |

## Start here

- [Contributor tracking issue #51](https://github.com/PTO-ISA/pyCircuit/issues/51)
- [Architecture and NDF/H hierarchy](ARCHITECTURE.md)
- [240 H3 contributor work items](MODULE_CHECKLIST.md)
- [7 H1 and 31 H2 assembly work items](ASSEMBLY_CHECKLIST.md)
- [Initial analysis snapshot and provenance](catalog.json)
- [Contribution rules](AGENTS.md)

NDF **L0 = architectural intent**, **L1 = behavior**, **L2 = microarchitecture**.
Hardware **H1/H2/H3 all belong inside NDF L2**. An H1 domain is not an NDF L1
clause. Cache names L1D/L2C and concrete NDF identities remain unchanged.

| H1 | H2 groups | H3 candidates |
| --- | --- | --- |
| SPE | IFU, OOO, IEX, LSU, BCTRL, DTU | 110 |
| SMT | THR, ARB, RSC, PRD, XTD | 26 |
| TMU | TRN, TRF, BGF, TUL migration names | 21 |
| VEC | VEX, SFU, SHU, TLOP | 23 |
| CUBE | DFL, MMA, ACC, WBK | 18 |
| MEM | MIF, L2C, NOC, COH | 22 |
| GPE | GCU, GMV, GMM, IPF | 20 |

All 240 source candidates are represented. The recommendations currently
include leaves, contained state, interfaces, aliases and unresolved ownership.
They do not supersede unresolved architecture decisions. For example, TMU.TUL
migration work must not create a second scalar T/U owner, and IQ/ISQ/S2/S3
must not independently allocate the same resident state.

## What each contributor receives

Every H3 card contains input and output names, payload/type, meaning and
evidence status; state ownership; required capabilities; expected behavior;
source evidence and open questions; and a closure checklist. Assembly cards
add external-boundary proposals and their complete child candidate list.

- `declared`: the cited external source explicitly names this port. This does
  not prove current compiler support or execution in this design directory.
- `proposed`: a contributor-facing interface suggestion grounded in the role;
  fields, widths, nominal identity and timing still need to be frozen.
- `unresolved`: the source does not settle the interface or its ownership.
- No independent ports: the entry is contained state or a migration alias;
  implement its accepted owner and preserve the mapping rather than inventing
  another mutable component.

`catalog.json` is the initial analysis/provenance snapshot, not an execution
registry or a compiler input. Reviewed module cards, implementations and tests
become the live contracts. Do not interpret the frozen snapshot's initial
execution status as a later implementation assessment.

## Source and test placement

```text
designs/davincioo/
  contracts/                 shared nominal payload and state contracts
  spe/assembly.py            H1 composition when implemented
  spe/ooo/assembly.py        H2 composition when implemented
  spe/ooo/rob.md             H3 contract and contributor checklist
  spe/ooo/rob.py             accepted H3 implementation when contributed
  tests/spe/ooo/             design-local unit and seam tests
  tests/system/              whole-design and ELF integration harnesses
  tools/                    design-local inventory/run/oracle tools
```

These `.py` paths are future implementation locations, not generated stubs.
Contained-state and alias cards must first settle their accepted containing
file/owner. Repeated instances share parameterized definitions; they do not
create copied source modules.

## Use designs to mature framework capabilities

1. Claim an ID, exact file/test ownership, interface task and expected result
   in the tracking issue. Other contributors can work on independent owners.
2. Link L0 intent and L1 observable behavior to the H3/L2 implementation.
3. Freeze ports, state/reset, arbitration, functional branches and recovery.
4. Try the smallest current-checkout compilation and execution case.
5. Reduce a genuine framework gap to a generic failing test; reference the
   existing capability issue or open a focused one. Shared IR legality comes
   before backend behavior. No design-path allowlist or host-side workaround.
6. Merge the framework/primitive fix, then the dependent design PR with
   gfsim expected-result and backpressure/cancellation evidence.
7. Integrate H3 -> H2 -> H1 -> whole core. Add PYC/RTL parity where admitted;
   retain the explicit provisional-Table rejection until its lowering closes.

The first IEX pilots use the merged #46 optional-output and #48 recursive
equality/invariant capabilities. Banked state, multi-selection, alias proofs
and stateful PYC/RTL map to the remaining capability roadmap in #50. A module
with verified prerequisites can proceed independently of unrelated work.

Recommended integration order is shared identities and reference-behavior
litmus tests; transport and context pilots; SPE fetch/rename/issue/commit;
scalar memory and raw Tile banks; numeric engines; group rendezvous; then
whole-core live-ELF execution. A pre-recorded trace is a local oracle/input
fixture, not a substitute for the final live fetch/decode execution path.

## Check the inventory

```bash
python3 designs/davincioo/tools/check_catalog.py
```

The checker verifies exact source-candidate coverage, H1/H2/H3 and NDF axis
separation, port evidence metadata, card links and provenance bounds. It does
not simulate hardware or promote a proposed contract. Module PRs must provide
their own executable evidence and keep the card's closure state accurate.
