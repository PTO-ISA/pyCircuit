// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc
// RUN: %FileCheck %s --check-prefix=PYC < %t.pyc
// RUN: %pycc %t.pyc --cpp %t.cpp --logic-depth=64
// RUN: %FileCheck %s --check-prefix=CPP < %t.cpp
// RUN: %pycc %t.pyc --verilog %t.sv --logic-depth=64
// RUN: %FileCheck %s --check-prefix=RTL < %t.sv
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t.sv --json-out %t.audit.json

// Two dispositions in one rule must carry two distinct obligation IDs. A single
// block-scoped anchor used to collide and made both emitters reject the module.
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "memory_ordering_multi"} {
  ac.type_scope @types {
    ac.struct @Input fields [
      {name = "pending", type = i4}, {name = "alias", type = i4},
      {name = "disjoint", type = i4}, {name = "ready", type = i4},
      {name = "executed", type = i4}, {name = "identity", type = i4},
      {name = "producer", type = i4}, {name = "consumer", type = i4},
      {name = "killed", type = i4}
    ]
    ac.struct @Output fields [
      {name = "first", type = i4}, {name = "second", type = i4}
    ]
  } {dlti.dl_spec = #dlti.dl_spec<
      !ac.struct<@types::@Input> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 36 : i64},
      !ac.struct<@types::@Output> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 8 : i64}>}

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
    %killed = ac.var.get %item field "killed" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %wait_a, %bypass_a, %forward_a, %replay_a, %stale_a = "ac.load_disposition"(
        %pending, %alias, %disjoint, %ready, %executed, %identity, %killed) <{
      lanes = 4 : i64
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
          !ac.var<i4>, !ac.var<i4>) -> (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
                                        !ac.var<i4>, !ac.var<i4>)
    %wait_b, %bypass_b, %forward_b, %replay_b, %stale_b = "ac.load_disposition"(
        %pending, %disjoint, %alias, %ready, %executed, %identity, %killed) <{
      lanes = 4 : i64
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
          !ac.var<i4>, !ac.var<i4>) -> (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
                                        !ac.var<i4>, !ac.var<i4>)
    %first = "ac.reservation_set"(%stale_a, %wait_a) <{lanes = 4 : i64}>
        : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %second = "ac.reservation_set"(%stale_b, %wait_b) <{lanes = 4 : i64}>
        : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %result = ac.var.record %first, %second : !ac.var<i4>, !ac.var<i4>
        -> !ac.var<!ac.struct<@types::@Output>>
    %handshake = ac.marker.obligation %result state pending resolver handshake
        origin "order:return" path "true"
        : !ac.var<!ac.struct<@types::@Output>>
    ac.rule.return %handshake : !ac.var<!ac.struct<@types::@Output>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Input>>)
      -> !ac.queue<!ac.struct<@types::@Output>>
  ac.sink %output {ac.name = "sink"}
      : !ac.queue<!ac.struct<@types::@Output>>
}

// PYC: obligation_id = "no_stale_response:output:disposition0"
// PYC: obligation_id = "no_stale_response:output:disposition1"
// CPP: obligation_no_stale_response_output_disposition0_coverage = 0;
// CPP: obligation_no_stale_response_output_disposition1_coverage = 0;
// RTL: obligation_no_stale_response_output_disposition0: assert property
// RTL: obligation_no_stale_response_output_disposition1: assert property
