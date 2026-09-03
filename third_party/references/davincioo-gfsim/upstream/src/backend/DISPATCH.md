# Dispatch

This document describes the dispatch module implemented in:

- [dispatch.hpp](./dispatch.hpp)
- [dispatch.cpp](./dispatch.cpp)

## Purpose

`Dispatch` routes backend-issued `PTOInst` objects to engine-specific input
queues based on the typed `engine_kind` field.

## Inputs

- `inst_in`
  - type: `SimQueue<PTOInstRef>`
  - producer: `Rename`
  - meaning: issued and renamed instruction stream leaving the rename stage

## Outputs

Boundary outputs are all `SimQueue<PTOInstRef>` queues owned by the parent
`Core` topology and consumed by `ReadyTable`.

- VEC routed queues
- CUBE routed queues
- TMA routed queues
- SCALAR routed queues

The output index space is contiguous:

1. SCALAR outputs
2. VEC outputs
3. CUBE outputs
4. TMA outputs

## Inner State / Private State

- round-robin cursor for VEC outputs
- round-robin cursor for CUBE outputs
- round-robin cursor for TMA outputs
- configured engine counts per family

## Owned Storage

This module owns no physical queue storage.

Queue storage is owned by `Core`.

## Features

- consumes one instruction at a time from the dispatch input queue
- checks `PTOInst.engine_kind`
- routes `VEC` instructions to VEC engine queues
- routes `CUBE` instructions to CUBE engine queues
- routes `TMA` instructions to TMA engine queues
- routes fallback or scalar instructions to SCALAR engine queues
  for later ready-table initialization and issue-queue residency
- uses round-robin selection across configured engines of the same kind

## PTOInstRef Pop/Update/Push Rule

Dispatch owns the routing-stage runtime transition.

When dispatch moves an instruction:

- pop `PTOInstRef` from `inst_in`
- inspect `engine_kind`
- set:
  - `dispatched = true`
  - source-readiness runtime state
  - `timestamps.dispatch_cycle`
- preserve rename-resolved tile tags already written onto the instruction
- push the same `PTOInstRef` to the selected engine queue

There is no `NONE` engine lane in the current topology. Fallback or
otherwise-unclassified ops route to the `SCALAR` engine family.

## Cycle Behavior

Per cycle:

1. inspect the input queue head
2. select the target engine family
3. if the selected output queue has space, pop the input instruction
4. mark it dispatched
5. push it to the chosen engine queue

## Limitations / Simplifications

- only one instruction is dispatched per cycle
- dispatch does not model arbitration cost beyond queue fullness
- no fairness metrics or structural hazard reporting yet
