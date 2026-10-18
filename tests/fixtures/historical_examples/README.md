# Historical example assets

These 54 directories were removed from the current example catalog after the
2026-10-18 relevance review. They preserve the complete source, independent
oracles, vectors and historical generated receipts. Retirement is not a passing
test result, and these directories are not registered by the examples gate.

Existing API consumers still use their explicitly referenced fixtures (including
module_loop). Other assets are retained for semantic comparison and may be built
standalone with the existing CMake helper; preservation does not mean automatic
API coverage or successful current execution. Do not add a second runner here.

Generated receipts describe their original source/build locations. They are
historical evidence and must not be presented as fresh verification after this
move. Current designs and authoring guidance live in examples/GUIDE.md.

| Former example | Disposition |
| --- | --- |
| [module_loop](module_loop/README.md) | Merge teaching entry |
| [wire_ops](wire_ops/README.md) | API regression candidate |
| [arith](arith/README.md) | API regression candidate |
| [counter](counter/README.md) | Merge teaching entry |
| [hier_modules](hier_modules/README.md) | Merge teaching entry |
| [boundary_value_ports](boundary_value_ports/README.md) | API regression candidate |
| [net_resolution_depth_smoke](net_resolution_depth_smoke/README.md) | API regression candidate |
| [cache_params](cache_params/README.md) | API regression candidate |
| [multiclock_regs](multiclock_regs/README.md) | Merge teaching entry |
| [obs_points](obs_points/README.md) | Merge teaching entry |
| [decode_rules](decode_rules/README.md) | API regression candidate |
| [jit_control_flow](jit_control_flow/README.md) | Retire JIT naming |
| [pipeline_builder](pipeline_builder/README.md) | Merge teaching entry |
| [jit_pipeline_vec](jit_pipeline_vec/README.md) | Retire JIT naming |
| [fastfwd](fastfwd/README.md) | API regression candidate |
| [traffic_lights_ce_pyc](traffic_lights_ce_pyc/README.md) | API regression candidate |
| [bit_widths](bit_widths/README.md) | API regression candidate |
| [record_projection](record_projection/README.md) | API regression candidate |
| [record_projection_update](record_projection_update/README.md) | API regression candidate |
| [record_spread_pipeline](record_spread_pipeline/README.md) | API regression candidate |
| [encoded_enum_pipeline](encoded_enum_pipeline/README.md) | API regression candidate |
| [masked_decode_pipeline](masked_decode_pipeline/README.md) | API regression candidate |
| [bitfield_scalar_pipeline](bitfield_scalar_pipeline/README.md) | API regression candidate |
| [bitfield_decode_pipeline](bitfield_decode_pipeline/README.md) | API regression candidate |
| [inferred_boundary_pipeline](inferred_boundary_pipeline/README.md) | Merge teaching entry |
| [nested_payload_pipeline](nested_payload_pipeline/README.md) | API regression candidate |
| [aggregate_payload_pipeline](aggregate_payload_pipeline/README.md) | API regression candidate |
| [recursive_aggregate_payload_pipeline](recursive_aggregate_payload_pipeline/README.md) | API regression candidate |
| [rule_pipeline](rule_pipeline/README.md) | Merge teaching entry |
| [popcount_pipeline](popcount_pipeline/README.md) | API regression candidate |
| [queue_pipeline](queue_pipeline/README.md) | Merge teaching entry |
| [struct_pipeline](struct_pipeline/README.md) | Merge teaching entry |
| [rule_pair_pipeline](rule_pair_pipeline/README.md) | Merge teaching entry |
| [multi_input_rule_pipeline](multi_input_rule_pipeline/README.md) | Merge teaching entry |
| [latency_pipeline](latency_pipeline/README.md) | Merge teaching entry |
| [fork_pipeline](fork_pipeline/README.md) | Merge teaching entry |
| [barrier_pipeline](barrier_pipeline/README.md) | Merge teaching entry |
| [broadcast_pipeline](broadcast_pipeline/README.md) | Merge teaching entry |
| [conditional_pipeline](conditional_pipeline/README.md) | Merge teaching entry |
| [count_zeros_pipeline](count_zeros_pipeline/README.md) | API regression candidate |
| [bit_primitive_pipeline](bit_primitive_pipeline/README.md) | API regression candidate |
| [onehot_encode](onehot_encode/README.md) | API regression candidate |
| [frontend_composition_pipeline](frontend_composition_pipeline/README.md) | API regression candidate |
| [select_pipeline](select_pipeline/README.md) | Merge teaching entry |
| [enum_helpers](enum_helpers/README.md) | API regression candidate |
| [feedback_pipeline](feedback_pipeline/README.md) | Merge teaching entry |
| [loop_control_pipeline](loop_control_pipeline/README.md) | Merge teaching entry |
| [recursive_pipeline](recursive_pipeline/README.md) | Merge teaching entry |
| [bounded_full_u64](bounded_full_u64/README.md) | API regression candidate |
| [array_reductions](array_reductions/README.md) | API regression candidate |
| [array_combinators](array_combinators/README.md) | API regression candidate |
| [bounded_integer_operations](bounded_integer_operations/README.md) | API regression candidate |
| [recursive_array_updates](recursive_array_updates/README.md) | API regression candidate |
| [array_scans](array_scans/README.md) | API regression candidate |

The separate `system_benches/` directory retains old long-form benches when a
current example adopts a shorter teaching scenario. Their last failure or
not-run status stays documented; a new short scenario does not close that gap.

The separate `documentation/` directory preserves ten former README snapshots,
including historical cost comparisons removed from the active tutorials. These
are documentation records, outside the 54-directory source archive count.
