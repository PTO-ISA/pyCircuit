# Examples navigation

Examples use the single Python → MLIR → C++/Verilog → runner flow.
The current catalog contains **62** runnable examples and **12** API-owned coverage cases.

- [Writing and verification standard](STANDARD.md)
- [System execution](../docs/architecture/system-execution.md)
- [Machine-readable catalog](catalog.json)
- [Current language](../docs/reference/language.md)

## Runnable examples

| Design | Source | Native test | RTL test | Generated output |
| --- | --- | --- | --- | --- |
| [hello_counter](hello_counter/README.md) | [Python](hello_counter/hello_counter.py) | compiler-generated | compiler-generated | — |
| [module_loop](module_loop/README.md) | [Python](module_loop/module_loop.py) | [driver](module_loop/driver.cpp) | [testbench](module_loop/rtl_tb.sv) | [artifacts](module_loop/GENERATED.md) |
| [wire_ops](wire_ops/README.md) | [Python](wire_ops/wire_ops.py) | [driver](wire_ops/driver.cpp) | [testbench](wire_ops/rtl_tb.sv) | [artifacts](wire_ops/GENERATED.md) |
| [arith](arith/README.md) | [Python](arith/arith.py) | [driver](arith/driver.cpp) | [testbench](arith/rtl_tb.sv) | [artifacts](arith/GENERATED.md) |
| [counter](counter/README.md) | [Python](counter/counter.py) | [driver](counter/driver.cpp) | [testbench](counter/rtl_tb.sv) | [artifacts](counter/GENERATED.md) |
| [hier_modules](hier_modules/README.md) | [Python](hier_modules/hier_modules.py) | [driver](hier_modules/driver.cpp) | [testbench](hier_modules/rtl_tb.sv) | [artifacts](hier_modules/GENERATED.md) |
| [boundary_value_ports](boundary_value_ports/README.md) | [Python](boundary_value_ports/boundary_value_ports.py) | [driver](boundary_value_ports/driver.cpp) | [testbench](boundary_value_ports/rtl_tb.sv) | [artifacts](boundary_value_ports/GENERATED.md) |
| [net_resolution_depth_smoke](net_resolution_depth_smoke/README.md) | [Python](net_resolution_depth_smoke/net_resolution_depth_smoke.py) | [driver](net_resolution_depth_smoke/driver.cpp) | [testbench](net_resolution_depth_smoke/rtl_tb.sv) | [artifacts](net_resolution_depth_smoke/GENERATED.md) |
| [cache_params](cache_params/README.md) | [Python](cache_params/cache_params.py) | [driver](cache_params/driver.cpp) | [testbench](cache_params/rtl_tb.sv) | [artifacts](cache_params/GENERATED.md) |
| [multiclock_regs](multiclock_regs/README.md) | [Python](multiclock_regs/multiclock_regs.py) | [driver](multiclock_regs/driver.cpp) | [testbench](multiclock_regs/rtl_tb.sv) | [artifacts](multiclock_regs/GENERATED.md) |
| multiclock_domain_isolation | [Python](multiclock_domain_isolation/multiclock_domain_isolation.py) | [driver](multiclock_domain_isolation/driver.cpp) | [testbench](multiclock_domain_isolation/rtl_tb.sv) | — |
| [obs_points](obs_points/README.md) | [Python](obs_points/obs_points.py) | [driver](obs_points/driver.cpp) | [testbench](obs_points/rtl_tb.sv) | [artifacts](obs_points/GENERATED.md) |
| [decode_rules](decode_rules/README.md) | [Python](decode_rules/decode_rules.py) | [driver](decode_rules/driver.cpp) | [testbench](decode_rules/rtl_tb.sv) | [artifacts](decode_rules/GENERATED.md) |
| [jit_control_flow](jit_control_flow/README.md) | [Python](jit_control_flow/jit_control_flow.py) | [driver](jit_control_flow/driver.cpp) | [testbench](jit_control_flow/rtl_tb.sv) | [artifacts](jit_control_flow/GENERATED.md) |
| [rob](rob/README.md) | [Python](rob/rob.py) | [driver](rob/driver.cpp) | [testbench](rob/rtl_tb.sv) | [artifacts](rob/GENERATED.md) |
| [struct_transform](struct_transform/README.md) | [Python](struct_transform/struct_transform.py) | [driver](struct_transform/driver.cpp) | [testbench](struct_transform/rtl_tb.sv) | [artifacts](struct_transform/GENERATED.md) |
| [pipeline_builder](pipeline_builder/README.md) | [Python](pipeline_builder/pipeline_builder.py) | [driver](pipeline_builder/driver.cpp) | [testbench](pipeline_builder/rtl_tb.sv) | [artifacts](pipeline_builder/GENERATED.md) |
| [jit_pipeline_vec](jit_pipeline_vec/README.md) | [Python](jit_pipeline_vec/jit_pipeline_vec.py) | [driver](jit_pipeline_vec/driver.cpp) | [testbench](jit_pipeline_vec/rtl_tb.sv) | [artifacts](jit_pipeline_vec/GENERATED.md) |
| [fastfwd](fastfwd/README.md) | [Python](fastfwd/fastfwd.py) | [driver](fastfwd/driver.cpp) | [testbench](fastfwd/rtl_tb.sv) | [artifacts](fastfwd/GENERATED.md) |
| [digital_filter](digital_filter/README.md) | [Python](digital_filter/digital_filter.py) | [driver](digital_filter/driver.cpp) | [testbench](digital_filter/rtl_tb.sv) | [artifacts](digital_filter/GENERATED.md) |
| [traffic_lights_ce_pyc](traffic_lights_ce_pyc/README.md) | [Python](traffic_lights_ce_pyc/traffic_lights_ce_pyc.py) | [driver](traffic_lights_ce_pyc/driver.cpp) | [testbench](traffic_lights_ce_pyc/rtl_tb.sv) | [artifacts](traffic_lights_ce_pyc/GENERATED.md) |
| [digital_clock](digital_clock/README.md) | [Python](digital_clock/digital_clock.py) | [driver](digital_clock/driver.cpp) | [testbench](digital_clock/rtl_tb.sv) | [artifacts](digital_clock/GENERATED.md) |
| [bit_widths](bit_widths/README.md) | [Python](bit_widths/bit_widths.py) | [driver](bit_widths/driver.cpp) | [testbench](bit_widths/rtl_tb.sv) | [artifacts](bit_widths/GENERATED.md) |
| [record_projection](record_projection/README.md) | [Python](record_projection/record_projection.py) | [driver](record_projection/driver.cpp) | [testbench](record_projection/rtl_tb.sv) | [artifacts](record_projection/GENERATED.md) |
| [record_projection_update](record_projection_update/README.md) | [Python](record_projection_update/record_projection_update.py) | [driver](record_projection_update/driver.cpp) | [testbench](record_projection_update/rtl_tb.sv) | [artifacts](record_projection_update/GENERATED.md) |
| [record_spread_pipeline](record_spread_pipeline/README.md) | [Python](record_spread_pipeline/record_spread_pipeline.py) | [driver](record_spread_pipeline/driver.cpp) | [testbench](record_spread_pipeline/rtl_tb.sv) | [artifacts](record_spread_pipeline/GENERATED.md) |
| [encoded_enum_pipeline](encoded_enum_pipeline/README.md) | [Python](encoded_enum_pipeline/encoded_enum_pipeline.py) | [driver](encoded_enum_pipeline/driver.cpp) | [testbench](encoded_enum_pipeline/rtl_tb.sv) | [artifacts](encoded_enum_pipeline/GENERATED.md) |
| [enum_payload_pipeline](enum_payload_pipeline/README.md) | [Python](enum_payload_pipeline/enum_payload_pipeline.py) | [driver](enum_payload_pipeline/driver.cpp) | [testbench](enum_payload_pipeline/rtl_tb.sv) | [artifacts](enum_payload_pipeline/GENERATED.md) |
| [masked_decode_pipeline](masked_decode_pipeline/README.md) | [Python](masked_decode_pipeline/masked_decode_pipeline.py) | [driver](masked_decode_pipeline/driver.cpp) | [testbench](masked_decode_pipeline/rtl_tb.sv) | [artifacts](masked_decode_pipeline/GENERATED.md) |
| [bitfield_scalar_pipeline](bitfield_scalar_pipeline/README.md) | [Python](bitfield_scalar_pipeline/bitfield_scalar_pipeline.py) | [driver](bitfield_scalar_pipeline/driver.cpp) | [testbench](bitfield_scalar_pipeline/rtl_tb.sv) | — |
| [bitfield_decode_pipeline](bitfield_decode_pipeline/README.md) | [Python](bitfield_decode_pipeline/bitfield_decode_pipeline.py) | [driver](bitfield_decode_pipeline/driver.cpp) | [testbench](bitfield_decode_pipeline/rtl_tb.sv) | — |
| [inferred_boundary_pipeline](inferred_boundary_pipeline/README.md) | [Python](inferred_boundary_pipeline/inferred_boundary_pipeline.py) | [driver](inferred_boundary_pipeline/driver.cpp) | [testbench](inferred_boundary_pipeline/rtl_tb.sv) | — |
| [nested_payload_pipeline](nested_payload_pipeline/README.md) | [Python](nested_payload_pipeline/nested_payload_pipeline.py) | [driver](nested_payload_pipeline/driver.cpp) | [testbench](nested_payload_pipeline/rtl_tb.sv) | [artifacts](nested_payload_pipeline/GENERATED.md) |
| [rule_pipeline](rule_pipeline/README.md) | [Python](rule_pipeline/rule_pipeline.py) | [driver](rule_pipeline/driver.cpp) | [testbench](rule_pipeline/rtl_tb.sv) | [artifacts](rule_pipeline/GENERATED.md) |
| [popcount_pipeline](popcount_pipeline/README.md) | [Python](popcount_pipeline/popcount_pipeline.py) | [driver](popcount_pipeline/driver.cpp) | [testbench](popcount_pipeline/rtl_tb.sv) | [artifacts](popcount_pipeline/GENERATED.md) |
| [queue_pipeline](queue_pipeline/README.md) | [Python](queue_pipeline/queue_pipeline.py) | [driver](queue_pipeline/driver.cpp) | [testbench](queue_pipeline/rtl_tb.sv) | [artifacts](queue_pipeline/GENERATED.md) |
| [struct_pipeline](struct_pipeline/README.md) | [Python](struct_pipeline/struct_pipeline.py) | [driver](struct_pipeline/driver.cpp) | [testbench](struct_pipeline/rtl_tb.sv) | — |
| [rule_pair_pipeline](rule_pair_pipeline/README.md) | [Python](rule_pair_pipeline/rule_pair_pipeline.py) | [driver](rule_pair_pipeline/driver.cpp) | [testbench](rule_pair_pipeline/rtl_tb.sv) | [artifacts](rule_pair_pipeline/GENERATED.md) |
| [multi_input_rule_pipeline](multi_input_rule_pipeline/README.md) | [Python](multi_input_rule_pipeline/multi_input_rule_pipeline.py) | [driver](multi_input_rule_pipeline/driver.cpp) | [testbench](multi_input_rule_pipeline/rtl_tb.sv) | [artifacts](multi_input_rule_pipeline/GENERATED.md) |
| [latency_pipeline](latency_pipeline/README.md) | [Python](latency_pipeline/latency_pipeline.py) | [driver](latency_pipeline/driver.cpp) | [testbench](latency_pipeline/rtl_tb.sv) | [artifacts](latency_pipeline/GENERATED.md) |
| [fifo_loopback](fifo_loopback/README.md) | [Python](fifo_loopback/fifo_loopback.py) | [driver](fifo_loopback/driver.cpp) | [testbench](fifo_loopback/rtl_tb.sv) | [artifacts](fifo_loopback/GENERATED.md) |
| [fork_pipeline](fork_pipeline/README.md) | [Python](fork_pipeline/fork_pipeline.py) | [driver](fork_pipeline/driver.cpp) | [testbench](fork_pipeline/rtl_tb.sv) | [artifacts](fork_pipeline/GENERATED.md) |
| [barrier_pipeline](barrier_pipeline/README.md) | [Python](barrier_pipeline/barrier_pipeline.py) | [driver](barrier_pipeline/driver.cpp) | [testbench](barrier_pipeline/rtl_tb.sv) | [artifacts](barrier_pipeline/GENERATED.md) |
| [broadcast_pipeline](broadcast_pipeline/README.md) | [Python](broadcast_pipeline/broadcast_pipeline.py) | [driver](broadcast_pipeline/driver.cpp) | [testbench](broadcast_pipeline/rtl_tb.sv) | [artifacts](broadcast_pipeline/GENERATED.md) |
| [conditional_pipeline](conditional_pipeline/README.md) | [Python](conditional_pipeline/conditional_pipeline.py) | [driver](conditional_pipeline/driver.cpp) | [testbench](conditional_pipeline/rtl_tb.sv) | [artifacts](conditional_pipeline/GENERATED.md) |
| [count_zeros_pipeline](count_zeros_pipeline/README.md) | [Python](count_zeros_pipeline/count_zeros_pipeline.py) | [driver](count_zeros_pipeline/driver.cpp) | [testbench](count_zeros_pipeline/rtl_tb.sv) | [artifacts](count_zeros_pipeline/GENERATED.md) |
| [bit_primitive_pipeline](bit_primitive_pipeline/README.md) | [Python](bit_primitive_pipeline/bit_primitive_pipeline.py) | [driver](bit_primitive_pipeline/driver.cpp) | [testbench](bit_primitive_pipeline/rtl_tb.sv) | — |
| [onehot_encode](onehot_encode/README.md) | [Python](onehot_encode/onehot_encode.py) | [driver](onehot_encode/driver.cpp) | [testbench](onehot_encode/rtl_tb.sv) | [artifacts](onehot_encode/GENERATED.md) |
| [frontend_composition_pipeline](frontend_composition_pipeline/README.md) | [Python](frontend_composition_pipeline/frontend_composition_pipeline.py) | [driver](frontend_composition_pipeline/driver.cpp) | [testbench](frontend_composition_pipeline/rtl_tb.sv) | — |
| [select_pipeline](select_pipeline/README.md) | [Python](select_pipeline/select_pipeline.py) | [driver](select_pipeline/driver.cpp) | [testbench](select_pipeline/rtl_tb.sv) | — |
| [enum_helpers](enum_helpers/README.md) | [Python](enum_helpers/enum_helpers.py) | [driver](enum_helpers/driver.cpp) | [testbench](enum_helpers/rtl_tb.sv) | — |
| [feedback_pipeline](feedback_pipeline/README.md) | [Python](feedback_pipeline/feedback_pipeline.py) | [driver](feedback_pipeline/driver.cpp) | [testbench](feedback_pipeline/rtl_tb.sv) | — |
| [loop_control_pipeline](loop_control_pipeline/README.md) | [Python](loop_control_pipeline/loop_control_pipeline.py) | [driver](loop_control_pipeline/driver.cpp) | [testbench](loop_control_pipeline/rtl_tb.sv) | — |
| [table_rule](table_rule/README.md) | [Python](table_rule/table_rule.py) | [driver](table_rule/driver.cpp) | [testbench](table_rule/rtl_tb.sv) | — |
| [issue_queue_2picker](issue_queue_2picker/README.md) | [Python](issue_queue_2picker/issue_queue_2picker.py) | [driver](issue_queue_2picker/driver.cpp) | [testbench](issue_queue_2picker/rtl_tb.sv) | [artifacts](issue_queue_2picker/GENERATED.md) |
| [recursive_pipeline](recursive_pipeline/README.md) | [Python](recursive_pipeline/recursive_pipeline.py) | [driver](recursive_pipeline/driver.cpp) | [testbench](recursive_pipeline/rtl_tb.sv) | [artifacts](recursive_pipeline/GENERATED.md) |
| [bounded_full_u64](bounded_full_u64/README.md) | [Python](bounded_full_u64/bounded_full_u64.py) | [driver](bounded_full_u64/driver.cpp) | [testbench](bounded_full_u64/rtl_tb.sv) | [artifacts](bounded_full_u64/GENERATED.md) |
| [dodgeball_game](dodgeball_game/README.md) | [Python](dodgeball_game/dodgeball_game.py) | [driver](dodgeball_game/driver.cpp) | [testbench](dodgeball_game/rtl_tb.sv) | [artifacts](dodgeball_game/GENERATED.md) |
| [bf16_fmac](bf16_fmac/README.md) | [Python](bf16_fmac/bf16_fmac.py) | [driver](bf16_fmac/driver.cpp) | [testbench](bf16_fmac/rtl_tb.sv) | [artifacts](bf16_fmac/GENERATED.md) |
| [calculator](calculator/README.md) | [Python](calculator/calculator.py) | [driver](calculator/driver.cpp) | [testbench](calculator/rtl_tb.sv) | — |
| [dma](dma/README.md) | [Python](dma/dma.py) | [driver](dma/driver.cpp) | [testbench](dma/rtl_tb.sv) | [artifacts](dma/GENERATED.md) |
| [memory_banks](memory_banks/README.md) | [Python](memory_banks/memory_banks.py) | [driver](memory_banks/driver.cpp) | [testbench](memory_banks/rtl_tb.sv) | [artifacts](memory_banks/GENERATED.md) |

