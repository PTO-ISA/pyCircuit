# NDF model traceability validation

The workflow adds an explicit NDF 0.2 dependency at
`5e3e6b5f09e06b09aaf52844c4b37e38362ac097`. It does not make that experimental
format mandatory for all pyCircuit models or change hardware semantics.

| Check | Actual result |
| --- | --- |
| Binding and existing driver regressions | 49 passed, rc 0; includes 5 binding/workflow tests and 44 driver tests |
| Real NDF CLI | Format 0.2; native check/build/export and bound-graph check/build rc 0 |
| Installed example graph | 20 authored nodes / 27 edges; 16 bound definitions give 37 nodes / 55 edges |
| Standard evidence integration | One NDF 0.1 evidence record gives 38 nodes / 59 edges; dangling targets and an empty requirement graph fail |
| CompilerDev/Runtime SDK | Existing build reconfigured and built from the tool checkout, installed in a new prefix; all final stages rc 0 |
| Style and docs | Changed-file hooks and strict MkDocs PASS |

The core implementation is `0ec04e920abcc1c41a446462ddfb49c861f4bae0`.
The evidence follow-up retains standard JSON evidence when projecting the bound
graph, with the same real-CLI regression. Runtime/compiler calculations are
unchanged. Installation checks compare the installed CLI/adapter bytes with the
producing source. NDF dependency identity is checked through the example lock.

The independent small counter contract exercises the general bridge separately
from the processor example. A raw NDF check can accept a document when blank
lines separate clause headings from metadata and only document nodes remain;
this adapter rejects the empty-requirement result. An existing output directory
is preserved, and a failed new attempt retains INCOMPLETE rather than PASS.

Traceability PASS means the authored clauses resolve to named implementation
and verification definitions. It does not claim behavioral correctness or
approval. The public example's execution results are recorded with that
example's own source and verification scope. Existing numerical/execution gates
at `2b8b831` remain in [their original validation record](shared-model-validation.md):
API 25/25, examples 13/13 with Verilator 5.048 and 847 passed / 4 skipped.
They were not rerun or relabelled as results of this metadata/CLI update.
Independent implementation review and full nightly remain NOT_RUN.

## Native NDF gate follow-up

The public command now runs native build/check/diff/coverage and trace, including
explicit eligibility inventory. The real-CLI counter regression checks draft
coverage as N/A, an active L1 gap, a distinct snapshot/evidence diff and a trace
containing actual implementation, verification, intent and evidence. It retains
source paths so the native diff does not mistake projection for clause moves.

Existing processor execution and technology evidence is unchanged. This
follow-up validates NDF commands and reporting; ISA/native/RTL and synthesis
matrices are not rerun. Independent review and approval remain pending.
