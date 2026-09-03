# IssueQueue

This document describes the distributed issue-queue module implemented in:

- [issue_queue.hpp](./issue_queue.hpp)
- [issue_queue.cpp](./issue_queue.cpp)

## Purpose

`IssueQueue` is the per-engine distributed issue buffer between `ReadyTable`
and `Engine`.

It holds resident `PTOInstRef` entries, tracks per-input valid/ready state on
the instruction object itself, consumes wakeup broadcasts, and only issues a
ready instruction into the target engine.

## Inputs

- `enqueue_in`
  - type: `SimQueue<PTOInstRef>`
  - producer: `ReadyTable`
  - meaning: instructions already annotated with current ready-table state
- `wakeup_in`
  - type: `SimQueue<PTOInstRef>`
  - producer: `ReadyTable`
  - meaning: completed instructions whose output tile tags may wake resident entries

## Outputs

- `issued_out`
  - type: `SimQueue<PTOInstRef>`
  - consumer: `Engine`
  - meaning: ready instructions selected for execution

## Inner State / Private State

- resident `entries_`
- configured depth
- engine kind

## Owned Storage

This module owns only its internal resident entry deque.

Boundary queue storage is owned by `Core`.

## Features

- one logical issue queue per engine lane
- resident `PTOInstRef` storage
- per-source valid/ready tracking through the instruction object
- wakeup by matching completed output tile tags against waiting input tile tags
- oldest-ready-first pick by `rob_id`

## PTOInstRef Pop/Update/Push Rule

IssueQueue owns the issue-stage runtime transition.

When it consumes an enqueue:

- pop `PTOInstRef` from `enqueue_in`
- preserve the ready state initialized by `ReadyTable`
- place the instruction into the resident IQ state

When it consumes wakeup:

- pop `PTOInstRef` from `wakeup_in`
- compare wakeup output tile tags against all resident waiting inputs
- mark matching `tile_input_ready` bits true
- recompute `src_ready`

When it issues:

- pick the oldest resident instruction whose valid inputs are all ready
- stamp `timestamps.issue_cycle`
- push the same `PTOInstRef` to `issued_out`
- remove the entry from resident IQ storage

## Cycle Behavior

Per cycle:

1. consume all visible wakeup packets and update resident entries
2. accept one new enqueue if capacity exists
3. if a ready entry exists and engine output has space, pick and issue it

## Limitations / Simplifications

- single-width enqueue and single-width issue
- no separate `inflight` residency after pick; deallocation happens at issue
- oldest-ready-first uses `rob_id` only
- no replay, cancellation, or arbitration retry path yet
