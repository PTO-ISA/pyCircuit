# ROB

This document describes the current `gfsim` backend reorder buffer
implementation in:

- [rob.hpp](./rob.hpp)
- [rob.cpp](./rob.cpp)

## Purpose

The ROB is the current backend owner for `PTOInst` lifecycle state.

Its responsibilities are:

- accept `PTOInst` payloads from the backend input queue
- allocate ROB slots in program order
- track per-entry status
- issue instructions to rename
- send retire notifications to rename
- observe completion through an explicit completion-event queue
- retire completed head entries in order
- preserve parent-owned queue wiring rules from the modeling framework

## Input / Output Contract

The ROB module consumes exactly one boundary input queue and drives one boundary
output queue pair:

- `INPUT(inst_in, 0)` of type `SimQueue<PTOInstRef>`
- `INPUT(done_in, 1)` of type `SimQueue<PTOInstRef>`
- `OUTPUT(rename_out, 0)` of type `SimQueue<PTOInstRef>`
- `OUTPUT(retire_out, 1)` of type `SimQueue<PTOInstRef>`

`rename_out` carries issued instructions toward rename. `retire_out` carries
the retiring instruction reference to rename so overwritten tile tags can be
recycled. The completion input queue carries completed `PTOInstRef` packets
back from the wakeup/ready-table path.

The ROB also keeps a `processed_` log of retired instructions for the top-level
simulator to report.

## Parent Ownership

The ROB follows the queue ownership rules from the model framework:

- parent modules own interconnect queue storage
- child modules receive only queue pointers for bound ports

In the current topology, `Core` owns the queue that connects frontend to
backend, and `ROB` consumes that queue through its input binding.

## Parameterization

ROB sizing comes from TOML config through `ROBConfig`:

```toml
[rob]
entries = 64
```

Current rules:

- `entries` must be greater than zero
- `entries` must be a power of two

These rules match the wrap-friendly ring-buffer discipline used in LinxCore.

## Internal State

The ROB owns:

- `std::vector<ROBEntry> entries_`
- `head_`
- `tail_`
- `count_`
- `next_rid_`

This is a classic ring-buffer structure:

- `head_` points to the oldest live entry
- `tail_` points to the next free allocation slot
- `count_` tracks occupancy
- `next_rid_` assigns a monotonically increasing ROB id

## ROBEntry

Each `ROBEntry` contains:

- `rid`
- `status`
- `inst`

`inst` is a `std::shared_ptr<PTOInst>`, so the ROB and downstream execution
pipeline can observe the same logical instruction object while the ROB retains
retirement ownership.

## PTOInstRef Pop/Update/Push Rule

ROB is one of the stages that must update runtime state when it pops and pushes
`PTOInstRef`.

Current ownership points:

- on alloc:
  - pop from `inst_in`
  - assign `rob_id`
  - clear runtime transport state such as `engine_id`
  - clear stage-latency / source-readiness runtime state
  - stamp `timestamps.rob_alloc_cycle`
  - set ROB entry status to `Alloc`
- on issue:
  - pop no queue; inspect owned entry
  - for no-engine instructions, mark immediate terminal status locally
  - otherwise push the same `PTOInstRef` to `rename_out`
- on completion:
  - pop completed `PTOInstRef` from `done_in`
  - update ROB entry state by `rob_id`
  - consume the completion timestamp already written by engine
- on retire:
  - stamp `timestamps.rob_retire_cycle`
  - push the same `PTOInstRef` to `retire_out`
  - copy final `PTOInst` data into `processed_`
  - free the ROB slot

## Status Model

Current entry states are:

- `Free`
- `Alloc`
- `Issued`
- `Completed`
- `Resolved`
- `Exception`

Intended meaning:

- `Free`: slot is unallocated
- `Alloc`: instruction has entered the ROB but has not been issued
- `Issued`: instruction has been accepted by the backend execution path
- `Completed`: execution/writeback finished
- `Resolved`: instruction is ready to retire normally
- `Exception`: instruction reached an exceptional terminal state

## Current Cycle Behavior

Each cycle, the ROB does the following in order:

1. consume at most one engine completion from `done_in`
2. retire the head entry if its status is `Completed` or `Exception`
3. allocate one new `PTOInst` from the input queue if space is available
4. issue one alloc-state entry toward rename if rename output space exists

This is the current minimal Davinci model-top behavior: fetch one instruction
from the trace side when the ROB has space, place it in the ROB, wait a dummy
latency, and retire it in order.

## Current Transition Policy

The current status progression is:

- `Free -> Alloc`
- `Alloc -> Issued`
- `Issued -> Completed`
- `Completed -> Resolved -> Free`

`Exception` replaces `Completed` for unsupported or unknown instruction cases.
In the current simplified backend, `Resolved` is a transient retirement state
that is immediately cleared back to `Free` after the instruction is copied into
the processed log.

## Retirement

`RetireCompletedHead()` retires only from the head in order.

Retirement behavior:

- stop if the head entry is not `Completed` or `Exception`
- copy the observed `PTOInst` into `processed_`
- mark the entry as `Resolved`
- clear the entry back to `Free`
- advance `head_`
- decrement `count_`

This preserves in-order retirement semantics.

## Relation To LinxCore

This ROB borrows the high-level structure from LinxCore:

- power-of-two depth
- explicit head / tail / count bookkeeping
- vector-backed entry storage
- explicit per-entry state
- in-order head retirement

It is still much simpler than the LinxCore backend ROB because it does not yet
model:

- multi-alloc / multi-retire width
- distinct issue / execute / complete owner modules
- a dedicated completion payload separate from `PTOInstRef`
- traps with detailed payload propagation
- checkpoint / redirect / flush interactions
- wakeup, scoreboard, or memory ordering interactions

## Expected Future Evolution

Likely next steps:

- split issue / completion / retire into richer multi-width stages
- add richer completion event payloads beyond the current minimal event
- add flush / redirect behavior
- add exception payload fields to `ROBEntry`
- expose ROB occupancy / state probes for trace and debugging
