// RUN: %split_file %s %t
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/per-key.mlir -o %t/per-key.frozen
// RUN: %not %acir_opt --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t/per-key.frozen -o %t/per-key.out 2>&1 | %FileCheck %s --check-prefix=ORDER
// RUN: test ! -s %t/per-key.out
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %t/width.mlir -o %t/width.frozen
// RUN: %not %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu %t/width.frozen -o %t/width.out 2>&1 | %FileCheck %s --check-prefix=WIDTH
// RUN: test ! -s %t/width.out

//--- per-key.mlir
builtin.module attributes {ac.contract_epoch = "0.1"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "Msg", fields = [{name = "key", type = i32}]}> : () -> ()
  }) : () -> ()
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload !ac.transaction<@types::@Msg> action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
    ac.guarantee "correlation" = "key"
  }
  ac.module @Top() parameters {} graph {
    ac.queue @q payload !ac.transaction<@types::@Msg> entries 1 ordering "per_key" protocol @rv
        ownership "exclusive" id "q" path "q"
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}

//--- width.mlir
builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i4 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }
  ac.module @Top() parameters {} graph {
    ac.queue @q payload i4 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "q" path "q"
    ac.process @workload kind "workload" {
      %v = arith.constant 1 : i4
      %ok = ac.try_send @q %v : i4
      ac.yield_sim
    }
    ac.return
  }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}

// ORDER: ACLOWER-UNSUPPORTED-CONSTRUCT
// WIDTH: ACLOWER-UNSUPPORTED-CONSTRUCT
