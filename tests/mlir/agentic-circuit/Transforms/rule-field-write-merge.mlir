// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %t/field-merge.mlir -o /dev/null
// RUN: %not %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %t/allocator-conflict.mlir 2>&1 | %FileCheck %s --check-prefix=CONFLICT
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/masked-declarative.mlir -o /dev/null
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/field-merge.mlir -o %t/field-merge-frozen.mlir
// RUN: %acir_queue_plan %t/field-merge-frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_cxxgen %t/field-merge-frozen.mlir | %FileCheck %s --check-prefix=GFSIM
// RUN: %acir_queue_pycgen %t/field-merge-frozen.mlir | %FileCheck %s --check-prefix=PYC

// Two rules may update disjoint fields of one Entry in one tick. Both proposals
// narrow to `mode "field"` with the exact canonical field set, so the
// disjointness proof accepts them and the writer-overlap diagnostic that used
// to force one logical entry into several state banks no longer fires.
//
// The exemption that lets one replace writer coexist with field writers is
// scoped to declarative owner-local writers, because Decision 0156 fixes their
// commit order. Both declarative field spellings qualify: `ac.table.write` in
// field mode and `ac.table.masked_write`. A field proposal raised by a rule is
// not ordered against that allocation and still has to arbitrate, which the
// allocator-conflict case pins.

// The field-merge case now reaches every backend: the plan admits the field
// mode, gfsim selects FieldMerge for both rules, and PYC merges each named
// field into the committed image while leaving the untouched field alone
// (mirrors the Ordering contract pinned by firing-field-write.mlir).
//
// PLAN: "write_fields":["admitted"]
// PLAN: "write_fields":["src_ready"]
// GFSIM-COUNT-2: gfsim::TableWriteMode::FieldMerge
// PYC: [[IMAGE:%.*]] = pyc.reg %clk, %rst, {{%.*}}, {{%.*}}, {{%.*}} : i6
// PYC: [[KEPT_LOW:%.*]] = pyc.extract [[IMAGE]] {lsb = 0} : i6 -> i5
// PYC: [[MERGED_A:%.*]] = pyc.concat({{%.*}}, [[KEPT_LOW]]) : (i1, i5) -> i6
// PYC: [[ADMITTED:%.*]] = pyc.select {{%.*}}, [[MERGED_A]], [[IMAGE]] : i1, i6, i6 -> i6
// PYC: [[KEPT_A:%.*]] = pyc.extract [[ADMITTED]] {lsb = 5} : i6 -> i1
// PYC: [[KEPT_TAG:%.*]] = pyc.extract [[ADMITTED]] {lsb = 0} : i6 -> i4
// PYC: [[MERGED_B:%.*]] = pyc.concat([[KEPT_A]], {{%.*}}, [[KEPT_TAG]]) : (i1, i1, i4) -> i6
// PYC: [[SRC_READY:%.*]] = pyc.select {{%.*}}, [[MERGED_B]], [[ADMITTED]] : i1, i6, i6 -> i6
// PYC: [[COMMIT:%.*]] = pyc.select {{%.*}}, [[SRC_READY]], [[IMAGE]] : i1, i6, i6 -> i6
// PYC: pyc.assign {{%.*}}, [[COMMIT]] : i6

