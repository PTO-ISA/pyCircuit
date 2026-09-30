# M6-01 accepted: process recovery and moved compiler prefix

Status: done for this bounded macOS-arm64 packet. Accepted: 2026-10-01.
Implementation/test commit: `477beae8` on `codex/gfsim-source-units`.
Product behavior remains the accepted M5 baseline `d351079b`.

V48 now has real SIGKILL/recovery evidence: 24 replacement checkpoints across
compile/link/CPP emit/RTL emit, five reachable first-publication compile points,
three interruptions during rollback recovery, and deterministic reader lock
blocking/release. V47 moves a complete SDK to a path with spaces/Unicode and
uses clean Python/compiler/loader/CMake environments, final-only dual emission,
and Runtime-only build/run with independent observations.

- [Full packet and scope](https://github.com/PTO-ISA/pyCircuit/blob/477beae8/docs/work-items/m6-publication-relocation.md)
- [Independent Sol high review](https://github.com/PTO-ISA/pyCircuit/blob/477beae8/docs/reviews/20261001-m6-publication-relocation-review.md)
- [Raw evidence and file manifest](https://github.com/PTO-ISA/pyCircuit/blob/477beae8/docs/gates/logs/20261001-m6-process-relocation/README.md)

Test/fixture binding:
`2d65e3c76ad557a3de030a0cdbbc53b4518fa5007c4705a789de4dce44f9dc45`.
PM gate: 5 system tests passed; regression 131 passed with 3 Windows-only skips;
hooks and strict docs passed. Relocation also passed with Python 3.12.12 and
3.14.6. No public interface/IR/runtime/ABI was changed and no new product
approval was required by this existing C3 contract validation.

M6 is not complete overall: broader first-publication/fault and platform
matrices, measured incremental-build/scale work, actual parallel scheduling
and M7 release remain. SYSTEM/EXPECT B remains outside this packet. Other
agents' unrelated planning changes are preserved.
