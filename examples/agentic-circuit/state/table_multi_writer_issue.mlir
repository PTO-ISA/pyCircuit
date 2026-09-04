module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "table_multi_writer_issue"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "age", type = i8}, {name = "src0_tag", type = i8}, {name = "src0_ready", type = i1}, {name = "src1_tag", type = i8}, {name = "src1_ready", type = i1}]
    ac.struct @Wakeup fields [{name = "tag", type = i8}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 6 : i64}, !ac.struct<@types::@Wakeup> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.table @issue entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/issue"
  %wakeups = ac.source depth 2 latency 1 {ac.name = "wakeups"} : !ac.queue<!ac.struct<@types::@Wakeup>>
  ac.slot @wakeup, %wakeups owner "/" stable_id "slot/wakeup" : !ac.queue<!ac.struct<@types::@Wakeup>>
  %table_match_3 = ac.table.match @issue predicate {
  ^predicate(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %match_3_v0 = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_3_v1 = ac.var.get %entry field "src0_ready" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_3_v2 = ac.var.constant false as !ac.var<i1>
    %match_3_v3 = ac.var.cmp "eq" %match_3_v1, %match_3_v2 : !ac.var<i1> -> !ac.var<i1>
    %match_3_v4 = ac.var.mul %match_3_v0, %match_3_v3 : !ac.var<i1>
    %match_3_v5 = ac.var.get %entry field "src0_tag" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i8>
    %match_3_v6, %match_3_v7 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    %match_3_v8 = ac.var.get %match_3_v7 field "tag" : !ac.var<!ac.struct<@types::@Wakeup>> -> !ac.var<i8>
    %match_3_v9 = ac.var.cmp "eq" %match_3_v5, %match_3_v8 : !ac.var<i8> -> !ac.var<i1>
    %match_3_v10 = ac.var.mul %match_3_v4, %match_3_v9 : !ac.var<i1>
    ac.table.match.yield %match_3_v10 : !ac.var<i1>
  } -> !ac.var<i4>
  ac.table.masked_write @issue %table_match_3 : !ac.var<i4> mode "field" write_fields ["src0_ready"] enable {
  ^enable:
    %enable_v0, %enable_v1 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    ac.table.yield %enable_v0 : !ac.var<i1>
  } value {
  ^value(%old: !ac.var<!ac.struct<@types::@Entry>>):
    %value_v0 = ac.var.constant true as !ac.var<i1>
    %value_v1 = ac.var.with %old, %value_v0 field "src0_ready" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %value_v1 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/issue__masked_write", ac.name = "issue__masked_write"}
  %table_match_5 = ac.table.match @issue predicate {
  ^predicate(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %match_5_v0 = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_5_v1 = ac.var.get %entry field "src1_ready" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_5_v2 = ac.var.constant false as !ac.var<i1>
    %match_5_v3 = ac.var.cmp "eq" %match_5_v1, %match_5_v2 : !ac.var<i1> -> !ac.var<i1>
    %match_5_v4 = ac.var.mul %match_5_v0, %match_5_v3 : !ac.var<i1>
    %match_5_v5 = ac.var.get %entry field "src1_tag" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i8>
    %match_5_v6, %match_5_v7 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    %match_5_v8 = ac.var.get %match_5_v7 field "tag" : !ac.var<!ac.struct<@types::@Wakeup>> -> !ac.var<i8>
    %match_5_v9 = ac.var.cmp "eq" %match_5_v5, %match_5_v8 : !ac.var<i8> -> !ac.var<i1>
    %match_5_v10 = ac.var.mul %match_5_v4, %match_5_v9 : !ac.var<i1>
    ac.table.match.yield %match_5_v10 : !ac.var<i1>
  } -> !ac.var<i4>
  ac.table.masked_write @issue %table_match_5 : !ac.var<i4> mode "field" write_fields ["src1_ready"] enable {
  ^enable:
    %enable_v0, %enable_v1 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    ac.table.yield %enable_v0 : !ac.var<i1>
  } value {
  ^value(%old: !ac.var<!ac.struct<@types::@Entry>>):
    %value_v0 = ac.var.constant true as !ac.var<i1>
    %value_v1 = ac.var.with %old, %value_v0 field "src1_ready" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %value_v1 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/issue__masked_write_1", ac.name = "issue__masked_write_1"}
  %table_match_7 = ac.table.match @issue predicate {
  ^predicate(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %match_7_v0 = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_7_v1 = ac.var.get %entry field "src0_ready" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_7_v2 = ac.var.mul %match_7_v0, %match_7_v1 : !ac.var<i1>
    %match_7_v3 = ac.var.get %entry field "src1_ready" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %match_7_v4 = ac.var.mul %match_7_v2, %match_7_v3 : !ac.var<i1>
    ac.table.match.yield %match_7_v4 : !ac.var<i1>
  } -> !ac.var<i4>
  %table_choose_8_index, %table_choose_8_valid = ac.table.choose @issue %table_match_7 : !ac.var<i4> count 1 policy "min" key {
  ^key(%entry: !ac.var<!ac.struct<@types::@Entry>>):
    %choose_8_v0 = ac.var.get %entry field "age" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i8>
    ac.table.choose.yield %choose_8_v0 : !ac.var<i8>
  } -> !ac.var<i2>, !ac.var<i1>
  %output = ac.table.read @issue depth 1 latency 1 address {
  ^address:
    ac.table.yield %table_choose_8_index : !ac.var<i2>
  } when {
  ^when:
    ac.table.yield %table_choose_8_valid : !ac.var<i1>
  } {ac.endpoint_path = "/output", ac.name = "output"} -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.table.write @issue mode "field" write_fields ["valid"] address {
  ^address:
    ac.table.yield %table_choose_8_index : !ac.var<i2>
  } enable {
  ^enable:
    ac.table.yield %table_choose_8_valid : !ac.var<i1>
  } value {
  ^value:
    %v0 = ac.table.get @issue [%table_choose_8_index] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %v1 = ac.var.constant false as !ac.var<i1>
    %v2 = ac.var.with %v0, %v1 field "valid" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.table.yield %v2 : !ac.var<!ac.struct<@types::@Entry>>
  } {ac.endpoint_path = "/issue__write", ac.name = "issue__write"}
  ac.slot.release @wakeup when {
  ^when:
    %v0, %v1 = ac.slot.get @wakeup : !ac.var<i1>, !ac.var<!ac.struct<@types::@Wakeup>>
    ac.slot.yield %v0 : !ac.var<i1>
  } {ac.endpoint_path = "/wakeup__release", ac.name = "wakeup__release"}
  ac.sink %output {ac.name = "sink_12"} : !ac.queue<!ac.struct<@types::@Entry>>
}
