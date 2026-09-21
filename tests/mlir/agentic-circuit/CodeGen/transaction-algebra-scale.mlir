// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/n8.mlir -o %t/n8.frozen.mlir
// RUN: %acir_queue_pycgen %t/n8.frozen.mlir > %t/n8.pyc
// RUN: %pycc %t/n8.pyc --cpp %t/n8.cpp --logic-depth=512
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t/n8.cpp
// RUN: %pycc %t/n8.pyc --verilog %t/n8.sv --logic-depth=512
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t/n8.sv --json-out %t/n8.audit.json
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t/n8.sv
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/n10.mlir -o %t/n10.frozen.mlir
// RUN: %acir_queue_pycgen %t/n10.frozen.mlir > %t/n10.pyc
// RUN: %pycc %t/n10.pyc --cpp %t/n10.cpp --logic-depth=512
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t/n10.cpp
// RUN: %pycc %t/n10.pyc --verilog %t/n10.sv --logic-depth=512
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t/n10.sv --json-out %t/n10.audit.json
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t/n10.sv

//--- n8.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "transaction_scale_8"} {
  %input = ac.source depth 8 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.rule %input depths [8] latencies [1]
      name "scale8" stable_id "scale8" domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %reserved = "ac.reservation_set"(%item, %item, %item) <{lanes = 8 : i64}>
        : (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>) -> !ac.var<i8>
    %accepted = "ac.transaction_group"(%item, %reserved) <{
      lanes = 8 : i64, policy = #ac<transaction_group_policy valid_prefix>
    }> : (!ac.var<i8>, !ac.var<i8>) -> !ac.var<i8>
    %allocation, %allocator_accepted, %next_free = "ac.multi_allocator"(
        %item, %accepted, %item) <{
      lanes = 8 : i64, reuse_policy = #ac<same_cycle_reuse_policy allow>,
      generation_bits = 3 : i64, generation_policy = "increment_on_allocate"
    }> : (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>) ->
        (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>)
    %commit_mask = "ac.reservation_set"(
        %allocator_accepted, %allocator_accepted, %allocator_accepted) <{
      lanes = 8 : i64, commit = true
    }> : (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>) -> !ac.var<i8>
    %winners = "ac.age_select_k"(%item, %item, %item, %item, %item,
        %item, %item, %item, %item) <{
      lanes = 8 : i64, count = 4 : i64, ordering = "oldest_first"
    }> : (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>,
          !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>) -> !ac.var<i8>
    %deps, %ready = "ac.dependency_set"(
        %item, %allocation, %item, %item, %item) <{lanes = 8 : i64}>
        : (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>)
        -> (!ac.var<i8>, !ac.var<i1>)
    %completed = "ac.terminal_transaction"(
        %commit_mask, %winners, %reserved) <{lanes = 8 : i64}>
        : (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>) -> !ac.var<i8>
    %handshake = ac.marker.obligation %completed state pending resolver handshake
        origin "scale8:return" path "true" : !ac.var<i8>
    ac.rule.return %handshake : !ac.var<i8>
  } {ac.name = "output"} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

//--- n10.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "transaction_scale_10"} {
  %input = ac.source depth 10 latency 1 {ac.name = "input"} : !ac.queue<i10>
  %output = ac.rule %input depths [10] latencies [1]
      name "scale10" stable_id "scale10" domain "cycle" type exact {
  ^body(%item: !ac.var<i10>):
    %reserved = "ac.reservation_set"(%item, %item, %item) <{lanes = 10 : i64}>
        : (!ac.var<i10>, !ac.var<i10>, !ac.var<i10>) -> !ac.var<i10>
    %accepted = "ac.transaction_group"(%item, %reserved) <{
      lanes = 10 : i64, policy = #ac<transaction_group_policy valid_prefix>
    }> : (!ac.var<i10>, !ac.var<i10>) -> !ac.var<i10>
    %allocation, %allocator_accepted, %next_free = "ac.multi_allocator"(
        %item, %accepted, %item) <{
      lanes = 10 : i64, reuse_policy = #ac<same_cycle_reuse_policy allow>,
      generation_bits = 4 : i64, generation_policy = "increment_on_allocate"
    }> : (!ac.var<i10>, !ac.var<i10>, !ac.var<i10>) ->
        (!ac.var<i10>, !ac.var<i10>, !ac.var<i10>)
    %commit_mask = "ac.reservation_set"(
        %allocator_accepted, %allocator_accepted, %allocator_accepted) <{
      lanes = 10 : i64, commit = true
    }> : (!ac.var<i10>, !ac.var<i10>, !ac.var<i10>) -> !ac.var<i10>
    %winners = "ac.age_select_k"(%item, %item, %item, %item, %item,
        %item, %item, %item, %item, %item, %item) <{
      lanes = 10 : i64, count = 4 : i64, ordering = "oldest_first"
    }> : (!ac.var<i10>, !ac.var<i10>, !ac.var<i10>, !ac.var<i10>,
          !ac.var<i10>, !ac.var<i10>, !ac.var<i10>, !ac.var<i10>,
          !ac.var<i10>, !ac.var<i10>, !ac.var<i10>) -> !ac.var<i10>
    %deps, %ready = "ac.dependency_set"(
        %item, %allocation, %item, %item, %item) <{lanes = 10 : i64}>
        : (!ac.var<i10>, !ac.var<i10>, !ac.var<i10>, !ac.var<i10>,
           !ac.var<i10>) -> (!ac.var<i10>, !ac.var<i1>)
    %completed = "ac.terminal_transaction"(
        %commit_mask, %winners, %reserved) <{lanes = 10 : i64}>
        : (!ac.var<i10>, !ac.var<i10>, !ac.var<i10>) -> !ac.var<i10>
    %handshake = ac.marker.obligation %completed state pending resolver handshake
        origin "scale10:return" path "true" : !ac.var<i10>
    ac.rule.return %handshake : !ac.var<i10>
  } {ac.name = "output"} : (!ac.queue<i10>) -> !ac.queue<i10>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i10>
}
