# Rename

This document describes the rename module implemented in:

- [rename.hpp](./rename.hpp)
- [rename.cpp](./rename.cpp)

## Purpose

`Rename` assigns tile tags to local PTO tile operands and maintains the current
speculative mapping from tile address to live tile tag.

It sits between `ROB` and `Dispatch` and follows strict pop-update-push queue
ownership.

## Inputs

- `inst_in`
  - type: `SimQueue<PTOInstRef>`
  - producer: `ROB`
  - meaning: issued instructions that require rename before dispatch
- `retire_in`
  - type: `SimQueue<PTOInstRef>`
  - producer: `ROB`
  - meaning: retirement notifications used to recycle replaced tile tags

## Outputs

- `inst_out`
  - type: `SimQueue<PTOInstRef>`
  - consumer: `Dispatch`
  - meaning: renamed instructions ready for dispatch

## Inner State / Private State

- `free_tile_tags_`
- `smap_`
- `RenameConfig`

`smap_` is the current speculative map from tile address to live tile tag.
`free_tile_tags_` is an ordered set-backed free list used for duplicate-safe
tag recycling.

## Owned Storage

This module owns no boundary queue storage.

`Core` owns all physical queues and binds queue pointers into `Rename`.

## Features

- assigns tile tags for rename-managed local outputs
- resolves tile tags for rename-managed local inputs
- records replaced output mappings per output tile
- recycles replaced mappings after retirement
- stalls when insufficient tile tags are free for the head instruction

## PTOInstRef Pop/Update/Push Rule

Rename owns the rename-stage runtime transition.

When rename consumes a retire notification:

- pop `PTOInstRef` from `retire_in`
- recycle each valid `output_replaced_tile_tags` entry

When rename processes an instruction:

- pop `PTOInstRef` from `inst_in`
- stamp `timestamps.rename_cycle`
- look up SMAP for rename-managed inputs and write `tile_tag`
- allocate fresh tile tags for rename-managed outputs
- record overwritten SMAP tags in `output_replaced_tile_tags`
- update SMAP to the new output mappings
- push the same `PTOInstRef` to `inst_out`

## Cycle Behavior

Per cycle:

1. consume all visible retire notifications and recycle replaced tags
2. inspect the head instruction
3. if output queue has space and the free-tag set can satisfy all managed outputs:
   - pop the instruction
   - rename it
   - push it forward
4. otherwise stall the head instruction in place

## Limitations / Simplifications

- single-width rename
- associative SMAP with no explicit capacity model
- no checkpoint/flush rollback yet
- rename domain is heuristic and currently local-tile-only by opcode role
