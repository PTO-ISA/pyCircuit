# Sidecar Testbench Schedule

Author: haochongyu@member of Zhangcheng and Xiekunpeng team

This document describes the PR scope for the sidecar testbench schedule path.
It intentionally stays limited to the sidecar schedule path.

## Problem

The original inline testbench path emits the test schedule directly into generated C++.
For long-running or dense cycle-by-cycle testbenches, the generated translation unit can grow with the number of scheduled events.
That makes the C++ compile step sensitive to test length, even when the DUT structure is unchanged.

The sidecar path keeps the C++ runner stable and moves the schedule payload into a sidecar file.
The runner loads that sidecar file at runtime and executes the same drive/expect semantics through a loop.

## User-visible mode

Use inline mode for the existing behavior:

```bash
pycc <design.py> --tb-schedule-mode inline
```

Use sidecar mode when the testbench schedule should be externalized:

```bash
pycc <design.py> --tb-schedule-mode sidecar
```

The sidecar mode can emit a sidecar binary schedule container.
The generated C++ runner links against the sidecar support header and reads the sidecar schedule at process startup.

## Sidecar sections in this PR

| Section | Purpose |
| --- | --- |
| `string_table` | Deduplicates port names, protocol names, and messages. |
| `port_table` | Describes sidecar-visible ports, roles, widths, and protocols. |
| `event_table` | Stores expect/check events by cycle, phase, port, and value words. |
| `frame_table` | Groups drive events that occur on the same cycle and phase. |
| `pattern_table` | Stores compact periodic drive patterns detected from repeated cycle events. |

## Runtime behavior

Sidecar execution has three steps:

1. Load the sidecar schedule.
2. Convert the sidecar sections into the generated runner schedule structure.
3. Step the DUT cycle by cycle while applying drive frames, periodic drive patterns, and post-cycle checks.

The generated C++ code no longer embeds every scheduled cycle as a separate C++ statement or array entry.
Only the runner logic and the static port binding remain in generated code.

## PR boundary

This PR only introduces the sidecar schedule path and the Sidecar sections needed by that path.
It does not add higher-level workload APIs or scoreboard policies.
Those are intentionally left out so the first upstream review can focus on the minimal mechanical split between generated C++ runner code and schedule data.
