// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/forked-cursor.mlir 2>&1 | %FileCheck %s --check-prefix=CURSOR
// RUN: %not %acir_opt %t/noncursor.mlir 2>&1 | %FileCheck %s --check-prefix=TYPE
// RUN: %not %acir_opt %t/wrong-owner.mlir 2>&1 | %FileCheck %s --check-prefix=OWNER
// RUN: %not %acir_opt %t/duplicate-document-owner.mlir 2>&1 | %FileCheck %s --check-prefix=MULTI-OWNER
// RUN: %not %acir_opt %t/duplicate-cursor.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE-CURSOR
// RUN: %not %acir_opt %t/source-is-not-path.mlir 2>&1 | %FileCheck %s --check-prefix=SOURCE-ID
// RUN: %not %acir_opt %t/forwarded-fork.mlir 2>&1 | %FileCheck %s --check-prefix=FORWARDED-FORK
// RUN: %not %acir_opt %t/ambiguous-merge.mlir 2>&1 | %FileCheck %s --check-prefix=AMBIGUOUS
// RUN: %not %acir_opt %t/cursor-noncursor-merge.mlir 2>&1 | %FileCheck %s --check-prefix=NONCURSOR-MERGE
// RUN: %not %acir_opt %t/for-induction-cursor.mlir 2>&1 | %FileCheck %s --check-prefix=FOR-INDUCTION
// RUN: %not %acir_opt %t/decode-non-next.mlir 2>&1 | %FileCheck %s --check-prefix=DECODE-NON-NEXT
// RUN: %not %acir_opt %t/decode-not-in-process.mlir 2>&1 | %FileCheck %s --check-prefix=DECODE-NOT-PROCESS
// RUN: %not %acir_opt %t/eof-non-cursor.mlir 2>&1 | %FileCheck %s --check-prefix=EOF-TYPE
// RUN: %not %acir_opt %t/position-non-cursor.mlir 2>&1 | %FileCheck %s --check-prefix=POSITION-TYPE

//--- forked-cursor.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "workload" {
      %cursor = ac.trace.open source "input"
      %next, %raw, %advanced = ac.trace.next %cursor from source "input" : i32
      %other, %raw2, %advanced2 = ac.trace.next %cursor from source "input" : i32
      ac.yield_sim
    }
    ac.return
  }
}
// CURSOR: trace cursor provenance has more than one advancing consumer

//--- noncursor.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "workload" {
      %bad = arith.constant 0 : index
      %next, %raw, %advanced = ac.trace.next %bad from source "input" : i32
      ac.yield_sim
    }
    ac.return
  }
}
// TYPE: trace cursor must originate from ac.trace.open or ac.trace.next

//--- wrong-owner.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "workload" {
      %cursor = ac.trace.open source "input"
      %next, %raw, %advanced = ac.trace.next %cursor from source "other" : i32
      ac.yield_sim
    }
    ac.return
  }
}
// OWNER: trace cursor owner does not match 'from source'

//--- duplicate-document-owner.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @left kind "workload" {
      %cursor = ac.trace.open source "pto"
      ac.yield_sim
    }
    ac.process @right kind "workload" {
      %cursor = ac.trace.open source "pto"
      ac.yield_sim
    }
    ac.return
  }
}
// MULTI-OWNER: trace source 'pto' must have exactly one cursor owner

//--- source-is-not-path.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @workload kind "workload" {
      %cursor = ac.trace.open source "traces/model.json"
      ac.yield_sim
    }
    ac.return
  }
}
// SOURCE-ID: trace source must be one stable logical identifier segment

//--- duplicate-cursor.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @workload kind "workload" {
      %first = ac.trace.open source "pto"
      %second = ac.trace.open source "pto"
      ac.yield_sim
    }
    ac.return
  }
}
// DUPLICATE-CURSOR: trace source 'pto' must have exactly one cursor owner

