# Issue #40 P2 FM16 repository boundary

FM16's README, package marker, behavioral simulator, NPU source, and SW5809s
source moved to `hengliao1972/DavinciOO@26d193dd` under
`srcs/core/system/fm16/`. The consumer locks exact pyCircuit revision
`0f9e0a38`, rejects mismatched/preloaded modules, packages the lock, and passes
four compatibility tests.

pyCircuit removes the complete directory, build registry entry, and stale Ruff
overrides without a forwarding copy, symlink, or placeholder. Example guidance
now describes both structural and CycleAware entrypoints.

The release-layout gate scans the framework example tree for generic
product-system file/class naming (`system`, `soc`, `board`, `cluster`) rather
than naming FM16 or another consumer. Negative and ordinary-leaf positive tests
lock the intended boundary; the authorized `designs/davincioo/` contributor
program remains outside the scanned example root under Decision 0222.
