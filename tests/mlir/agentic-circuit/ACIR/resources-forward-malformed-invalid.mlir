// RUN: %split_file %s %t
// RUN: %not %acir_opt_public %t/queue-before-protocol.mlir 2>&1 | %FileCheck %s --check-prefix=ORDERING
// RUN: %not %acir_opt_public %t/protocol-before-queue.mlir 2>&1 | %FileCheck %s --check-prefix=CORRELATION

//--- queue-before-protocol.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Top() parameters {} graph {
    ac.queue @ready payload i32 entries 8 ordering "fifo" protocol @p
        ownership "exclusive" id "ready" path "ready"
    ac.return
  }
  ac.protocol @p {
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @done initial false terminal true
    ac.event @push from @sender to @receiver payload i32 action "offer"
    ac.transition from @idle to @done on @push transfer true retain false guard {}
    ac.guarantee "ordering" = 1 : i64
    ac.guarantee "delivery" = "exactly_once"
    ac.guarantee "completion" = "on_terminal_phase"
    ac.guarantee "backpressure" = "capacity"
    ac.guarantee "stable_pending" = true
    ac.guarantee "max_inflight" = 1 : i64
  }
}
// ORDERING: protocol ordering guarantee must be a string

//--- protocol-before-queue.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.protocol @p {
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @done initial false terminal true
    ac.event @push from @sender to @receiver payload i32 action "offer"
    ac.transition from @idle to @done on @push transfer true retain false guard {}
    ac.guarantee "ordering" = "per_key"
    ac.guarantee "correlation" = 1 : i64
    ac.guarantee "delivery" = "exactly_once"
    ac.guarantee "completion" = "on_terminal_phase"
    ac.guarantee "backpressure" = "capacity"
    ac.guarantee "stable_pending" = true
    ac.guarantee "max_inflight" = 1 : i64
  }
  ac.module @Top() parameters {} graph {
    ac.queue @ready payload i32 entries 8 ordering "per_key" protocol @p
        ownership "exclusive" id "ready" path "ready"
    ac.return
  }
}
// CORRELATION: protocol correlation guarantee must be a non-empty string
