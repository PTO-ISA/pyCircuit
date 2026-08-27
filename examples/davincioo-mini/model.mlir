// DavinciOO-shaped mini out-of-order core.
//
// The Core owns every inter-stage FIFO. Each pipeline stage is a child module
// whose process accesses those FIFOs through @Core::@queue references.
builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i32 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }

  ac.module @TraceSource() parameters {} graph {
    ac.queue @pc payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "pc" path "pc" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c2 = arith.constant 2 : i32
      %c3 = arith.constant 3 : i32
      %c4 = arith.constant 4 : i32
      %c5 = arith.constant 5 : i32
      %c6 = arith.constant 6 : i32
      %i1 = arith.constant 4195329 : i32
      %i2 = arith.constant 4212738 : i32
      %i3 = arith.constant 8424706 : i32
      %i4 = arith.constant 12636775 : i32
      %i5 = arith.constant 8394500 : i32
      %i6 = arith.constant 4281569 : i32
      %pc, %pc_valid = ac.try_recv @pc : i32
      %active = arith.cmpi ult, %pc, %c6 : i32
      scf.if %active {
        %is1 = arith.cmpi eq, %pc, %c1 : i32
        %is2 = arith.cmpi eq, %pc, %c2 : i32
        %is3 = arith.cmpi eq, %pc, %c3 : i32
        %is4 = arith.cmpi eq, %pc, %c4 : i32
        %is5 = arith.cmpi eq, %pc, %c5 : i32
        %v5 = arith.select %is5, %i6, %i1 : i32
        %v4 = arith.select %is4, %i5, %v5 : i32
        %v3 = arith.select %is3, %i4, %v4 : i32
        %v2 = arith.select %is2, %i3, %v3 : i32
        %inst = arith.select %is1, %i2, %v2 : i32
        %sent = ac.try_send @Core::@rob_in %inst : i32
        scf.if %sent {
          %next = arith.addi %pc, %c1 : i32
          %stored = ac.try_send @pc %next : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @ROB() parameters {} graph {
    ac.queue @head payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "head" path "head" watermarks {kind = "register"}
    ac.queue @tail payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "tail" path "tail" watermarks {kind = "register"}
    ac.queue @done payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done" path "done" watermarks {kind = "regfile"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c6 = arith.constant 6 : i32
      %c26 = arith.constant 26 : i32

      %incoming, %has_incoming = ac.try_recv @Core::@rob_in : i32
      scf.if %has_incoming {
        %tail, %tail_valid = ac.try_recv @tail : i32
        %rid = arith.addi %tail, %c1 : i32
        %rid_bits = arith.shli %rid, %c26 : i32
        %stamped = arith.ori %incoming, %rid_bits : i32
        %forwarded = ac.try_send @Core::@rob_to_rename %stamped : i32
        scf.if %forwarded {
          %tail_stored = ac.try_send @tail %rid : i32
        }
      }

      %completed, %has_completed = ac.try_recv @Core::@rob_done : i32
      scf.if %has_completed {
        %rid_shifted = arith.shrui %completed, %c26 : i32
        %done_index = ac.try_send @done %rid_shifted : i32
        %done_value = ac.try_send @done %c1 : i32
      }

      %head, %head_valid = ac.try_recv @head : i32
      %next_head = arith.addi %head, %c1 : i32
      %read_index = ac.try_send @done %next_head : i32
      %is_done, %done_valid = ac.try_recv @done : i32
      %can_retire = arith.cmpi eq, %is_done, %c1 : i32
      scf.if %can_retire {
        %head_stored = ac.try_send @head %next_head : i32
        %clear_index = ac.try_send @done %next_head : i32
        %clear_value = ac.try_send @done %c0 : i32
        %retired, %retired_valid = ac.try_recv @Core::@retired : i32
        %new_retired = arith.addi %retired, %c1 : i32
        %retired_stored = ac.try_send @Core::@retired %new_retired : i32
        %reported, %reported_valid = ac.try_recv @Core::@retired : i32
        %six = arith.cmpi eq, %reported, %c6 : i32
        %finish = arith.andi %reported_valid, %six : i1
        scf.if %finish {
          ac.assert %finish, "retired"
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @Rename() parameters {} graph {
    ac.process @step kind "control" {
      %inst, %valid = ac.try_recv @Core::@rob_to_rename : i32
      scf.if %valid {
        %sent = ac.try_send @Core::@rename_to_dispatch %inst : i32
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @Dispatch() parameters {} graph {
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c2 = arith.constant 2 : i32
      %c3 = arith.constant 3 : i32
      %c8 = arith.constant 8 : i32
      %c3mask = arith.constant 3 : i32
      %inst, %valid = ac.try_recv @Core::@rename_to_dispatch : i32
      scf.if %valid {
        %shifted = arith.shrui %inst, %c8 : i32
        %engine = arith.andi %shifted, %c3mask : i32
        %is_s = arith.cmpi eq, %engine, %c0 : i32
        %is_v = arith.cmpi eq, %engine, %c1 : i32
        %is_c = arith.cmpi eq, %engine, %c2 : i32
        %is_t = arith.cmpi eq, %engine, %c3 : i32
        scf.if %is_s {
          %sent_s = ac.try_send @Core::@dispatch_to_iq_s %inst : i32
        }
        scf.if %is_v {
          %sent_v = ac.try_send @Core::@dispatch_to_iq_v %inst : i32
        }
        scf.if %is_c {
          %sent_c = ac.try_send @Core::@dispatch_to_iq_c %inst : i32
        }
        scf.if %is_t {
          %sent_t = ac.try_send @Core::@dispatch_to_iq_t %inst : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @ReadyTable() parameters {} graph {
    ac.process @step kind "control" {
      %completed, %valid = ac.try_recv @Core::@wakeup : i32
      scf.if %valid {
        %rob = ac.try_send @Core::@rob_done %completed : i32
        %s = ac.try_send @Core::@ready_to_iq_s %completed : i32
        %v = ac.try_send @Core::@ready_to_iq_v %completed : i32
        %c = ac.try_send @Core::@ready_to_iq_c %completed : i32
        %t = ac.try_send @Core::@ready_to_iq_t %completed : i32
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @IssueQueueS() parameters {} graph {
    ac.queue @slot0 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot0" path "slot0" watermarks {kind = "register"}
    ac.queue @slot1 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot1" path "slot1" watermarks {kind = "register"}
    ac.queue @slot2 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot2" path "slot2" watermarks {kind = "register"}
    ac.queue @slot3 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot3" path "slot3" watermarks {kind = "register"}
    ac.queue @ready payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready" path "ready" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c10 = arith.constant 10 : i32
      %c14 = arith.constant 14 : i32
      %c15 = arith.constant 15 : i32
      %false = arith.constant false
      %update, %has_update = ac.try_recv @Core::@ready_to_iq_s : i32
      scf.if %has_update {
        %dst_shift = arith.shrui %update, %c10 : i32
        %dst = arith.andi %dst_shift, %c15 : i32
        %bit = arith.shli %c1, %dst : i32
        %bits, %bits_valid = ac.try_recv @ready : i32
        %new_bits = arith.ori %bits, %bit : i32
        %bits_stored = ac.try_send @ready %new_bits : i32
      }
      %s0, %g0 = ac.try_recv @slot0 : i32
      %s1, %g1 = ac.try_recv @slot1 : i32
      %s2, %g2 = ac.try_recv @slot2 : i32
      %s3, %g3 = ac.try_recv @slot3 : i32
      %bits_now, %bits_now_valid = ac.try_recv @ready : i32
      %src0_shift = arith.shrui %s0, %c14 : i32
      %src0 = arith.andi %src0_shift, %c15 : i32
      %mask0 = arith.shli %c1, %src0 : i32
      %hit0 = arith.andi %bits_now, %mask0 : i32
      %zero_src0 = arith.cmpi eq, %src0, %c0 : i32
      %ready_src0 = arith.cmpi ne, %hit0, %c0 : i32
      %dep0 = arith.ori %zero_src0, %ready_src0 : i1
      %valid0 = arith.cmpi ne, %s0, %c0 : i32
      %issue0 = arith.andi %valid0, %dep0 : i1
      scf.if %issue0 {
        %issued = ac.try_send @Core::@iq_to_eng_s %s0 : i32
        scf.if %issued {
          %cleared = ac.try_send @slot0 %c0 : i32
        }
      }
      %not_issue = arith.cmpi eq, %issue0, %false : i1
      scf.if %not_issue {
        %incoming, %has_incoming = ac.try_recv @Core::@dispatch_to_iq_s : i32
        scf.if %has_incoming {
          %empty0 = arith.cmpi eq, %s0, %c0 : i32
          %empty1 = arith.cmpi eq, %s1, %c0 : i32
          %empty2 = arith.cmpi eq, %s2, %c0 : i32
          %empty3 = arith.cmpi eq, %s3, %c0 : i32
          scf.if %empty0 {
            %put0 = ac.try_send @slot0 %incoming : i32
          } else {
            scf.if %empty1 {
              %put1 = ac.try_send @slot1 %incoming : i32
            } else {
              scf.if %empty2 {
                %put2 = ac.try_send @slot2 %incoming : i32
              } else {
                scf.if %empty3 {
                  %put3 = ac.try_send @slot3 %incoming : i32
                }
              }
            }
          }
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @IssueQueueV() parameters {} graph {
    ac.queue @slot0 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot0" path "slot0" watermarks {kind = "register"}
    ac.queue @slot1 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot1" path "slot1" watermarks {kind = "register"}
    ac.queue @slot2 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot2" path "slot2" watermarks {kind = "register"}
    ac.queue @slot3 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot3" path "slot3" watermarks {kind = "register"}
    ac.queue @ready payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready" path "ready" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c10 = arith.constant 10 : i32
      %c14 = arith.constant 14 : i32
      %c15 = arith.constant 15 : i32
      %update, %has_update = ac.try_recv @Core::@ready_to_iq_v : i32
      scf.if %has_update {
        %dst_shift = arith.shrui %update, %c10 : i32
        %dst = arith.andi %dst_shift, %c15 : i32
        %bit = arith.shli %c1, %dst : i32
        %bits, %bits_valid = ac.try_recv @ready : i32
        %new_bits = arith.ori %bits, %bit : i32
        %bits_stored = ac.try_send @ready %new_bits : i32
      }
      %slot, %slot_valid = ac.try_recv @slot0 : i32
      %bits_now, %bits_now_valid = ac.try_recv @ready : i32
      %src_shift = arith.shrui %slot, %c14 : i32
      %src = arith.andi %src_shift, %c15 : i32
      %mask = arith.shli %c1, %src : i32
      %hit = arith.andi %bits_now, %mask : i32
      %src_zero = arith.cmpi eq, %src, %c0 : i32
      %src_ready = arith.cmpi ne, %hit, %c0 : i32
      %dep = arith.ori %src_zero, %src_ready : i1
      %occupied = arith.cmpi ne, %slot, %c0 : i32
      %issue = arith.andi %occupied, %dep : i1
      scf.if %issue {
        %sent = ac.try_send @Core::@iq_to_eng_v %slot : i32
        scf.if %sent {
          %clear = ac.try_send @slot0 %c0 : i32
        }
      }
      %idle = arith.cmpi eq, %slot, %c0 : i32
      scf.if %idle {
        %incoming, %has_incoming = ac.try_recv @Core::@dispatch_to_iq_v : i32
        scf.if %has_incoming {
          %store = ac.try_send @slot0 %incoming : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @IssueQueueC() parameters {} graph {
    ac.queue @slot0 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot0" path "slot0" watermarks {kind = "register"}
    ac.queue @slot1 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot1" path "slot1" watermarks {kind = "register"}
    ac.queue @slot2 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot2" path "slot2" watermarks {kind = "register"}
    ac.queue @slot3 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot3" path "slot3" watermarks {kind = "register"}
    ac.queue @ready payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready" path "ready" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c10 = arith.constant 10 : i32
      %c14 = arith.constant 14 : i32
      %c15 = arith.constant 15 : i32
      %update, %has_update = ac.try_recv @Core::@ready_to_iq_c : i32
      scf.if %has_update {
        %dst_shift = arith.shrui %update, %c10 : i32
        %dst = arith.andi %dst_shift, %c15 : i32
        %bit = arith.shli %c1, %dst : i32
        %bits, %bits_valid = ac.try_recv @ready : i32
        %new_bits = arith.ori %bits, %bit : i32
        %bits_stored = ac.try_send @ready %new_bits : i32
      }
      %slot, %slot_valid = ac.try_recv @slot0 : i32
      %bits_now, %bits_now_valid = ac.try_recv @ready : i32
      %src_shift = arith.shrui %slot, %c14 : i32
      %src = arith.andi %src_shift, %c15 : i32
      %mask = arith.shli %c1, %src : i32
      %hit = arith.andi %bits_now, %mask : i32
      %src_zero = arith.cmpi eq, %src, %c0 : i32
      %src_ready = arith.cmpi ne, %hit, %c0 : i32
      %dep = arith.ori %src_zero, %src_ready : i1
      %occupied = arith.cmpi ne, %slot, %c0 : i32
      %issue = arith.andi %occupied, %dep : i1
      scf.if %issue {
        %sent = ac.try_send @Core::@iq_to_eng_c %slot : i32
        scf.if %sent {
          %clear = ac.try_send @slot0 %c0 : i32
        }
      }
      %idle = arith.cmpi eq, %slot, %c0 : i32
      scf.if %idle {
        %incoming, %has_incoming = ac.try_recv @Core::@dispatch_to_iq_c : i32
        scf.if %has_incoming {
          %store = ac.try_send @slot0 %incoming : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @IssueQueueT() parameters {} graph {
    ac.queue @slot0 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot0" path "slot0" watermarks {kind = "register"}
    ac.queue @slot1 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot1" path "slot1" watermarks {kind = "register"}
    ac.queue @slot2 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot2" path "slot2" watermarks {kind = "register"}
    ac.queue @slot3 payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot3" path "slot3" watermarks {kind = "register"}
    ac.queue @ready payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready" path "ready" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c10 = arith.constant 10 : i32
      %c14 = arith.constant 14 : i32
      %c15 = arith.constant 15 : i32
      %update, %has_update = ac.try_recv @Core::@ready_to_iq_t : i32
      scf.if %has_update {
        %dst_shift = arith.shrui %update, %c10 : i32
        %dst = arith.andi %dst_shift, %c15 : i32
        %bit = arith.shli %c1, %dst : i32
        %bits, %bits_valid = ac.try_recv @ready : i32
        %new_bits = arith.ori %bits, %bit : i32
        %bits_stored = ac.try_send @ready %new_bits : i32
      }
      %slot, %slot_valid = ac.try_recv @slot0 : i32
      %bits_now, %bits_now_valid = ac.try_recv @ready : i32
      %src_shift = arith.shrui %slot, %c14 : i32
      %src = arith.andi %src_shift, %c15 : i32
      %mask = arith.shli %c1, %src : i32
      %hit = arith.andi %bits_now, %mask : i32
      %src_zero = arith.cmpi eq, %src, %c0 : i32
      %src_ready = arith.cmpi ne, %hit, %c0 : i32
      %dep = arith.ori %src_zero, %src_ready : i1
      %occupied = arith.cmpi ne, %slot, %c0 : i32
      %issue = arith.andi %occupied, %dep : i1
      scf.if %issue {
        %sent = ac.try_send @Core::@iq_to_eng_t %slot : i32
        scf.if %sent {
          %clear = ac.try_send @slot0 %c0 : i32
        }
      }
      %idle = arith.cmpi eq, %slot, %c0 : i32
      scf.if %idle {
        %incoming, %has_incoming = ac.try_recv @Core::@dispatch_to_iq_t : i32
        scf.if %has_incoming {
          %store = ac.try_send @slot0 %incoming : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @EngineS() parameters {} graph {
    ac.queue @busy payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}
    ac.queue @remain payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "remain" path "remain" watermarks {kind = "register"}
    ac.queue @current payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "current" path "current" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c15 = arith.constant 15 : i32
      %c22 = arith.constant 22 : i32
      %busy, %busy_valid = ac.try_recv @busy : i32
      %idle = arith.cmpi eq, %busy, %c0 : i32
      scf.if %idle {
        %inst, %valid = ac.try_recv @Core::@iq_to_eng_s : i32
        scf.if %valid {
          %shifted = arith.shrui %inst, %c22 : i32
          %latency = arith.andi %shifted, %c15 : i32
          %set_current = ac.try_send @current %inst : i32
          %set_remain = ac.try_send @remain %latency : i32
          %set_busy = ac.try_send @busy %c1 : i32
        }
      } else {
        %remain, %remain_valid = ac.try_recv @remain : i32
        %done = arith.cmpi ule, %remain, %c1 : i32
        scf.if %done {
          %inst, %inst_valid = ac.try_recv @current : i32
          %wakeup = ac.try_send @Core::@wakeup %inst : i32
          %clear_busy = ac.try_send @busy %c0 : i32
          %clear_remain = ac.try_send @remain %c0 : i32
        } else {
          %next = arith.subi %remain, %c1 : i32
          %store = ac.try_send @remain %next : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @EngineV() parameters {} graph {
    ac.queue @busy payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}
    ac.queue @remain payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "remain" path "remain" watermarks {kind = "register"}
    ac.queue @current payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "current" path "current" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c15 = arith.constant 15 : i32
      %c22 = arith.constant 22 : i32
      %busy, %busy_valid = ac.try_recv @busy : i32
      %idle = arith.cmpi eq, %busy, %c0 : i32
      scf.if %idle {
        %inst, %valid = ac.try_recv @Core::@iq_to_eng_v : i32
        scf.if %valid {
          %shifted = arith.shrui %inst, %c22 : i32
          %latency = arith.andi %shifted, %c15 : i32
          %set_current = ac.try_send @current %inst : i32
          %set_remain = ac.try_send @remain %latency : i32
          %set_busy = ac.try_send @busy %c1 : i32
        }
      } else {
        %remain, %remain_valid = ac.try_recv @remain : i32
        %done = arith.cmpi ule, %remain, %c1 : i32
        scf.if %done {
          %inst, %inst_valid = ac.try_recv @current : i32
          %wakeup = ac.try_send @Core::@wakeup %inst : i32
          %clear_busy = ac.try_send @busy %c0 : i32
          %clear_remain = ac.try_send @remain %c0 : i32
        } else {
          %next = arith.subi %remain, %c1 : i32
          %store = ac.try_send @remain %next : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @EngineC() parameters {} graph {
    ac.queue @busy payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}
    ac.queue @remain payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "remain" path "remain" watermarks {kind = "register"}
    ac.queue @current payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "current" path "current" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c15 = arith.constant 15 : i32
      %c22 = arith.constant 22 : i32
      %busy, %busy_valid = ac.try_recv @busy : i32
      %idle = arith.cmpi eq, %busy, %c0 : i32
      scf.if %idle {
        %inst, %valid = ac.try_recv @Core::@iq_to_eng_c : i32
        scf.if %valid {
          %shifted = arith.shrui %inst, %c22 : i32
          %latency = arith.andi %shifted, %c15 : i32
          %set_current = ac.try_send @current %inst : i32
          %set_remain = ac.try_send @remain %latency : i32
          %set_busy = ac.try_send @busy %c1 : i32
        }
      } else {
        %remain, %remain_valid = ac.try_recv @remain : i32
        %done = arith.cmpi ule, %remain, %c1 : i32
        scf.if %done {
          %inst, %inst_valid = ac.try_recv @current : i32
          %wakeup = ac.try_send @Core::@wakeup %inst : i32
          %clear_busy = ac.try_send @busy %c0 : i32
          %clear_remain = ac.try_send @remain %c0 : i32
        } else {
          %next = arith.subi %remain, %c1 : i32
          %store = ac.try_send @remain %next : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @EngineT() parameters {} graph {
    ac.queue @busy payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}
    ac.queue @remain payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "remain" path "remain" watermarks {kind = "register"}
    ac.queue @current payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "current" path "current" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i32
      %c1 = arith.constant 1 : i32
      %c15 = arith.constant 15 : i32
      %c22 = arith.constant 22 : i32
      %busy, %busy_valid = ac.try_recv @busy : i32
      %idle = arith.cmpi eq, %busy, %c0 : i32
      scf.if %idle {
        %inst, %valid = ac.try_recv @Core::@iq_to_eng_t : i32
        scf.if %valid {
          %shifted = arith.shrui %inst, %c22 : i32
          %latency = arith.andi %shifted, %c15 : i32
          %set_current = ac.try_send @current %inst : i32
          %set_remain = ac.try_send @remain %latency : i32
          %set_busy = ac.try_send @busy %c1 : i32
        }
      } else {
        %remain, %remain_valid = ac.try_recv @remain : i32
        %done = arith.cmpi ule, %remain, %c1 : i32
        scf.if %done {
          %inst, %inst_valid = ac.try_recv @current : i32
          %wakeup = ac.try_send @Core::@wakeup %inst : i32
          %clear_busy = ac.try_send @busy %c0 : i32
          %clear_remain = ac.try_send @remain %c0 : i32
        } else {
          %next = arith.subi %remain, %c1 : i32
          %store = ac.try_send @remain %next : i32
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @Core() parameters {} graph {
    ac.queue @clock payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "clock" path "clock" watermarks {kind = "register"}
    ac.queue @retired payload i32 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "retired" path "retired" watermarks {kind = "register"}
    ac.queue @rob_in payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "rob_in" path "rob_in"
    ac.queue @rob_to_rename payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "rob_to_rename" path "rob_to_rename"
    ac.queue @rename_to_dispatch payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "rename_to_dispatch" path "rename_to_dispatch"
    ac.queue @dispatch_to_iq_s payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "dispatch_to_iq_s" path "dispatch_to_iq_s"
    ac.queue @dispatch_to_iq_v payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "dispatch_to_iq_v" path "dispatch_to_iq_v"
    ac.queue @dispatch_to_iq_c payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "dispatch_to_iq_c" path "dispatch_to_iq_c"
    ac.queue @dispatch_to_iq_t payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "dispatch_to_iq_t" path "dispatch_to_iq_t"
    ac.queue @iq_to_eng_s payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "iq_to_eng_s" path "iq_to_eng_s"
    ac.queue @iq_to_eng_v payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "iq_to_eng_v" path "iq_to_eng_v"
    ac.queue @iq_to_eng_c payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "iq_to_eng_c" path "iq_to_eng_c"
    ac.queue @iq_to_eng_t payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "iq_to_eng_t" path "iq_to_eng_t"
    ac.queue @wakeup payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "wakeup" path "wakeup"
    ac.queue @rob_done payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "rob_done" path "rob_done"
    ac.queue @ready_to_iq_s payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready_to_iq_s" path "ready_to_iq_s"
    ac.queue @ready_to_iq_v payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready_to_iq_v" path "ready_to_iq_v"
    ac.queue @ready_to_iq_c payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready_to_iq_c" path "ready_to_iq_c"
    ac.queue @ready_to_iq_t payload i32 entries 4 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready_to_iq_t" path "ready_to_iq_t"

    ac.instance @trace of @TraceSource() static {} id "trace" path "trace" : () -> ()
    ac.instance @rob of @ROB() static {} id "rob" path "rob" : () -> ()
    ac.instance @rename of @Rename() static {} id "rename" path "rename" : () -> ()
    ac.instance @dispatch of @Dispatch() static {} id "dispatch" path "dispatch" : () -> ()
    ac.instance @ready of @ReadyTable() static {} id "ready" path "ready" : () -> ()
    ac.instance @iq_s of @IssueQueueS() static {} id "iq_s" path "iq_s" : () -> ()
    ac.instance @iq_v of @IssueQueueV() static {} id "iq_v" path "iq_v" : () -> ()
    ac.instance @iq_c of @IssueQueueC() static {} id "iq_c" path "iq_c" : () -> ()
    ac.instance @iq_t of @IssueQueueT() static {} id "iq_t" path "iq_t" : () -> ()
    ac.instance @eng_s of @EngineS() static {} id "eng_s" path "eng_s" : () -> ()
    ac.instance @eng_v of @EngineV() static {} id "eng_v" path "eng_v" : () -> ()
    ac.instance @eng_c of @EngineC() static {} id "eng_c" path "eng_c" : () -> ()
    ac.instance @eng_t of @EngineT() static {} id "eng_t" path "eng_t" : () -> ()

    ac.process @tick kind "workload" {
      %unused, %valid = ac.try_recv @clock : i32
      ac.yield_sim
    }
    ac.return
  }

  ac.system @soc root @Core as "root" tick 0 "cycle"
      workload @Core::@tick seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
