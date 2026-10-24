# Static routing acceptance

The host harness runs independent source admission, explicit link closure, and
generated System checks. `--phase source` is the bounded first checkpoint;
`--phase link` adds contextual topology admission; the default `all` also runs
native workers 1/2 and real four-state RTL from the same final artifact.

The first fixtures cover clean same-source ancestors in both declaration orders,
acyclic interface-only sibling/crossbranch consumers, immutable reference alias
forwarding, saved snapshots, distinct allocation state, and X/Z payloads. Source
negatives exercise scope, provider authority, path grammar, reference rebinding,
and direct versus reference binding categories. Link negatives retain unused
contextual definitions and invalid actual paths rather than pruning them.

Execution expectations are derived from the declared initial state and successful
rising edges. Late failure must preserve storage, clock history, epoch, and prior
observations across the entire System. A stage failure remains a failed test;
later stages are reported as not run. No generated transition code is rewritten.

Typed transaction Modules currently reject Python static parameters. Therefore
parameter-dependent reference signature refusal and direct generic actual/default
preservation belong to native contract tests; this harness does not expand source
admission to manufacture those cases. Contextual Collections and nonuniform
parameterized contexts likewise need native coverage where Python cannot express
the required graph under its current language contract.

The S1 slice adds three bounded cases. `partial-writes` routes a Struct field and
a Table element from each of two simultaneous contributors in separate branches.
The right branch also contains a disabled writer whose raw mask selects the left
field and element 0. Its guard and the exterior left writer's guard are
complements of the same committed gate, so overlap is legally exclusive. With
gate zero, OR-ing the disabled raw mask into the right subtree's permission would
incorrectly claim the exterior left update. Held X/Z data and saved local views
are checked alongside the committed merged state. `partial-late-failure` uses the
same graph and preserves all ten physical state lanes, sample histories, epoch,
and prior observations when a later source check fails. Native discard/retry is
also required. `reference-resources` targets an ordinary Module whose own direct
Queue and byte Memory operations check head timing and byte-mask writes. It adds
four epochs after the existing seven samples to reach a successful pop and read
back the high-byte write; existing fixture stimuli remain unchanged.

Independent contributors have no inter-Rule observation ordering contract. The
host compares the complete partial-write event set by epoch and event name,
without removing events or changing per-transaction snapshots. Full routed
runtime/probe status must remain NOT RUN until the ordinary link path succeeds.

The direct `indexed-demand` witness covers a guarded, unavailable index after an
unconditional Table write. Its four known Boolean input combinations require the
first write to commit when the second write's branch is untaken, and the entire
Rule to hold when the branch is taken but the index is unavailable.
`indexed-demand-conjunction` additionally authors `choose and True`; it keeps the
same numerical oracle. These Boolean cases make no X/Z idempotency assumption:
four-state `1 & Z` yields X, and any extension using X/Z selectors must retain
that distinction.

`indexed-demand-path-mutation` runs in the existing harness's `link` and `all`
phases. It generates a fresh final artifact, requires standalone hardware
verification and both baseline emitters to succeed, then locates the indexed
requirement through `ac.sampled_admission.requirements`, its control ordinal,
the actual Rule yield, and the corresponding `ac.value.require`. The selection
must be unique and its payload must be a captured selector rather than the
earlier constant index. Only that operation's path operand changes to an already
dominating, exact-type literal zero; source names, SSA numbers, source lines and
management logs do not select the mutation. The standalone verifier and both
public emitters must reject it. Fresh outputs must remain absent, replacement
bundles must retain their complete original bytes, and source units plus the
original final must remain unchanged. All five entrances run even while the
regression is expected-red, preserving exact diagnostics and protection results.
