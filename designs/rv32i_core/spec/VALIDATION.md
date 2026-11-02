# NDF workflow validation

The public workflow was executed at demo `f7e29346137cad86a36c93dab29e27a1f66afe8f`
with installed tool `0ec04e920abcc1c41a446462ddfb49c861f4bae0` and NDF
`5e3e6b5f09e06b09aaf52844c4b37e38362ac097` (format 0.2). The SDK was configured,
built and installed from the tool checkout; its installed CLI/adapter bytes
match that source. Python overrides were removed during execution.

| Check | Actual result |
| --- | --- |
| Authored NDF input/Spec/acceptance | 20 nodes, 27 relationships; check/build/export rc 0 |
| Definition-bound NDF graph | 37 nodes, 55 relationships, 16 Python definition bindings; check/build rc 0 |
| Fresh program preparation | Six programs; clang/Spike or separately authored closed-base expectation, rc 0 |
| Public execution | 28/28 functional/timing × C++/Icarus views PASS; runner rc 0 |
| Independent raw/state/RAM/store checker | 28 views, 7 complete-state groups, same final IR per policy; rc 0 |
| Native/RTL cycle observations | 14 comparisons equal, including throughput, load-use and waiting-store reset |

The programs are families, throughput, smoke, c-array, ecall and illegal_fence_i.
Seven selected scenarios use the unchanged fast schedules plus smoke's existing
reset-during-waiting-store schedule. Each public build/run stage retains its
180-second limit. Twenty-four views agree with Spike; four FENCE.I views use
the separately authored illegal-encoding expectation. They are not Spike matches.
The C program retires 434 normal instructions plus its expected trap in 567
virtual cycles in each view. Throughput checks assert retirement cycles 4..67,
five occupied stages and no load-use stalls; smoke checks one load-use bubble.
The memory/store/state comparisons remain independent of DUT calculations.

[Machine results](validation.json) record the exact selected scenarios and
[standard NDF evidence](workflow-evidence.json) links execution to the four
verification clauses. The command is published in portable variable form;
the workflow retains the resolved commands, independent exit codes and raw
outputs in its selected output directory. Reproduce with the single command
in [the workflow guide](README.md#run-traceability-and-real-model-execution).

All eight hardware source ASTs are unchanged from `c00f45c`. For both policies,
all six physical RTL files (top and five hierarchy modules) match that
publication's mapping inputs byte-for-byte. The existing 13,596-cell SKY130
mapping and mapped C simulation evidence can therefore be retained; synthesis
was not rerun. Final IR and runtime results were generated and checked in this
workflow, not copied from those outputs. The historical 160-view campaign
remains separate and was not rerun or relabelled as this workflow's result.

The new NDF evidence adds one evidence node and four `evidenced-by` edges when
indexed with the authored Spec. Indexing it records the scope of that command;
it does not promote draft requirements to active or establish formal refinement.
Independent review, specification approval, official ISA certification,
full formal equivalence, STA and placement/routing remain unrecorded or NOT_RUN.

## Final installed-consumer replay

The standard-evidence-capable SDK is built/installed at tool
`d4a36a518d18fc233d5adbd14c8e971ec080fc7d`. Its installed CLI/adapter files
match that source. With demo `d2789a52914646bae276d541f6e7cc9277801956`,
the same public workflow with `--case c-array` passes all four stages (each rc 0):
NDF source/evidence traceability, fresh C/Spike preparation, four real model
executions and the independent raw/state/RAM/store checker. The graph including
standard evidence has 38 nodes and 59 relationships, with 16 definition bindings.

This is a separate 4/4 C-program replay: 434 normal retirements plus one expected
trap and 567 virtual cycles per view; one complete-state group and the same
verified final IR per policy. All 12 physical RTL comparisons still match the
historical mapping inputs. It is not another 28-view or 160-view campaign.
The following publication changes only result text/records, not the tested
hardware sources, workflow code or normative clause bodies.

## Native NDF gate

The [native gate results](NDF_GATE.md) distinguish the first unapproved draft
from the evidence-bearing draft, refresh both indexes, inspect actual native
diffs and run native coverage and trace. Empty active-L1 coverage is explicitly
N/A for this draft. This graph-only follow-up does not rerun or relabel any
previous processor, RTL or mapping result.
