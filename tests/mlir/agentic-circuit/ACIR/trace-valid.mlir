// RUN: %acir_opt_public %s | %FileCheck %s
// RUN: %acir_opt_public %s | %acir_opt_public | %FileCheck %s
// RUN: %acir_opt_public --pass-pipeline='builtin.module(canonicalize,cse)' %s | %FileCheck %s --check-prefix=EFFECTS

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Trace() parameters {} graph {
    ac.stat @decoded kind "counter"
    ac.stat @position kind "gauge"
    ac.stat @eof kind "counter"
    ac.stat @observed kind "gauge"
    ac.process @decoder kind "monitor" { ac.yield_sim }
    ac.process @workload kind "workload" {
      %cursor0 = ac.trace.open source "pto"
      %cursor1, %raw, %advanced = ac.trace.next %cursor0 from source "pto" : i32
      %decoded = ac.trace.decode %raw : i32 to i64
      %position = ac.trace.position %cursor1 from source "pto"
      %eof = ac.trace.eof %cursor1 from source "pto"
      %observed = ac.probe @decoder kind "module" : i64
      %index_cursor0 = ac.trace.open source "index_records"
      %index_cursor1, %index_raw, %index_advanced = ac.trace.next %index_cursor0 from source "index_records" : index
      %index_decoded = ac.trace.decode %index_raw : index to i64
      ac.stat.add @decoded %decoded : i64
      ac.stat.add @position %position : index
      ac.stat.add @eof %eof : i1
      ac.stat.add @observed %observed : i64
      ac.stat.add @decoded %index_decoded : i64
      ac.yield_sim
    }
    ac.return
  }
  ac.module @LoopTrace(i1) parameters {} graph {
  ^bb0(%keep_running : i1):
    ac.stat @position kind "gauge"
    ac.process @workload kind "workload" captures(%keep_running : i1) {
    ^bb0(%condition : i1):
      %cursor0 = ac.trace.open source "runtime_loop"
      %cursor1 = scf.while (%cursor = %cursor0) : (index) -> index {
        scf.condition(%condition) %cursor : index
      } do {
      ^bb0(%cursor : index):
        %next, %raw, %advanced = ac.trace.next %cursor from source "runtime_loop" : i32
        ac.wait_until %advanced
        scf.yield %next : index
      }
      %position = ac.trace.position %cursor1 from source "runtime_loop"
      ac.stat.add @position %position : index
      ac.yield_sim
    }
    ac.return
  }
  ac.module @ForTrace() parameters {} graph {
    ac.stat @position kind "gauge"
    ac.stat @ordinary kind "gauge"
    ac.process @workload kind "workload" {
      %cursor0 = ac.trace.open source "for_loop"
      %lb = index.constant 0
      %ub = index.constant 4
      %step = index.constant 1
      %cursor1 = scf.for %i = %lb to %ub step %step
          iter_args(%cursor = %cursor0) -> index {
        scf.yield %cursor : index
      }
      %ordinary0 = index.constant 7
      %ordinary1 = scf.for %i = %lb to %ub step %step
          iter_args(%ordinary = %ordinary0) -> index {
        scf.yield %ordinary : index
      }
      %position = ac.trace.position %cursor1 from source "for_loop"
      ac.stat.add @position %position : index
      ac.stat.add @ordinary %ordinary1 : index
      ac.yield_sim
    }
    ac.return
  }
  ac.module @MergedTrace(i1) parameters {} graph {
  ^bb0(%condition : i1):
    ac.process @workload kind "workload" captures(%condition : i1) {
    ^bb0(%branch : i1):
      %cursor = ac.trace.open source "merged"
      %merged = scf.if %branch -> index {
        scf.yield %cursor : index
      } else {
        scf.yield %cursor : index
      }
      %next, %raw, %advanced = ac.trace.next %merged from source "merged" : i32
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK: ac.trace.open source "pto"
// CHECK: ac.trace.next
// CHECK: ac.trace.decode
// CHECK: ac.trace.position
// CHECK: ac.trace.eof
// CHECK: ac.module @LoopTrace
// CHECK: scf.while
// CHECK: ac.trace.next
// CHECK: ac.module @ForTrace
// CHECK: ac.module @MergedTrace
// EFFECTS: ac.trace.open
// EFFECTS: ac.trace.next
// EFFECTS: ac.trace.position
// EFFECTS: ac.trace.eof
// EFFECTS: ac.probe
// EFFECTS: ac.stat.add
// EFFECTS: ac.yield_sim
