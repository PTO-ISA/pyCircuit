// RUN: %acir_opt_public %s | %FileCheck %s
// RUN: %acir_opt_public %s | %acir_opt_public | %FileCheck %s
// RUN: %acir_opt_public --pass-pipeline='builtin.module(canonicalize,cse)' %s | %FileCheck %s --check-prefix=EFFECTS

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.protocol @fifo {
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @done initial false terminal true
    ac.event @push from @sender to @receiver payload i64 action "offer"
    ac.transition from @idle to @done on @push transfer true retain false guard {}
  }
  ac.module @Observed(i1) parameters {} graph {
  ^bb0(%static_condition : i1):
    ac.queue @queue payload i64 entries 8 ordering "fifo" protocol @fifo
        ownership "exclusive" id "queue" path "queue"
    ac.require %static_condition, "static capacity contract"
    ac.ensure %static_condition, "static topology contract"
    ac.stat @cycles kind "counter"
    ac.stat @occupancy kind "gauge"
    ac.stat @latency kind "histogram"
    ac.stat @events kind "event_log"
    ac.process @monitor kind "monitor" {
      %true = arith.constant true
      %one = arith.constant 1 : i64
      ac.require %true, "capacity contract"
      ac.ensure %true, "latency contract"
      ac.assert %true, "runtime contract"
      %observed = ac.probe @queue kind "queue" : i64
      ac.stat.add @cycles %one : i64
      ac.instrumentation @debug {
        ac.stat.add @occupancy %observed : i64
      }
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK: ac.require %{{.*}}, "static capacity contract"
// CHECK: ac.ensure %{{.*}}, "static topology contract"
// CHECK: ac.stat @cycles kind "counter"
// CHECK: ac.stat @occupancy kind "gauge"
// CHECK: ac.stat @latency kind "histogram"
// CHECK: ac.stat @events kind "event_log"
// CHECK: ac.require
// CHECK: ac.ensure
// CHECK: ac.assert
// CHECK: ac.probe
// CHECK: ac.stat.add
// CHECK: ac.instrumentation @debug
// EFFECTS: ac.require
// EFFECTS: ac.ensure
// EFFECTS: ac.assert
// EFFECTS: ac.probe
// EFFECTS: ac.stat.add
// EFFECTS: ac.yield_sim
