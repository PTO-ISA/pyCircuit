// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc
// RUN: %FileCheck %s --check-prefix=PYC < %t.pyc
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.gfsim.cpp
// RUN: %FileCheck %s --check-prefix=CPP < %t.gfsim.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.gfsim.cpp
// RUN: %pycc %t.pyc --cpp %t.cpp --logic-depth=64
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -include %t.cpp %S/transaction-algebra-harness.cpp -o %t.run
// RUN: %t.run | %FileCheck %s --check-prefix=EXEC
// RUN: %pycc %t.pyc --verilog %t.sv --logic-depth=64
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t.sv --json-out %t.audit.json
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t.sv
// RUN: iverilog -g2012 -DSYNTHESIS -I%source_root/library/verilog -s tb_transaction_algebra -o %t.vvp %t.sv %S/transaction-algebra-tb.sv
// RUN: vvp %t.vvp | %FileCheck %s --check-prefix=RTL-EXEC

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "transaction_algebra"} {
  ac.type_scope @types {
    ac.struct @Input fields [
      {name = "valid", type = i4}, {name = "r0", type = i4},
      {name = "r1", type = i4}, {name = "r2", type = i4},
      {name = "candidates", type = i4},
      {name = "age0", type = i3}, {name = "age1", type = i3},
      {name = "age2", type = i3}, {name = "age3", type = i3},
      {name = "current", type = i4}, {name = "resolve", type = i4},
      {name = "kill", type = i4}, {name = "add", type = i4},
      {name = "identity", type = i4}, {name = "effects", type = i4},
      {name = "terminal", type = i4}, {name = "free", type = i4},
      {name = "release", type = i4}
    ]
    ac.struct @Output fields [
      {name = "reserved", type = i4}, {name = "accepted", type = i4},
      {name = "accepted_all", type = i4},
      {name = "accepted_independent", type = i4},
      {name = "allocation", type = i4},
      {name = "allocator_accepted", type = i4},
      {name = "commit_mask", type = i4},
      {name = "next_free", type = i4},
      {name = "allocation_reuse", type = i4},
      {name = "allocator_accepted_reuse", type = i4},
      {name = "next_free_reuse", type = i4},
      {name = "winners", type = i4},
      {name = "deps_next", type = i4}, {name = "ready", type = i1},
      {name = "completed", type = i4}
    ]
  } {dlti.dl_spec = #dlti.dl_spec<
      !ac.struct<@types::@Input> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 68 : i64},
      !ac.struct<@types::@Output> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 57 : i64}>}

  %input = ac.source depth 4 latency 1 {ac.name = "input"}
      : !ac.queue<!ac.struct<@types::@Input>>
  %output = ac.rule %input depths [4] latencies [1]
      name "dispatch" stable_id "dispatch" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Input>>):
    %valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %r0 = ac.var.get %item field "r0" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %r1 = ac.var.get %item field "r1" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %r2 = ac.var.get %item field "r2" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %candidates = ac.var.get %item field "candidates" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %age0 = ac.var.get %item field "age0" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i3>
    %age1 = ac.var.get %item field "age1" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i3>
    %age2 = ac.var.get %item field "age2" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i3>
    %age3 = ac.var.get %item field "age3" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i3>
    %current = ac.var.get %item field "current" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %resolve = ac.var.get %item field "resolve" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %kill = ac.var.get %item field "kill" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %add = ac.var.get %item field "add" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %identity = ac.var.get %item field "identity" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %effects = ac.var.get %item field "effects" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %terminal = ac.var.get %item field "terminal" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %free = ac.var.get %item field "free" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %release = ac.var.get %item field "release" : !ac.var<!ac.struct<@types::@Input>> -> !ac.var<i4>
    %reserved = "ac.reservation_set"(%r0, %r1, %r2) <{lanes = 4 : i64}>
        : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %accepted = "ac.transaction_group"(%valid, %reserved) <{
        lanes = 4 : i64, policy = #ac<transaction_group_policy valid_prefix>
    }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %accepted_all = "ac.transaction_group"(%valid, %reserved) <{
        lanes = 4 : i64, policy = #ac<transaction_group_policy all_or_none>
    }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %accepted_independent = "ac.transaction_group"(%valid, %reserved) <{
        lanes = 4 : i64, policy = #ac<transaction_group_policy independent>
    }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %allocation, %allocator_accepted, %next_free = "ac.multi_allocator"(
        %free, %accepted, %release) <{
      lanes = 4 : i64, reuse_policy = #ac<same_cycle_reuse_policy forbid>,
      generation_bits = 2 : i64, generation_policy = "increment_on_allocate"
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) ->
        (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
    %commit_mask = "ac.reservation_set"(
        %allocator_accepted, %allocator_accepted, %allocator_accepted) <{
      lanes = 4 : i64, commit = true
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %allocation_reuse, %allocator_accepted_reuse, %next_free_reuse =
        "ac.multi_allocator"(%free, %accepted, %release) <{
      lanes = 4 : i64, reuse_policy = #ac<same_cycle_reuse_policy allow>,
      generation_bits = 2 : i64, generation_policy = "increment_on_allocate"
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) ->
        (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
    %winners = "ac.age_select_k"(
        %candidates, %age0, %age1, %age2, %age3) <{
      lanes = 4 : i64, count = 2 : i64, ordering = "oldest_first"
    }> : (!ac.var<i4>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3>)
        -> !ac.var<i4>
    %deps_next, %ready = "ac.dependency_set"(
        %current, %add, %resolve, %kill, %identity) <{lanes = 4 : i64}>
        : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
        -> (!ac.var<i4>, !ac.var<i1>)
    %completed = "ac.terminal_transaction"(
        %commit_mask, %effects, %terminal) <{lanes = 4 : i64}>
        : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %result = ac.var.record %reserved, %accepted, %accepted_all,
        %accepted_independent, %allocation, %allocator_accepted, %commit_mask,
        %next_free, %allocation_reuse, %allocator_accepted_reuse,
        %next_free_reuse, %winners, %deps_next, %ready, %completed :
        !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
        !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
        !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i1>, !ac.var<i4>
        -> !ac.var<!ac.struct<@types::@Output>>
    %handshake = ac.marker.obligation %result state pending resolver handshake
        origin "dispatch:return" path "true"
        : !ac.var<!ac.struct<@types::@Output>>
    ac.rule.return %handshake : !ac.var<!ac.struct<@types::@Output>>
  } {ac.name = "output"} : (!ac.queue<!ac.struct<@types::@Input>>)
      -> !ac.queue<!ac.struct<@types::@Output>>
  ac.sink %output {ac.name = "sink"}
      : !ac.queue<!ac.struct<@types::@Output>>
}

// PLAN: "kind":"reservation_set"
// PLAN: "kind":"transaction_group"
// PLAN-SAME: "predicate":"valid_prefix"
// PLAN: "kind":"transaction_group"
// PLAN-SAME: "predicate":"all_or_none"
// PLAN: "kind":"transaction_group"
// PLAN-SAME: "predicate":"independent"
// PLAN: "kind":"multi_allocator_accepted"
// PLAN-SAME: "predicate":"forbid"
// PLAN: "kind":"multi_allocator_accepted"
// PLAN-SAME: "predicate":"allow"
// PLAN: "kind":"age_select_k"
// PLAN-SAME: "predicate":"oldest_first"
// PLAN: "kind":"dependency_set_next"
// PLAN: "kind":"terminal_transaction"
// PYC: pyc.popcount
// CPP: std::uint64_t candidate_free = free_mask | release_mask;
// CPP: std::array<std::uint64_t, 4> ages
// EXEC: transaction algebra PASS 200
// RTL-EXEC: transaction algebra RTL PASS 50
