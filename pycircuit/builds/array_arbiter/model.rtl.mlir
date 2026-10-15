module attributes {acir.instances = [{definition = "src_array_arbiter_1242b928.ArrayArbiter", name = "top", parent, rules = ["allocate", "clock", "drain", "issue"]}, {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.0", parent = "top", rules = ["allocate", "peek", "take", "wake"]}, {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.1", parent = "top", rules = ["allocate", "peek", "take", "wake"]}, {definition = "src_array_arbiter_1242b928.Slot", name = "top.slots.2", parent = "top", rules = ["allocate", "peek", "take", "wake"]}, {definition = "src_array_arbiter_1242b928.AgeMatrix", name = "top.age", parent = "top", rules = ["allocate", "choose", "release"]}], acir.model = {config = [], constants = {src_array_arbiter_1242b928.N = 3 : i64}, format_version = 9 : i64, top = "ArrayArbiter", type_order = ["src_array_arbiter_1242b928.Entry"], types = {src_array_arbiter_1242b928.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}, acir.stage = "rtl"} {
  %c30_i32 = arith.constant 30 : i32
  %c10_i32 = arith.constant 10 : i32
  %c20_i32 = arith.constant 20 : i32
  %c5_i32 = arith.constant 5 : i32
  %c3_i64 = arith.constant 3 : i64
  %false = arith.constant false
  %c2_i64 = arith.constant 2 : i64
  %c1_i64 = arith.constant 1 : i64
  %c0_i64 = arith.constant 0 : i64
  %c3_i32 = arith.constant 3 : i32
  %c4_i32 = arith.constant 4 : i32
  %true = arith.constant true
  %c2_i32 = arith.constant 2 : i32
  %c1_i32 = arith.constant 1 : i32
  %c0_i32 = arith.constant 0 : i32
  acir.circuit attributes {conflicts = [], edges = [{from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.output", source_branch = 26 : i64, target_branches = [16], to = 3 : i64}, {from = 5 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.output", source_branch = 26 : i64, target_branches = [16], to = 3 : i64}], logical_states = [{kind = "cell", name = "top.age.older", parts = [{path = [0, 0], resource = "top.age.older$value$0$0"}, {path = [0, 1], resource = "top.age.older$value$0$1"}, {path = [0, 2], resource = "top.age.older$value$0$2"}, {path = [1, 0], resource = "top.age.older$value$1$0"}, {path = [1, 1], resource = "top.age.older$value$1$1"}, {path = [1, 2], resource = "top.age.older$value$1$2"}, {path = [2, 0], resource = "top.age.older$value$2$0"}, {path = [2, 1], resource = "top.age.older$value$2$1"}, {path = [2, 2], resource = "top.age.older$value$2$2"}], prototype = [[false, false, false], [false, false, false], [false, false, false]]}, {kind = "cell", name = "top.age.used", parts = [{path = [0], resource = "top.age.used$value$0"}, {path = [1], resource = "top.age.used$value$1"}, {path = [2], resource = "top.age.used$value$2"}], prototype = [false, false, false]}, {kind = "cell", name = "top.cycle", parts = [{path = [], resource = "top.cycle$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.issued", parts = [{path = [], resource = "top.issued$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.last", parts = [{path = [], resource = "top.last$value"}], prototype = 0 : i32}, {count = "top.output$count", kind = "queue", name = "top.output", parts = [{path = [0], resource = "top.output$storage$0"}], prototype = [99 : i32]}, {kind = "cell", name = "top.retired", parts = [{path = [], resource = "top.retired$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.slots.0.state", parts = [{path = [0], resource = "top.slots.0.state$value$0"}, {path = [1], resource = "top.slots.0.state$value$1"}, {path = [2], resource = "top.slots.0.state$value$2"}], prototype = [false, false, 0 : i32]}, {kind = "cell", name = "top.slots.1.state", parts = [{path = [0], resource = "top.slots.1.state$value$0"}, {path = [1], resource = "top.slots.1.state$value$1"}, {path = [2], resource = "top.slots.1.state$value$2"}], prototype = [false, false, 0 : i32]}, {kind = "cell", name = "top.slots.2.state", parts = [{path = [0], resource = "top.slots.2.state$value$0"}, {path = [1], resource = "top.slots.2.state$value$1"}, {path = [2], resource = "top.slots.2.state$value$2"}], prototype = [false, false, 0 : i32]}], schedule = [0, 1, 2, 4, 5, 3, 6], stage = "rtl"} {
    acir.state @top.age.older$value$0$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.older$value$0$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.older$value$0$2 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.older$value$1$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.older$value$1$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.older$value$1$2 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.older$value$2$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.older$value$2$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.older$value$2$2 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.used$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.used$value$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.age.used$value$2 : i1 {initial = false, kind = "cell"}
    acir.state @top.cycle$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.issued$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.last$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.output$count : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.output$storage$0 : i32 {initial = 99 : i32, kind = "cell"}
    acir.state @top.retired$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.slots.0.state$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.slots.0.state$value$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.slots.0.state$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.slots.1.state$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.slots.1.state$value$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.slots.1.state$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.slots.2.state$value$0 : i1 {initial = false, kind = "cell"}
    acir.state @top.slots.2.state$value$1 : i1 {initial = false, kind = "cell"}
    acir.state @top.slots.2.state$value$2 : i32 {initial = 0 : i32, kind = "cell"}
    %0 = acir.current {aspect = "value", resource = "top.cycle$value"} : i32 loc(#loc1)
    %1 = arith.cmpi ne, %0, %c0_i32 : i32
    %2 = arith.cmpi ne, %0, %c1_i32 : i32
    %3 = arith.cmpi eq, %0, %c2_i32 : i32
    %4 = acir.current {aspect = "value", resource = "top.slots.1.state$value$0"} : i1 loc(#loc2)
    %5 = acir.current {aspect = "value", resource = "top.slots.1.state$value$1"} : i1 loc(#loc2)
    %6 = acir.current {aspect = "value", resource = "top.slots.1.state$value$2"} : i32 loc(#loc2)
    %7 = acir.aggregate(%4, %5, %6) : (i1, i1, i32) -> tuple<i1, i1, i32> loc(#loc2)
    %8 = arith.xori %4, %true : i1
    %9 = arith.andi %3, %8 : i1
    %10 = arith.andi %2, %9 : i1
    %11 = arith.andi %1, %10 : i1
    %12 = acir.fire "top.allocate#0" %11 {successors = [], txn = 0 : i64}
    %13 = arith.cmpi eq, %0, %c1_i32 : i32 loc(#loc3)
    %14 = acir.current {aspect = "value", resource = "top.slots.0.state$value$0"} : i1 loc(#loc2)
    %15 = acir.current {aspect = "value", resource = "top.slots.0.state$value$1"} : i1 loc(#loc2)
    %16 = acir.current {aspect = "value", resource = "top.slots.0.state$value$2"} : i32 loc(#loc2)
    %17 = acir.aggregate(%14, %15, %16) : (i1, i1, i32) -> tuple<i1, i1, i32> loc(#loc2)
    %18 = arith.xori %14, %true : i1 loc(#loc2)
    %19 = arith.andi %13, %18 : i1 loc(#loc2)
    %20 = arith.andi %1, %19 : i1 loc(#loc2)
    %21 = acir.fire "top.allocate#1" %20 {successors = [], txn = 1 : i64} loc(#loc2)
    %22 = arith.cmpi eq, %0, %c0_i32 : i32 loc(#loc1)
    %23 = acir.current {aspect = "value", resource = "top.slots.2.state$value$0"} : i1 loc(#loc2)
    %24 = acir.current {aspect = "value", resource = "top.slots.2.state$value$1"} : i1 loc(#loc2)
    %25 = acir.current {aspect = "value", resource = "top.slots.2.state$value$2"} : i32 loc(#loc2)
    %26 = acir.aggregate(%23, %24, %25) : (i1, i1, i32) -> tuple<i1, i1, i32>
    %27 = arith.xori %23, %true : i1 loc(#loc2)
    %28 = arith.andi %22, %27 : i1 loc(#loc2)
    %29 = acir.fire "top.allocate#2" %28 {successors = [], txn = 2 : i64} loc(#loc2)
    %30 = arith.cmpi uge, %0, %c4_i32 : i32
    %31 = arith.remui %0, %c2_i32 : i32 loc(#loc4)
    %32 = arith.cmpi eq, %31, %c0_i32 : i32 loc(#loc4)
    %33 = acir.current {aspect = "value", resource = "top.output$count"} : i32 loc(#loc5)
    %34 = arith.cmpi ne, %33, %c0_i32 : i32 loc(#loc6)
    %35 = arith.andi %32, %34 : i1 loc(#loc6)
    %36 = arith.andi %30, %35 : i1 loc(#loc6)
    %37 = acir.fire "top.drain#5" %36 {successors = [3], txn = 5 : i64} loc(#loc6)
    %38 = arith.cmpi uge, %0, %c3_i32 : i32
    %39 = acir.export %true : i1 -> i1
    %40 = acir.export %true : i1 -> i1
    %41 = arith.andi %39, %40 : i1
    %42 = acir.export %true : i1 -> i1
    %43 = arith.andi %41, %42 : i1
    %44 = acir.export %17 : tuple<i1, i1, i32> -> tuple<i1, i1, i32> loc(#loc7)
    %45 = acir.export %7 : tuple<i1, i1, i32> -> tuple<i1, i1, i32> loc(#loc7)
    %46 = acir.export %26 : tuple<i1, i1, i32> -> tuple<i1, i1, i32> loc(#loc7)
    %47 = acir.extract %44[%c0_i32] : (tuple<i1, i1, i32>, i32) -> i1 loc(#loc8)
    %48 = arith.xori %47, %true : i1 loc(#loc8)
    %49 = arith.ori %48, %43 : i1 loc(#loc8)
    %50 = arith.andi %43, %49 : i1 loc(#loc8)
    %51 = acir.extract %45[%c0_i32] : (tuple<i1, i1, i32>, i32) -> i1 loc(#loc8)
    %52 = arith.xori %51, %true : i1 loc(#loc8)
    %53 = arith.ori %52, %43 : i1 loc(#loc8)
    %54 = arith.andi %43, %53 : i1 loc(#loc8)
    %55 = arith.andi %50, %54 : i1 loc(#loc8)
    %56 = acir.extract %46[%c0_i32] : (tuple<i1, i1, i32>, i32) -> i1 loc(#loc8)
    %57 = arith.xori %56, %true : i1 loc(#loc8)
    %58 = arith.ori %57, %43 : i1 loc(#loc8)
    %59 = arith.andi %43, %58 : i1 loc(#loc8)
    %60 = arith.andi %55, %59 : i1 loc(#loc8)
    %61 = arith.andi %38, %60 : i1 loc(#loc8)
    %62 = arith.cmpi ult, %0, %c3_i32 : i32 loc(#loc9)
    %63 = arith.ori %62, %60 : i1 loc(#loc9)
    %64 = arith.andi %61, %63 : i1 loc(#loc9)
    %65 = acir.extract %44[%c1_i32] : (tuple<i1, i1, i32>, i32) -> i1 loc(#loc10)
    %66 = arith.andi %47, %65 : i1 loc(#loc10)
    %67 = acir.extract %45[%c1_i32] : (tuple<i1, i1, i32>, i32) -> i1 loc(#loc10)
    %68 = arith.andi %51, %67 : i1 loc(#loc10)
    %69 = acir.extract %46[%c1_i32] : (tuple<i1, i1, i32>, i32) -> i1 loc(#loc10)
    %70 = arith.andi %56, %69 : i1 loc(#loc10)
    %71 = acir.current {aspect = "value", resource = "top.age.older$value$0$0"} : i1
    %72 = acir.current {aspect = "value", resource = "top.age.older$value$0$1"} : i1
    %73 = acir.current {aspect = "value", resource = "top.age.older$value$0$2"} : i1
    %74 = acir.current {aspect = "value", resource = "top.age.older$value$1$0"} : i1
    %75 = acir.current {aspect = "value", resource = "top.age.older$value$1$1"} : i1
    %76 = acir.current {aspect = "value", resource = "top.age.older$value$1$2"} : i1
    %77 = acir.current {aspect = "value", resource = "top.age.older$value$2$0"} : i1
    %78 = acir.current {aspect = "value", resource = "top.age.older$value$2$1"} : i1
    %79 = acir.current {aspect = "value", resource = "top.age.older$value$2$2"} : i1
    %80 = arith.andi %70, %79 : i1
    %81 = arith.andi %68, %76 : i1 loc(#loc11)
    %82 = arith.andi %66, %73 : i1 loc(#loc11)
    %83 = arith.xori %82, %true : i1 loc(#loc11)
    %84 = arith.select %81, %false, %83 : i1 loc(#loc11)
    %85 = arith.select %80, %false, %84 : i1 loc(#loc11)
    %86 = arith.select %70, %85, %false : i1 loc(#loc11)
    %87 = arith.andi %70, %78 : i1 loc(#loc11)
    %88 = arith.andi %68, %75 : i1 loc(#loc11)
    %89 = arith.andi %66, %72 : i1 loc(#loc11)
    %90 = arith.xori %89, %true : i1 loc(#loc11)
    %91 = arith.select %88, %false, %90 : i1 loc(#loc11)
    %92 = arith.select %87, %false, %91 : i1 loc(#loc11)
    %93 = arith.select %68, %92, %false : i1 loc(#loc11)
    %94 = arith.andi %70, %77 : i1 loc(#loc11)
    %95 = arith.andi %68, %74 : i1 loc(#loc11)
    %96 = arith.andi %66, %71 : i1 loc(#loc11)
    %97 = arith.xori %96, %true : i1 loc(#loc11)
    %98 = arith.select %95, %false, %97 : i1 loc(#loc11)
    %99 = arith.select %94, %false, %98 : i1 loc(#loc11)
    %100 = arith.select %66, %99, %false : i1 loc(#loc11)
    %101 = arith.select %93, %true, %100 : i1 loc(#loc11)
    %102 = arith.select %86, %true, %101 : i1 loc(#loc11)
    %103 = arith.andi %38, %102 : i1 loc(#loc11)
    %104 = arith.xori %103, %true : i1 loc(#loc11)
    %105 = arith.ori %104, %60 : i1
    %106 = arith.extui %93 : i1 to i32 loc(#loc12)
    %107 = arith.select %86, %c2_i32, %106 : i32 loc(#loc12)
    %108 = arith.cmpi eq, %107, %c0_i32 : i32 loc(#loc12)
    %109 = arith.cmpi eq, %107, %c1_i32 : i32 loc(#loc13)
    %110 = arith.select %108, %true, %109 : i1 loc(#loc13)
    %111 = arith.cmpi eq, %107, %c2_i32 : i32 loc(#loc14)
    %112 = arith.select %110, %true, %111 : i1 loc(#loc14)
    %113 = arith.andi %60, %112 : i1 loc(#loc14)
    %114 = arith.andi %105, %113 : i1 loc(#loc14)
    %115 = arith.ori %104, %114 : i1 loc(#loc14)
    %116 = arith.andi %64, %115 : i1 loc(#loc14)
    %117 = arith.andi %115, %60 : i1
    %118 = arith.ori %104, %117 : i1
    %119 = arith.andi %116, %118 : i1
    %120 = arith.cmpi ne, %107, %c0_i32 : i32 loc(#loc15)
    %121 = arith.andi %103, %120 : i1 loc(#loc15)
    %122 = arith.xori %121, %true : i1 loc(#loc15)
    %123 = arith.ori %122, %117 : i1
    %124 = arith.andi %123, %60 : i1
    %125 = arith.ori %122, %124 : i1
    %126 = arith.andi %119, %125 : i1
    %127 = arith.cmpi ne, %107, %c1_i32 : i32 loc(#loc13)
    %128 = arith.andi %121, %127 : i1 loc(#loc13)
    %129 = arith.xori %128, %true : i1 loc(#loc13)
    %130 = arith.ori %129, %124 : i1
    %131 = arith.andi %130, %60 : i1
    %132 = arith.ori %129, %131 : i1
    %133 = arith.andi %126, %132 : i1
    %134 = arith.andi %128, %111 : i1
    %135 = arith.xori %134, %true : i1
    %136 = arith.ori %135, %131 : i1
    %137 = arith.ori %135, %136 : i1
    %138 = arith.ori %135, %137 : i1
    %139 = arith.andi %133, %138 : i1
    %140 = arith.andi %134, %23 : i1 loc(#loc16)
    %141 = arith.xori %140, %true : i1 loc(#loc16)
    %142 = arith.ori %141, %137 : i1
    %143 = arith.ori %141, %142 : i1
    %144 = arith.andi %139, %143 : i1
    %145 = arith.andi %23, %24 : i1 loc(#loc17)
    %146 = arith.andi %136, %145 : i1 loc(#loc17)
    %147 = arith.ori %135, %146 : i1 loc(#loc17)
    %148 = arith.andi %144, %147 : i1 loc(#loc17)
    %149 = arith.ori %135, %147 : i1 loc(#loc18)
    %150 = arith.andi %148, %149 : i1 loc(#loc18)
    %151 = arith.andi %121, %109 : i1
    %152 = arith.xori %151, %true : i1
    %153 = arith.ori %152, %124 : i1
    %154 = arith.ori %152, %153 : i1
    %155 = arith.ori %152, %154 : i1
    %156 = arith.andi %150, %155 : i1
    %157 = arith.andi %151, %4 : i1 loc(#loc16)
    %158 = arith.xori %157, %true : i1 loc(#loc16)
    %159 = arith.ori %158, %154 : i1
    %160 = arith.ori %158, %159 : i1
    %161 = arith.andi %156, %160 : i1
    %162 = arith.andi %4, %5 : i1 loc(#loc17)
    %163 = arith.andi %153, %162 : i1 loc(#loc17)
    %164 = arith.ori %152, %163 : i1 loc(#loc17)
    %165 = arith.andi %161, %164 : i1 loc(#loc17)
    %166 = arith.ori %152, %164 : i1 loc(#loc18)
    %167 = arith.andi %165, %166 : i1 loc(#loc18)
    %168 = arith.andi %103, %108 : i1
    %169 = arith.xori %168, %true : i1
    %170 = arith.ori %169, %117 : i1
    %171 = arith.ori %169, %170 : i1
    %172 = arith.ori %169, %171 : i1
    %173 = arith.andi %167, %172 : i1
    %174 = arith.andi %168, %14 : i1 loc(#loc16)
    %175 = arith.xori %174, %true : i1 loc(#loc16)
    %176 = arith.ori %175, %171 : i1
    %177 = arith.ori %175, %176 : i1
    %178 = arith.andi %173, %177 : i1
    %179 = arith.andi %14, %15 : i1 loc(#loc17)
    %180 = arith.andi %170, %179 : i1 loc(#loc17)
    %181 = arith.ori %169, %180 : i1 loc(#loc17)
    %182 = arith.andi %178, %181 : i1 loc(#loc17)
    %183 = arith.ori %169, %181 : i1 loc(#loc18)
    %184 = arith.andi %182, %183 : i1 loc(#loc18)
    %185 = arith.ori %109, %60 : i1
    %186 = arith.andi %60, %185 : i1
    %187 = arith.ori %108, %186 : i1
    %188 = arith.andi %60, %187 : i1
    %189 = arith.andi %115, %188 : i1
    %190 = arith.ori %104, %189 : i1
    %191 = arith.andi %184, %190 : i1
    %192 = arith.ori %104, %105 : i1 loc(#loc19)
    %193 = arith.ori %104, %192 : i1 loc(#loc19)
    %194 = arith.andi %191, %193 : i1 loc(#loc19)
    %195 = arith.extui %107 : i32 to i64
    %196 = arith.cmpi ult, %195, %c3_i64 : i64
    %197 = arith.andi %60, %196 : i1
    %198 = arith.andi %192, %197 : i1
    %199 = arith.ori %104, %198 : i1
    %200 = arith.andi %194, %199 : i1
    %201 = arith.cmpi ne, %107, %c2_i32 : i32 loc(#loc14)
    %202 = arith.ori %201, %145 : i1 loc(#loc14)
    %203 = arith.andi %60, %202 : i1 loc(#loc14)
    %204 = arith.select %109, %162, %203 : i1 loc(#loc14)
    %205 = arith.andi %60, %204 : i1 loc(#loc14)
    %206 = arith.andi %205, %186 : i1 loc(#loc14)
    %207 = arith.select %108, %179, %206 : i1 loc(#loc14)
    %208 = arith.andi %60, %207 : i1 loc(#loc14)
    %209 = arith.andi %208, %188 : i1 loc(#loc14)
    %210 = arith.andi %209, %113 : i1 loc(#loc14)
    %211 = arith.andi %105, %210 : i1 loc(#loc14)
    %212 = arith.ori %104, %211 : i1 loc(#loc14)
    %213 = arith.andi %200, %212 : i1 loc(#loc14)
    %214 = arith.andi %213, %192 : i1
    %215 = arith.andi %30, %32 : i1
    %216 = arith.andi %37, %215 : i1
    %217 = arith.extui %216 : i1 to i32 loc(#loc20)
    %218 = arith.subi %33, %217 : i32 loc(#loc20)
    %219 = arith.cmpi ult, %218, %c1_i32 : i32
    %220 = arith.andi %105, %219 : i1
    %221 = arith.ori %104, %220 : i1
    %222 = arith.andi %214, %221 : i1
    %223 = arith.andi %222, %192 : i1
    %224 = acir.fire "top.issue#3" %223 {successors = [], txn = 3 : i64}
    %225 = arith.cmpi eq, %0, %c5_i32 : i32
    %226 = acir.fire "top.slots.2.wake#4" %225 {successors = [], txn = 4 : i64}
    %227 = acir.fire "top.clock#6" %true {successors = [], txn = 6 : i64}
    %228 = arith.andi %29, %22 : i1
    %229 = acir.current {aspect = "value", resource = "top.age.used$value$0"} : i1
    %230 = acir.current {aspect = "value", resource = "top.age.used$value$1"} : i1
    %231 = acir.current {aspect = "value", resource = "top.age.used$value$2"} : i1
    %232 = arith.andi %1, %13 : i1
    %233 = arith.andi %21, %232 : i1
    %234 = arith.andi %2, %3 : i1
    %235 = arith.andi %1, %234 : i1
    %236 = arith.andi %12, %235 : i1
    %237 = arith.select %236, %229, %72 : i1 loc(#loc21)
    %238 = arith.select %236, %false, %74 : i1 loc(#loc21)
    %239 = arith.select %236, %false, %75 : i1 loc(#loc21)
    %240 = arith.select %236, %false, %76 : i1 loc(#loc21)
    %241 = arith.select %236, %231, %78 : i1 loc(#loc21)
    %242 = arith.select %233, %false, %71 : i1 loc(#loc21)
    %243 = arith.select %233, %false, %237 : i1 loc(#loc21)
    %244 = arith.select %233, %false, %73 : i1 loc(#loc21)
    %245 = arith.select %233, %230, %238 : i1 loc(#loc21)
    %246 = arith.select %233, %75, %239 : i1 loc(#loc21)
    %247 = arith.select %233, %76, %240 : i1 loc(#loc21)
    %248 = arith.select %233, %231, %77 : i1 loc(#loc21)
    %249 = arith.select %233, %78, %241 : i1 loc(#loc21)
    %250 = arith.select %228, %71, %242 : i1 loc(#loc21)
    %251 = arith.select %228, %72, %243 : i1 loc(#loc21)
    %252 = arith.select %228, %229, %244 : i1 loc(#loc21)
    %253 = arith.select %228, %74, %245 : i1 loc(#loc21)
    %254 = arith.select %228, %75, %246 : i1 loc(#loc21)
    %255 = arith.select %228, %230, %247 : i1 loc(#loc21)
    %256 = arith.select %228, %false, %248 : i1 loc(#loc21)
    %257 = arith.select %228, %false, %249 : i1 loc(#loc21)
    %258 = arith.select %228, %false, %79 : i1 loc(#loc21)
    acir.next %250 {aspect = "value", resource = "top.age.older$value$0$0"} : i1 loc(#loc21)
    acir.next %251 {aspect = "value", resource = "top.age.older$value$0$1"} : i1 loc(#loc21)
    acir.next %252 {aspect = "value", resource = "top.age.older$value$0$2"} : i1 loc(#loc21)
    acir.next %253 {aspect = "value", resource = "top.age.older$value$1$0"} : i1 loc(#loc21)
    acir.next %254 {aspect = "value", resource = "top.age.older$value$1$1"} : i1 loc(#loc21)
    acir.next %255 {aspect = "value", resource = "top.age.older$value$1$2"} : i1 loc(#loc21)
    acir.next %256 {aspect = "value", resource = "top.age.older$value$2$0"} : i1 loc(#loc21)
    acir.next %257 {aspect = "value", resource = "top.age.older$value$2$1"} : i1 loc(#loc21)
    acir.next %258 {aspect = "value", resource = "top.age.older$value$2$2"} : i1 loc(#loc21)
    %259 = arith.xori %77, %true : i1
    %260 = arith.xori %74, %true : i1
    %261 = arith.xori %71, %true : i1
    %262 = arith.andi %260, %261 : i1
    %263 = arith.andi %259, %262 : i1
    %264 = arith.xori %72, %true : i1
    %265 = arith.ori %264, %263 : i1
    %266 = arith.select %75, %263, %265 : i1
    %267 = arith.select %78, %263, %266 : i1
    %268 = arith.xori %78, %true : i1
    %269 = arith.xori %75, %true : i1
    %270 = arith.andi %268, %269 : i1
    %271 = arith.select %66, %267, %270 : i1
    %272 = arith.andi %259, %261 : i1
    %273 = arith.andi %66, %272 : i1
    %274 = arith.select %68, %271, %273 : i1
    %275 = arith.xori %66, %true : i1
    %276 = arith.xori %73, %true : i1
    %277 = arith.ori %276, %267 : i1
    %278 = arith.ori %275, %277 : i1
    %279 = arith.select %76, %271, %278 : i1
    %280 = arith.ori %276, %272 : i1
    %281 = arith.ori %275, %280 : i1
    %282 = arith.select %68, %279, %281 : i1
    %283 = arith.select %79, %274, %282 : i1
    %284 = arith.ori %264, %262 : i1
    %285 = arith.select %75, %262, %284 : i1
    %286 = arith.select %66, %285, %269 : i1
    %287 = arith.andi %66, %261 : i1
    %288 = arith.select %68, %286, %287 : i1
    %289 = arith.select %70, %283, %288 : i1
    %290 = arith.andi %38, %289 : i1
    %291 = arith.andi %224, %290 : i1
    %292 = arith.select %196, %195, %c0_i64 : i64
    %293 = arith.cmpi eq, %292, %c0_i64 : i64
    %294 = arith.select %293, %false, %229 : i1
    %295 = arith.cmpi eq, %292, %c1_i64 : i64
    %296 = arith.select %295, %false, %230 : i1
    %297 = arith.cmpi eq, %292, %c2_i64 : i64
    %298 = arith.select %297, %false, %231 : i1
    %299 = arith.select %236, %true, %230 : i1 loc(#loc22)
    %300 = arith.select %233, %true, %229 : i1 loc(#loc22)
    %301 = arith.select %233, %230, %299 : i1 loc(#loc22)
    %302 = arith.select %228, %229, %300 : i1 loc(#loc22)
    %303 = arith.select %228, %230, %301 : i1 loc(#loc22)
    %304 = arith.select %228, %true, %231 : i1 loc(#loc22)
    %305 = arith.select %291, %294, %302 : i1 loc(#loc22)
    %306 = arith.select %291, %296, %303 : i1 loc(#loc22)
    %307 = arith.select %291, %298, %304 : i1 loc(#loc22)
    acir.next %305 {aspect = "value", resource = "top.age.used$value$0"} : i1 loc(#loc22)
    acir.next %306 {aspect = "value", resource = "top.age.used$value$1"} : i1 loc(#loc22)
    acir.next %307 {aspect = "value", resource = "top.age.used$value$2"} : i1 loc(#loc22)
    %308 = arith.addi %0, %c1_i32 : i32 loc(#loc23)
    %309 = arith.select %227, %308, %0 : i32 loc(#loc23)
    acir.next %309 {aspect = "value", resource = "top.cycle$value"} : i32 loc(#loc23)
    %310 = acir.current {aspect = "value", resource = "top.issued$value"} : i32 loc(#loc23)
    %311 = arith.addi %310, %c1_i32 : i32 loc(#loc23)
    %312 = arith.select %291, %311, %310 : i32 loc(#loc23)
    acir.next %312 {aspect = "value", resource = "top.issued$value"} : i32 loc(#loc23)
    %313 = acir.current {aspect = "value", resource = "top.output$storage$0"} : i32 loc(#loc6)
    %314 = acir.current {aspect = "value", resource = "top.last$value"} : i32 loc(#loc20)
    %315 = arith.select %216, %313, %314 : i32 loc(#loc20)
    acir.next %315 {aspect = "value", resource = "top.last$value"} : i32 loc(#loc20)
    %316 = arith.cmpi eq, %217, %c0_i32 : i32 loc(#loc20)
    %317 = arith.select %316, %313, %c0_i32 : i32 loc(#loc20)
    %318 = arith.select %111, %25, %c0_i32 : i32
    %319 = arith.select %109, %6, %318 : i32
    %320 = arith.select %108, %16, %319 : i32
    %321 = arith.select %291, %218, %c0_i32 : i32
    %322 = arith.cmpi eq, %321, %c0_i32 : i32
    %323 = arith.select %322, %320, %317 : i32
    %324 = arith.select %291, %323, %317 : i32
    acir.next %324 {aspect = "value", resource = "top.output$storage$0"} : i32
    %325 = arith.extui %291 : i1 to i32
    %326 = arith.addi %218, %325 : i32
    acir.next %326 {aspect = "value", resource = "top.output$count"} : i32
    %327 = acir.current {aspect = "value", resource = "top.retired$value"} : i32
    %328 = arith.addi %327, %c1_i32 : i32
    %329 = arith.select %216, %328, %327 : i32
    acir.next %329 {aspect = "value", resource = "top.retired$value"} : i32
    %330 = arith.andi %261, %108 : i1
    %331 = arith.andi %260, %330 : i1
    %332 = arith.andi %259, %331 : i1
    %333 = arith.select %72, %332, %108 : i1
    %334 = arith.select %75, %332, %333 : i1
    %335 = arith.select %78, %332, %334 : i1
    %336 = arith.andi %269, %108 : i1
    %337 = arith.andi %268, %336 : i1
    %338 = arith.select %66, %335, %337 : i1
    %339 = arith.andi %259, %330 : i1
    %340 = arith.andi %66, %339 : i1
    %341 = arith.select %68, %338, %340 : i1
    %342 = arith.select %73, %335, %108 : i1
    %343 = arith.select %66, %342, %108 : i1
    %344 = arith.select %76, %338, %343 : i1
    %345 = arith.select %73, %339, %108 : i1
    %346 = arith.select %66, %345, %108 : i1
    %347 = arith.select %68, %344, %346 : i1
    %348 = arith.select %79, %341, %347 : i1
    %349 = arith.select %72, %331, %108 : i1
    %350 = arith.select %75, %331, %349 : i1
    %351 = arith.select %66, %350, %336 : i1
    %352 = arith.andi %66, %330 : i1
    %353 = arith.select %68, %351, %352 : i1
    %354 = arith.select %70, %348, %353 : i1
    %355 = arith.andi %38, %354 : i1
    %356 = arith.andi %224, %355 : i1
    %357 = arith.select %233, %true, %14 : i1
    %358 = arith.select %233, %true, %15 : i1
    %359 = arith.select %233, %c10_i32, %16 : i32
    %360 = arith.select %356, %false, %357 : i1
    %361 = arith.select %356, %false, %358 : i1
    %362 = arith.select %356, %c0_i32, %359 : i32
    acir.next %360 {aspect = "value", resource = "top.slots.0.state$value$0"} : i1
    acir.next %361 {aspect = "value", resource = "top.slots.0.state$value$1"} : i1
    acir.next %362 {aspect = "value", resource = "top.slots.0.state$value$2"} : i32
    %363 = arith.andi %120, %109 : i1
    %364 = arith.andi %261, %363 : i1
    %365 = arith.andi %260, %364 : i1
    %366 = arith.andi %259, %365 : i1
    %367 = arith.select %72, %366, %363 : i1
    %368 = arith.select %75, %366, %367 : i1
    %369 = arith.select %78, %366, %368 : i1
    %370 = arith.andi %269, %363 : i1
    %371 = arith.andi %268, %370 : i1
    %372 = arith.select %66, %369, %371 : i1
    %373 = arith.andi %259, %364 : i1
    %374 = arith.andi %66, %373 : i1
    %375 = arith.select %68, %372, %374 : i1
    %376 = arith.select %73, %369, %363 : i1
    %377 = arith.select %66, %376, %363 : i1
    %378 = arith.select %76, %372, %377 : i1
    %379 = arith.select %73, %373, %363 : i1
    %380 = arith.select %66, %379, %363 : i1
    %381 = arith.select %68, %378, %380 : i1
    %382 = arith.select %79, %375, %381 : i1
    %383 = arith.select %72, %365, %363 : i1
    %384 = arith.select %75, %365, %383 : i1
    %385 = arith.select %66, %384, %370 : i1
    %386 = arith.andi %66, %364 : i1
    %387 = arith.select %68, %385, %386 : i1
    %388 = arith.select %70, %382, %387 : i1
    %389 = arith.andi %38, %388 : i1
    %390 = arith.andi %224, %389 : i1
    %391 = arith.select %236, %true, %4 : i1 loc(#loc24)
    %392 = arith.select %236, %true, %5 : i1 loc(#loc24)
    %393 = arith.select %236, %c30_i32, %6 : i32 loc(#loc24)
    %394 = arith.select %390, %false, %391 : i1 loc(#loc24)
    %395 = arith.select %390, %false, %392 : i1 loc(#loc24)
    %396 = arith.select %390, %c0_i32, %393 : i32 loc(#loc24)
    acir.next %394 {aspect = "value", resource = "top.slots.1.state$value$0"} : i1 loc(#loc24)
    acir.next %395 {aspect = "value", resource = "top.slots.1.state$value$1"} : i1 loc(#loc24)
    acir.next %396 {aspect = "value", resource = "top.slots.1.state$value$2"} : i32 loc(#loc24)
    %397 = arith.andi %127, %111 : i1
    %398 = arith.andi %120, %397 : i1
    %399 = arith.andi %261, %398 : i1
    %400 = arith.andi %260, %399 : i1
    %401 = arith.andi %259, %400 : i1
    %402 = arith.select %72, %401, %398 : i1
    %403 = arith.select %75, %401, %402 : i1
    %404 = arith.select %78, %401, %403 : i1
    %405 = arith.andi %269, %398 : i1
    %406 = arith.andi %268, %405 : i1
    %407 = arith.select %66, %404, %406 : i1
    %408 = arith.andi %259, %399 : i1
    %409 = arith.andi %66, %408 : i1
    %410 = arith.select %68, %407, %409 : i1
    %411 = arith.select %73, %404, %398 : i1
    %412 = arith.select %66, %411, %398 : i1
    %413 = arith.select %76, %407, %412 : i1
    %414 = arith.select %73, %408, %398 : i1
    %415 = arith.select %66, %414, %398 : i1
    %416 = arith.select %68, %413, %415 : i1
    %417 = arith.select %79, %410, %416 : i1
    %418 = arith.select %72, %400, %398 : i1
    %419 = arith.select %75, %400, %418 : i1
    %420 = arith.select %66, %419, %405 : i1
    %421 = arith.andi %66, %399 : i1
    %422 = arith.select %68, %420, %421 : i1
    %423 = arith.select %70, %417, %422 : i1
    %424 = arith.andi %38, %423 : i1
    %425 = arith.andi %224, %424 : i1
    %426 = arith.andi %225, %23 : i1
    %427 = arith.andi %226, %426 : i1
    %428 = arith.select %228, %true, %23 : i1
    %429 = arith.select %228, %false, %24 : i1
    %430 = arith.select %228, %c20_i32, %25 : i32
    %431 = arith.select %427, %23, %428 : i1
    %432 = arith.select %427, %true, %429 : i1
    %433 = arith.select %427, %25, %430 : i32
    %434 = arith.select %425, %false, %431 : i1
    %435 = arith.select %425, %false, %432 : i1
    %436 = arith.select %425, %c0_i32, %433 : i32
    acir.next %434 {aspect = "value", resource = "top.slots.2.state$value$0"} : i1
    acir.next %435 {aspect = "value", resource = "top.slots.2.state$value$1"} : i1
    acir.next %436 {aspect = "value", resource = "top.slots.2.state$value$2"} : i32
    %437 = arith.andi %127, %201 : i1
    %438 = arith.andi %120, %437 : i1
    %439 = arith.xori %438, %true : i1
    %440 = arith.andi %269, %264 : i1
    %441 = arith.ori %261, %440 : i1
    %442 = arith.select %74, %440, %441 : i1
    %443 = arith.select %77, %440, %442 : i1
    %444 = arith.select %78, %263, %443 : i1
    %445 = arith.select %65, %444, %270 : i1
    %446 = arith.select %47, %445, %270 : i1
    %447 = arith.andi %65, %272 : i1
    %448 = arith.andi %47, %447 : i1
    %449 = arith.select %67, %446, %448 : i1
    %450 = arith.select %51, %449, %448 : i1
    %451 = arith.xori %76, %true : i1
    %452 = arith.andi %451, %276 : i1
    %453 = arith.ori %261, %452 : i1
    %454 = arith.select %74, %452, %453 : i1
    %455 = arith.select %77, %452, %454 : i1
    %456 = arith.ori %264, %452 : i1
    %457 = arith.select %75, %452, %456 : i1
    %458 = arith.ori %261, %457 : i1
    %459 = arith.select %74, %457, %458 : i1
    %460 = arith.select %77, %457, %459 : i1
    %461 = arith.select %78, %455, %460 : i1
    %462 = arith.ori %269, %451 : i1
    %463 = arith.select %78, %451, %462 : i1
    %464 = arith.select %65, %461, %463 : i1
    %465 = arith.select %47, %464, %463 : i1
    %466 = arith.xori %65, %true : i1
    %467 = arith.ori %261, %276 : i1
    %468 = arith.select %77, %276, %467 : i1
    %469 = arith.ori %466, %468 : i1
    %470 = arith.ori %48, %469 : i1
    %471 = arith.select %67, %465, %470 : i1
    %472 = arith.select %51, %471, %470 : i1
    %473 = arith.select %79, %450, %472 : i1
    %474 = arith.select %65, %442, %269 : i1
    %475 = arith.select %47, %474, %269 : i1
    %476 = arith.andi %65, %261 : i1
    %477 = arith.andi %47, %476 : i1
    %478 = arith.select %67, %475, %477 : i1
    %479 = arith.select %51, %478, %477 : i1
    %480 = arith.select %69, %473, %479 : i1
    %481 = arith.select %56, %480, %479 : i1
    %482 = arith.andi %38, %481 : i1
    %483 = arith.andi %224, %482 : i1
    acir.effect %483(%439) {branch = 16 : i64, frames = ["top:issue", "top:$dispatch10"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> ()
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":115:12)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":20:24)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":117:14)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":126:27)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":102:13)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":107:16)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":26:16)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:27)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":122:12)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":123:71)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":73:32)
#loc12 = loc("acir.binding:top_body.0.call.9.94.0.call.9.4:position")
#loc13 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable1")
#loc14 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable2")
#loc15 = loc("acir.binding:top_body.0.call.9.94.0.call.10.17:$enable0")
#loc16 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:20)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":35:36)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":37:17)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":101:13)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":103:22)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":52:35)
#loc22 = loc("acir.binding:top_body.0.call.0.8.0.call.4.13:flags")
#loc23 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":112:17)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/array_arbiter.py":38:17)
