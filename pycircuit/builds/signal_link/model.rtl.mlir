module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, format_version = 9 : i64, top = "SignalLink", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 0 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.result", source_branch = 2 : i64, target_branches = [4], to = 2 : i64}], logical_states = [{kind = "cell", name = "top.producer.count", parts = [{path = [], resource = "top.producer.count$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.producer.offset", parts = [{path = [], resource = "top.producer.offset$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.reader.last", parts = [{path = [], resource = "top.reader.last$value"}], prototype = 0 : i32}], schedule = [0, 1, 2], stage = "rtl"} {
    acir.state @top.producer.count$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.producer.offset$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.reader.last$value : i32 {initial = 0 : i32, kind = "cell"}
    %true = arith.constant true
    %0 = acir.export %true {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
    %1 = acir.current {aspect = "value", resource = "top.producer.count$value"} : i32 loc(#loc1)
    %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
    %2 = arith.cmpi ult, %1, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    %false = arith.constant false
    %3 = arith.select %0, %2, %false : i1
    %4 = arith.xori %3, %true : i1 loc(#loc2)
    %5 = arith.xori %0, %true : i1
    %true_0 = arith.constant true
    %6 = arith.andi %0, %true_0 : i1
    %7 = arith.ori %5, %6 : i1
    %8 = arith.ori %4, %7 : i1
    %9 = arith.andi %3, %8 : i1
    %10 = arith.ori %4, %8 : i1 loc(#loc2)
    %11 = arith.andi %10, %true_0 : i1 loc(#loc2)
    %12 = arith.ori %4, %11 : i1 loc(#loc2)
    %13 = arith.andi %9, %12 : i1 loc(#loc2)
    %14 = arith.andi %10, %true_0 : i1 loc(#loc3)
    %15 = arith.ori %4, %14 : i1 loc(#loc3)
    %16 = arith.andi %13, %15 : i1 loc(#loc3)
    %17 = arith.andi %16, %12 : i1 loc(#loc3)
    %18 = arith.andi %17, %12 : i1 loc(#loc3)
    %19 = arith.andi %true_0, %true_0 : i1 loc(#loc4)
    %20 = arith.andi %10, %19 : i1 loc(#loc4)
    %21 = arith.andi %3, %20 : i1 loc(#loc4)
    %22 = arith.xori %21, %true : i1 loc(#loc4)
    %23 = arith.ori %22, %8 : i1 loc(#loc4)
    %24 = arith.andi %18, %23 : i1 loc(#loc4)
    %25 = acir.fire "top.producer.advance#0" %24 {successors = [2], txn = 0 : i64} loc(#loc4)
    %26 = acir.fire "top.configure#1" %true {successors = [], txn = 1 : i64}
    %27 = arith.andi %25, %21 : i1
    %28 = acir.current {aspect = "value", resource = "top.producer.offset$value"} : i32 loc(#loc3)
    %29 = arith.addi %1, %28 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc3)
    %c0_i32 = arith.constant 0 : i32 loc(#loc5)
    %30 = arith.select %27, %29, %c0_i32 : i32 loc(#loc5)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %31 = arith.addi %30, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32
    %32 = arith.cmpi ne, %31, %c0_i32_1 {acir.id = "v7", acir.source_type = "bool"} : i32
    %33 = arith.cmpi eq, %31, %c0_i32_1 : i32 loc(#loc6)
    %false_2 = arith.constant false
    %34 = arith.ori %false_2, %27 : i1
    %35 = arith.ori %33, %34 : i1
    %36 = arith.andi %35, %34 : i1
    %37 = arith.andi %32, %36 : i1
    %38 = arith.ori %33, %35 : i1 loc(#loc7)
    %39 = arith.andi %38, %37 : i1 loc(#loc7)
    %40 = arith.ori %33, %39 : i1 loc(#loc7)
    %41 = arith.andi %37, %40 : i1 loc(#loc7)
    %42 = acir.fire "top.reader.observe#2" %41 {successors = [], txn = 2 : i64} loc(#loc7)
    %43 = arith.andi %25, %3 : i1
    %44 = arith.addi %1, %c1_i32 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc8)
    %45 = arith.select %43, %44, %1 : i32 loc(#loc8)
    acir.next %45 {aspect = "value", resource = "top.producer.count$value"} : i32 loc(#loc8)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %46 = arith.select %26, %c4_i32, %28 : i32
    acir.next %46 {aspect = "value", resource = "top.producer.offset$value"} : i32
    %47 = arith.andi %42, %32 : i1
    %48 = acir.current {aspect = "value", resource = "top.reader.last$value"} : i32
    %49 = arith.select %47, %31, %48 : i32
    acir.next %49 {aspect = "value", resource = "top.reader.last$value"} : i32
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:20)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:25)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:17)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":40:17)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:17)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":44:13)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:17)