//--- field-merge.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "two_rule"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "admitted", type = i1}, {name = "src_ready", type = i1}, {name = "tag", type = i4}]
    ac.struct @Update fields [{name = "index", type = i2}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}, !ac.struct<@types::@Update> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.table @entries entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/entries"
  %updates = ac.source depth 2 latency 1 {ac.name = "updates"} : !ac.queue<!ac.struct<@types::@Update>>
  %updates__fanout0, %updates__fanout1 = ac.broadcast %updates depths [1, 1] latencies [1, 1] {ac.output_names = ["updates__fanout0", "updates__fanout1"]} : !ac.queue<!ac.struct<@types::@Update>> -> (!ac.queue<!ac.struct<@types::@Update>>, !ac.queue<!ac.struct<@types::@Update>>)
  %first = ac.rule %updates__fanout0 depths [1] latencies [1] name "admit" stable_id "first" domain "cycle" type exact {
  ^rule(%item: !ac.var<!ac.struct<@types::@Update>>):
    %v0 = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %v1 = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %v2 = ac.table.get @entries [%v1] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %v3 = ac.var.constant true as !ac.var<i1>
    %v4 = ac.var.with %v2, %v3 field "admitted" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @entries [%v0] = %v4 mode "field" write_fields ["admitted"] : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %rule_ready = ac.marker.obligation %v2 state pending resolver handshake origin "admit:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %rule_ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "first", ac.source_file = "<queue-model>", ac.source_line = 17 : i64, ac.source_column = 1 : i64} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>  loc("<queue-model>":17:1)
  %second = ac.rule %updates__fanout1 depths [1] latencies [1] name "wake" stable_id "second" domain "cycle" type exact {
  ^rule(%item: !ac.var<!ac.struct<@types::@Update>>):
    %v0 = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %v1 = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %v2 = ac.table.get @entries [%v1] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %v3 = ac.var.constant true as !ac.var<i1>
    %v4 = ac.var.with %v2, %v3 field "src_ready" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @entries [%v0] = %v4 mode "field" write_fields ["src_ready"] : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %rule_ready = ac.marker.obligation %v2 state pending resolver handshake origin "wake:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %rule_ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "second", ac.source_file = "<queue-model>", ac.source_line = 24 : i64, ac.source_column = 1 : i64} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>  loc("<queue-model>":24:1)
  ac.sink %first {ac.name = "sink_4"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %second {ac.name = "sink_5"} : !ac.queue<!ac.struct<@types::@Entry>>
}

//--- allocator-conflict.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "guard_probe"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "admitted", type = i1}, {name = "value", type = i8}]
    ac.struct @Update fields [{name = "index", type = i2}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}, !ac.struct<@types::@Update> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.table @entries entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/entries"
  %updates = ac.source depth 2 latency 1 {ac.name = "updates"} : !ac.queue<!ac.struct<@types::@Update>>
  %allocations = ac.source depth 2 latency 1 {ac.name = "allocations"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.slot @allocation, %allocations owner "/" stable_id "slot/allocation" : !ac.queue<!ac.struct<@types::@Entry>>
  %first = ac.rule %updates depths [1] latencies [1] name "admit" stable_id "first" domain "cycle" type exact {
  ^rule(%item: !ac.var<!ac.struct<@types::@Update>>):
    %v0 = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %v1 = ac.var.get %item field "index" : !ac.var<!ac.struct<@types::@Update>> -> !ac.var<i2>
    %v2 = ac.table.get @entries [%v1] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %v3 = ac.var.constant true as !ac.var<i1>
    %v4 = ac.var.with %v2, %v3 field "admitted" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.propose @entries [%v0] = %v4 mode "field" write_fields ["admitted"] : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    %rule_ready = ac.marker.obligation %v2 state pending resolver handshake origin "admit:return" path "true" : !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %rule_ready : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.name = "first", ac.source_file = "<queue-model>", ac.source_line = 17 : i64, ac.source_column = 1 : i64} : (!ac.queue<!ac.struct<@types::@Update>>) -> !ac.queue<!ac.struct<@types::@Entry>>  loc("<queue-model>":17:1)
  %table_match_5 = ac.table.match @entries predicate {
  ^predicate(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %match_5_v0 = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_5_v1 = ac.var.constant false as !ac.var<i1>
    %match_5_v2 = ac.var.cmp "eq" %match_5_v0, %match_5_v1 : !ac.var<i1> -> !ac.var<i1>
    ac.table.match.yield %match_5_v2 : !ac.var<i1>
  } {domain_axes = array<i64: 0>, domain_shape = array<i64: 4>, domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
  %table_choose_6_index_0, %table_choose_6_valid_0 = ac.table.choose @entries %table_match_5 : !ac.var<i4> count 1 policy #ac<table_selection_policy first> stable_id "table-selection/free" key {} -> !ac.var<i2>, !ac.var<i1>
  ac.table.write @entries mode "replace" write_fields ["valid", "admitted", "value"] address {
  ^address:
    ac.table.yield %table_choose_6_index_0 : !ac.var<i2>
  } enable {
  ^enable:
    ac.table.yield %table_choose_6_valid_0 : !ac.var<i1>
  } value {
  ^value:
    %v0, %v1 = ac.slot.get @allocation : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %v1 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/entries__allocate", ac.name = "entries__allocate", ac.endpoint_id = "table-writer/2225f6c537bab40f6e9dbdc038291ada07c243b8dc72052f53768079dfb2f203"}
  ac.slot.release @allocation when {
  ^when:
    ac.slot.yield %table_choose_6_valid_0 : !ac.var<i1>
  } {ac.endpoint_path = "/allocation__release", ac.name = "allocation__release"}
  ac.sink %first {ac.name = "sink_9"} : !ac.queue<!ac.struct<@types::@Entry>>
}

//--- masked-declarative.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "issue"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "age", type = i8}, {name = "src0_tag", type = i8}, {name = "src0_ready", type = i1}, {name = "src1_tag", type = i8}, {name = "src1_ready", type = i1}]
    ac.struct @Wakeup fields [{name = "tag", type = i8}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 6 : i64}, !ac.struct<@types::@Wakeup> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.table @issue entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/issue"
  %wakeups = ac.source depth 2 latency 1 {ac.name = "wakeups"} : !ac.queue<!ac.struct<@types::@Wakeup>>
  %allocations = ac.source depth 2 latency 1 {ac.name = "allocations"} : !ac.queue<!ac.struct<@types::@Entry>>
  ac.slot @wakeup, %wakeups owner "/" stable_id "slot/wakeup" : !ac.queue<!ac.struct<@types::@Wakeup>>
  ac.slot @allocation, %allocations owner "/" stable_id "slot/allocation" : !ac.queue<!ac.struct<@types::@Entry>>
  %table_match_5 = ac.table.match @issue predicate {
  ^predicate(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %match_5_v0 = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_5_v1 = ac.var.get %entry field "src0_ready" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_5_v2 = ac.var.constant false as !ac.var<i1>
    %match_5_v3 = ac.var.cmp "eq" %match_5_v1, %match_5_v2 : !ac.var<i1> -> !ac.var<i1>
    %match_5_v4 = ac.var.mul %match_5_v0, %match_5_v3 : !ac.var<i1>
    %match_5_v5 = ac.var.get %entry field "src0_tag" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i8>
    %match_5_v6, %match_5_v7 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    %match_5_v8 = ac.var.get %match_5_v7 field "tag" : !ac.var<!ac.struct<@types::@Wakeup>> -> !ac.var<i8>
    %match_5_v9 = ac.var.cmp "eq" %match_5_v5, %match_5_v8 : !ac.var<i8> -> !ac.var<i1>
    %match_5_v10 = ac.var.mul %match_5_v4, %match_5_v9 : !ac.var<i1>
    ac.table.match.yield %match_5_v10 : !ac.var<i1>
  } {domain_axes = array<i64: 0>, domain_shape = array<i64: 4>, domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
  ac.table.masked_write @issue %table_match_5 : !ac.var<i4> mode "field" write_fields ["src0_ready"] enable {
  ^enable:
    %enable_v0, %enable_v1 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    ac.table.yield %enable_v0 : !ac.var<i1>
  } value {
  ^value(%old: !ac.var<!ac.struct<@types::@Entry>>):
    %value_v0 = ac.var.constant true as !ac.var<i1>
    %value_v1 = ac.var.with %old, %value_v0 field "src0_ready" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %value_v1 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/issue__masked_write_e3d492eb5e7b6405be9de262", ac.name = "issue__masked_write_e3d492eb5e7b6405be9de262", ac.endpoint_id = "table-writer/e3d492eb5e7b6405be9de26219a73142c0e6186eaa4f4a57ec7c32467ca5362d"}
  %table_match_7 = ac.table.match @issue predicate {
  ^predicate(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %match_7_v0 = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_7_v1 = ac.var.get %entry field "src1_ready" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_7_v2 = ac.var.constant false as !ac.var<i1>
    %match_7_v3 = ac.var.cmp "eq" %match_7_v1, %match_7_v2 : !ac.var<i1> -> !ac.var<i1>
    %match_7_v4 = ac.var.mul %match_7_v0, %match_7_v3 : !ac.var<i1>
    %match_7_v5 = ac.var.get %entry field "src1_tag" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i8>
    %match_7_v6, %match_7_v7 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    %match_7_v8 = ac.var.get %match_7_v7 field "tag" : !ac.var<!ac.struct<@types::@Wakeup>> -> !ac.var<i8>
    %match_7_v9 = ac.var.cmp "eq" %match_7_v5, %match_7_v8 : !ac.var<i8> -> !ac.var<i1>
    %match_7_v10 = ac.var.mul %match_7_v4, %match_7_v9 : !ac.var<i1>
    ac.table.match.yield %match_7_v10 : !ac.var<i1>
  } {domain_axes = array<i64: 0>, domain_shape = array<i64: 4>, domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
  ac.table.masked_write @issue %table_match_7 : !ac.var<i4> mode "field" write_fields ["src1_ready"] enable {
  ^enable:
    %enable_v0, %enable_v1 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    ac.table.yield %enable_v0 : !ac.var<i1>
  } value {
  ^value(%old: !ac.var<!ac.struct<@types::@Entry>>):
    %value_v0 = ac.var.constant true as !ac.var<i1>
    %value_v1 = ac.var.with %old, %value_v0 field "src1_ready" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %value_v1 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/issue__masked_write_a4f58b7ce7e1172f26123e26", ac.name = "issue__masked_write_a4f58b7ce7e1172f26123e26", ac.endpoint_id = "table-writer/a4f58b7ce7e1172f26123e2681720848598ba7f481c127e7294daf48aa6f41f8"}
  %table_match_9 = ac.table.match @issue predicate {
  ^predicate(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %match_9_v0 = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_9_v1 = ac.var.get %entry field "src0_ready" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_9_v2 = ac.var.mul %match_9_v0, %match_9_v1 : !ac.var<i1>
    %match_9_v3 = ac.var.get %entry field "src1_ready" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_9_v4 = ac.var.mul %match_9_v2, %match_9_v3 : !ac.var<i1>
    ac.table.match.yield %match_9_v4 : !ac.var<i1>
  } {domain_axes = array<i64: 0>, domain_shape = array<i64: 4>, domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
  %table_choose_10_index_0, %table_choose_10_valid_0 = ac.table.choose @issue %table_match_9 : !ac.var<i4> count 1 policy #ac<table_selection_policy min> key_order #ac<table_key_ordering unsigned> stable_id "table-selection/grant" key {
  ^key(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %choose_10_v0 = ac.var.get %entry field "age" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i8>
    ac.table.choose.yield %choose_10_v0 : !ac.var<i8>
  } -> !ac.var<i2>, !ac.var<i1>
  %output = ac.table.read @issue depth 1 latency 1 address {
  ^address:
    ac.table.yield %table_choose_10_index_0 : !ac.var<i2>
  } when {
  ^when:
    ac.table.yield %table_choose_10_valid_0 : !ac.var<i1>
  } {ac.endpoint_path = "/output", ac.name = "output"} -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.table.write @issue mode "field" write_fields ["valid"] address {
  ^address:
    ac.table.yield %table_choose_10_index_0 : !ac.var<i2>
  } enable {
  ^enable:
    ac.table.yield %table_choose_10_valid_0 : !ac.var<i1>
  } value {
  ^value:
    %v0 = ac.table.get @issue [%table_choose_10_index_0] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %v1 = ac.var.constant false as !ac.var<i1>
    %v2 = ac.var.with %v0, %v1 field "valid" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %v2 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/issue__write", ac.name = "issue__write", ac.endpoint_id = "table-writer/1e3dfdc0539aec1407c77cc5405cfd62680736e963ee4fc793100d04f26134d0"}
  %table_match_13 = ac.table.match @issue predicate {
  ^predicate(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %match_13_v0, %match_13_v1 = ac.slot.get @allocation : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    %match_13_v2 = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_13_v3 = ac.var.constant false as !ac.var<i1>
    %match_13_v4 = ac.var.cmp "eq" %match_13_v2, %match_13_v3 : !ac.var<i1> -> !ac.var<i1>
    %match_13_v5 = ac.var.mul %match_13_v0, %match_13_v4 : !ac.var<i1>
    ac.table.match.yield %match_13_v5 : !ac.var<i1>
  } {domain_axes = array<i64: 0>, domain_shape = array<i64: 4>, domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
  %table_choose_14_index_0, %table_choose_14_valid_0 = ac.table.choose @issue %table_match_13 : !ac.var<i4> count 1 policy #ac<table_selection_policy first> stable_id "table-selection/free" key {} -> !ac.var<i2>, !ac.var<i1>
  ac.table.write @issue mode "replace" write_fields ["valid", "age", "src0_tag", "src0_ready", "src1_tag", "src1_ready"] address {
  ^address:
    ac.table.yield %table_choose_14_index_0 : !ac.var<i2>
  } enable {
  ^enable:
    ac.table.yield %table_choose_14_valid_0 : !ac.var<i1>
  } value {
  ^value:
    %v0, %v1 = ac.slot.get @allocation : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %v1 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/issue__allocate", ac.name = "issue__allocate", ac.endpoint_id = "table-writer/a83650ed34b61c74970b1ce04e20f6ad651a52028f3f4d4ae2bd9113026b652c"}
  ac.slot.release @wakeup when {
  ^when:
    %v0, %v1 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    ac.slot.yield %v0 : !ac.var<i1>
  } {ac.endpoint_path = "/wakeup__release", ac.name = "wakeup__release"}
  ac.slot.release @allocation when {
  ^when:
    ac.slot.yield %table_choose_14_valid_0 : !ac.var<i1>
  } {ac.endpoint_path = "/allocation__release", ac.name = "allocation__release"}
  ac.sink %output {ac.name = "sink_18"} : !ac.queue<!ac.struct<@types::@Entry>>
}

// CONFLICT: same-field overlap on owner @entries requires explicit priority on every writer endpoint
