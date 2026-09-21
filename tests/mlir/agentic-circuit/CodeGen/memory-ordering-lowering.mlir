// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc
// RUN: %FileCheck %s --check-prefix=PYC < %t.pyc
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.gfsim.cpp
// RUN: %FileCheck %s --check-prefix=CPP < %t.gfsim.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.gfsim.cpp
// RUN: %pycc %t.pyc --cpp %t.cpp --logic-depth=64
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t.cpp
// RUN: %FileCheck %s --check-prefix=PYC-CPP < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -include %t.cpp %S/memory-ordering-harness.cpp -o %t.run
// RUN: %t.run | %FileCheck %s --check-prefix=EXEC
// RUN: %pycc %t.pyc --verilog %t.sv --logic-depth=64
// RUN: %FileCheck %s --check-prefix=RTL < %t.sv
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t.sv --json-out %t.audit.json
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t.sv
// RUN: iverilog -g2012 -DSYNTHESIS -I%source_root/library/verilog -s tb_memory_ordering -o %t.vvp %t.sv %S/memory-ordering-tb.sv
// RUN: vvp %t.vvp | %FileCheck %s --check-prefix=RTL-EXEC

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "memory_ordering"} {
  ac.type_scope @types {
    ac.struct @Input fields [
      {name = "pending", type = i4}, {name = "alias", type = i4},
      {name = "disjoint", type = i4}, {name = "ready", type = i4},
      {name = "executed", type = i4}, {name = "identity", type = i4},
      {name = "producer", type = i4}, {name = "consumer", type = i4},
      {name = "killed", type = i4}
    ]
    ac.struct @Output fields [
      {name = "applies", type = i4}, {name = "wait", type = i4},
      {name = "bypass", type = i4}, {name = "forward", type = i4},
      {name = "replay", type = i4}, {name = "stale", type = i4}
    ]
  } {dlti.dl_spec = #dlti.dl_spec<
      !ac.struct<@types::@Input> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 36 : i64},
      !ac.struct<@types::@Output> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 24 : i64}>}

  %input = ac.source depth 4 latency 1 {ac.name = "input"}
      : !ac.queue<!ac.struct<@types::@Input>>
  %output = ac.rule %input depths [4] latencies [1]
      name "order" stable_id "order" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Input>>):
    %pending = ac.var.get %item field "pending" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %alias = ac.var.get %item field "alias" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %disjoint = ac.var.get %item field "disjoint" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %ready = ac.var.get %item field "ready" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %executed = ac.var.get %item field "executed" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %identity = ac.var.get %item field "identity" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %producer = ac.var.get %item field "producer" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %consumer = ac.var.get %item field "consumer" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %killed = ac.var.get %item field "killed" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %applies = "ac.memory_order_edge"(%producer, %consumer) <{
      lanes = 4 : i64, kind = #ac<memory_order_kind must_wait>
    }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %wait, %bypass, %forward, %replay, %stale = "ac.load_disposition"(
        %pending, %alias, %disjoint, %ready, %executed, %identity, %killed) <{
      lanes = 4 : i64
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
          !ac.var<i4>, !ac.var<i4>) -> (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
                                        !ac.var<i4>, !ac.var<i4>)
    %result = ac.var.record %applies, %wait, %bypass, %forward, %replay,
        %stale : !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
        !ac.var<i4>, !ac.var<i4> -> !ac.var<!ac.struct<@types::@Output>>
    %handshake = ac.marker.obligation %result state pending resolver handshake
        origin "order:return" path "true"
        : !ac.var<!ac.struct<@types::@Output>>
    ac.rule.return %handshake : !ac.var<!ac.struct<@types::@Output>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Input>>)
      -> !ac.queue<!ac.struct<@types::@Output>>
  ac.sink %output {ac.name = "sink"}
      : !ac.queue<!ac.struct<@types::@Output>>
}

// PLAN: "kind":"memory_order_edge"
// PLAN-SAME: "predicate":"must_wait"
// PLAN: "kind":"load_disposition_wait"
// PLAN: "kind":"load_disposition_bypass"
// PLAN: "kind":"load_disposition_forward"
// PLAN: "kind":"load_disposition_replay"
// PLAN: "kind":"load_disposition_stale"
// PYC: pyc.and
// PYC: pyc.not
// PYC: obligation_id = "no_stale_response:output"
// PYC-SAME: obligation_kind = "no_stale_response"
// CPP: const std::uint64_t stale = pending & (~identity | killed);
// CPP: const std::uint64_t qualified = pending & ~stale;
// CPP: const std::uint64_t bypass = qualified & disjoint & ~alias;
// PYC-CPP: obligation_no_stale_response_output_coverage = 0;
// EXEC: memory ordering PASS 200
// RTL: module memory_ordering
// RTL: obligation_no_stale_response_output: assert property
// RTL: obligation_no_stale_response_output_coverage: cover property
// RTL-EXEC: memory ordering RTL PASS 200
