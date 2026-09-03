// RUN: %acir_opt_public %s | %FileCheck %s
// RUN: %acir_opt_public %s | %acir_opt_public | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.protocol @fifo {
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @pending initial false terminal false
    ac.state @done initial false terminal true
    ac.event @push from @sender to @receiver payload i32 action "offer"
    ac.event @accept from @receiver to @sender payload i32 action "accept"
    ac.event @cancel from @sender to @receiver payload i32 action "cancel"
    ac.transition from @idle to @pending on @push transfer false retain true guard {}
    ac.transition from @pending to @done on @accept transfer true retain false guard {}
    ac.transition from @pending to @done on @cancel transfer false retain false guard {}
    ac.guarantee "ordering" = "unordered"
    ac.guarantee "delivery" = "exactly_once"
    ac.guarantee "completion" = "on_accept"
    ac.guarantee "backpressure" = "capacity"
    ac.guarantee "stable_pending" = true
    ac.guarantee "max_inflight" = 1 : i64
  }
  ac.interface @QueueLink {
    ac.role @source dual @sink cardinality "exclusive"
    ac.role @sink dual @source cardinality "exclusive"
    ac.port @data : !ac.channel<i32, @fifo> from @source to @sink
        protocol_roles @sender to @receiver
  }
  ac.module @Top() parameters {} graph {
    ac.time_domain @core period 2 phase 0 scale 1
    ac.queue @ready payload i32 entries 8 bytes 64 ordering "fifo" protocol @fifo
        ownership "exclusive" id "ready" path "ready" watermarks {low = 2 : i64, high = 6 : i64}
    ac.event_queue @done payload !ac.event<i32> capacity 16 ordering "time_then_sequence"
        domain @core id "done" path "done"
    ac.instance @scheduler of @Arb() static {} id "scheduler" path "scheduler" : () -> ()
    ac.resource @compute capacity 4 issue_width 2 ii 1
        latency {kind = "fixed", ticks = 3 : i64}
        lifecycle {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}
        ownership "shared" arbiter @scheduler classes []
        id "compute" path "compute"
    ac.return
  }
  ac.module @Arb() parameters {} graph { ac.return }
}

// CHECK: ac.interface @QueueLink
// CHECK: ac.port @data
// CHECK: ac.queue @ready
// CHECK: ac.event_queue @done
// CHECK: ac.resource @compute
