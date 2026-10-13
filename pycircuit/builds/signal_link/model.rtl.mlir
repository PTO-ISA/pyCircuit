module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "SignalLink", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.result", source_branch = 3 : i64, target_branches = [4], to = 2 : i64}], schedule = [0, 1, 2], stage = "rtl"} {
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
    %23 = arith.andi %13, %true : i1 loc(#loc6)
    %24 = arith.ori %22, %23 : i1 loc(#loc6)
    %25 = arith.andi %21, %24 : i1 loc(#loc6)
    %26 = arith.xori %5, %true_0 : i1 loc(#loc7)
    %27 = arith.andi %13, %true : i1 loc(#loc7)
    %28 = arith.ori %26, %27 : i1 loc(#loc7)
    %29 = arith.andi %25, %28 : i1 loc(#loc7)
    %30 = arith.andi %true, %true : i1 loc(#loc3)
    %31 = arith.andi %13, %30 : i1 loc(#loc3)
    %32 = arith.andi %5, %31 : i1 loc(#loc3)
    %33 = arith.andi %5, %32 : i1 loc(#loc3)
    %34 = arith.xori %33, %true_0 : i1 loc(#loc3)
    %35 = arith.ori %34, %9 : i1 loc(#loc3)
    %36 = arith.andi %29, %35 : i1 loc(#loc3)
    %37 = acir.fire "top.producer.advance#1" %36 {successors = [2], txn = 1 : i64} loc(#loc3)
    %38 = acir.aggregate() : () -> i32 loc(#loc8)
    %39 = arith.andi %37, %33 : i1 loc(#loc9)
    %40 = arith.addi %3, %18 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc3)
    %41 = arith.select %39, %40, %38 : i32 loc(#loc3)
    %42 = arith.ori %false, %39 : i1 loc(#loc3)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc10)
    %43 = arith.addi %41, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc10)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc11)
    %44 = arith.cmpi ne, %43, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc11)
    %45 = arith.xori %44, %true_0 : i1
    %46 = arith.ori %45, %42 : i1
    %47 = arith.andi %46, %42 : i1
    %48 = arith.andi %44, %47 : i1
    %49 = arith.xori %44, %true_0 : i1 loc(#loc12)
    %50 = arith.xori %44, %true_0 : i1 loc(#loc13)
    %51 = arith.ori %50, %46 : i1 loc(#loc13)
    %52 = arith.andi %46, %42 : i1 loc(#loc13)
    %53 = arith.andi %44, %52 : i1 loc(#loc13)
    %54 = arith.andi %51, %53 : i1 loc(#loc13)
    %55 = arith.ori %49, %54 : i1 loc(#loc13)
    %56 = arith.andi %48, %55 : i1 loc(#loc13)
    %57 = acir.fire "top.reader.observe#2" %56 {successors = [], txn = 2 : i64} loc(#loc13)
    %58 = arith.andi %37, %5 : i1 loc(#loc9)
    %c1_i32_3 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc14)
    %59 = arith.addi %3, %c1_i32_3 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc14)
    %60 = arith.select %58, %59, %3 : i32 loc(#loc14)
    acir.next %60 {aspect = "value", resource = "top.producer.count"} : i32 loc(#loc14)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc15)
    %61 = arith.select %0, %c4_i32, %18 : i32 loc(#loc15)
    acir.next %61 {aspect = "value", resource = "top.producer.offset"} : i32 loc(#loc15)
    %62 = acir.current {aspect = "value", resource = "top.reader.last"} : i32 loc(#loc15)
    %63 = arith.andi %57, %44 : i1
    %64 = arith.select %63, %43, %62 : i32
    acir.next %64 {aspect = "value", resource = "top.reader.last"} : i32
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":49:18)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:17)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:20)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:25)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:17)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":40:17)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:28)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:29)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":37:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":44:13)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:25)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":56:28)
