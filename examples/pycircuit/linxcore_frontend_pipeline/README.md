# LinxCore domain-aware frontend/OOO prototype

This directory contains a bounded pyCircuit implementation of the production
LinxCore `F0..F4/IB/D1..D3` ownership chain. It is an optimizer and authoring
experiment, not a replacement for the superproject's production
`rtl/LinxCore` implementation.

## Implemented boundary

```text
F0 -> F1 -> F2 -> F3 -> F4/IB -> D1 -> D2 -> D3 -> S1-ready
```

The production IFU has parallel `I-F0..I-F4` and `B-F0..B-F4` pipelines. This
prototype implements the I-SIDE transport and carries the final prediction
record as sidebands; the B-SIDE predictor remains external and joins at F4/IB.

Each arrow is an elastic, single-entry ready/valid register boundary. Payload
and `valid` remain stable under downstream backpressure, and `flush` clears all
frontend/decode residency together.

| Stage | Prototype ownership | Production owner left external |
|---|---|---|
| F0 | PC, STID, fetch UID/sequence, checkpoint, epoch | restart arbitration and per-thread PC policy |
| F1 | atomic lookup launch gate | ITLB and L1I arrays/queues |
| F2 | exact result acceptance gate | PTW, miss table, refill identity checks |
| F3 | dense 2/4/6/8-byte split of a 16-byte window | cross-line carry and ordered line-context queue |
| F4/IB | retained final predecode/prediction sidebands | full multi-entry, per-STID Instruction Buffer and B-SIDE predictor |
| D1 | dense decode mask and demand count | generated opcode recipes, fusion history, CTU diversion |
| D2 | virtual RID/group count and tail-epoch snapshot | complete resource preview and PTag staging FIFOs |
| D3 | provisional reservation token | ROB/BROB/PC/rename/IQ physical owners and S1 atomic publication |

## Why the domain-aware form matters

`domain.next()` records the architectural temporal coordinate, while explicit
`domain.signal()` state records actual residency. The distinction is essential:
advancing the logical domain does not by itself implement ready/valid holding,
flush, replay, or resource ownership. Those are real state transitions and are
therefore visible in emitted MLIR as `pyc.reg` cut points.

This gives an optimizer two complementary facts:

1. cycle provenance for feed-forward alignment and legal retiming;
2. non-retimable transaction boundaries for backpressure, recovery, and
   physical allocation.

An optimizer may balance or retime pure F3/D1/D2 combinational work between
explicit cuts, but it must not move D2 preview across D3 reservation or collapse
F4/IB residency into D1.

The relevant pyCircuit decision contracts are Decision 0119 (logic-depth
analysis), Decision 0123 (MLIR combinational-cycle verification), Decision
0127 (registers are explicit dependency-graph cut points), and Decision 0128
(feedback must use explicit state). The prototype applies those existing
contracts; it does not introduce a new language semantic.

## Narrow verification

```bash
PYTHONPATH=python/pycircuit/src python3 -m pytest -q \
  tests/unit/test_linxcore_frontend_pipeline.py

PYTHONPATH=python/pycircuit/src python3 -m pycircuit.cli build \
  examples/pycircuit/linxcore_frontend_pipeline/tb_linxcore_frontend_pipeline.py \
  --out-dir /tmp/linxcore-frontend-pipeline \
  --target both --jobs 1 --logic-depth 256 --run-verilator
```
