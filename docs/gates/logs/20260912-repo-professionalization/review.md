# Repository professionalization review

The final tree keeps accepted source ownership intact while making every
top-level directory explicit in one repository-layout reference. Direct
pyCircuit utilities now live under `tools/pycircuit`; Agentic utilities remain
under `tools/agentic-circuit`; `flows/tools` contains only build and gate
implementation. Unit contracts pin these inventories and require every tracked
top-level root to remain documented.

The cleanup removes three completed migration/prototype pages from active
navigation and consolidates durable provenance in the historical repository
record. Current product docs use semantic headings without manual numbering,
and MkDocs excludes immutable gate evidence from the published site rather than
reporting every evidence file as an orphan page.

The current ACIR-to-C++ runtime path no longer exposes `LegacyDispatch*` names;
the hard-break replacement is the accurately named opaque generated dispatch
ABI. ACIR-to-ACSim also no longer infers register or regfile semantics
from queue symbols such as `pc`, `busy`, or `rf`; verified `watermarks.kind`
metadata is the only selection authority. No compatibility alias was added.

README and getting-started now form a hub-and-spoke onboarding: consistent linked
badges, one frontend-selection table, a tested source setup, tested pyCircuit
and Agentic quickstarts, direct documentation routes, an ownership map, and
clear contribution/security entrypoints. Six badge image endpoints returned
HTTP 200, and the GitHub release and workflow targets are live.

Gate ownership is now explicit and regression-tested. PR CI keeps only
repository/Python checks and avoids running API hygiene both through pre-commit
and directly. Agentic and example scripts no longer embed unrelated root or
semantic closure. Normal simulations, heavy nightly simulations, and the three
dedicated semantic cases form non-overlapping execution partitions. Release
reuses one integrated native build and invokes each closure lane exactly once.
The compile-intensive `bypass_unit` fixture moved unchanged from the normal
lane to nightly; its last full nightly result remains green, while both heavy
public examples were rerun across C++ and Verilator in this change.

Independent adversarial review found and closed four gate-maintenance issues:
the topology test now covers every closure script plus the discovered example
partition; the Agentic gate reuses its editable environment only when Python
and all three package/build metadata inputs match; and generated command
evidence records the resolved build and toolchain paths instead of a fixed
standalone layout.

Review retained historical decisions, gate logs, release tags, contract epochs,
trace schema versions, negative rejection tests, and actionable diagnostics for
removed syntax. Final native, Python, documentation, layout, semantic, and
onboarding gates found no remaining P0-P2 issue in this change.
