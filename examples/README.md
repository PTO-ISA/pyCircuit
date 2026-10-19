# Examples navigation

Examples use the single Python → MLIR → C++/Verilog → runner flow.
The current catalog inventories **17** registered designs and **25** API-owned coverage cases.

- [Recommended examples and learning path](GUIDE.md)
- [Writing and verification standard](STANDARD.md)
- [System execution](../docs/architecture/system-execution.md)
- [Machine-readable catalog](catalog.json)
- [Current language](../docs/reference/language.md)

[functional_float](functional_float/README.md) is a source-unit library
example rather than a registered `@system`; it is compiled and linked directly
with the public `pycircuit compile/link/emit` commands. Its README gives the
bounded Product/Sum command flow and does not add it to the system example
catalog.

## Full registered inventory

| Design | Source | System run | Native oracle | RTL oracle | Generated output |
| --- | --- | --- | --- | --- | --- |
| [hello_counter](hello_counter/README.md) | [Python](hello_counter/hello_counter.py) | [HelloCounter](hello_counter/hello_counter.py) | compiler-generated | compiler-generated | — |
| [multi_outputs](multi_outputs/README.md) | [Python](multi_outputs/multi_outputs.py) | [MultiOutputsSystem](multi_outputs/bench.py) | compiler-generated | compiler-generated | — |
| [multiclock_domain_isolation](multiclock_domain_isolation/README.md) | [Python](multiclock_domain_isolation/multiclock_domain_isolation.py) | — | [driver](multiclock_domain_isolation/driver.cpp) | [testbench](multiclock_domain_isolation/rtl_tb.sv) | [artifacts](multiclock_domain_isolation/GENERATED.md) |
| [rob](rob/README.md) | [Python](rob/rob.py) | [ExerciseRob](rob/bench.py) | [driver](rob/driver.cpp) | [testbench](rob/rtl_tb.sv) | [artifacts](rob/GENERATED.md) |
| [struct_transform](struct_transform/README.md) | [Python](struct_transform/struct_transform.py) | [ExerciseStructTransform](struct_transform/bench.py) | [driver](struct_transform/driver.cpp) | [testbench](struct_transform/rtl_tb.sv) | [artifacts](struct_transform/GENERATED.md) |
| [digital_filter](digital_filter/README.md) | [Python](digital_filter/digital_filter.py) | [ExerciseDigitalFilter](digital_filter/bench.py) | [driver](digital_filter/driver.cpp) | [testbench](digital_filter/rtl_tb.sv) | [artifacts](digital_filter/GENERATED.md) |
| [digital_clock](digital_clock/README.md) | [Python](digital_clock/digital_clock.py) | [ExerciseDigitalClock](digital_clock/bench.py) | [driver](digital_clock/driver.cpp) | [testbench](digital_clock/rtl_tb.sv) | [artifacts](digital_clock/GENERATED.md) |
| [enum_payload_pipeline](enum_payload_pipeline/README.md) | [Python](enum_payload_pipeline/enum_payload_pipeline.py) | [EnumPayloadPipelineSystem](enum_payload_pipeline/bench.py) | [driver](enum_payload_pipeline/driver.cpp) | [testbench](enum_payload_pipeline/rtl_tb.sv) | [artifacts](enum_payload_pipeline/GENERATED.md) |
| [queue_rule_pipeline](queue_rule_pipeline/README.md) | [Python](queue_rule_pipeline/queue_rule_pipeline.py) | [QueueRulePipelineSystem](queue_rule_pipeline/bench.py) | compiler-generated | compiler-generated | — |
| [fifo_loopback](fifo_loopback/README.md) | [Python](fifo_loopback/fifo_loopback.py) | [FifoLoopbackSystem](fifo_loopback/bench.py) | [driver](fifo_loopback/driver.cpp) | [testbench](fifo_loopback/rtl_tb.sv) | [artifacts](fifo_loopback/GENERATED.md) |
| [table_rule](table_rule/README.md) | [Python](table_rule/table_rule.py) | [ExerciseTableRule](table_rule/bench.py) | [driver](table_rule/driver.cpp) | [testbench](table_rule/rtl_tb.sv) | [artifacts](table_rule/GENERATED.md) |
| [issue_queue_2picker](issue_queue_2picker/README.md) | [Python](issue_queue_2picker/issue_queue_2picker.py) | [IssueQueue2PickerSystem](issue_queue_2picker/bench.py) | [driver](issue_queue_2picker/driver.cpp) | [testbench](issue_queue_2picker/rtl_tb.sv) | [artifacts](issue_queue_2picker/GENERATED.md) |
| [dodgeball_game](dodgeball_game/README.md) | [Python](dodgeball_game/dodgeball_game.py) | [ExerciseDodgeballGame](dodgeball_game/bench.py) | [driver](dodgeball_game/driver.cpp) | [testbench](dodgeball_game/rtl_tb.sv) | [artifacts](dodgeball_game/GENERATED.md) |
| [bf16_fmac](bf16_fmac/README.md) | [Python](bf16_fmac/bf16_fmac.py) | [ExerciseBF16Fmac](bf16_fmac/bench.py) | [driver](bf16_fmac/driver.cpp) | [testbench](bf16_fmac/rtl_tb.sv) | [artifacts](bf16_fmac/GENERATED.md) |
| [calculator](calculator/README.md) | [Python](calculator/calculator.py) | [ExerciseCalculator](calculator/bench.py) | [driver](calculator/driver.cpp) | [testbench](calculator/rtl_tb.sv) | [artifacts](calculator/GENERATED.md) |
| [dma](dma/README.md) | [Python](dma/dma.py) | [ExerciseDma](dma/bench.py) | [driver](dma/driver.cpp) | [testbench](dma/rtl_tb.sv) | [artifacts](dma/GENERATED.md) |
| [memory_banks](memory_banks/README.md) | [Python](memory_banks/memory_banks.py) | [ExerciseMemoryBanks](memory_banks/bench.py) | [driver](memory_banks/driver.cpp) | [testbench](memory_banks/rtl_tb.sv) | [artifacts](memory_banks/GENERATED.md) |

## API-owned coverage

These cases are covered by their current compiler test owners.

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

The aggregate build and this catalog contain the same runnable example set.
