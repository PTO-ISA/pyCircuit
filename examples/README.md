# Examples navigation

Current execution uses the single Python → MLIR → C++/Verilog → runner flow.
The current catalog inventories **31** registered designs and **25** API-owned coverage cases.
The inventory includes retained designs awaiting migration. Source migration
status, oracle ownership and candidate execution results are separate facts.
Historical generated receipts do not establish current compiler support.

- [Recommended examples and learning path](GUIDE.md)
- [Writing and verification standard](STANDARD.md)
- [System execution](../docs/architecture/system-execution.md)
- [Machine-readable catalog](catalog.json)
- [Current language](../docs/reference/language.md)

## Full registered inventory

| Design | Migration status | Source | Retained System | Native oracle | RTL oracle | Generated output |
| --- | --- | --- | --- | --- | --- | --- |
| [hello_counter](hello_counter/README.md) | migrated | [Python](hello_counter/hello_counter.py) | [HelloCounter](hello_counter/hello_counter.py) | compiler-generated | compiler-generated | — |
| [module_control_state_machine](module_control_state_machine/README.md) | migrated | [Python](module_control_state_machine/module_control_state_machine.py) | [ModuleControlStateMachine](module_control_state_machine/module_control_state_machine.py) | compiler-generated | compiler-generated | — |
| [local_method_accumulator](local_method_accumulator/README.md) | migrated | [Python](local_method_accumulator/local_method_accumulator.py) | [LocalMethodAccumulator](local_method_accumulator/local_method_accumulator.py) | compiler-generated | compiler-generated | — |
| [table_dispatch](table_dispatch/README.md) | migrated | [Python](table_dispatch/table_dispatch.py) | [TableDispatch](table_dispatch/table_dispatch.py) | compiler-generated | compiler-generated | — |
| [sibling_method_calls](sibling_method_calls/README.md) | migrated | [Python](sibling_method_calls/Top.py) | [Top](sibling_method_calls/Top.py) | compiler-generated | compiler-generated | — |
| [parent_state_interfaces](parent_state_interfaces/README.md) | migrated | [Python](parent_state_interfaces/top.py) | [Top](parent_state_interfaces/top.py) | compiler-generated | compiler-generated | — |
| [scalar_queue_pipeline](scalar_queue_pipeline/README.md) | migrated | [Python](scalar_queue_pipeline/Top.py) | [Top](scalar_queue_pipeline/Top.py) | compiler-generated | compiler-generated | — |
| [resource_composition](resource_composition/README.md) | migrated | [Python](resource_composition/Top.py) | [Top](resource_composition/Top.py) | compiler-generated | compiler-generated | — |
| [structured_packet](structured_packet/README.md) | migrated | [Python](structured_packet/Top.py) | [Top](structured_packet/Top.py) | compiler-generated | compiler-generated | — |
| [transactional_cumulative](transactional_cumulative/README.md) | migrated | [Python](transactional_cumulative/Top.py) | [Top](transactional_cumulative/Top.py) | compiler-generated | compiler-generated | — |
| [transactional_batch](transactional_batch/README.md) | migrated | [Python](transactional_batch/Top.py) | [Top](transactional_batch/Top.py) | compiler-generated | compiler-generated | — |
| [multi_outputs](multi_outputs/README.md) | pending-migration | [Python](multi_outputs/multi_outputs.py) | [MultiOutputsSystem](multi_outputs/bench.py) | compiler-generated | compiler-generated | — |
| [sibling_calls](sibling_calls/README.md) | migrated | [Python](sibling_calls/sibling_calls.py) | [SiblingCalls](sibling_calls/sibling_calls.py) | compiler-generated | compiler-generated | — |
| [sibling_atomic](sibling_atomic/README.md) | migrated | [Python](sibling_atomic/sibling_atomic.py) | [SiblingAtomic](sibling_atomic/bench.py) | compiler-generated | compiler-generated | — |
| [sibling_aliases](sibling_aliases/README.md) | migrated | [Python](sibling_aliases/sibling_aliases.py) | [SiblingAliases](sibling_aliases/bench.py) | compiler-generated | compiler-generated | — |
| [sibling_transitive](sibling_transitive/README.md) | pending-migration | [Python](sibling_transitive/sibling_transitive.py) | [SiblingTransitive](sibling_transitive/bench.py) | compiler-generated | compiler-generated | — |
| [multiclock_domain_isolation](multiclock_domain_isolation/README.md) | awaiting-capability | [Python](multiclock_domain_isolation/multiclock_domain_isolation.py) | — | [driver](multiclock_domain_isolation/driver.cpp) | [testbench](multiclock_domain_isolation/rtl_tb.sv) | [artifacts](multiclock_domain_isolation/GENERATED.md) |
| [rob](rob/README.md) | deferred | [Python](rob/rob.py) | [ExerciseRob](rob/bench.py) | [driver](rob/driver.cpp) | [testbench](rob/rtl_tb.sv) | [artifacts](rob/GENERATED.md) |
| [struct_transform](struct_transform/README.md) | pending-migration | [Python](struct_transform/struct_transform.py) | [ExerciseStructTransform](struct_transform/bench.py) | [driver](struct_transform/driver.cpp) | [testbench](struct_transform/rtl_tb.sv) | [artifacts](struct_transform/GENERATED.md) |
| [digital_filter](digital_filter/README.md) | pending-migration | [Python](digital_filter/digital_filter.py) | [ExerciseDigitalFilter](digital_filter/bench.py) | [driver](digital_filter/driver.cpp) | [testbench](digital_filter/rtl_tb.sv) | [artifacts](digital_filter/GENERATED.md) |
| [digital_clock](digital_clock/README.md) | pending-migration | [Python](digital_clock/digital_clock.py) | [ExerciseDigitalClock](digital_clock/bench.py) | [driver](digital_clock/driver.cpp) | [testbench](digital_clock/rtl_tb.sv) | [artifacts](digital_clock/GENERATED.md) |
| [enum_payload_pipeline](enum_payload_pipeline/README.md) | pending-migration | [Python](enum_payload_pipeline/enum_payload_pipeline.py) | [EnumPayloadPipelineSystem](enum_payload_pipeline/bench.py) | [driver](enum_payload_pipeline/driver.cpp) | [testbench](enum_payload_pipeline/rtl_tb.sv) | [artifacts](enum_payload_pipeline/GENERATED.md) |
| [queue_rule_pipeline](queue_rule_pipeline/README.md) | migrated | [Python](queue_rule_pipeline/queue_rule_pipeline.py) | [QueueRulePipelineSystem](queue_rule_pipeline/bench.py) | compiler-generated | compiler-generated | — |
| [fifo_loopback](fifo_loopback/README.md) | pending-migration | [Python](fifo_loopback/fifo_loopback.py) | [FifoLoopbackSystem](fifo_loopback/bench.py) | [driver](fifo_loopback/driver.cpp) | [testbench](fifo_loopback/rtl_tb.sv) | [artifacts](fifo_loopback/GENERATED.md) |
| [table_rule](table_rule/README.md) | pending-migration | [Python](table_rule/table_rule.py) | [ExerciseTableRule](table_rule/bench.py) | [driver](table_rule/driver.cpp) | [testbench](table_rule/rtl_tb.sv) | [artifacts](table_rule/GENERATED.md) |
| [issue_queue_2picker](issue_queue_2picker/README.md) | pending-migration | [Python](issue_queue_2picker/issue_queue_2picker.py) | [IssueQueue2PickerSystem](issue_queue_2picker/bench.py) | [driver](issue_queue_2picker/driver.cpp) | [testbench](issue_queue_2picker/rtl_tb.sv) | [artifacts](issue_queue_2picker/GENERATED.md) |
| [dodgeball_game](dodgeball_game/README.md) | pending-migration | [Python](dodgeball_game/dodgeball_game.py) | [ExerciseDodgeballGame](dodgeball_game/bench.py) | [driver](dodgeball_game/driver.cpp) | [testbench](dodgeball_game/rtl_tb.sv) | [artifacts](dodgeball_game/GENERATED.md) |
| [bf16_fmac](bf16_fmac/README.md) | pending-migration | [Python](bf16_fmac/bf16_fmac.py) | [ExerciseBF16Fmac](bf16_fmac/bench.py) | [driver](bf16_fmac/driver.cpp) | [testbench](bf16_fmac/rtl_tb.sv) | [artifacts](bf16_fmac/GENERATED.md) |
| [calculator](calculator/README.md) | pending-migration | [Python](calculator/calculator.py) | [ExerciseCalculator](calculator/bench.py) | [driver](calculator/driver.cpp) | [testbench](calculator/rtl_tb.sv) | [artifacts](calculator/GENERATED.md) |
| [dma](dma/README.md) | awaiting-capability | [Python](dma/dma.py) | [ExerciseDma](dma/bench.py) | [driver](dma/driver.cpp) | [testbench](dma/rtl_tb.sv) | [artifacts](dma/GENERATED.md) |
| [memory_banks](memory_banks/README.md) | awaiting-capability | [Python](memory_banks/memory_banks.py) | [ExerciseMemoryBanks](memory_banks/bench.py) | [driver](memory_banks/driver.cpp) | [testbench](memory_banks/rtl_tb.sv) | [artifacts](memory_banks/GENERATED.md) |

