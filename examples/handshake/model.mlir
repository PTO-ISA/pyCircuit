// Two-phase handshake: send, wait_until, recv, complete.
// The wait splits the process into pcs [@entry, @s1] so the queue xfer
// becomes visible before the receive.

builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i32 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }

  ac.module @Handshake() parameters {} graph {
    ac.queue @link payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "link" path "link"

    ac.process @agent kind "workload" {
      %seven = arith.constant 7 : i32
      %ready = arith.constant 1 : i1
      %ok = ac.try_send @link %seven : i32
      ac.wait_until %ready
      %value, %got = ac.try_recv @link : i32
      scf.if %got {
        ac.assert %got, "token"
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.system @soc root @Handshake as "root" tick 0 "cycle"
      workload @Handshake::@agent seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
