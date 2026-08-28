// 最小 RISC-V 核：取指 / 解码 / 执行 / 写回（每拍流水线里只有一条指令）。
//
// 程序（RV32I）：
//   0: addi x1, x0, 2
//   4: addi x2, x0, 3
//   8: add  x3, x1, x2
//   c: ecall            ; 写回阶段结束仿真，诊断 x3=5
//
// 保留队列名（Owner 设备，不是 FIFO）：
//   @pc    程序计数器
//   @rf    32 项寄存器堆（send+recv = 读；连续两次 send = 写；x0 恒 0）
//   @busy  多周期占用：取指置 1，写回清 0
//
// 端到端：examples/riscv-mini/run.sh

builtin.module attributes {ac.contract_epoch = "0.3"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i32 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }

  ac.module @Core() parameters {} graph {
    ac.queue @pc payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "pc" path "pc" watermarks {kind = "register"}
    ac.queue @rf payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "rf" path "rf" watermarks {kind = "regfile"}
    ac.queue @busy payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}

    ac.queue @if_id_instr payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "if_id_instr" path "if_id_instr"
    ac.queue @id_ex_a payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "id_ex_a" path "id_ex_a"
    ac.queue @id_ex_b payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "id_ex_b" path "id_ex_b"
    ac.queue @id_ex_rd payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "id_ex_rd" path "id_ex_rd"
    ac.queue @id_ex_op payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "id_ex_op" path "id_ex_op"
    ac.queue @ex_wb_rd payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ex_wb_rd" path "ex_wb_rd"
    ac.queue @ex_wb_val payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ex_wb_val" path "ex_wb_val"
    ac.queue @ex_wb_halt payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ex_wb_halt" path "ex_wb_halt"

    ac.process @fetch kind "workload" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c4 = arith.constant 4 : i32
      %c8 = arith.constant 8 : i32
      %c12 = arith.constant 12 : i32
      %nop = arith.constant 0x00000013 : i32
      %i0 = arith.constant 0x00200093 : i32
      %i1 = arith.constant 0x00300113 : i32
      %i2 = arith.constant 0x002081B3 : i32
      %i3 = arith.constant 0x00000073 : i32
      %busy, %bgot = ac.try_recv @busy : i32
      %idle = arith.cmpi eq, %busy, %c0 : i32
      scf.if %idle {
        %pc, %pgot = ac.try_recv @pc : i32
        %is0 = arith.cmpi eq, %pc, %c0 : i32
        %is4 = arith.cmpi eq, %pc, %c4 : i32
        %is8 = arith.cmpi eq, %pc, %c8 : i32
        %is12 = arith.cmpi eq, %pc, %c12 : i32
        %w0 = arith.select %is12, %i3, %nop : i32
        %w1 = arith.select %is8, %i2, %w0 : i32
        %w2 = arith.select %is4, %i1, %w1 : i32
        %instr = arith.select %is0, %i0, %w2 : i32
        %ok_i = ac.try_send @if_id_instr %instr : i32
        %npc = arith.addi %pc, %c4 : i32
        %ok_p = ac.try_send @pc %npc : i32
        %ok_b = ac.try_send @busy %c1 : i32
      }
      ac.yield_sim
    }

    ac.process @decode kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c2 = arith.constant 2 : i32
      %c7 = arith.constant 7 : i32
      %c12 = arith.constant 12 : i32
      %c13 = arith.constant 0x13 : i32
      %c15 = arith.constant 15 : i32
      %c20 = arith.constant 20 : i32
      %c33 = arith.constant 0x33 : i32
      %c73 = arith.constant 0x73 : i32
      %c7f = arith.constant 0x7f : i32
      %c1f = arith.constant 0x1f : i32
      %instr, %got = ac.try_recv @if_id_instr : i32
      scf.if %got {
        %opcode = arith.andi %instr, %c7f : i32
        %sh7 = arith.shrui %instr, %c7 : i32
        %rd = arith.andi %sh7, %c1f : i32
        %sh12 = arith.shrui %instr, %c12 : i32
        %funct3 = arith.andi %sh12, %c7 : i32
        %sh15 = arith.shrui %instr, %c15 : i32
        %rs1 = arith.andi %sh15, %c1f : i32
        %sh20 = arith.shrui %instr, %c20 : i32
        %rs2 = arith.andi %sh20, %c1f : i32
        %ok_rs1 = ac.try_send @rf %rs1 : i32
        %a, %ga = ac.try_recv @rf : i32
        %ok_rs2 = ac.try_send @rf %rs2 : i32
        %b_rs2, %gb = ac.try_recv @rf : i32
        %is_opimm = arith.cmpi eq, %opcode, %c13 : i32
        %is_op = arith.cmpi eq, %opcode, %c33 : i32
        %is_sys = arith.cmpi eq, %opcode, %c73 : i32
        %f3z = arith.cmpi eq, %funct3, %c0 : i32
        %is_addi = arith.andi %is_opimm, %f3z : i1
        %is_add = arith.andi %is_op, %f3z : i1
        %b = arith.select %is_addi, %sh20, %b_rs2 : i32
        %op_tmp = arith.select %is_add, %c1, %c0 : i32
        %op = arith.select %is_sys, %c2, %op_tmp : i32
        %s0 = ac.try_send @id_ex_a %a : i32
        %s1 = ac.try_send @id_ex_b %b : i32
        %s2 = ac.try_send @id_ex_rd %rd : i32
        %s3 = ac.try_send @id_ex_op %op : i32
      }
      ac.yield_sim
    }

    ac.process @execute kind "control" {
      %c0 = arith.constant 0 : i32
      %c2 = arith.constant 2 : i32
      %c3 = arith.constant 3 : i32
      %a, %ga = ac.try_recv @id_ex_a : i32
      %b, %gb = ac.try_recv @id_ex_b : i32
      %rd, %grd = ac.try_recv @id_ex_rd : i32
      %op, %gop = ac.try_recv @id_ex_op : i32
      %t0 = arith.andi %ga, %gb : i1
      %t1 = arith.andi %grd, %gop : i1
      %all = arith.andi %t0, %t1 : i1
      scf.if %all {
        %sum = arith.addi %a, %b : i32
        %is_halt = arith.cmpi eq, %op, %c2 : i32
        %wb_rd = arith.select %is_halt, %c0, %rd : i32
        %s0 = ac.try_send @ex_wb_rd %wb_rd : i32
        %s1 = ac.try_send @ex_wb_val %sum : i32
        %ok = ac.try_send @rf %c3 : i32
        %x3, %gx = ac.try_recv @rf : i32
        %hval = arith.select %is_halt, %x3, %c0 : i32
        %s2 = ac.try_send @ex_wb_halt %hval : i32
      }
      ac.yield_sim
    }

    ac.process @writeback kind "control" {
      %c0 = arith.constant 0 : i32
      %rd, %grd = ac.try_recv @ex_wb_rd : i32
      %val, %gv = ac.try_recv @ex_wb_val : i32
      %do = arith.andi %grd, %gv : i1
      scf.if %do {
        %w0 = ac.try_send @rf %rd : i32
        %w1 = ac.try_send @rf %val : i32
        %clr = ac.try_send @busy %c0 : i32
      }
      %h, %gh = ac.try_recv @ex_wb_halt : i32
      %nz = arith.cmpi ne, %h, %c0 : i32
      %halt = arith.andi %gh, %nz : i1
      scf.if %halt {
        ac.assert %halt, "x3"
      }
      ac.yield_sim
    }

    ac.return
  }

  ac.system @soc root @Core as "root" tick 0 "cycle"
      workload @Core::@fetch seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
