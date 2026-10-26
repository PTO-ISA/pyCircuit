# Selected Module entry boundaries

A parameterized Module definition can serve both as the selected public top and
as a child of another retained closed Module. Those occurrences may use different
actual parameters. A declaration-local callback and a second ancestor-routed
copy must not execute together in the shared child definition.

For a selected ordinary parameterized Module whose transaction closure needs
occurrence lifting and whose definition is also allocated by a retained closed
Module, source link creates one stateless common-IR entry boundary. Other roots
retain their existing representation. Source `@system` declarations remain
non-instantiable and are outside this normalization.

The boundary contains one typed instance of the original definition, with the
selected actual arguments and directly connected public inputs/results. It adds
no user storage, cycle, runtime guard or source wrapper. Selected callbacks route
at this boundary; callbacks under the retained caller route at that caller. The
original definition and each occurrence's state remain shared and distinct,
respectively. Unselected ordinary definitions retain their existing checks.

## Native representation

The canonical adapter symbol is `pycircuit.__link_entry__`, and its child is
named `entry`. Source publications cannot define/import that reserved symbol or
supply entry-boundary metadata. The sole executable `ac.system` carries exactly:

```mlir
ac.entry_boundary = {
  version = 1 : i64,
  logical_entry = {
    callee = @original.Selected,
    parameters = [...],
    type_arguments = []
  }
}
```

The logical entry preserves the original entry expression attributes. The
physical entry selects the canonical adapter with empty parameters and type
arguments. Domain and execution policy remain unchanged. Absence of the metadata
forbids the reserved symbol; presence requires the exact scaffold and forbids
extra users, state, instances, metadata fields or unverified aliases.

The adapter has the logical root's source owner and resolved public signature,
input/output names and domain ordinals. Before routing its body contains only
the bound instance and an ordered yield. It does not inherit the logical root's
source-call signature metadata. The instance occurrence copies the authenticated
declaration occurrence, changing only `site.definition` to the adapter symbol.
Adapter, instance and yield locations are the original declaration location;
child actual literals retain the original logical declaration's `ac.origin`
and use the canonical span from that declaration. Only the child Instance's
occurrence uses the derived adapter definition. Verification reconstructs these
fields individually.

After routing, compiler-generated Rules, internal ports and connections may
appear in the adapter under the existing transaction proofs. The single ordinary
child and original public prefix remain fixed. Lifted results use the availability
policy of their authenticated lexical owner; the additional boundary must
preserve unavailable results, valid zero values and whole-system rollback.

## Verification and publication

Source-body/header admission and existing source normalization precede entry
normalization. Its independent original-to-normalized proof compares every
original definition, attribute, type and SSA/CFG connection, then verifies the
one added scaffold and the exact System change. The transaction proof consumes
this authenticated normalized input, preserving the complete
original → normalized → routed → final proof chain.

Attribute validation checks closed grammar and placement. Bounded native
structural admission precedes binding and signature evaluation. Routed/final
loads independently validate the scaffold, provenance, actuals, public prefix
and logical/physical relationship. Applicability is checked at normalization;
loads do not require the unrelated caller that triggered it to remain present.

Boundary packages require `ac.routed_contract = 3 : i64`; ordinary packages keep
contract 2. Removing metadata, downgrading the contract or supplying contract 3
without a valid boundary rejects. Saved boundaries use
[anchor schema 2](saved-anchor.md#identity-and-format), with separate logical and
implementation root rows. Standalone legality does not prove equivalence to an
unavailable historical source after a consistent rewrite of the whole artifact.

The requested top, parameter configuration, public ports, `pyc_root` and
`pyc_dut` interfaces retain their meaning. CLI `root_cpp_name` reports the actual
physical implementation type, which is the adapter in this case; it is not a
stable internal C++ type-name ABI. Source maps attribute generated entry sites
through the logical declaration. `SourceLinkStages.beforeEntryNormalization`
and `normalizedEntry` expose diagnostic snapshots on successful link;
`beforeTransactions` remains the exact input to transaction routing.