## Remaining migration work

These entries retain their design sources and independent verification assets.
Removing a design from execution selection does not count as a pass.

| Design | Remaining work |
| --- | --- |
| multi_outputs | Value-only Module results must become resource connections while preserving independent outputs and the shared Struct interface. |
| sibling_transitive | The original four-cycle guard/log history passes C++ workers 1/2 and RTL. The adapted Reader observes Method results inside its own Rule; original Rule-result -> Module-output -> consumer availability remains unvalidated and must be migrated without adding a cycle. |
| multiclock_domain_isolation | Transactional multi-clock authoring and scheduling are unfinished; preserve the physical-domain isolation design and oracle. |
| rob | Deferred by the user on 2026-11-01. The sequential Method core and complete 284-row System are preserved with the original 569-Work physical oracle. Generic fixes clear transaction routing; the current candidate still exceeds the separate 1 MiB physical-storage correspondence proof budget. Full execution and the same-Work external root boundary remain unverified. |
| struct_transform | The unchanged 72-row System history passes C++ workers 1/2 and RTL through an existing-instance Method, retaining old-Q transformation and capture timing. Original 149-Work physical and four-state drivers are retained but root admission/same-Work boundary adaptation remains incomplete. The System-adapter pass does not establish original physical coverage. |
| digital_filter | The FIR algorithm and state now use an existing-instance Method with the complete 177-row System adapter. Original physical sampling and four-state export boundaries remain pending. Cold generated C++ preparation exceeds the existing verifier build timeout; input history and hardware timing are unchanged. |
| digital_clock | Register the clock Rule and export State without changing the 50 MHz divider or full-duration rollover history. |
| enum_payload_pipeline | Old downstream_pop Queue capacity borrowing and raw protocol bindings require a timing migration; preserve the original independent Queue oracle. |
| fifo_loopback | Old downstream_pop Queue capacity borrowing and raw external FIFO ports require an explicit timing adapter or semantic clarification. |
| table_rule | Migrate old downstream_pop Queue admission and raw protocol binding while preserving old-entry responses, Table updates and stalls. |
| issue_queue_2picker | The existing-instance Method preserves both dependent shifts, insertion and invalid payloads. Generic nominal four-state ternaries now compile. The complete System, original physical sampling and four-state export checks remain pending; no output State or Queue delay is substituted. |
| dodgeball_game | Register the game Rule and export State while preserving original divider, VGA and game timing. |
| bf16_fmac | Migrate returned Rule and value-only helper Module boundaries while preserving four register stages and the independent finite-bit arithmetic oracle. |
| calculator | Replace candidate-state reads with ordinary locals and register/export the step transaction; preserve the complete keypad history. |
| dma | Transactional Memory admission is unfinished; preserve read-first memory, latency, arbitration, DMA histories and independent models. |
| memory_banks | Transactional Memory admission is unfinished; preserve four banks, read latency, routing, priority merge and independent models. |

