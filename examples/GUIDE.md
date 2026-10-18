# Recommended examples

Start with these 17 current designs. Each teaches a distinct pattern. Superseded
wrappers and their original oracles are preserved as 54 source directories in
[the historical fixture archive](../tests/fixtures/historical_examples/README.md),
outside the current example catalog.

## Learning path

| Example | What to learn |
| --- | --- |
| [hello_counter](hello_counter/README.md) | A small System, persistent state and a nested Rule. |
| [multi_outputs](multi_outputs/README.md) | Direct Module calls; Struct connections and independent tuple results. |
| [struct_transform](struct_transform/README.md) | Structured payloads, combinational locals and registered state. |
| [enum_payload_pipeline](enum_payload_pipeline/README.md) | Nominal Enum fields, structured updates and saved value snapshots. |
| [queue_rule_pipeline](queue_rule_pipeline/README.md) | Typed cross-Module Rule calls, atomic Queue transfer, ordinary `for`, and local `after` ordering. |
| [table_rule](table_rule/README.md) | Persistent Table entries and nested calls in one transaction. |
| [calculator](calculator/README.md) | A complete control function with sequential local calculations and state updates. |

Use separate Rules for independent functions. A Rule's statements and nested
calls form one transaction. Register Rules in their Module; use local `after`
when independently registered Rules require ordered visibility. Plain source
order alone does not establish that relationship.

Each static Module call creates a distinct child instance. Bind the result once
and reuse it to fan out existing outputs. Pass fields that travel together as one
named Struct connection; return a tuple only for independent destinations.
Module-scope annotated declarations are persistent state, while Rule locals and
ordinary Module expressions remain combinational values.

## Design references

| Example | Distinct design purpose |
| --- | --- |
| [rob](rob/README.md) | Ordered Rules, state visibility, retirement and flush. |
| [issue_queue_2picker](issue_queue_2picker/README.md) | Two dependent selections and atomic queue-state transitions. |
| [digital_filter](digital_filter/README.md) | FIR history and a finite-width datapath. |
| [bf16_fmac](bf16_fmac/README.md) | Four registered datapath stages. Its README defines the supported arithmetic; this is not a complete IEEE FMA. |
| [dma](dma/README.md) | Memory controllers and cross-Module atomic transfers. |
| [memory_banks](memory_banks/README.md) | Independent banks, read latency, routing and priority arbitration. |
| [digital_clock](digital_clock/README.md) | Clock-divider state and long-duration decimal carry behavior. |
| [dodgeball_game](dodgeball_game/README.md) | VGA timing and game state. |
| [fifo_loopback](fifo_loopback/README.md) | An explicit external FIFO protocol boundary. |
| [multiclock_domain_isolation](multiclock_domain_isolation/README.md) | Independent physical clock/reset domains across child instances. |

All 17 current designs passed the 34-test examples nightly on 2026-10-18.
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

There is no Signal declaration/set/get API. Use typed Module and Rule values for
connections, compiler-propagated Rule output availability, and state or Queue
when a value must survive across cycles.

Prefer a short System scenario and generated C++/RTL execution. Add handwritten
host or RTL tests only for a specific boundary the System cannot exercise.
See the [authoring standard](STANDARD.md) and
[frontend guide](../docs/development/agent-frontend-guide.md).
