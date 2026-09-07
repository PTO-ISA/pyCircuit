# Repository layout verification

## Enforce the release-owned layout {#VER-LAYOUT-001}
<!-- ndf: kind=verif level=must layer=L3 status=stable verifies=ARC-RELEASE-001,ARC-LAYOUT-001,ARC-HISTORY-001 -->

`tools/agentic-circuit/check-release-layout.py` rejects tracked product-version
and phase paths, consumer product-system classes under framework examples, and
checks the required semantic roots. `tools/agentic-circuit/check-ndf.py` validates
clause metadata, stable IDs, relationship targets, and L1 verification
coverage. Repository contract tests execute both gates.
