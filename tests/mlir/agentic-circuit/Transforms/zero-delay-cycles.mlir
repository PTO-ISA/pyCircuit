// RUN: %split_file %s %t
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-verify-model)' %t/self-loop.mlir 2>&1 | %FileCheck %s --check-prefix=SELF
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-verify-model)' %t/multi-node.mlir 2>&1 | %FileCheck %s --check-prefix=MULTI
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/stateful-edge.mlir | %FileCheck %s --check-prefix=STATEFUL
// RUN: %not %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-verify-model)' %t/zero-delay-stateful.mlir 2>&1 | %FileCheck %s --check-prefix=STATEFUL-ZERO

//--- self-loop.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
  "ac.module"() <{sym_name = "Pure", function_type = (i32) -> i32, static_params = {}}> ({
  ^bb0(%arg0 : i32):
    "ac.return"(%arg0) : (i32) -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "Top", function_type = () -> (), static_params = {}}> ({
    %x = "ac.instance"(%x) <{definition = @Pure, sym_name = "x", stable_id = "x", path = "x", static_args = {}}> : (i32) -> i32
    ac.process @workload kind "workload" { ac.yield_sim }
    "ac.return"() : () -> ()
  }) : () -> ()
}
// SELF: forbidden zero-delay cycle: root.x -> root.x

//--- multi-node.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
  "ac.module"() <{sym_name = "Pure", function_type = (i32) -> i32, static_params = {}}> ({
  ^bb0(%arg0 : i32):
    "ac.return"(%arg0) : (i32) -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "Top", function_type = () -> (), static_params = {}}> ({
    %a = "ac.instance"(%b) <{definition = @Pure, sym_name = "a", stable_id = "a", path = "a", static_args = {}}> : (i32) -> i32
    %b = "ac.instance"(%a) <{definition = @Pure, sym_name = "b", stable_id = "b", path = "b", static_args = {}}> : (i32) -> i32
    ac.process @workload kind "workload" { ac.yield_sim }
    "ac.return"() : () -> ()
  }) : () -> ()
}
// MULTI: forbidden zero-delay cycle: root.a -> root.b -> root.a

//--- stateful-edge.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
  "ac.module"() <{sym_name = "Pure", function_type = (i32) -> i32, static_params = {}}> ({
  ^bb0(%arg0 : i32):
    "ac.return"(%arg0) : (i32) -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "Stateful", function_type = (i32) -> i32, static_params = {}}> ({
  ^bb0(%arg0 : i32):
    ac.process @state kind "control" { ac.yield_sim }
    "ac.return"(%arg0) : (i32) -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "Top", function_type = () -> (), static_params = {}}> ({
    %a = "ac.instance"(%b) <{definition = @Stateful, sym_name = "a", stable_id = "a", path = "a", static_args = {}}> : (i32) -> i32
    %b = "ac.instance"(%a) <{definition = @Pure, sym_name = "b", stable_id = "b", path = "b", static_args = {}}> : (i32) -> i32
    ac.process @workload kind "workload" { ac.yield_sim }
    "ac.return"() : () -> ()
  }) : () -> ()
}
// STATEFUL: ac.topology_frozen = true

//--- zero-delay-stateful.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.protocol @p {
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @done initial false terminal true
    ac.event @push from @sender to @receiver payload i32 action "offer"
    ac.transition from @idle to @done on @push transfer true retain false guard {}
  }
  "ac.module"() <{sym_name = "Top", function_type = () -> (), static_params = {}}> ({
    "ac.queue"() <{sym_name = "q", stable_id = "q", path = "q", payload = i32, entry_capacity = 1 : i64, ordering = "fifo", protocol = @p, ownership = "exclusive", delay_ticks = 0 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// STATEFUL-ZERO: stateful declaration delay_ticks must be exactly one positive tick
