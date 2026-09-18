// RUN: %acir_queue_plan %s > /dev/null
// RUN: %acir_queue_cxxgen %s | %FileCheck %s --check-prefix=GFSIM
// RUN: %acir_queue_pycgen %s | %FileCheck %s --check-prefix=PYC

// A Table may carry both a field-mode and a replace-mode firing write when every
// conflicting endpoint declares explicit writer arbitration.  Both backends must
// then commit in gfsim's order: every FieldMerge against the committed image
// first, then every Replace on top of the field pass, so the replace writer wins.
// A single block-order accumulator would apply the field writer last instead.

module attributes {ac.frozen_owners = [], ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "two_rule", ac.topology_frozen = true} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "admitted", type = i1}, {name = "src_ready", type = i1}, {name = "tag", type = i4}]
    ac.struct @Update fields [{name = "index", type = i2}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}, !ac.struct<@types::@Update> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.table @entries entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/entries"
  %0 = ac.source depth 2 latency 1 {ac.name = "updates"} : !ac.queue<!ac.struct<@types::@Update>>
  %1:2 = ac.broadcast %0 depths [1, 1] latencies [1, 1] {ac.output_names = ["updates__fanout0", "updates__fanout1"]} : !ac.queue<!ac.struct<@types::@Update>> -> (!ac.queue<!ac.struct<@types::@Update>>, !ac.queue<!ac.struct<@types::@Update>>)
  %2 = ac.firing %1#0 depths [1] latencies [1] stable_id "first" domain "cycle" {
  ^bb0(%arg0: !ac.var<!ac.struct<@types::@Update>>):
    %4 = ac.var.constant true as !ac.var<i1>
    ac.firing.condition %4 : !ac.var<i1>
    %5 = ac.var.get %arg0 field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %6 = ac.table.get @entries[%5] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %7 = ac.var.with %6, %4 field "admitted" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @entries[%5] = %7 when %4 : !ac.var<i1> mode "replace" write_fields ["admitted", "src_ready", "tag"] {ac.arbitration = #ac.writer_priority<0>, ac.endpoint_id = "writer/admit"} : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.firing.output %6 when %4 ordinal 0 : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1>
    ac.state.snapshot @entries[%5 : !ac.var<i2>] for %4 : !ac.var<i1> kind  dynamic read_fields ["admitted", "src_ready", "tag"]
    ac.firing.yield %6 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @entries}], ac.arbitration_membership = [{declared_rank = 0 : i64, endpoint_stable_id = "first", owner = @entries, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>}], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @entries}, {declared_rank = 0 : i64, endpoint_stable_id = "first", guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, owner = @entries, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>, resource = @entries}], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "first", ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "admit", ac.rule_footprints = [{access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @entries}, {access = "replace", fields = ["admitted", "src_ready", "tag"], guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @entries}], ac.rule_priority = 0 : i64, ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>, ac.source_column = 1 : i64, ac.source_file = "<queue-model>", ac.source_line = 17 : i64, ac.state_accesses = [{guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind dynamic>, kind = #ac<rule_state_access_kind read>, resource = @entries}, {fields = ["admitted", "src_ready", "tag"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind dynamic>, kind = #ac<rule_state_access_kind replace>, resource = @entries}], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @entries}]} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  %3 = ac.firing %1#1 depths [1] latencies [1] stable_id "second" domain "cycle" {
  ^bb0(%arg0: !ac.var<!ac.struct<@types::@Update>>):
    %4 = ac.var.constant true as !ac.var<i1>
    ac.firing.condition %4 : !ac.var<i1>
    %5 = ac.var.get %arg0 field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %6 = ac.table.get @entries[%5] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %7 = ac.var.with %6, %4 field "src_ready" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @entries[%5] = %7 when %4 : !ac.var<i1> mode "field" write_fields ["src_ready"] {ac.arbitration = #ac.writer_priority<1>, ac.endpoint_id = "writer/wake"} : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.firing.output %6 when %4 ordinal 0 : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1>
    ac.state.snapshot @entries[%5 : !ac.var<i2>] for %4 : !ac.var<i1> kind  dynamic read_fields ["admitted", "src_ready", "tag"]
    ac.firing.yield %6 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @entries}], ac.arbitration_membership = [{declared_rank = 1 : i64, endpoint_stable_id = "second", owner = @entries, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>}], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @entries}, {declared_rank = 1 : i64, endpoint_stable_id = "second", guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, owner = @entries, policy = #ac<writer_arbitration_policy priority>, resolution = #ac<writer_arbitration_resolution winner_takes_transaction>, resource = @entries}], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "second", ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "wake", ac.rule_footprints = [{access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @entries}, {access = "field", fields = ["src_ready"], guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @entries}], ac.rule_priority = 1 : i64, ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>, ac.source_column = 1 : i64, ac.source_file = "<queue-model>", ac.source_line = 24 : i64, ac.state_accesses = [{guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind dynamic>, kind = #ac<rule_state_access_kind read>, resource = @entries}, {fields = ["src_ready"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind dynamic>, kind = #ac<rule_state_access_kind field_write>, resource = @entries}], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @entries}]} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %2 {ac.name = "sink_4"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %3 {ac.name = "sink_5"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// The write mode is selected per state write, not per block: the replace rule
// keeps Replace and the field rule keeps FieldMerge.
// GFSIM: gfsim::TableWriteMode::Replace,
// GFSIM: gfsim::TableWriteMode::FieldMerge,

// The committed image of one Entry anchors every check below, so no check can
// be satisfied by the unrelated queue-payload merge earlier in the function.
// PYC: [[IMAGE:%.*]] = pyc.reg %clk, %rst, {{%.*}}, {{%.*}}, {{%.*}} : i6
// The field pass merges only "src_ready" and keeps "admitted" and "tag" from
// the committed image.
// PYC: [[KEEP_ADMITTED:%.*]] = pyc.extract [[IMAGE]] {lsb = 5} : i6 -> i1
// PYC: [[KEEP_TAG:%.*]] = pyc.extract [[IMAGE]] {lsb = 0} : i6 -> i4
// PYC: [[MERGED:%.*]] = pyc.concat([[KEEP_ADMITTED]], {{%.*}}, [[KEEP_TAG]]) : (i1, i1, i4) -> i6
// PYC: [[FIELD_ACC:%.*]] = pyc.select {{%.*}}, [[MERGED]], [[IMAGE]] : i1, i6, i6 -> i6
// PYC: [[FIELD_SPLAT:%.*]] = pyc.select {{%.*}}, [[FIELD_ACC]], [[IMAGE]] : i1, i6, i6 -> i6
// The replace pass takes the whole FIELD_SPLAT as its false arm, so it commits
// on top of the field pass and the replace writer wins, exactly as gfsim does.
// PYC: [[REPLACE_SPLAT:%.*]] = pyc.select {{%.*}}, {{%.*}}, [[FIELD_SPLAT]] : i1, i6, i6 -> i6
// PYC: pyc.assign {{%.*}}, [[REPLACE_SPLAT]] : i6
