// Trace-driven DavinciOO-shaped out-of-order timing model.
// Generated mechanically from the repeated IQ/engine template in this example.
builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.protocol @rv {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @moved initial false terminal true
    ac.event @offer from @producer to @consumer payload i64 action "offer"
    ac.transition from @idle to @moved on @offer transfer true retain false guard {}
  }

  ac.module @TraceSource() parameters {} graph {
    ac.queue @cursor payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "cursor" path "cursor" watermarks {kind = "register"}
    ac.queue @eof_reported payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "eof_reported" path "eof_reported" watermarks {kind = "register"}
    ac.stat @fetched kind "counter"
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %cursor, %cursor_valid = ac.try_recv @cursor : i64
      %reported, %reported_valid = ac.try_recv @eof_reported : i64
      %cursor_index = arith.index_cast %cursor : i64 to index
      %next_index, %handle, %advanced = ac.trace.next %cursor_index from source "pto" : i64
      scf.if %advanced {
        %sent = ac.try_send @Core::@rob_in %handle : i64
        scf.if %sent {
          ac.stat.add @fetched %c1 : i64
        }
        %next_i64 = arith.index_cast %next_index : index to i64
        %saved_cursor = arith.select %sent, %next_i64, %cursor : i64
        %cursor_stored = ac.try_send @cursor %saved_cursor : i64
      } else {
        %cursor_stored = ac.try_send @cursor %cursor : i64
      }
      %eof = ac.trace.eof %cursor_index from source "pto"
      %not_reported = arith.cmpi eq, %reported, %c0 : i64
      %report_eof = arith.andi %eof, %not_reported : i1
      scf.if %report_eof {
        %position = ac.trace.position %cursor_index from source "pto"
        %total = arith.index_cast %position : index to i64
        %sent_total = ac.try_send @Core::@trace_total %total : i64
        %new_reported = arith.select %sent_total, %c1, %c0 : i64
        %reported_stored = ac.try_send @eof_reported %new_reported : i64
      } else {
        %reported_stored = ac.try_send @eof_reported %reported : i64
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @ROB() parameters {} graph {
    ac.queue @head payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "head" path "head" watermarks {kind = "register"}
    ac.queue @tail payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "tail" path "tail" watermarks {kind = "register"}
    ac.queue @done0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done0" path "done0" watermarks {kind = "register"}
    ac.queue @done1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done1" path "done1" watermarks {kind = "register"}
    ac.queue @done2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done2" path "done2" watermarks {kind = "register"}
    ac.queue @done3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done3" path "done3" watermarks {kind = "register"}
    ac.queue @done4 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done4" path "done4" watermarks {kind = "register"}
    ac.queue @done5 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done5" path "done5" watermarks {kind = "register"}
    ac.queue @done6 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done6" path "done6" watermarks {kind = "register"}
    ac.queue @done7 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "done7" path "done7" watermarks {kind = "register"}
    ac.stat @retired kind "counter"
    ac.stat @opcode_0 kind "counter"
    ac.stat @opcode_1 kind "counter"
    ac.stat @opcode_2 kind "counter"
    ac.stat @opcode_3 kind "counter"
    ac.stat @opcode_4 kind "counter"
    ac.stat @opcode_5 kind "counter"
    ac.stat @engine_s kind "counter"
    ac.stat @engine_v kind "counter"
    ac.stat @engine_c kind "counter"
    ac.stat @engine_t kind "counter"
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %c2 = arith.constant 2 : i64
      %c3 = arith.constant 3 : i64
      %c4 = arith.constant 4 : i64
      %c5 = arith.constant 5 : i64
      %c6 = arith.constant 6 : i64
      %c7 = arith.constant 7 : i64
      %c8 = arith.constant 8 : i64
      %c255 = arith.constant 255 : i64
      %mask7 = arith.constant 7 : i64
      %head, %head_valid = ac.try_recv @head : i64
      %tail, %tail_valid = ac.try_recv @tail : i64
      %d0, %dv0 = ac.try_recv @done0 : i64
      %d1, %dv1 = ac.try_recv @done1 : i64
      %d2, %dv2 = ac.try_recv @done2 : i64
      %d3, %dv3 = ac.try_recv @done3 : i64
      %d4, %dv4 = ac.try_recv @done4 : i64
      %d5, %dv5 = ac.try_recv @done5 : i64
      %d6, %dv6 = ac.try_recv @done6 : i64
      %d7, %dv7 = ac.try_recv @done7 : i64
      %completed, %has_completed = ac.try_recv @Core::@rob_done : i64
      %completed_desc = ac.trace.decode %completed : i64 to i64
      %completed_seq = arith.andi %completed_desc, %c255 : i64
      %completed_slot_base = arith.andi %completed_seq, %mask7 : i64
      %completed_slot = arith.addi %completed_slot_base, %c1 : i64
      %complete0a = arith.cmpi eq, %completed_slot, %c1 : i64
      %complete0 = arith.andi %has_completed, %complete0a : i1
      %sd0 = arith.select %complete0, %c1, %d0 : i64
      %complete1a = arith.cmpi eq, %completed_slot, %c2 : i64
      %complete1 = arith.andi %has_completed, %complete1a : i1
      %sd1 = arith.select %complete1, %c1, %d1 : i64
      %complete2a = arith.cmpi eq, %completed_slot, %c3 : i64
      %complete2 = arith.andi %has_completed, %complete2a : i1
      %sd2 = arith.select %complete2, %c1, %d2 : i64
      %complete3a = arith.cmpi eq, %completed_slot, %c4 : i64
      %complete3 = arith.andi %has_completed, %complete3a : i1
      %sd3 = arith.select %complete3, %c1, %d3 : i64
      %complete4a = arith.cmpi eq, %completed_slot, %c5 : i64
      %complete4 = arith.andi %has_completed, %complete4a : i1
      %sd4 = arith.select %complete4, %c1, %d4 : i64
      %complete5a = arith.cmpi eq, %completed_slot, %c6 : i64
      %complete5 = arith.andi %has_completed, %complete5a : i1
      %sd5 = arith.select %complete5, %c1, %d5 : i64
      %complete6a = arith.cmpi eq, %completed_slot, %c7 : i64
      %complete6 = arith.andi %has_completed, %complete6a : i1
      %sd6 = arith.select %complete6, %c1, %d6 : i64
      %complete7a = arith.cmpi eq, %completed_slot, %c8 : i64
      %complete7 = arith.andi %has_completed, %complete7a : i1
      %sd7 = arith.select %complete7, %c1, %d7 : i64
      %occupancy = arith.subi %tail, %head : i64
      %room = arith.cmpi ult, %occupancy, %c8 : i64
      %incoming, %has_incoming = scf.if %room -> (i64, i1) {
        %value, %valid = ac.try_recv @Core::@rob_in : i64
        scf.yield %value, %valid : i64, i1
      } else {
        %zero_valid = arith.cmpi ne, %c0, %c0 : i64
        scf.yield %c0, %zero_valid : i64, i1
      }
      scf.if %has_incoming {
        %forwarded = ac.try_send @Core::@rob_to_rename %incoming : i64
      }
      %new_tail0 = arith.addi %tail, %c1 : i64
      %new_tail = arith.select %has_incoming, %new_tail0, %tail : i64
      %head_slot_base = arith.andi %head, %mask7 : i64
      %head_slot = arith.addi %head_slot_base, %c1 : i64
      %head_is6 = arith.cmpi eq, %head_slot, %c7 : i64
      %hd6 = arith.select %head_is6, %sd6, %sd7 : i64
      %head_is5 = arith.cmpi eq, %head_slot, %c6 : i64
      %hd5 = arith.select %head_is5, %sd5, %hd6 : i64
      %head_is4 = arith.cmpi eq, %head_slot, %c5 : i64
      %hd4 = arith.select %head_is4, %sd4, %hd5 : i64
      %head_is3 = arith.cmpi eq, %head_slot, %c4 : i64
      %hd3 = arith.select %head_is3, %sd3, %hd4 : i64
      %head_is2 = arith.cmpi eq, %head_slot, %c3 : i64
      %hd2 = arith.select %head_is2, %sd2, %hd3 : i64
      %head_is1 = arith.cmpi eq, %head_slot, %c2 : i64
      %hd1 = arith.select %head_is1, %sd1, %hd2 : i64
      %head_is0 = arith.cmpi eq, %head_slot, %c1 : i64
      %hd0 = arith.select %head_is0, %sd0, %hd1 : i64
      %can_retire = arith.cmpi eq, %hd0, %c1 : i64
      %new_head0 = arith.addi %head, %c1 : i64
      %new_head = arith.select %can_retire, %new_head0, %head : i64
      %retire_slot0a = arith.cmpi eq, %head_slot, %c1 : i64
      %retire_slot0 = arith.andi %can_retire, %retire_slot0a : i1
      %nd0 = arith.select %retire_slot0, %c0, %sd0 : i64
      %retire_slot1a = arith.cmpi eq, %head_slot, %c2 : i64
      %retire_slot1 = arith.andi %can_retire, %retire_slot1a : i1
      %nd1 = arith.select %retire_slot1, %c0, %sd1 : i64
      %retire_slot2a = arith.cmpi eq, %head_slot, %c3 : i64
      %retire_slot2 = arith.andi %can_retire, %retire_slot2a : i1
      %nd2 = arith.select %retire_slot2, %c0, %sd2 : i64
      %retire_slot3a = arith.cmpi eq, %head_slot, %c4 : i64
      %retire_slot3 = arith.andi %can_retire, %retire_slot3a : i1
      %nd3 = arith.select %retire_slot3, %c0, %sd3 : i64
      %retire_slot4a = arith.cmpi eq, %head_slot, %c5 : i64
      %retire_slot4 = arith.andi %can_retire, %retire_slot4a : i1
      %nd4 = arith.select %retire_slot4, %c0, %sd4 : i64
      %retire_slot5a = arith.cmpi eq, %head_slot, %c6 : i64
      %retire_slot5 = arith.andi %can_retire, %retire_slot5a : i1
      %nd5 = arith.select %retire_slot5, %c0, %sd5 : i64
      %retire_slot6a = arith.cmpi eq, %head_slot, %c7 : i64
      %retire_slot6 = arith.andi %can_retire, %retire_slot6a : i1
      %nd6 = arith.select %retire_slot6, %c0, %sd6 : i64
      %retire_slot7a = arith.cmpi eq, %head_slot, %c8 : i64
      %retire_slot7 = arith.andi %can_retire, %retire_slot7a : i1
      %nd7 = arith.select %retire_slot7, %c0, %sd7 : i64
      scf.if %can_retire {
        %retired_one = arith.constant 1 : i64
        ac.stat.add @retired %retired_one : i64
        %retired_desc = ac.trace.decode %head : i64 to i64
        %opcode_shifted = arith.shrui %retired_desc, %c8 : i64
        %opcode = arith.andi %opcode_shifted, %mask7 : i64
        %is_opcode0 = arith.cmpi eq, %opcode, %c0 : i64
        scf.if %is_opcode0 {
          ac.stat.add @opcode_0 %retired_one : i64
        }
        %is_opcode1 = arith.cmpi eq, %opcode, %c1 : i64
        scf.if %is_opcode1 {
          ac.stat.add @opcode_1 %retired_one : i64
        }
        %is_opcode2 = arith.cmpi eq, %opcode, %c2 : i64
        scf.if %is_opcode2 {
          ac.stat.add @opcode_2 %retired_one : i64
        }
        %is_opcode3 = arith.cmpi eq, %opcode, %c3 : i64
        scf.if %is_opcode3 {
          ac.stat.add @opcode_3 %retired_one : i64
        }
        %is_opcode4 = arith.cmpi eq, %opcode, %c4 : i64
        scf.if %is_opcode4 {
          ac.stat.add @opcode_4 %retired_one : i64
        }
        %is_opcode5 = arith.cmpi eq, %opcode, %c5 : i64
        scf.if %is_opcode5 {
          ac.stat.add @opcode_5 %retired_one : i64
        }
        // Engine ownership is an architecture decision derived from opcode.
        %is_engine_s = arith.cmpi eq, %opcode, %c0 : i64
        scf.if %is_engine_s {
          ac.stat.add @engine_s %retired_one : i64
        }
        %is_engine_v = arith.cmpi eq, %opcode, %c2 : i64
        scf.if %is_engine_v {
          ac.stat.add @engine_v %retired_one : i64
        }
        %is_engine_c0 = arith.cmpi eq, %opcode, %c3 : i64
        %is_engine_c1 = arith.cmpi eq, %opcode, %c4 : i64
        %is_engine_c = arith.ori %is_engine_c0, %is_engine_c1 : i1
        scf.if %is_engine_c {
          ac.stat.add @engine_c %retired_one : i64
        }
        %is_engine_t0 = arith.cmpi eq, %opcode, %c1 : i64
        %is_engine_t1 = arith.cmpi eq, %opcode, %c5 : i64
        %is_engine_t = arith.ori %is_engine_t0, %is_engine_t1 : i1
        scf.if %is_engine_t {
          ac.stat.add @engine_t %retired_one : i64
        }
      }
      %total, %has_total = ac.try_recv @Core::@trace_total : i64
      %all_retired = arith.cmpi eq, %new_head, %total : i64
      %nonzero_total = arith.cmpi ne, %total, %c0 : i64
      %eof_done0 = arith.andi %has_total, %nonzero_total : i1
      %eof_done = arith.andi %eof_done0, %all_retired : i1
      scf.if %eof_done {
        ac.assert %eof_done, "retired"
      }
      %head_stored = ac.try_send @head %new_head : i64
      %tail_stored = ac.try_send @tail %new_tail : i64
      %done0_stored = ac.try_send @done0 %nd0 : i64
      %done1_stored = ac.try_send @done1 %nd1 : i64
      %done2_stored = ac.try_send @done2 %nd2 : i64
      %done3_stored = ac.try_send @done3 %nd3 : i64
      %done4_stored = ac.try_send @done4 %nd4 : i64
      %done5_stored = ac.try_send @done5 %nd5 : i64
      %done6_stored = ac.try_send @done6 %nd6 : i64
      %done7_stored = ac.try_send @done7 %nd7 : i64
      ac.yield_sim
    }
    ac.return
  }

  ac.module @Rename() parameters {} graph {
    ac.process @step kind "control" {
      %handle, %valid = ac.try_recv @Core::@rob_to_rename : i64
      scf.if %valid {
        %sent = ac.try_send @Core::@rename_to_dispatch %handle : i64
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @Dispatch() parameters {} graph {
    ac.stat @dispatched kind "counter"
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %c2 = arith.constant 2 : i64
      %c3 = arith.constant 3 : i64
      %c4 = arith.constant 4 : i64
      %c5 = arith.constant 5 : i64
      %c8 = arith.constant 8 : i64
      %mask7 = arith.constant 7 : i64
      %handle, %valid = ac.try_recv @Core::@rename_to_dispatch : i64
      scf.if %valid {
        ac.stat.add @dispatched %c1 : i64
        %desc = ac.trace.decode %handle : i64 to i64
        %shifted = arith.shrui %desc, %c8 : i64
        %opcode = arith.andi %shifted, %mask7 : i64
        // Opcode-to-engine routing belongs to this architecture model.
        %is_s = arith.cmpi eq, %opcode, %c0 : i64
        %is_v = arith.cmpi eq, %opcode, %c2 : i64
        %is_c0 = arith.cmpi eq, %opcode, %c3 : i64
        %is_c1 = arith.cmpi eq, %opcode, %c4 : i64
        %is_c = arith.ori %is_c0, %is_c1 : i1
        %is_t0 = arith.cmpi eq, %opcode, %c1 : i64
        %is_t1 = arith.cmpi eq, %opcode, %c5 : i64
        %is_t = arith.ori %is_t0, %is_t1 : i1
        scf.if %is_s { %sent_s = ac.try_send @Core::@dispatch_to_iq_s %handle : i64 }
        scf.if %is_v { %sent_v = ac.try_send @Core::@dispatch_to_iq_v %handle : i64 }
        scf.if %is_c { %sent_c = ac.try_send @Core::@dispatch_to_iq_c %handle : i64 }
        scf.if %is_t { %sent_t = ac.try_send @Core::@dispatch_to_iq_t %handle : i64 }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @CompletionBroadcast() parameters {} graph {
    ac.stat @completed kind "counter"
    ac.process @step kind "control" {
      %one = arith.constant 1 : i64
      %handle, %valid = ac.try_recv @Core::@wakeup : i64
      scf.if %valid {
        ac.stat.add @completed %one : i64
        %rob = ac.try_send @Core::@rob_done %handle : i64
        %s = ac.try_send @Core::@ready_to_iq_s %handle : i64
        %v = ac.try_send @Core::@ready_to_iq_v %handle : i64
        %c = ac.try_send @Core::@ready_to_iq_c %handle : i64
        %t = ac.try_send @Core::@ready_to_iq_t %handle : i64
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @IssueQueueS() parameters {} graph {
    ac.queue @slot0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot0" path "slot0" watermarks {kind = "register"}
    ac.queue @slot1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot1" path "slot1" watermarks {kind = "register"}
    ac.queue @slot2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot2" path "slot2" watermarks {kind = "register"}
    ac.queue @slot3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot3" path "slot3" watermarks {kind = "register"}
    ac.queue @slot4 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot4" path "slot4" watermarks {kind = "register"}
    ac.queue @slot5 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot5" path "slot5" watermarks {kind = "register"}
    ac.queue @slot6 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot6" path "slot6" watermarks {kind = "register"}
    ac.queue @slot7 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot7" path "slot7" watermarks {kind = "register"}
    ac.queue @valid payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "valid" path "valid" watermarks {kind = "register"}
    ac.queue @ready0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready0" path "ready0" watermarks {kind = "register"}
    ac.queue @ready1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready1" path "ready1" watermarks {kind = "register"}
    ac.queue @ready2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready2" path "ready2" watermarks {kind = "register"}
    ac.queue @ready3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready3" path "ready3" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %c2 = arith.constant 2 : i64
      %c3 = arith.constant 3 : i64
      %c4 = arith.constant 4 : i64
      %c5 = arith.constant 5 : i64
      %c6 = arith.constant 6 : i64
      %c7 = arith.constant 7 : i64
      %c8 = arith.constant 8 : i64
      %c35 = arith.constant 35 : i64
      %c63 = arith.constant 63 : i64
      %c255 = arith.constant 255 : i64
      %mask7 = arith.constant 7 : i64
      %mask63 = arith.constant 63 : i64
      %mask255 = arith.constant 255 : i64
      %false = arith.constant false
      %valid_bits, %valid_ok = ac.try_recv @valid : i64
      %s0, %s0_ok = ac.try_recv @slot0 : i64
      %s1, %s1_ok = ac.try_recv @slot1 : i64
      %s2, %s2_ok = ac.try_recv @slot2 : i64
      %s3, %s3_ok = ac.try_recv @slot3 : i64
      %s4, %s4_ok = ac.try_recv @slot4 : i64
      %s5, %s5_ok = ac.try_recv @slot5 : i64
      %s6, %s6_ok = ac.try_recv @slot6 : i64
      %s7, %s7_ok = ac.try_recv @slot7 : i64
      %r0, %r0_ok = ac.try_recv @ready0 : i64
      %r1, %r1_ok = ac.try_recv @ready1 : i64
      %r2, %r2_ok = ac.try_recv @ready2 : i64
      %r3, %r3_ok = ac.try_recv @ready3 : i64
      %update, %has_update = ac.try_recv @Core::@ready_to_iq_s : i64
      %update_desc = ac.trace.decode %update : i64 to i64
      %update_seq = arith.andi %update_desc, %mask255 : i64
      %update_word = arith.shrui %update_seq, %c6 : i64
      %update_off = arith.andi %update_seq, %mask63 : i64
      %update_bit = arith.shli %c1, %update_off : i64
      %update_is0a = arith.cmpi eq, %update_word, %c0 : i64
      %update_is0 = arith.andi %has_update, %update_is0a : i1
      %r0_set = arith.ori %r0, %update_bit : i64
      %nr0 = arith.select %update_is0, %r0_set, %r0 : i64
      %update_is1a = arith.cmpi eq, %update_word, %c1 : i64
      %update_is1 = arith.andi %has_update, %update_is1a : i1
      %r1_set = arith.ori %r1, %update_bit : i64
      %nr1 = arith.select %update_is1, %r1_set, %r1 : i64
      %update_is2a = arith.cmpi eq, %update_word, %c2 : i64
      %update_is2 = arith.andi %has_update, %update_is2a : i1
      %r2_set = arith.ori %r2, %update_bit : i64
      %nr2 = arith.select %update_is2, %r2_set, %r2 : i64
      %update_is3a = arith.cmpi eq, %update_word, %c3 : i64
      %update_is3 = arith.andi %has_update, %update_is3a : i1
      %r3_set = arith.ori %r3, %update_bit : i64
      %nr3 = arith.select %update_is3, %r3_set, %r3 : i64
      %q0_desc = ac.trace.decode %s0 : i64 to i64
      %q0_dv_s = arith.shrui %q0_desc, %c35 : i64
      %q0_dvalid = arith.andi %q0_dv_s, %mask7 : i64
      %q0_dep0_shift = arith.constant 11 : i64
      %q0_dep0_s = arith.shrui %q0_desc, %q0_dep0_shift : i64
      %q0_dep0 = arith.andi %q0_dep0_s, %mask255 : i64
      %q0_dep0_word = arith.shrui %q0_dep0, %c6 : i64
      %q0_dep0_off = arith.andi %q0_dep0, %mask63 : i64
      %q0_dep0_bit = arith.shli %c1, %q0_dep0_off : i64
      %q0_dep0_w0 = arith.cmpi eq, %q0_dep0_word, %c0 : i64
      %q0_dep0_w1 = arith.cmpi eq, %q0_dep0_word, %c1 : i64
      %q0_dep0_w2 = arith.cmpi eq, %q0_dep0_word, %c2 : i64
      %q0_dep0_sel1 = arith.select %q0_dep0_w1, %nr1, %nr3 : i64
      %q0_dep0_sel0 = arith.select %q0_dep0_w0, %nr0, %q0_dep0_sel1 : i64
      %q0_dep0_bits = arith.select %q0_dep0_w2, %nr2, %q0_dep0_sel0 : i64
      %q0_dep0_hit0 = arith.andi %q0_dep0_bits, %q0_dep0_bit : i64
      %q0_dep0_hit = arith.cmpi ne, %q0_dep0_hit0, %c0 : i64
      %q0_dep0_vbit = arith.constant 1 : i64
      %q0_dep0_vhit = arith.andi %q0_dvalid, %q0_dep0_vbit : i64
      %q0_dep0_used = arith.cmpi ne, %q0_dep0_vhit, %c0 : i64
      %q0_dep0_unused = arith.cmpi eq, %q0_dep0_used, %false : i1
      %q0_dep0_ready = arith.ori %q0_dep0_unused, %q0_dep0_hit : i1
      %q0_dep1_shift = arith.constant 19 : i64
      %q0_dep1_s = arith.shrui %q0_desc, %q0_dep1_shift : i64
      %q0_dep1 = arith.andi %q0_dep1_s, %mask255 : i64
      %q0_dep1_word = arith.shrui %q0_dep1, %c6 : i64
      %q0_dep1_off = arith.andi %q0_dep1, %mask63 : i64
      %q0_dep1_bit = arith.shli %c1, %q0_dep1_off : i64
      %q0_dep1_w0 = arith.cmpi eq, %q0_dep1_word, %c0 : i64
      %q0_dep1_w1 = arith.cmpi eq, %q0_dep1_word, %c1 : i64
      %q0_dep1_w2 = arith.cmpi eq, %q0_dep1_word, %c2 : i64
      %q0_dep1_sel1 = arith.select %q0_dep1_w1, %nr1, %nr3 : i64
      %q0_dep1_sel0 = arith.select %q0_dep1_w0, %nr0, %q0_dep1_sel1 : i64
      %q0_dep1_bits = arith.select %q0_dep1_w2, %nr2, %q0_dep1_sel0 : i64
      %q0_dep1_hit0 = arith.andi %q0_dep1_bits, %q0_dep1_bit : i64
      %q0_dep1_hit = arith.cmpi ne, %q0_dep1_hit0, %c0 : i64
      %q0_dep1_vbit = arith.constant 2 : i64
      %q0_dep1_vhit = arith.andi %q0_dvalid, %q0_dep1_vbit : i64
      %q0_dep1_used = arith.cmpi ne, %q0_dep1_vhit, %c0 : i64
      %q0_dep1_unused = arith.cmpi eq, %q0_dep1_used, %false : i1
      %q0_dep1_ready = arith.ori %q0_dep1_unused, %q0_dep1_hit : i1
      %q0_dep2_shift = arith.constant 27 : i64
      %q0_dep2_s = arith.shrui %q0_desc, %q0_dep2_shift : i64
      %q0_dep2 = arith.andi %q0_dep2_s, %mask255 : i64
      %q0_dep2_word = arith.shrui %q0_dep2, %c6 : i64
      %q0_dep2_off = arith.andi %q0_dep2, %mask63 : i64
      %q0_dep2_bit = arith.shli %c1, %q0_dep2_off : i64
      %q0_dep2_w0 = arith.cmpi eq, %q0_dep2_word, %c0 : i64
      %q0_dep2_w1 = arith.cmpi eq, %q0_dep2_word, %c1 : i64
      %q0_dep2_w2 = arith.cmpi eq, %q0_dep2_word, %c2 : i64
      %q0_dep2_sel1 = arith.select %q0_dep2_w1, %nr1, %nr3 : i64
      %q0_dep2_sel0 = arith.select %q0_dep2_w0, %nr0, %q0_dep2_sel1 : i64
      %q0_dep2_bits = arith.select %q0_dep2_w2, %nr2, %q0_dep2_sel0 : i64
      %q0_dep2_hit0 = arith.andi %q0_dep2_bits, %q0_dep2_bit : i64
      %q0_dep2_hit = arith.cmpi ne, %q0_dep2_hit0, %c0 : i64
      %q0_dep2_vbit = arith.constant 4 : i64
      %q0_dep2_vhit = arith.andi %q0_dvalid, %q0_dep2_vbit : i64
      %q0_dep2_used = arith.cmpi ne, %q0_dep2_vhit, %c0 : i64
      %q0_dep2_unused = arith.cmpi eq, %q0_dep2_used, %false : i1
      %q0_dep2_ready = arith.ori %q0_dep2_unused, %q0_dep2_hit : i1
      %q0_deps01 = arith.andi %q0_dep0_ready, %q0_dep1_ready : i1
      %q0_deps = arith.andi %q0_deps01, %q0_dep2_ready : i1
      %q0_vbit = arith.constant 1 : i64
      %q0_vhit = arith.andi %valid_bits, %q0_vbit : i64
      %q0_occupied = arith.cmpi ne, %q0_vhit, %c0 : i64
      %q0_eligible = arith.andi %q0_occupied, %q0_deps : i1
      %q1_desc = ac.trace.decode %s1 : i64 to i64
      %q1_dv_s = arith.shrui %q1_desc, %c35 : i64
      %q1_dvalid = arith.andi %q1_dv_s, %mask7 : i64
      %q1_dep0_shift = arith.constant 11 : i64
      %q1_dep0_s = arith.shrui %q1_desc, %q1_dep0_shift : i64
      %q1_dep0 = arith.andi %q1_dep0_s, %mask255 : i64
      %q1_dep0_word = arith.shrui %q1_dep0, %c6 : i64
      %q1_dep0_off = arith.andi %q1_dep0, %mask63 : i64
      %q1_dep0_bit = arith.shli %c1, %q1_dep0_off : i64
      %q1_dep0_w0 = arith.cmpi eq, %q1_dep0_word, %c0 : i64
      %q1_dep0_w1 = arith.cmpi eq, %q1_dep0_word, %c1 : i64
      %q1_dep0_w2 = arith.cmpi eq, %q1_dep0_word, %c2 : i64
      %q1_dep0_sel1 = arith.select %q1_dep0_w1, %nr1, %nr3 : i64
      %q1_dep0_sel0 = arith.select %q1_dep0_w0, %nr0, %q1_dep0_sel1 : i64
      %q1_dep0_bits = arith.select %q1_dep0_w2, %nr2, %q1_dep0_sel0 : i64
      %q1_dep0_hit0 = arith.andi %q1_dep0_bits, %q1_dep0_bit : i64
      %q1_dep0_hit = arith.cmpi ne, %q1_dep0_hit0, %c0 : i64
      %q1_dep0_vbit = arith.constant 1 : i64
      %q1_dep0_vhit = arith.andi %q1_dvalid, %q1_dep0_vbit : i64
      %q1_dep0_used = arith.cmpi ne, %q1_dep0_vhit, %c0 : i64
      %q1_dep0_unused = arith.cmpi eq, %q1_dep0_used, %false : i1
      %q1_dep0_ready = arith.ori %q1_dep0_unused, %q1_dep0_hit : i1
      %q1_dep1_shift = arith.constant 19 : i64
      %q1_dep1_s = arith.shrui %q1_desc, %q1_dep1_shift : i64
      %q1_dep1 = arith.andi %q1_dep1_s, %mask255 : i64
      %q1_dep1_word = arith.shrui %q1_dep1, %c6 : i64
      %q1_dep1_off = arith.andi %q1_dep1, %mask63 : i64
      %q1_dep1_bit = arith.shli %c1, %q1_dep1_off : i64
      %q1_dep1_w0 = arith.cmpi eq, %q1_dep1_word, %c0 : i64
      %q1_dep1_w1 = arith.cmpi eq, %q1_dep1_word, %c1 : i64
      %q1_dep1_w2 = arith.cmpi eq, %q1_dep1_word, %c2 : i64
      %q1_dep1_sel1 = arith.select %q1_dep1_w1, %nr1, %nr3 : i64
      %q1_dep1_sel0 = arith.select %q1_dep1_w0, %nr0, %q1_dep1_sel1 : i64
      %q1_dep1_bits = arith.select %q1_dep1_w2, %nr2, %q1_dep1_sel0 : i64
      %q1_dep1_hit0 = arith.andi %q1_dep1_bits, %q1_dep1_bit : i64
      %q1_dep1_hit = arith.cmpi ne, %q1_dep1_hit0, %c0 : i64
      %q1_dep1_vbit = arith.constant 2 : i64
      %q1_dep1_vhit = arith.andi %q1_dvalid, %q1_dep1_vbit : i64
      %q1_dep1_used = arith.cmpi ne, %q1_dep1_vhit, %c0 : i64
      %q1_dep1_unused = arith.cmpi eq, %q1_dep1_used, %false : i1
      %q1_dep1_ready = arith.ori %q1_dep1_unused, %q1_dep1_hit : i1
      %q1_dep2_shift = arith.constant 27 : i64
      %q1_dep2_s = arith.shrui %q1_desc, %q1_dep2_shift : i64
      %q1_dep2 = arith.andi %q1_dep2_s, %mask255 : i64
      %q1_dep2_word = arith.shrui %q1_dep2, %c6 : i64
      %q1_dep2_off = arith.andi %q1_dep2, %mask63 : i64
      %q1_dep2_bit = arith.shli %c1, %q1_dep2_off : i64
      %q1_dep2_w0 = arith.cmpi eq, %q1_dep2_word, %c0 : i64
      %q1_dep2_w1 = arith.cmpi eq, %q1_dep2_word, %c1 : i64
      %q1_dep2_w2 = arith.cmpi eq, %q1_dep2_word, %c2 : i64
      %q1_dep2_sel1 = arith.select %q1_dep2_w1, %nr1, %nr3 : i64
      %q1_dep2_sel0 = arith.select %q1_dep2_w0, %nr0, %q1_dep2_sel1 : i64
      %q1_dep2_bits = arith.select %q1_dep2_w2, %nr2, %q1_dep2_sel0 : i64
      %q1_dep2_hit0 = arith.andi %q1_dep2_bits, %q1_dep2_bit : i64
      %q1_dep2_hit = arith.cmpi ne, %q1_dep2_hit0, %c0 : i64
      %q1_dep2_vbit = arith.constant 4 : i64
      %q1_dep2_vhit = arith.andi %q1_dvalid, %q1_dep2_vbit : i64
      %q1_dep2_used = arith.cmpi ne, %q1_dep2_vhit, %c0 : i64
      %q1_dep2_unused = arith.cmpi eq, %q1_dep2_used, %false : i1
      %q1_dep2_ready = arith.ori %q1_dep2_unused, %q1_dep2_hit : i1
      %q1_deps01 = arith.andi %q1_dep0_ready, %q1_dep1_ready : i1
      %q1_deps = arith.andi %q1_deps01, %q1_dep2_ready : i1
      %q1_vbit = arith.constant 2 : i64
      %q1_vhit = arith.andi %valid_bits, %q1_vbit : i64
      %q1_occupied = arith.cmpi ne, %q1_vhit, %c0 : i64
      %q1_eligible = arith.andi %q1_occupied, %q1_deps : i1
      %q2_desc = ac.trace.decode %s2 : i64 to i64
      %q2_dv_s = arith.shrui %q2_desc, %c35 : i64
      %q2_dvalid = arith.andi %q2_dv_s, %mask7 : i64
      %q2_dep0_shift = arith.constant 11 : i64
      %q2_dep0_s = arith.shrui %q2_desc, %q2_dep0_shift : i64
      %q2_dep0 = arith.andi %q2_dep0_s, %mask255 : i64
      %q2_dep0_word = arith.shrui %q2_dep0, %c6 : i64
      %q2_dep0_off = arith.andi %q2_dep0, %mask63 : i64
      %q2_dep0_bit = arith.shli %c1, %q2_dep0_off : i64
      %q2_dep0_w0 = arith.cmpi eq, %q2_dep0_word, %c0 : i64
      %q2_dep0_w1 = arith.cmpi eq, %q2_dep0_word, %c1 : i64
      %q2_dep0_w2 = arith.cmpi eq, %q2_dep0_word, %c2 : i64
      %q2_dep0_sel1 = arith.select %q2_dep0_w1, %nr1, %nr3 : i64
      %q2_dep0_sel0 = arith.select %q2_dep0_w0, %nr0, %q2_dep0_sel1 : i64
      %q2_dep0_bits = arith.select %q2_dep0_w2, %nr2, %q2_dep0_sel0 : i64
      %q2_dep0_hit0 = arith.andi %q2_dep0_bits, %q2_dep0_bit : i64
      %q2_dep0_hit = arith.cmpi ne, %q2_dep0_hit0, %c0 : i64
      %q2_dep0_vbit = arith.constant 1 : i64
      %q2_dep0_vhit = arith.andi %q2_dvalid, %q2_dep0_vbit : i64
      %q2_dep0_used = arith.cmpi ne, %q2_dep0_vhit, %c0 : i64
      %q2_dep0_unused = arith.cmpi eq, %q2_dep0_used, %false : i1
      %q2_dep0_ready = arith.ori %q2_dep0_unused, %q2_dep0_hit : i1
      %q2_dep1_shift = arith.constant 19 : i64
      %q2_dep1_s = arith.shrui %q2_desc, %q2_dep1_shift : i64
      %q2_dep1 = arith.andi %q2_dep1_s, %mask255 : i64
      %q2_dep1_word = arith.shrui %q2_dep1, %c6 : i64
      %q2_dep1_off = arith.andi %q2_dep1, %mask63 : i64
      %q2_dep1_bit = arith.shli %c1, %q2_dep1_off : i64
      %q2_dep1_w0 = arith.cmpi eq, %q2_dep1_word, %c0 : i64
      %q2_dep1_w1 = arith.cmpi eq, %q2_dep1_word, %c1 : i64
      %q2_dep1_w2 = arith.cmpi eq, %q2_dep1_word, %c2 : i64
      %q2_dep1_sel1 = arith.select %q2_dep1_w1, %nr1, %nr3 : i64
      %q2_dep1_sel0 = arith.select %q2_dep1_w0, %nr0, %q2_dep1_sel1 : i64
      %q2_dep1_bits = arith.select %q2_dep1_w2, %nr2, %q2_dep1_sel0 : i64
      %q2_dep1_hit0 = arith.andi %q2_dep1_bits, %q2_dep1_bit : i64
      %q2_dep1_hit = arith.cmpi ne, %q2_dep1_hit0, %c0 : i64
      %q2_dep1_vbit = arith.constant 2 : i64
      %q2_dep1_vhit = arith.andi %q2_dvalid, %q2_dep1_vbit : i64
      %q2_dep1_used = arith.cmpi ne, %q2_dep1_vhit, %c0 : i64
      %q2_dep1_unused = arith.cmpi eq, %q2_dep1_used, %false : i1
      %q2_dep1_ready = arith.ori %q2_dep1_unused, %q2_dep1_hit : i1
      %q2_dep2_shift = arith.constant 27 : i64
      %q2_dep2_s = arith.shrui %q2_desc, %q2_dep2_shift : i64
      %q2_dep2 = arith.andi %q2_dep2_s, %mask255 : i64
      %q2_dep2_word = arith.shrui %q2_dep2, %c6 : i64
      %q2_dep2_off = arith.andi %q2_dep2, %mask63 : i64
      %q2_dep2_bit = arith.shli %c1, %q2_dep2_off : i64
      %q2_dep2_w0 = arith.cmpi eq, %q2_dep2_word, %c0 : i64
      %q2_dep2_w1 = arith.cmpi eq, %q2_dep2_word, %c1 : i64
      %q2_dep2_w2 = arith.cmpi eq, %q2_dep2_word, %c2 : i64
      %q2_dep2_sel1 = arith.select %q2_dep2_w1, %nr1, %nr3 : i64
      %q2_dep2_sel0 = arith.select %q2_dep2_w0, %nr0, %q2_dep2_sel1 : i64
      %q2_dep2_bits = arith.select %q2_dep2_w2, %nr2, %q2_dep2_sel0 : i64
      %q2_dep2_hit0 = arith.andi %q2_dep2_bits, %q2_dep2_bit : i64
      %q2_dep2_hit = arith.cmpi ne, %q2_dep2_hit0, %c0 : i64
      %q2_dep2_vbit = arith.constant 4 : i64
      %q2_dep2_vhit = arith.andi %q2_dvalid, %q2_dep2_vbit : i64
      %q2_dep2_used = arith.cmpi ne, %q2_dep2_vhit, %c0 : i64
      %q2_dep2_unused = arith.cmpi eq, %q2_dep2_used, %false : i1
      %q2_dep2_ready = arith.ori %q2_dep2_unused, %q2_dep2_hit : i1
      %q2_deps01 = arith.andi %q2_dep0_ready, %q2_dep1_ready : i1
      %q2_deps = arith.andi %q2_deps01, %q2_dep2_ready : i1
      %q2_vbit = arith.constant 4 : i64
      %q2_vhit = arith.andi %valid_bits, %q2_vbit : i64
      %q2_occupied = arith.cmpi ne, %q2_vhit, %c0 : i64
      %q2_eligible = arith.andi %q2_occupied, %q2_deps : i1
      %q3_desc = ac.trace.decode %s3 : i64 to i64
      %q3_dv_s = arith.shrui %q3_desc, %c35 : i64
      %q3_dvalid = arith.andi %q3_dv_s, %mask7 : i64
      %q3_dep0_shift = arith.constant 11 : i64
      %q3_dep0_s = arith.shrui %q3_desc, %q3_dep0_shift : i64
      %q3_dep0 = arith.andi %q3_dep0_s, %mask255 : i64
      %q3_dep0_word = arith.shrui %q3_dep0, %c6 : i64
      %q3_dep0_off = arith.andi %q3_dep0, %mask63 : i64
      %q3_dep0_bit = arith.shli %c1, %q3_dep0_off : i64
      %q3_dep0_w0 = arith.cmpi eq, %q3_dep0_word, %c0 : i64
      %q3_dep0_w1 = arith.cmpi eq, %q3_dep0_word, %c1 : i64
      %q3_dep0_w2 = arith.cmpi eq, %q3_dep0_word, %c2 : i64
      %q3_dep0_sel1 = arith.select %q3_dep0_w1, %nr1, %nr3 : i64
      %q3_dep0_sel0 = arith.select %q3_dep0_w0, %nr0, %q3_dep0_sel1 : i64
      %q3_dep0_bits = arith.select %q3_dep0_w2, %nr2, %q3_dep0_sel0 : i64
      %q3_dep0_hit0 = arith.andi %q3_dep0_bits, %q3_dep0_bit : i64
      %q3_dep0_hit = arith.cmpi ne, %q3_dep0_hit0, %c0 : i64
      %q3_dep0_vbit = arith.constant 1 : i64
      %q3_dep0_vhit = arith.andi %q3_dvalid, %q3_dep0_vbit : i64
      %q3_dep0_used = arith.cmpi ne, %q3_dep0_vhit, %c0 : i64
      %q3_dep0_unused = arith.cmpi eq, %q3_dep0_used, %false : i1
      %q3_dep0_ready = arith.ori %q3_dep0_unused, %q3_dep0_hit : i1
      %q3_dep1_shift = arith.constant 19 : i64
      %q3_dep1_s = arith.shrui %q3_desc, %q3_dep1_shift : i64
      %q3_dep1 = arith.andi %q3_dep1_s, %mask255 : i64
      %q3_dep1_word = arith.shrui %q3_dep1, %c6 : i64
      %q3_dep1_off = arith.andi %q3_dep1, %mask63 : i64
      %q3_dep1_bit = arith.shli %c1, %q3_dep1_off : i64
      %q3_dep1_w0 = arith.cmpi eq, %q3_dep1_word, %c0 : i64
      %q3_dep1_w1 = arith.cmpi eq, %q3_dep1_word, %c1 : i64
      %q3_dep1_w2 = arith.cmpi eq, %q3_dep1_word, %c2 : i64
      %q3_dep1_sel1 = arith.select %q3_dep1_w1, %nr1, %nr3 : i64
      %q3_dep1_sel0 = arith.select %q3_dep1_w0, %nr0, %q3_dep1_sel1 : i64
      %q3_dep1_bits = arith.select %q3_dep1_w2, %nr2, %q3_dep1_sel0 : i64
      %q3_dep1_hit0 = arith.andi %q3_dep1_bits, %q3_dep1_bit : i64
      %q3_dep1_hit = arith.cmpi ne, %q3_dep1_hit0, %c0 : i64
      %q3_dep1_vbit = arith.constant 2 : i64
      %q3_dep1_vhit = arith.andi %q3_dvalid, %q3_dep1_vbit : i64
      %q3_dep1_used = arith.cmpi ne, %q3_dep1_vhit, %c0 : i64
      %q3_dep1_unused = arith.cmpi eq, %q3_dep1_used, %false : i1
      %q3_dep1_ready = arith.ori %q3_dep1_unused, %q3_dep1_hit : i1
      %q3_dep2_shift = arith.constant 27 : i64
      %q3_dep2_s = arith.shrui %q3_desc, %q3_dep2_shift : i64
      %q3_dep2 = arith.andi %q3_dep2_s, %mask255 : i64
      %q3_dep2_word = arith.shrui %q3_dep2, %c6 : i64
      %q3_dep2_off = arith.andi %q3_dep2, %mask63 : i64
      %q3_dep2_bit = arith.shli %c1, %q3_dep2_off : i64
      %q3_dep2_w0 = arith.cmpi eq, %q3_dep2_word, %c0 : i64
      %q3_dep2_w1 = arith.cmpi eq, %q3_dep2_word, %c1 : i64
      %q3_dep2_w2 = arith.cmpi eq, %q3_dep2_word, %c2 : i64
      %q3_dep2_sel1 = arith.select %q3_dep2_w1, %nr1, %nr3 : i64
      %q3_dep2_sel0 = arith.select %q3_dep2_w0, %nr0, %q3_dep2_sel1 : i64
      %q3_dep2_bits = arith.select %q3_dep2_w2, %nr2, %q3_dep2_sel0 : i64
      %q3_dep2_hit0 = arith.andi %q3_dep2_bits, %q3_dep2_bit : i64
      %q3_dep2_hit = arith.cmpi ne, %q3_dep2_hit0, %c0 : i64
      %q3_dep2_vbit = arith.constant 4 : i64
      %q3_dep2_vhit = arith.andi %q3_dvalid, %q3_dep2_vbit : i64
      %q3_dep2_used = arith.cmpi ne, %q3_dep2_vhit, %c0 : i64
      %q3_dep2_unused = arith.cmpi eq, %q3_dep2_used, %false : i1
      %q3_dep2_ready = arith.ori %q3_dep2_unused, %q3_dep2_hit : i1
      %q3_deps01 = arith.andi %q3_dep0_ready, %q3_dep1_ready : i1
      %q3_deps = arith.andi %q3_deps01, %q3_dep2_ready : i1
      %q3_vbit = arith.constant 8 : i64
      %q3_vhit = arith.andi %valid_bits, %q3_vbit : i64
      %q3_occupied = arith.cmpi ne, %q3_vhit, %c0 : i64
      %q3_eligible = arith.andi %q3_occupied, %q3_deps : i1
      %q4_desc = ac.trace.decode %s4 : i64 to i64
      %q4_dv_s = arith.shrui %q4_desc, %c35 : i64
      %q4_dvalid = arith.andi %q4_dv_s, %mask7 : i64
      %q4_dep0_shift = arith.constant 11 : i64
      %q4_dep0_s = arith.shrui %q4_desc, %q4_dep0_shift : i64
      %q4_dep0 = arith.andi %q4_dep0_s, %mask255 : i64
      %q4_dep0_word = arith.shrui %q4_dep0, %c6 : i64
      %q4_dep0_off = arith.andi %q4_dep0, %mask63 : i64
      %q4_dep0_bit = arith.shli %c1, %q4_dep0_off : i64
      %q4_dep0_w0 = arith.cmpi eq, %q4_dep0_word, %c0 : i64
      %q4_dep0_w1 = arith.cmpi eq, %q4_dep0_word, %c1 : i64
      %q4_dep0_w2 = arith.cmpi eq, %q4_dep0_word, %c2 : i64
      %q4_dep0_sel1 = arith.select %q4_dep0_w1, %nr1, %nr3 : i64
      %q4_dep0_sel0 = arith.select %q4_dep0_w0, %nr0, %q4_dep0_sel1 : i64
      %q4_dep0_bits = arith.select %q4_dep0_w2, %nr2, %q4_dep0_sel0 : i64
      %q4_dep0_hit0 = arith.andi %q4_dep0_bits, %q4_dep0_bit : i64
      %q4_dep0_hit = arith.cmpi ne, %q4_dep0_hit0, %c0 : i64
      %q4_dep0_vbit = arith.constant 1 : i64
      %q4_dep0_vhit = arith.andi %q4_dvalid, %q4_dep0_vbit : i64
      %q4_dep0_used = arith.cmpi ne, %q4_dep0_vhit, %c0 : i64
      %q4_dep0_unused = arith.cmpi eq, %q4_dep0_used, %false : i1
      %q4_dep0_ready = arith.ori %q4_dep0_unused, %q4_dep0_hit : i1
      %q4_dep1_shift = arith.constant 19 : i64
      %q4_dep1_s = arith.shrui %q4_desc, %q4_dep1_shift : i64
      %q4_dep1 = arith.andi %q4_dep1_s, %mask255 : i64
      %q4_dep1_word = arith.shrui %q4_dep1, %c6 : i64
      %q4_dep1_off = arith.andi %q4_dep1, %mask63 : i64
      %q4_dep1_bit = arith.shli %c1, %q4_dep1_off : i64
      %q4_dep1_w0 = arith.cmpi eq, %q4_dep1_word, %c0 : i64
      %q4_dep1_w1 = arith.cmpi eq, %q4_dep1_word, %c1 : i64
      %q4_dep1_w2 = arith.cmpi eq, %q4_dep1_word, %c2 : i64
      %q4_dep1_sel1 = arith.select %q4_dep1_w1, %nr1, %nr3 : i64
      %q4_dep1_sel0 = arith.select %q4_dep1_w0, %nr0, %q4_dep1_sel1 : i64
      %q4_dep1_bits = arith.select %q4_dep1_w2, %nr2, %q4_dep1_sel0 : i64
      %q4_dep1_hit0 = arith.andi %q4_dep1_bits, %q4_dep1_bit : i64
      %q4_dep1_hit = arith.cmpi ne, %q4_dep1_hit0, %c0 : i64
      %q4_dep1_vbit = arith.constant 2 : i64
      %q4_dep1_vhit = arith.andi %q4_dvalid, %q4_dep1_vbit : i64
      %q4_dep1_used = arith.cmpi ne, %q4_dep1_vhit, %c0 : i64
      %q4_dep1_unused = arith.cmpi eq, %q4_dep1_used, %false : i1
      %q4_dep1_ready = arith.ori %q4_dep1_unused, %q4_dep1_hit : i1
      %q4_dep2_shift = arith.constant 27 : i64
      %q4_dep2_s = arith.shrui %q4_desc, %q4_dep2_shift : i64
      %q4_dep2 = arith.andi %q4_dep2_s, %mask255 : i64
      %q4_dep2_word = arith.shrui %q4_dep2, %c6 : i64
      %q4_dep2_off = arith.andi %q4_dep2, %mask63 : i64
      %q4_dep2_bit = arith.shli %c1, %q4_dep2_off : i64
      %q4_dep2_w0 = arith.cmpi eq, %q4_dep2_word, %c0 : i64
      %q4_dep2_w1 = arith.cmpi eq, %q4_dep2_word, %c1 : i64
      %q4_dep2_w2 = arith.cmpi eq, %q4_dep2_word, %c2 : i64
      %q4_dep2_sel1 = arith.select %q4_dep2_w1, %nr1, %nr3 : i64
      %q4_dep2_sel0 = arith.select %q4_dep2_w0, %nr0, %q4_dep2_sel1 : i64
      %q4_dep2_bits = arith.select %q4_dep2_w2, %nr2, %q4_dep2_sel0 : i64
      %q4_dep2_hit0 = arith.andi %q4_dep2_bits, %q4_dep2_bit : i64
      %q4_dep2_hit = arith.cmpi ne, %q4_dep2_hit0, %c0 : i64
      %q4_dep2_vbit = arith.constant 4 : i64
      %q4_dep2_vhit = arith.andi %q4_dvalid, %q4_dep2_vbit : i64
      %q4_dep2_used = arith.cmpi ne, %q4_dep2_vhit, %c0 : i64
      %q4_dep2_unused = arith.cmpi eq, %q4_dep2_used, %false : i1
      %q4_dep2_ready = arith.ori %q4_dep2_unused, %q4_dep2_hit : i1
      %q4_deps01 = arith.andi %q4_dep0_ready, %q4_dep1_ready : i1
      %q4_deps = arith.andi %q4_deps01, %q4_dep2_ready : i1
      %q4_vbit = arith.constant 16 : i64
      %q4_vhit = arith.andi %valid_bits, %q4_vbit : i64
      %q4_occupied = arith.cmpi ne, %q4_vhit, %c0 : i64
      %q4_eligible = arith.andi %q4_occupied, %q4_deps : i1
      %q5_desc = ac.trace.decode %s5 : i64 to i64
      %q5_dv_s = arith.shrui %q5_desc, %c35 : i64
      %q5_dvalid = arith.andi %q5_dv_s, %mask7 : i64
      %q5_dep0_shift = arith.constant 11 : i64
      %q5_dep0_s = arith.shrui %q5_desc, %q5_dep0_shift : i64
      %q5_dep0 = arith.andi %q5_dep0_s, %mask255 : i64
      %q5_dep0_word = arith.shrui %q5_dep0, %c6 : i64
      %q5_dep0_off = arith.andi %q5_dep0, %mask63 : i64
      %q5_dep0_bit = arith.shli %c1, %q5_dep0_off : i64
      %q5_dep0_w0 = arith.cmpi eq, %q5_dep0_word, %c0 : i64
      %q5_dep0_w1 = arith.cmpi eq, %q5_dep0_word, %c1 : i64
      %q5_dep0_w2 = arith.cmpi eq, %q5_dep0_word, %c2 : i64
      %q5_dep0_sel1 = arith.select %q5_dep0_w1, %nr1, %nr3 : i64
      %q5_dep0_sel0 = arith.select %q5_dep0_w0, %nr0, %q5_dep0_sel1 : i64
      %q5_dep0_bits = arith.select %q5_dep0_w2, %nr2, %q5_dep0_sel0 : i64
      %q5_dep0_hit0 = arith.andi %q5_dep0_bits, %q5_dep0_bit : i64
      %q5_dep0_hit = arith.cmpi ne, %q5_dep0_hit0, %c0 : i64
      %q5_dep0_vbit = arith.constant 1 : i64
      %q5_dep0_vhit = arith.andi %q5_dvalid, %q5_dep0_vbit : i64
      %q5_dep0_used = arith.cmpi ne, %q5_dep0_vhit, %c0 : i64
      %q5_dep0_unused = arith.cmpi eq, %q5_dep0_used, %false : i1
      %q5_dep0_ready = arith.ori %q5_dep0_unused, %q5_dep0_hit : i1
      %q5_dep1_shift = arith.constant 19 : i64
      %q5_dep1_s = arith.shrui %q5_desc, %q5_dep1_shift : i64
      %q5_dep1 = arith.andi %q5_dep1_s, %mask255 : i64
      %q5_dep1_word = arith.shrui %q5_dep1, %c6 : i64
      %q5_dep1_off = arith.andi %q5_dep1, %mask63 : i64
      %q5_dep1_bit = arith.shli %c1, %q5_dep1_off : i64
      %q5_dep1_w0 = arith.cmpi eq, %q5_dep1_word, %c0 : i64
      %q5_dep1_w1 = arith.cmpi eq, %q5_dep1_word, %c1 : i64
      %q5_dep1_w2 = arith.cmpi eq, %q5_dep1_word, %c2 : i64
      %q5_dep1_sel1 = arith.select %q5_dep1_w1, %nr1, %nr3 : i64
      %q5_dep1_sel0 = arith.select %q5_dep1_w0, %nr0, %q5_dep1_sel1 : i64
      %q5_dep1_bits = arith.select %q5_dep1_w2, %nr2, %q5_dep1_sel0 : i64
      %q5_dep1_hit0 = arith.andi %q5_dep1_bits, %q5_dep1_bit : i64
      %q5_dep1_hit = arith.cmpi ne, %q5_dep1_hit0, %c0 : i64
      %q5_dep1_vbit = arith.constant 2 : i64
      %q5_dep1_vhit = arith.andi %q5_dvalid, %q5_dep1_vbit : i64
      %q5_dep1_used = arith.cmpi ne, %q5_dep1_vhit, %c0 : i64
      %q5_dep1_unused = arith.cmpi eq, %q5_dep1_used, %false : i1
      %q5_dep1_ready = arith.ori %q5_dep1_unused, %q5_dep1_hit : i1
      %q5_dep2_shift = arith.constant 27 : i64
      %q5_dep2_s = arith.shrui %q5_desc, %q5_dep2_shift : i64
      %q5_dep2 = arith.andi %q5_dep2_s, %mask255 : i64
      %q5_dep2_word = arith.shrui %q5_dep2, %c6 : i64
      %q5_dep2_off = arith.andi %q5_dep2, %mask63 : i64
      %q5_dep2_bit = arith.shli %c1, %q5_dep2_off : i64
      %q5_dep2_w0 = arith.cmpi eq, %q5_dep2_word, %c0 : i64
      %q5_dep2_w1 = arith.cmpi eq, %q5_dep2_word, %c1 : i64
      %q5_dep2_w2 = arith.cmpi eq, %q5_dep2_word, %c2 : i64
      %q5_dep2_sel1 = arith.select %q5_dep2_w1, %nr1, %nr3 : i64
      %q5_dep2_sel0 = arith.select %q5_dep2_w0, %nr0, %q5_dep2_sel1 : i64
      %q5_dep2_bits = arith.select %q5_dep2_w2, %nr2, %q5_dep2_sel0 : i64
      %q5_dep2_hit0 = arith.andi %q5_dep2_bits, %q5_dep2_bit : i64
      %q5_dep2_hit = arith.cmpi ne, %q5_dep2_hit0, %c0 : i64
      %q5_dep2_vbit = arith.constant 4 : i64
      %q5_dep2_vhit = arith.andi %q5_dvalid, %q5_dep2_vbit : i64
      %q5_dep2_used = arith.cmpi ne, %q5_dep2_vhit, %c0 : i64
      %q5_dep2_unused = arith.cmpi eq, %q5_dep2_used, %false : i1
      %q5_dep2_ready = arith.ori %q5_dep2_unused, %q5_dep2_hit : i1
      %q5_deps01 = arith.andi %q5_dep0_ready, %q5_dep1_ready : i1
      %q5_deps = arith.andi %q5_deps01, %q5_dep2_ready : i1
      %q5_vbit = arith.constant 32 : i64
      %q5_vhit = arith.andi %valid_bits, %q5_vbit : i64
      %q5_occupied = arith.cmpi ne, %q5_vhit, %c0 : i64
      %q5_eligible = arith.andi %q5_occupied, %q5_deps : i1
      %q6_desc = ac.trace.decode %s6 : i64 to i64
      %q6_dv_s = arith.shrui %q6_desc, %c35 : i64
      %q6_dvalid = arith.andi %q6_dv_s, %mask7 : i64
      %q6_dep0_shift = arith.constant 11 : i64
      %q6_dep0_s = arith.shrui %q6_desc, %q6_dep0_shift : i64
      %q6_dep0 = arith.andi %q6_dep0_s, %mask255 : i64
      %q6_dep0_word = arith.shrui %q6_dep0, %c6 : i64
      %q6_dep0_off = arith.andi %q6_dep0, %mask63 : i64
      %q6_dep0_bit = arith.shli %c1, %q6_dep0_off : i64
      %q6_dep0_w0 = arith.cmpi eq, %q6_dep0_word, %c0 : i64
      %q6_dep0_w1 = arith.cmpi eq, %q6_dep0_word, %c1 : i64
      %q6_dep0_w2 = arith.cmpi eq, %q6_dep0_word, %c2 : i64
      %q6_dep0_sel1 = arith.select %q6_dep0_w1, %nr1, %nr3 : i64
      %q6_dep0_sel0 = arith.select %q6_dep0_w0, %nr0, %q6_dep0_sel1 : i64
      %q6_dep0_bits = arith.select %q6_dep0_w2, %nr2, %q6_dep0_sel0 : i64
      %q6_dep0_hit0 = arith.andi %q6_dep0_bits, %q6_dep0_bit : i64
      %q6_dep0_hit = arith.cmpi ne, %q6_dep0_hit0, %c0 : i64
      %q6_dep0_vbit = arith.constant 1 : i64
      %q6_dep0_vhit = arith.andi %q6_dvalid, %q6_dep0_vbit : i64
      %q6_dep0_used = arith.cmpi ne, %q6_dep0_vhit, %c0 : i64
      %q6_dep0_unused = arith.cmpi eq, %q6_dep0_used, %false : i1
      %q6_dep0_ready = arith.ori %q6_dep0_unused, %q6_dep0_hit : i1
      %q6_dep1_shift = arith.constant 19 : i64
      %q6_dep1_s = arith.shrui %q6_desc, %q6_dep1_shift : i64
      %q6_dep1 = arith.andi %q6_dep1_s, %mask255 : i64
      %q6_dep1_word = arith.shrui %q6_dep1, %c6 : i64
      %q6_dep1_off = arith.andi %q6_dep1, %mask63 : i64
      %q6_dep1_bit = arith.shli %c1, %q6_dep1_off : i64
      %q6_dep1_w0 = arith.cmpi eq, %q6_dep1_word, %c0 : i64
      %q6_dep1_w1 = arith.cmpi eq, %q6_dep1_word, %c1 : i64
      %q6_dep1_w2 = arith.cmpi eq, %q6_dep1_word, %c2 : i64
      %q6_dep1_sel1 = arith.select %q6_dep1_w1, %nr1, %nr3 : i64
      %q6_dep1_sel0 = arith.select %q6_dep1_w0, %nr0, %q6_dep1_sel1 : i64
      %q6_dep1_bits = arith.select %q6_dep1_w2, %nr2, %q6_dep1_sel0 : i64
      %q6_dep1_hit0 = arith.andi %q6_dep1_bits, %q6_dep1_bit : i64
      %q6_dep1_hit = arith.cmpi ne, %q6_dep1_hit0, %c0 : i64
      %q6_dep1_vbit = arith.constant 2 : i64
      %q6_dep1_vhit = arith.andi %q6_dvalid, %q6_dep1_vbit : i64
      %q6_dep1_used = arith.cmpi ne, %q6_dep1_vhit, %c0 : i64
      %q6_dep1_unused = arith.cmpi eq, %q6_dep1_used, %false : i1
      %q6_dep1_ready = arith.ori %q6_dep1_unused, %q6_dep1_hit : i1
      %q6_dep2_shift = arith.constant 27 : i64
      %q6_dep2_s = arith.shrui %q6_desc, %q6_dep2_shift : i64
      %q6_dep2 = arith.andi %q6_dep2_s, %mask255 : i64
      %q6_dep2_word = arith.shrui %q6_dep2, %c6 : i64
      %q6_dep2_off = arith.andi %q6_dep2, %mask63 : i64
      %q6_dep2_bit = arith.shli %c1, %q6_dep2_off : i64
      %q6_dep2_w0 = arith.cmpi eq, %q6_dep2_word, %c0 : i64
      %q6_dep2_w1 = arith.cmpi eq, %q6_dep2_word, %c1 : i64
      %q6_dep2_w2 = arith.cmpi eq, %q6_dep2_word, %c2 : i64
      %q6_dep2_sel1 = arith.select %q6_dep2_w1, %nr1, %nr3 : i64
      %q6_dep2_sel0 = arith.select %q6_dep2_w0, %nr0, %q6_dep2_sel1 : i64
      %q6_dep2_bits = arith.select %q6_dep2_w2, %nr2, %q6_dep2_sel0 : i64
      %q6_dep2_hit0 = arith.andi %q6_dep2_bits, %q6_dep2_bit : i64
      %q6_dep2_hit = arith.cmpi ne, %q6_dep2_hit0, %c0 : i64
      %q6_dep2_vbit = arith.constant 4 : i64
      %q6_dep2_vhit = arith.andi %q6_dvalid, %q6_dep2_vbit : i64
      %q6_dep2_used = arith.cmpi ne, %q6_dep2_vhit, %c0 : i64
      %q6_dep2_unused = arith.cmpi eq, %q6_dep2_used, %false : i1
      %q6_dep2_ready = arith.ori %q6_dep2_unused, %q6_dep2_hit : i1
      %q6_deps01 = arith.andi %q6_dep0_ready, %q6_dep1_ready : i1
      %q6_deps = arith.andi %q6_deps01, %q6_dep2_ready : i1
      %q6_vbit = arith.constant 64 : i64
      %q6_vhit = arith.andi %valid_bits, %q6_vbit : i64
      %q6_occupied = arith.cmpi ne, %q6_vhit, %c0 : i64
      %q6_eligible = arith.andi %q6_occupied, %q6_deps : i1
      %q7_desc = ac.trace.decode %s7 : i64 to i64
      %q7_dv_s = arith.shrui %q7_desc, %c35 : i64
      %q7_dvalid = arith.andi %q7_dv_s, %mask7 : i64
      %q7_dep0_shift = arith.constant 11 : i64
      %q7_dep0_s = arith.shrui %q7_desc, %q7_dep0_shift : i64
      %q7_dep0 = arith.andi %q7_dep0_s, %mask255 : i64
      %q7_dep0_word = arith.shrui %q7_dep0, %c6 : i64
      %q7_dep0_off = arith.andi %q7_dep0, %mask63 : i64
      %q7_dep0_bit = arith.shli %c1, %q7_dep0_off : i64
      %q7_dep0_w0 = arith.cmpi eq, %q7_dep0_word, %c0 : i64
      %q7_dep0_w1 = arith.cmpi eq, %q7_dep0_word, %c1 : i64
      %q7_dep0_w2 = arith.cmpi eq, %q7_dep0_word, %c2 : i64
      %q7_dep0_sel1 = arith.select %q7_dep0_w1, %nr1, %nr3 : i64
      %q7_dep0_sel0 = arith.select %q7_dep0_w0, %nr0, %q7_dep0_sel1 : i64
      %q7_dep0_bits = arith.select %q7_dep0_w2, %nr2, %q7_dep0_sel0 : i64
      %q7_dep0_hit0 = arith.andi %q7_dep0_bits, %q7_dep0_bit : i64
      %q7_dep0_hit = arith.cmpi ne, %q7_dep0_hit0, %c0 : i64
      %q7_dep0_vbit = arith.constant 1 : i64
      %q7_dep0_vhit = arith.andi %q7_dvalid, %q7_dep0_vbit : i64
      %q7_dep0_used = arith.cmpi ne, %q7_dep0_vhit, %c0 : i64
      %q7_dep0_unused = arith.cmpi eq, %q7_dep0_used, %false : i1
      %q7_dep0_ready = arith.ori %q7_dep0_unused, %q7_dep0_hit : i1
      %q7_dep1_shift = arith.constant 19 : i64
      %q7_dep1_s = arith.shrui %q7_desc, %q7_dep1_shift : i64
      %q7_dep1 = arith.andi %q7_dep1_s, %mask255 : i64
      %q7_dep1_word = arith.shrui %q7_dep1, %c6 : i64
      %q7_dep1_off = arith.andi %q7_dep1, %mask63 : i64
      %q7_dep1_bit = arith.shli %c1, %q7_dep1_off : i64
      %q7_dep1_w0 = arith.cmpi eq, %q7_dep1_word, %c0 : i64
      %q7_dep1_w1 = arith.cmpi eq, %q7_dep1_word, %c1 : i64
      %q7_dep1_w2 = arith.cmpi eq, %q7_dep1_word, %c2 : i64
      %q7_dep1_sel1 = arith.select %q7_dep1_w1, %nr1, %nr3 : i64
      %q7_dep1_sel0 = arith.select %q7_dep1_w0, %nr0, %q7_dep1_sel1 : i64
      %q7_dep1_bits = arith.select %q7_dep1_w2, %nr2, %q7_dep1_sel0 : i64
      %q7_dep1_hit0 = arith.andi %q7_dep1_bits, %q7_dep1_bit : i64
      %q7_dep1_hit = arith.cmpi ne, %q7_dep1_hit0, %c0 : i64
      %q7_dep1_vbit = arith.constant 2 : i64
      %q7_dep1_vhit = arith.andi %q7_dvalid, %q7_dep1_vbit : i64
      %q7_dep1_used = arith.cmpi ne, %q7_dep1_vhit, %c0 : i64
      %q7_dep1_unused = arith.cmpi eq, %q7_dep1_used, %false : i1
      %q7_dep1_ready = arith.ori %q7_dep1_unused, %q7_dep1_hit : i1
      %q7_dep2_shift = arith.constant 27 : i64
      %q7_dep2_s = arith.shrui %q7_desc, %q7_dep2_shift : i64
      %q7_dep2 = arith.andi %q7_dep2_s, %mask255 : i64
      %q7_dep2_word = arith.shrui %q7_dep2, %c6 : i64
      %q7_dep2_off = arith.andi %q7_dep2, %mask63 : i64
      %q7_dep2_bit = arith.shli %c1, %q7_dep2_off : i64
      %q7_dep2_w0 = arith.cmpi eq, %q7_dep2_word, %c0 : i64
      %q7_dep2_w1 = arith.cmpi eq, %q7_dep2_word, %c1 : i64
      %q7_dep2_w2 = arith.cmpi eq, %q7_dep2_word, %c2 : i64
      %q7_dep2_sel1 = arith.select %q7_dep2_w1, %nr1, %nr3 : i64
      %q7_dep2_sel0 = arith.select %q7_dep2_w0, %nr0, %q7_dep2_sel1 : i64
      %q7_dep2_bits = arith.select %q7_dep2_w2, %nr2, %q7_dep2_sel0 : i64
      %q7_dep2_hit0 = arith.andi %q7_dep2_bits, %q7_dep2_bit : i64
      %q7_dep2_hit = arith.cmpi ne, %q7_dep2_hit0, %c0 : i64
      %q7_dep2_vbit = arith.constant 4 : i64
      %q7_dep2_vhit = arith.andi %q7_dvalid, %q7_dep2_vbit : i64
      %q7_dep2_used = arith.cmpi ne, %q7_dep2_vhit, %c0 : i64
      %q7_dep2_unused = arith.cmpi eq, %q7_dep2_used, %false : i1
      %q7_dep2_ready = arith.ori %q7_dep2_unused, %q7_dep2_hit : i1
      %q7_deps01 = arith.andi %q7_dep0_ready, %q7_dep1_ready : i1
      %q7_deps = arith.andi %q7_deps01, %q7_dep2_ready : i1
      %q7_vbit = arith.constant 128 : i64
      %q7_vhit = arith.andi %valid_bits, %q7_vbit : i64
      %q7_occupied = arith.cmpi ne, %q7_vhit, %c0 : i64
      %q7_eligible = arith.andi %q7_occupied, %q7_deps : i1
      %oldest_init = arith.constant 256 : i64
      %index_init = arith.constant 8 : i64
      %q0_older = arith.cmpi ult, %s0, %oldest_init : i64
      %q0_choose = arith.andi %q0_eligible, %q0_older : i1
      %oldest0 = arith.select %q0_choose, %s0, %oldest_init : i64
      %index0 = arith.select %q0_choose, %c0, %index_init : i64
      %q1_older = arith.cmpi ult, %s1, %oldest0 : i64
      %q1_choose = arith.andi %q1_eligible, %q1_older : i1
      %oldest1 = arith.select %q1_choose, %s1, %oldest0 : i64
      %index1 = arith.select %q1_choose, %c1, %index0 : i64
      %q2_older = arith.cmpi ult, %s2, %oldest1 : i64
      %q2_choose = arith.andi %q2_eligible, %q2_older : i1
      %oldest2 = arith.select %q2_choose, %s2, %oldest1 : i64
      %index2 = arith.select %q2_choose, %c2, %index1 : i64
      %q3_older = arith.cmpi ult, %s3, %oldest2 : i64
      %q3_choose = arith.andi %q3_eligible, %q3_older : i1
      %oldest3 = arith.select %q3_choose, %s3, %oldest2 : i64
      %index3 = arith.select %q3_choose, %c3, %index2 : i64
      %q4_older = arith.cmpi ult, %s4, %oldest3 : i64
      %q4_choose = arith.andi %q4_eligible, %q4_older : i1
      %oldest4 = arith.select %q4_choose, %s4, %oldest3 : i64
      %index4 = arith.select %q4_choose, %c4, %index3 : i64
      %q5_older = arith.cmpi ult, %s5, %oldest4 : i64
      %q5_choose = arith.andi %q5_eligible, %q5_older : i1
      %oldest5 = arith.select %q5_choose, %s5, %oldest4 : i64
      %index5 = arith.select %q5_choose, %c5, %index4 : i64
      %q6_older = arith.cmpi ult, %s6, %oldest5 : i64
      %q6_choose = arith.andi %q6_eligible, %q6_older : i1
      %oldest6 = arith.select %q6_choose, %s6, %oldest5 : i64
      %index6 = arith.select %q6_choose, %c6, %index5 : i64
      %q7_older = arith.cmpi ult, %s7, %oldest6 : i64
      %q7_choose = arith.andi %q7_eligible, %q7_older : i1
      %oldest7 = arith.select %q7_choose, %s7, %oldest6 : i64
      %index7 = arith.select %q7_choose, %c7, %index6 : i64
      %has_issue = arith.cmpi ne, %index7, %c8 : i64
      %issued = scf.if %has_issue -> i1 {
        %sent = ac.try_send @Core::@iq_to_eng_s %oldest7 : i64
        scf.yield %sent : i1
      } else {
        scf.yield %false : i1
      }
      %clear_mask0 = arith.constant -2 : i64
      %cleared0 = arith.andi %valid_bits, %clear_mask0 : i64
      %issued_slot0a = arith.cmpi eq, %index7, %c0 : i64
      %issued_slot0 = arith.andi %issued, %issued_slot0a : i1
      %va0 = arith.select %issued_slot0, %cleared0, %valid_bits : i64
      %clear_mask1 = arith.constant -3 : i64
      %cleared1 = arith.andi %va0, %clear_mask1 : i64
      %issued_slot1a = arith.cmpi eq, %index7, %c1 : i64
      %issued_slot1 = arith.andi %issued, %issued_slot1a : i1
      %va1 = arith.select %issued_slot1, %cleared1, %va0 : i64
      %clear_mask2 = arith.constant -5 : i64
      %cleared2 = arith.andi %va1, %clear_mask2 : i64
      %issued_slot2a = arith.cmpi eq, %index7, %c2 : i64
      %issued_slot2 = arith.andi %issued, %issued_slot2a : i1
      %va2 = arith.select %issued_slot2, %cleared2, %va1 : i64
      %clear_mask3 = arith.constant -9 : i64
      %cleared3 = arith.andi %va2, %clear_mask3 : i64
      %issued_slot3a = arith.cmpi eq, %index7, %c3 : i64
      %issued_slot3 = arith.andi %issued, %issued_slot3a : i1
      %va3 = arith.select %issued_slot3, %cleared3, %va2 : i64
      %clear_mask4 = arith.constant -17 : i64
      %cleared4 = arith.andi %va3, %clear_mask4 : i64
      %issued_slot4a = arith.cmpi eq, %index7, %c4 : i64
      %issued_slot4 = arith.andi %issued, %issued_slot4a : i1
      %va4 = arith.select %issued_slot4, %cleared4, %va3 : i64
      %clear_mask5 = arith.constant -33 : i64
      %cleared5 = arith.andi %va4, %clear_mask5 : i64
      %issued_slot5a = arith.cmpi eq, %index7, %c5 : i64
      %issued_slot5 = arith.andi %issued, %issued_slot5a : i1
      %va5 = arith.select %issued_slot5, %cleared5, %va4 : i64
      %clear_mask6 = arith.constant -65 : i64
      %cleared6 = arith.andi %va5, %clear_mask6 : i64
      %issued_slot6a = arith.cmpi eq, %index7, %c6 : i64
      %issued_slot6 = arith.andi %issued, %issued_slot6a : i1
      %va6 = arith.select %issued_slot6, %cleared6, %va5 : i64
      %clear_mask7 = arith.constant -129 : i64
      %cleared7 = arith.andi %va6, %clear_mask7 : i64
      %issued_slot7a = arith.cmpi eq, %index7, %c7 : i64
      %issued_slot7 = arith.andi %issued, %issued_slot7a : i1
      %va7 = arith.select %issued_slot7, %cleared7, %va6 : i64
      %full = arith.cmpi eq, %va7, %c255 : i64
      %not_full = arith.cmpi eq, %full, %false : i1
      %incoming, %has_incoming = scf.if %not_full -> (i64, i1) {
        %value, %ok = ac.try_recv @Core::@dispatch_to_iq_s : i64
        scf.yield %value, %ok : i64, i1
      } else {
        scf.yield %c0, %false : i64, i1
      }
      %ins_bit7 = arith.constant 128 : i64
      %ins_hit7 = arith.andi %va7, %ins_bit7 : i64
      %empty7 = arith.cmpi eq, %ins_hit7, %c0 : i64
      %ins7 = arith.select %empty7, %c7, %c8 : i64
      %ins_bit6 = arith.constant 64 : i64
      %ins_hit6 = arith.andi %va7, %ins_bit6 : i64
      %empty6 = arith.cmpi eq, %ins_hit6, %c0 : i64
      %ins6 = arith.select %empty6, %c6, %ins7 : i64
      %ins_bit5 = arith.constant 32 : i64
      %ins_hit5 = arith.andi %va7, %ins_bit5 : i64
      %empty5 = arith.cmpi eq, %ins_hit5, %c0 : i64
      %ins5 = arith.select %empty5, %c5, %ins6 : i64
      %ins_bit4 = arith.constant 16 : i64
      %ins_hit4 = arith.andi %va7, %ins_bit4 : i64
      %empty4 = arith.cmpi eq, %ins_hit4, %c0 : i64
      %ins4 = arith.select %empty4, %c4, %ins5 : i64
      %ins_bit3 = arith.constant 8 : i64
      %ins_hit3 = arith.andi %va7, %ins_bit3 : i64
      %empty3 = arith.cmpi eq, %ins_hit3, %c0 : i64
      %ins3 = arith.select %empty3, %c3, %ins4 : i64
      %ins_bit2 = arith.constant 4 : i64
      %ins_hit2 = arith.andi %va7, %ins_bit2 : i64
      %empty2 = arith.cmpi eq, %ins_hit2, %c0 : i64
      %ins2 = arith.select %empty2, %c2, %ins3 : i64
      %ins_bit1 = arith.constant 2 : i64
      %ins_hit1 = arith.andi %va7, %ins_bit1 : i64
      %empty1 = arith.cmpi eq, %ins_hit1, %c0 : i64
      %ins1 = arith.select %empty1, %c1, %ins2 : i64
      %ins_bit0 = arith.constant 1 : i64
      %ins_hit0 = arith.andi %va7, %ins_bit0 : i64
      %empty0 = arith.cmpi eq, %ins_hit0, %c0 : i64
      %ins0 = arith.select %empty0, %c0, %ins1 : i64
      %put0a = arith.cmpi eq, %ins0, %c0 : i64
      %put0 = arith.andi %has_incoming, %put0a : i1
      %ns0 = arith.select %put0, %incoming, %s0 : i64
      %put1a = arith.cmpi eq, %ins0, %c1 : i64
      %put1 = arith.andi %has_incoming, %put1a : i1
      %ns1 = arith.select %put1, %incoming, %s1 : i64
      %put2a = arith.cmpi eq, %ins0, %c2 : i64
      %put2 = arith.andi %has_incoming, %put2a : i1
      %ns2 = arith.select %put2, %incoming, %s2 : i64
      %put3a = arith.cmpi eq, %ins0, %c3 : i64
      %put3 = arith.andi %has_incoming, %put3a : i1
      %ns3 = arith.select %put3, %incoming, %s3 : i64
      %put4a = arith.cmpi eq, %ins0, %c4 : i64
      %put4 = arith.andi %has_incoming, %put4a : i1
      %ns4 = arith.select %put4, %incoming, %s4 : i64
      %put5a = arith.cmpi eq, %ins0, %c5 : i64
      %put5 = arith.andi %has_incoming, %put5a : i1
      %ns5 = arith.select %put5, %incoming, %s5 : i64
      %put6a = arith.cmpi eq, %ins0, %c6 : i64
      %put6 = arith.andi %has_incoming, %put6a : i1
      %ns6 = arith.select %put6, %incoming, %s6 : i64
      %put7a = arith.cmpi eq, %ins0, %c7 : i64
      %put7 = arith.andi %has_incoming, %put7a : i1
      %ns7 = arith.select %put7, %incoming, %s7 : i64
      %with0 = arith.ori %va7, %ins_bit0 : i64
      %nv0 = arith.select %put0, %with0, %va7 : i64
      %with1 = arith.ori %nv0, %ins_bit1 : i64
      %nv1 = arith.select %put1, %with1, %nv0 : i64
      %with2 = arith.ori %nv1, %ins_bit2 : i64
      %nv2 = arith.select %put2, %with2, %nv1 : i64
      %with3 = arith.ori %nv2, %ins_bit3 : i64
      %nv3 = arith.select %put3, %with3, %nv2 : i64
      %with4 = arith.ori %nv3, %ins_bit4 : i64
      %nv4 = arith.select %put4, %with4, %nv3 : i64
      %with5 = arith.ori %nv4, %ins_bit5 : i64
      %nv5 = arith.select %put5, %with5, %nv4 : i64
      %with6 = arith.ori %nv5, %ins_bit6 : i64
      %nv6 = arith.select %put6, %with6, %nv5 : i64
      %with7 = arith.ori %nv6, %ins_bit7 : i64
      %nv7 = arith.select %put7, %with7, %nv6 : i64
      %valid_stored = ac.try_send @valid %nv7 : i64
      %slot0_stored = ac.try_send @slot0 %ns0 : i64
      %slot1_stored = ac.try_send @slot1 %ns1 : i64
      %slot2_stored = ac.try_send @slot2 %ns2 : i64
      %slot3_stored = ac.try_send @slot3 %ns3 : i64
      %slot4_stored = ac.try_send @slot4 %ns4 : i64
      %slot5_stored = ac.try_send @slot5 %ns5 : i64
      %slot6_stored = ac.try_send @slot6 %ns6 : i64
      %slot7_stored = ac.try_send @slot7 %ns7 : i64
      %ready0_stored = ac.try_send @ready0 %nr0 : i64
      %ready1_stored = ac.try_send @ready1 %nr1 : i64
      %ready2_stored = ac.try_send @ready2 %nr2 : i64
      %ready3_stored = ac.try_send @ready3 %nr3 : i64
      ac.yield_sim
    }
    ac.return
  }

  ac.module @IssueQueueV() parameters {} graph {
    ac.queue @slot0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot0" path "slot0" watermarks {kind = "register"}
    ac.queue @slot1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot1" path "slot1" watermarks {kind = "register"}
    ac.queue @slot2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot2" path "slot2" watermarks {kind = "register"}
    ac.queue @slot3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot3" path "slot3" watermarks {kind = "register"}
    ac.queue @slot4 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot4" path "slot4" watermarks {kind = "register"}
    ac.queue @slot5 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot5" path "slot5" watermarks {kind = "register"}
    ac.queue @slot6 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot6" path "slot6" watermarks {kind = "register"}
    ac.queue @slot7 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot7" path "slot7" watermarks {kind = "register"}
    ac.queue @valid payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "valid" path "valid" watermarks {kind = "register"}
    ac.queue @ready0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready0" path "ready0" watermarks {kind = "register"}
    ac.queue @ready1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready1" path "ready1" watermarks {kind = "register"}
    ac.queue @ready2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready2" path "ready2" watermarks {kind = "register"}
    ac.queue @ready3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready3" path "ready3" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %c2 = arith.constant 2 : i64
      %c3 = arith.constant 3 : i64
      %c4 = arith.constant 4 : i64
      %c5 = arith.constant 5 : i64
      %c6 = arith.constant 6 : i64
      %c7 = arith.constant 7 : i64
      %c8 = arith.constant 8 : i64
      %c35 = arith.constant 35 : i64
      %c63 = arith.constant 63 : i64
      %c255 = arith.constant 255 : i64
      %mask7 = arith.constant 7 : i64
      %mask63 = arith.constant 63 : i64
      %mask255 = arith.constant 255 : i64
      %false = arith.constant false
      %valid_bits, %valid_ok = ac.try_recv @valid : i64
      %s0, %s0_ok = ac.try_recv @slot0 : i64
      %s1, %s1_ok = ac.try_recv @slot1 : i64
      %s2, %s2_ok = ac.try_recv @slot2 : i64
      %s3, %s3_ok = ac.try_recv @slot3 : i64
      %s4, %s4_ok = ac.try_recv @slot4 : i64
      %s5, %s5_ok = ac.try_recv @slot5 : i64
      %s6, %s6_ok = ac.try_recv @slot6 : i64
      %s7, %s7_ok = ac.try_recv @slot7 : i64
      %r0, %r0_ok = ac.try_recv @ready0 : i64
      %r1, %r1_ok = ac.try_recv @ready1 : i64
      %r2, %r2_ok = ac.try_recv @ready2 : i64
      %r3, %r3_ok = ac.try_recv @ready3 : i64
      %update, %has_update = ac.try_recv @Core::@ready_to_iq_v : i64
      %update_desc = ac.trace.decode %update : i64 to i64
      %update_seq = arith.andi %update_desc, %mask255 : i64
      %update_word = arith.shrui %update_seq, %c6 : i64
      %update_off = arith.andi %update_seq, %mask63 : i64
      %update_bit = arith.shli %c1, %update_off : i64
      %update_is0a = arith.cmpi eq, %update_word, %c0 : i64
      %update_is0 = arith.andi %has_update, %update_is0a : i1
      %r0_set = arith.ori %r0, %update_bit : i64
      %nr0 = arith.select %update_is0, %r0_set, %r0 : i64
      %update_is1a = arith.cmpi eq, %update_word, %c1 : i64
      %update_is1 = arith.andi %has_update, %update_is1a : i1
      %r1_set = arith.ori %r1, %update_bit : i64
      %nr1 = arith.select %update_is1, %r1_set, %r1 : i64
      %update_is2a = arith.cmpi eq, %update_word, %c2 : i64
      %update_is2 = arith.andi %has_update, %update_is2a : i1
      %r2_set = arith.ori %r2, %update_bit : i64
      %nr2 = arith.select %update_is2, %r2_set, %r2 : i64
      %update_is3a = arith.cmpi eq, %update_word, %c3 : i64
      %update_is3 = arith.andi %has_update, %update_is3a : i1
      %r3_set = arith.ori %r3, %update_bit : i64
      %nr3 = arith.select %update_is3, %r3_set, %r3 : i64
      %q0_desc = ac.trace.decode %s0 : i64 to i64
      %q0_dv_s = arith.shrui %q0_desc, %c35 : i64
      %q0_dvalid = arith.andi %q0_dv_s, %mask7 : i64
      %q0_dep0_shift = arith.constant 11 : i64
      %q0_dep0_s = arith.shrui %q0_desc, %q0_dep0_shift : i64
      %q0_dep0 = arith.andi %q0_dep0_s, %mask255 : i64
      %q0_dep0_word = arith.shrui %q0_dep0, %c6 : i64
      %q0_dep0_off = arith.andi %q0_dep0, %mask63 : i64
      %q0_dep0_bit = arith.shli %c1, %q0_dep0_off : i64
      %q0_dep0_w0 = arith.cmpi eq, %q0_dep0_word, %c0 : i64
      %q0_dep0_w1 = arith.cmpi eq, %q0_dep0_word, %c1 : i64
      %q0_dep0_w2 = arith.cmpi eq, %q0_dep0_word, %c2 : i64
      %q0_dep0_sel1 = arith.select %q0_dep0_w1, %nr1, %nr3 : i64
      %q0_dep0_sel0 = arith.select %q0_dep0_w0, %nr0, %q0_dep0_sel1 : i64
      %q0_dep0_bits = arith.select %q0_dep0_w2, %nr2, %q0_dep0_sel0 : i64
      %q0_dep0_hit0 = arith.andi %q0_dep0_bits, %q0_dep0_bit : i64
      %q0_dep0_hit = arith.cmpi ne, %q0_dep0_hit0, %c0 : i64
      %q0_dep0_vbit = arith.constant 1 : i64
      %q0_dep0_vhit = arith.andi %q0_dvalid, %q0_dep0_vbit : i64
      %q0_dep0_used = arith.cmpi ne, %q0_dep0_vhit, %c0 : i64
      %q0_dep0_unused = arith.cmpi eq, %q0_dep0_used, %false : i1
      %q0_dep0_ready = arith.ori %q0_dep0_unused, %q0_dep0_hit : i1
      %q0_dep1_shift = arith.constant 19 : i64
      %q0_dep1_s = arith.shrui %q0_desc, %q0_dep1_shift : i64
      %q0_dep1 = arith.andi %q0_dep1_s, %mask255 : i64
      %q0_dep1_word = arith.shrui %q0_dep1, %c6 : i64
      %q0_dep1_off = arith.andi %q0_dep1, %mask63 : i64
      %q0_dep1_bit = arith.shli %c1, %q0_dep1_off : i64
      %q0_dep1_w0 = arith.cmpi eq, %q0_dep1_word, %c0 : i64
      %q0_dep1_w1 = arith.cmpi eq, %q0_dep1_word, %c1 : i64
      %q0_dep1_w2 = arith.cmpi eq, %q0_dep1_word, %c2 : i64
      %q0_dep1_sel1 = arith.select %q0_dep1_w1, %nr1, %nr3 : i64
      %q0_dep1_sel0 = arith.select %q0_dep1_w0, %nr0, %q0_dep1_sel1 : i64
      %q0_dep1_bits = arith.select %q0_dep1_w2, %nr2, %q0_dep1_sel0 : i64
      %q0_dep1_hit0 = arith.andi %q0_dep1_bits, %q0_dep1_bit : i64
      %q0_dep1_hit = arith.cmpi ne, %q0_dep1_hit0, %c0 : i64
      %q0_dep1_vbit = arith.constant 2 : i64
      %q0_dep1_vhit = arith.andi %q0_dvalid, %q0_dep1_vbit : i64
      %q0_dep1_used = arith.cmpi ne, %q0_dep1_vhit, %c0 : i64
      %q0_dep1_unused = arith.cmpi eq, %q0_dep1_used, %false : i1
      %q0_dep1_ready = arith.ori %q0_dep1_unused, %q0_dep1_hit : i1
      %q0_dep2_shift = arith.constant 27 : i64
      %q0_dep2_s = arith.shrui %q0_desc, %q0_dep2_shift : i64
      %q0_dep2 = arith.andi %q0_dep2_s, %mask255 : i64
      %q0_dep2_word = arith.shrui %q0_dep2, %c6 : i64
      %q0_dep2_off = arith.andi %q0_dep2, %mask63 : i64
      %q0_dep2_bit = arith.shli %c1, %q0_dep2_off : i64
      %q0_dep2_w0 = arith.cmpi eq, %q0_dep2_word, %c0 : i64
      %q0_dep2_w1 = arith.cmpi eq, %q0_dep2_word, %c1 : i64
      %q0_dep2_w2 = arith.cmpi eq, %q0_dep2_word, %c2 : i64
      %q0_dep2_sel1 = arith.select %q0_dep2_w1, %nr1, %nr3 : i64
      %q0_dep2_sel0 = arith.select %q0_dep2_w0, %nr0, %q0_dep2_sel1 : i64
      %q0_dep2_bits = arith.select %q0_dep2_w2, %nr2, %q0_dep2_sel0 : i64
      %q0_dep2_hit0 = arith.andi %q0_dep2_bits, %q0_dep2_bit : i64
      %q0_dep2_hit = arith.cmpi ne, %q0_dep2_hit0, %c0 : i64
      %q0_dep2_vbit = arith.constant 4 : i64
      %q0_dep2_vhit = arith.andi %q0_dvalid, %q0_dep2_vbit : i64
      %q0_dep2_used = arith.cmpi ne, %q0_dep2_vhit, %c0 : i64
      %q0_dep2_unused = arith.cmpi eq, %q0_dep2_used, %false : i1
      %q0_dep2_ready = arith.ori %q0_dep2_unused, %q0_dep2_hit : i1
      %q0_deps01 = arith.andi %q0_dep0_ready, %q0_dep1_ready : i1
      %q0_deps = arith.andi %q0_deps01, %q0_dep2_ready : i1
      %q0_vbit = arith.constant 1 : i64
      %q0_vhit = arith.andi %valid_bits, %q0_vbit : i64
      %q0_occupied = arith.cmpi ne, %q0_vhit, %c0 : i64
      %q0_eligible = arith.andi %q0_occupied, %q0_deps : i1
      %q1_desc = ac.trace.decode %s1 : i64 to i64
      %q1_dv_s = arith.shrui %q1_desc, %c35 : i64
      %q1_dvalid = arith.andi %q1_dv_s, %mask7 : i64
      %q1_dep0_shift = arith.constant 11 : i64
      %q1_dep0_s = arith.shrui %q1_desc, %q1_dep0_shift : i64
      %q1_dep0 = arith.andi %q1_dep0_s, %mask255 : i64
      %q1_dep0_word = arith.shrui %q1_dep0, %c6 : i64
      %q1_dep0_off = arith.andi %q1_dep0, %mask63 : i64
      %q1_dep0_bit = arith.shli %c1, %q1_dep0_off : i64
      %q1_dep0_w0 = arith.cmpi eq, %q1_dep0_word, %c0 : i64
      %q1_dep0_w1 = arith.cmpi eq, %q1_dep0_word, %c1 : i64
      %q1_dep0_w2 = arith.cmpi eq, %q1_dep0_word, %c2 : i64
      %q1_dep0_sel1 = arith.select %q1_dep0_w1, %nr1, %nr3 : i64
      %q1_dep0_sel0 = arith.select %q1_dep0_w0, %nr0, %q1_dep0_sel1 : i64
      %q1_dep0_bits = arith.select %q1_dep0_w2, %nr2, %q1_dep0_sel0 : i64
      %q1_dep0_hit0 = arith.andi %q1_dep0_bits, %q1_dep0_bit : i64
      %q1_dep0_hit = arith.cmpi ne, %q1_dep0_hit0, %c0 : i64
      %q1_dep0_vbit = arith.constant 1 : i64
      %q1_dep0_vhit = arith.andi %q1_dvalid, %q1_dep0_vbit : i64
      %q1_dep0_used = arith.cmpi ne, %q1_dep0_vhit, %c0 : i64
      %q1_dep0_unused = arith.cmpi eq, %q1_dep0_used, %false : i1
      %q1_dep0_ready = arith.ori %q1_dep0_unused, %q1_dep0_hit : i1
      %q1_dep1_shift = arith.constant 19 : i64
      %q1_dep1_s = arith.shrui %q1_desc, %q1_dep1_shift : i64
      %q1_dep1 = arith.andi %q1_dep1_s, %mask255 : i64
      %q1_dep1_word = arith.shrui %q1_dep1, %c6 : i64
      %q1_dep1_off = arith.andi %q1_dep1, %mask63 : i64
      %q1_dep1_bit = arith.shli %c1, %q1_dep1_off : i64
      %q1_dep1_w0 = arith.cmpi eq, %q1_dep1_word, %c0 : i64
      %q1_dep1_w1 = arith.cmpi eq, %q1_dep1_word, %c1 : i64
      %q1_dep1_w2 = arith.cmpi eq, %q1_dep1_word, %c2 : i64
      %q1_dep1_sel1 = arith.select %q1_dep1_w1, %nr1, %nr3 : i64
      %q1_dep1_sel0 = arith.select %q1_dep1_w0, %nr0, %q1_dep1_sel1 : i64
      %q1_dep1_bits = arith.select %q1_dep1_w2, %nr2, %q1_dep1_sel0 : i64
      %q1_dep1_hit0 = arith.andi %q1_dep1_bits, %q1_dep1_bit : i64
      %q1_dep1_hit = arith.cmpi ne, %q1_dep1_hit0, %c0 : i64
      %q1_dep1_vbit = arith.constant 2 : i64
      %q1_dep1_vhit = arith.andi %q1_dvalid, %q1_dep1_vbit : i64
      %q1_dep1_used = arith.cmpi ne, %q1_dep1_vhit, %c0 : i64
      %q1_dep1_unused = arith.cmpi eq, %q1_dep1_used, %false : i1
      %q1_dep1_ready = arith.ori %q1_dep1_unused, %q1_dep1_hit : i1
      %q1_dep2_shift = arith.constant 27 : i64
      %q1_dep2_s = arith.shrui %q1_desc, %q1_dep2_shift : i64
      %q1_dep2 = arith.andi %q1_dep2_s, %mask255 : i64
      %q1_dep2_word = arith.shrui %q1_dep2, %c6 : i64
      %q1_dep2_off = arith.andi %q1_dep2, %mask63 : i64
      %q1_dep2_bit = arith.shli %c1, %q1_dep2_off : i64
      %q1_dep2_w0 = arith.cmpi eq, %q1_dep2_word, %c0 : i64
      %q1_dep2_w1 = arith.cmpi eq, %q1_dep2_word, %c1 : i64
      %q1_dep2_w2 = arith.cmpi eq, %q1_dep2_word, %c2 : i64
      %q1_dep2_sel1 = arith.select %q1_dep2_w1, %nr1, %nr3 : i64
      %q1_dep2_sel0 = arith.select %q1_dep2_w0, %nr0, %q1_dep2_sel1 : i64
      %q1_dep2_bits = arith.select %q1_dep2_w2, %nr2, %q1_dep2_sel0 : i64
      %q1_dep2_hit0 = arith.andi %q1_dep2_bits, %q1_dep2_bit : i64
      %q1_dep2_hit = arith.cmpi ne, %q1_dep2_hit0, %c0 : i64
      %q1_dep2_vbit = arith.constant 4 : i64
      %q1_dep2_vhit = arith.andi %q1_dvalid, %q1_dep2_vbit : i64
      %q1_dep2_used = arith.cmpi ne, %q1_dep2_vhit, %c0 : i64
      %q1_dep2_unused = arith.cmpi eq, %q1_dep2_used, %false : i1
      %q1_dep2_ready = arith.ori %q1_dep2_unused, %q1_dep2_hit : i1
      %q1_deps01 = arith.andi %q1_dep0_ready, %q1_dep1_ready : i1
      %q1_deps = arith.andi %q1_deps01, %q1_dep2_ready : i1
      %q1_vbit = arith.constant 2 : i64
      %q1_vhit = arith.andi %valid_bits, %q1_vbit : i64
      %q1_occupied = arith.cmpi ne, %q1_vhit, %c0 : i64
      %q1_eligible = arith.andi %q1_occupied, %q1_deps : i1
      %q2_desc = ac.trace.decode %s2 : i64 to i64
      %q2_dv_s = arith.shrui %q2_desc, %c35 : i64
      %q2_dvalid = arith.andi %q2_dv_s, %mask7 : i64
      %q2_dep0_shift = arith.constant 11 : i64
      %q2_dep0_s = arith.shrui %q2_desc, %q2_dep0_shift : i64
      %q2_dep0 = arith.andi %q2_dep0_s, %mask255 : i64
      %q2_dep0_word = arith.shrui %q2_dep0, %c6 : i64
      %q2_dep0_off = arith.andi %q2_dep0, %mask63 : i64
      %q2_dep0_bit = arith.shli %c1, %q2_dep0_off : i64
      %q2_dep0_w0 = arith.cmpi eq, %q2_dep0_word, %c0 : i64
      %q2_dep0_w1 = arith.cmpi eq, %q2_dep0_word, %c1 : i64
      %q2_dep0_w2 = arith.cmpi eq, %q2_dep0_word, %c2 : i64
      %q2_dep0_sel1 = arith.select %q2_dep0_w1, %nr1, %nr3 : i64
      %q2_dep0_sel0 = arith.select %q2_dep0_w0, %nr0, %q2_dep0_sel1 : i64
      %q2_dep0_bits = arith.select %q2_dep0_w2, %nr2, %q2_dep0_sel0 : i64
      %q2_dep0_hit0 = arith.andi %q2_dep0_bits, %q2_dep0_bit : i64
      %q2_dep0_hit = arith.cmpi ne, %q2_dep0_hit0, %c0 : i64
      %q2_dep0_vbit = arith.constant 1 : i64
      %q2_dep0_vhit = arith.andi %q2_dvalid, %q2_dep0_vbit : i64
      %q2_dep0_used = arith.cmpi ne, %q2_dep0_vhit, %c0 : i64
      %q2_dep0_unused = arith.cmpi eq, %q2_dep0_used, %false : i1
      %q2_dep0_ready = arith.ori %q2_dep0_unused, %q2_dep0_hit : i1
      %q2_dep1_shift = arith.constant 19 : i64
      %q2_dep1_s = arith.shrui %q2_desc, %q2_dep1_shift : i64
      %q2_dep1 = arith.andi %q2_dep1_s, %mask255 : i64
      %q2_dep1_word = arith.shrui %q2_dep1, %c6 : i64
      %q2_dep1_off = arith.andi %q2_dep1, %mask63 : i64
      %q2_dep1_bit = arith.shli %c1, %q2_dep1_off : i64
      %q2_dep1_w0 = arith.cmpi eq, %q2_dep1_word, %c0 : i64
      %q2_dep1_w1 = arith.cmpi eq, %q2_dep1_word, %c1 : i64
      %q2_dep1_w2 = arith.cmpi eq, %q2_dep1_word, %c2 : i64
      %q2_dep1_sel1 = arith.select %q2_dep1_w1, %nr1, %nr3 : i64
      %q2_dep1_sel0 = arith.select %q2_dep1_w0, %nr0, %q2_dep1_sel1 : i64
      %q2_dep1_bits = arith.select %q2_dep1_w2, %nr2, %q2_dep1_sel0 : i64
      %q2_dep1_hit0 = arith.andi %q2_dep1_bits, %q2_dep1_bit : i64
      %q2_dep1_hit = arith.cmpi ne, %q2_dep1_hit0, %c0 : i64
      %q2_dep1_vbit = arith.constant 2 : i64
      %q2_dep1_vhit = arith.andi %q2_dvalid, %q2_dep1_vbit : i64
      %q2_dep1_used = arith.cmpi ne, %q2_dep1_vhit, %c0 : i64
      %q2_dep1_unused = arith.cmpi eq, %q2_dep1_used, %false : i1
      %q2_dep1_ready = arith.ori %q2_dep1_unused, %q2_dep1_hit : i1
      %q2_dep2_shift = arith.constant 27 : i64
      %q2_dep2_s = arith.shrui %q2_desc, %q2_dep2_shift : i64
      %q2_dep2 = arith.andi %q2_dep2_s, %mask255 : i64
      %q2_dep2_word = arith.shrui %q2_dep2, %c6 : i64
      %q2_dep2_off = arith.andi %q2_dep2, %mask63 : i64
      %q2_dep2_bit = arith.shli %c1, %q2_dep2_off : i64
      %q2_dep2_w0 = arith.cmpi eq, %q2_dep2_word, %c0 : i64
      %q2_dep2_w1 = arith.cmpi eq, %q2_dep2_word, %c1 : i64
      %q2_dep2_w2 = arith.cmpi eq, %q2_dep2_word, %c2 : i64
      %q2_dep2_sel1 = arith.select %q2_dep2_w1, %nr1, %nr3 : i64
      %q2_dep2_sel0 = arith.select %q2_dep2_w0, %nr0, %q2_dep2_sel1 : i64
      %q2_dep2_bits = arith.select %q2_dep2_w2, %nr2, %q2_dep2_sel0 : i64
      %q2_dep2_hit0 = arith.andi %q2_dep2_bits, %q2_dep2_bit : i64
      %q2_dep2_hit = arith.cmpi ne, %q2_dep2_hit0, %c0 : i64
      %q2_dep2_vbit = arith.constant 4 : i64
      %q2_dep2_vhit = arith.andi %q2_dvalid, %q2_dep2_vbit : i64
      %q2_dep2_used = arith.cmpi ne, %q2_dep2_vhit, %c0 : i64
      %q2_dep2_unused = arith.cmpi eq, %q2_dep2_used, %false : i1
      %q2_dep2_ready = arith.ori %q2_dep2_unused, %q2_dep2_hit : i1
      %q2_deps01 = arith.andi %q2_dep0_ready, %q2_dep1_ready : i1
      %q2_deps = arith.andi %q2_deps01, %q2_dep2_ready : i1
      %q2_vbit = arith.constant 4 : i64
      %q2_vhit = arith.andi %valid_bits, %q2_vbit : i64
      %q2_occupied = arith.cmpi ne, %q2_vhit, %c0 : i64
      %q2_eligible = arith.andi %q2_occupied, %q2_deps : i1
      %q3_desc = ac.trace.decode %s3 : i64 to i64
      %q3_dv_s = arith.shrui %q3_desc, %c35 : i64
      %q3_dvalid = arith.andi %q3_dv_s, %mask7 : i64
      %q3_dep0_shift = arith.constant 11 : i64
      %q3_dep0_s = arith.shrui %q3_desc, %q3_dep0_shift : i64
      %q3_dep0 = arith.andi %q3_dep0_s, %mask255 : i64
      %q3_dep0_word = arith.shrui %q3_dep0, %c6 : i64
      %q3_dep0_off = arith.andi %q3_dep0, %mask63 : i64
      %q3_dep0_bit = arith.shli %c1, %q3_dep0_off : i64
      %q3_dep0_w0 = arith.cmpi eq, %q3_dep0_word, %c0 : i64
      %q3_dep0_w1 = arith.cmpi eq, %q3_dep0_word, %c1 : i64
      %q3_dep0_w2 = arith.cmpi eq, %q3_dep0_word, %c2 : i64
      %q3_dep0_sel1 = arith.select %q3_dep0_w1, %nr1, %nr3 : i64
      %q3_dep0_sel0 = arith.select %q3_dep0_w0, %nr0, %q3_dep0_sel1 : i64
      %q3_dep0_bits = arith.select %q3_dep0_w2, %nr2, %q3_dep0_sel0 : i64
      %q3_dep0_hit0 = arith.andi %q3_dep0_bits, %q3_dep0_bit : i64
      %q3_dep0_hit = arith.cmpi ne, %q3_dep0_hit0, %c0 : i64
      %q3_dep0_vbit = arith.constant 1 : i64
      %q3_dep0_vhit = arith.andi %q3_dvalid, %q3_dep0_vbit : i64
      %q3_dep0_used = arith.cmpi ne, %q3_dep0_vhit, %c0 : i64
      %q3_dep0_unused = arith.cmpi eq, %q3_dep0_used, %false : i1
      %q3_dep0_ready = arith.ori %q3_dep0_unused, %q3_dep0_hit : i1
      %q3_dep1_shift = arith.constant 19 : i64
      %q3_dep1_s = arith.shrui %q3_desc, %q3_dep1_shift : i64
      %q3_dep1 = arith.andi %q3_dep1_s, %mask255 : i64
      %q3_dep1_word = arith.shrui %q3_dep1, %c6 : i64
      %q3_dep1_off = arith.andi %q3_dep1, %mask63 : i64
      %q3_dep1_bit = arith.shli %c1, %q3_dep1_off : i64
      %q3_dep1_w0 = arith.cmpi eq, %q3_dep1_word, %c0 : i64
      %q3_dep1_w1 = arith.cmpi eq, %q3_dep1_word, %c1 : i64
      %q3_dep1_w2 = arith.cmpi eq, %q3_dep1_word, %c2 : i64
      %q3_dep1_sel1 = arith.select %q3_dep1_w1, %nr1, %nr3 : i64
      %q3_dep1_sel0 = arith.select %q3_dep1_w0, %nr0, %q3_dep1_sel1 : i64
      %q3_dep1_bits = arith.select %q3_dep1_w2, %nr2, %q3_dep1_sel0 : i64
      %q3_dep1_hit0 = arith.andi %q3_dep1_bits, %q3_dep1_bit : i64
      %q3_dep1_hit = arith.cmpi ne, %q3_dep1_hit0, %c0 : i64
      %q3_dep1_vbit = arith.constant 2 : i64
      %q3_dep1_vhit = arith.andi %q3_dvalid, %q3_dep1_vbit : i64
      %q3_dep1_used = arith.cmpi ne, %q3_dep1_vhit, %c0 : i64
      %q3_dep1_unused = arith.cmpi eq, %q3_dep1_used, %false : i1
      %q3_dep1_ready = arith.ori %q3_dep1_unused, %q3_dep1_hit : i1
      %q3_dep2_shift = arith.constant 27 : i64
      %q3_dep2_s = arith.shrui %q3_desc, %q3_dep2_shift : i64
      %q3_dep2 = arith.andi %q3_dep2_s, %mask255 : i64
      %q3_dep2_word = arith.shrui %q3_dep2, %c6 : i64
      %q3_dep2_off = arith.andi %q3_dep2, %mask63 : i64
      %q3_dep2_bit = arith.shli %c1, %q3_dep2_off : i64
      %q3_dep2_w0 = arith.cmpi eq, %q3_dep2_word, %c0 : i64
      %q3_dep2_w1 = arith.cmpi eq, %q3_dep2_word, %c1 : i64
      %q3_dep2_w2 = arith.cmpi eq, %q3_dep2_word, %c2 : i64
      %q3_dep2_sel1 = arith.select %q3_dep2_w1, %nr1, %nr3 : i64
      %q3_dep2_sel0 = arith.select %q3_dep2_w0, %nr0, %q3_dep2_sel1 : i64
      %q3_dep2_bits = arith.select %q3_dep2_w2, %nr2, %q3_dep2_sel0 : i64
      %q3_dep2_hit0 = arith.andi %q3_dep2_bits, %q3_dep2_bit : i64
      %q3_dep2_hit = arith.cmpi ne, %q3_dep2_hit0, %c0 : i64
      %q3_dep2_vbit = arith.constant 4 : i64
      %q3_dep2_vhit = arith.andi %q3_dvalid, %q3_dep2_vbit : i64
      %q3_dep2_used = arith.cmpi ne, %q3_dep2_vhit, %c0 : i64
      %q3_dep2_unused = arith.cmpi eq, %q3_dep2_used, %false : i1
      %q3_dep2_ready = arith.ori %q3_dep2_unused, %q3_dep2_hit : i1
      %q3_deps01 = arith.andi %q3_dep0_ready, %q3_dep1_ready : i1
      %q3_deps = arith.andi %q3_deps01, %q3_dep2_ready : i1
      %q3_vbit = arith.constant 8 : i64
      %q3_vhit = arith.andi %valid_bits, %q3_vbit : i64
      %q3_occupied = arith.cmpi ne, %q3_vhit, %c0 : i64
      %q3_eligible = arith.andi %q3_occupied, %q3_deps : i1
      %q4_desc = ac.trace.decode %s4 : i64 to i64
      %q4_dv_s = arith.shrui %q4_desc, %c35 : i64
      %q4_dvalid = arith.andi %q4_dv_s, %mask7 : i64
      %q4_dep0_shift = arith.constant 11 : i64
      %q4_dep0_s = arith.shrui %q4_desc, %q4_dep0_shift : i64
      %q4_dep0 = arith.andi %q4_dep0_s, %mask255 : i64
      %q4_dep0_word = arith.shrui %q4_dep0, %c6 : i64
      %q4_dep0_off = arith.andi %q4_dep0, %mask63 : i64
      %q4_dep0_bit = arith.shli %c1, %q4_dep0_off : i64
      %q4_dep0_w0 = arith.cmpi eq, %q4_dep0_word, %c0 : i64
      %q4_dep0_w1 = arith.cmpi eq, %q4_dep0_word, %c1 : i64
      %q4_dep0_w2 = arith.cmpi eq, %q4_dep0_word, %c2 : i64
      %q4_dep0_sel1 = arith.select %q4_dep0_w1, %nr1, %nr3 : i64
      %q4_dep0_sel0 = arith.select %q4_dep0_w0, %nr0, %q4_dep0_sel1 : i64
      %q4_dep0_bits = arith.select %q4_dep0_w2, %nr2, %q4_dep0_sel0 : i64
      %q4_dep0_hit0 = arith.andi %q4_dep0_bits, %q4_dep0_bit : i64
      %q4_dep0_hit = arith.cmpi ne, %q4_dep0_hit0, %c0 : i64
      %q4_dep0_vbit = arith.constant 1 : i64
      %q4_dep0_vhit = arith.andi %q4_dvalid, %q4_dep0_vbit : i64
      %q4_dep0_used = arith.cmpi ne, %q4_dep0_vhit, %c0 : i64
      %q4_dep0_unused = arith.cmpi eq, %q4_dep0_used, %false : i1
      %q4_dep0_ready = arith.ori %q4_dep0_unused, %q4_dep0_hit : i1
      %q4_dep1_shift = arith.constant 19 : i64
      %q4_dep1_s = arith.shrui %q4_desc, %q4_dep1_shift : i64
      %q4_dep1 = arith.andi %q4_dep1_s, %mask255 : i64
      %q4_dep1_word = arith.shrui %q4_dep1, %c6 : i64
      %q4_dep1_off = arith.andi %q4_dep1, %mask63 : i64
      %q4_dep1_bit = arith.shli %c1, %q4_dep1_off : i64
      %q4_dep1_w0 = arith.cmpi eq, %q4_dep1_word, %c0 : i64
      %q4_dep1_w1 = arith.cmpi eq, %q4_dep1_word, %c1 : i64
      %q4_dep1_w2 = arith.cmpi eq, %q4_dep1_word, %c2 : i64
      %q4_dep1_sel1 = arith.select %q4_dep1_w1, %nr1, %nr3 : i64
      %q4_dep1_sel0 = arith.select %q4_dep1_w0, %nr0, %q4_dep1_sel1 : i64
      %q4_dep1_bits = arith.select %q4_dep1_w2, %nr2, %q4_dep1_sel0 : i64
      %q4_dep1_hit0 = arith.andi %q4_dep1_bits, %q4_dep1_bit : i64
      %q4_dep1_hit = arith.cmpi ne, %q4_dep1_hit0, %c0 : i64
      %q4_dep1_vbit = arith.constant 2 : i64
      %q4_dep1_vhit = arith.andi %q4_dvalid, %q4_dep1_vbit : i64
      %q4_dep1_used = arith.cmpi ne, %q4_dep1_vhit, %c0 : i64
      %q4_dep1_unused = arith.cmpi eq, %q4_dep1_used, %false : i1
      %q4_dep1_ready = arith.ori %q4_dep1_unused, %q4_dep1_hit : i1
      %q4_dep2_shift = arith.constant 27 : i64
      %q4_dep2_s = arith.shrui %q4_desc, %q4_dep2_shift : i64
      %q4_dep2 = arith.andi %q4_dep2_s, %mask255 : i64
      %q4_dep2_word = arith.shrui %q4_dep2, %c6 : i64
      %q4_dep2_off = arith.andi %q4_dep2, %mask63 : i64
      %q4_dep2_bit = arith.shli %c1, %q4_dep2_off : i64
      %q4_dep2_w0 = arith.cmpi eq, %q4_dep2_word, %c0 : i64
      %q4_dep2_w1 = arith.cmpi eq, %q4_dep2_word, %c1 : i64
      %q4_dep2_w2 = arith.cmpi eq, %q4_dep2_word, %c2 : i64
      %q4_dep2_sel1 = arith.select %q4_dep2_w1, %nr1, %nr3 : i64
      %q4_dep2_sel0 = arith.select %q4_dep2_w0, %nr0, %q4_dep2_sel1 : i64
      %q4_dep2_bits = arith.select %q4_dep2_w2, %nr2, %q4_dep2_sel0 : i64
      %q4_dep2_hit0 = arith.andi %q4_dep2_bits, %q4_dep2_bit : i64
      %q4_dep2_hit = arith.cmpi ne, %q4_dep2_hit0, %c0 : i64
      %q4_dep2_vbit = arith.constant 4 : i64
      %q4_dep2_vhit = arith.andi %q4_dvalid, %q4_dep2_vbit : i64
      %q4_dep2_used = arith.cmpi ne, %q4_dep2_vhit, %c0 : i64
      %q4_dep2_unused = arith.cmpi eq, %q4_dep2_used, %false : i1
      %q4_dep2_ready = arith.ori %q4_dep2_unused, %q4_dep2_hit : i1
      %q4_deps01 = arith.andi %q4_dep0_ready, %q4_dep1_ready : i1
      %q4_deps = arith.andi %q4_deps01, %q4_dep2_ready : i1
      %q4_vbit = arith.constant 16 : i64
      %q4_vhit = arith.andi %valid_bits, %q4_vbit : i64
      %q4_occupied = arith.cmpi ne, %q4_vhit, %c0 : i64
      %q4_eligible = arith.andi %q4_occupied, %q4_deps : i1
      %q5_desc = ac.trace.decode %s5 : i64 to i64
      %q5_dv_s = arith.shrui %q5_desc, %c35 : i64
      %q5_dvalid = arith.andi %q5_dv_s, %mask7 : i64
      %q5_dep0_shift = arith.constant 11 : i64
      %q5_dep0_s = arith.shrui %q5_desc, %q5_dep0_shift : i64
      %q5_dep0 = arith.andi %q5_dep0_s, %mask255 : i64
      %q5_dep0_word = arith.shrui %q5_dep0, %c6 : i64
      %q5_dep0_off = arith.andi %q5_dep0, %mask63 : i64
      %q5_dep0_bit = arith.shli %c1, %q5_dep0_off : i64
      %q5_dep0_w0 = arith.cmpi eq, %q5_dep0_word, %c0 : i64
      %q5_dep0_w1 = arith.cmpi eq, %q5_dep0_word, %c1 : i64
      %q5_dep0_w2 = arith.cmpi eq, %q5_dep0_word, %c2 : i64
      %q5_dep0_sel1 = arith.select %q5_dep0_w1, %nr1, %nr3 : i64
      %q5_dep0_sel0 = arith.select %q5_dep0_w0, %nr0, %q5_dep0_sel1 : i64
      %q5_dep0_bits = arith.select %q5_dep0_w2, %nr2, %q5_dep0_sel0 : i64
      %q5_dep0_hit0 = arith.andi %q5_dep0_bits, %q5_dep0_bit : i64
      %q5_dep0_hit = arith.cmpi ne, %q5_dep0_hit0, %c0 : i64
      %q5_dep0_vbit = arith.constant 1 : i64
      %q5_dep0_vhit = arith.andi %q5_dvalid, %q5_dep0_vbit : i64
      %q5_dep0_used = arith.cmpi ne, %q5_dep0_vhit, %c0 : i64
      %q5_dep0_unused = arith.cmpi eq, %q5_dep0_used, %false : i1
      %q5_dep0_ready = arith.ori %q5_dep0_unused, %q5_dep0_hit : i1
      %q5_dep1_shift = arith.constant 19 : i64
      %q5_dep1_s = arith.shrui %q5_desc, %q5_dep1_shift : i64
      %q5_dep1 = arith.andi %q5_dep1_s, %mask255 : i64
      %q5_dep1_word = arith.shrui %q5_dep1, %c6 : i64
      %q5_dep1_off = arith.andi %q5_dep1, %mask63 : i64
      %q5_dep1_bit = arith.shli %c1, %q5_dep1_off : i64
      %q5_dep1_w0 = arith.cmpi eq, %q5_dep1_word, %c0 : i64
      %q5_dep1_w1 = arith.cmpi eq, %q5_dep1_word, %c1 : i64
      %q5_dep1_w2 = arith.cmpi eq, %q5_dep1_word, %c2 : i64
      %q5_dep1_sel1 = arith.select %q5_dep1_w1, %nr1, %nr3 : i64
      %q5_dep1_sel0 = arith.select %q5_dep1_w0, %nr0, %q5_dep1_sel1 : i64
      %q5_dep1_bits = arith.select %q5_dep1_w2, %nr2, %q5_dep1_sel0 : i64
      %q5_dep1_hit0 = arith.andi %q5_dep1_bits, %q5_dep1_bit : i64
      %q5_dep1_hit = arith.cmpi ne, %q5_dep1_hit0, %c0 : i64
      %q5_dep1_vbit = arith.constant 2 : i64
      %q5_dep1_vhit = arith.andi %q5_dvalid, %q5_dep1_vbit : i64
      %q5_dep1_used = arith.cmpi ne, %q5_dep1_vhit, %c0 : i64
      %q5_dep1_unused = arith.cmpi eq, %q5_dep1_used, %false : i1
      %q5_dep1_ready = arith.ori %q5_dep1_unused, %q5_dep1_hit : i1
      %q5_dep2_shift = arith.constant 27 : i64
      %q5_dep2_s = arith.shrui %q5_desc, %q5_dep2_shift : i64
      %q5_dep2 = arith.andi %q5_dep2_s, %mask255 : i64
      %q5_dep2_word = arith.shrui %q5_dep2, %c6 : i64
      %q5_dep2_off = arith.andi %q5_dep2, %mask63 : i64
      %q5_dep2_bit = arith.shli %c1, %q5_dep2_off : i64
      %q5_dep2_w0 = arith.cmpi eq, %q5_dep2_word, %c0 : i64
      %q5_dep2_w1 = arith.cmpi eq, %q5_dep2_word, %c1 : i64
      %q5_dep2_w2 = arith.cmpi eq, %q5_dep2_word, %c2 : i64
      %q5_dep2_sel1 = arith.select %q5_dep2_w1, %nr1, %nr3 : i64
      %q5_dep2_sel0 = arith.select %q5_dep2_w0, %nr0, %q5_dep2_sel1 : i64
      %q5_dep2_bits = arith.select %q5_dep2_w2, %nr2, %q5_dep2_sel0 : i64
      %q5_dep2_hit0 = arith.andi %q5_dep2_bits, %q5_dep2_bit : i64
      %q5_dep2_hit = arith.cmpi ne, %q5_dep2_hit0, %c0 : i64
      %q5_dep2_vbit = arith.constant 4 : i64
      %q5_dep2_vhit = arith.andi %q5_dvalid, %q5_dep2_vbit : i64
      %q5_dep2_used = arith.cmpi ne, %q5_dep2_vhit, %c0 : i64
      %q5_dep2_unused = arith.cmpi eq, %q5_dep2_used, %false : i1
      %q5_dep2_ready = arith.ori %q5_dep2_unused, %q5_dep2_hit : i1
      %q5_deps01 = arith.andi %q5_dep0_ready, %q5_dep1_ready : i1
      %q5_deps = arith.andi %q5_deps01, %q5_dep2_ready : i1
      %q5_vbit = arith.constant 32 : i64
      %q5_vhit = arith.andi %valid_bits, %q5_vbit : i64
      %q5_occupied = arith.cmpi ne, %q5_vhit, %c0 : i64
      %q5_eligible = arith.andi %q5_occupied, %q5_deps : i1
      %q6_desc = ac.trace.decode %s6 : i64 to i64
      %q6_dv_s = arith.shrui %q6_desc, %c35 : i64
      %q6_dvalid = arith.andi %q6_dv_s, %mask7 : i64
      %q6_dep0_shift = arith.constant 11 : i64
      %q6_dep0_s = arith.shrui %q6_desc, %q6_dep0_shift : i64
      %q6_dep0 = arith.andi %q6_dep0_s, %mask255 : i64
      %q6_dep0_word = arith.shrui %q6_dep0, %c6 : i64
      %q6_dep0_off = arith.andi %q6_dep0, %mask63 : i64
      %q6_dep0_bit = arith.shli %c1, %q6_dep0_off : i64
      %q6_dep0_w0 = arith.cmpi eq, %q6_dep0_word, %c0 : i64
      %q6_dep0_w1 = arith.cmpi eq, %q6_dep0_word, %c1 : i64
      %q6_dep0_w2 = arith.cmpi eq, %q6_dep0_word, %c2 : i64
      %q6_dep0_sel1 = arith.select %q6_dep0_w1, %nr1, %nr3 : i64
      %q6_dep0_sel0 = arith.select %q6_dep0_w0, %nr0, %q6_dep0_sel1 : i64
      %q6_dep0_bits = arith.select %q6_dep0_w2, %nr2, %q6_dep0_sel0 : i64
      %q6_dep0_hit0 = arith.andi %q6_dep0_bits, %q6_dep0_bit : i64
      %q6_dep0_hit = arith.cmpi ne, %q6_dep0_hit0, %c0 : i64
      %q6_dep0_vbit = arith.constant 1 : i64
      %q6_dep0_vhit = arith.andi %q6_dvalid, %q6_dep0_vbit : i64
      %q6_dep0_used = arith.cmpi ne, %q6_dep0_vhit, %c0 : i64
      %q6_dep0_unused = arith.cmpi eq, %q6_dep0_used, %false : i1
      %q6_dep0_ready = arith.ori %q6_dep0_unused, %q6_dep0_hit : i1
      %q6_dep1_shift = arith.constant 19 : i64
      %q6_dep1_s = arith.shrui %q6_desc, %q6_dep1_shift : i64
      %q6_dep1 = arith.andi %q6_dep1_s, %mask255 : i64
      %q6_dep1_word = arith.shrui %q6_dep1, %c6 : i64
      %q6_dep1_off = arith.andi %q6_dep1, %mask63 : i64
      %q6_dep1_bit = arith.shli %c1, %q6_dep1_off : i64
      %q6_dep1_w0 = arith.cmpi eq, %q6_dep1_word, %c0 : i64
      %q6_dep1_w1 = arith.cmpi eq, %q6_dep1_word, %c1 : i64
      %q6_dep1_w2 = arith.cmpi eq, %q6_dep1_word, %c2 : i64
      %q6_dep1_sel1 = arith.select %q6_dep1_w1, %nr1, %nr3 : i64
      %q6_dep1_sel0 = arith.select %q6_dep1_w0, %nr0, %q6_dep1_sel1 : i64
      %q6_dep1_bits = arith.select %q6_dep1_w2, %nr2, %q6_dep1_sel0 : i64
      %q6_dep1_hit0 = arith.andi %q6_dep1_bits, %q6_dep1_bit : i64
      %q6_dep1_hit = arith.cmpi ne, %q6_dep1_hit0, %c0 : i64
      %q6_dep1_vbit = arith.constant 2 : i64
      %q6_dep1_vhit = arith.andi %q6_dvalid, %q6_dep1_vbit : i64
      %q6_dep1_used = arith.cmpi ne, %q6_dep1_vhit, %c0 : i64
      %q6_dep1_unused = arith.cmpi eq, %q6_dep1_used, %false : i1
      %q6_dep1_ready = arith.ori %q6_dep1_unused, %q6_dep1_hit : i1
      %q6_dep2_shift = arith.constant 27 : i64
      %q6_dep2_s = arith.shrui %q6_desc, %q6_dep2_shift : i64
      %q6_dep2 = arith.andi %q6_dep2_s, %mask255 : i64
      %q6_dep2_word = arith.shrui %q6_dep2, %c6 : i64
      %q6_dep2_off = arith.andi %q6_dep2, %mask63 : i64
      %q6_dep2_bit = arith.shli %c1, %q6_dep2_off : i64
      %q6_dep2_w0 = arith.cmpi eq, %q6_dep2_word, %c0 : i64
      %q6_dep2_w1 = arith.cmpi eq, %q6_dep2_word, %c1 : i64
      %q6_dep2_w2 = arith.cmpi eq, %q6_dep2_word, %c2 : i64
      %q6_dep2_sel1 = arith.select %q6_dep2_w1, %nr1, %nr3 : i64
      %q6_dep2_sel0 = arith.select %q6_dep2_w0, %nr0, %q6_dep2_sel1 : i64
      %q6_dep2_bits = arith.select %q6_dep2_w2, %nr2, %q6_dep2_sel0 : i64
      %q6_dep2_hit0 = arith.andi %q6_dep2_bits, %q6_dep2_bit : i64
      %q6_dep2_hit = arith.cmpi ne, %q6_dep2_hit0, %c0 : i64
      %q6_dep2_vbit = arith.constant 4 : i64
      %q6_dep2_vhit = arith.andi %q6_dvalid, %q6_dep2_vbit : i64
      %q6_dep2_used = arith.cmpi ne, %q6_dep2_vhit, %c0 : i64
      %q6_dep2_unused = arith.cmpi eq, %q6_dep2_used, %false : i1
      %q6_dep2_ready = arith.ori %q6_dep2_unused, %q6_dep2_hit : i1
      %q6_deps01 = arith.andi %q6_dep0_ready, %q6_dep1_ready : i1
      %q6_deps = arith.andi %q6_deps01, %q6_dep2_ready : i1
      %q6_vbit = arith.constant 64 : i64
      %q6_vhit = arith.andi %valid_bits, %q6_vbit : i64
      %q6_occupied = arith.cmpi ne, %q6_vhit, %c0 : i64
      %q6_eligible = arith.andi %q6_occupied, %q6_deps : i1
      %q7_desc = ac.trace.decode %s7 : i64 to i64
      %q7_dv_s = arith.shrui %q7_desc, %c35 : i64
      %q7_dvalid = arith.andi %q7_dv_s, %mask7 : i64
      %q7_dep0_shift = arith.constant 11 : i64
      %q7_dep0_s = arith.shrui %q7_desc, %q7_dep0_shift : i64
      %q7_dep0 = arith.andi %q7_dep0_s, %mask255 : i64
      %q7_dep0_word = arith.shrui %q7_dep0, %c6 : i64
      %q7_dep0_off = arith.andi %q7_dep0, %mask63 : i64
      %q7_dep0_bit = arith.shli %c1, %q7_dep0_off : i64
      %q7_dep0_w0 = arith.cmpi eq, %q7_dep0_word, %c0 : i64
      %q7_dep0_w1 = arith.cmpi eq, %q7_dep0_word, %c1 : i64
      %q7_dep0_w2 = arith.cmpi eq, %q7_dep0_word, %c2 : i64
      %q7_dep0_sel1 = arith.select %q7_dep0_w1, %nr1, %nr3 : i64
      %q7_dep0_sel0 = arith.select %q7_dep0_w0, %nr0, %q7_dep0_sel1 : i64
      %q7_dep0_bits = arith.select %q7_dep0_w2, %nr2, %q7_dep0_sel0 : i64
      %q7_dep0_hit0 = arith.andi %q7_dep0_bits, %q7_dep0_bit : i64
      %q7_dep0_hit = arith.cmpi ne, %q7_dep0_hit0, %c0 : i64
      %q7_dep0_vbit = arith.constant 1 : i64
      %q7_dep0_vhit = arith.andi %q7_dvalid, %q7_dep0_vbit : i64
      %q7_dep0_used = arith.cmpi ne, %q7_dep0_vhit, %c0 : i64
      %q7_dep0_unused = arith.cmpi eq, %q7_dep0_used, %false : i1
      %q7_dep0_ready = arith.ori %q7_dep0_unused, %q7_dep0_hit : i1
      %q7_dep1_shift = arith.constant 19 : i64
      %q7_dep1_s = arith.shrui %q7_desc, %q7_dep1_shift : i64
      %q7_dep1 = arith.andi %q7_dep1_s, %mask255 : i64
      %q7_dep1_word = arith.shrui %q7_dep1, %c6 : i64
      %q7_dep1_off = arith.andi %q7_dep1, %mask63 : i64
      %q7_dep1_bit = arith.shli %c1, %q7_dep1_off : i64
      %q7_dep1_w0 = arith.cmpi eq, %q7_dep1_word, %c0 : i64
      %q7_dep1_w1 = arith.cmpi eq, %q7_dep1_word, %c1 : i64
      %q7_dep1_w2 = arith.cmpi eq, %q7_dep1_word, %c2 : i64
      %q7_dep1_sel1 = arith.select %q7_dep1_w1, %nr1, %nr3 : i64
      %q7_dep1_sel0 = arith.select %q7_dep1_w0, %nr0, %q7_dep1_sel1 : i64
      %q7_dep1_bits = arith.select %q7_dep1_w2, %nr2, %q7_dep1_sel0 : i64
      %q7_dep1_hit0 = arith.andi %q7_dep1_bits, %q7_dep1_bit : i64
      %q7_dep1_hit = arith.cmpi ne, %q7_dep1_hit0, %c0 : i64
      %q7_dep1_vbit = arith.constant 2 : i64
      %q7_dep1_vhit = arith.andi %q7_dvalid, %q7_dep1_vbit : i64
      %q7_dep1_used = arith.cmpi ne, %q7_dep1_vhit, %c0 : i64
      %q7_dep1_unused = arith.cmpi eq, %q7_dep1_used, %false : i1
      %q7_dep1_ready = arith.ori %q7_dep1_unused, %q7_dep1_hit : i1
      %q7_dep2_shift = arith.constant 27 : i64
      %q7_dep2_s = arith.shrui %q7_desc, %q7_dep2_shift : i64
      %q7_dep2 = arith.andi %q7_dep2_s, %mask255 : i64
      %q7_dep2_word = arith.shrui %q7_dep2, %c6 : i64
      %q7_dep2_off = arith.andi %q7_dep2, %mask63 : i64
      %q7_dep2_bit = arith.shli %c1, %q7_dep2_off : i64
      %q7_dep2_w0 = arith.cmpi eq, %q7_dep2_word, %c0 : i64
      %q7_dep2_w1 = arith.cmpi eq, %q7_dep2_word, %c1 : i64
      %q7_dep2_w2 = arith.cmpi eq, %q7_dep2_word, %c2 : i64
      %q7_dep2_sel1 = arith.select %q7_dep2_w1, %nr1, %nr3 : i64
      %q7_dep2_sel0 = arith.select %q7_dep2_w0, %nr0, %q7_dep2_sel1 : i64
      %q7_dep2_bits = arith.select %q7_dep2_w2, %nr2, %q7_dep2_sel0 : i64
      %q7_dep2_hit0 = arith.andi %q7_dep2_bits, %q7_dep2_bit : i64
      %q7_dep2_hit = arith.cmpi ne, %q7_dep2_hit0, %c0 : i64
      %q7_dep2_vbit = arith.constant 4 : i64
      %q7_dep2_vhit = arith.andi %q7_dvalid, %q7_dep2_vbit : i64
      %q7_dep2_used = arith.cmpi ne, %q7_dep2_vhit, %c0 : i64
      %q7_dep2_unused = arith.cmpi eq, %q7_dep2_used, %false : i1
      %q7_dep2_ready = arith.ori %q7_dep2_unused, %q7_dep2_hit : i1
      %q7_deps01 = arith.andi %q7_dep0_ready, %q7_dep1_ready : i1
      %q7_deps = arith.andi %q7_deps01, %q7_dep2_ready : i1
      %q7_vbit = arith.constant 128 : i64
      %q7_vhit = arith.andi %valid_bits, %q7_vbit : i64
      %q7_occupied = arith.cmpi ne, %q7_vhit, %c0 : i64
      %q7_eligible = arith.andi %q7_occupied, %q7_deps : i1
      %oldest_init = arith.constant 256 : i64
      %index_init = arith.constant 8 : i64
      %q0_older = arith.cmpi ult, %s0, %oldest_init : i64
      %q0_choose = arith.andi %q0_eligible, %q0_older : i1
      %oldest0 = arith.select %q0_choose, %s0, %oldest_init : i64
      %index0 = arith.select %q0_choose, %c0, %index_init : i64
      %q1_older = arith.cmpi ult, %s1, %oldest0 : i64
      %q1_choose = arith.andi %q1_eligible, %q1_older : i1
      %oldest1 = arith.select %q1_choose, %s1, %oldest0 : i64
      %index1 = arith.select %q1_choose, %c1, %index0 : i64
      %q2_older = arith.cmpi ult, %s2, %oldest1 : i64
      %q2_choose = arith.andi %q2_eligible, %q2_older : i1
      %oldest2 = arith.select %q2_choose, %s2, %oldest1 : i64
      %index2 = arith.select %q2_choose, %c2, %index1 : i64
      %q3_older = arith.cmpi ult, %s3, %oldest2 : i64
      %q3_choose = arith.andi %q3_eligible, %q3_older : i1
      %oldest3 = arith.select %q3_choose, %s3, %oldest2 : i64
      %index3 = arith.select %q3_choose, %c3, %index2 : i64
      %q4_older = arith.cmpi ult, %s4, %oldest3 : i64
      %q4_choose = arith.andi %q4_eligible, %q4_older : i1
      %oldest4 = arith.select %q4_choose, %s4, %oldest3 : i64
      %index4 = arith.select %q4_choose, %c4, %index3 : i64
      %q5_older = arith.cmpi ult, %s5, %oldest4 : i64
      %q5_choose = arith.andi %q5_eligible, %q5_older : i1
      %oldest5 = arith.select %q5_choose, %s5, %oldest4 : i64
      %index5 = arith.select %q5_choose, %c5, %index4 : i64
      %q6_older = arith.cmpi ult, %s6, %oldest5 : i64
      %q6_choose = arith.andi %q6_eligible, %q6_older : i1
      %oldest6 = arith.select %q6_choose, %s6, %oldest5 : i64
      %index6 = arith.select %q6_choose, %c6, %index5 : i64
      %q7_older = arith.cmpi ult, %s7, %oldest6 : i64
      %q7_choose = arith.andi %q7_eligible, %q7_older : i1
      %oldest7 = arith.select %q7_choose, %s7, %oldest6 : i64
      %index7 = arith.select %q7_choose, %c7, %index6 : i64
      %has_issue = arith.cmpi ne, %index7, %c8 : i64
      %issued = scf.if %has_issue -> i1 {
        %sent = ac.try_send @Core::@iq_to_eng_v %oldest7 : i64
        scf.yield %sent : i1
      } else {
        scf.yield %false : i1
      }
      %clear_mask0 = arith.constant -2 : i64
      %cleared0 = arith.andi %valid_bits, %clear_mask0 : i64
      %issued_slot0a = arith.cmpi eq, %index7, %c0 : i64
      %issued_slot0 = arith.andi %issued, %issued_slot0a : i1
      %va0 = arith.select %issued_slot0, %cleared0, %valid_bits : i64
      %clear_mask1 = arith.constant -3 : i64
      %cleared1 = arith.andi %va0, %clear_mask1 : i64
      %issued_slot1a = arith.cmpi eq, %index7, %c1 : i64
      %issued_slot1 = arith.andi %issued, %issued_slot1a : i1
      %va1 = arith.select %issued_slot1, %cleared1, %va0 : i64
      %clear_mask2 = arith.constant -5 : i64
      %cleared2 = arith.andi %va1, %clear_mask2 : i64
      %issued_slot2a = arith.cmpi eq, %index7, %c2 : i64
      %issued_slot2 = arith.andi %issued, %issued_slot2a : i1
      %va2 = arith.select %issued_slot2, %cleared2, %va1 : i64
      %clear_mask3 = arith.constant -9 : i64
      %cleared3 = arith.andi %va2, %clear_mask3 : i64
      %issued_slot3a = arith.cmpi eq, %index7, %c3 : i64
      %issued_slot3 = arith.andi %issued, %issued_slot3a : i1
      %va3 = arith.select %issued_slot3, %cleared3, %va2 : i64
      %clear_mask4 = arith.constant -17 : i64
      %cleared4 = arith.andi %va3, %clear_mask4 : i64
      %issued_slot4a = arith.cmpi eq, %index7, %c4 : i64
      %issued_slot4 = arith.andi %issued, %issued_slot4a : i1
      %va4 = arith.select %issued_slot4, %cleared4, %va3 : i64
      %clear_mask5 = arith.constant -33 : i64
      %cleared5 = arith.andi %va4, %clear_mask5 : i64
      %issued_slot5a = arith.cmpi eq, %index7, %c5 : i64
      %issued_slot5 = arith.andi %issued, %issued_slot5a : i1
      %va5 = arith.select %issued_slot5, %cleared5, %va4 : i64
      %clear_mask6 = arith.constant -65 : i64
      %cleared6 = arith.andi %va5, %clear_mask6 : i64
      %issued_slot6a = arith.cmpi eq, %index7, %c6 : i64
      %issued_slot6 = arith.andi %issued, %issued_slot6a : i1
      %va6 = arith.select %issued_slot6, %cleared6, %va5 : i64
      %clear_mask7 = arith.constant -129 : i64
      %cleared7 = arith.andi %va6, %clear_mask7 : i64
      %issued_slot7a = arith.cmpi eq, %index7, %c7 : i64
      %issued_slot7 = arith.andi %issued, %issued_slot7a : i1
      %va7 = arith.select %issued_slot7, %cleared7, %va6 : i64
      %full = arith.cmpi eq, %va7, %c255 : i64
      %not_full = arith.cmpi eq, %full, %false : i1
      %incoming, %has_incoming = scf.if %not_full -> (i64, i1) {
        %value, %ok = ac.try_recv @Core::@dispatch_to_iq_v : i64
        scf.yield %value, %ok : i64, i1
      } else {
        scf.yield %c0, %false : i64, i1
      }
      %ins_bit7 = arith.constant 128 : i64
      %ins_hit7 = arith.andi %va7, %ins_bit7 : i64
      %empty7 = arith.cmpi eq, %ins_hit7, %c0 : i64
      %ins7 = arith.select %empty7, %c7, %c8 : i64
      %ins_bit6 = arith.constant 64 : i64
      %ins_hit6 = arith.andi %va7, %ins_bit6 : i64
      %empty6 = arith.cmpi eq, %ins_hit6, %c0 : i64
      %ins6 = arith.select %empty6, %c6, %ins7 : i64
      %ins_bit5 = arith.constant 32 : i64
      %ins_hit5 = arith.andi %va7, %ins_bit5 : i64
      %empty5 = arith.cmpi eq, %ins_hit5, %c0 : i64
      %ins5 = arith.select %empty5, %c5, %ins6 : i64
      %ins_bit4 = arith.constant 16 : i64
      %ins_hit4 = arith.andi %va7, %ins_bit4 : i64
      %empty4 = arith.cmpi eq, %ins_hit4, %c0 : i64
      %ins4 = arith.select %empty4, %c4, %ins5 : i64
      %ins_bit3 = arith.constant 8 : i64
      %ins_hit3 = arith.andi %va7, %ins_bit3 : i64
      %empty3 = arith.cmpi eq, %ins_hit3, %c0 : i64
      %ins3 = arith.select %empty3, %c3, %ins4 : i64
      %ins_bit2 = arith.constant 4 : i64
      %ins_hit2 = arith.andi %va7, %ins_bit2 : i64
      %empty2 = arith.cmpi eq, %ins_hit2, %c0 : i64
      %ins2 = arith.select %empty2, %c2, %ins3 : i64
      %ins_bit1 = arith.constant 2 : i64
      %ins_hit1 = arith.andi %va7, %ins_bit1 : i64
      %empty1 = arith.cmpi eq, %ins_hit1, %c0 : i64
      %ins1 = arith.select %empty1, %c1, %ins2 : i64
      %ins_bit0 = arith.constant 1 : i64
      %ins_hit0 = arith.andi %va7, %ins_bit0 : i64
      %empty0 = arith.cmpi eq, %ins_hit0, %c0 : i64
      %ins0 = arith.select %empty0, %c0, %ins1 : i64
      %put0a = arith.cmpi eq, %ins0, %c0 : i64
      %put0 = arith.andi %has_incoming, %put0a : i1
      %ns0 = arith.select %put0, %incoming, %s0 : i64
      %put1a = arith.cmpi eq, %ins0, %c1 : i64
      %put1 = arith.andi %has_incoming, %put1a : i1
      %ns1 = arith.select %put1, %incoming, %s1 : i64
      %put2a = arith.cmpi eq, %ins0, %c2 : i64
      %put2 = arith.andi %has_incoming, %put2a : i1
      %ns2 = arith.select %put2, %incoming, %s2 : i64
      %put3a = arith.cmpi eq, %ins0, %c3 : i64
      %put3 = arith.andi %has_incoming, %put3a : i1
      %ns3 = arith.select %put3, %incoming, %s3 : i64
      %put4a = arith.cmpi eq, %ins0, %c4 : i64
      %put4 = arith.andi %has_incoming, %put4a : i1
      %ns4 = arith.select %put4, %incoming, %s4 : i64
      %put5a = arith.cmpi eq, %ins0, %c5 : i64
      %put5 = arith.andi %has_incoming, %put5a : i1
      %ns5 = arith.select %put5, %incoming, %s5 : i64
      %put6a = arith.cmpi eq, %ins0, %c6 : i64
      %put6 = arith.andi %has_incoming, %put6a : i1
      %ns6 = arith.select %put6, %incoming, %s6 : i64
      %put7a = arith.cmpi eq, %ins0, %c7 : i64
      %put7 = arith.andi %has_incoming, %put7a : i1
      %ns7 = arith.select %put7, %incoming, %s7 : i64
      %with0 = arith.ori %va7, %ins_bit0 : i64
      %nv0 = arith.select %put0, %with0, %va7 : i64
      %with1 = arith.ori %nv0, %ins_bit1 : i64
      %nv1 = arith.select %put1, %with1, %nv0 : i64
      %with2 = arith.ori %nv1, %ins_bit2 : i64
      %nv2 = arith.select %put2, %with2, %nv1 : i64
      %with3 = arith.ori %nv2, %ins_bit3 : i64
      %nv3 = arith.select %put3, %with3, %nv2 : i64
      %with4 = arith.ori %nv3, %ins_bit4 : i64
      %nv4 = arith.select %put4, %with4, %nv3 : i64
      %with5 = arith.ori %nv4, %ins_bit5 : i64
      %nv5 = arith.select %put5, %with5, %nv4 : i64
      %with6 = arith.ori %nv5, %ins_bit6 : i64
      %nv6 = arith.select %put6, %with6, %nv5 : i64
      %with7 = arith.ori %nv6, %ins_bit7 : i64
      %nv7 = arith.select %put7, %with7, %nv6 : i64
      %valid_stored = ac.try_send @valid %nv7 : i64
      %slot0_stored = ac.try_send @slot0 %ns0 : i64
      %slot1_stored = ac.try_send @slot1 %ns1 : i64
      %slot2_stored = ac.try_send @slot2 %ns2 : i64
      %slot3_stored = ac.try_send @slot3 %ns3 : i64
      %slot4_stored = ac.try_send @slot4 %ns4 : i64
      %slot5_stored = ac.try_send @slot5 %ns5 : i64
      %slot6_stored = ac.try_send @slot6 %ns6 : i64
      %slot7_stored = ac.try_send @slot7 %ns7 : i64
      %ready0_stored = ac.try_send @ready0 %nr0 : i64
      %ready1_stored = ac.try_send @ready1 %nr1 : i64
      %ready2_stored = ac.try_send @ready2 %nr2 : i64
      %ready3_stored = ac.try_send @ready3 %nr3 : i64
      ac.yield_sim
    }
    ac.return
  }

  ac.module @IssueQueueC() parameters {} graph {
    ac.queue @slot0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot0" path "slot0" watermarks {kind = "register"}
    ac.queue @slot1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot1" path "slot1" watermarks {kind = "register"}
    ac.queue @slot2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot2" path "slot2" watermarks {kind = "register"}
    ac.queue @slot3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot3" path "slot3" watermarks {kind = "register"}
    ac.queue @slot4 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot4" path "slot4" watermarks {kind = "register"}
    ac.queue @slot5 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot5" path "slot5" watermarks {kind = "register"}
    ac.queue @slot6 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot6" path "slot6" watermarks {kind = "register"}
    ac.queue @slot7 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot7" path "slot7" watermarks {kind = "register"}
    ac.queue @valid payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "valid" path "valid" watermarks {kind = "register"}
    ac.queue @ready0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready0" path "ready0" watermarks {kind = "register"}
    ac.queue @ready1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready1" path "ready1" watermarks {kind = "register"}
    ac.queue @ready2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready2" path "ready2" watermarks {kind = "register"}
    ac.queue @ready3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready3" path "ready3" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %c2 = arith.constant 2 : i64
      %c3 = arith.constant 3 : i64
      %c4 = arith.constant 4 : i64
      %c5 = arith.constant 5 : i64
      %c6 = arith.constant 6 : i64
      %c7 = arith.constant 7 : i64
      %c8 = arith.constant 8 : i64
      %c35 = arith.constant 35 : i64
      %c63 = arith.constant 63 : i64
      %c255 = arith.constant 255 : i64
      %mask7 = arith.constant 7 : i64
      %mask63 = arith.constant 63 : i64
      %mask255 = arith.constant 255 : i64
      %false = arith.constant false
      %valid_bits, %valid_ok = ac.try_recv @valid : i64
      %s0, %s0_ok = ac.try_recv @slot0 : i64
      %s1, %s1_ok = ac.try_recv @slot1 : i64
      %s2, %s2_ok = ac.try_recv @slot2 : i64
      %s3, %s3_ok = ac.try_recv @slot3 : i64
      %s4, %s4_ok = ac.try_recv @slot4 : i64
      %s5, %s5_ok = ac.try_recv @slot5 : i64
      %s6, %s6_ok = ac.try_recv @slot6 : i64
      %s7, %s7_ok = ac.try_recv @slot7 : i64
      %r0, %r0_ok = ac.try_recv @ready0 : i64
      %r1, %r1_ok = ac.try_recv @ready1 : i64
      %r2, %r2_ok = ac.try_recv @ready2 : i64
      %r3, %r3_ok = ac.try_recv @ready3 : i64
      %update, %has_update = ac.try_recv @Core::@ready_to_iq_c : i64
      %update_desc = ac.trace.decode %update : i64 to i64
      %update_seq = arith.andi %update_desc, %mask255 : i64
      %update_word = arith.shrui %update_seq, %c6 : i64
      %update_off = arith.andi %update_seq, %mask63 : i64
      %update_bit = arith.shli %c1, %update_off : i64
      %update_is0a = arith.cmpi eq, %update_word, %c0 : i64
      %update_is0 = arith.andi %has_update, %update_is0a : i1
      %r0_set = arith.ori %r0, %update_bit : i64
      %nr0 = arith.select %update_is0, %r0_set, %r0 : i64
      %update_is1a = arith.cmpi eq, %update_word, %c1 : i64
      %update_is1 = arith.andi %has_update, %update_is1a : i1
      %r1_set = arith.ori %r1, %update_bit : i64
      %nr1 = arith.select %update_is1, %r1_set, %r1 : i64
      %update_is2a = arith.cmpi eq, %update_word, %c2 : i64
      %update_is2 = arith.andi %has_update, %update_is2a : i1
      %r2_set = arith.ori %r2, %update_bit : i64
      %nr2 = arith.select %update_is2, %r2_set, %r2 : i64
      %update_is3a = arith.cmpi eq, %update_word, %c3 : i64
      %update_is3 = arith.andi %has_update, %update_is3a : i1
      %r3_set = arith.ori %r3, %update_bit : i64
      %nr3 = arith.select %update_is3, %r3_set, %r3 : i64
      %q0_desc = ac.trace.decode %s0 : i64 to i64
      %q0_dv_s = arith.shrui %q0_desc, %c35 : i64
      %q0_dvalid = arith.andi %q0_dv_s, %mask7 : i64
      %q0_dep0_shift = arith.constant 11 : i64
      %q0_dep0_s = arith.shrui %q0_desc, %q0_dep0_shift : i64
      %q0_dep0 = arith.andi %q0_dep0_s, %mask255 : i64
      %q0_dep0_word = arith.shrui %q0_dep0, %c6 : i64
      %q0_dep0_off = arith.andi %q0_dep0, %mask63 : i64
      %q0_dep0_bit = arith.shli %c1, %q0_dep0_off : i64
      %q0_dep0_w0 = arith.cmpi eq, %q0_dep0_word, %c0 : i64
      %q0_dep0_w1 = arith.cmpi eq, %q0_dep0_word, %c1 : i64
      %q0_dep0_w2 = arith.cmpi eq, %q0_dep0_word, %c2 : i64
      %q0_dep0_sel1 = arith.select %q0_dep0_w1, %nr1, %nr3 : i64
      %q0_dep0_sel0 = arith.select %q0_dep0_w0, %nr0, %q0_dep0_sel1 : i64
      %q0_dep0_bits = arith.select %q0_dep0_w2, %nr2, %q0_dep0_sel0 : i64
      %q0_dep0_hit0 = arith.andi %q0_dep0_bits, %q0_dep0_bit : i64
      %q0_dep0_hit = arith.cmpi ne, %q0_dep0_hit0, %c0 : i64
      %q0_dep0_vbit = arith.constant 1 : i64
      %q0_dep0_vhit = arith.andi %q0_dvalid, %q0_dep0_vbit : i64
      %q0_dep0_used = arith.cmpi ne, %q0_dep0_vhit, %c0 : i64
      %q0_dep0_unused = arith.cmpi eq, %q0_dep0_used, %false : i1
      %q0_dep0_ready = arith.ori %q0_dep0_unused, %q0_dep0_hit : i1
      %q0_dep1_shift = arith.constant 19 : i64
      %q0_dep1_s = arith.shrui %q0_desc, %q0_dep1_shift : i64
      %q0_dep1 = arith.andi %q0_dep1_s, %mask255 : i64
      %q0_dep1_word = arith.shrui %q0_dep1, %c6 : i64
      %q0_dep1_off = arith.andi %q0_dep1, %mask63 : i64
      %q0_dep1_bit = arith.shli %c1, %q0_dep1_off : i64
      %q0_dep1_w0 = arith.cmpi eq, %q0_dep1_word, %c0 : i64
      %q0_dep1_w1 = arith.cmpi eq, %q0_dep1_word, %c1 : i64
      %q0_dep1_w2 = arith.cmpi eq, %q0_dep1_word, %c2 : i64
      %q0_dep1_sel1 = arith.select %q0_dep1_w1, %nr1, %nr3 : i64
      %q0_dep1_sel0 = arith.select %q0_dep1_w0, %nr0, %q0_dep1_sel1 : i64
      %q0_dep1_bits = arith.select %q0_dep1_w2, %nr2, %q0_dep1_sel0 : i64
      %q0_dep1_hit0 = arith.andi %q0_dep1_bits, %q0_dep1_bit : i64
      %q0_dep1_hit = arith.cmpi ne, %q0_dep1_hit0, %c0 : i64
      %q0_dep1_vbit = arith.constant 2 : i64
      %q0_dep1_vhit = arith.andi %q0_dvalid, %q0_dep1_vbit : i64
      %q0_dep1_used = arith.cmpi ne, %q0_dep1_vhit, %c0 : i64
      %q0_dep1_unused = arith.cmpi eq, %q0_dep1_used, %false : i1
      %q0_dep1_ready = arith.ori %q0_dep1_unused, %q0_dep1_hit : i1
      %q0_dep2_shift = arith.constant 27 : i64
      %q0_dep2_s = arith.shrui %q0_desc, %q0_dep2_shift : i64
      %q0_dep2 = arith.andi %q0_dep2_s, %mask255 : i64
      %q0_dep2_word = arith.shrui %q0_dep2, %c6 : i64
      %q0_dep2_off = arith.andi %q0_dep2, %mask63 : i64
      %q0_dep2_bit = arith.shli %c1, %q0_dep2_off : i64
      %q0_dep2_w0 = arith.cmpi eq, %q0_dep2_word, %c0 : i64
      %q0_dep2_w1 = arith.cmpi eq, %q0_dep2_word, %c1 : i64
      %q0_dep2_w2 = arith.cmpi eq, %q0_dep2_word, %c2 : i64
      %q0_dep2_sel1 = arith.select %q0_dep2_w1, %nr1, %nr3 : i64
      %q0_dep2_sel0 = arith.select %q0_dep2_w0, %nr0, %q0_dep2_sel1 : i64
      %q0_dep2_bits = arith.select %q0_dep2_w2, %nr2, %q0_dep2_sel0 : i64
      %q0_dep2_hit0 = arith.andi %q0_dep2_bits, %q0_dep2_bit : i64
      %q0_dep2_hit = arith.cmpi ne, %q0_dep2_hit0, %c0 : i64
      %q0_dep2_vbit = arith.constant 4 : i64
      %q0_dep2_vhit = arith.andi %q0_dvalid, %q0_dep2_vbit : i64
      %q0_dep2_used = arith.cmpi ne, %q0_dep2_vhit, %c0 : i64
      %q0_dep2_unused = arith.cmpi eq, %q0_dep2_used, %false : i1
      %q0_dep2_ready = arith.ori %q0_dep2_unused, %q0_dep2_hit : i1
      %q0_deps01 = arith.andi %q0_dep0_ready, %q0_dep1_ready : i1
      %q0_deps = arith.andi %q0_deps01, %q0_dep2_ready : i1
      %q0_vbit = arith.constant 1 : i64
      %q0_vhit = arith.andi %valid_bits, %q0_vbit : i64
      %q0_occupied = arith.cmpi ne, %q0_vhit, %c0 : i64
      %q0_eligible = arith.andi %q0_occupied, %q0_deps : i1
      %q1_desc = ac.trace.decode %s1 : i64 to i64
      %q1_dv_s = arith.shrui %q1_desc, %c35 : i64
      %q1_dvalid = arith.andi %q1_dv_s, %mask7 : i64
      %q1_dep0_shift = arith.constant 11 : i64
      %q1_dep0_s = arith.shrui %q1_desc, %q1_dep0_shift : i64
      %q1_dep0 = arith.andi %q1_dep0_s, %mask255 : i64
      %q1_dep0_word = arith.shrui %q1_dep0, %c6 : i64
      %q1_dep0_off = arith.andi %q1_dep0, %mask63 : i64
      %q1_dep0_bit = arith.shli %c1, %q1_dep0_off : i64
      %q1_dep0_w0 = arith.cmpi eq, %q1_dep0_word, %c0 : i64
      %q1_dep0_w1 = arith.cmpi eq, %q1_dep0_word, %c1 : i64
      %q1_dep0_w2 = arith.cmpi eq, %q1_dep0_word, %c2 : i64
      %q1_dep0_sel1 = arith.select %q1_dep0_w1, %nr1, %nr3 : i64
      %q1_dep0_sel0 = arith.select %q1_dep0_w0, %nr0, %q1_dep0_sel1 : i64
      %q1_dep0_bits = arith.select %q1_dep0_w2, %nr2, %q1_dep0_sel0 : i64
      %q1_dep0_hit0 = arith.andi %q1_dep0_bits, %q1_dep0_bit : i64
      %q1_dep0_hit = arith.cmpi ne, %q1_dep0_hit0, %c0 : i64
      %q1_dep0_vbit = arith.constant 1 : i64
      %q1_dep0_vhit = arith.andi %q1_dvalid, %q1_dep0_vbit : i64
      %q1_dep0_used = arith.cmpi ne, %q1_dep0_vhit, %c0 : i64
      %q1_dep0_unused = arith.cmpi eq, %q1_dep0_used, %false : i1
      %q1_dep0_ready = arith.ori %q1_dep0_unused, %q1_dep0_hit : i1
      %q1_dep1_shift = arith.constant 19 : i64
      %q1_dep1_s = arith.shrui %q1_desc, %q1_dep1_shift : i64
      %q1_dep1 = arith.andi %q1_dep1_s, %mask255 : i64
      %q1_dep1_word = arith.shrui %q1_dep1, %c6 : i64
      %q1_dep1_off = arith.andi %q1_dep1, %mask63 : i64
      %q1_dep1_bit = arith.shli %c1, %q1_dep1_off : i64
      %q1_dep1_w0 = arith.cmpi eq, %q1_dep1_word, %c0 : i64
      %q1_dep1_w1 = arith.cmpi eq, %q1_dep1_word, %c1 : i64
      %q1_dep1_w2 = arith.cmpi eq, %q1_dep1_word, %c2 : i64
      %q1_dep1_sel1 = arith.select %q1_dep1_w1, %nr1, %nr3 : i64
      %q1_dep1_sel0 = arith.select %q1_dep1_w0, %nr0, %q1_dep1_sel1 : i64
      %q1_dep1_bits = arith.select %q1_dep1_w2, %nr2, %q1_dep1_sel0 : i64
      %q1_dep1_hit0 = arith.andi %q1_dep1_bits, %q1_dep1_bit : i64
      %q1_dep1_hit = arith.cmpi ne, %q1_dep1_hit0, %c0 : i64
      %q1_dep1_vbit = arith.constant 2 : i64
      %q1_dep1_vhit = arith.andi %q1_dvalid, %q1_dep1_vbit : i64
      %q1_dep1_used = arith.cmpi ne, %q1_dep1_vhit, %c0 : i64
      %q1_dep1_unused = arith.cmpi eq, %q1_dep1_used, %false : i1
      %q1_dep1_ready = arith.ori %q1_dep1_unused, %q1_dep1_hit : i1
      %q1_dep2_shift = arith.constant 27 : i64
      %q1_dep2_s = arith.shrui %q1_desc, %q1_dep2_shift : i64
      %q1_dep2 = arith.andi %q1_dep2_s, %mask255 : i64
      %q1_dep2_word = arith.shrui %q1_dep2, %c6 : i64
      %q1_dep2_off = arith.andi %q1_dep2, %mask63 : i64
      %q1_dep2_bit = arith.shli %c1, %q1_dep2_off : i64
      %q1_dep2_w0 = arith.cmpi eq, %q1_dep2_word, %c0 : i64
      %q1_dep2_w1 = arith.cmpi eq, %q1_dep2_word, %c1 : i64
      %q1_dep2_w2 = arith.cmpi eq, %q1_dep2_word, %c2 : i64
      %q1_dep2_sel1 = arith.select %q1_dep2_w1, %nr1, %nr3 : i64
      %q1_dep2_sel0 = arith.select %q1_dep2_w0, %nr0, %q1_dep2_sel1 : i64
      %q1_dep2_bits = arith.select %q1_dep2_w2, %nr2, %q1_dep2_sel0 : i64
      %q1_dep2_hit0 = arith.andi %q1_dep2_bits, %q1_dep2_bit : i64
      %q1_dep2_hit = arith.cmpi ne, %q1_dep2_hit0, %c0 : i64
      %q1_dep2_vbit = arith.constant 4 : i64
      %q1_dep2_vhit = arith.andi %q1_dvalid, %q1_dep2_vbit : i64
      %q1_dep2_used = arith.cmpi ne, %q1_dep2_vhit, %c0 : i64
      %q1_dep2_unused = arith.cmpi eq, %q1_dep2_used, %false : i1
      %q1_dep2_ready = arith.ori %q1_dep2_unused, %q1_dep2_hit : i1
      %q1_deps01 = arith.andi %q1_dep0_ready, %q1_dep1_ready : i1
      %q1_deps = arith.andi %q1_deps01, %q1_dep2_ready : i1
      %q1_vbit = arith.constant 2 : i64
      %q1_vhit = arith.andi %valid_bits, %q1_vbit : i64
      %q1_occupied = arith.cmpi ne, %q1_vhit, %c0 : i64
      %q1_eligible = arith.andi %q1_occupied, %q1_deps : i1
      %q2_desc = ac.trace.decode %s2 : i64 to i64
      %q2_dv_s = arith.shrui %q2_desc, %c35 : i64
      %q2_dvalid = arith.andi %q2_dv_s, %mask7 : i64
      %q2_dep0_shift = arith.constant 11 : i64
      %q2_dep0_s = arith.shrui %q2_desc, %q2_dep0_shift : i64
      %q2_dep0 = arith.andi %q2_dep0_s, %mask255 : i64
      %q2_dep0_word = arith.shrui %q2_dep0, %c6 : i64
      %q2_dep0_off = arith.andi %q2_dep0, %mask63 : i64
      %q2_dep0_bit = arith.shli %c1, %q2_dep0_off : i64
      %q2_dep0_w0 = arith.cmpi eq, %q2_dep0_word, %c0 : i64
      %q2_dep0_w1 = arith.cmpi eq, %q2_dep0_word, %c1 : i64
      %q2_dep0_w2 = arith.cmpi eq, %q2_dep0_word, %c2 : i64
      %q2_dep0_sel1 = arith.select %q2_dep0_w1, %nr1, %nr3 : i64
      %q2_dep0_sel0 = arith.select %q2_dep0_w0, %nr0, %q2_dep0_sel1 : i64
      %q2_dep0_bits = arith.select %q2_dep0_w2, %nr2, %q2_dep0_sel0 : i64
      %q2_dep0_hit0 = arith.andi %q2_dep0_bits, %q2_dep0_bit : i64
      %q2_dep0_hit = arith.cmpi ne, %q2_dep0_hit0, %c0 : i64
      %q2_dep0_vbit = arith.constant 1 : i64
      %q2_dep0_vhit = arith.andi %q2_dvalid, %q2_dep0_vbit : i64
      %q2_dep0_used = arith.cmpi ne, %q2_dep0_vhit, %c0 : i64
      %q2_dep0_unused = arith.cmpi eq, %q2_dep0_used, %false : i1
      %q2_dep0_ready = arith.ori %q2_dep0_unused, %q2_dep0_hit : i1
      %q2_dep1_shift = arith.constant 19 : i64
      %q2_dep1_s = arith.shrui %q2_desc, %q2_dep1_shift : i64
      %q2_dep1 = arith.andi %q2_dep1_s, %mask255 : i64
      %q2_dep1_word = arith.shrui %q2_dep1, %c6 : i64
      %q2_dep1_off = arith.andi %q2_dep1, %mask63 : i64
      %q2_dep1_bit = arith.shli %c1, %q2_dep1_off : i64
      %q2_dep1_w0 = arith.cmpi eq, %q2_dep1_word, %c0 : i64
      %q2_dep1_w1 = arith.cmpi eq, %q2_dep1_word, %c1 : i64
      %q2_dep1_w2 = arith.cmpi eq, %q2_dep1_word, %c2 : i64
      %q2_dep1_sel1 = arith.select %q2_dep1_w1, %nr1, %nr3 : i64
      %q2_dep1_sel0 = arith.select %q2_dep1_w0, %nr0, %q2_dep1_sel1 : i64
      %q2_dep1_bits = arith.select %q2_dep1_w2, %nr2, %q2_dep1_sel0 : i64
      %q2_dep1_hit0 = arith.andi %q2_dep1_bits, %q2_dep1_bit : i64
      %q2_dep1_hit = arith.cmpi ne, %q2_dep1_hit0, %c0 : i64
      %q2_dep1_vbit = arith.constant 2 : i64
      %q2_dep1_vhit = arith.andi %q2_dvalid, %q2_dep1_vbit : i64
      %q2_dep1_used = arith.cmpi ne, %q2_dep1_vhit, %c0 : i64
      %q2_dep1_unused = arith.cmpi eq, %q2_dep1_used, %false : i1
      %q2_dep1_ready = arith.ori %q2_dep1_unused, %q2_dep1_hit : i1
      %q2_dep2_shift = arith.constant 27 : i64
      %q2_dep2_s = arith.shrui %q2_desc, %q2_dep2_shift : i64
      %q2_dep2 = arith.andi %q2_dep2_s, %mask255 : i64
      %q2_dep2_word = arith.shrui %q2_dep2, %c6 : i64
      %q2_dep2_off = arith.andi %q2_dep2, %mask63 : i64
      %q2_dep2_bit = arith.shli %c1, %q2_dep2_off : i64
      %q2_dep2_w0 = arith.cmpi eq, %q2_dep2_word, %c0 : i64
      %q2_dep2_w1 = arith.cmpi eq, %q2_dep2_word, %c1 : i64
      %q2_dep2_w2 = arith.cmpi eq, %q2_dep2_word, %c2 : i64
      %q2_dep2_sel1 = arith.select %q2_dep2_w1, %nr1, %nr3 : i64
      %q2_dep2_sel0 = arith.select %q2_dep2_w0, %nr0, %q2_dep2_sel1 : i64
      %q2_dep2_bits = arith.select %q2_dep2_w2, %nr2, %q2_dep2_sel0 : i64
      %q2_dep2_hit0 = arith.andi %q2_dep2_bits, %q2_dep2_bit : i64
      %q2_dep2_hit = arith.cmpi ne, %q2_dep2_hit0, %c0 : i64
      %q2_dep2_vbit = arith.constant 4 : i64
      %q2_dep2_vhit = arith.andi %q2_dvalid, %q2_dep2_vbit : i64
      %q2_dep2_used = arith.cmpi ne, %q2_dep2_vhit, %c0 : i64
      %q2_dep2_unused = arith.cmpi eq, %q2_dep2_used, %false : i1
      %q2_dep2_ready = arith.ori %q2_dep2_unused, %q2_dep2_hit : i1
      %q2_deps01 = arith.andi %q2_dep0_ready, %q2_dep1_ready : i1
      %q2_deps = arith.andi %q2_deps01, %q2_dep2_ready : i1
      %q2_vbit = arith.constant 4 : i64
      %q2_vhit = arith.andi %valid_bits, %q2_vbit : i64
      %q2_occupied = arith.cmpi ne, %q2_vhit, %c0 : i64
      %q2_eligible = arith.andi %q2_occupied, %q2_deps : i1
      %q3_desc = ac.trace.decode %s3 : i64 to i64
      %q3_dv_s = arith.shrui %q3_desc, %c35 : i64
      %q3_dvalid = arith.andi %q3_dv_s, %mask7 : i64
      %q3_dep0_shift = arith.constant 11 : i64
      %q3_dep0_s = arith.shrui %q3_desc, %q3_dep0_shift : i64
      %q3_dep0 = arith.andi %q3_dep0_s, %mask255 : i64
      %q3_dep0_word = arith.shrui %q3_dep0, %c6 : i64
      %q3_dep0_off = arith.andi %q3_dep0, %mask63 : i64
      %q3_dep0_bit = arith.shli %c1, %q3_dep0_off : i64
      %q3_dep0_w0 = arith.cmpi eq, %q3_dep0_word, %c0 : i64
      %q3_dep0_w1 = arith.cmpi eq, %q3_dep0_word, %c1 : i64
      %q3_dep0_w2 = arith.cmpi eq, %q3_dep0_word, %c2 : i64
      %q3_dep0_sel1 = arith.select %q3_dep0_w1, %nr1, %nr3 : i64
      %q3_dep0_sel0 = arith.select %q3_dep0_w0, %nr0, %q3_dep0_sel1 : i64
      %q3_dep0_bits = arith.select %q3_dep0_w2, %nr2, %q3_dep0_sel0 : i64
      %q3_dep0_hit0 = arith.andi %q3_dep0_bits, %q3_dep0_bit : i64
      %q3_dep0_hit = arith.cmpi ne, %q3_dep0_hit0, %c0 : i64
      %q3_dep0_vbit = arith.constant 1 : i64
      %q3_dep0_vhit = arith.andi %q3_dvalid, %q3_dep0_vbit : i64
      %q3_dep0_used = arith.cmpi ne, %q3_dep0_vhit, %c0 : i64
      %q3_dep0_unused = arith.cmpi eq, %q3_dep0_used, %false : i1
      %q3_dep0_ready = arith.ori %q3_dep0_unused, %q3_dep0_hit : i1
      %q3_dep1_shift = arith.constant 19 : i64
      %q3_dep1_s = arith.shrui %q3_desc, %q3_dep1_shift : i64
      %q3_dep1 = arith.andi %q3_dep1_s, %mask255 : i64
      %q3_dep1_word = arith.shrui %q3_dep1, %c6 : i64
      %q3_dep1_off = arith.andi %q3_dep1, %mask63 : i64
      %q3_dep1_bit = arith.shli %c1, %q3_dep1_off : i64
      %q3_dep1_w0 = arith.cmpi eq, %q3_dep1_word, %c0 : i64
      %q3_dep1_w1 = arith.cmpi eq, %q3_dep1_word, %c1 : i64
      %q3_dep1_w2 = arith.cmpi eq, %q3_dep1_word, %c2 : i64
      %q3_dep1_sel1 = arith.select %q3_dep1_w1, %nr1, %nr3 : i64
      %q3_dep1_sel0 = arith.select %q3_dep1_w0, %nr0, %q3_dep1_sel1 : i64
      %q3_dep1_bits = arith.select %q3_dep1_w2, %nr2, %q3_dep1_sel0 : i64
      %q3_dep1_hit0 = arith.andi %q3_dep1_bits, %q3_dep1_bit : i64
      %q3_dep1_hit = arith.cmpi ne, %q3_dep1_hit0, %c0 : i64
      %q3_dep1_vbit = arith.constant 2 : i64
      %q3_dep1_vhit = arith.andi %q3_dvalid, %q3_dep1_vbit : i64
      %q3_dep1_used = arith.cmpi ne, %q3_dep1_vhit, %c0 : i64
      %q3_dep1_unused = arith.cmpi eq, %q3_dep1_used, %false : i1
      %q3_dep1_ready = arith.ori %q3_dep1_unused, %q3_dep1_hit : i1
      %q3_dep2_shift = arith.constant 27 : i64
      %q3_dep2_s = arith.shrui %q3_desc, %q3_dep2_shift : i64
      %q3_dep2 = arith.andi %q3_dep2_s, %mask255 : i64
      %q3_dep2_word = arith.shrui %q3_dep2, %c6 : i64
      %q3_dep2_off = arith.andi %q3_dep2, %mask63 : i64
      %q3_dep2_bit = arith.shli %c1, %q3_dep2_off : i64
      %q3_dep2_w0 = arith.cmpi eq, %q3_dep2_word, %c0 : i64
      %q3_dep2_w1 = arith.cmpi eq, %q3_dep2_word, %c1 : i64
      %q3_dep2_w2 = arith.cmpi eq, %q3_dep2_word, %c2 : i64
      %q3_dep2_sel1 = arith.select %q3_dep2_w1, %nr1, %nr3 : i64
      %q3_dep2_sel0 = arith.select %q3_dep2_w0, %nr0, %q3_dep2_sel1 : i64
      %q3_dep2_bits = arith.select %q3_dep2_w2, %nr2, %q3_dep2_sel0 : i64
      %q3_dep2_hit0 = arith.andi %q3_dep2_bits, %q3_dep2_bit : i64
      %q3_dep2_hit = arith.cmpi ne, %q3_dep2_hit0, %c0 : i64
      %q3_dep2_vbit = arith.constant 4 : i64
      %q3_dep2_vhit = arith.andi %q3_dvalid, %q3_dep2_vbit : i64
      %q3_dep2_used = arith.cmpi ne, %q3_dep2_vhit, %c0 : i64
      %q3_dep2_unused = arith.cmpi eq, %q3_dep2_used, %false : i1
      %q3_dep2_ready = arith.ori %q3_dep2_unused, %q3_dep2_hit : i1
      %q3_deps01 = arith.andi %q3_dep0_ready, %q3_dep1_ready : i1
      %q3_deps = arith.andi %q3_deps01, %q3_dep2_ready : i1
      %q3_vbit = arith.constant 8 : i64
      %q3_vhit = arith.andi %valid_bits, %q3_vbit : i64
      %q3_occupied = arith.cmpi ne, %q3_vhit, %c0 : i64
      %q3_eligible = arith.andi %q3_occupied, %q3_deps : i1
      %q4_desc = ac.trace.decode %s4 : i64 to i64
      %q4_dv_s = arith.shrui %q4_desc, %c35 : i64
      %q4_dvalid = arith.andi %q4_dv_s, %mask7 : i64
      %q4_dep0_shift = arith.constant 11 : i64
      %q4_dep0_s = arith.shrui %q4_desc, %q4_dep0_shift : i64
      %q4_dep0 = arith.andi %q4_dep0_s, %mask255 : i64
      %q4_dep0_word = arith.shrui %q4_dep0, %c6 : i64
      %q4_dep0_off = arith.andi %q4_dep0, %mask63 : i64
      %q4_dep0_bit = arith.shli %c1, %q4_dep0_off : i64
      %q4_dep0_w0 = arith.cmpi eq, %q4_dep0_word, %c0 : i64
      %q4_dep0_w1 = arith.cmpi eq, %q4_dep0_word, %c1 : i64
      %q4_dep0_w2 = arith.cmpi eq, %q4_dep0_word, %c2 : i64
      %q4_dep0_sel1 = arith.select %q4_dep0_w1, %nr1, %nr3 : i64
      %q4_dep0_sel0 = arith.select %q4_dep0_w0, %nr0, %q4_dep0_sel1 : i64
      %q4_dep0_bits = arith.select %q4_dep0_w2, %nr2, %q4_dep0_sel0 : i64
      %q4_dep0_hit0 = arith.andi %q4_dep0_bits, %q4_dep0_bit : i64
      %q4_dep0_hit = arith.cmpi ne, %q4_dep0_hit0, %c0 : i64
      %q4_dep0_vbit = arith.constant 1 : i64
      %q4_dep0_vhit = arith.andi %q4_dvalid, %q4_dep0_vbit : i64
      %q4_dep0_used = arith.cmpi ne, %q4_dep0_vhit, %c0 : i64
      %q4_dep0_unused = arith.cmpi eq, %q4_dep0_used, %false : i1
      %q4_dep0_ready = arith.ori %q4_dep0_unused, %q4_dep0_hit : i1
      %q4_dep1_shift = arith.constant 19 : i64
      %q4_dep1_s = arith.shrui %q4_desc, %q4_dep1_shift : i64
      %q4_dep1 = arith.andi %q4_dep1_s, %mask255 : i64
      %q4_dep1_word = arith.shrui %q4_dep1, %c6 : i64
      %q4_dep1_off = arith.andi %q4_dep1, %mask63 : i64
      %q4_dep1_bit = arith.shli %c1, %q4_dep1_off : i64
      %q4_dep1_w0 = arith.cmpi eq, %q4_dep1_word, %c0 : i64
      %q4_dep1_w1 = arith.cmpi eq, %q4_dep1_word, %c1 : i64
      %q4_dep1_w2 = arith.cmpi eq, %q4_dep1_word, %c2 : i64
      %q4_dep1_sel1 = arith.select %q4_dep1_w1, %nr1, %nr3 : i64
      %q4_dep1_sel0 = arith.select %q4_dep1_w0, %nr0, %q4_dep1_sel1 : i64
      %q4_dep1_bits = arith.select %q4_dep1_w2, %nr2, %q4_dep1_sel0 : i64
      %q4_dep1_hit0 = arith.andi %q4_dep1_bits, %q4_dep1_bit : i64
      %q4_dep1_hit = arith.cmpi ne, %q4_dep1_hit0, %c0 : i64
      %q4_dep1_vbit = arith.constant 2 : i64
      %q4_dep1_vhit = arith.andi %q4_dvalid, %q4_dep1_vbit : i64
      %q4_dep1_used = arith.cmpi ne, %q4_dep1_vhit, %c0 : i64
      %q4_dep1_unused = arith.cmpi eq, %q4_dep1_used, %false : i1
      %q4_dep1_ready = arith.ori %q4_dep1_unused, %q4_dep1_hit : i1
      %q4_dep2_shift = arith.constant 27 : i64
      %q4_dep2_s = arith.shrui %q4_desc, %q4_dep2_shift : i64
      %q4_dep2 = arith.andi %q4_dep2_s, %mask255 : i64
      %q4_dep2_word = arith.shrui %q4_dep2, %c6 : i64
      %q4_dep2_off = arith.andi %q4_dep2, %mask63 : i64
      %q4_dep2_bit = arith.shli %c1, %q4_dep2_off : i64
      %q4_dep2_w0 = arith.cmpi eq, %q4_dep2_word, %c0 : i64
      %q4_dep2_w1 = arith.cmpi eq, %q4_dep2_word, %c1 : i64
      %q4_dep2_w2 = arith.cmpi eq, %q4_dep2_word, %c2 : i64
      %q4_dep2_sel1 = arith.select %q4_dep2_w1, %nr1, %nr3 : i64
      %q4_dep2_sel0 = arith.select %q4_dep2_w0, %nr0, %q4_dep2_sel1 : i64
      %q4_dep2_bits = arith.select %q4_dep2_w2, %nr2, %q4_dep2_sel0 : i64
      %q4_dep2_hit0 = arith.andi %q4_dep2_bits, %q4_dep2_bit : i64
      %q4_dep2_hit = arith.cmpi ne, %q4_dep2_hit0, %c0 : i64
      %q4_dep2_vbit = arith.constant 4 : i64
      %q4_dep2_vhit = arith.andi %q4_dvalid, %q4_dep2_vbit : i64
      %q4_dep2_used = arith.cmpi ne, %q4_dep2_vhit, %c0 : i64
      %q4_dep2_unused = arith.cmpi eq, %q4_dep2_used, %false : i1
      %q4_dep2_ready = arith.ori %q4_dep2_unused, %q4_dep2_hit : i1
      %q4_deps01 = arith.andi %q4_dep0_ready, %q4_dep1_ready : i1
      %q4_deps = arith.andi %q4_deps01, %q4_dep2_ready : i1
      %q4_vbit = arith.constant 16 : i64
      %q4_vhit = arith.andi %valid_bits, %q4_vbit : i64
      %q4_occupied = arith.cmpi ne, %q4_vhit, %c0 : i64
      %q4_eligible = arith.andi %q4_occupied, %q4_deps : i1
      %q5_desc = ac.trace.decode %s5 : i64 to i64
      %q5_dv_s = arith.shrui %q5_desc, %c35 : i64
      %q5_dvalid = arith.andi %q5_dv_s, %mask7 : i64
      %q5_dep0_shift = arith.constant 11 : i64
      %q5_dep0_s = arith.shrui %q5_desc, %q5_dep0_shift : i64
      %q5_dep0 = arith.andi %q5_dep0_s, %mask255 : i64
      %q5_dep0_word = arith.shrui %q5_dep0, %c6 : i64
      %q5_dep0_off = arith.andi %q5_dep0, %mask63 : i64
      %q5_dep0_bit = arith.shli %c1, %q5_dep0_off : i64
      %q5_dep0_w0 = arith.cmpi eq, %q5_dep0_word, %c0 : i64
      %q5_dep0_w1 = arith.cmpi eq, %q5_dep0_word, %c1 : i64
      %q5_dep0_w2 = arith.cmpi eq, %q5_dep0_word, %c2 : i64
      %q5_dep0_sel1 = arith.select %q5_dep0_w1, %nr1, %nr3 : i64
      %q5_dep0_sel0 = arith.select %q5_dep0_w0, %nr0, %q5_dep0_sel1 : i64
      %q5_dep0_bits = arith.select %q5_dep0_w2, %nr2, %q5_dep0_sel0 : i64
      %q5_dep0_hit0 = arith.andi %q5_dep0_bits, %q5_dep0_bit : i64
      %q5_dep0_hit = arith.cmpi ne, %q5_dep0_hit0, %c0 : i64
      %q5_dep0_vbit = arith.constant 1 : i64
      %q5_dep0_vhit = arith.andi %q5_dvalid, %q5_dep0_vbit : i64
      %q5_dep0_used = arith.cmpi ne, %q5_dep0_vhit, %c0 : i64
      %q5_dep0_unused = arith.cmpi eq, %q5_dep0_used, %false : i1
      %q5_dep0_ready = arith.ori %q5_dep0_unused, %q5_dep0_hit : i1
      %q5_dep1_shift = arith.constant 19 : i64
      %q5_dep1_s = arith.shrui %q5_desc, %q5_dep1_shift : i64
      %q5_dep1 = arith.andi %q5_dep1_s, %mask255 : i64
      %q5_dep1_word = arith.shrui %q5_dep1, %c6 : i64
      %q5_dep1_off = arith.andi %q5_dep1, %mask63 : i64
      %q5_dep1_bit = arith.shli %c1, %q5_dep1_off : i64
      %q5_dep1_w0 = arith.cmpi eq, %q5_dep1_word, %c0 : i64
      %q5_dep1_w1 = arith.cmpi eq, %q5_dep1_word, %c1 : i64
      %q5_dep1_w2 = arith.cmpi eq, %q5_dep1_word, %c2 : i64
      %q5_dep1_sel1 = arith.select %q5_dep1_w1, %nr1, %nr3 : i64
      %q5_dep1_sel0 = arith.select %q5_dep1_w0, %nr0, %q5_dep1_sel1 : i64
      %q5_dep1_bits = arith.select %q5_dep1_w2, %nr2, %q5_dep1_sel0 : i64
      %q5_dep1_hit0 = arith.andi %q5_dep1_bits, %q5_dep1_bit : i64
      %q5_dep1_hit = arith.cmpi ne, %q5_dep1_hit0, %c0 : i64
      %q5_dep1_vbit = arith.constant 2 : i64
      %q5_dep1_vhit = arith.andi %q5_dvalid, %q5_dep1_vbit : i64
      %q5_dep1_used = arith.cmpi ne, %q5_dep1_vhit, %c0 : i64
      %q5_dep1_unused = arith.cmpi eq, %q5_dep1_used, %false : i1
      %q5_dep1_ready = arith.ori %q5_dep1_unused, %q5_dep1_hit : i1
      %q5_dep2_shift = arith.constant 27 : i64
      %q5_dep2_s = arith.shrui %q5_desc, %q5_dep2_shift : i64
      %q5_dep2 = arith.andi %q5_dep2_s, %mask255 : i64
      %q5_dep2_word = arith.shrui %q5_dep2, %c6 : i64
      %q5_dep2_off = arith.andi %q5_dep2, %mask63 : i64
      %q5_dep2_bit = arith.shli %c1, %q5_dep2_off : i64
      %q5_dep2_w0 = arith.cmpi eq, %q5_dep2_word, %c0 : i64
      %q5_dep2_w1 = arith.cmpi eq, %q5_dep2_word, %c1 : i64
      %q5_dep2_w2 = arith.cmpi eq, %q5_dep2_word, %c2 : i64
      %q5_dep2_sel1 = arith.select %q5_dep2_w1, %nr1, %nr3 : i64
      %q5_dep2_sel0 = arith.select %q5_dep2_w0, %nr0, %q5_dep2_sel1 : i64
      %q5_dep2_bits = arith.select %q5_dep2_w2, %nr2, %q5_dep2_sel0 : i64
      %q5_dep2_hit0 = arith.andi %q5_dep2_bits, %q5_dep2_bit : i64
      %q5_dep2_hit = arith.cmpi ne, %q5_dep2_hit0, %c0 : i64
      %q5_dep2_vbit = arith.constant 4 : i64
      %q5_dep2_vhit = arith.andi %q5_dvalid, %q5_dep2_vbit : i64
      %q5_dep2_used = arith.cmpi ne, %q5_dep2_vhit, %c0 : i64
      %q5_dep2_unused = arith.cmpi eq, %q5_dep2_used, %false : i1
      %q5_dep2_ready = arith.ori %q5_dep2_unused, %q5_dep2_hit : i1
      %q5_deps01 = arith.andi %q5_dep0_ready, %q5_dep1_ready : i1
      %q5_deps = arith.andi %q5_deps01, %q5_dep2_ready : i1
      %q5_vbit = arith.constant 32 : i64
      %q5_vhit = arith.andi %valid_bits, %q5_vbit : i64
      %q5_occupied = arith.cmpi ne, %q5_vhit, %c0 : i64
      %q5_eligible = arith.andi %q5_occupied, %q5_deps : i1
      %q6_desc = ac.trace.decode %s6 : i64 to i64
      %q6_dv_s = arith.shrui %q6_desc, %c35 : i64
      %q6_dvalid = arith.andi %q6_dv_s, %mask7 : i64
      %q6_dep0_shift = arith.constant 11 : i64
      %q6_dep0_s = arith.shrui %q6_desc, %q6_dep0_shift : i64
      %q6_dep0 = arith.andi %q6_dep0_s, %mask255 : i64
      %q6_dep0_word = arith.shrui %q6_dep0, %c6 : i64
      %q6_dep0_off = arith.andi %q6_dep0, %mask63 : i64
      %q6_dep0_bit = arith.shli %c1, %q6_dep0_off : i64
      %q6_dep0_w0 = arith.cmpi eq, %q6_dep0_word, %c0 : i64
      %q6_dep0_w1 = arith.cmpi eq, %q6_dep0_word, %c1 : i64
      %q6_dep0_w2 = arith.cmpi eq, %q6_dep0_word, %c2 : i64
      %q6_dep0_sel1 = arith.select %q6_dep0_w1, %nr1, %nr3 : i64
      %q6_dep0_sel0 = arith.select %q6_dep0_w0, %nr0, %q6_dep0_sel1 : i64
      %q6_dep0_bits = arith.select %q6_dep0_w2, %nr2, %q6_dep0_sel0 : i64
      %q6_dep0_hit0 = arith.andi %q6_dep0_bits, %q6_dep0_bit : i64
      %q6_dep0_hit = arith.cmpi ne, %q6_dep0_hit0, %c0 : i64
      %q6_dep0_vbit = arith.constant 1 : i64
      %q6_dep0_vhit = arith.andi %q6_dvalid, %q6_dep0_vbit : i64
      %q6_dep0_used = arith.cmpi ne, %q6_dep0_vhit, %c0 : i64
      %q6_dep0_unused = arith.cmpi eq, %q6_dep0_used, %false : i1
      %q6_dep0_ready = arith.ori %q6_dep0_unused, %q6_dep0_hit : i1
      %q6_dep1_shift = arith.constant 19 : i64
      %q6_dep1_s = arith.shrui %q6_desc, %q6_dep1_shift : i64
      %q6_dep1 = arith.andi %q6_dep1_s, %mask255 : i64
      %q6_dep1_word = arith.shrui %q6_dep1, %c6 : i64
      %q6_dep1_off = arith.andi %q6_dep1, %mask63 : i64
      %q6_dep1_bit = arith.shli %c1, %q6_dep1_off : i64
      %q6_dep1_w0 = arith.cmpi eq, %q6_dep1_word, %c0 : i64
      %q6_dep1_w1 = arith.cmpi eq, %q6_dep1_word, %c1 : i64
      %q6_dep1_w2 = arith.cmpi eq, %q6_dep1_word, %c2 : i64
      %q6_dep1_sel1 = arith.select %q6_dep1_w1, %nr1, %nr3 : i64
      %q6_dep1_sel0 = arith.select %q6_dep1_w0, %nr0, %q6_dep1_sel1 : i64
      %q6_dep1_bits = arith.select %q6_dep1_w2, %nr2, %q6_dep1_sel0 : i64
      %q6_dep1_hit0 = arith.andi %q6_dep1_bits, %q6_dep1_bit : i64
      %q6_dep1_hit = arith.cmpi ne, %q6_dep1_hit0, %c0 : i64
      %q6_dep1_vbit = arith.constant 2 : i64
      %q6_dep1_vhit = arith.andi %q6_dvalid, %q6_dep1_vbit : i64
      %q6_dep1_used = arith.cmpi ne, %q6_dep1_vhit, %c0 : i64
      %q6_dep1_unused = arith.cmpi eq, %q6_dep1_used, %false : i1
      %q6_dep1_ready = arith.ori %q6_dep1_unused, %q6_dep1_hit : i1
      %q6_dep2_shift = arith.constant 27 : i64
      %q6_dep2_s = arith.shrui %q6_desc, %q6_dep2_shift : i64
      %q6_dep2 = arith.andi %q6_dep2_s, %mask255 : i64
      %q6_dep2_word = arith.shrui %q6_dep2, %c6 : i64
      %q6_dep2_off = arith.andi %q6_dep2, %mask63 : i64
      %q6_dep2_bit = arith.shli %c1, %q6_dep2_off : i64
      %q6_dep2_w0 = arith.cmpi eq, %q6_dep2_word, %c0 : i64
      %q6_dep2_w1 = arith.cmpi eq, %q6_dep2_word, %c1 : i64
      %q6_dep2_w2 = arith.cmpi eq, %q6_dep2_word, %c2 : i64
      %q6_dep2_sel1 = arith.select %q6_dep2_w1, %nr1, %nr3 : i64
      %q6_dep2_sel0 = arith.select %q6_dep2_w0, %nr0, %q6_dep2_sel1 : i64
      %q6_dep2_bits = arith.select %q6_dep2_w2, %nr2, %q6_dep2_sel0 : i64
      %q6_dep2_hit0 = arith.andi %q6_dep2_bits, %q6_dep2_bit : i64
      %q6_dep2_hit = arith.cmpi ne, %q6_dep2_hit0, %c0 : i64
      %q6_dep2_vbit = arith.constant 4 : i64
      %q6_dep2_vhit = arith.andi %q6_dvalid, %q6_dep2_vbit : i64
      %q6_dep2_used = arith.cmpi ne, %q6_dep2_vhit, %c0 : i64
      %q6_dep2_unused = arith.cmpi eq, %q6_dep2_used, %false : i1
      %q6_dep2_ready = arith.ori %q6_dep2_unused, %q6_dep2_hit : i1
      %q6_deps01 = arith.andi %q6_dep0_ready, %q6_dep1_ready : i1
      %q6_deps = arith.andi %q6_deps01, %q6_dep2_ready : i1
      %q6_vbit = arith.constant 64 : i64
      %q6_vhit = arith.andi %valid_bits, %q6_vbit : i64
      %q6_occupied = arith.cmpi ne, %q6_vhit, %c0 : i64
      %q6_eligible = arith.andi %q6_occupied, %q6_deps : i1
      %q7_desc = ac.trace.decode %s7 : i64 to i64
      %q7_dv_s = arith.shrui %q7_desc, %c35 : i64
      %q7_dvalid = arith.andi %q7_dv_s, %mask7 : i64
      %q7_dep0_shift = arith.constant 11 : i64
      %q7_dep0_s = arith.shrui %q7_desc, %q7_dep0_shift : i64
      %q7_dep0 = arith.andi %q7_dep0_s, %mask255 : i64
      %q7_dep0_word = arith.shrui %q7_dep0, %c6 : i64
      %q7_dep0_off = arith.andi %q7_dep0, %mask63 : i64
      %q7_dep0_bit = arith.shli %c1, %q7_dep0_off : i64
      %q7_dep0_w0 = arith.cmpi eq, %q7_dep0_word, %c0 : i64
      %q7_dep0_w1 = arith.cmpi eq, %q7_dep0_word, %c1 : i64
      %q7_dep0_w2 = arith.cmpi eq, %q7_dep0_word, %c2 : i64
      %q7_dep0_sel1 = arith.select %q7_dep0_w1, %nr1, %nr3 : i64
      %q7_dep0_sel0 = arith.select %q7_dep0_w0, %nr0, %q7_dep0_sel1 : i64
      %q7_dep0_bits = arith.select %q7_dep0_w2, %nr2, %q7_dep0_sel0 : i64
      %q7_dep0_hit0 = arith.andi %q7_dep0_bits, %q7_dep0_bit : i64
      %q7_dep0_hit = arith.cmpi ne, %q7_dep0_hit0, %c0 : i64
      %q7_dep0_vbit = arith.constant 1 : i64
      %q7_dep0_vhit = arith.andi %q7_dvalid, %q7_dep0_vbit : i64
      %q7_dep0_used = arith.cmpi ne, %q7_dep0_vhit, %c0 : i64
      %q7_dep0_unused = arith.cmpi eq, %q7_dep0_used, %false : i1
      %q7_dep0_ready = arith.ori %q7_dep0_unused, %q7_dep0_hit : i1
      %q7_dep1_shift = arith.constant 19 : i64
      %q7_dep1_s = arith.shrui %q7_desc, %q7_dep1_shift : i64
      %q7_dep1 = arith.andi %q7_dep1_s, %mask255 : i64
      %q7_dep1_word = arith.shrui %q7_dep1, %c6 : i64
      %q7_dep1_off = arith.andi %q7_dep1, %mask63 : i64
      %q7_dep1_bit = arith.shli %c1, %q7_dep1_off : i64
      %q7_dep1_w0 = arith.cmpi eq, %q7_dep1_word, %c0 : i64
      %q7_dep1_w1 = arith.cmpi eq, %q7_dep1_word, %c1 : i64
      %q7_dep1_w2 = arith.cmpi eq, %q7_dep1_word, %c2 : i64
      %q7_dep1_sel1 = arith.select %q7_dep1_w1, %nr1, %nr3 : i64
      %q7_dep1_sel0 = arith.select %q7_dep1_w0, %nr0, %q7_dep1_sel1 : i64
      %q7_dep1_bits = arith.select %q7_dep1_w2, %nr2, %q7_dep1_sel0 : i64
      %q7_dep1_hit0 = arith.andi %q7_dep1_bits, %q7_dep1_bit : i64
      %q7_dep1_hit = arith.cmpi ne, %q7_dep1_hit0, %c0 : i64
      %q7_dep1_vbit = arith.constant 2 : i64
      %q7_dep1_vhit = arith.andi %q7_dvalid, %q7_dep1_vbit : i64
      %q7_dep1_used = arith.cmpi ne, %q7_dep1_vhit, %c0 : i64
      %q7_dep1_unused = arith.cmpi eq, %q7_dep1_used, %false : i1
      %q7_dep1_ready = arith.ori %q7_dep1_unused, %q7_dep1_hit : i1
      %q7_dep2_shift = arith.constant 27 : i64
      %q7_dep2_s = arith.shrui %q7_desc, %q7_dep2_shift : i64
      %q7_dep2 = arith.andi %q7_dep2_s, %mask255 : i64
      %q7_dep2_word = arith.shrui %q7_dep2, %c6 : i64
      %q7_dep2_off = arith.andi %q7_dep2, %mask63 : i64
      %q7_dep2_bit = arith.shli %c1, %q7_dep2_off : i64
      %q7_dep2_w0 = arith.cmpi eq, %q7_dep2_word, %c0 : i64
      %q7_dep2_w1 = arith.cmpi eq, %q7_dep2_word, %c1 : i64
      %q7_dep2_w2 = arith.cmpi eq, %q7_dep2_word, %c2 : i64
      %q7_dep2_sel1 = arith.select %q7_dep2_w1, %nr1, %nr3 : i64
      %q7_dep2_sel0 = arith.select %q7_dep2_w0, %nr0, %q7_dep2_sel1 : i64
      %q7_dep2_bits = arith.select %q7_dep2_w2, %nr2, %q7_dep2_sel0 : i64
      %q7_dep2_hit0 = arith.andi %q7_dep2_bits, %q7_dep2_bit : i64
      %q7_dep2_hit = arith.cmpi ne, %q7_dep2_hit0, %c0 : i64
      %q7_dep2_vbit = arith.constant 4 : i64
      %q7_dep2_vhit = arith.andi %q7_dvalid, %q7_dep2_vbit : i64
      %q7_dep2_used = arith.cmpi ne, %q7_dep2_vhit, %c0 : i64
      %q7_dep2_unused = arith.cmpi eq, %q7_dep2_used, %false : i1
      %q7_dep2_ready = arith.ori %q7_dep2_unused, %q7_dep2_hit : i1
      %q7_deps01 = arith.andi %q7_dep0_ready, %q7_dep1_ready : i1
      %q7_deps = arith.andi %q7_deps01, %q7_dep2_ready : i1
      %q7_vbit = arith.constant 128 : i64
      %q7_vhit = arith.andi %valid_bits, %q7_vbit : i64
      %q7_occupied = arith.cmpi ne, %q7_vhit, %c0 : i64
      %q7_eligible = arith.andi %q7_occupied, %q7_deps : i1
      %oldest_init = arith.constant 256 : i64
      %index_init = arith.constant 8 : i64
      %q0_older = arith.cmpi ult, %s0, %oldest_init : i64
      %q0_choose = arith.andi %q0_eligible, %q0_older : i1
      %oldest0 = arith.select %q0_choose, %s0, %oldest_init : i64
      %index0 = arith.select %q0_choose, %c0, %index_init : i64
      %q1_older = arith.cmpi ult, %s1, %oldest0 : i64
      %q1_choose = arith.andi %q1_eligible, %q1_older : i1
      %oldest1 = arith.select %q1_choose, %s1, %oldest0 : i64
      %index1 = arith.select %q1_choose, %c1, %index0 : i64
      %q2_older = arith.cmpi ult, %s2, %oldest1 : i64
      %q2_choose = arith.andi %q2_eligible, %q2_older : i1
      %oldest2 = arith.select %q2_choose, %s2, %oldest1 : i64
      %index2 = arith.select %q2_choose, %c2, %index1 : i64
      %q3_older = arith.cmpi ult, %s3, %oldest2 : i64
      %q3_choose = arith.andi %q3_eligible, %q3_older : i1
      %oldest3 = arith.select %q3_choose, %s3, %oldest2 : i64
      %index3 = arith.select %q3_choose, %c3, %index2 : i64
      %q4_older = arith.cmpi ult, %s4, %oldest3 : i64
      %q4_choose = arith.andi %q4_eligible, %q4_older : i1
      %oldest4 = arith.select %q4_choose, %s4, %oldest3 : i64
      %index4 = arith.select %q4_choose, %c4, %index3 : i64
      %q5_older = arith.cmpi ult, %s5, %oldest4 : i64
      %q5_choose = arith.andi %q5_eligible, %q5_older : i1
      %oldest5 = arith.select %q5_choose, %s5, %oldest4 : i64
      %index5 = arith.select %q5_choose, %c5, %index4 : i64
      %q6_older = arith.cmpi ult, %s6, %oldest5 : i64
      %q6_choose = arith.andi %q6_eligible, %q6_older : i1
      %oldest6 = arith.select %q6_choose, %s6, %oldest5 : i64
      %index6 = arith.select %q6_choose, %c6, %index5 : i64
      %q7_older = arith.cmpi ult, %s7, %oldest6 : i64
      %q7_choose = arith.andi %q7_eligible, %q7_older : i1
      %oldest7 = arith.select %q7_choose, %s7, %oldest6 : i64
      %index7 = arith.select %q7_choose, %c7, %index6 : i64
      %has_issue = arith.cmpi ne, %index7, %c8 : i64
      %issued = scf.if %has_issue -> i1 {
        %sent = ac.try_send @Core::@iq_to_eng_c %oldest7 : i64
        scf.yield %sent : i1
      } else {
        scf.yield %false : i1
      }
      %clear_mask0 = arith.constant -2 : i64
      %cleared0 = arith.andi %valid_bits, %clear_mask0 : i64
      %issued_slot0a = arith.cmpi eq, %index7, %c0 : i64
      %issued_slot0 = arith.andi %issued, %issued_slot0a : i1
      %va0 = arith.select %issued_slot0, %cleared0, %valid_bits : i64
      %clear_mask1 = arith.constant -3 : i64
      %cleared1 = arith.andi %va0, %clear_mask1 : i64
      %issued_slot1a = arith.cmpi eq, %index7, %c1 : i64
      %issued_slot1 = arith.andi %issued, %issued_slot1a : i1
      %va1 = arith.select %issued_slot1, %cleared1, %va0 : i64
      %clear_mask2 = arith.constant -5 : i64
      %cleared2 = arith.andi %va1, %clear_mask2 : i64
      %issued_slot2a = arith.cmpi eq, %index7, %c2 : i64
      %issued_slot2 = arith.andi %issued, %issued_slot2a : i1
      %va2 = arith.select %issued_slot2, %cleared2, %va1 : i64
      %clear_mask3 = arith.constant -9 : i64
      %cleared3 = arith.andi %va2, %clear_mask3 : i64
      %issued_slot3a = arith.cmpi eq, %index7, %c3 : i64
      %issued_slot3 = arith.andi %issued, %issued_slot3a : i1
      %va3 = arith.select %issued_slot3, %cleared3, %va2 : i64
      %clear_mask4 = arith.constant -17 : i64
      %cleared4 = arith.andi %va3, %clear_mask4 : i64
      %issued_slot4a = arith.cmpi eq, %index7, %c4 : i64
      %issued_slot4 = arith.andi %issued, %issued_slot4a : i1
      %va4 = arith.select %issued_slot4, %cleared4, %va3 : i64
      %clear_mask5 = arith.constant -33 : i64
      %cleared5 = arith.andi %va4, %clear_mask5 : i64
      %issued_slot5a = arith.cmpi eq, %index7, %c5 : i64
      %issued_slot5 = arith.andi %issued, %issued_slot5a : i1
      %va5 = arith.select %issued_slot5, %cleared5, %va4 : i64
      %clear_mask6 = arith.constant -65 : i64
      %cleared6 = arith.andi %va5, %clear_mask6 : i64
      %issued_slot6a = arith.cmpi eq, %index7, %c6 : i64
      %issued_slot6 = arith.andi %issued, %issued_slot6a : i1
      %va6 = arith.select %issued_slot6, %cleared6, %va5 : i64
      %clear_mask7 = arith.constant -129 : i64
      %cleared7 = arith.andi %va6, %clear_mask7 : i64
      %issued_slot7a = arith.cmpi eq, %index7, %c7 : i64
      %issued_slot7 = arith.andi %issued, %issued_slot7a : i1
      %va7 = arith.select %issued_slot7, %cleared7, %va6 : i64
      %full = arith.cmpi eq, %va7, %c255 : i64
      %not_full = arith.cmpi eq, %full, %false : i1
      %incoming, %has_incoming = scf.if %not_full -> (i64, i1) {
        %value, %ok = ac.try_recv @Core::@dispatch_to_iq_c : i64
        scf.yield %value, %ok : i64, i1
      } else {
        scf.yield %c0, %false : i64, i1
      }
      %ins_bit7 = arith.constant 128 : i64
      %ins_hit7 = arith.andi %va7, %ins_bit7 : i64
      %empty7 = arith.cmpi eq, %ins_hit7, %c0 : i64
      %ins7 = arith.select %empty7, %c7, %c8 : i64
      %ins_bit6 = arith.constant 64 : i64
      %ins_hit6 = arith.andi %va7, %ins_bit6 : i64
      %empty6 = arith.cmpi eq, %ins_hit6, %c0 : i64
      %ins6 = arith.select %empty6, %c6, %ins7 : i64
      %ins_bit5 = arith.constant 32 : i64
      %ins_hit5 = arith.andi %va7, %ins_bit5 : i64
      %empty5 = arith.cmpi eq, %ins_hit5, %c0 : i64
      %ins5 = arith.select %empty5, %c5, %ins6 : i64
      %ins_bit4 = arith.constant 16 : i64
      %ins_hit4 = arith.andi %va7, %ins_bit4 : i64
      %empty4 = arith.cmpi eq, %ins_hit4, %c0 : i64
      %ins4 = arith.select %empty4, %c4, %ins5 : i64
      %ins_bit3 = arith.constant 8 : i64
      %ins_hit3 = arith.andi %va7, %ins_bit3 : i64
      %empty3 = arith.cmpi eq, %ins_hit3, %c0 : i64
      %ins3 = arith.select %empty3, %c3, %ins4 : i64
      %ins_bit2 = arith.constant 4 : i64
      %ins_hit2 = arith.andi %va7, %ins_bit2 : i64
      %empty2 = arith.cmpi eq, %ins_hit2, %c0 : i64
      %ins2 = arith.select %empty2, %c2, %ins3 : i64
      %ins_bit1 = arith.constant 2 : i64
      %ins_hit1 = arith.andi %va7, %ins_bit1 : i64
      %empty1 = arith.cmpi eq, %ins_hit1, %c0 : i64
      %ins1 = arith.select %empty1, %c1, %ins2 : i64
      %ins_bit0 = arith.constant 1 : i64
      %ins_hit0 = arith.andi %va7, %ins_bit0 : i64
      %empty0 = arith.cmpi eq, %ins_hit0, %c0 : i64
      %ins0 = arith.select %empty0, %c0, %ins1 : i64
      %put0a = arith.cmpi eq, %ins0, %c0 : i64
      %put0 = arith.andi %has_incoming, %put0a : i1
      %ns0 = arith.select %put0, %incoming, %s0 : i64
      %put1a = arith.cmpi eq, %ins0, %c1 : i64
      %put1 = arith.andi %has_incoming, %put1a : i1
      %ns1 = arith.select %put1, %incoming, %s1 : i64
      %put2a = arith.cmpi eq, %ins0, %c2 : i64
      %put2 = arith.andi %has_incoming, %put2a : i1
      %ns2 = arith.select %put2, %incoming, %s2 : i64
      %put3a = arith.cmpi eq, %ins0, %c3 : i64
      %put3 = arith.andi %has_incoming, %put3a : i1
      %ns3 = arith.select %put3, %incoming, %s3 : i64
      %put4a = arith.cmpi eq, %ins0, %c4 : i64
      %put4 = arith.andi %has_incoming, %put4a : i1
      %ns4 = arith.select %put4, %incoming, %s4 : i64
      %put5a = arith.cmpi eq, %ins0, %c5 : i64
      %put5 = arith.andi %has_incoming, %put5a : i1
      %ns5 = arith.select %put5, %incoming, %s5 : i64
      %put6a = arith.cmpi eq, %ins0, %c6 : i64
      %put6 = arith.andi %has_incoming, %put6a : i1
      %ns6 = arith.select %put6, %incoming, %s6 : i64
      %put7a = arith.cmpi eq, %ins0, %c7 : i64
      %put7 = arith.andi %has_incoming, %put7a : i1
      %ns7 = arith.select %put7, %incoming, %s7 : i64
      %with0 = arith.ori %va7, %ins_bit0 : i64
      %nv0 = arith.select %put0, %with0, %va7 : i64
      %with1 = arith.ori %nv0, %ins_bit1 : i64
      %nv1 = arith.select %put1, %with1, %nv0 : i64
      %with2 = arith.ori %nv1, %ins_bit2 : i64
      %nv2 = arith.select %put2, %with2, %nv1 : i64
      %with3 = arith.ori %nv2, %ins_bit3 : i64
      %nv3 = arith.select %put3, %with3, %nv2 : i64
      %with4 = arith.ori %nv3, %ins_bit4 : i64
      %nv4 = arith.select %put4, %with4, %nv3 : i64
      %with5 = arith.ori %nv4, %ins_bit5 : i64
      %nv5 = arith.select %put5, %with5, %nv4 : i64
      %with6 = arith.ori %nv5, %ins_bit6 : i64
      %nv6 = arith.select %put6, %with6, %nv5 : i64
      %with7 = arith.ori %nv6, %ins_bit7 : i64
      %nv7 = arith.select %put7, %with7, %nv6 : i64
      %valid_stored = ac.try_send @valid %nv7 : i64
      %slot0_stored = ac.try_send @slot0 %ns0 : i64
      %slot1_stored = ac.try_send @slot1 %ns1 : i64
      %slot2_stored = ac.try_send @slot2 %ns2 : i64
      %slot3_stored = ac.try_send @slot3 %ns3 : i64
      %slot4_stored = ac.try_send @slot4 %ns4 : i64
      %slot5_stored = ac.try_send @slot5 %ns5 : i64
      %slot6_stored = ac.try_send @slot6 %ns6 : i64
      %slot7_stored = ac.try_send @slot7 %ns7 : i64
      %ready0_stored = ac.try_send @ready0 %nr0 : i64
      %ready1_stored = ac.try_send @ready1 %nr1 : i64
      %ready2_stored = ac.try_send @ready2 %nr2 : i64
      %ready3_stored = ac.try_send @ready3 %nr3 : i64
      ac.yield_sim
    }
    ac.return
  }

  ac.module @IssueQueueT() parameters {} graph {
    ac.queue @slot0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot0" path "slot0" watermarks {kind = "register"}
    ac.queue @slot1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot1" path "slot1" watermarks {kind = "register"}
    ac.queue @slot2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot2" path "slot2" watermarks {kind = "register"}
    ac.queue @slot3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot3" path "slot3" watermarks {kind = "register"}
    ac.queue @slot4 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot4" path "slot4" watermarks {kind = "register"}
    ac.queue @slot5 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot5" path "slot5" watermarks {kind = "register"}
    ac.queue @slot6 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot6" path "slot6" watermarks {kind = "register"}
    ac.queue @slot7 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "slot7" path "slot7" watermarks {kind = "register"}
    ac.queue @valid payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "valid" path "valid" watermarks {kind = "register"}
    ac.queue @ready0 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready0" path "ready0" watermarks {kind = "register"}
    ac.queue @ready1 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready1" path "ready1" watermarks {kind = "register"}
    ac.queue @ready2 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready2" path "ready2" watermarks {kind = "register"}
    ac.queue @ready3 payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready3" path "ready3" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %c2 = arith.constant 2 : i64
      %c3 = arith.constant 3 : i64
      %c4 = arith.constant 4 : i64
      %c5 = arith.constant 5 : i64
      %c6 = arith.constant 6 : i64
      %c7 = arith.constant 7 : i64
      %c8 = arith.constant 8 : i64
      %c35 = arith.constant 35 : i64
      %c63 = arith.constant 63 : i64
      %c255 = arith.constant 255 : i64
      %mask7 = arith.constant 7 : i64
      %mask63 = arith.constant 63 : i64
      %mask255 = arith.constant 255 : i64
      %false = arith.constant false
      %valid_bits, %valid_ok = ac.try_recv @valid : i64
      %s0, %s0_ok = ac.try_recv @slot0 : i64
      %s1, %s1_ok = ac.try_recv @slot1 : i64
      %s2, %s2_ok = ac.try_recv @slot2 : i64
      %s3, %s3_ok = ac.try_recv @slot3 : i64
      %s4, %s4_ok = ac.try_recv @slot4 : i64
      %s5, %s5_ok = ac.try_recv @slot5 : i64
      %s6, %s6_ok = ac.try_recv @slot6 : i64
      %s7, %s7_ok = ac.try_recv @slot7 : i64
      %r0, %r0_ok = ac.try_recv @ready0 : i64
      %r1, %r1_ok = ac.try_recv @ready1 : i64
      %r2, %r2_ok = ac.try_recv @ready2 : i64
      %r3, %r3_ok = ac.try_recv @ready3 : i64
      %update, %has_update = ac.try_recv @Core::@ready_to_iq_t : i64
      %update_desc = ac.trace.decode %update : i64 to i64
      %update_seq = arith.andi %update_desc, %mask255 : i64
      %update_word = arith.shrui %update_seq, %c6 : i64
      %update_off = arith.andi %update_seq, %mask63 : i64
      %update_bit = arith.shli %c1, %update_off : i64
      %update_is0a = arith.cmpi eq, %update_word, %c0 : i64
      %update_is0 = arith.andi %has_update, %update_is0a : i1
      %r0_set = arith.ori %r0, %update_bit : i64
      %nr0 = arith.select %update_is0, %r0_set, %r0 : i64
      %update_is1a = arith.cmpi eq, %update_word, %c1 : i64
      %update_is1 = arith.andi %has_update, %update_is1a : i1
      %r1_set = arith.ori %r1, %update_bit : i64
      %nr1 = arith.select %update_is1, %r1_set, %r1 : i64
      %update_is2a = arith.cmpi eq, %update_word, %c2 : i64
      %update_is2 = arith.andi %has_update, %update_is2a : i1
      %r2_set = arith.ori %r2, %update_bit : i64
      %nr2 = arith.select %update_is2, %r2_set, %r2 : i64
      %update_is3a = arith.cmpi eq, %update_word, %c3 : i64
      %update_is3 = arith.andi %has_update, %update_is3a : i1
      %r3_set = arith.ori %r3, %update_bit : i64
      %nr3 = arith.select %update_is3, %r3_set, %r3 : i64
      %q0_desc = ac.trace.decode %s0 : i64 to i64
      %q0_dv_s = arith.shrui %q0_desc, %c35 : i64
      %q0_dvalid = arith.andi %q0_dv_s, %mask7 : i64
      %q0_dep0_shift = arith.constant 11 : i64
      %q0_dep0_s = arith.shrui %q0_desc, %q0_dep0_shift : i64
      %q0_dep0 = arith.andi %q0_dep0_s, %mask255 : i64
      %q0_dep0_word = arith.shrui %q0_dep0, %c6 : i64
      %q0_dep0_off = arith.andi %q0_dep0, %mask63 : i64
      %q0_dep0_bit = arith.shli %c1, %q0_dep0_off : i64
      %q0_dep0_w0 = arith.cmpi eq, %q0_dep0_word, %c0 : i64
      %q0_dep0_w1 = arith.cmpi eq, %q0_dep0_word, %c1 : i64
      %q0_dep0_w2 = arith.cmpi eq, %q0_dep0_word, %c2 : i64
      %q0_dep0_sel1 = arith.select %q0_dep0_w1, %nr1, %nr3 : i64
      %q0_dep0_sel0 = arith.select %q0_dep0_w0, %nr0, %q0_dep0_sel1 : i64
      %q0_dep0_bits = arith.select %q0_dep0_w2, %nr2, %q0_dep0_sel0 : i64
      %q0_dep0_hit0 = arith.andi %q0_dep0_bits, %q0_dep0_bit : i64
      %q0_dep0_hit = arith.cmpi ne, %q0_dep0_hit0, %c0 : i64
      %q0_dep0_vbit = arith.constant 1 : i64
      %q0_dep0_vhit = arith.andi %q0_dvalid, %q0_dep0_vbit : i64
      %q0_dep0_used = arith.cmpi ne, %q0_dep0_vhit, %c0 : i64
      %q0_dep0_unused = arith.cmpi eq, %q0_dep0_used, %false : i1
      %q0_dep0_ready = arith.ori %q0_dep0_unused, %q0_dep0_hit : i1
      %q0_dep1_shift = arith.constant 19 : i64
      %q0_dep1_s = arith.shrui %q0_desc, %q0_dep1_shift : i64
      %q0_dep1 = arith.andi %q0_dep1_s, %mask255 : i64
      %q0_dep1_word = arith.shrui %q0_dep1, %c6 : i64
      %q0_dep1_off = arith.andi %q0_dep1, %mask63 : i64
      %q0_dep1_bit = arith.shli %c1, %q0_dep1_off : i64
      %q0_dep1_w0 = arith.cmpi eq, %q0_dep1_word, %c0 : i64
      %q0_dep1_w1 = arith.cmpi eq, %q0_dep1_word, %c1 : i64
      %q0_dep1_w2 = arith.cmpi eq, %q0_dep1_word, %c2 : i64
      %q0_dep1_sel1 = arith.select %q0_dep1_w1, %nr1, %nr3 : i64
      %q0_dep1_sel0 = arith.select %q0_dep1_w0, %nr0, %q0_dep1_sel1 : i64
      %q0_dep1_bits = arith.select %q0_dep1_w2, %nr2, %q0_dep1_sel0 : i64
      %q0_dep1_hit0 = arith.andi %q0_dep1_bits, %q0_dep1_bit : i64
      %q0_dep1_hit = arith.cmpi ne, %q0_dep1_hit0, %c0 : i64
      %q0_dep1_vbit = arith.constant 2 : i64
      %q0_dep1_vhit = arith.andi %q0_dvalid, %q0_dep1_vbit : i64
      %q0_dep1_used = arith.cmpi ne, %q0_dep1_vhit, %c0 : i64
      %q0_dep1_unused = arith.cmpi eq, %q0_dep1_used, %false : i1
      %q0_dep1_ready = arith.ori %q0_dep1_unused, %q0_dep1_hit : i1
      %q0_dep2_shift = arith.constant 27 : i64
      %q0_dep2_s = arith.shrui %q0_desc, %q0_dep2_shift : i64
      %q0_dep2 = arith.andi %q0_dep2_s, %mask255 : i64
      %q0_dep2_word = arith.shrui %q0_dep2, %c6 : i64
      %q0_dep2_off = arith.andi %q0_dep2, %mask63 : i64
      %q0_dep2_bit = arith.shli %c1, %q0_dep2_off : i64
      %q0_dep2_w0 = arith.cmpi eq, %q0_dep2_word, %c0 : i64
      %q0_dep2_w1 = arith.cmpi eq, %q0_dep2_word, %c1 : i64
      %q0_dep2_w2 = arith.cmpi eq, %q0_dep2_word, %c2 : i64
      %q0_dep2_sel1 = arith.select %q0_dep2_w1, %nr1, %nr3 : i64
      %q0_dep2_sel0 = arith.select %q0_dep2_w0, %nr0, %q0_dep2_sel1 : i64
      %q0_dep2_bits = arith.select %q0_dep2_w2, %nr2, %q0_dep2_sel0 : i64
      %q0_dep2_hit0 = arith.andi %q0_dep2_bits, %q0_dep2_bit : i64
      %q0_dep2_hit = arith.cmpi ne, %q0_dep2_hit0, %c0 : i64
      %q0_dep2_vbit = arith.constant 4 : i64
      %q0_dep2_vhit = arith.andi %q0_dvalid, %q0_dep2_vbit : i64
      %q0_dep2_used = arith.cmpi ne, %q0_dep2_vhit, %c0 : i64
      %q0_dep2_unused = arith.cmpi eq, %q0_dep2_used, %false : i1
      %q0_dep2_ready = arith.ori %q0_dep2_unused, %q0_dep2_hit : i1
      %q0_deps01 = arith.andi %q0_dep0_ready, %q0_dep1_ready : i1
      %q0_deps = arith.andi %q0_deps01, %q0_dep2_ready : i1
      %q0_vbit = arith.constant 1 : i64
      %q0_vhit = arith.andi %valid_bits, %q0_vbit : i64
      %q0_occupied = arith.cmpi ne, %q0_vhit, %c0 : i64
      %q0_eligible = arith.andi %q0_occupied, %q0_deps : i1
      %q1_desc = ac.trace.decode %s1 : i64 to i64
      %q1_dv_s = arith.shrui %q1_desc, %c35 : i64
      %q1_dvalid = arith.andi %q1_dv_s, %mask7 : i64
      %q1_dep0_shift = arith.constant 11 : i64
      %q1_dep0_s = arith.shrui %q1_desc, %q1_dep0_shift : i64
      %q1_dep0 = arith.andi %q1_dep0_s, %mask255 : i64
      %q1_dep0_word = arith.shrui %q1_dep0, %c6 : i64
      %q1_dep0_off = arith.andi %q1_dep0, %mask63 : i64
      %q1_dep0_bit = arith.shli %c1, %q1_dep0_off : i64
      %q1_dep0_w0 = arith.cmpi eq, %q1_dep0_word, %c0 : i64
      %q1_dep0_w1 = arith.cmpi eq, %q1_dep0_word, %c1 : i64
      %q1_dep0_w2 = arith.cmpi eq, %q1_dep0_word, %c2 : i64
      %q1_dep0_sel1 = arith.select %q1_dep0_w1, %nr1, %nr3 : i64
      %q1_dep0_sel0 = arith.select %q1_dep0_w0, %nr0, %q1_dep0_sel1 : i64
      %q1_dep0_bits = arith.select %q1_dep0_w2, %nr2, %q1_dep0_sel0 : i64
      %q1_dep0_hit0 = arith.andi %q1_dep0_bits, %q1_dep0_bit : i64
      %q1_dep0_hit = arith.cmpi ne, %q1_dep0_hit0, %c0 : i64
      %q1_dep0_vbit = arith.constant 1 : i64
      %q1_dep0_vhit = arith.andi %q1_dvalid, %q1_dep0_vbit : i64
      %q1_dep0_used = arith.cmpi ne, %q1_dep0_vhit, %c0 : i64
      %q1_dep0_unused = arith.cmpi eq, %q1_dep0_used, %false : i1
      %q1_dep0_ready = arith.ori %q1_dep0_unused, %q1_dep0_hit : i1
      %q1_dep1_shift = arith.constant 19 : i64
      %q1_dep1_s = arith.shrui %q1_desc, %q1_dep1_shift : i64
      %q1_dep1 = arith.andi %q1_dep1_s, %mask255 : i64
      %q1_dep1_word = arith.shrui %q1_dep1, %c6 : i64
      %q1_dep1_off = arith.andi %q1_dep1, %mask63 : i64
      %q1_dep1_bit = arith.shli %c1, %q1_dep1_off : i64
      %q1_dep1_w0 = arith.cmpi eq, %q1_dep1_word, %c0 : i64
      %q1_dep1_w1 = arith.cmpi eq, %q1_dep1_word, %c1 : i64
      %q1_dep1_w2 = arith.cmpi eq, %q1_dep1_word, %c2 : i64
      %q1_dep1_sel1 = arith.select %q1_dep1_w1, %nr1, %nr3 : i64
      %q1_dep1_sel0 = arith.select %q1_dep1_w0, %nr0, %q1_dep1_sel1 : i64
      %q1_dep1_bits = arith.select %q1_dep1_w2, %nr2, %q1_dep1_sel0 : i64
      %q1_dep1_hit0 = arith.andi %q1_dep1_bits, %q1_dep1_bit : i64
      %q1_dep1_hit = arith.cmpi ne, %q1_dep1_hit0, %c0 : i64
      %q1_dep1_vbit = arith.constant 2 : i64
      %q1_dep1_vhit = arith.andi %q1_dvalid, %q1_dep1_vbit : i64
      %q1_dep1_used = arith.cmpi ne, %q1_dep1_vhit, %c0 : i64
      %q1_dep1_unused = arith.cmpi eq, %q1_dep1_used, %false : i1
      %q1_dep1_ready = arith.ori %q1_dep1_unused, %q1_dep1_hit : i1
      %q1_dep2_shift = arith.constant 27 : i64
      %q1_dep2_s = arith.shrui %q1_desc, %q1_dep2_shift : i64
      %q1_dep2 = arith.andi %q1_dep2_s, %mask255 : i64
      %q1_dep2_word = arith.shrui %q1_dep2, %c6 : i64
      %q1_dep2_off = arith.andi %q1_dep2, %mask63 : i64
      %q1_dep2_bit = arith.shli %c1, %q1_dep2_off : i64
      %q1_dep2_w0 = arith.cmpi eq, %q1_dep2_word, %c0 : i64
      %q1_dep2_w1 = arith.cmpi eq, %q1_dep2_word, %c1 : i64
      %q1_dep2_w2 = arith.cmpi eq, %q1_dep2_word, %c2 : i64
      %q1_dep2_sel1 = arith.select %q1_dep2_w1, %nr1, %nr3 : i64
      %q1_dep2_sel0 = arith.select %q1_dep2_w0, %nr0, %q1_dep2_sel1 : i64
      %q1_dep2_bits = arith.select %q1_dep2_w2, %nr2, %q1_dep2_sel0 : i64
      %q1_dep2_hit0 = arith.andi %q1_dep2_bits, %q1_dep2_bit : i64
      %q1_dep2_hit = arith.cmpi ne, %q1_dep2_hit0, %c0 : i64
      %q1_dep2_vbit = arith.constant 4 : i64
      %q1_dep2_vhit = arith.andi %q1_dvalid, %q1_dep2_vbit : i64
      %q1_dep2_used = arith.cmpi ne, %q1_dep2_vhit, %c0 : i64
      %q1_dep2_unused = arith.cmpi eq, %q1_dep2_used, %false : i1
      %q1_dep2_ready = arith.ori %q1_dep2_unused, %q1_dep2_hit : i1
      %q1_deps01 = arith.andi %q1_dep0_ready, %q1_dep1_ready : i1
      %q1_deps = arith.andi %q1_deps01, %q1_dep2_ready : i1
      %q1_vbit = arith.constant 2 : i64
      %q1_vhit = arith.andi %valid_bits, %q1_vbit : i64
      %q1_occupied = arith.cmpi ne, %q1_vhit, %c0 : i64
      %q1_eligible = arith.andi %q1_occupied, %q1_deps : i1
      %q2_desc = ac.trace.decode %s2 : i64 to i64
      %q2_dv_s = arith.shrui %q2_desc, %c35 : i64
      %q2_dvalid = arith.andi %q2_dv_s, %mask7 : i64
      %q2_dep0_shift = arith.constant 11 : i64
      %q2_dep0_s = arith.shrui %q2_desc, %q2_dep0_shift : i64
      %q2_dep0 = arith.andi %q2_dep0_s, %mask255 : i64
      %q2_dep0_word = arith.shrui %q2_dep0, %c6 : i64
      %q2_dep0_off = arith.andi %q2_dep0, %mask63 : i64
      %q2_dep0_bit = arith.shli %c1, %q2_dep0_off : i64
      %q2_dep0_w0 = arith.cmpi eq, %q2_dep0_word, %c0 : i64
      %q2_dep0_w1 = arith.cmpi eq, %q2_dep0_word, %c1 : i64
      %q2_dep0_w2 = arith.cmpi eq, %q2_dep0_word, %c2 : i64
      %q2_dep0_sel1 = arith.select %q2_dep0_w1, %nr1, %nr3 : i64
      %q2_dep0_sel0 = arith.select %q2_dep0_w0, %nr0, %q2_dep0_sel1 : i64
      %q2_dep0_bits = arith.select %q2_dep0_w2, %nr2, %q2_dep0_sel0 : i64
      %q2_dep0_hit0 = arith.andi %q2_dep0_bits, %q2_dep0_bit : i64
      %q2_dep0_hit = arith.cmpi ne, %q2_dep0_hit0, %c0 : i64
      %q2_dep0_vbit = arith.constant 1 : i64
      %q2_dep0_vhit = arith.andi %q2_dvalid, %q2_dep0_vbit : i64
      %q2_dep0_used = arith.cmpi ne, %q2_dep0_vhit, %c0 : i64
      %q2_dep0_unused = arith.cmpi eq, %q2_dep0_used, %false : i1
      %q2_dep0_ready = arith.ori %q2_dep0_unused, %q2_dep0_hit : i1
      %q2_dep1_shift = arith.constant 19 : i64
      %q2_dep1_s = arith.shrui %q2_desc, %q2_dep1_shift : i64
      %q2_dep1 = arith.andi %q2_dep1_s, %mask255 : i64
      %q2_dep1_word = arith.shrui %q2_dep1, %c6 : i64
      %q2_dep1_off = arith.andi %q2_dep1, %mask63 : i64
      %q2_dep1_bit = arith.shli %c1, %q2_dep1_off : i64
      %q2_dep1_w0 = arith.cmpi eq, %q2_dep1_word, %c0 : i64
      %q2_dep1_w1 = arith.cmpi eq, %q2_dep1_word, %c1 : i64
      %q2_dep1_w2 = arith.cmpi eq, %q2_dep1_word, %c2 : i64
      %q2_dep1_sel1 = arith.select %q2_dep1_w1, %nr1, %nr3 : i64
      %q2_dep1_sel0 = arith.select %q2_dep1_w0, %nr0, %q2_dep1_sel1 : i64
      %q2_dep1_bits = arith.select %q2_dep1_w2, %nr2, %q2_dep1_sel0 : i64
      %q2_dep1_hit0 = arith.andi %q2_dep1_bits, %q2_dep1_bit : i64
      %q2_dep1_hit = arith.cmpi ne, %q2_dep1_hit0, %c0 : i64
      %q2_dep1_vbit = arith.constant 2 : i64
      %q2_dep1_vhit = arith.andi %q2_dvalid, %q2_dep1_vbit : i64
      %q2_dep1_used = arith.cmpi ne, %q2_dep1_vhit, %c0 : i64
      %q2_dep1_unused = arith.cmpi eq, %q2_dep1_used, %false : i1
      %q2_dep1_ready = arith.ori %q2_dep1_unused, %q2_dep1_hit : i1
      %q2_dep2_shift = arith.constant 27 : i64
      %q2_dep2_s = arith.shrui %q2_desc, %q2_dep2_shift : i64
      %q2_dep2 = arith.andi %q2_dep2_s, %mask255 : i64
      %q2_dep2_word = arith.shrui %q2_dep2, %c6 : i64
      %q2_dep2_off = arith.andi %q2_dep2, %mask63 : i64
      %q2_dep2_bit = arith.shli %c1, %q2_dep2_off : i64
      %q2_dep2_w0 = arith.cmpi eq, %q2_dep2_word, %c0 : i64
      %q2_dep2_w1 = arith.cmpi eq, %q2_dep2_word, %c1 : i64
      %q2_dep2_w2 = arith.cmpi eq, %q2_dep2_word, %c2 : i64
      %q2_dep2_sel1 = arith.select %q2_dep2_w1, %nr1, %nr3 : i64
      %q2_dep2_sel0 = arith.select %q2_dep2_w0, %nr0, %q2_dep2_sel1 : i64
      %q2_dep2_bits = arith.select %q2_dep2_w2, %nr2, %q2_dep2_sel0 : i64
      %q2_dep2_hit0 = arith.andi %q2_dep2_bits, %q2_dep2_bit : i64
      %q2_dep2_hit = arith.cmpi ne, %q2_dep2_hit0, %c0 : i64
      %q2_dep2_vbit = arith.constant 4 : i64
      %q2_dep2_vhit = arith.andi %q2_dvalid, %q2_dep2_vbit : i64
      %q2_dep2_used = arith.cmpi ne, %q2_dep2_vhit, %c0 : i64
      %q2_dep2_unused = arith.cmpi eq, %q2_dep2_used, %false : i1
      %q2_dep2_ready = arith.ori %q2_dep2_unused, %q2_dep2_hit : i1
      %q2_deps01 = arith.andi %q2_dep0_ready, %q2_dep1_ready : i1
      %q2_deps = arith.andi %q2_deps01, %q2_dep2_ready : i1
      %q2_vbit = arith.constant 4 : i64
      %q2_vhit = arith.andi %valid_bits, %q2_vbit : i64
      %q2_occupied = arith.cmpi ne, %q2_vhit, %c0 : i64
      %q2_eligible = arith.andi %q2_occupied, %q2_deps : i1
      %q3_desc = ac.trace.decode %s3 : i64 to i64
      %q3_dv_s = arith.shrui %q3_desc, %c35 : i64
      %q3_dvalid = arith.andi %q3_dv_s, %mask7 : i64
      %q3_dep0_shift = arith.constant 11 : i64
      %q3_dep0_s = arith.shrui %q3_desc, %q3_dep0_shift : i64
      %q3_dep0 = arith.andi %q3_dep0_s, %mask255 : i64
      %q3_dep0_word = arith.shrui %q3_dep0, %c6 : i64
      %q3_dep0_off = arith.andi %q3_dep0, %mask63 : i64
      %q3_dep0_bit = arith.shli %c1, %q3_dep0_off : i64
      %q3_dep0_w0 = arith.cmpi eq, %q3_dep0_word, %c0 : i64
      %q3_dep0_w1 = arith.cmpi eq, %q3_dep0_word, %c1 : i64
      %q3_dep0_w2 = arith.cmpi eq, %q3_dep0_word, %c2 : i64
      %q3_dep0_sel1 = arith.select %q3_dep0_w1, %nr1, %nr3 : i64
      %q3_dep0_sel0 = arith.select %q3_dep0_w0, %nr0, %q3_dep0_sel1 : i64
      %q3_dep0_bits = arith.select %q3_dep0_w2, %nr2, %q3_dep0_sel0 : i64
      %q3_dep0_hit0 = arith.andi %q3_dep0_bits, %q3_dep0_bit : i64
      %q3_dep0_hit = arith.cmpi ne, %q3_dep0_hit0, %c0 : i64
      %q3_dep0_vbit = arith.constant 1 : i64
      %q3_dep0_vhit = arith.andi %q3_dvalid, %q3_dep0_vbit : i64
      %q3_dep0_used = arith.cmpi ne, %q3_dep0_vhit, %c0 : i64
      %q3_dep0_unused = arith.cmpi eq, %q3_dep0_used, %false : i1
      %q3_dep0_ready = arith.ori %q3_dep0_unused, %q3_dep0_hit : i1
      %q3_dep1_shift = arith.constant 19 : i64
      %q3_dep1_s = arith.shrui %q3_desc, %q3_dep1_shift : i64
      %q3_dep1 = arith.andi %q3_dep1_s, %mask255 : i64
      %q3_dep1_word = arith.shrui %q3_dep1, %c6 : i64
      %q3_dep1_off = arith.andi %q3_dep1, %mask63 : i64
      %q3_dep1_bit = arith.shli %c1, %q3_dep1_off : i64
      %q3_dep1_w0 = arith.cmpi eq, %q3_dep1_word, %c0 : i64
      %q3_dep1_w1 = arith.cmpi eq, %q3_dep1_word, %c1 : i64
      %q3_dep1_w2 = arith.cmpi eq, %q3_dep1_word, %c2 : i64
      %q3_dep1_sel1 = arith.select %q3_dep1_w1, %nr1, %nr3 : i64
      %q3_dep1_sel0 = arith.select %q3_dep1_w0, %nr0, %q3_dep1_sel1 : i64
      %q3_dep1_bits = arith.select %q3_dep1_w2, %nr2, %q3_dep1_sel0 : i64
      %q3_dep1_hit0 = arith.andi %q3_dep1_bits, %q3_dep1_bit : i64
      %q3_dep1_hit = arith.cmpi ne, %q3_dep1_hit0, %c0 : i64
      %q3_dep1_vbit = arith.constant 2 : i64
      %q3_dep1_vhit = arith.andi %q3_dvalid, %q3_dep1_vbit : i64
      %q3_dep1_used = arith.cmpi ne, %q3_dep1_vhit, %c0 : i64
      %q3_dep1_unused = arith.cmpi eq, %q3_dep1_used, %false : i1
      %q3_dep1_ready = arith.ori %q3_dep1_unused, %q3_dep1_hit : i1
      %q3_dep2_shift = arith.constant 27 : i64
      %q3_dep2_s = arith.shrui %q3_desc, %q3_dep2_shift : i64
      %q3_dep2 = arith.andi %q3_dep2_s, %mask255 : i64
      %q3_dep2_word = arith.shrui %q3_dep2, %c6 : i64
      %q3_dep2_off = arith.andi %q3_dep2, %mask63 : i64
      %q3_dep2_bit = arith.shli %c1, %q3_dep2_off : i64
      %q3_dep2_w0 = arith.cmpi eq, %q3_dep2_word, %c0 : i64
      %q3_dep2_w1 = arith.cmpi eq, %q3_dep2_word, %c1 : i64
      %q3_dep2_w2 = arith.cmpi eq, %q3_dep2_word, %c2 : i64
      %q3_dep2_sel1 = arith.select %q3_dep2_w1, %nr1, %nr3 : i64
      %q3_dep2_sel0 = arith.select %q3_dep2_w0, %nr0, %q3_dep2_sel1 : i64
      %q3_dep2_bits = arith.select %q3_dep2_w2, %nr2, %q3_dep2_sel0 : i64
      %q3_dep2_hit0 = arith.andi %q3_dep2_bits, %q3_dep2_bit : i64
      %q3_dep2_hit = arith.cmpi ne, %q3_dep2_hit0, %c0 : i64
      %q3_dep2_vbit = arith.constant 4 : i64
      %q3_dep2_vhit = arith.andi %q3_dvalid, %q3_dep2_vbit : i64
      %q3_dep2_used = arith.cmpi ne, %q3_dep2_vhit, %c0 : i64
      %q3_dep2_unused = arith.cmpi eq, %q3_dep2_used, %false : i1
      %q3_dep2_ready = arith.ori %q3_dep2_unused, %q3_dep2_hit : i1
      %q3_deps01 = arith.andi %q3_dep0_ready, %q3_dep1_ready : i1
      %q3_deps = arith.andi %q3_deps01, %q3_dep2_ready : i1
      %q3_vbit = arith.constant 8 : i64
      %q3_vhit = arith.andi %valid_bits, %q3_vbit : i64
      %q3_occupied = arith.cmpi ne, %q3_vhit, %c0 : i64
      %q3_eligible = arith.andi %q3_occupied, %q3_deps : i1
      %q4_desc = ac.trace.decode %s4 : i64 to i64
      %q4_dv_s = arith.shrui %q4_desc, %c35 : i64
      %q4_dvalid = arith.andi %q4_dv_s, %mask7 : i64
      %q4_dep0_shift = arith.constant 11 : i64
      %q4_dep0_s = arith.shrui %q4_desc, %q4_dep0_shift : i64
      %q4_dep0 = arith.andi %q4_dep0_s, %mask255 : i64
      %q4_dep0_word = arith.shrui %q4_dep0, %c6 : i64
      %q4_dep0_off = arith.andi %q4_dep0, %mask63 : i64
      %q4_dep0_bit = arith.shli %c1, %q4_dep0_off : i64
      %q4_dep0_w0 = arith.cmpi eq, %q4_dep0_word, %c0 : i64
      %q4_dep0_w1 = arith.cmpi eq, %q4_dep0_word, %c1 : i64
      %q4_dep0_w2 = arith.cmpi eq, %q4_dep0_word, %c2 : i64
      %q4_dep0_sel1 = arith.select %q4_dep0_w1, %nr1, %nr3 : i64
      %q4_dep0_sel0 = arith.select %q4_dep0_w0, %nr0, %q4_dep0_sel1 : i64
      %q4_dep0_bits = arith.select %q4_dep0_w2, %nr2, %q4_dep0_sel0 : i64
      %q4_dep0_hit0 = arith.andi %q4_dep0_bits, %q4_dep0_bit : i64
      %q4_dep0_hit = arith.cmpi ne, %q4_dep0_hit0, %c0 : i64
      %q4_dep0_vbit = arith.constant 1 : i64
      %q4_dep0_vhit = arith.andi %q4_dvalid, %q4_dep0_vbit : i64
      %q4_dep0_used = arith.cmpi ne, %q4_dep0_vhit, %c0 : i64
      %q4_dep0_unused = arith.cmpi eq, %q4_dep0_used, %false : i1
      %q4_dep0_ready = arith.ori %q4_dep0_unused, %q4_dep0_hit : i1
      %q4_dep1_shift = arith.constant 19 : i64
      %q4_dep1_s = arith.shrui %q4_desc, %q4_dep1_shift : i64
      %q4_dep1 = arith.andi %q4_dep1_s, %mask255 : i64
      %q4_dep1_word = arith.shrui %q4_dep1, %c6 : i64
      %q4_dep1_off = arith.andi %q4_dep1, %mask63 : i64
      %q4_dep1_bit = arith.shli %c1, %q4_dep1_off : i64
      %q4_dep1_w0 = arith.cmpi eq, %q4_dep1_word, %c0 : i64
      %q4_dep1_w1 = arith.cmpi eq, %q4_dep1_word, %c1 : i64
      %q4_dep1_w2 = arith.cmpi eq, %q4_dep1_word, %c2 : i64
      %q4_dep1_sel1 = arith.select %q4_dep1_w1, %nr1, %nr3 : i64
      %q4_dep1_sel0 = arith.select %q4_dep1_w0, %nr0, %q4_dep1_sel1 : i64
      %q4_dep1_bits = arith.select %q4_dep1_w2, %nr2, %q4_dep1_sel0 : i64
      %q4_dep1_hit0 = arith.andi %q4_dep1_bits, %q4_dep1_bit : i64
      %q4_dep1_hit = arith.cmpi ne, %q4_dep1_hit0, %c0 : i64
      %q4_dep1_vbit = arith.constant 2 : i64
      %q4_dep1_vhit = arith.andi %q4_dvalid, %q4_dep1_vbit : i64
      %q4_dep1_used = arith.cmpi ne, %q4_dep1_vhit, %c0 : i64
      %q4_dep1_unused = arith.cmpi eq, %q4_dep1_used, %false : i1
      %q4_dep1_ready = arith.ori %q4_dep1_unused, %q4_dep1_hit : i1
      %q4_dep2_shift = arith.constant 27 : i64
      %q4_dep2_s = arith.shrui %q4_desc, %q4_dep2_shift : i64
      %q4_dep2 = arith.andi %q4_dep2_s, %mask255 : i64
      %q4_dep2_word = arith.shrui %q4_dep2, %c6 : i64
      %q4_dep2_off = arith.andi %q4_dep2, %mask63 : i64
      %q4_dep2_bit = arith.shli %c1, %q4_dep2_off : i64
      %q4_dep2_w0 = arith.cmpi eq, %q4_dep2_word, %c0 : i64
      %q4_dep2_w1 = arith.cmpi eq, %q4_dep2_word, %c1 : i64
      %q4_dep2_w2 = arith.cmpi eq, %q4_dep2_word, %c2 : i64
      %q4_dep2_sel1 = arith.select %q4_dep2_w1, %nr1, %nr3 : i64
      %q4_dep2_sel0 = arith.select %q4_dep2_w0, %nr0, %q4_dep2_sel1 : i64
      %q4_dep2_bits = arith.select %q4_dep2_w2, %nr2, %q4_dep2_sel0 : i64
      %q4_dep2_hit0 = arith.andi %q4_dep2_bits, %q4_dep2_bit : i64
      %q4_dep2_hit = arith.cmpi ne, %q4_dep2_hit0, %c0 : i64
      %q4_dep2_vbit = arith.constant 4 : i64
      %q4_dep2_vhit = arith.andi %q4_dvalid, %q4_dep2_vbit : i64
      %q4_dep2_used = arith.cmpi ne, %q4_dep2_vhit, %c0 : i64
      %q4_dep2_unused = arith.cmpi eq, %q4_dep2_used, %false : i1
      %q4_dep2_ready = arith.ori %q4_dep2_unused, %q4_dep2_hit : i1
      %q4_deps01 = arith.andi %q4_dep0_ready, %q4_dep1_ready : i1
      %q4_deps = arith.andi %q4_deps01, %q4_dep2_ready : i1
      %q4_vbit = arith.constant 16 : i64
      %q4_vhit = arith.andi %valid_bits, %q4_vbit : i64
      %q4_occupied = arith.cmpi ne, %q4_vhit, %c0 : i64
      %q4_eligible = arith.andi %q4_occupied, %q4_deps : i1
      %q5_desc = ac.trace.decode %s5 : i64 to i64
      %q5_dv_s = arith.shrui %q5_desc, %c35 : i64
      %q5_dvalid = arith.andi %q5_dv_s, %mask7 : i64
      %q5_dep0_shift = arith.constant 11 : i64
      %q5_dep0_s = arith.shrui %q5_desc, %q5_dep0_shift : i64
      %q5_dep0 = arith.andi %q5_dep0_s, %mask255 : i64
      %q5_dep0_word = arith.shrui %q5_dep0, %c6 : i64
      %q5_dep0_off = arith.andi %q5_dep0, %mask63 : i64
      %q5_dep0_bit = arith.shli %c1, %q5_dep0_off : i64
      %q5_dep0_w0 = arith.cmpi eq, %q5_dep0_word, %c0 : i64
      %q5_dep0_w1 = arith.cmpi eq, %q5_dep0_word, %c1 : i64
      %q5_dep0_w2 = arith.cmpi eq, %q5_dep0_word, %c2 : i64
      %q5_dep0_sel1 = arith.select %q5_dep0_w1, %nr1, %nr3 : i64
      %q5_dep0_sel0 = arith.select %q5_dep0_w0, %nr0, %q5_dep0_sel1 : i64
      %q5_dep0_bits = arith.select %q5_dep0_w2, %nr2, %q5_dep0_sel0 : i64
      %q5_dep0_hit0 = arith.andi %q5_dep0_bits, %q5_dep0_bit : i64
      %q5_dep0_hit = arith.cmpi ne, %q5_dep0_hit0, %c0 : i64
      %q5_dep0_vbit = arith.constant 1 : i64
      %q5_dep0_vhit = arith.andi %q5_dvalid, %q5_dep0_vbit : i64
      %q5_dep0_used = arith.cmpi ne, %q5_dep0_vhit, %c0 : i64
      %q5_dep0_unused = arith.cmpi eq, %q5_dep0_used, %false : i1
      %q5_dep0_ready = arith.ori %q5_dep0_unused, %q5_dep0_hit : i1
      %q5_dep1_shift = arith.constant 19 : i64
      %q5_dep1_s = arith.shrui %q5_desc, %q5_dep1_shift : i64
      %q5_dep1 = arith.andi %q5_dep1_s, %mask255 : i64
      %q5_dep1_word = arith.shrui %q5_dep1, %c6 : i64
      %q5_dep1_off = arith.andi %q5_dep1, %mask63 : i64
      %q5_dep1_bit = arith.shli %c1, %q5_dep1_off : i64
      %q5_dep1_w0 = arith.cmpi eq, %q5_dep1_word, %c0 : i64
      %q5_dep1_w1 = arith.cmpi eq, %q5_dep1_word, %c1 : i64
      %q5_dep1_w2 = arith.cmpi eq, %q5_dep1_word, %c2 : i64
      %q5_dep1_sel1 = arith.select %q5_dep1_w1, %nr1, %nr3 : i64
      %q5_dep1_sel0 = arith.select %q5_dep1_w0, %nr0, %q5_dep1_sel1 : i64
      %q5_dep1_bits = arith.select %q5_dep1_w2, %nr2, %q5_dep1_sel0 : i64
      %q5_dep1_hit0 = arith.andi %q5_dep1_bits, %q5_dep1_bit : i64
      %q5_dep1_hit = arith.cmpi ne, %q5_dep1_hit0, %c0 : i64
      %q5_dep1_vbit = arith.constant 2 : i64
      %q5_dep1_vhit = arith.andi %q5_dvalid, %q5_dep1_vbit : i64
      %q5_dep1_used = arith.cmpi ne, %q5_dep1_vhit, %c0 : i64
      %q5_dep1_unused = arith.cmpi eq, %q5_dep1_used, %false : i1
      %q5_dep1_ready = arith.ori %q5_dep1_unused, %q5_dep1_hit : i1
      %q5_dep2_shift = arith.constant 27 : i64
      %q5_dep2_s = arith.shrui %q5_desc, %q5_dep2_shift : i64
      %q5_dep2 = arith.andi %q5_dep2_s, %mask255 : i64
      %q5_dep2_word = arith.shrui %q5_dep2, %c6 : i64
      %q5_dep2_off = arith.andi %q5_dep2, %mask63 : i64
      %q5_dep2_bit = arith.shli %c1, %q5_dep2_off : i64
      %q5_dep2_w0 = arith.cmpi eq, %q5_dep2_word, %c0 : i64
      %q5_dep2_w1 = arith.cmpi eq, %q5_dep2_word, %c1 : i64
      %q5_dep2_w2 = arith.cmpi eq, %q5_dep2_word, %c2 : i64
      %q5_dep2_sel1 = arith.select %q5_dep2_w1, %nr1, %nr3 : i64
      %q5_dep2_sel0 = arith.select %q5_dep2_w0, %nr0, %q5_dep2_sel1 : i64
      %q5_dep2_bits = arith.select %q5_dep2_w2, %nr2, %q5_dep2_sel0 : i64
      %q5_dep2_hit0 = arith.andi %q5_dep2_bits, %q5_dep2_bit : i64
      %q5_dep2_hit = arith.cmpi ne, %q5_dep2_hit0, %c0 : i64
      %q5_dep2_vbit = arith.constant 4 : i64
      %q5_dep2_vhit = arith.andi %q5_dvalid, %q5_dep2_vbit : i64
      %q5_dep2_used = arith.cmpi ne, %q5_dep2_vhit, %c0 : i64
      %q5_dep2_unused = arith.cmpi eq, %q5_dep2_used, %false : i1
      %q5_dep2_ready = arith.ori %q5_dep2_unused, %q5_dep2_hit : i1
      %q5_deps01 = arith.andi %q5_dep0_ready, %q5_dep1_ready : i1
      %q5_deps = arith.andi %q5_deps01, %q5_dep2_ready : i1
      %q5_vbit = arith.constant 32 : i64
      %q5_vhit = arith.andi %valid_bits, %q5_vbit : i64
      %q5_occupied = arith.cmpi ne, %q5_vhit, %c0 : i64
      %q5_eligible = arith.andi %q5_occupied, %q5_deps : i1
      %q6_desc = ac.trace.decode %s6 : i64 to i64
      %q6_dv_s = arith.shrui %q6_desc, %c35 : i64
      %q6_dvalid = arith.andi %q6_dv_s, %mask7 : i64
      %q6_dep0_shift = arith.constant 11 : i64
      %q6_dep0_s = arith.shrui %q6_desc, %q6_dep0_shift : i64
      %q6_dep0 = arith.andi %q6_dep0_s, %mask255 : i64
      %q6_dep0_word = arith.shrui %q6_dep0, %c6 : i64
      %q6_dep0_off = arith.andi %q6_dep0, %mask63 : i64
      %q6_dep0_bit = arith.shli %c1, %q6_dep0_off : i64
      %q6_dep0_w0 = arith.cmpi eq, %q6_dep0_word, %c0 : i64
      %q6_dep0_w1 = arith.cmpi eq, %q6_dep0_word, %c1 : i64
      %q6_dep0_w2 = arith.cmpi eq, %q6_dep0_word, %c2 : i64
      %q6_dep0_sel1 = arith.select %q6_dep0_w1, %nr1, %nr3 : i64
      %q6_dep0_sel0 = arith.select %q6_dep0_w0, %nr0, %q6_dep0_sel1 : i64
      %q6_dep0_bits = arith.select %q6_dep0_w2, %nr2, %q6_dep0_sel0 : i64
      %q6_dep0_hit0 = arith.andi %q6_dep0_bits, %q6_dep0_bit : i64
      %q6_dep0_hit = arith.cmpi ne, %q6_dep0_hit0, %c0 : i64
      %q6_dep0_vbit = arith.constant 1 : i64
      %q6_dep0_vhit = arith.andi %q6_dvalid, %q6_dep0_vbit : i64
      %q6_dep0_used = arith.cmpi ne, %q6_dep0_vhit, %c0 : i64
      %q6_dep0_unused = arith.cmpi eq, %q6_dep0_used, %false : i1
      %q6_dep0_ready = arith.ori %q6_dep0_unused, %q6_dep0_hit : i1
      %q6_dep1_shift = arith.constant 19 : i64
      %q6_dep1_s = arith.shrui %q6_desc, %q6_dep1_shift : i64
      %q6_dep1 = arith.andi %q6_dep1_s, %mask255 : i64
      %q6_dep1_word = arith.shrui %q6_dep1, %c6 : i64
      %q6_dep1_off = arith.andi %q6_dep1, %mask63 : i64
      %q6_dep1_bit = arith.shli %c1, %q6_dep1_off : i64
      %q6_dep1_w0 = arith.cmpi eq, %q6_dep1_word, %c0 : i64
      %q6_dep1_w1 = arith.cmpi eq, %q6_dep1_word, %c1 : i64
      %q6_dep1_w2 = arith.cmpi eq, %q6_dep1_word, %c2 : i64
      %q6_dep1_sel1 = arith.select %q6_dep1_w1, %nr1, %nr3 : i64
      %q6_dep1_sel0 = arith.select %q6_dep1_w0, %nr0, %q6_dep1_sel1 : i64
      %q6_dep1_bits = arith.select %q6_dep1_w2, %nr2, %q6_dep1_sel0 : i64
      %q6_dep1_hit0 = arith.andi %q6_dep1_bits, %q6_dep1_bit : i64
      %q6_dep1_hit = arith.cmpi ne, %q6_dep1_hit0, %c0 : i64
      %q6_dep1_vbit = arith.constant 2 : i64
      %q6_dep1_vhit = arith.andi %q6_dvalid, %q6_dep1_vbit : i64
      %q6_dep1_used = arith.cmpi ne, %q6_dep1_vhit, %c0 : i64
      %q6_dep1_unused = arith.cmpi eq, %q6_dep1_used, %false : i1
      %q6_dep1_ready = arith.ori %q6_dep1_unused, %q6_dep1_hit : i1
      %q6_dep2_shift = arith.constant 27 : i64
      %q6_dep2_s = arith.shrui %q6_desc, %q6_dep2_shift : i64
      %q6_dep2 = arith.andi %q6_dep2_s, %mask255 : i64
      %q6_dep2_word = arith.shrui %q6_dep2, %c6 : i64
      %q6_dep2_off = arith.andi %q6_dep2, %mask63 : i64
      %q6_dep2_bit = arith.shli %c1, %q6_dep2_off : i64
      %q6_dep2_w0 = arith.cmpi eq, %q6_dep2_word, %c0 : i64
      %q6_dep2_w1 = arith.cmpi eq, %q6_dep2_word, %c1 : i64
      %q6_dep2_w2 = arith.cmpi eq, %q6_dep2_word, %c2 : i64
      %q6_dep2_sel1 = arith.select %q6_dep2_w1, %nr1, %nr3 : i64
      %q6_dep2_sel0 = arith.select %q6_dep2_w0, %nr0, %q6_dep2_sel1 : i64
      %q6_dep2_bits = arith.select %q6_dep2_w2, %nr2, %q6_dep2_sel0 : i64
      %q6_dep2_hit0 = arith.andi %q6_dep2_bits, %q6_dep2_bit : i64
      %q6_dep2_hit = arith.cmpi ne, %q6_dep2_hit0, %c0 : i64
      %q6_dep2_vbit = arith.constant 4 : i64
      %q6_dep2_vhit = arith.andi %q6_dvalid, %q6_dep2_vbit : i64
      %q6_dep2_used = arith.cmpi ne, %q6_dep2_vhit, %c0 : i64
      %q6_dep2_unused = arith.cmpi eq, %q6_dep2_used, %false : i1
      %q6_dep2_ready = arith.ori %q6_dep2_unused, %q6_dep2_hit : i1
      %q6_deps01 = arith.andi %q6_dep0_ready, %q6_dep1_ready : i1
      %q6_deps = arith.andi %q6_deps01, %q6_dep2_ready : i1
      %q6_vbit = arith.constant 64 : i64
      %q6_vhit = arith.andi %valid_bits, %q6_vbit : i64
      %q6_occupied = arith.cmpi ne, %q6_vhit, %c0 : i64
      %q6_eligible = arith.andi %q6_occupied, %q6_deps : i1
      %q7_desc = ac.trace.decode %s7 : i64 to i64
      %q7_dv_s = arith.shrui %q7_desc, %c35 : i64
      %q7_dvalid = arith.andi %q7_dv_s, %mask7 : i64
      %q7_dep0_shift = arith.constant 11 : i64
      %q7_dep0_s = arith.shrui %q7_desc, %q7_dep0_shift : i64
      %q7_dep0 = arith.andi %q7_dep0_s, %mask255 : i64
      %q7_dep0_word = arith.shrui %q7_dep0, %c6 : i64
      %q7_dep0_off = arith.andi %q7_dep0, %mask63 : i64
      %q7_dep0_bit = arith.shli %c1, %q7_dep0_off : i64
      %q7_dep0_w0 = arith.cmpi eq, %q7_dep0_word, %c0 : i64
      %q7_dep0_w1 = arith.cmpi eq, %q7_dep0_word, %c1 : i64
      %q7_dep0_w2 = arith.cmpi eq, %q7_dep0_word, %c2 : i64
      %q7_dep0_sel1 = arith.select %q7_dep0_w1, %nr1, %nr3 : i64
      %q7_dep0_sel0 = arith.select %q7_dep0_w0, %nr0, %q7_dep0_sel1 : i64
      %q7_dep0_bits = arith.select %q7_dep0_w2, %nr2, %q7_dep0_sel0 : i64
      %q7_dep0_hit0 = arith.andi %q7_dep0_bits, %q7_dep0_bit : i64
      %q7_dep0_hit = arith.cmpi ne, %q7_dep0_hit0, %c0 : i64
      %q7_dep0_vbit = arith.constant 1 : i64
      %q7_dep0_vhit = arith.andi %q7_dvalid, %q7_dep0_vbit : i64
      %q7_dep0_used = arith.cmpi ne, %q7_dep0_vhit, %c0 : i64
      %q7_dep0_unused = arith.cmpi eq, %q7_dep0_used, %false : i1
      %q7_dep0_ready = arith.ori %q7_dep0_unused, %q7_dep0_hit : i1
      %q7_dep1_shift = arith.constant 19 : i64
      %q7_dep1_s = arith.shrui %q7_desc, %q7_dep1_shift : i64
      %q7_dep1 = arith.andi %q7_dep1_s, %mask255 : i64
      %q7_dep1_word = arith.shrui %q7_dep1, %c6 : i64
      %q7_dep1_off = arith.andi %q7_dep1, %mask63 : i64
      %q7_dep1_bit = arith.shli %c1, %q7_dep1_off : i64
      %q7_dep1_w0 = arith.cmpi eq, %q7_dep1_word, %c0 : i64
      %q7_dep1_w1 = arith.cmpi eq, %q7_dep1_word, %c1 : i64
      %q7_dep1_w2 = arith.cmpi eq, %q7_dep1_word, %c2 : i64
      %q7_dep1_sel1 = arith.select %q7_dep1_w1, %nr1, %nr3 : i64
      %q7_dep1_sel0 = arith.select %q7_dep1_w0, %nr0, %q7_dep1_sel1 : i64
      %q7_dep1_bits = arith.select %q7_dep1_w2, %nr2, %q7_dep1_sel0 : i64
      %q7_dep1_hit0 = arith.andi %q7_dep1_bits, %q7_dep1_bit : i64
      %q7_dep1_hit = arith.cmpi ne, %q7_dep1_hit0, %c0 : i64
      %q7_dep1_vbit = arith.constant 2 : i64
      %q7_dep1_vhit = arith.andi %q7_dvalid, %q7_dep1_vbit : i64
      %q7_dep1_used = arith.cmpi ne, %q7_dep1_vhit, %c0 : i64
      %q7_dep1_unused = arith.cmpi eq, %q7_dep1_used, %false : i1
      %q7_dep1_ready = arith.ori %q7_dep1_unused, %q7_dep1_hit : i1
      %q7_dep2_shift = arith.constant 27 : i64
      %q7_dep2_s = arith.shrui %q7_desc, %q7_dep2_shift : i64
      %q7_dep2 = arith.andi %q7_dep2_s, %mask255 : i64
      %q7_dep2_word = arith.shrui %q7_dep2, %c6 : i64
      %q7_dep2_off = arith.andi %q7_dep2, %mask63 : i64
      %q7_dep2_bit = arith.shli %c1, %q7_dep2_off : i64
      %q7_dep2_w0 = arith.cmpi eq, %q7_dep2_word, %c0 : i64
      %q7_dep2_w1 = arith.cmpi eq, %q7_dep2_word, %c1 : i64
      %q7_dep2_w2 = arith.cmpi eq, %q7_dep2_word, %c2 : i64
      %q7_dep2_sel1 = arith.select %q7_dep2_w1, %nr1, %nr3 : i64
      %q7_dep2_sel0 = arith.select %q7_dep2_w0, %nr0, %q7_dep2_sel1 : i64
      %q7_dep2_bits = arith.select %q7_dep2_w2, %nr2, %q7_dep2_sel0 : i64
      %q7_dep2_hit0 = arith.andi %q7_dep2_bits, %q7_dep2_bit : i64
      %q7_dep2_hit = arith.cmpi ne, %q7_dep2_hit0, %c0 : i64
      %q7_dep2_vbit = arith.constant 4 : i64
      %q7_dep2_vhit = arith.andi %q7_dvalid, %q7_dep2_vbit : i64
      %q7_dep2_used = arith.cmpi ne, %q7_dep2_vhit, %c0 : i64
      %q7_dep2_unused = arith.cmpi eq, %q7_dep2_used, %false : i1
      %q7_dep2_ready = arith.ori %q7_dep2_unused, %q7_dep2_hit : i1
      %q7_deps01 = arith.andi %q7_dep0_ready, %q7_dep1_ready : i1
      %q7_deps = arith.andi %q7_deps01, %q7_dep2_ready : i1
      %q7_vbit = arith.constant 128 : i64
      %q7_vhit = arith.andi %valid_bits, %q7_vbit : i64
      %q7_occupied = arith.cmpi ne, %q7_vhit, %c0 : i64
      %q7_eligible = arith.andi %q7_occupied, %q7_deps : i1
      %oldest_init = arith.constant 256 : i64
      %index_init = arith.constant 8 : i64
      %q0_older = arith.cmpi ult, %s0, %oldest_init : i64
      %q0_choose = arith.andi %q0_eligible, %q0_older : i1
      %oldest0 = arith.select %q0_choose, %s0, %oldest_init : i64
      %index0 = arith.select %q0_choose, %c0, %index_init : i64
      %q1_older = arith.cmpi ult, %s1, %oldest0 : i64
      %q1_choose = arith.andi %q1_eligible, %q1_older : i1
      %oldest1 = arith.select %q1_choose, %s1, %oldest0 : i64
      %index1 = arith.select %q1_choose, %c1, %index0 : i64
      %q2_older = arith.cmpi ult, %s2, %oldest1 : i64
      %q2_choose = arith.andi %q2_eligible, %q2_older : i1
      %oldest2 = arith.select %q2_choose, %s2, %oldest1 : i64
      %index2 = arith.select %q2_choose, %c2, %index1 : i64
      %q3_older = arith.cmpi ult, %s3, %oldest2 : i64
      %q3_choose = arith.andi %q3_eligible, %q3_older : i1
      %oldest3 = arith.select %q3_choose, %s3, %oldest2 : i64
      %index3 = arith.select %q3_choose, %c3, %index2 : i64
      %q4_older = arith.cmpi ult, %s4, %oldest3 : i64
      %q4_choose = arith.andi %q4_eligible, %q4_older : i1
      %oldest4 = arith.select %q4_choose, %s4, %oldest3 : i64
      %index4 = arith.select %q4_choose, %c4, %index3 : i64
      %q5_older = arith.cmpi ult, %s5, %oldest4 : i64
      %q5_choose = arith.andi %q5_eligible, %q5_older : i1
      %oldest5 = arith.select %q5_choose, %s5, %oldest4 : i64
      %index5 = arith.select %q5_choose, %c5, %index4 : i64
      %q6_older = arith.cmpi ult, %s6, %oldest5 : i64
      %q6_choose = arith.andi %q6_eligible, %q6_older : i1
      %oldest6 = arith.select %q6_choose, %s6, %oldest5 : i64
      %index6 = arith.select %q6_choose, %c6, %index5 : i64
      %q7_older = arith.cmpi ult, %s7, %oldest6 : i64
      %q7_choose = arith.andi %q7_eligible, %q7_older : i1
      %oldest7 = arith.select %q7_choose, %s7, %oldest6 : i64
      %index7 = arith.select %q7_choose, %c7, %index6 : i64
      %has_issue = arith.cmpi ne, %index7, %c8 : i64
      %issued = scf.if %has_issue -> i1 {
        %sent = ac.try_send @Core::@iq_to_eng_t %oldest7 : i64
        scf.yield %sent : i1
      } else {
        scf.yield %false : i1
      }
      %clear_mask0 = arith.constant -2 : i64
      %cleared0 = arith.andi %valid_bits, %clear_mask0 : i64
      %issued_slot0a = arith.cmpi eq, %index7, %c0 : i64
      %issued_slot0 = arith.andi %issued, %issued_slot0a : i1
      %va0 = arith.select %issued_slot0, %cleared0, %valid_bits : i64
      %clear_mask1 = arith.constant -3 : i64
      %cleared1 = arith.andi %va0, %clear_mask1 : i64
      %issued_slot1a = arith.cmpi eq, %index7, %c1 : i64
      %issued_slot1 = arith.andi %issued, %issued_slot1a : i1
      %va1 = arith.select %issued_slot1, %cleared1, %va0 : i64
      %clear_mask2 = arith.constant -5 : i64
      %cleared2 = arith.andi %va1, %clear_mask2 : i64
      %issued_slot2a = arith.cmpi eq, %index7, %c2 : i64
      %issued_slot2 = arith.andi %issued, %issued_slot2a : i1
      %va2 = arith.select %issued_slot2, %cleared2, %va1 : i64
      %clear_mask3 = arith.constant -9 : i64
      %cleared3 = arith.andi %va2, %clear_mask3 : i64
      %issued_slot3a = arith.cmpi eq, %index7, %c3 : i64
      %issued_slot3 = arith.andi %issued, %issued_slot3a : i1
      %va3 = arith.select %issued_slot3, %cleared3, %va2 : i64
      %clear_mask4 = arith.constant -17 : i64
      %cleared4 = arith.andi %va3, %clear_mask4 : i64
      %issued_slot4a = arith.cmpi eq, %index7, %c4 : i64
      %issued_slot4 = arith.andi %issued, %issued_slot4a : i1
      %va4 = arith.select %issued_slot4, %cleared4, %va3 : i64
      %clear_mask5 = arith.constant -33 : i64
      %cleared5 = arith.andi %va4, %clear_mask5 : i64
      %issued_slot5a = arith.cmpi eq, %index7, %c5 : i64
      %issued_slot5 = arith.andi %issued, %issued_slot5a : i1
      %va5 = arith.select %issued_slot5, %cleared5, %va4 : i64
      %clear_mask6 = arith.constant -65 : i64
      %cleared6 = arith.andi %va5, %clear_mask6 : i64
      %issued_slot6a = arith.cmpi eq, %index7, %c6 : i64
      %issued_slot6 = arith.andi %issued, %issued_slot6a : i1
      %va6 = arith.select %issued_slot6, %cleared6, %va5 : i64
      %clear_mask7 = arith.constant -129 : i64
      %cleared7 = arith.andi %va6, %clear_mask7 : i64
      %issued_slot7a = arith.cmpi eq, %index7, %c7 : i64
      %issued_slot7 = arith.andi %issued, %issued_slot7a : i1
      %va7 = arith.select %issued_slot7, %cleared7, %va6 : i64
      %full = arith.cmpi eq, %va7, %c255 : i64
      %not_full = arith.cmpi eq, %full, %false : i1
      %incoming, %has_incoming = scf.if %not_full -> (i64, i1) {
        %value, %ok = ac.try_recv @Core::@dispatch_to_iq_t : i64
        scf.yield %value, %ok : i64, i1
      } else {
        scf.yield %c0, %false : i64, i1
      }
      %ins_bit7 = arith.constant 128 : i64
      %ins_hit7 = arith.andi %va7, %ins_bit7 : i64
      %empty7 = arith.cmpi eq, %ins_hit7, %c0 : i64
      %ins7 = arith.select %empty7, %c7, %c8 : i64
      %ins_bit6 = arith.constant 64 : i64
      %ins_hit6 = arith.andi %va7, %ins_bit6 : i64
      %empty6 = arith.cmpi eq, %ins_hit6, %c0 : i64
      %ins6 = arith.select %empty6, %c6, %ins7 : i64
      %ins_bit5 = arith.constant 32 : i64
      %ins_hit5 = arith.andi %va7, %ins_bit5 : i64
      %empty5 = arith.cmpi eq, %ins_hit5, %c0 : i64
      %ins5 = arith.select %empty5, %c5, %ins6 : i64
      %ins_bit4 = arith.constant 16 : i64
      %ins_hit4 = arith.andi %va7, %ins_bit4 : i64
      %empty4 = arith.cmpi eq, %ins_hit4, %c0 : i64
      %ins4 = arith.select %empty4, %c4, %ins5 : i64
      %ins_bit3 = arith.constant 8 : i64
      %ins_hit3 = arith.andi %va7, %ins_bit3 : i64
      %empty3 = arith.cmpi eq, %ins_hit3, %c0 : i64
      %ins3 = arith.select %empty3, %c3, %ins4 : i64
      %ins_bit2 = arith.constant 4 : i64
      %ins_hit2 = arith.andi %va7, %ins_bit2 : i64
      %empty2 = arith.cmpi eq, %ins_hit2, %c0 : i64
      %ins2 = arith.select %empty2, %c2, %ins3 : i64
      %ins_bit1 = arith.constant 2 : i64
      %ins_hit1 = arith.andi %va7, %ins_bit1 : i64
      %empty1 = arith.cmpi eq, %ins_hit1, %c0 : i64
      %ins1 = arith.select %empty1, %c1, %ins2 : i64
      %ins_bit0 = arith.constant 1 : i64
      %ins_hit0 = arith.andi %va7, %ins_bit0 : i64
      %empty0 = arith.cmpi eq, %ins_hit0, %c0 : i64
      %ins0 = arith.select %empty0, %c0, %ins1 : i64
      %put0a = arith.cmpi eq, %ins0, %c0 : i64
      %put0 = arith.andi %has_incoming, %put0a : i1
      %ns0 = arith.select %put0, %incoming, %s0 : i64
      %put1a = arith.cmpi eq, %ins0, %c1 : i64
      %put1 = arith.andi %has_incoming, %put1a : i1
      %ns1 = arith.select %put1, %incoming, %s1 : i64
      %put2a = arith.cmpi eq, %ins0, %c2 : i64
      %put2 = arith.andi %has_incoming, %put2a : i1
      %ns2 = arith.select %put2, %incoming, %s2 : i64
      %put3a = arith.cmpi eq, %ins0, %c3 : i64
      %put3 = arith.andi %has_incoming, %put3a : i1
      %ns3 = arith.select %put3, %incoming, %s3 : i64
      %put4a = arith.cmpi eq, %ins0, %c4 : i64
      %put4 = arith.andi %has_incoming, %put4a : i1
      %ns4 = arith.select %put4, %incoming, %s4 : i64
      %put5a = arith.cmpi eq, %ins0, %c5 : i64
      %put5 = arith.andi %has_incoming, %put5a : i1
      %ns5 = arith.select %put5, %incoming, %s5 : i64
      %put6a = arith.cmpi eq, %ins0, %c6 : i64
      %put6 = arith.andi %has_incoming, %put6a : i1
      %ns6 = arith.select %put6, %incoming, %s6 : i64
      %put7a = arith.cmpi eq, %ins0, %c7 : i64
      %put7 = arith.andi %has_incoming, %put7a : i1
      %ns7 = arith.select %put7, %incoming, %s7 : i64
      %with0 = arith.ori %va7, %ins_bit0 : i64
      %nv0 = arith.select %put0, %with0, %va7 : i64
      %with1 = arith.ori %nv0, %ins_bit1 : i64
      %nv1 = arith.select %put1, %with1, %nv0 : i64
      %with2 = arith.ori %nv1, %ins_bit2 : i64
      %nv2 = arith.select %put2, %with2, %nv1 : i64
      %with3 = arith.ori %nv2, %ins_bit3 : i64
      %nv3 = arith.select %put3, %with3, %nv2 : i64
      %with4 = arith.ori %nv3, %ins_bit4 : i64
      %nv4 = arith.select %put4, %with4, %nv3 : i64
      %with5 = arith.ori %nv4, %ins_bit5 : i64
      %nv5 = arith.select %put5, %with5, %nv4 : i64
      %with6 = arith.ori %nv5, %ins_bit6 : i64
      %nv6 = arith.select %put6, %with6, %nv5 : i64
      %with7 = arith.ori %nv6, %ins_bit7 : i64
      %nv7 = arith.select %put7, %with7, %nv6 : i64
      %valid_stored = ac.try_send @valid %nv7 : i64
      %slot0_stored = ac.try_send @slot0 %ns0 : i64
      %slot1_stored = ac.try_send @slot1 %ns1 : i64
      %slot2_stored = ac.try_send @slot2 %ns2 : i64
      %slot3_stored = ac.try_send @slot3 %ns3 : i64
      %slot4_stored = ac.try_send @slot4 %ns4 : i64
      %slot5_stored = ac.try_send @slot5 %ns5 : i64
      %slot6_stored = ac.try_send @slot6 %ns6 : i64
      %slot7_stored = ac.try_send @slot7 %ns7 : i64
      %ready0_stored = ac.try_send @ready0 %nr0 : i64
      %ready1_stored = ac.try_send @ready1 %nr1 : i64
      %ready2_stored = ac.try_send @ready2 %nr2 : i64
      %ready3_stored = ac.try_send @ready3 %nr3 : i64
      ac.yield_sim
    }
    ac.return
  }

  ac.module @EngineS() parameters {} graph {
    ac.queue @busy payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}
    ac.queue @remain payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "remain" path "remain" watermarks {kind = "register"}
    ac.queue @current payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "current" path "current" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      %busy, %busy_valid = ac.try_recv @busy : i64
      %remain, %remain_valid = ac.try_recv @remain : i64
      %current, %current_valid = ac.try_recv @current : i64
      %idle = arith.cmpi eq, %busy, %c0 : i64
      scf.if %idle {
        %handle, %valid = ac.try_recv @Core::@iq_to_eng_s : i64
        scf.if %valid {
          // Scalar TASSIGN latency is an architectural one-cycle constant.
          %set_busy = ac.try_send @busy %c1 : i64
          %set_remain = ac.try_send @remain %c1 : i64
          %set_current = ac.try_send @current %handle : i64
        } else {
          %keep_busy = ac.try_send @busy %c0 : i64
          %keep_remain = ac.try_send @remain %c0 : i64
          %keep_current = ac.try_send @current %current : i64
        }
      } else {
        %done = arith.cmpi ule, %remain, %c1 : i64
        scf.if %done {
          %sent = ac.try_send @Core::@wakeup %current : i64
          %next_busy = arith.select %sent, %c0, %c1 : i64
          %busy_stored = ac.try_send @busy %next_busy : i64
          %remain_stored = ac.try_send @remain %remain : i64
          %current_stored = ac.try_send @current %current : i64
        } else {
          %next_remain = arith.subi %remain, %c1 : i64
          %busy_stored = ac.try_send @busy %c1 : i64
          %remain_stored = ac.try_send @remain %next_remain : i64
          %current_stored = ac.try_send @current %current : i64
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @EngineV() parameters {} graph {
    ac.queue @busy payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}
    ac.queue @remain payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "remain" path "remain" watermarks {kind = "register"}
    ac.queue @current payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "current" path "current" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      // Vector architecture: 512 bytes/cycle plus 4 cycles setup.
      %c4 = arith.constant 4 : i64
      %c38 = arith.constant 38 : i64
      %c511 = arith.constant 511 : i64
      %c512 = arith.constant 512 : i64
      %busy, %busy_valid = ac.try_recv @busy : i64
      %remain, %remain_valid = ac.try_recv @remain : i64
      %current, %current_valid = ac.try_recv @current : i64
      %idle = arith.cmpi eq, %busy, %c0 : i64
      scf.if %idle {
        %handle, %valid = ac.try_recv @Core::@iq_to_eng_v : i64
        scf.if %valid {
          %desc = ac.trace.decode %handle : i64 to i64
          %workload = arith.shrui %desc, %c38 : i64
          %rounded = arith.addi %workload, %c511 : i64
          %transfer_cycles = arith.divui %rounded, %c512 : i64
          %latency = arith.addi %transfer_cycles, %c4 : i64
          %set_busy = ac.try_send @busy %c1 : i64
          %set_remain = ac.try_send @remain %latency : i64
          %set_current = ac.try_send @current %handle : i64
        } else {
          %keep_busy = ac.try_send @busy %c0 : i64
          %keep_remain = ac.try_send @remain %c0 : i64
          %keep_current = ac.try_send @current %current : i64
        }
      } else {
        %done = arith.cmpi ule, %remain, %c1 : i64
        scf.if %done {
          %sent = ac.try_send @Core::@wakeup %current : i64
          %next_busy = arith.select %sent, %c0, %c1 : i64
          %busy_stored = ac.try_send @busy %next_busy : i64
          %remain_stored = ac.try_send @remain %remain : i64
          %current_stored = ac.try_send @current %current : i64
        } else {
          %next_remain = arith.subi %remain, %c1 : i64
          %busy_stored = ac.try_send @busy %c1 : i64
          %remain_stored = ac.try_send @remain %next_remain : i64
          %current_stored = ac.try_send @current %current : i64
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @EngineC() parameters {} graph {
    ac.queue @busy payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}
    ac.queue @remain payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "remain" path "remain" watermarks {kind = "register"}
    ac.queue @current payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "current" path "current" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      // Cube architecture: 4096 scalar MAC operations per cycle.
      %c38 = arith.constant 38 : i64
      %c4095 = arith.constant 4095 : i64
      %c4096 = arith.constant 4096 : i64
      %busy, %busy_valid = ac.try_recv @busy : i64
      %remain, %remain_valid = ac.try_recv @remain : i64
      %current, %current_valid = ac.try_recv @current : i64
      %idle = arith.cmpi eq, %busy, %c0 : i64
      scf.if %idle {
        %handle, %valid = ac.try_recv @Core::@iq_to_eng_c : i64
        scf.if %valid {
          %desc = ac.trace.decode %handle : i64 to i64
          %workload = arith.shrui %desc, %c38 : i64
          %rounded = arith.addi %workload, %c4095 : i64
          %raw_latency = arith.divui %rounded, %c4096 : i64
          %has_work = arith.cmpi ne, %workload, %c0 : i64
          %latency = arith.select %has_work, %raw_latency, %c1 : i64
          %set_busy = ac.try_send @busy %c1 : i64
          %set_remain = ac.try_send @remain %latency : i64
          %set_current = ac.try_send @current %handle : i64
        } else {
          %keep_busy = ac.try_send @busy %c0 : i64
          %keep_remain = ac.try_send @remain %c0 : i64
          %keep_current = ac.try_send @current %current : i64
        }
      } else {
        %done = arith.cmpi ule, %remain, %c1 : i64
        scf.if %done {
          %sent = ac.try_send @Core::@wakeup %current : i64
          %next_busy = arith.select %sent, %c0, %c1 : i64
          %busy_stored = ac.try_send @busy %next_busy : i64
          %remain_stored = ac.try_send @remain %remain : i64
          %current_stored = ac.try_send @current %current : i64
        } else {
          %next_remain = arith.subi %remain, %c1 : i64
          %busy_stored = ac.try_send @busy %c1 : i64
          %remain_stored = ac.try_send @remain %next_remain : i64
          %current_stored = ac.try_send @current %current : i64
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @EngineT() parameters {} graph {
    ac.queue @busy payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "busy" path "busy" watermarks {kind = "register"}
    ac.queue @remain payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "remain" path "remain" watermarks {kind = "register"}
    ac.queue @current payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "current" path "current" watermarks {kind = "register"}
    ac.process @step kind "control" {
      %c0 = arith.constant 0 : i64
      %c1 = arith.constant 1 : i64
      // TMA architecture: 512 transferred bytes per cycle.
      %c38 = arith.constant 38 : i64
      %c511 = arith.constant 511 : i64
      %c512 = arith.constant 512 : i64
      %busy, %busy_valid = ac.try_recv @busy : i64
      %remain, %remain_valid = ac.try_recv @remain : i64
      %current, %current_valid = ac.try_recv @current : i64
      %idle = arith.cmpi eq, %busy, %c0 : i64
      scf.if %idle {
        %handle, %valid = ac.try_recv @Core::@iq_to_eng_t : i64
        scf.if %valid {
          %desc = ac.trace.decode %handle : i64 to i64
          %workload = arith.shrui %desc, %c38 : i64
          %rounded = arith.addi %workload, %c511 : i64
          %raw_latency = arith.divui %rounded, %c512 : i64
          %has_work = arith.cmpi ne, %workload, %c0 : i64
          %latency = arith.select %has_work, %raw_latency, %c1 : i64
          %set_busy = ac.try_send @busy %c1 : i64
          %set_remain = ac.try_send @remain %latency : i64
          %set_current = ac.try_send @current %handle : i64
        } else {
          %keep_busy = ac.try_send @busy %c0 : i64
          %keep_remain = ac.try_send @remain %c0 : i64
          %keep_current = ac.try_send @current %current : i64
        }
      } else {
        %done = arith.cmpi ule, %remain, %c1 : i64
        scf.if %done {
          %sent = ac.try_send @Core::@wakeup %current : i64
          %next_busy = arith.select %sent, %c0, %c1 : i64
          %busy_stored = ac.try_send @busy %next_busy : i64
          %remain_stored = ac.try_send @remain %remain : i64
          %current_stored = ac.try_send @current %current : i64
        } else {
          %next_remain = arith.subi %remain, %c1 : i64
          %busy_stored = ac.try_send @busy %c1 : i64
          %remain_stored = ac.try_send @remain %next_remain : i64
          %current_stored = ac.try_send @current %current : i64
        }
      }
      ac.yield_sim
    }
    ac.return
  }

  ac.module @Core() parameters {} graph {
    ac.queue @clock payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "clock" path "clock" watermarks {kind = "register"}
    ac.queue @trace_total payload i64 entries 1 ordering "fifo" protocol @rv
        ownership "exclusive" id "trace_total" path "trace_total" watermarks {kind = "register"}
    ac.queue @rob_in payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "rob_in" path "rob_in"
    ac.queue @rob_to_rename payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "rob_to_rename" path "rob_to_rename"
    ac.queue @rename_to_dispatch payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "rename_to_dispatch" path "rename_to_dispatch"
    ac.queue @dispatch_to_iq_s payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "dispatch_to_iq_s" path "dispatch_to_iq_s"
    ac.queue @dispatch_to_iq_v payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "dispatch_to_iq_v" path "dispatch_to_iq_v"
    ac.queue @dispatch_to_iq_c payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "dispatch_to_iq_c" path "dispatch_to_iq_c"
    ac.queue @dispatch_to_iq_t payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "dispatch_to_iq_t" path "dispatch_to_iq_t"
    ac.queue @iq_to_eng_s payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "iq_to_eng_s" path "iq_to_eng_s"
    ac.queue @iq_to_eng_v payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "iq_to_eng_v" path "iq_to_eng_v"
    ac.queue @iq_to_eng_c payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "iq_to_eng_c" path "iq_to_eng_c"
    ac.queue @iq_to_eng_t payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "iq_to_eng_t" path "iq_to_eng_t"
    ac.queue @wakeup payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "wakeup" path "wakeup"
    ac.queue @rob_done payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "rob_done" path "rob_done"
    ac.queue @ready_to_iq_s payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready_to_iq_s" path "ready_to_iq_s"
    ac.queue @ready_to_iq_v payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready_to_iq_v" path "ready_to_iq_v"
    ac.queue @ready_to_iq_c payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready_to_iq_c" path "ready_to_iq_c"
    ac.queue @ready_to_iq_t payload i64 entries 136 ordering "fifo" protocol @rv
        ownership "exclusive" id "ready_to_iq_t" path "ready_to_iq_t"
    ac.instance @trace of @TraceSource() static {} id "trace" path "trace" : () -> ()
    ac.instance @rob of @ROB() static {} id "rob" path "rob" : () -> ()
    ac.instance @rename of @Rename() static {} id "rename" path "rename" : () -> ()
    ac.instance @dispatch of @Dispatch() static {} id "dispatch" path "dispatch" : () -> ()
    ac.instance @broadcast of @CompletionBroadcast() static {} id "broadcast" path "broadcast" : () -> ()
    ac.instance @iq_s of @IssueQueueS() static {} id "iq_s" path "iq_s" : () -> ()
    ac.instance @iq_v of @IssueQueueV() static {} id "iq_v" path "iq_v" : () -> ()
    ac.instance @iq_c of @IssueQueueC() static {} id "iq_c" path "iq_c" : () -> ()
    ac.instance @iq_t of @IssueQueueT() static {} id "iq_t" path "iq_t" : () -> ()
    ac.instance @eng_s of @EngineS() static {} id "eng_s" path "eng_s" : () -> ()
    ac.instance @eng_v of @EngineV() static {} id "eng_v" path "eng_v" : () -> ()
    ac.instance @eng_c of @EngineC() static {} id "eng_c" path "eng_c" : () -> ()
    ac.instance @eng_t of @EngineT() static {} id "eng_t" path "eng_t" : () -> ()
    ac.process @tick kind "workload" {
      %clock, %valid = ac.try_recv @clock : i64
      ac.yield_sim
    }
    ac.return
  }

  ac.system @soc root @Core as "root" tick 0 "cycle"
      workload @Core::@tick seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
