# Release-owned repository layout

## Remove product versions and phases from source paths {#D-RELEASE-LAYOUT-001}
<!-- ndf: kind=decision level=must layer=L1 status=stable affects=ARC-RELEASE-001,ARC-LAYOUT-001,ARC-HISTORY-001 -->

**Context.** Versioned directories caused several generations of examples,
tests, and contracts to appear active at the same time. Phase plans and current
implementation documentation were also indistinguishable.

**Decision.** The repository uses semantic paths only. Releases identify
versions. A new release changes files in place and does not create a parallel
versioned source tree. The migration is a hard break: old paths have no aliases,
redirect files, or compatibility symlinks.

**Consequence.** Consumers must use the paths shipped by the selected release.
Repository gates reject new product-version or phase tokens in tracked paths.
