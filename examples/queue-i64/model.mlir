// i64 fifo datapath: two operands, add, complete with sum=5.

builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i64 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }

  ac.module @WideAdder() parameters {} graph {
    ac.queue @op_a payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "op_a" path "op_a"
    ac.queue @op_b payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "op_b" path "op_b"
    ac.queue @result payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "result" path "result"

    ac.process @source kind "workload" {
      %two = arith.constant 2 : i64
      %three = arith.constant 3 : i64
      %ok_a = ac.try_send @op_a %two : i64
      %ok_b = ac.try_send @op_b %three : i64
      ac.yield_sim
    }
    ac.process @alu kind "control" {
      %a, %got_a = ac.try_recv @op_a : i64
      %b, %got_b = ac.try_recv @op_b : i64
      %both = arith.andi %got_a, %got_b : i1
      scf.if %both {
        %sum = arith.addi %a, %b : i64
        %ok = ac.try_send @result %sum : i64
      }
      ac.yield_sim
    }
    ac.process @sink kind "control" {
      %value, %got = ac.try_recv @result : i64
      scf.if %got {
        ac.assert %got, "sum"
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.system @soc root @WideAdder as "root" tick 0 "cycle"
      workload @WideAdder::@source seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
