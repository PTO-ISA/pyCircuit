# Queue and block model

## Use one current compositional block model {#D-BLOCK-MODEL-001}
<!-- ndf: kind=decision level=must layer=L2 status=stable depends-on=ARC-RELEASE-001 affects=ARC-LAYOUT-001 -->

**Context.** Queue/Var, explicit memory, parameterized blocks, and end-to-end
workspaces were developed in successive release phases. Keeping those phase
names in source paths made them look like competing APIs.

**Decision.** `ac.queue`, `ac.var`, explicit memory, parameterized blocks, and
workspace examples form one current model. Examples are grouped by purpose:
pipelines, memory, blocks, architecture models, and complete workspaces.

**Consequence.** New work extends the existing semantic group. It does not add
a versioned or phase-numbered sibling.
