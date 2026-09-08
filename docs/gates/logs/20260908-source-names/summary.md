# Source names for generated replay

Branch: feat/davincioo-rob; builds use this checkout and the pyc6 fixed environment.
Decision 0228 observation metadata; existing scheduling/ownership contracts remain.

## Change

Original rule names are preserved before nested-function lifting. Optional,
verified ac.source_name metadata survives rule/firing/transform lowering and
module instance generation into QueueGraph plans. Module labels use source
function names and per-parent occurrence indices, independent of output aliases
and static specialization hashes. Generated observation registration installs
labels before the session snapshots topology at start.

Replay descriptors add display_name, display_path, display_parent_path and
module_parameters (exact textual static attributes). Existing name/path/IDs
remain available. Compiler-generated state scopes are transparent only for
display. Repeated rules have indexed display paths and unchanged titles.
Native objects without metadata retain their names; the viewer no longer
attempts to decode compiler symbols. Display metadata remains included in frozen
integrity fingerprints; those hashes are expected to change with labels.

The viewer groups cards by display parent and exposes canonical identity and
parameters on heading click. New ROB operations are recover, acknowledge,
complete, handoff and allocate under dual_rob_system/rob[0] and rob[1].
No explicit alias authoring API, scheduler changes, or event format changes.

## Evidence

- rob.log: five generated-model scenario groups pass, including scan/activation
  and recording parity. behavior-parity.json additionally compares each complete
  projection with the pre-naming ROB artifacts, byte for byte.
- frontend.log: 259 passed, 4 skipped. An existing substring assertion for the
  ac.source operation matched ac.source_name metadata; it now checks the exact
  operation token instead.
- regressions.log: 12 targeted frontend/native source-label and literal regressions pass.
  Covers output renaming, nested rules, duplicate calls, static specializations,
  supported nested module syntax, malformed metadata and unchanged execution IR
  after removing descriptive attributes and integrity hashes.
- native-replay.log: eight native ReplayTest cases pass, including nested display
  paths, transparent scopes, unchanged identity, native fallback and trace fields.
- existing-rob-and-hierarchy.log: four existing ROB gates plus the nested-module
  execution gate pass.
- cli.log: all seven run-command tests pass after current-checkout installation.
- browser.log: source titles, module grouping, canonical/parameter details,
  native fallback, play/pause/step/seek and exact wide field values pass offline.
  Chromium requires sandbox escalation for its system calls in this environment.

New artifacts and entry page:
`.pycircuit_out/davincioo-rob/20260908-source-names/index.html`.
All five traces/pages are regenerated; earlier ROB evidence is preserved.
Screenshots are in that run's browser directory.

## Limitations and implementation choices

Display paths describe the current source layout and may change on call reorder.
Observation registration initializes metadata rather than modifying primitive
constructor signatures. The replay adapter snapshots metadata only at start;
changing it during recording is unsupported. Source labeling covers existing
supported frontend constructs, without expanding nested-module authoring syntax.
Formatting and MkDocs checks pass; producer and viewer unit suites pass four
tests each. Strict decision-status reports only the same 35 missing historical
summary references for Decisions 0176–0210 (decision-status.log); no historical
evidence is fabricated. No commit or publication was performed.
