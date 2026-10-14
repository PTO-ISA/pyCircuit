module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, format_version = 9 : i64, top = "MixedPorts", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 0 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.result", source_branch = 3 : i64, target_branches = [4], to = 1 : i64}], logical_states = [{count = "top.accepted$count", kind = "queue", name = "top.accepted", parts = [{path = [0], resource = "top.accepted$storage$0"}, {path = [1], resource = "top.accepted$storage$1"}], prototype = [0 : i32, 0 : i32]}, {kind = "cell", name = "top.last_accepted", parts = [{path = [], resource = "top.last_accepted$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.ledger.observed", parts = [{path = [], resource = "top.ledger.observed$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.observer.last", parts = [{path = [], resource = "top.observer.last$value"}], prototype = 0 : i32}, {count = "top.source$count", kind = "queue", name = "top.source", parts = [{path = [0], resource = "top.source$storage$0"}], prototype = [7 : i32]}], schedule = [0, 1], stage = "rtl"} {
    acir.state @top.accepted$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.accepted$storage$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.accepted$storage$1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.last_accepted$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.ledger.observed$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.observer.last$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.source$count : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.source$storage$0 : i32 {initial = 7 : i32, kind = "cell"}
    %0 = acir.current {aspect = "value", resource = "top.source$count"} : i32 loc(#loc1)
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    %1 = arith.cmpi ne, %0, %c0_i32 : i32 loc(#loc1)
    %true = arith.constant true
    %2 = arith.andi %1, %true : i1
    %c0_i32_0 = arith.constant 0 : i32 loc(#loc1)
    %3 = arith.cmpi eq, %0, %c0_i32_0 : i32 loc(#loc1)
    %true_1 = arith.constant true
    %4 = arith.xori %3, %true_1 : i1
    %5 = arith.andi %2, %4 : i1
    %6 = arith.andi %5, %1 : i1
    %7 = acir.export %true_1 {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1
    %8 = acir.current {aspect = "value", resource = "top.source$storage$0"} : i32 loc(#loc1)
    %9 = acir.aggregate(%8) : (i32) -> tuple<i32> loc(#loc1)
    %c0_i32_2 = arith.constant 0 : i32 loc(#loc1)
    %c0_i32_3 = arith.constant 0 : i32 loc(#loc1)
    %10 = acir.extract %9[%c0_i32_3] : (tuple<i32>, i32) -> i32 loc(#loc1)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %11 = arith.addi %10, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32
    %12 = acir.aggregate(%11, %10, %10, %true_1, %true_1, %true_1) : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1>
    %c0_i32_4 = arith.constant 0 : i32 loc(#loc2)
    %c0_i32_5 = arith.constant 0 : i32 loc(#loc2)
    %false = arith.constant false
    %13 = acir.aggregate(%c0_i32_4, %c0_i32_5, %10, %false, %false, %true_1) : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1>
    %c0_i32_6 = arith.constant 0 : i32
    %14 = acir.extract %12[%c0_i32_6] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %c0_i32_7 = arith.constant 0 : i32
    %15 = acir.extract %13[%c0_i32_7] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %16 = arith.select %7, %14, %15 : i32
    %c1_i32_8 = arith.constant 1 : i32
    %17 = acir.extract %12[%c1_i32_8] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %c1_i32_9 = arith.constant 1 : i32
    %18 = acir.extract %13[%c1_i32_9] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %19 = arith.select %7, %17, %18 : i32
    %c2_i32 = arith.constant 2 : i32
    %20 = acir.extract %12[%c2_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %c2_i32_10 = arith.constant 2 : i32
    %21 = acir.extract %13[%c2_i32_10] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %22 = arith.select %7, %20, %21 : i32
    %c3_i32 = arith.constant 3 : i32
    %23 = acir.extract %12[%c3_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %c3_i32_11 = arith.constant 3 : i32
    %24 = acir.extract %13[%c3_i32_11] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %25 = arith.select %7, %23, %24 : i1
    %c4_i32 = arith.constant 4 : i32
    %26 = acir.extract %12[%c4_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %c4_i32_12 = arith.constant 4 : i32
    %27 = acir.extract %13[%c4_i32_12] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %28 = arith.select %7, %26, %27 : i1
    %c5_i32 = arith.constant 5 : i32
    %29 = acir.extract %12[%c5_i32] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %c5_i32_13 = arith.constant 5 : i32
    %30 = acir.extract %13[%c5_i32_13] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %31 = arith.select %7, %29, %30 : i1
    %32 = acir.aggregate(%16, %19, %22, %25, %28, %31) : (i32, i32, i32, i1, i1, i1) -> tuple<i32, i32, i32, i1, i1, i1>
    %c3_i32_14 = arith.constant 3 : i32
    %c3_i32_15 = arith.constant 3 : i32
    %33 = acir.extract %32[%c3_i32_15] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %34 = arith.xori %33, %true_1 : i1
    %35 = arith.xori %7, %true_1 : i1
    %36 = arith.andi %7, %1 : i1
    %37 = arith.ori %35, %36 : i1
    %38 = arith.andi %7, %37 : i1
    %39 = arith.andi %35, %1 : i1
    %40 = arith.ori %38, %39 : i1
    %41 = arith.ori %34, %40 : i1
    %42 = arith.andi %6, %41 : i1
    %43 = arith.ori %34, %true : i1
    %44 = arith.andi %42, %43 : i1
    %45 = acir.current {aspect = "value", resource = "top.accepted$count"} : i32
    %c0_i32_16 = arith.constant 0 : i32
    %46 = arith.subi %45, %c0_i32_16 : i32
    %c2_i32_17 = arith.constant 2 : i32
    %47 = arith.cmpi uge, %46, %c2_i32_17 : i32
    %48 = arith.xori %47, %true_1 : i1
    %49 = arith.ori %34, %48 : i1
    %50 = arith.andi %44, %49 : i1
    %c4_i32_18 = arith.constant 4 : i32
    %c4_i32_19 = arith.constant 4 : i32
    %51 = acir.extract %32[%c4_i32_19] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %52 = arith.xori %51, %true_1 : i1
    %53 = arith.ori %52, %40 : i1
    %54 = arith.andi %50, %53 : i1
    %c5_i32_20 = arith.constant 5 : i32
    %c5_i32_21 = arith.constant 5 : i32
    %55 = acir.extract %32[%c5_i32_21] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i1
    %56 = arith.xori %55, %true_1 : i1
    %57 = arith.ori %56, %40 : i1
    %58 = arith.andi %54, %57 : i1
    %59 = acir.fire "top.router.route#0" %58 {successors = [1], txn = 0 : i64}
    %false_22 = arith.constant false
    %60 = arith.andi %59, %55 : i1
    %61 = arith.ori %false_22, %60 : i1
    %62 = acir.fire "top.observer.sample#1" %61 {successors = [], txn = 1 : i64}
    %63 = arith.andi %59, %33 : i1
    %c0_i32_23 = arith.constant 0 : i32
    %c0_i32_24 = arith.constant 0 : i32
    %64 = arith.cmpi eq, %c0_i32_23, %c0_i32_24 : i32
    %65 = acir.current {aspect = "value", resource = "top.accepted$storage$0"} : i32
    %66 = acir.current {aspect = "value", resource = "top.accepted$storage$1"} : i32
    %67 = acir.aggregate(%65, %66) : (i32, i32) -> tuple<i32, i32>
    %c0_i32_25 = arith.constant 0 : i32
    %c0_i32_26 = arith.constant 0 : i32
    %68 = acir.extract %67[%c0_i32_26] : (tuple<i32, i32>, i32) -> i32
    %c1_i32_27 = arith.constant 1 : i32
    %69 = arith.cmpi eq, %c0_i32_23, %c1_i32_27 : i32
    %c1_i32_28 = arith.constant 1 : i32
    %c1_i32_29 = arith.constant 1 : i32
    %70 = acir.extract %67[%c1_i32_29] : (tuple<i32, i32>, i32) -> i32
    %c0_i32_30 = arith.constant 0 : i32
    %71 = arith.select %69, %70, %c0_i32_30 : i32
    %72 = arith.select %64, %68, %71 : i32
    %c0_i32_31 = arith.constant 0 : i32
    %73 = arith.cmpi eq, %c0_i32_23, %c0_i32_31 : i32
    %c1_i32_32 = arith.constant 1 : i32
    %c1_i32_33 = arith.constant 1 : i32
    %74 = acir.extract %67[%c1_i32_33] : (tuple<i32, i32>, i32) -> i32
    %c0_i32_34 = arith.constant 0 : i32
    %75 = arith.select %73, %74, %c0_i32_34 : i32
    %76 = acir.aggregate(%72, %75) : (i32, i32) -> tuple<i32, i32>
    %c0_i32_35 = arith.constant 0 : i32
    %c0_i32_36 = arith.constant 0 : i32
    %77 = acir.extract %32[%c0_i32_36] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %78 = arith.subi %45, %c0_i32_23 : i32
    %c0_i32_37 = arith.constant 0 : i32
    %79 = arith.select %63, %78, %c0_i32_37 : i32
    %c0_i32_38 = arith.constant 0 : i32
    %80 = acir.extract %76[%c0_i32_38] : (tuple<i32, i32>, i32) -> i32
    %c0_i32_39 = arith.constant 0 : i32
    %81 = arith.cmpi eq, %79, %c0_i32_39 : i32
    %82 = arith.select %81, %77, %80 : i32
    %c1_i32_40 = arith.constant 1 : i32
    %83 = acir.extract %76[%c1_i32_40] : (tuple<i32, i32>, i32) -> i32
    %c1_i32_41 = arith.constant 1 : i32
    %84 = arith.cmpi eq, %79, %c1_i32_41 : i32
    %85 = arith.select %84, %77, %83 : i32
    %86 = acir.aggregate(%82, %85) : (i32, i32) -> tuple<i32, i32>
    %c0_i32_42 = arith.constant 0 : i32
    %87 = acir.extract %86[%c0_i32_42] : (tuple<i32, i32>, i32) -> i32
    %c0_i32_43 = arith.constant 0 : i32
    %88 = acir.extract %76[%c0_i32_43] : (tuple<i32, i32>, i32) -> i32
    %89 = arith.select %63, %87, %88 : i32
    %c1_i32_44 = arith.constant 1 : i32
    %90 = acir.extract %86[%c1_i32_44] : (tuple<i32, i32>, i32) -> i32
    %c1_i32_45 = arith.constant 1 : i32
    %91 = acir.extract %76[%c1_i32_45] : (tuple<i32, i32>, i32) -> i32
    %92 = arith.select %63, %90, %91 : i32
    %93 = acir.aggregate(%89, %92) : (i32, i32) -> tuple<i32, i32>
    %c0_i32_46 = arith.constant 0 : i32
    %94 = acir.extract %93[%c0_i32_46] : (tuple<i32, i32>, i32) -> i32
    acir.next %94 {aspect = "value", resource = "top.accepted$storage$0"} : i32
    %c1_i32_47 = arith.constant 1 : i32
    %95 = acir.extract %93[%c1_i32_47] : (tuple<i32, i32>, i32) -> i32
    acir.next %95 {aspect = "value", resource = "top.accepted$storage$1"} : i32
    %c1_i32_48 = arith.constant 1 : i32
    %c0_i32_49 = arith.constant 0 : i32
    %96 = arith.select %63, %c1_i32_48, %c0_i32_49 : i32
    %97 = arith.addi %78, %96 : i32
    acir.next %97 {aspect = "value", resource = "top.accepted$count"} : i32
    %98 = arith.andi %59, %51 : i1
    %c1_i32_50 = arith.constant 1 : i32
    %99 = acir.extract %32[%c1_i32_50] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %100 = acir.current {aspect = "value", resource = "top.last_accepted$value"} : i32
    %101 = arith.select %98, %99, %100 : i32
    acir.next %101 {aspect = "value", resource = "top.last_accepted$value"} : i32
    %102 = acir.current {aspect = "value", resource = "top.ledger.observed$value"} : i32
    %103 = arith.select %59, %10, %102 : i32
    acir.next %103 {aspect = "value", resource = "top.ledger.observed$value"} : i32
    %c2_i32_51 = arith.constant 2 : i32
    %c2_i32_52 = arith.constant 2 : i32
    %104 = acir.extract %32[%c2_i32_52] : (tuple<i32, i32, i32, i1, i1, i1>, i32) -> i32
    %c0_i32_53 = arith.constant 0 : i32 loc(#loc3)
    %105 = arith.select %60, %104, %c0_i32_53 : i32 loc(#loc3)
    %106 = acir.current {aspect = "value", resource = "top.observer.last$value"} : i32
    %107 = arith.select %62, %105, %106 : i32
    acir.next %107 {aspect = "value", resource = "top.observer.last$value"} : i32
    %c0_i32_54 = arith.constant 0 : i32
    %c1_i32_55 = arith.constant 1 : i32
    %c0_i32_56 = arith.constant 0 : i32
    %108 = arith.select %59, %c1_i32_55, %c0_i32_56 : i32
    %109 = arith.addi %c0_i32_54, %108 : i32
    %c0_i32_57 = arith.constant 0 : i32
    %110 = arith.cmpi eq, %109, %c0_i32_57 : i32
    %c0_i32_58 = arith.constant 0 : i32
    %c0_i32_59 = arith.constant 0 : i32
    %111 = acir.extract %9[%c0_i32_59] : (tuple<i32>, i32) -> i32
    %c0_i32_60 = arith.constant 0 : i32
    %112 = arith.select %110, %111, %c0_i32_60 : i32
    %113 = acir.aggregate(%112) : (i32) -> tuple<i32>
    %c0_i32_61 = arith.constant 0 : i32
    %114 = acir.extract %113[%c0_i32_61] : (tuple<i32>, i32) -> i32
    acir.next %114 {aspect = "value", resource = "top.source$storage$0"} : i32
    %115 = arith.subi %0, %109 : i32
    acir.next %115 {aspect = "value", resource = "top.source$count"} : i32
    %116 = arith.select %7, %35, %false : i1
    %117 = acir.unary %116 "not" {acir.source_type = "bool"} : i1 -> i1
    acir.effect %59(%117) {branch = 0 : i64, frames = ["top.router:route"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> ()
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":35:17)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":39:9)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/mixed_ports.py":22:16)
