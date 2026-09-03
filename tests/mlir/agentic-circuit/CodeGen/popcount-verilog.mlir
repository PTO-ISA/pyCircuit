// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %s --pycgen %acir_queue_pycgen | %FileCheck %s --check-prefix=VERILOG

module attributes {ac.contract_epoch = "0.4", ac.system = "popcount"} {
  ac.type_scope @types {
    ac.struct @Item fields [{name = "value", type = i8}, {name = "count", type = i4}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Item> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Item>>
  %output = ac.transform %input depths [2] latencies [1] {
  ^transform(%item: !ac.var<!ac.struct<@types::@Item>>):
    %value = ac.var.get %item field "value" : !ac.var<!ac.struct<@types::@Item>> -> !ac.var<i8>
    %count = ac.var.popcount %value : !ac.var<i8> -> !ac.var<i4>
    %next = ac.var.with %item, %count field "count" : !ac.var<!ac.struct<@types::@Item>>, !ac.var<i4> -> !ac.var<!ac.struct<@types::@Item>>
    ac.transform.yield %next : !ac.var<!ac.struct<@types::@Item>>
  } {ac.output_names = ["output"]} : (!ac.queue<!ac.struct<@types::@Item>>) -> !ac.queue<!ac.struct<@types::@Item>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Item>>
}

// VERILOG: module popcount (
// VERILOG: assign
// VERILOG: +
// VERILOG: module pyc_fifo #(
