# P1a and governance independent review

Date: 2026-10-01. Reviewer: independent code/governance review instance, dispatched `gpt-6.1-sol` / `high`. Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`; HEAD: `114b1ae1c79191817f82931825706e617234d9c0` plus the observed dirty overlay. Reviewer authored no source or governance changes; only this report. Skills applied: pyc6, pycircuit-design-review, pyc-build-v60.

Verdict: **accepted / PASS for the three governance files and P1a naming-only packet**. No unresolved in-scope finding. This is neither broad framework semantic acceptance nor interface approval.

## Finding disposition

The initial P2 wording inconsistency is closed. `docs/development/project-governance.md:63` now binds architecture to the actual architect preset at xhigh. Its Complex table row at line 78 now requires actual architect, separate decomposition agent, bounded implementation/test lanes and independent review. These match the mandatory sequence and native binding below them. The independent reviewer reread the corrected candidate.

## Evidence and bounded conclusions

The governance additions prohibit fixture/name/width/literal/shape recognition as product semantics or admission, compatibility aliases, fallback/duplicate semantic compilers and backend/common-IR bypasses. They explicitly preserve legitimate source constants, independently derived oracle literals, fixed hardware primitive signatures, and adapters implementing an approved phase boundary directly. They require separate architecture/decomposition/implementation/test/review instances and actual model/effort recording, including no relabeling of prior Sol/Luna instances as the new `gpt-6.1-sol` default. These satisfy the requested governance boundary.

All three files in `ir-renames-p1a.json` have recorded before/after hashes matching the backup and candidate bytes. Reapplying only the declared replacement map to each backup reproduces the candidate exactly. CMake equals HEAD after the sole source-entry substitution `FinalHardwareProgram.cpp` → `FinalHardwareDesign.cpp`. Active compiler/test search found no remaining `FinalHardwareProgram` include/view/rebuild symbols. The same byte checks passed after the independent test run. This proves P1a changes naming only; it does not prove source-authority semantics, broader naming cleanup, or whole-framework correctness.

Fresh independent command from the candidate checkout:

```text
.pycircuit_out/m3-s2a-build/bin/ACIRFinalProgramTests --gtest_brief=1
Exit status: 0
[==========] 69 tests from 10 test suites ran. (18920 ms total)
[  PASSED  ] 69 tests.
```

Expected negative-test diagnostics were emitted; all 69 tests passed. The PM's same-checkout P1a build and focused CTest results were independently inspected in `.pycircuit_out/hardware-rescan/p1a-build.log` and `p1a-tests.log` (1/1 CTest binary passed, 20.59 seconds). The strict documentation log `governance-docs.log` ends with successful documentation build. The independently executed binary is bound below. No extra semantic test is needed for this mechanical packet.

Planning-checkout consistency was also checked: `docs/development/project-governance.md` and `.codex/skills/pycircuit-project-manager/SKILL.md` are byte-identical across the implementation and planning checkouts; the AGENTS hardware-boundary sections match. This does not inspect or accept unrelated planning-checkout drafts.

The pre-existing dirty normalized-source-input authority change in `ACIRNumericCompositionSchema.cpp` and its tests is explicitly excluded and remains pending quarantine or repair. The separately reported APInt repair and 92/92 native math result are not used as broad semantic acceptance here. No independent claim is made about register-pipeline examples, docs outside these three governance files, other IR names, or consumer migration.

## Reviewed content binding

| File | SHA-256 |
| --- | --- |
| `AGENTS.md` | `423ef98ee3966b0e2c4e00b92e4c1c38620e7f61e66813580feb1764514871dd` |
| `docs/development/project-governance.md` | `ceb8f942a682cb9eb4938f906095bc8f3b58da4aeaf350e064e10ceb7a91758f` |
| `.codex/skills/pycircuit-project-manager/SKILL.md` | `c5a79b49745a9f12577f69264afc18a9f259475b13d7fa0d8bcd2a8e171a39cd` |
| `compiler/acir/lib/Compiler/FinalHardwareDesign.h` | `9b5019467e6e4007924bc7dc73f97ab5ffc21e1662a2b17317e6531afe36f31e` |
| `compiler/acir/lib/Compiler/FinalHardwareDesign.cpp` | `9558641896bed405880203b7757deba46b8856189d3c1ad8fa126d116fe917b5` |
| `compiler/acir/lib/Compiler/FinalProgram.cpp` | `b71e4283e4aea4cb889c220de4bdcb5794bcce444e524b9a2e16e1d6d69c9700` |
| `compiler/acir/lib/Compiler/CMakeLists.txt` | `5cc8d12d47a671d0a3b85250c435be048174a179eb4ec8e0a807894a93a2b12b` |
| `.pycircuit_out/hardware-rescan/ir-renames-p1a.json` | `df5f4e57d8d062d7e3ba1d0d4baea432f65effc2b2231ef345fee65878f2bd26` |
| `.pycircuit_out/m3-s2a-build/bin/ACIRFinalProgramTests` | `e11065ac2a82d54b4b9ad3ceaad71aaa04a966208f0f9e1023fbf910c9c4d88f` |

Any material change to a listed source, governance file, rename packet or tested binary invalidates its corresponding review conclusion. Further P1 packets require their own bounded review and gates.
