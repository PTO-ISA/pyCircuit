# Repository contract verification

## Release layout verification {#VER-RELEASE-001}
<!-- ndf: kind=verif level=must layer=L2 status=stable verifies=ARC-RELEASE-001 -->

Run `python3 tools/agentic-circuit/check-release-layout.py`. The check rejects
versioned active Agentic Circuit paths and every deprecated repository root.

## Responsibility layout verification {#VER-LAYOUT-002}
<!-- ndf: kind=verif level=must layer=L2 status=stable verifies=ARC-LAYOUT-001 -->

Run `python3 tools/agentic-circuit/check-release-layout.py` and
`python3 tools/agentic-circuit/check-contracts.py`. The checks require the
canonical compiler, Python, simulator, schema, tool, example, test, reference,
and documentation roots.

## Historical-source verification {#VER-HISTORY-001}
<!-- ndf: kind=verif level=must layer=L2 status=stable verifies=ARC-HISTORY-001 -->

Run `python3 tools/agentic-circuit/check-ndf.py`. The NDF check requires every
historical reference to carry a Git origin, a full revision, and an explicit
origin status while leaving archived gate evidence unchanged.