## API-owned coverage

These cases are covered by their current compiler test owners.

| Case | Test owner |
| --- | --- |
| inferred_module_pipeline | [test](../tests/compiler/lit/Source/Inputs/direct-call-graph.py) |
| inferred_nested_module_pipeline | [test](../tests/compiler/lit/Source/Inputs/direct-call-graph.py) |
| inferred_nested_rule | [test](../tests/compiler/lit/Source/Inputs/direct-call-graph.py) |
| inferred_stateful_module | [test](../tests/compiler/lit/Source/Inputs/direct-call-graph.py) |
| mem_rdw_olddata | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) |
| memory_busy | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) |
| memory_pipeline | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) |
| memory_simple | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) |
| reorder_pipeline | [test](../tests/compiler/lit/Source/Inputs/queue-source.py) |
| route_merge_pipeline | [test](../tests/compiler/lit/Source/Inputs/queue-source.py) |
| sync_mem_init_zero | [test](../tests/compiler/lit/Source/Inputs/pythonic-memory.py) |
| typed_integer_operations | [test](../tests/compiler/lit/Source/Inputs/static-divrem.py) |

## Build and run

```sh
cmake -S examples -B /absolute/build/examples -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/examples --parallel 4
ctest --test-dir /absolute/build/examples --output-on-failure --no-tests=error
```

The aggregate build and this catalog contain the same runnable example set.
