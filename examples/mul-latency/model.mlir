// Multi-cycle multiply: wait_for a shared ALU resource, then muli + stat.add.

builtin.module attributes {ac.contract_epoch = "0.3"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i32 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }

  ac.module @MulUnit() parameters {} graph {
    ac.queue @result payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "result" path "result"
    ac.resource @alu capacity 1 issue_width 1 ii 1
        latency {kind = "fixed", ticks = 1 : i64}
        lifecycle {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}
        ownership "exclusive" classes []
        id "alu" path "alu"
    ac.stat @muls kind "counter"

    ac.process @worker kind "workload" {
      %a = arith.constant 3 : i32
      %b = arith.constant 7 : i32
      %one = arith.constant 1 : i32
      ac.wait_for @alu
      %prod = arith.muli %a, %b : i32
      ac.stat.add @muls %one : i32
      %ok = ac.try_send @result %prod : i32
      ac.yield_sim
    }
    ac.process @sink kind "control" {
      %value, %got = ac.try_recv @result : i32
      scf.if %got {
        ac.assert %got, "product"
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.system @soc root @MulUnit as "root" tick 0 "cycle"
      workload @MulUnit::@worker seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
