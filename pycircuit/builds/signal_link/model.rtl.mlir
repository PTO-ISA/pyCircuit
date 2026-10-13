module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "SignalLink", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.producer.result", source_branch = 3 : i64, target_branches = [0], to = 2 : i64}], schedule = [0, 1, 2], stage = "rtl"} {
    acir.state @top.producer.count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.producer.offset : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.reader.last : i32 {initial = 0 : i32, kind = "cell"}
    %true = arith.constant true
    %false = arith.constant false
    %true_0 = arith.constant true
    %0 = acir.fire "top.configure#0" %true_0 {successors = [], txn = 0 : i64}
    %true_1 = arith.constant true loc(#loc1)
    %1 = acir.export %true_1 {acir.id = "v2", acir.source_type = "bool"} : i1 -> i1 loc(#loc1)
    %2 = arith.xori %1, %true_0 : i1 loc(#loc1)
    %false_2 = arith.constant {acir.source_type = "bool"} false
    %3 = acir.current {aspect = "value", resource = "top.producer.count"} : i32 loc(#loc2)
    %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
    %4 = arith.cmpi ult, %3, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    %5 = arith.select %2, %false_2, %4 : i1
    %6 = arith.xori %5, %true_0 : i1
    %7 = arith.andi %1, %true : i1
    %8 = arith.ori %2, %7 : i1
    %9 = arith.ori %6, %8 : i1
    %10 = arith.andi %5, %9 : i1
    %11 = arith.xori %5, %true_0 : i1 loc(#loc3)
    %12 = arith.xori %5, %true_0 : i1 loc(#loc4)
    %13 = arith.ori %12, %9 : i1 loc(#loc4)
    %14 = arith.andi %13, %true : i1 loc(#loc3)
    %15 = arith.ori %11, %14 : i1 loc(#loc3)
    %16 = arith.andi %10, %15 : i1 loc(#loc3)
    %17 = arith.xori %5, %true_0 : i1 loc(#loc5)
    %18 = acir.current {aspect = "value", resource = "top.producer.offset"} : i32 loc(#loc5)
    %19 = arith.andi %13, %true : i1 loc(#loc5)
    %20 = arith.ori %17, %19 : i1 loc(#loc5)
    %21 = arith.andi %16, %20 : i1 loc(#loc5)
    %22 = arith.xori %5, %true_0 : i1 loc(#loc6)
    %23 = arith.andi %true, %true : i1 loc(#loc3)
    %24 = arith.andi %13, %23 : i1 loc(#loc3)
    %25 = arith.ori %22, %24 : i1 loc(#loc3)
    %26 = arith.andi %21, %25 : i1 loc(#loc3)
    %27 = arith.xori %5, %true_0 : i1 loc(#loc7)
    %28 = arith.andi %13, %true : i1 loc(#loc7)
    %29 = arith.ori %27, %28 : i1 loc(#loc7)
    %30 = arith.andi %26, %29 : i1 loc(#loc7)
    %31 = arith.xori %5, %true_0 : i1 loc(#loc8)
    %32 = arith.andi %13, %true : i1 loc(#loc8)
    %33 = arith.ori %31, %32 : i1 loc(#loc8)
    %34 = arith.andi %30, %33 : i1 loc(#loc8)
    %35 = acir.fire "top.producer.advance#1" %34 {successors = [2], txn = 1 : i64} loc(#loc8)
    %36 = acir.aggregate() : () -> i32 loc(#loc9)
    %37 = arith.andi %35, %5 : i1
    %38 = arith.addi %3, %18 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc3)
    %39 = arith.select %37, %38, %36 : i32 loc(#loc3)
    %40 = arith.ori %false, %37 : i1 loc(#loc3)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc10)
    %41 = arith.addi %39, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc10)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc11)
    %42 = arith.cmpi ne, %41, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc11)
    %43 = arith.xori %42, %true_0 : i1
    %44 = arith.ori %43, %40 : i1
    %45 = arith.andi %44, %40 : i1
    %46 = arith.andi %42, %45 : i1
    %47 = arith.xori %42, %true_0 : i1 loc(#loc12)
    %48 = arith.xori %42, %true_0 : i1 loc(#loc13)
    %49 = arith.ori %48, %44 : i1 loc(#loc13)
    %50 = arith.andi %44, %40 : i1 loc(#loc13)
    %51 = arith.andi %42, %50 : i1 loc(#loc13)
    %52 = arith.andi %49, %51 : i1 loc(#loc13)
    %53 = arith.ori %47, %52 : i1 loc(#loc13)
    %54 = arith.andi %46, %53 : i1 loc(#loc13)
    %55 = acir.fire "top.reader.observe#2" %54 {successors = [], txn = 2 : i64} loc(#loc13)
    %56 = arith.andi %35, %5 : i1
    %c1_i32_3 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc14)
    %57 = arith.addi %3, %c1_i32_3 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc14)
    %58 = arith.select %56, %57, %3 : i32 loc(#loc14)
    acir.next %58 {aspect = "value", resource = "top.producer.count"} : i32 loc(#loc14)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc15)
    %59 = arith.select %0, %c4_i32, %18 : i32 loc(#loc15)
    acir.next %59 {aspect = "value", resource = "top.producer.offset"} : i32 loc(#loc15)
    %60 = acir.current {aspect = "value", resource = "top.reader.last"} : i32 loc(#loc15)
    %61 = arith.andi %55, %42 : i1
    %62 = arith.select %61, %41, %60 : i32
    acir.next %62 {aspect = "value", resource = "top.reader.last"} : i32
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":51:18)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:18)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:13)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:26)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:17)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:17)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:28)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":43:29)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":38:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":45:13)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:25)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":57:28)
