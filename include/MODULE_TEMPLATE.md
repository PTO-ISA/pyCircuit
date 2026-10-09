# Module definitions, instances and Work rules

A module definition declares typed inputs, outputs, rules and child instance
structure. An instance is one occurrence of that definition with its own
leaf state. Reusing a definition never shares state between occurrences.

| Concept | ACIR | gfsim | SystemVerilog |
| --- | --- | --- | --- |
| Definition | `ac.module` | Concrete class, optionally parameterized by T | `module`, optionally with `parameter type T` |
| Instance | `ac.instance` callee plus operands/results | Owned pointer to a constructed child | Named module instantiation |
| Signal | Typed SSA value | `wire<T>` | `wire T` |
| Input/output | Block arguments / `ac.yield` | Declared input/output wire members | Explicit port directions |
| Rule | Captures, combinational region, results | Local helper invoked within Work | Combinational expressions |
| Storage | Standard leaf instance | Typed DFF/DFFE or memory child | Standard leaf instance |

`SimObj` defines the abstract lifecycle. `SimModule` is a concrete empty module
with default no-op hooks; a real definition supplies ports, child ownership and
Work/Xfer behavior. An empty module reports no work. Generated combinational
modules must supply their own activity behavior rather than inherit that
empty-module result. DFF/DFFE/memory derive from the same module base.

The definition's structural body fixes children before execution. Constructors
create child instances; no rule allocates modules. Child definitions belong in
their own source-owned files. The system invokes roots, and parents invoke
children; a child is not separately registered at system level.

## Data and layout

T is bits or a finite hardware struct. All ports and internal connections use
the same signal representation; input/output is a direction, not a data type.
See [the gfsim interface](gfsim/README.md) for static C++ struct layouts and
[the RTL interface](verilog/README.md) for four-state packed type parameters.
There is no additional wire object graph, reference protocol or compatibility
WIDTH interface. Both representations follow declaration-order, MSB-first,
no-padding struct layout.

## Work and Xfer

Work calls the module's rule helpers according to actual data dependencies.
A rule is a combinational subfunction with explicit inputs/results, not another
module instance or a scheduler object. Ordinary rules drive wires. Storage-leaf
rules additionally prepare private next-state candidates from current storage.
No Work rule publishes current register or memory state.

Xfer visits child instances and commits prepared leaf state. Only a leaf owns
storage. DFF/DFFE's disabled enables naturally hold; memory strobes select
which stored bits/bytes update. DiscardNext cancels proposals, including staged
clock history. The abstract lifecycle has no Eval hook.

Standard memory sources are organized into Work combination and Xfer commit.
Their procedural implementations are library behavior; ordinary design modules
still use assign and child instantiation. Memory read latency, address bounds,
read-during-write, reset, Q lifetime and unknown-input contracts are preserved.

These changes do not yet implement general per-field codegen scheduling or an
independent asynchronous-memory read call. C++ combinational outputs are valid
at successful Work completion; Xfer alone does not settle parent output caches.

## Current scope

The old design-specific parent/child template files were removed in the prior
development process and are not an installed API. This document defines the general
organization; the owning examples of storage behavior are dff.h, sync_mem.h,
byte_mem.h and their corresponding RTL sources.

This page is an organization guide, not executable module source or candidate
evidence. Its illustrative structure does not by itself establish TableGen
generation, build, frontend/backend integration or test coverage.
