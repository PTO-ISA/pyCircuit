# Recommended examples

Start with these 21 current designs. Each teaches a distinct pattern. Superseded
wrappers and their original oracles are preserved as 54 source directories in
[the historical fixture archive](../tests/fixtures/historical_examples/README.md),
outside the current example catalog.

## Learning path

| Example | What to learn |
| --- | --- |
| [hello_counter](hello_counter/README.md) | A small System, persistent state and a nested Rule. |
| [multi_outputs](multi_outputs/README.md) | Direct Module calls; Struct connections and independent tuple results. |
| [sibling_calls](sibling_calls/README.md) | Existing Module references and independent callbacks reading committed state. |
| [sibling_atomic](sibling_atomic/README.md) | A late sibling guard holds the whole transaction while the reader continues. |
| [sibling_aliases](sibling_aliases/README.md) | Aliases and reference passthrough share state; separate allocations stay independent. |
| [sibling_transitive](sibling_transitive/README.md) | Guarded Method results and compiler-propagated output availability. |
| [struct_transform](struct_transform/README.md) | Structured payloads, combinational locals and registered state. |
| [enum_payload_pipeline](enum_payload_pipeline/README.md) | Nominal Enum fields, structured updates and saved value snapshots. |
| [queue_rule_pipeline](queue_rule_pipeline/README.md) | Typed cross-Module Method calls, atomic Queue transfer, ordinary `for`, and committed snapshots. |
| [table_rule](table_rule/README.md) | Persistent Table entries and nested calls in one transaction. |
| [calculator](calculator/README.md) | A complete control function with sequential local calculations and state updates. |

A directly nested Rule is one autonomous computation per Module or System
instance. Its declaration activates it; do not add a trailing call. At Module
wiring, its bare name supplies its scalar, Struct or tuple results. Captures
supply inputs; any Rule formal requires an explicit default binding. Source-global
Rule templates retain explicit actual calls, such as `advance(phase)` in the
benches.

Use separate Rules for independent functions: each reads committed state.
Sequential assignments and Method calls within one Rule share its candidate
state and atomic commit. Put dependent operations in that one callback, as in
ROB. Callable component operations use Methods, including zero-argument reads;
local autonomous Rules are not callable. `after` ordering is retired.

Each static Module call creates a distinct child instance. Bind the result once
and reuse it to fan out existing outputs. Pass fields that travel together as one
named Struct connection; return a tuple only for independent destinations.
One value remains a scalar, without a one-field Struct or singleton tuple.
Module-scope annotated declarations are persistent state, while Rule locals and
ordinary Module expressions remain combinational values.

## Design references

| Example | Distinct design purpose |
| --- | --- |
| [rob](rob/README.md) | One atomic callback, sequential Methods, retirement and flush. |
| [issue_queue_2picker](issue_queue_2picker/README.md) | Two dependent selections and atomic queue-state transitions. |
| [digital_filter](digital_filter/README.md) | FIR history and a finite-width datapath. |
| [bf16_fmac](bf16_fmac/README.md) | Four registered datapath stages. Its README defines the supported arithmetic; this is not a complete IEEE FMA. |
| [dma](dma/README.md) | Memory controllers and cross-Module atomic transfers. |
| [memory_banks](memory_banks/README.md) | Independent banks, read latency, routing and priority arbitration. |
| [digital_clock](digital_clock/README.md) | Clock-divider state and long-duration decimal carry behavior. |
| [dodgeball_game](dodgeball_game/README.md) | VGA timing and game state. |
| [fifo_loopback](fifo_loopback/README.md) | An explicit external FIFO protocol boundary. |
| [multiclock_domain_isolation](multiclock_domain_isolation/README.md) | Compiler conformance reference for independent physical domains; not a frontend wiring template. |

The previous set of 17 designs passed the 34-test examples nightly on 2026-10-18.
That historical run predates the sibling-calls example.
DMA, memory banks and Table use readable 42-, 26- and 16-cycle System scenarios;
their independent Module oracles remain. Earlier long-form benches are preserved
in the historical fixture archive and were not revalidated by these shorter
scenarios. The full 100,000,001-cycle digital-clock System also passed.
Long clock rollover and exhaustive four-state runs belong to the nightly tier.

## Keeping the set useful

Operator variants, malformed IR, type-admission matrices and resource-limit
stress belong in API tests. Queue protocol recipes may share a tutorial, while
retaining distinct atomicity and backpressure regressions in their test owners.
Do not combine all variants into one giant demonstration module.

There is no Signal declaration/set/get API. Use typed Module and autonomous Rule
results for connections, compiler-propagated Rule output availability, and state
or Queue when a value must survive across cycles.

Prefer a short System scenario and generated C++/RTL execution. Add handwritten
host or RTL tests only for a specific boundary the System cannot exercise.
See the [authoring standard](STANDARD.md) and
[frontend guide](../docs/development/agent-frontend-guide.md).
