// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "count_zeros"} {
  ac.type_scope @types {
    ac.struct @Item fields [{name = "value", type = i13}, {name = "leading", type = i4}, {name = "trailing", type = i4}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Item> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}>}
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Item>>
  %output = ac.transform %input depths [2] latencies [1] {
  ^transform(%item: !ac.var<!ac.struct<@types::@Item>>):
    %value = ac.var.get %item field "value" : !ac.var<!ac.struct<@types::@Item>> -> !ac.var<i13>
    %leading = ac.var.count_zeros %value direction "leading" : !ac.var<i13> -> !ac.var<i4>
    %trailing = ac.var.count_zeros %value direction "trailing" : !ac.var<i13> -> !ac.var<i4>
    %with_leading = ac.var.with %item, %leading field "leading" : !ac.var<!ac.struct<@types::@Item>>, !ac.var<i4> -> !ac.var<!ac.struct<@types::@Item>>
    %next = ac.var.with %with_leading, %trailing field "trailing" : !ac.var<!ac.struct<@types::@Item>>, !ac.var<i4> -> !ac.var<!ac.struct<@types::@Item>>
    ac.transform.yield %next : !ac.var<!ac.struct<@types::@Item>>
  } {ac.output_names = ["output"]} : (!ac.queue<!ac.struct<@types::@Item>>) -> !ac.queue<!ac.struct<@types::@Item>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Item>>
}

// PYC: pyc.count_zeros {{.*}} {direction = "leading"} : i13 -> i4
// PYC: pyc.count_zeros {{.*}} {direction = "trailing"} : i13 -> i4
// PYC-NOT: implementation_id
// PYC-NOT: lzc
