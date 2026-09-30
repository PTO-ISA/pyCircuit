# Architecture overview

The active design path has one semantic authority and one saved final input for
both backends:

```text
Python source → per-source capture/interface units → link and MLIR verify
              → final hardware design → C++ or Verilog
```

Python capture records source structure; it does not execute the design. The
compiler resolves source-owned interfaces and derives the hardware model.
Persistent registers use current/next state; the system commits proposals at
Xfer. C++ and Verilog consume the same verified final artifact.

The active profile is portless function modules, nested rules, explicit
registration, one default clock, finite scalar state, and empty static
arguments. Generated C++ retains source-owned translation units and links one
Runtime. Runtime-only consumers do not need LLVM; CompilerDev is pinned to
LLVM/MLIR 22.1.8.

CycleAwareSignal, automatic cycle balancing, structural builders, Agentic
Circuit/QueueGraph scheduling, and the PYC C++ compiler are retired routes.
Queues, a full `@system` contract, memory/CDC, multiple clocks, four-state
source values, and external typed ports remain later capability work. See the
[M5 migration guide](../development/m5-migration.md) and
[Decision 0283](../rfcs/pyc6-decisions.md#decision-0283-approved-source-unit-hardware-cutover-for-the-scalar-profile).
Candidate verification remains in progress.
