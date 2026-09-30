# M4 completion: usable from-source migration preview

Status: done — revision-8 M4 bounded exit accepted. Product base: `ce4fbbff`. Current scope comes from revision 8 of
planning branch `codex/gfsim-migration-governance` modernization plan, M4 section.
The prior ledger's mandatory full ABI/RTL-package prerequisites overstated that
scope; preserve them as later C3/M5 obligations, not M4 exit blockers.

## Exact exit

A checked-in documented CMake workflow independently compiles ordinary module
function sources through public compile/link into design_top.ac, then consumes
one immutable final snapshot with private native emit helpers. Source-owned C++
implementation groups compile as independent TUs; declarations remain header-only.
A standard generated pycircuit_system runner uses existing SimExecutor for limits,
statistics, failure and termination. Verilator executes the same final hardware
through a simulation-only clock adapter and the same executor/serializer.

External independent testbench owns expected values and pass/fail; source design
contains its algorithm and legitimate observations. No @system root, unapproved
SYSTEM/EXPECT B, consumer design, model C ABI, installed new emit or legacy cutover
is introduced. Preview generated.json is file-management metadata, not a claim of
complete C3 ABI/source-map delivery; RTL is explicitly aggregate private simulation
input and is not falsely attributed to a source-owned group.

Astra xhigh scope review approves this exit subject to standard M1-C runner
behavior, output protection and independent current-candidate verification.
The user's [default-sink clarification](../rfcs/migration/approvals/m1-runner-default-sink.md)
applies. Explicit sink writes canonical Event/Result JSONL; default is silent.

## Ownership

- unit_pair_fix, Luna high: new SystemRunner runtime header/implementation only;
  reuse SimExecutor/config parser, do not implement a second lifecycle.
- PM: native final runner metadata/RTL adapter emission, readonly RTL check taps,
  private native transport, snapshot/publication materializer and CMake integration.
- decl_cpp_impl, separate Luna high: checked-in design fixtures and independent
  JSONL oracle, unit/system tests. No product implementation authorship this packet.
- other_agent_code_review, Sol high: independent final code review.
- decl_arch_conformance, Astra xhigh: independent scope/RTL architecture review.

All writers preserve other changes. PM owns the checkout-local native build.
Do not create new primitives or reconstruct source semantics in Python.

## Verification requirements

Clean documented configure/build/run without pytest creating the main project;
actual per-source compile commands, declaration header and independent C++ TUs;
same final artifact for CPP/RTL; exact external oracle/events/statistics/results;
finite config/startup rejection and protected sinks; hold vs zero-rule activity;
failed epoch has no commit/events; reset/replay including RTL reset edge handling;
invalid final and native target capability refusal preserve prior output;
source dependency rebuild evidence; malformed/truncated/missing/duplicate terminal
record oracle rejection. No required selector is silently skipped.

Final acceptance needs frozen hashes, current-checkout focused native/regression
and workflow evidence, independent code review and architecture conformance.
M4 is not done until the whole path is reproduced and documented.

## Verified exit evidence

The final independent lane passes 17 workflow tests plus 10 strict JSONL oracle
tests, with zero skips. It exercises both backends for ordinary execution, empty
clocked activity, zero-rule quiescence, failure suppression, successful and failed
same-executor Reset/replay, typed bool/signed/literal observations, source/final
report-name rejection, source dependency rebuild, protected sinks and generated
outputs, managed-input races, and the documented external oracle.

Existing targeted regressions pass 331 cases with three Windows-only skips.
Seven native binaries pass 116 tests. A standalone native build from an empty
output directory completed 106 build steps; a second empty preview output was
configured and built with those helpers and the documented runner/oracle commands
passed for both backends. Strict MkDocs and applicable pre-commit passed.

Common observation validation now rejects duplicate report names within one
actual instance at build/verify time (M1-C 493–505); distinct instances may reuse
the same name. The static IR has five register declarations (four root plus one
child definition); the elaborated design has six registers (two child instances).

This verifies the whole revised M4 exit. Final reviewer findings and content
bindings are archived in docs/gates/logs/20260930-m4-completion/ before acceptance.
Full C3 ABI/source-map/RTL distribution and the public emit hard break remain
explicit later obligations, rather than being silently dropped.

## PM acceptance

Independent Sol code review and Astra architecture/exit review both APPROVE the
same code and documentation candidates. M4 is complete for the documented macOS
source-tree workflow. [Final evidence](../gates/logs/20260930-m4-completion/README.md)
records scope, exact commands, raw results and content bindings. No required
selector in the focused M4 lane is skipped. Promotion from verified to done is
an acceptance-only documentation change after those verdicts.
