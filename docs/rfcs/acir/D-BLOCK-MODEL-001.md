# Queue and block model

## Use one current compositional block model {#D-BLOCK-MODEL-001}
<!-- ndf: kind=decision level=must layer=L2 status=stable depends-on=ARC-RELEASE-001 affects=ARC-LAYOUT-001 -->

**Context.** Queue/Var, explicit memory, and parameterized blocks were developed
in successive release phases. Keeping those phase names in source paths made
them look like competing APIs.

**Decision.** `ac.queue`, `ac.var`, explicit memory, and parameterized blocks
form one current model. Framework examples are grouped by generic semantic
purpose: pipelines, memory, blocks, state, and types. Complete architecture
models and executable workspaces belong to consumer repositories.

**Consequence.** New work extends the existing semantic group. It does not add
a versioned or phase-numbered sibling.
