# Source ownership and generated names

The current compiler preserves source ownership through final-design emission.
Each compiled unit is bound to its source owner; implementation sources produce
source-owned C++/RTL groups and provenance inventories. These records support
review and diagnostics. They do not create model identity or promise exact
source-line mappings for generated code.

C++ and Verilog names are derived deterministically from source definitions
under the approved target-legalization rules. A legalized name collision is an
error; the compiler does not append a counter or content digest to make it
unique. Static-argument admission follows the [language reference](language.md).

The shared legalizer preserves ordinary ASCII identifiers that are legal in
both targets and do not use reserved names. Other names use `pyc_` followed by
the lowercase hexadecimal UTF-8 bytes of the original component. This includes
keywords, non-ASCII names, names beginning with a digit or underscore, names
ending with an underscore, names containing `__`, and source names beginning
with `pyc_`. Encoding trailing underscores also prevents compiler-added
separators from creating reserved double underscores in compound C++ names.

The `pyc_` prefix reserves a disjoint compiler namespace: source spellings under
that prefix are encoded, while generated families and helper methods use
semantic names such as `pyc_family_` and `pyc_validate_checks`. Generated C++
helpers use this spelling throughout declarations and calls. No compatibility
aliases using the C++ implementation-reserved `__pyc_` prefix are emitted.
Original source spellings remain in source ownership and provenance records;
target legalization does not change semantic identity.

The shared generation preflight checks module classes, generated family classes,
nominal Enum/Struct types and C++ class/namespace conflicts. RTL typedefs are
checked in their package namespace, separately from global module names. The
current RTL spelling joins legalized qualified components with underscores;
distinct names such as `a_b.State` and `a.b_State` therefore cannot coexist in
that emitted type namespace. Both targets reject the collision before producing
a bundle. Type identity is not silently merged and names are not renamed ad hoc.

The older family/case naming and QueueGraph source-map contracts are retired.
See [Current limitations](../development/known-limitations.md),
[C3 source maps](https://github.com/PTO-ISA/pyCircuit/blob/main/docs/rfcs/contracts/approvals/c3-source-map.md), and the
[source-unit workflow](../development/source-unit-workflow.md) for current limits.
