# Repository and release charter

## Release tags own product versions {#ARC-RELEASE-001}
<!-- ndf: kind=req level=must layer=L1 status=stable -->

The checked-out source tree describes one current product. Git releases and
tags assign product versions. Source paths, public type names, test names, and
example groups do not encode a product version or implementation phase.

Serialized documents may retain their own schema version, and dependency locks
retain exact external tool versions. Those values version protocols and
dependencies, not repository layouts.

## Current source uses semantic partitions {#ARC-LAYOUT-001}
<!-- ndf: kind=req level=must layer=L1 status=stable depends-on=ARC-RELEASE-001 -->

Current source is partitioned by responsibility: specifications under
`docs/acir/spec`, public examples under `examples/agentic-circuit`, imported
upstream material under `third_party/references`, and test-owned golden data
under `tests/goldens/agentic-circuit`.

## Historical design remains in Git history {#ARC-HISTORY-001}
<!-- ndf: kind=req level=must layer=L1 status=stable depends-on=ARC-RELEASE-001,REF-HISTORY-001 -->

Completed plans, superseded proposals, and release audits are not duplicated in
the current tree. [[REF-HISTORY-001]] pins the last pre-migration tree so each
historical document remains reproducible through Git.
