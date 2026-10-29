# Current capabilities and remaining work

The [transactional language profile](../reference/language.md#transactional-frontend-replacement)
defines current admission. The retained legacy reference describes migration
requirements, not additional supported syntax. This page is the current gap
inventory; execution outcomes belong to candidate-bound gate reports.

## Current capability and verification owners

| Capability | Current scope | Independent verification entry |
| --- | --- | --- |
| State and control | Bits, Enum and finite Struct State (omitted initializer means X); old-Q reads; explicit Rule registration; Module registration branches, payload local joins, conditional Rule/Method bodies, early callable returns and parameterless autorules | `CodeGen/transactional-state-machine.test`, `transactional-state-interfaces.test`, `transactional-control-flow.test`, `transactional-uninitialized.test` |
| Methods | Local and existing-instance calls, by-value arguments/results, nested guard chains and atomic effects | `CodeGen/transactional-sibling-methods.test` |
| Resource connections | Parent-owned default depth-2 Queues, explicit depth, State/Queue binding, aliases, multilevel references and independent outputs | `CodeGen/transactional-resource-composition.test`, `transactional-queue-pipeline.test` |
| Queue behavior | Old-occupancy admission, backpressure, nonconsuming observation and atomic State/Queue effects; public System Queue-reset execution remains unverified | `CodeGen/transactional-queue-timing.test` |
| Nominal payloads | Explicit complete Struct construction/projection, nested Bits/Enum fields, whole-record State updates and Queue transport | `CodeGen/transactional-structured-packet.test` |
| Failure and four-state data | Global checks precede commit and log publication; X/Z payloads preserve their leaf representation | Transactional failure and structured-packet regressions; native State rollback supplement; generated post-failure storage remains unverified; four-state RTL uses Icarus |
| Source units and saved IR | Independent compile against interfaces, explicit body closure, linked/Routed/Final verification and dual emission | Source-link native tests and transactional saved-artifact regressions |

Paths in the table are relative to `tests/compiler/lit/` unless identified as
native tests. These entries identify the owning tests, not a claim that a
particular candidate passed. See [testing and gates](testing-and-gates.md) for
selection, commands and reporting.

## Implementation gaps

| Area | Remaining work and retained obligation |
| --- | --- |
| Source expressions and effects | Field assignment, constructor defaults, ordinary helpers and loops require migration. Retain their source fixtures and independent oracles. |
| Control knownness | Current execution admission requires provably known controls. Unknown payload support does not establish dynamic X/Z guard semantics; unresolved choices remain in the replacement plan. |
| External Queue interfaces | A root Queue cannot be exported as an external payload-only interface. Nested exports preserve resource identity; a System can consume them internally. |
| Memory, Table and collections | Existing storage/runtime foundations and historical scenarios remain requirements. Their transactional timing, partial-write and family-call migration is not established by Bits/Struct Queue support. Preserve approved timing decisions and resolve only genuinely unspecified semantics. |
| Parameters and advanced types | General parameter-dependent elaboration, broader arithmetic and remaining historical payload paths require explicit admission and independent execution evidence. Existing primitive support alone does not establish frontend support. |
| System reset admission and failure observation | Finite transactional `@system(samples=...)` schedules execute reset within the supported profile. Public generated Systems cannot resample failed State/Queue storage before Reset. Retained schedules/oracles and precise coverage limits are listed in [transactional test scenarios](transactional-test-scenarios.md). |
| System and runtime | Default-clock generated Systems are supported within the current profile. t-gfsim (transactional-gfsim) supports closed single-clock Bits State/Queue Systems with one worker and known clock/reset samples. Nominal payload generation, broader host actions and automatic multi-clock scheduling remain gaps. |
| Complete legacy designs | `module_loop`, `multi_outputs`, `queue_rule_pipeline`, `struct_transform`, `enum_payload_pipeline` and other retained designs need migration of their full behavior. A smaller new example does not validate their original algorithms, stimuli or timing. |
| Packaging and portability | Focused native checks do not establish full nightly, cross-platform release, wheel or performance acceptance. Runtime-only consumers must remain independent of LLVM. |

Omission from the replacement does not cancel an existing capability. The
[replacement plan](transactional-frontend-replacement-plan.md) governs semantic
clarification and retained requirements. Cancelled Signal declaration/set/get
APIs and retired candidate-state forwarding must not be restored as migration
fallbacks. A known meaning with missing implementation is an implementation gap.
