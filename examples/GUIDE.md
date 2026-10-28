# Recommended examples

Start with the transactional examples below. Their READMEs describe independent
expectations and identify focused test owners; the presence of a generated
System or matching backend traces alone does not prove correctness. Consult
[current capabilities and gaps](../docs/development/known-limitations.md) before
using a retained design as an authoring template.

## Learning path

| Example | What to learn |
| --- | --- |
| [hello_counter](hello_counter/README.md) | Persistent state and explicit Rule registration. |
| [module_control_state_machine](module_control_state_machine/README.md) | Module branches, guards, autorules and old-Q observations. |
| [local_method_accumulator](local_method_accumulator/README.md) | By-value Methods and rollback of a complete call chain. |
| [sibling_calls](sibling_calls/README.md) | Existing Module references and independent committed-state observations. |
| [sibling_method_calls](sibling_method_calls/README.md) | Calls on an existing independently compiled Module instance. |
| [parent_state_interfaces](parent_state_interfaces/README.md) | Parent-owned State, external Rules and resource aliases. |
| [scalar_queue_pipeline](scalar_queue_pipeline/README.md) | Parent-owned Queues and atomic backpressure. |
| [resource_composition](resource_composition/README.md) | Multilevel references and independent scalar outputs. |
| [structured_packet](structured_packet/README.md) | Nested nominal payloads, whole-record State and Queue transport. |
| [transactional_cumulative](transactional_cumulative/README.md) | Cross-module accumulation, four-sample guard and backpressure. |
| [transactional_batch](transactional_batch/README.md) | Module combinational values, guarded Methods and static branch output. |

Follow the [frontend guide](../docs/development/agent-frontend-guide.md) for
interfaces and Rule authoring, and the [example standard](STANDARD.md) for
scenario and oracle ownership.

## Retained migration inputs

`multi_outputs`, `sibling_atomic`, `sibling_aliases`,
`sibling_transitive`, `struct_transform`, `enum_payload_pipeline`,
`queue_rule_pipeline`, `table_rule` and `calculator` retain distinct behaviors
and original oracles. Their broader source forms require migration; old
candidate-state forwarding must be replaced with explicit SSA values, without
silently changing the design's functional goal.

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

The design references are retained requirements, not current execution claims.
Short DMA, memory-bank and Table Systems do not validate their original long
Module benches. Long clock rollover and exhaustive four-state scenarios remain
in their existing nightly owners. The historical fixture archive preserves
comparison assets; retirement or archival is not a pass.

Operator matrices, malformed IR and analysis limits belong to API tests. Reuse
existing build and verification helpers without combining unrelated behaviors
into a single large example or adding a second execution route.
