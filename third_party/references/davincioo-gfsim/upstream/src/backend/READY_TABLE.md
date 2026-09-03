# ReadyTable

This document describes the ready-table and wakeup-broadcast module implemented in:

- [ready_table.hpp](./ready_table.hpp)
- [ready_table.cpp](./ready_table.cpp)

## Purpose

`ReadyTable` records global tile-tag readiness for instructions that are not
currently resident inside an `IssueQueue`.

It also owns the wakeup fanout path:

- consume completed `PTOInstRef` from the global wakeup queue
- mark their produced tile tags ready
- broadcast the same `PTOInstRef` to all issue-queue wakeup queues
- forward the same `PTOInstRef` to ROB completion

## Inputs

- `dispatch_inputs_[i]`
  - type: `SimQueue<PTOInstRef>`
  - producer: `Dispatch`
  - meaning: per-engine routed instructions before issue-queue residency
- `wakeup_in`
  - type: `SimQueue<PTOInstRef>`
  - producer: `Engine`
  - meaning: completed instructions used for wakeup and ROB completion

## Outputs

- `issue_enqueue_outputs_[i]`
  - type: `SimQueue<PTOInstRef>`
  - consumer: matching `IssueQueue`
  - meaning: instructions with initial source-ready state set from the ready table
- `issue_wakeup_outputs_[i]`
  - type: `SimQueue<PTOInstRef>`
  - consumer: every `IssueQueue`
  - meaning: broadcast wakeup stream
- `rob_done_out`
  - type: `SimQueue<PTOInstRef>`
  - consumer: `ROB`
  - meaning: completed instruction stream for completion/retire ownership

## Inner State / Private State

- `ready_tags_`
- configured issue-queue count

`ready_tags_` is the non-resident/global ready state for tile tags.

## Owned Storage

This module owns only the ready-tag set.

Boundary queue storage is owned by `Core`.

## Features

- initialize `tile_input_valid` / `tile_input_ready`
- maintain global tile-tag readiness
- clear newly allocated output tags from the ready set
- mark completed output tags ready
- broadcast completed instructions to all issue queues and ROB

## PTOInstRef Pop/Update/Push Rule

ReadyTable owns the ready-table and wakeup-broadcast transition.

For dispatch-side traffic:

- pop `PTOInstRef` from the routed dispatch queue
- initialize `tile_input_valid`
- initialize `tile_input_ready` by querying `ready_tags_`
- recompute `src_ready`
- clear all rename-managed output tags from `ready_tags_`
- push the same `PTOInstRef` to the matching issue-queue enqueue output

For wakeup-side traffic:

- peek the completed instruction
- only pop when every issue-wakeup output queue and ROB-done queue can accept it
- pop `PTOInstRef` from `wakeup_in`
- mark produced output tile tags ready
- push the same `PTOInstRef` to every issue-wakeup output queue
- push the same `PTOInstRef` to `rob_done_out`

## Cycle Behavior

Per cycle:

1. if a wakeup packet is visible and all fanout outputs can accept it:
   - consume it
   - update global ready tags
   - broadcast it
2. for each routed dispatch input:
   - if the paired output queue has space, consume one instruction
   - initialize ready state and forward it to the issue queue

## Limitations / Simplifications

- the ready table is a simple set of ready tile tags
- no speculative/non-spec split yet
- no explicit wakeup priority or bandwidth model beyond queue fullness
