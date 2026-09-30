# M7-01 accepted: bounded local preview

Status: done for the local macOS 26/arm64 preview. Accepted: 2026-10-01.
Implementation/test/evidence commit: `4584ad0b` on `codex/gfsim-source-units`.
Baseline: `25152f9c`; the approved scalar, portless-root, default-clock,
empty-static-argument and serial-simulation profile is unchanged.

The current checkout was freshly built and installed, then exercised through
source compile/link, same-final CPP/RTL emission, native runtime/ABI, relocated
SDK and isolated wheel consumers. The corrected build presets, structured
retirement scanner and current semantic closure now cover earlier M6 packets.
No public source/IR/CLI/schema/ABI/manifest/version contract was changed.

- [Full packet and scope](https://github.com/PTO-ISA/pyCircuit/blob/4584ad0b/docs/work-items/m7-local-preview.md)
- [Independent Sol high review](https://github.com/PTO-ISA/pyCircuit/blob/4584ad0b/docs/reviews/20261001-m7-local-preview-review.md)
- [Raw outcomes, commands and byte bindings](https://github.com/PTO-ISA/pyCircuit/blob/4584ad0b/docs/gates/logs/20261001-m7-preview/README.md)
- [Current reproduction runbook](https://github.com/PTO-ISA/pyCircuit/blob/4584ad0b/docs/development/release-runbook.md)

Reviewed fourteen-file binding:
`8b5e7c857bae3abad72e1d2495e9e21f19dbe0d809672756d0f794368b105250`.

| Gate | Outcome |
| --- | --- |
| Fresh build/install and actual corrected preset | passed; 197 and 112 build steps |
| Native tests | 20/20 |
| Semantic closure | 311 passed, 2 documented V44 scheduling cases deselected |
| Unit suite | 324 passed, 3 Windows-only skips, 79 marker deselections |
| Independent preset/retirement/layout | 19 passed |
| Independent public/runtime/model ABI | 25 passed |
| Moved-prefix and disposable wheel smoke | 1 passed each |
| Retirement/repository/strict decision status/hooks/docs | passed in implementation checkout |

The local wheel is explicitly tagged `macosx_26_0_arm64`, SHA-256
`9707a2dbbf05960ae574f4905a364b86f948a6a654a33b827f3f604909ac35e8`.
It is installation smoke on the tested host only; no formal macOS-15 minimum,
Linux/Windows or published SDK claim. Failed preliminary environment/test and
live-script-edit runs were retained and excluded from final acceptance; final
immutable semantic R3 returned zero and wrote a fresh pass summary.

M7-01 meets the revision-8 scoped preview exit. It does not complete all M6/M7 or
the full capability roadmap. Existing `v6.1.0` identifies older `d4926615...`;
no release workflow, new version/tag/index or publication was dispatched.
SYSTEM/EXPECT B remains unapproved. Wider M3 capability work, parallel simulation,
platform/fault/SDK coverage, Unicode host directories, selective backend rebuild
and RSS/throughput remain visible backlog.

Planning validation: targeted hooks pass. The planning checkout still has twelve
preexisting strict-doc missing links in unchanged historical documents; the
implementation checkout's strict build passes. Other agents' planning edits,
including unapproved proposal files, remain untouched.
