# U01 native review-C RED

Fresh current-source results: foundation contracts 36/36 passed; configured Packet/source-admission system tests 49/49 passed with zero skips; SourceUnit/header GTests passed 13/14.

The single failing test is `SourceUnitReviewTest.RawAndSnapshotLocationsRetainProviderPathButIgnoreDiagnosticCoordinates`. It clones the real compiler-produced consumer interface snapshot, preserves the field `location.path` as the authoritative provider path `packet.py`, and changes only `line`/`end_line` to 777. `SourceHeaderRegistry` rejects it with `import snapshot differs from its owning header declaration`.

The frozen C2 boundary requires snapshot comparison to ignore nested diagnostic location coordinates while retaining strict provider-path, shape, semantic property, body, operation-order, and operand equality. Other cases in the same test passed: raw field and helper-parameter provider-path mismatches reject; snapshot field/parameter path mismatches reject; missing snapshot helper-parameter location rejects. Existing operand swap, body/order mutation, origin-anchor, binding/default-gap, forward-malformed-record, exact-AST-path, same-type Pair, source-admission, malformed-transport, dependency-order, and header-only cases also pass.

No implementation was edited by the test lane. Build/source ownership was returned for a narrow snapshot normalization fix. This is not a complete U01 or framework green result.
