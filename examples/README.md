# Examples navigation

Examples use the single Python → MLIR → C++/Verilog → runner flow.
The current catalog contains **69** runnable examples and **25** API-owned coverage cases.

- [Writing and verification standard](STANDARD.md)
- [System execution](../docs/architecture/system-execution.md)
- [Machine-readable catalog](catalog.json)
- [Current language](../docs/reference/language.md)

## Runnable examples

| Design | Source | System run | Native oracle | RTL oracle | Generated output |
| --- | --- | --- | --- | --- | --- |
| [hello_counter](hello_counter/README.md) | [Python](hello_counter/hello_counter.py) | [HelloCounter](hello_counter/hello_counter.py) | compiler-generated | compiler-generated | — |
| [module_loop](module_loop/README.md) | [Python](module_loop/module_loop.py) | — | [driver](module_loop/driver.cpp) | [testbench](module_loop/rtl_tb.sv) | [artifacts](module_loop/GENERATED.md) |
| [wire_ops](wire_ops/README.md) | [Python](wire_ops/wire_ops.py) | [ExerciseWireOps](wire_ops/bench.py) | [driver](wire_ops/driver.cpp) | [testbench](wire_ops/rtl_tb.sv) | [artifacts](wire_ops/GENERATED.md) |
| [arith](arith/README.md) | [Python](arith/arith.py) | [ExerciseArith](arith/bench.py) | [driver](arith/driver.cpp) | [testbench](arith/rtl_tb.sv) | [artifacts](arith/GENERATED.md) |
| [counter](counter/README.md) | [Python](counter/counter.py) | [ExerciseCounter](counter/bench.py) | [driver](counter/driver.cpp) | [testbench](counter/rtl_tb.sv) | [artifacts](counter/GENERATED.md) |
| [hier_modules](hier_modules/README.md) | [Python](hier_modules/hier_modules.py) | [ExerciseHierModules](hier_modules/bench.py) | [driver](hier_modules/driver.cpp) | [testbench](hier_modules/rtl_tb.sv) | [artifacts](hier_modules/GENERATED.md) |
| [boundary_value_ports](boundary_value_ports/README.md) | [Python](boundary_value_ports/boundary_value_ports.py) | [ExerciseBoundaryValuePorts](boundary_value_ports/bench.py) | [driver](boundary_value_ports/driver.cpp) | [testbench](boundary_value_ports/rtl_tb.sv) | [artifacts](boundary_value_ports/GENERATED.md) |
| [net_resolution_depth_smoke](net_resolution_depth_smoke/README.md) | [Python](net_resolution_depth_smoke/net_resolution_depth_smoke.py) | [ExerciseNetResolutionDepthSmoke](net_resolution_depth_smoke/bench.py) | [driver](net_resolution_depth_smoke/driver.cpp) | [testbench](net_resolution_depth_smoke/rtl_tb.sv) | [artifacts](net_resolution_depth_smoke/GENERATED.md) |
| [cache_params](cache_params/README.md) | [Python](cache_params/cache_params.py) | [ExerciseCacheParams](cache_params/bench.py) | [driver](cache_params/driver.cpp) | [testbench](cache_params/rtl_tb.sv) | [artifacts](cache_params/GENERATED.md) |
| [multiclock_regs](multiclock_regs/README.md) | [Python](multiclock_regs/multiclock_regs.py) | — | [driver](multiclock_regs/driver.cpp) | [testbench](multiclock_regs/rtl_tb.sv) | [artifacts](multiclock_regs/GENERATED.md) |
| multiclock_domain_isolation | [Python](multiclock_domain_isolation/multiclock_domain_isolation.py) | — | [driver](multiclock_domain_isolation/driver.cpp) | [testbench](multiclock_domain_isolation/rtl_tb.sv) | — |
| [obs_points](obs_points/README.md) | [Python](obs_points/obs_points.py) | [ExerciseObsPoints](obs_points/bench.py) | [driver](obs_points/driver.cpp) | [testbench](obs_points/rtl_tb.sv) | [artifacts](obs_points/GENERATED.md) |
| [decode_rules](decode_rules/README.md) | [Python](decode_rules/decode_rules.py) | [ExerciseDecodeRules](decode_rules/bench.py) | [driver](decode_rules/driver.cpp) | [testbench](decode_rules/rtl_tb.sv) | [artifacts](decode_rules/GENERATED.md) |
| [jit_control_flow](jit_control_flow/README.md) | [Python](jit_control_flow/jit_control_flow.py) | [ExerciseJitControlFlow](jit_control_flow/bench.py) | [driver](jit_control_flow/driver.cpp) | [testbench](jit_control_flow/rtl_tb.sv) | [artifacts](jit_control_flow/GENERATED.md) |
| [rob](rob/README.md) | [Python](rob/rob.py) | [ExerciseRob](rob/bench.py) | [driver](rob/driver.cpp) | [testbench](rob/rtl_tb.sv) | [artifacts](rob/GENERATED.md) |
| [struct_transform](struct_transform/README.md) | [Python](struct_transform/struct_transform.py) | [ExerciseStructTransform](struct_transform/bench.py) | [driver](struct_transform/driver.cpp) | [testbench](struct_transform/rtl_tb.sv) | [artifacts](struct_transform/GENERATED.md) |
| [pipeline_builder](pipeline_builder/README.md) | [Python](pipeline_builder/pipeline_builder.py) | [ExercisePipelineBuilder](pipeline_builder/bench.py) | [driver](pipeline_builder/driver.cpp) | [testbench](pipeline_builder/rtl_tb.sv) | [artifacts](pipeline_builder/GENERATED.md) |
| [jit_pipeline_vec](jit_pipeline_vec/README.md) | [Python](jit_pipeline_vec/jit_pipeline_vec.py) | [ExerciseJitPipelineVec](jit_pipeline_vec/bench.py) | [driver](jit_pipeline_vec/driver.cpp) | [testbench](jit_pipeline_vec/rtl_tb.sv) | [artifacts](jit_pipeline_vec/GENERATED.md) |
| [fastfwd](fastfwd/README.md) | [Python](fastfwd/fastfwd.py) | [FastfwdSystem](fastfwd/bench.py) | [driver](fastfwd/driver.cpp) | [testbench](fastfwd/rtl_tb.sv) | [artifacts](fastfwd/GENERATED.md) |
| [digital_filter](digital_filter/README.md) | [Python](digital_filter/digital_filter.py) | [ExerciseDigitalFilter](digital_filter/bench.py) | [driver](digital_filter/driver.cpp) | [testbench](digital_filter/rtl_tb.sv) | [artifacts](digital_filter/GENERATED.md) |
| [traffic_lights_ce_pyc](traffic_lights_ce_pyc/README.md) | [Python](traffic_lights_ce_pyc/traffic_lights_ce_pyc.py) | [ExerciseTopTrafficLights](traffic_lights_ce_pyc/bench.py) | [driver](traffic_lights_ce_pyc/driver.cpp) | [testbench](traffic_lights_ce_pyc/rtl_tb.sv) | [artifacts](traffic_lights_ce_pyc/GENERATED.md) |
| [digital_clock](digital_clock/README.md) | [Python](digital_clock/digital_clock.py) | [ExerciseDigitalClock](digital_clock/bench.py) | [driver](digital_clock/driver.cpp) | [testbench](digital_clock/rtl_tb.sv) | [artifacts](digital_clock/GENERATED.md) |
| [bit_widths](bit_widths/README.md) | [Python](bit_widths/bit_widths.py) | [BitWidthsSystem](bit_widths/bench.py) | [driver](bit_widths/driver.cpp) | [testbench](bit_widths/rtl_tb.sv) | [artifacts](bit_widths/GENERATED.md) |
| [record_projection](record_projection/README.md) | [Python](record_projection/record_projection.py) | [RecordProjectionSystem](record_projection/bench.py) | [driver](record_projection/driver.cpp) | [testbench](record_projection/rtl_tb.sv) | [artifacts](record_projection/GENERATED.md) |
| [record_projection_update](record_projection_update/README.md) | [Python](record_projection_update/record_projection_update.py) | [RecordProjectionUpdateSystem](record_projection_update/bench.py) | [driver](record_projection_update/driver.cpp) | [testbench](record_projection_update/rtl_tb.sv) | [artifacts](record_projection_update/GENERATED.md) |
| [record_spread_pipeline](record_spread_pipeline/README.md) | [Python](record_spread_pipeline/record_spread_pipeline.py) | [ExerciseRecordSpreadPipeline](record_spread_pipeline/bench.py) | [driver](record_spread_pipeline/driver.cpp) | [testbench](record_spread_pipeline/rtl_tb.sv) | [artifacts](record_spread_pipeline/GENERATED.md) |
| [encoded_enum_pipeline](encoded_enum_pipeline/README.md) | [Python](encoded_enum_pipeline/encoded_enum_pipeline.py) | [EncodedEnumPipelineSystem](encoded_enum_pipeline/bench.py) | [driver](encoded_enum_pipeline/driver.cpp) | [testbench](encoded_enum_pipeline/rtl_tb.sv) | [artifacts](encoded_enum_pipeline/GENERATED.md) |
| [enum_payload_pipeline](enum_payload_pipeline/README.md) | [Python](enum_payload_pipeline/enum_payload_pipeline.py) | [EnumPayloadPipelineSystem](enum_payload_pipeline/bench.py) | [driver](enum_payload_pipeline/driver.cpp) | [testbench](enum_payload_pipeline/rtl_tb.sv) | [artifacts](enum_payload_pipeline/GENERATED.md) |
| [masked_decode_pipeline](masked_decode_pipeline/README.md) | [Python](masked_decode_pipeline/masked_decode_pipeline.py) | [MaskedDecodePipelineSystem](masked_decode_pipeline/bench.py) | [driver](masked_decode_pipeline/driver.cpp) | [testbench](masked_decode_pipeline/rtl_tb.sv) | [artifacts](masked_decode_pipeline/GENERATED.md) |
| [bitfield_scalar_pipeline](bitfield_scalar_pipeline/README.md) | [Python](bitfield_scalar_pipeline/bitfield_scalar_pipeline.py) | [BitfieldScalarPipelineSystem](bitfield_scalar_pipeline/bench.py) | [driver](bitfield_scalar_pipeline/driver.cpp) | [testbench](bitfield_scalar_pipeline/rtl_tb.sv) | — |
| [bitfield_decode_pipeline](bitfield_decode_pipeline/README.md) | [Python](bitfield_decode_pipeline/bitfield_decode_pipeline.py) | [BitfieldDecodePipelineSystem](bitfield_decode_pipeline/bench.py) | [driver](bitfield_decode_pipeline/driver.cpp) | [testbench](bitfield_decode_pipeline/rtl_tb.sv) | [artifacts](bitfield_decode_pipeline/GENERATED.md) |
| [inferred_boundary_pipeline](inferred_boundary_pipeline/README.md) | [Python](inferred_boundary_pipeline/inferred_boundary_pipeline.py) | [InferredBoundaryPipelineSystem](inferred_boundary_pipeline/bench.py) | [driver](inferred_boundary_pipeline/driver.cpp) | [testbench](inferred_boundary_pipeline/rtl_tb.sv) | [artifacts](inferred_boundary_pipeline/GENERATED.md) |
| [nested_payload_pipeline](nested_payload_pipeline/README.md) | [Python](nested_payload_pipeline/nested_payload_pipeline.py) | [NestedPayloadPipelineSystem](nested_payload_pipeline/bench.py) | [driver](nested_payload_pipeline/driver.cpp) | [testbench](nested_payload_pipeline/rtl_tb.sv) | [artifacts](nested_payload_pipeline/GENERATED.md) |
| [aggregate_payload_pipeline](aggregate_payload_pipeline/README.md) | [Python](aggregate_payload_pipeline/aggregate_payload_pipeline.py) | [AggregatePayloadPipelineSystem](aggregate_payload_pipeline/bench.py) | [driver](aggregate_payload_pipeline/driver.cpp) | [testbench](aggregate_payload_pipeline/rtl_tb.sv) | [artifacts](aggregate_payload_pipeline/GENERATED.md) |
| [recursive_aggregate_payload_pipeline](recursive_aggregate_payload_pipeline/README.md) | [Python](recursive_aggregate_payload_pipeline/recursive_aggregate_payload_pipeline.py) | [RecursiveAggregatePayloadPipelineSystem](recursive_aggregate_payload_pipeline/bench.py) | [driver](recursive_aggregate_payload_pipeline/driver.cpp) | [testbench](recursive_aggregate_payload_pipeline/rtl_tb.sv) | [artifacts](recursive_aggregate_payload_pipeline/GENERATED.md) |
| [rule_pipeline](rule_pipeline/README.md) | [Python](rule_pipeline/rule_pipeline.py) | [RulePipelineSystem](rule_pipeline/bench.py) | [driver](rule_pipeline/driver.cpp) | [testbench](rule_pipeline/rtl_tb.sv) | [artifacts](rule_pipeline/GENERATED.md) |
| [popcount_pipeline](popcount_pipeline/README.md) | [Python](popcount_pipeline/popcount_pipeline.py) | [PopcountPipelineSystem](popcount_pipeline/bench.py) | [driver](popcount_pipeline/driver.cpp) | [testbench](popcount_pipeline/rtl_tb.sv) | [artifacts](popcount_pipeline/GENERATED.md) |
| [queue_pipeline](queue_pipeline/README.md) | [Python](queue_pipeline/queue_pipeline.py) | [QueuePipelineSystem](queue_pipeline/bench.py) | [driver](queue_pipeline/driver.cpp) | [testbench](queue_pipeline/rtl_tb.sv) | [artifacts](queue_pipeline/GENERATED.md) |
| [struct_pipeline](struct_pipeline/README.md) | [Python](struct_pipeline/struct_pipeline.py) | [StructPipelineSystem](struct_pipeline/bench.py) | [driver](struct_pipeline/driver.cpp) | [testbench](struct_pipeline/rtl_tb.sv) | — |
| [rule_pair_pipeline](rule_pair_pipeline/README.md) | [Python](rule_pair_pipeline/rule_pair_pipeline.py) | [RulePairPipelineSystem](rule_pair_pipeline/bench.py) | [driver](rule_pair_pipeline/driver.cpp) | [testbench](rule_pair_pipeline/rtl_tb.sv) | [artifacts](rule_pair_pipeline/GENERATED.md) |
| [multi_input_rule_pipeline](multi_input_rule_pipeline/README.md) | [Python](multi_input_rule_pipeline/multi_input_rule_pipeline.py) | [MultiInputRulePipelineSystem](multi_input_rule_pipeline/bench.py) | [driver](multi_input_rule_pipeline/driver.cpp) | [testbench](multi_input_rule_pipeline/rtl_tb.sv) | [artifacts](multi_input_rule_pipeline/GENERATED.md) |
| [latency_pipeline](latency_pipeline/README.md) | [Python](latency_pipeline/latency_pipeline.py) | [LatencyPipelineSystem](latency_pipeline/bench.py) | [driver](latency_pipeline/driver.cpp) | [testbench](latency_pipeline/rtl_tb.sv) | [artifacts](latency_pipeline/GENERATED.md) |
| [fifo_loopback](fifo_loopback/README.md) | [Python](fifo_loopback/fifo_loopback.py) | [FifoLoopbackSystem](fifo_loopback/bench.py) | [driver](fifo_loopback/driver.cpp) | [testbench](fifo_loopback/rtl_tb.sv) | [artifacts](fifo_loopback/GENERATED.md) |
| [fork_pipeline](fork_pipeline/README.md) | [Python](fork_pipeline/fork_pipeline.py) | [ForkPipelineSystem](fork_pipeline/bench.py) | [driver](fork_pipeline/driver.cpp) | [testbench](fork_pipeline/rtl_tb.sv) | [artifacts](fork_pipeline/GENERATED.md) |
| [barrier_pipeline](barrier_pipeline/README.md) | [Python](barrier_pipeline/barrier_pipeline.py) | [ExerciseBarrierPipeline](barrier_pipeline/bench.py) | [driver](barrier_pipeline/driver.cpp) | [testbench](barrier_pipeline/rtl_tb.sv) | [artifacts](barrier_pipeline/GENERATED.md) |
| [broadcast_pipeline](broadcast_pipeline/README.md) | [Python](broadcast_pipeline/broadcast_pipeline.py) | [BroadcastPipelineSystem](broadcast_pipeline/bench.py) | [driver](broadcast_pipeline/driver.cpp) | [testbench](broadcast_pipeline/rtl_tb.sv) | [artifacts](broadcast_pipeline/GENERATED.md) |
| [conditional_pipeline](conditional_pipeline/README.md) | [Python](conditional_pipeline/conditional_pipeline.py) | [ConditionalPipelineSystem](conditional_pipeline/bench.py) | [driver](conditional_pipeline/driver.cpp) | [testbench](conditional_pipeline/rtl_tb.sv) | [artifacts](conditional_pipeline/GENERATED.md) |
| [count_zeros_pipeline](count_zeros_pipeline/README.md) | [Python](count_zeros_pipeline/count_zeros_pipeline.py) | [CountZerosPipelineSystem](count_zeros_pipeline/bench.py) | [driver](count_zeros_pipeline/driver.cpp) | [testbench](count_zeros_pipeline/rtl_tb.sv) | [artifacts](count_zeros_pipeline/GENERATED.md) |
| [bit_primitive_pipeline](bit_primitive_pipeline/README.md) | [Python](bit_primitive_pipeline/bit_primitive_pipeline.py) | [BitPrimitivePipelineSystem](bit_primitive_pipeline/bench.py) | [driver](bit_primitive_pipeline/driver.cpp) | [testbench](bit_primitive_pipeline/rtl_tb.sv) | [artifacts](bit_primitive_pipeline/GENERATED.md) |
| [onehot_encode](onehot_encode/README.md) | [Python](onehot_encode/onehot_encode.py) | [OnehotEncodeSystem](onehot_encode/bench.py) | [driver](onehot_encode/driver.cpp) | [testbench](onehot_encode/rtl_tb.sv) | [artifacts](onehot_encode/GENERATED.md) |
| [frontend_composition_pipeline](frontend_composition_pipeline/README.md) | [Python](frontend_composition_pipeline/frontend_composition_pipeline.py) | [FrontendCompositionPipelineSystem](frontend_composition_pipeline/bench.py) | [driver](frontend_composition_pipeline/driver.cpp) | [testbench](frontend_composition_pipeline/rtl_tb.sv) | [artifacts](frontend_composition_pipeline/GENERATED.md) |
| [select_pipeline](select_pipeline/README.md) | [Python](select_pipeline/select_pipeline.py) | [ExerciseSelectPipeline](select_pipeline/bench.py) | [driver](select_pipeline/driver.cpp) | [testbench](select_pipeline/rtl_tb.sv) | [artifacts](select_pipeline/GENERATED.md) |
| [enum_helpers](enum_helpers/README.md) | [Python](enum_helpers/enum_helpers.py) | [EnumHelpersSystem](enum_helpers/bench.py) | [driver](enum_helpers/driver.cpp) | [testbench](enum_helpers/rtl_tb.sv) | — |
| [feedback_pipeline](feedback_pipeline/README.md) | [Python](feedback_pipeline/feedback_pipeline.py) | [ExerciseFeedbackPipeline](feedback_pipeline/bench.py) | [driver](feedback_pipeline/driver.cpp) | [testbench](feedback_pipeline/rtl_tb.sv) | [artifacts](feedback_pipeline/GENERATED.md) |
| [loop_control_pipeline](loop_control_pipeline/README.md) | [Python](loop_control_pipeline/loop_control_pipeline.py) | [ExerciseLoopControlPipeline](loop_control_pipeline/bench.py) | [driver](loop_control_pipeline/driver.cpp) | [testbench](loop_control_pipeline/rtl_tb.sv) | [artifacts](loop_control_pipeline/GENERATED.md) |
| [table_rule](table_rule/README.md) | [Python](table_rule/table_rule.py) | [ExerciseTableRule](table_rule/bench.py) | [driver](table_rule/driver.cpp) | [testbench](table_rule/rtl_tb.sv) | — |
| [issue_queue_2picker](issue_queue_2picker/README.md) | [Python](issue_queue_2picker/issue_queue_2picker.py) | [IssueQueue2PickerSystem](issue_queue_2picker/bench.py) | [driver](issue_queue_2picker/driver.cpp) | [testbench](issue_queue_2picker/rtl_tb.sv) | [artifacts](issue_queue_2picker/GENERATED.md) |
| [recursive_pipeline](recursive_pipeline/README.md) | [Python](recursive_pipeline/recursive_pipeline.py) | [RecursivePipelineSystem](recursive_pipeline/bench.py) | [driver](recursive_pipeline/driver.cpp) | [testbench](recursive_pipeline/rtl_tb.sv) | [artifacts](recursive_pipeline/GENERATED.md) |
| [bounded_full_u64](bounded_full_u64/README.md) | [Python](bounded_full_u64/bounded_full_u64.py) | [BoundedFullU64System](bounded_full_u64/bench.py) | [driver](bounded_full_u64/driver.cpp) | [testbench](bounded_full_u64/rtl_tb.sv) | [artifacts](bounded_full_u64/GENERATED.md) |
| [dodgeball_game](dodgeball_game/README.md) | [Python](dodgeball_game/dodgeball_game.py) | [ExerciseDodgeballGame](dodgeball_game/bench.py) | [driver](dodgeball_game/driver.cpp) | [testbench](dodgeball_game/rtl_tb.sv) | [artifacts](dodgeball_game/GENERATED.md) |
| [bf16_fmac](bf16_fmac/README.md) | [Python](bf16_fmac/bf16_fmac.py) | [ExerciseBF16Fmac](bf16_fmac/bench.py) | [driver](bf16_fmac/driver.cpp) | [testbench](bf16_fmac/rtl_tb.sv) | [artifacts](bf16_fmac/GENERATED.md) |
| [calculator](calculator/README.md) | [Python](calculator/calculator.py) | [ExerciseCalculator](calculator/bench.py) | [driver](calculator/driver.cpp) | [testbench](calculator/rtl_tb.sv) | — |
| [dma](dma/README.md) | [Python](dma/dma.py) | [ExerciseDma](dma/bench.py) | [driver](dma/driver.cpp) | [testbench](dma/rtl_tb.sv) | [artifacts](dma/GENERATED.md) |
| [memory_banks](memory_banks/README.md) | [Python](memory_banks/memory_banks.py) | [ExerciseMemoryBanks](memory_banks/bench.py) | [driver](memory_banks/driver.cpp) | [testbench](memory_banks/rtl_tb.sv) | [artifacts](memory_banks/GENERATED.md) |
| [array_reductions](array_reductions/README.md) | [Python](array_reductions/array_reductions.py) | [ArrayReductionsSystem](array_reductions/bench.py) | [driver](array_reductions/driver.cpp) | [testbench](array_reductions/rtl_tb.sv) | [artifacts](array_reductions/GENERATED.md) |
| [array_combinators](array_combinators/README.md) | [Python](array_combinators/array_combinators.py) | [ArrayCombinatorsSystem](array_combinators/bench.py) | [driver](array_combinators/driver.cpp) | [testbench](array_combinators/rtl_tb.sv) | [artifacts](array_combinators/GENERATED.md) |
| [bounded_integer_operations](bounded_integer_operations/README.md) | [Python](bounded_integer_operations/bounded_integer_operations.py) | [BoundedIntegerOperationsSystem](bounded_integer_operations/bench.py) | [driver](bounded_integer_operations/driver.cpp) | [testbench](bounded_integer_operations/rtl_tb.sv) | [artifacts](bounded_integer_operations/GENERATED.md) |
| [recursive_array_updates](recursive_array_updates/README.md) | [Python](recursive_array_updates/recursive_array_updates.py) | [RecursiveArrayUpdatesSystem](recursive_array_updates/bench.py) | [driver](recursive_array_updates/driver.cpp) | [testbench](recursive_array_updates/rtl_tb.sv) | [artifacts](recursive_array_updates/GENERATED.md) |
| [array_scans](array_scans/README.md) | [Python](array_scans/array_scans.py) | [ArrayScansSystem](array_scans/bench.py) | [driver](array_scans/driver.cpp) | [testbench](array_scans/rtl_tb.sv) | [artifacts](array_scans/GENERATED.md) |

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
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/examples --parallel 4
ctest --test-dir /absolute/build/examples --output-on-failure --no-tests=error
```

The aggregate build and this catalog contain the same runnable example set.
