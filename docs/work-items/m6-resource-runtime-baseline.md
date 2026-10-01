# M6-04 accepted: selected resource/finite-runtime baseline

Status: done for the selected developer measurement scope. Accepted: 2026-10-01.
Product/tool/test/evidence commit `1efec35e`; base `9ff015a2`.
Independent Sol high review APPROVE; implementation/test instances are separate Luna high.

- [Complete packet](https://github.com/PTO-ISA/pyCircuit/blob/1efec35e/docs/work-items/m6-resource-runtime-baseline.md)
- [Independent review](https://github.com/PTO-ISA/pyCircuit/blob/1efec35e/docs/reviews/20261001-m6-resource-runtime-review.md)
- [Report/commands/logs/byte bindings](https://github.com/PTO-ISA/pyCircuit/blob/1efec35e/docs/gates/logs/20261001-m6-resource-runtime/README.md)

Five-file aggregate:
`4418364a20201086fd83aa9c085625025705951e88ad81576232b201ebc61cf7`.
Report SHA-256:
`8b013fc93bc1309710230ac64525a8725d63026719da86f87ad291915aa2c274`.

Selected shared1/16, distinct1/8 use the existing Python source-unit generator,
real per-source compile/link, same-final CPP/RTL emits and source-owned file/TU
inventories. Four cases, 32 phases and 24 finite C ABI runs passed. Each
1k/10k/100k epoch sample repeats twice, verifies actual terminal epoch/status,
report gauges, last error and Reset/replay on the existing standard executor.
The three-epoch CPP literal oracle remains intact; RTL emit/file inventory is
verified, but RTL simulation/throughput is not measured in this packet.

Independent focused and PM post-format tests: 39 passed. Full unit marker lane:
353 passed, 3 Windows-only skips, 79 marker deselections. Hooks and strict docs
passed in implementation checkout. Timeout child cleanup, NaN/Infinity limits,
source/TU inventory/contained files, exact developer input hashes and malformed
ABI-table validation were tightened after real review findings.

RSS is sampled owned-POSIX-group KiB sum, with short-lived/detached/shared-page
and sampling-overhead limits. Rates include C ABI/executor or process startup/
replay/sampling costs. No exact peak, exclusive memory, isolated steady state,
full size matrix or performance threshold is claimed; shared1 initial12s
had concurrent PM test workload and remains exploratory. No runtime/IR/public
CLI/receipt/ABI/schema was changed. Wider M6 responsibilities remain open.

Planning targeted hooks pass; its strict docs retain the twelve existing
historical missing links, separately from implementation strict pass.
