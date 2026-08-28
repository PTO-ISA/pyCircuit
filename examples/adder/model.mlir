// 带激励和加法硬件的最小可仿真模型。
//
// 结构（v0.1 队列符号是模块内平坦引用，激励和 ALU 写在同一模块里）：
//   @source  workload  注入操作数 2 和 3
//   @alu     control   从队列取出、相加、写出
//   @sink    control   收到和之后结束仿真
//
// 端到端：examples/adder/run.sh

builtin.module attributes {ac.contract_epoch = "0.3"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i32 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }

  ac.module @Adder() parameters {} graph {
    ac.queue @op_a payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "op_a" path "op_a"
    ac.queue @op_b payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "op_b" path "op_b"
    ac.queue @result payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "result" path "result"

    ac.process @source kind "workload" {
      %two = arith.constant 2 : i32
      %three = arith.constant 3 : i32
      %ok_a = ac.try_send @op_a %two : i32
      %ok_b = ac.try_send @op_b %three : i32
      ac.yield_sim
    }

    ac.process @alu kind "control" {
      %a, %got_a = ac.try_recv @op_a : i32
      %b, %got_b = ac.try_recv @op_b : i32
      %both = arith.andi %got_a, %got_b : i1
      scf.if %both {
        %sum = arith.addi %a, %b : i32
        %ok = ac.try_send @result %sum : i32
      }
      ac.yield_sim
    }

    ac.process @sink kind "control" {
      %value, %got = ac.try_recv @result : i32
      scf.if %got {
        ac.assert %got, "sum"
      }
      ac.yield_sim
    }

    ac.return
  }

  ac.system @soc root @Adder as "root" tick 0 "cycle"
      workload @Adder::@source seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