//--- forwarded-fork.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M(i1) parameters {} graph {
  ^bb0(%condition : i1):
    ac.process @p kind "workload" captures(%condition : i1) {
    ^bb0(%condition_copy : i1):
      %cursor = ac.trace.open source "pto"
      %forwarded = scf.if %condition_copy -> index {
        scf.yield %cursor : index
      } else {
        scf.yield %cursor : index
      }
      %next0, %raw0, %advanced0 = ac.trace.next %cursor from source "pto" : i32
      %next1, %raw1, %advanced1 = ac.trace.next %forwarded from source "pto" : i32
      ac.yield_sim
    }
    ac.return
  }
}
// FORWARDED-FORK: trace cursor provenance has more than one advancing consumer

//--- ambiguous-merge.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M(i1) parameters {} graph {
  ^bb0(%condition : i1):
    ac.process @p kind "workload" captures(%condition : i1) {
    ^bb0(%condition_copy : i1):
      %left = ac.trace.open source "left"
      %right = ac.trace.open source "right"
      %merged = scf.if %condition_copy -> index {
        scf.yield %left : index
      } else {
        scf.yield %right : index
      }
      %next, %raw, %advanced = ac.trace.next %merged from source "left" : i32
      ac.yield_sim
    }
    ac.return
  }
}
// AMBIGUOUS: trace cursor forwarding merges distinct provenance

//--- cursor-noncursor-merge.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M(i1) parameters {} graph {
  ^bb0(%condition : i1):
    ac.process @p kind "workload" captures(%condition : i1) {
    ^bb0(%branch : i1):
      %cursor = ac.trace.open source "pto"
      %ordinary = index.constant 0
      %merged = scf.if %branch -> index {
        scf.yield %cursor : index
      } else {
        scf.yield %ordinary : index
      }
      %next, %raw, %advanced = ac.trace.next %merged from source "pto" : i32
      ac.yield_sim
    }
    ac.return
  }
}
// NONCURSOR-MERGE: trace cursor forwarding merges cursor and non-cursor values

//--- for-induction-cursor.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "workload" {
      %cursor = ac.trace.open source "pto"
      %lb = index.constant 0
      %ub = index.constant 4
      %step = index.constant 1
      %laundered = scf.for %i = %lb to %ub step %step
          iter_args(%iter = %cursor) -> index {
        scf.yield %i : index
      }
      %next, %raw, %advanced = ac.trace.next %laundered from source "pto" : i32
      ac.yield_sim
    }
    ac.return
  }
}
// FOR-INDUCTION: trace cursor forwarding merges cursor and non-cursor values
// DECODE-NON-NEXT: trace.decode input must be an ac.trace.next entry
// DECODE-NOT-PROCESS: operation is not legal in an ac.module structural Graph region

//--- decode-non-next.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "workload" {
      %cursor = ac.trace.open source "input"
      %next, %raw, %advanced = ac.trace.next %cursor from source "input" : i32
      %bad = arith.constant 0 : i32
      %decoded = ac.trace.decode %bad : i32 to i64
      ac.yield_sim
    }
    ac.return
  }
}

//--- decode-not-in-process.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    %cursor = ac.trace.open source "input"
    %next, %raw, %advanced = ac.trace.next %cursor from source "input" : i32
    %decoded = ac.trace.decode %raw : i32 to i64
    ac.return
  }
}

//--- eof-non-cursor.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "workload" {
      %bad = arith.constant 1 : i32
      %eof = ac.trace.eof %bad from source "pto"
      ac.yield_sim
    }
    ac.return
  }
}
// EOF-TYPE: error: use of value '%bad' expects different type than prior uses: 'index' vs 'i32'

//--- position-non-cursor.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.process @p kind "workload" {
      %bad = arith.constant 1 : i32
      %position = ac.trace.position %bad from source "pto"
      ac.yield_sim
    }
    ac.return
  }
}
// POSITION-TYPE: error: use of value '%bad' expects different type than prior uses: 'index' vs 'i32'
