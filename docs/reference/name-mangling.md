# Source ownership and generated names

The current compiler preserves source ownership through final-design emission.
Each compiled unit is bound to its source owner; implementation sources produce
source-owned C++/RTL groups and provenance inventories. These records support
review and diagnostics. They do not create model identity or promise exact
source-line mappings for generated code.

C++ and Verilog names are derived deterministically from source definitions
under the approved target-legalization rules. A legalized name collision is an
error; the compiler does not append a counter or content digest to make it
unique. Static arguments are empty in the current public profile.

The older family/case naming and QueueGraph source-map contracts are retired.
See [Decision 0283](../rfcs/pyc6-decisions.md#decision-0283-approved-source-unit-hardware-cutover-for-the-scalar-profile),
[C3 source maps](../rfcs/migration/approvals/c3-source-map.md), and the
[M5 migration guide](../development/m5-migration.md) for current limits.
