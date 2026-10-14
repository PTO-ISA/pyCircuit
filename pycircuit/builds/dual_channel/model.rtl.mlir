module attributes {acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "DualChannel", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [], logical_states = [{count = "top.left_out$count", kind = "queue", name = "top.left_out", parts = [{path = [0], resource = "top.left_out$storage$0"}], prototype = [0 : i32]}, {count = "top.right_out$count", kind = "queue", name = "top.right_out", parts = [{path = [0], resource = "top.right_out$storage$0"}], prototype = [0 : i32]}, {count = "top.source$count", kind = "queue", name = "top.source", parts = [{path = [0], resource = "top.source$storage$0"}], prototype = [7 : i32]}], schedule = [0, 1], stage = "rtl"} {
    acir.state @top.left_out$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left_out$storage$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right_out$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right_out$storage$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.source$count : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.source$storage$0 : i32 {initial = 7 : i32, kind = "cell"}
    %0 = acir.current {aspect = "value", resource = "top.source$storage$0"} : i32 loc(#loc1)
    %1 = acir.aggregate(%0) : (i32) -> tuple<i32> loc(#loc1)
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    %c0_i32_0 = arith.constant 0 : i32 loc(#loc1)
    %2 = acir.extract %1[%c0_i32_0] : (tuple<i32>, i32) -> i32 loc(#loc1)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %3 = arith.addi %2, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32
    %4 = arith.cmpi ne, %3, %c0_i32_1 {acir.id = "v7", acir.source_type = "bool"} : i32
    %5 = arith.cmpi eq, %3, %c0_i32_1 : i32 loc(#loc2)
    %6 = acir.current {aspect = "value", resource = "top.source$count"} : i32 loc(#loc1)
    %c0_i32_2 = arith.constant 0 : i32 loc(#loc1)
    %7 = arith.cmpi ne, %6, %c0_i32_2 : i32 loc(#loc1)
    %false = arith.constant false
    %8 = acir.unary %false "not" {acir.source_type = "bool"} : i1 -> i1
    %9 = acir.assert %8
    %10 = arith.andi %7, %9 : i1
    %11 = arith.ori %5, %10 : i1
    %12 = arith.andi %11, %10 : i1
    %13 = arith.andi %4, %12 : i1
    %14 = arith.ori %5, %11 : i1 loc(#loc3)
    %15 = arith.andi %14, %13 : i1 loc(#loc3)
    %16 = arith.andi %4, %15 : i1 loc(#loc3)
    %true = arith.constant true
    %17 = arith.xori %16, %true : i1
    %18 = arith.ori %17, %11 : i1
    %19 = arith.andi %13, %18 : i1
    %true_3 = arith.constant true
    %20 = arith.andi %11, %true_3 : i1
    %21 = arith.ori %17, %20 : i1
    %22 = arith.andi %19, %21 : i1
    %23 = acir.current {aspect = "value", resource = "top.left_out$count"} : i32
    %c0_i32_4 = arith.constant 0 : i32
    %24 = arith.subi %23, %c0_i32_4 : i32
    %c1_i32_5 = arith.constant 1 : i32
    %25 = arith.cmpi uge, %24, %c1_i32_5 : i32
    %26 = arith.xori %25, %true : i1
    %27 = arith.andi %11, %26 : i1
    %28 = arith.ori %17, %27 : i1
    %29 = arith.andi %22, %28 : i1
    %30 = acir.fire "top.left#0" %29 {successors = [], txn = 0 : i64}
    %31 = arith.andi %11, %true_3 : i1
    %32 = arith.ori %17, %31 : i1
    %33 = arith.andi %19, %32 : i1
    %34 = acir.current {aspect = "value", resource = "top.right_out$count"} : i32
    %c0_i32_6 = arith.constant 0 : i32
    %35 = arith.subi %34, %c0_i32_6 : i32
    %c1_i32_7 = arith.constant 1 : i32
    %36 = arith.cmpi uge, %35, %c1_i32_7 : i32
    %37 = arith.xori %36, %true : i1
    %38 = arith.andi %11, %37 : i1
    %39 = arith.ori %17, %38 : i1
    %40 = arith.andi %33, %39 : i1
    %41 = acir.fire "top.right#1" %40 {successors = [], txn = 1 : i64}
    %42 = arith.andi %30, %16 : i1
    %c0_i32_8 = arith.constant 0 : i32
    %c0_i32_9 = arith.constant 0 : i32
    %43 = arith.cmpi eq, %c0_i32_8, %c0_i32_9 : i32
    %44 = acir.current {aspect = "value", resource = "top.left_out$storage$0"} : i32
    %45 = acir.aggregate(%44) : (i32) -> tuple<i32>
    %c0_i32_10 = arith.constant 0 : i32
    %c0_i32_11 = arith.constant 0 : i32
    %46 = acir.extract %45[%c0_i32_11] : (tuple<i32>, i32) -> i32
    %c0_i32_12 = arith.constant 0 : i32
    %47 = arith.select %43, %46, %c0_i32_12 : i32
    %48 = acir.aggregate(%47) : (i32) -> tuple<i32>
    %49 = arith.subi %23, %c0_i32_8 : i32
    %c0_i32_13 = arith.constant 0 : i32
    %50 = arith.select %42, %49, %c0_i32_13 : i32
    %c0_i32_14 = arith.constant 0 : i32
    %51 = acir.extract %48[%c0_i32_14] : (tuple<i32>, i32) -> i32
    %c0_i32_15 = arith.constant 0 : i32
    %52 = arith.cmpi eq, %50, %c0_i32_15 : i32
    %53 = arith.select %52, %3, %51 : i32
    %54 = acir.aggregate(%53) : (i32) -> tuple<i32>
    %c0_i32_16 = arith.constant 0 : i32
    %55 = acir.extract %54[%c0_i32_16] : (tuple<i32>, i32) -> i32
    %c0_i32_17 = arith.constant 0 : i32
    %56 = acir.extract %48[%c0_i32_17] : (tuple<i32>, i32) -> i32
    %57 = arith.select %42, %55, %56 : i32
    %58 = acir.aggregate(%57) : (i32) -> tuple<i32>
    %c0_i32_18 = arith.constant 0 : i32
    %59 = acir.extract %58[%c0_i32_18] : (tuple<i32>, i32) -> i32
    acir.next %59 {aspect = "value", resource = "top.left_out$storage$0"} : i32
    %c1_i32_19 = arith.constant 1 : i32
    %c0_i32_20 = arith.constant 0 : i32
    %60 = arith.select %42, %c1_i32_19, %c0_i32_20 : i32
    %61 = arith.addi %49, %60 : i32
    acir.next %61 {aspect = "value", resource = "top.left_out$count"} : i32
    %62 = arith.andi %41, %16 : i1
    %c0_i32_21 = arith.constant 0 : i32
    %c0_i32_22 = arith.constant 0 : i32
    %63 = arith.cmpi eq, %c0_i32_21, %c0_i32_22 : i32
    %64 = acir.current {aspect = "value", resource = "top.right_out$storage$0"} : i32
    %65 = acir.aggregate(%64) : (i32) -> tuple<i32>
    %c0_i32_23 = arith.constant 0 : i32
    %c0_i32_24 = arith.constant 0 : i32
    %66 = acir.extract %65[%c0_i32_24] : (tuple<i32>, i32) -> i32
    %c0_i32_25 = arith.constant 0 : i32
    %67 = arith.select %63, %66, %c0_i32_25 : i32
    %68 = acir.aggregate(%67) : (i32) -> tuple<i32>
    %c2_i32 = arith.constant 2 : i32
    %69 = arith.addi %2, %c2_i32 : i32
    %70 = arith.subi %34, %c0_i32_21 : i32
    %c0_i32_26 = arith.constant 0 : i32
    %71 = arith.select %62, %70, %c0_i32_26 : i32
    %c0_i32_27 = arith.constant 0 : i32
    %72 = acir.extract %68[%c0_i32_27] : (tuple<i32>, i32) -> i32
    %c0_i32_28 = arith.constant 0 : i32
    %73 = arith.cmpi eq, %71, %c0_i32_28 : i32
    %74 = arith.select %73, %69, %72 : i32
    %75 = acir.aggregate(%74) : (i32) -> tuple<i32>
    %c0_i32_29 = arith.constant 0 : i32
    %76 = acir.extract %75[%c0_i32_29] : (tuple<i32>, i32) -> i32
    %c0_i32_30 = arith.constant 0 : i32
    %77 = acir.extract %68[%c0_i32_30] : (tuple<i32>, i32) -> i32
    %78 = arith.select %62, %76, %77 : i32
    %79 = acir.aggregate(%78) : (i32) -> tuple<i32>
    %c0_i32_31 = arith.constant 0 : i32
    %80 = acir.extract %79[%c0_i32_31] : (tuple<i32>, i32) -> i32
    acir.next %80 {aspect = "value", resource = "top.right_out$storage$0"} : i32
    %c1_i32_32 = arith.constant 1 : i32
    %c0_i32_33 = arith.constant 0 : i32
    %81 = arith.select %62, %c1_i32_32, %c0_i32_33 : i32
    %82 = arith.addi %70, %81 : i32
    acir.next %82 {aspect = "value", resource = "top.right_out$count"} : i32
    %c0_i32_34 = arith.constant 0 : i32
    %c0_i32_35 = arith.constant 0 : i32
    %83 = arith.cmpi eq, %c0_i32_34, %c0_i32_35 : i32
    %c0_i32_36 = arith.constant 0 : i32
    %c0_i32_37 = arith.constant 0 : i32
    %84 = acir.extract %1[%c0_i32_37] : (tuple<i32>, i32) -> i32
    %c0_i32_38 = arith.constant 0 : i32
    %85 = arith.select %83, %84, %c0_i32_38 : i32
    %86 = acir.aggregate(%85) : (i32) -> tuple<i32>
    %c0_i32_39 = arith.constant 0 : i32
    %87 = acir.extract %86[%c0_i32_39] : (tuple<i32>, i32) -> i32
    acir.next %87 {aspect = "value", resource = "top.source$storage$0"} : i32
    %88 = arith.subi %6, %c0_i32_34 : i32
    acir.next %88 {aspect = "value", resource = "top.source$count"} : i32
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":15:16)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":36:12)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dual_channel.py":37:24)
