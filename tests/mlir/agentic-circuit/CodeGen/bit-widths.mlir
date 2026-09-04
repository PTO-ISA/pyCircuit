// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %FileCheck %s --check-prefix=GFSIM < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.cpp -o %t.o
// RUN: %acir_queue_pycgen %t.frozen.mlir | %FileCheck %s --check-prefix=PYC

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "bit_widths"} {
  ac.type_scope @types {
    ac.struct @Bits fields [{name = "left", type = i3}, {name = "right", type = i3}, {name = "result", type = i3}, {name = "priority_index", type = i2}, {name = "priority_valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Bits> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 5 : i64}>}
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Bits>>
  %output = ac.transform %input depths [1] latencies [1] {
  ^transform(%item: !ac.var<!ac.struct<@types::@Bits>>):
    %left = ac.var.get %item field "left" : !ac.var<!ac.struct<@types::@Bits>> -> !ac.var<i3>
    %right = ac.var.get %item field "right" : !ac.var<!ac.struct<@types::@Bits>> -> !ac.var<i3>
    %one = ac.var.constant 1 : i3 as !ac.var<i3>
    %anded = ac.var.and %left, %right : !ac.var<i3>
    %ored = ac.var.or %anded, %one : !ac.var<i3>
    %xored = ac.var.xor %ored, %right : !ac.var<i3>
    %inverted = ac.var.not %xored : !ac.var<i3> -> !ac.var<i3>
    %shifted_left = ac.var.shl %inverted, %one : !ac.var<i3>
    %shifted_right = ac.var.shr %shifted_left, %one : !ac.var<i3>
    %priority_index, %priority_valid = ac.var.priority_encode %left order "low" : !ac.var<i3> -> !ac.var<i2>, !ac.var<i1>
    %with_result = ac.var.with %item, %shifted_right field "result" : !ac.var<!ac.struct<@types::@Bits>>, !ac.var<i3> -> !ac.var<!ac.struct<@types::@Bits>>
    %with_index = ac.var.with %with_result, %priority_index field "priority_index" : !ac.var<!ac.struct<@types::@Bits>>, !ac.var<i2> -> !ac.var<!ac.struct<@types::@Bits>>
    %next = ac.var.with %with_index, %priority_valid field "priority_valid" : !ac.var<!ac.struct<@types::@Bits>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Bits>>
    ac.transform.yield %next : !ac.var<!ac.struct<@types::@Bits>>
  } {ac.output_names = ["output"]} : (!ac.queue<!ac.struct<@types::@Bits>>) -> !ac.queue<!ac.struct<@types::@Bits>>
  ac.sink %output {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Bits>>
}

// GFSIM: #include "gfsim/bits.h"
// GFSIM: gfsim::UInt<3> left{};
// GFSIM: gfsim::UInt<3> right{};
// GFSIM: gfsim::UInt<3> result{};
// GFSIM: gfsim::UInt<2> priority_index{};
// GFSIM: gfsim::UInt<1> priority_valid{};
// GFSIM: auto v3 = v0 & v1;
// GFSIM: auto v6 = ~v5;
// GFSIM: auto v7 = v6 << v2;
// GFSIM: auto v8 = v7 >> v2;
// GFSIM: auto priority_v9 = gfsim::priorityEncode(v0, true);
// GFSIM-NEXT: auto v9 = priority_v9.index;
// GFSIM-NEXT: auto v10 = priority_v9.valid;

// PYC: = pyc.and
// PYC: = pyc.or
// PYC: = pyc.xor
// PYC: = pyc.not
// PYC: = pyc.shl
// PYC: = pyc.lshr
// PYC: = pyc.priority_encode
