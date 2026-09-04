// RUN: %acir_opt_public %s | %FileCheck %s
// RUN: %acir_opt_public %s | %acir_opt_public | %FileCheck %s
// RUN: %acir_opt_public --pass-pipeline='builtin.module(canonicalize,cse)' %s | %FileCheck %s --check-prefix=EFFECTS

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.protocol @fifo {
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @done initial false terminal true
    ac.event @push from @sender to @receiver payload i32 action "offer"
    ac.transition from @idle to @done on @push transfer true retain false guard {}
  }
  ac.module @Top(i32) parameters {} graph {
  ^bb0(%arg0 : i32):
    ac.time_domain @core period 1 phase 0 scale 1
    ac.queue @ready payload i32 entries 8 ordering "fifo" protocol @fifo
        ownership "exclusive" id "ready" path "ready"
    ac.event_queue @done payload !ac.event<i32> capacity 8
        ordering "time_then_sequence" domain @core id "done" path "done"
    ac.resource @compute capacity 1 issue_width 1 ii 1
        latency {kind = "fixed", ticks = 1 : i64}
        lifecycle {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}
        ownership "exclusive" classes [] id "compute" path "compute"
    ac.process @worker kind "workload" captures(%arg0 : i32) {
    ^bb0(%value : i32):
      ac.yield_sim
    }
    ac.process @control kind "control" captures(%arg0 : i32) {
    ^bb0(%capture : i32):
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %idx = index.constant 0
      %ready = arith.cmpi eq, %c0, %c0 : i64
      %accepted = ac.try_send @ready %capture : i32
      %value, %received = ac.try_recv @ready : i32
      ac.schedule @worker %value after %c1 : i32
      ac.wait_until %ready
      ac.wait_for @compute
      ac.await_event @done
      scf.if %accepted {
        ac.assert %received, "receive follows accepted send"
      }
      ac.yield_sim
    }
    ac.return
  }
  ac.module @BranchLocal(i1) parameters {} graph {
  ^bb0(%condition : i1):
    ac.process @control kind "control" captures(%condition : i1) {
    ^bb0(%branch : i1):
      %cursor = ac.trace.open source "branch_linear"
      %next, %captured, %advanced = ac.trace.next %cursor from source "branch_linear" : !ac.resource_token<@r>
      scf.if %branch {
        ac.wait_until %branch
      } else {
        %decoded = ac.trace.decode %captured : !ac.resource_token<@r> to i64
      }
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK: ac.process @control kind "control" captures(%{{.*}} : i32)
// CHECK: ac.try_send @ready
// CHECK: ac.try_recv @ready
// CHECK: ac.schedule @worker
// CHECK: ac.wait_until
// CHECK: ac.wait_for @compute
// CHECK: ac.await_event @done
// CHECK: scf.if
// CHECK: ac.yield_sim
// CHECK: ac.module @BranchLocal
// EFFECTS: ac.try_send
// EFFECTS: ac.try_recv
// EFFECTS: ac.schedule
// EFFECTS: ac.wait_until
// EFFECTS: ac.wait_for
// EFFECTS: ac.await_event
// EFFECTS: ac.yield_sim
