// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-verify-value-constraints,ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -DGENERATED_SOURCE=\"%t.cpp\" %S/Inputs/table-runtime-row-main.cpp -o %t.gfsim
// RUN: %t.gfsim | %FileCheck %s --check-prefix=GFSIM
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc
// RUN: %pycc %t.pyc --cpp %t.pyc.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -DGENERATED_SOURCE=\"%t.pyc.cpp\" %S/Inputs/table-runtime-row-pyc-main.cpp %source_root/library/cpp/pyc_runtime.cpp -o %t.pyc-cpp
// RUN: %t.pyc-cpp | %FileCheck %s --check-prefix=PYC-CPP
// RUN: %python %source_root/compiler/acir/tools/acir-queue-veriloggen.py %t.frozen.mlir --pycgen %acir_queue_pycgen -o %t.sv
// RUN: verilator --binary --timing -Wno-fatal --top-module tb --Mdir %t.vdir %t.sv %S/Inputs/table-runtime-row-tb.sv > %t.verilator.log 2>&1
// RUN: %t.vdir/Vtb | %FileCheck %s --check-prefix=VERILATOR

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "table_runtime_row"} {
  ac.type_scope @types {
    ac.struct @Request fields [{name = "row", type = i2}, {name = "tag", type = i8}]
    ac.struct @Result fields [{name = "index", type = i4}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Request> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}, !ac.struct<@types::@Result> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.table @state entry i8 entries 16 init 0 owner "/" stable_id "table/state" {
    shape = array<i64: 4, 4>, axis_widths = array<i64: 2, 2>,
    layout = "row_major", layout_version = 1 : i64,
    schema_id = "sha256:72c2b39db6c2f5550cd8083fde47b4b8f719dee3d41885d7c03e3fc33919530f",
    init_version = 1 : i64,
    init_image = [10 : i8, 11 : i8, 12 : i8, 13 : i8,
                                  20 : i8, 21 : i8, 22 : i8, 23 : i8,
                  32 : i8, 32 : i8, 34 : i8, 35 : i8,
                  40 : i8, 41 : i8, 42 : i8, 43 : i8]
  }
  %requests = ac.source depth 1 latency 1 {ac.name = "requests"}
      : !ac.queue<!ac.struct<@types::@Request>>
  %results = ac.rule %requests depths [1] latencies [1]
      name "lookup" stable_id "lookup_0" domain "cycle" type exact {
  ^body(%request: !ac.var<!ac.struct<@types::@Request>>):
    %enabled = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %enabled : !ac.var<i1>
    %row = ac.var.get %request field "row"
        : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i2>
    %zero_way = ac.var.constant 0 : i2 as !ac.var<i2>
    %base = ac.table.index @state [%row, %zero_way]
        : !ac.var<i2>, !ac.var<i2> -> !ac.var<i4>
    %matches = ac.table.match @state base %base : !ac.var<i4> predicate {
    ^predicate(%entry: !ac.var<i8>):
      %tag = ac.var.get %request field "tag"
          : !ac.var<!ac.struct<@types::@Request>> -> !ac.var<i8>
      %valid = ac.var.cmp "eq" %entry, %tag : !ac.var<i8> -> !ac.var<i1>
      ac.table.match.yield %valid : !ac.var<i1>
    } {domain_axes = array<i64: 1>, domain_shape = array<i64: 4>,
       domain_strides = array<i64: 1>, domain_offset = 0 : i64} -> !ac.var<i4>
    %index, %valid = ac.table.choose @state %matches : !ac.var<i4>
        count 1 policy #ac<table_selection_policy first>
        stable_id "state/runtime-row-first" key {} -> !ac.var<i4>, !ac.var<i1>
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    ac.table.propose @state[%index] = %zero when %valid : !ac.var<i1>
        mode "replace" write_fields ["$entry"] : !ac.var<i4>, !ac.var<i8>
    %result = ac.var.record %index, %valid : !ac.var<i4>, !ac.var<i1>
        -> !ac.var<!ac.struct<@types::@Result>>
    ac.rule.output %result when %enabled ordinal 0
        : !ac.var<!ac.struct<@types::@Result>>, !ac.var<i1>
    %ready = ac.marker.obligation %result state pending resolver handshake
        origin "lookup:return" path "true" : !ac.var<!ac.struct<@types::@Result>>
    ac.rule.return %ready : !ac.var<!ac.struct<@types::@Result>>
  } {ac.name = "results"} : (!ac.queue<!ac.struct<@types::@Request>>)
      -> !ac.queue<!ac.struct<@types::@Result>>
  ac.sink %results {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Result>>
}

// PYC: func.func @table_runtime_row
// PYC-COUNT-4: pyc.cmp {{.*}} {predicate = "eq"} : i8, i8 -> i1
// PYC: pyc.add
// PYC-NOT: ac.table

// GFSIM: PASS gfsim table runtime row capture and update
// PYC-CPP: PASS pyc-cpp table runtime row capture and update
// VERILATOR: PASS verilator table runtime row capture and update
