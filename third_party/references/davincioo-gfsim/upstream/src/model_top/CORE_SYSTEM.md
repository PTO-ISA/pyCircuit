# CoreSystem

This document describes the wrapper system implemented in:

- [core_system.hpp](./core_system.hpp)
- [core_system.cpp](./core_system.cpp)

## Purpose

`CoreSystem` is the top-level cycle driver around `Core`.

It provides:

- ownership through `SimSystem`
- reset/build/step lifecycle
- explicit run-loop helper methods
- summary extraction

## Inputs

- loaded trace instruction vector through:
  - `LoadTrace(std::vector<PTOInst>)`

## Outputs

- `SimulationResult`

The result includes:

- total record count
- total simulated cycles
- ROB capacity / final count
- processed instruction log
- opcode histogram

## Inner State / Private State

- owned `Core` module instance

## Owned Storage

`CoreSystem` owns the top-level `Core` instance through the `SimSystem`
container.

`Core` owns the queue storage and child modules below that.

## Features

- build once
- reset before execution
- expose:
  - `RunReference(...)`
  - `step()`
  - `getCycles()`
  - `needTerminate()`
  - `enableTrace(...)`
  - `PrintPipeView(...)`
- step until `Core::Done()`
- abort if no module or queue reports progress for 2000 consecutive cycles
- collect final processed stream and counters

## Cycle Behavior

1. build the `Core` topology
2. reset the topology
3. run a cycle loop of:
   - `RunReference(...)`
   - `step()`
   - consume the per-step progress signal
   - optional `PrintPipeView(...)`
   - `enableTrace(...)`
   - `needTerminate()`
4. stop only when the core and all owned queues/modules are drained or a
   stop-cycle limit is reached
5. raise a deadlock error if nothing moved for the configured watchdog window
6. collect final summary

## Limitations / Simplifications

- no external time source or wall-clock pacing
- no interactive stepping CLI yet
- no partial-run checkpoint/restore support
- deadlock detection is progress-based, not a semantic dependency analysis
