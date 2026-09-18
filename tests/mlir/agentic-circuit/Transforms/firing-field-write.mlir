// RUN: %acir_queue_plan %s | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %s | %FileCheck %s --check-prefix=GFSIM
// RUN: %acir_queue_pycgen %s | %FileCheck %s --check-prefix=PYC

// Two rules that update disjoint fields of one Table Entry must coexist in one
// tick without explicit priority.  The frozen module carries one `mode "field"`
// proposal per rule, so every backend is exercised: the plan admits the field
// mode, gfsim selects its field-merge write mode, and PYC merges exactly the
// named fields into the committed image and leaves the untouched field alone.

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
    ac.table.propose @entries[%5] = %7 when %4 : !ac.var<i1> mode "field" write_fields ["admitted"] : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.firing.output %6 when %4 ordinal 0 : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1>
    ac.state.snapshot @entries[%5 : !ac.var<i2>] for %4 : !ac.var<i1> kind  dynamic read_fields ["admitted", "src_ready", "tag"]
    ac.firing.yield %6 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @entries}], ac.arbitration_membership = [], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @entries}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, resource = @entries}], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "first", ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "admit", ac.rule_footprints = [{access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @entries}, {access = "field", fields = ["admitted"], guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @entries}], ac.rule_priority = 0 : i64, ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>, ac.source_column = 1 : i64, ac.source_file = "<queue-model>", ac.source_line = 17 : i64, ac.state_accesses = [{guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind dynamic>, kind = #ac<rule_state_access_kind read>, resource = @entries}, {fields = ["admitted"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind dynamic>, kind = #ac<rule_state_access_kind field_write>, resource = @entries}], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @entries}]} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  %3 = ac.firing %1#1 depths [1] latencies [1] stable_id "second" domain "cycle" {
  ^bb0(%arg0: !ac.var<!ac.struct<@types::@Update>>):
    %4 = ac.var.constant true as !ac.var<i1>
    ac.firing.condition %4 : !ac.var<i1>
    %5 = ac.var.get %arg0 field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %6 = ac.table.get @entries[%5] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %7 = ac.var.with %6, %4 field "src_ready" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @entries[%5] = %7 when %4 : !ac.var<i1> mode "field" write_fields ["src_ready"] : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.firing.output %6 when %4 ordinal 0 : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1>
    ac.state.snapshot @entries[%5 : !ac.var<i2>] for %4 : !ac.var<i1> kind  dynamic read_fields ["admitted", "src_ready", "tag"]
    ac.firing.yield %6 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.activation_sources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @entries}], ac.arbitration_membership = [], ac.checks_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind input_available>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_check_kind output_capacity>, ordinal = 0 : i64}], ac.effects_typed = [{guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind input_consume>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind output_produce>, ordinal = 0 : i64}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_read>, resource = @entries}, {guard_kind = #ac<rule_guard_kind always>, kind = #ac<rule_effect_kind state_write>, resource = @entries}], ac.guard_kind = #ac<rule_guard_kind always>, ac.initially_active = false, ac.name = "second", ac.output_presence = [{ordinal = 0 : i64, presence_kind = #ac<rule_output_presence_kind always>}], ac.rule_definition = "wake", ac.rule_footprints = [{access = "read", guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @entries}, {access = "field", fields = ["src_ready"], guard_kind = #ac<rule_guard_kind always>, index_kind = "dynamic", resource = @entries}], ac.rule_priority = 1 : i64, ac.schedule_kind = #ac<rule_schedule_kind lexical_priority>, ac.source_column = 1 : i64, ac.source_file = "<queue-model>", ac.source_line = 24 : i64, ac.state_accesses = [{guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind dynamic>, kind = #ac<rule_state_access_kind read>, resource = @entries}, {fields = ["src_ready"], guard_kind = #ac<rule_guard_kind always>, index_kind = #ac<rule_index_kind dynamic>, kind = #ac<rule_state_access_kind field_write>, resource = @entries}], ac.transaction_resources = [{kind = #ac<activation_resource_kind input_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind output_queue>, ordinal = 0 : i64}, {kind = #ac<activation_resource_kind state>, resource = @entries}]} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %2 {ac.name = "sink_4"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %3 {ac.name = "sink_5"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// PLAN: "write_fields":["admitted"]
// PLAN: "write_fields":["src_ready"]

// GFSIM-COUNT-2: gfsim::TableWriteMode::FieldMerge

// [[IMG]] is the committed image of one Entry.  The first rule merges only
// "admitted", so the remaining image bits survive; the second rule merges
// "src_ready" on top of that result, so both fields and the untouched "tag"
// reach the next tick.
// The committed image of one Entry is the base of every merge below.  A field
// write publishes the whole Entry value, so the merge has to rebuild the value
// from the committed image: only the named field comes from the proposal.

// PYC: [[IMAGE:%.*]] = pyc.reg %clk, %rst, {{%.*}}, {{%.*}}, {{%.*}} : i6
// The first rule writes "admitted".  The untouched low five bits are taken
// from [[IMAGE]], not from the proposal, and the result is what gets merged.
// PYC: [[KEPT_LOW:%.*]] = pyc.extract [[IMAGE]] {lsb = 0} : i6 -> i5
// PYC: [[MERGED_A:%.*]] = pyc.concat({{%.*}}, [[KEPT_LOW]]) : (i1, i5) -> i6
// PYC: [[ADMITTED:%.*]] = pyc.select {{%.*}}, [[MERGED_A]], [[IMAGE]] : i1, i6, i6 -> i6
// The second rule writes "src_ready" on top of that result, so the first
// rule's "admitted" and the never-written "tag" both survive the second merge.
// PYC: [[KEPT_A:%.*]] = pyc.extract [[ADMITTED]] {lsb = 5} : i6 -> i1
// PYC: [[KEPT_TAG:%.*]] = pyc.extract [[ADMITTED]] {lsb = 0} : i6 -> i4
// PYC: [[MERGED_B:%.*]] = pyc.concat([[KEPT_A]], {{%.*}}, [[KEPT_TAG]]) : (i1, i1, i4) -> i6
// PYC: [[SRC_READY:%.*]] = pyc.select {{%.*}}, [[MERGED_B]], [[ADMITTED]] : i1, i6, i6 -> i6
// PYC: [[COMMIT:%.*]] = pyc.select {{%.*}}, [[SRC_READY]], [[IMAGE]] : i1, i6, i6 -> i6
// PYC: pyc.assign {{%.*}}, [[COMMIT]] : i6