## API-owned coverage

These are retained coverage obligations with named compiler test owners.
An owner or retained root is not a current passing execution claim;
the catalog preserves each root's existing scope limits.

| Case | Test owner | Source root |
| --- | --- | --- |
| inferred_module_pipeline | [test](../tests/compiler/lit/Source/Inputs/direct-call-graph.py) | — |
| inferred_nested_module_pipeline | [test](../tests/compiler/lit/Source/Inputs/direct-call-graph.py) | — |
| inferred_nested_rule | [test](../tests/compiler/lit/Source/Inputs/direct-call-graph.py) | — |
| inferred_stateful_module | [test](../tests/compiler/lit/Source/Inputs/direct-call-graph.py) | — |
| mem_rdw_olddata | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) | — |
| memory_busy | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) | — |
| memory_pipeline | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) | — |
| memory_simple | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) | — |
| reorder_pipeline | [test](../tests/compiler/lit/Source/Inputs/queue-source.py) | — |
| route_merge_pipeline | [test](../tests/compiler/lit/Source/Inputs/queue-source.py) | — |
| sync_mem_init_zero | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) | — |
| typed_integer_operations | [test](../tests/compiler/lit/Source/Inputs/static-divrem.py) | — |
| reset_invalidate_order_smoke | [test](../tests/compiler/lit/Source/source-historical-features.test) | [history_features.bench.ResetInvalidateOrderSystem](../tests/compiler/lit/Source/Inputs/history-features/bench.py) |
| trace_dsl_smoke | [test](../tests/compiler/lit/Source/source-historical-features.test) | [history_features.bench.TraceDslSystem](../tests/compiler/lit/Source/Inputs/history-features/bench.py) |
| xz_value_model_smoke | [test](../tests/compiler/lit/Source/source-historical-features.test) | [history_features.bench.XzValueModelSystem](../tests/compiler/lit/Source/Inputs/history-features/bench.py) |
| gfsim_expect_pipeline | [test](../tests/compiler/lit/Source/source-historical-pipelines.test) | [history_pipeline.gfsim_expect_pipeline.gfsim_expect_pipeline](../tests/compiler/lit/Source/Inputs/history-pipeline/gfsim_expect_pipeline.py) |
| credit_pipeline | [test](../tests/compiler/lit/Source/source-historical-pipelines.test) | [history_pipeline.pyc_credit_pipeline.pyc_credit_pipeline](../tests/compiler/lit/Source/Inputs/history-pipeline/pyc_credit_pipeline.py) |
| reusable_circular_rob | [test](../tests/compiler/lit/Source/source-historical-residents.test) | [history_resident.resident_systems.reusable_circular_rob](../tests/compiler/lit/Source/Inputs/history-resident/resident_systems.py) |
| reusable_oldest_ready_isq | [test](../tests/compiler/lit/Source/source-historical-residents.test) | [history_resident.resident_systems.reusable_oldest_ready_isq](../tests/compiler/lit/Source/Inputs/history-resident/resident_systems.py) |
| dependency_pipeline | [test](../tests/compiler/lit/Source/source-historical-scheduling.test) | [history_scheduling.scheduling_systems.pyc_dependency_pipeline](../tests/compiler/lit/Source/Inputs/history-scheduling/scheduling_systems.py) |
| schedule_v2 | [test](../tests/compiler/lit/Source/source-historical-scheduling.test) | [history_scheduling.scheduling_systems.schedule_v2](../tests/compiler/lit/Source/Inputs/history-scheduling/scheduling_systems.py) |
| scalar_parameterized_types | [test](../tests/compiler/lit/Source/source-historical-scalar-parameterized.test) | [history_scalar_parameterized.bench.ScalarParameterizedTypesSystem](../tests/compiler/lit/Source/Inputs/history-scalar-parameterized-types/bench.py) |
| slot_rule_mailbox | [test](../tests/compiler/lit/Source/source-historical-slot-issue.test) | [history_slot_issue.systems.slot_rule_mailbox](../tests/compiler/lit/Source/Inputs/history-slot-issue/systems.py) |
| issue | [test](../tests/compiler/lit/Source/source-historical-slot-issue.test) | [history_slot_issue.systems.resident_issue](../tests/compiler/lit/Source/Inputs/history-slot-issue/systems.py) |
| routed_dependency_pipeline | [test](../tests/compiler/lit/Source/source-historical-routed.test) | [history_routed.routed_systems.routed_dependency_pipeline](../tests/compiler/lit/Source/Inputs/history-routed/routed_systems.py) |

## Build and run

```sh
cmake -S examples -B /absolute/build/examples -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/examples --parallel 4
ctest --test-dir /absolute/build/examples --output-on-failure --no-tests=error
```

The aggregate build selects the current execution set from this inventory.
See [migration inventory](../docs/development/main-capability-migration.md)
for test ownership, retained obligations and the candidate validation policy.
