# Engine

This document describes the generic engine module implemented in:

- [engine.hpp](./engine.hpp)
- [engine.cpp](./engine.cpp)

## Purpose

`Engine` is the current dummy execution resource for `SCALAR`, `VEC`, `CUBE`, and `TMA`
backend work.

It models latency and completion, not detailed functional execution.

## Inputs

- `inst_in`
  - type: `SimQueue<PTOInstRef>`
  - producer: `IssueQueue`
  - meaning: ready instructions selected for this engine instance

## Outputs

- `done_out`
  - type: `SimQueue<PTOInstRef>`
  - consumer: `ReadyTable`
  - meaning: completed instruction stream used for wakeup broadcast and ROB completion

## Inner State / Private State

- `kind_`
- `config_`
- `execute_q_`

`execute_q_` is a private `SimQueue<PTOInstRef>` inside the engine. The queue
stores per-op latency on write and only exposes the instruction for completion
once that latency has counted down to zero.

## Owned Storage

The engine owns only its local in-flight queue state.

It does not own boundary interconnect queues; those are owned by `Core`.

## Features

- one logical engine instance per configured execution lane
- per-engine per-op latency configuration from TOML
- one in-flight instruction at a time through a private latency queue
- emits a completion event when latency reaches zero
- emits a completed `PTOInstRef` when latency reaches zero

## PTOInstRef Pop/Update/Push Rule

Engine owns the execution-stage runtime transition.

When engine accepts an instruction:

- pop `PTOInstRef` from `inst_in`
- assign `engine_id`
- look up latency from the engine config based on opcode
- assign runtime latency
- stamp `timestamps.engine_pop_cycle`
- write the shared instruction into `execute_q_` with that latency

When engine completes an instruction:

- wait for the private latency queue to expose the completed instruction
- update the in-flight instruction runtime latency to zero
- stamp `timestamps.engine_complete_cycle`
- push the same `PTOInstRef` to `done_out`
- remove the instruction from the private latency queue

The engine does not directly update ROB entry state. Completion is routed
through `ReadyTable`, which broadcasts wakeup and forwards the same
`PTOInstRef` to ROB completion.

## Cycle Behavior

Per cycle:

1. advance the private latency queue
2. if a completed instruction is visible and output space exists, pop it and
   push it to `done_out`
3. if the private latency queue has space and input is non-empty, pop one
   instruction and enqueue it for execution with its configured latency

## Limitations / Simplifications

- no result data generation
- no bypass network
- the latency model is queue-based and per-op, but still not a full functional
  execution model
- capacity is fixed to one in-flight instruction per engine instance
- completion is returned through a PTOInstRef wakeup queue
- deadlock avoidance is external; `CoreSystem` aborts after sustained no-progress cycles
