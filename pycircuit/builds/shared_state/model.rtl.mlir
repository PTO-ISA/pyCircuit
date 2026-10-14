module attributes {acir.config = {choose_left = true}, acir.model = {config = [{name = "choose_left", type = "bool"}], constants = {}, format_version = 9 : i64, top = "SharedState", type_order = [], types = {}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [], logical_states = [{count = "top.first$count", kind = "queue", name = "top.first", parts = [{path = [0], resource = "top.first$storage$0"}], prototype = [3 : i32]}, {count = "top.independent_out$count", kind = "queue", name = "top.independent_out", parts = [{path = [0], resource = "top.independent_out$storage$0"}], prototype = [0 : i32]}, {count = "top.left_out$count", kind = "queue", name = "top.left_out", parts = [{path = [0], resource = "top.left_out$storage$0"}], prototype = [0 : i32]}, {count = "top.right_out$count", kind = "queue", name = "top.right_out", parts = [{path = [0], resource = "top.right_out$storage$0"}], prototype = [0 : i32]}, {count = "top.second$count", kind = "queue", name = "top.second", parts = [{path = [0], resource = "top.second$storage$0"}], prototype = [5 : i32]}, {kind = "cell", name = "top.separate.total", parts = [{path = [], resource = "top.separate.total$value"}], prototype = 0 : i32}, {kind = "cell", name = "top.shared.total", parts = [{path = [], resource = "top.shared.total$value"}], prototype = 0 : i32}, {count = "top.third$count", kind = "queue", name = "top.third", parts = [{path = [0], resource = "top.third$storage$0"}], prototype = [9 : i32]}], schedule = [0, 1, 2], stage = "rtl"} {
    acir.state @top.first$count : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.first$storage$0 : i32 {initial = 3 : i32, kind = "cell"}
    acir.state @top.independent_out$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.independent_out$storage$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left_out$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.left_out$storage$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right_out$count : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.right_out$storage$0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.second$count : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.second$storage$0 : i32 {initial = 5 : i32, kind = "cell"}
    acir.state @top.separate.total$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.shared.total$value : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.third$count : i32 {initial = 1 : i32, kind = "cell"}
    acir.state @top.third$storage$0 : i32 {initial = 9 : i32, kind = "cell"}
    %true = arith.constant true
    %0 = acir.export %true {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1
    %1 = arith.xori %0, %true : i1 loc(#loc1)
    %true_0 = arith.constant true
    %2 = arith.ori %1, %true_0 : i1
    %3 = arith.andi %0, %2 : i1
    %4 = acir.current {aspect = "value", resource = "top.first$count"} : i32 loc(#loc2)
    %c0_i32 = arith.constant 0 : i32 loc(#loc2)
    %5 = arith.cmpi ne, %4, %c0_i32 : i32 loc(#loc2)
    %6 = arith.ori %1, %5 : i1 loc(#loc2)
    %7 = arith.andi %3, %6 : i1 loc(#loc2)
    %8 = arith.andi %7, %6 : i1 loc(#loc2)
    %9 = arith.ori %1, %true_0 : i1 loc(#loc2)
    %10 = arith.andi %8, %9 : i1 loc(#loc2)
    %c0_i32_1 = arith.constant 0 : i32 loc(#loc2)
    %11 = arith.cmpi eq, %4, %c0_i32_1 : i32 loc(#loc2)
    %12 = arith.xori %11, %true : i1 loc(#loc2)
    %13 = arith.ori %1, %12 : i1 loc(#loc2)
    %14 = arith.andi %10, %13 : i1 loc(#loc2)
    %15 = arith.andi %true_0, %5 : i1 loc(#loc1)
    %16 = arith.ori %1, %15 : i1 loc(#loc1)
    %17 = arith.andi %14, %16 : i1 loc(#loc1)
    %18 = arith.andi %17, %6 : i1 loc(#loc1)
    %19 = arith.andi %18, %6 : i1 loc(#loc1)
    %20 = arith.ori %1, %true_0 : i1 loc(#loc3)
    %21 = arith.andi %19, %20 : i1 loc(#loc3)
    %c0_i32_2 = arith.constant 0 : i32 loc(#loc3)
    %22 = arith.cmpi eq, %4, %c0_i32_2 : i32 loc(#loc3)
    %23 = arith.xori %22, %true : i1 loc(#loc3)
    %24 = arith.ori %1, %23 : i1 loc(#loc3)
    %25 = arith.andi %21, %24 : i1 loc(#loc3)
    %26 = arith.andi %0, %5 : i1
    %27 = arith.xori %26, %true : i1
    %28 = arith.ori %27, %true_0 : i1
    %29 = arith.andi %25, %28 : i1
    %30 = acir.current {aspect = "value", resource = "top.left_out$count"} : i32
    %c0_i32_3 = arith.constant 0 : i32
    %31 = arith.subi %30, %c0_i32_3 : i32
    %c1_i32 = arith.constant 1 : i32
    %32 = arith.cmpi uge, %31, %c1_i32 : i32
    %33 = arith.xori %32, %true : i1
    %34 = arith.ori %27, %33 : i1
    %35 = arith.andi %29, %34 : i1
    %36 = acir.fire "top.left.transfer#0" %35 {successors = [], txn = 0 : i64}
    %37 = acir.unary %true "not" {acir.id = "v7", acir.source_type = "bool"} : i1 -> i1 loc(#loc4)
    %38 = acir.export %37 {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc4)
    %39 = arith.xori %38, %true : i1 loc(#loc1)
    %40 = arith.ori %39, %true_0 : i1 loc(#loc1)
    %41 = arith.andi %38, %40 : i1 loc(#loc1)
    %42 = acir.current {aspect = "value", resource = "top.second$count"} : i32 loc(#loc2)
    %c0_i32_4 = arith.constant 0 : i32 loc(#loc2)
    %43 = arith.cmpi ne, %42, %c0_i32_4 : i32 loc(#loc2)
    %44 = arith.ori %39, %43 : i1 loc(#loc2)
    %45 = arith.andi %41, %44 : i1 loc(#loc2)
    %46 = arith.andi %45, %44 : i1 loc(#loc2)
    %47 = arith.ori %39, %true_0 : i1 loc(#loc2)
    %48 = arith.andi %46, %47 : i1 loc(#loc2)
    %c0_i32_5 = arith.constant 0 : i32 loc(#loc2)
    %49 = arith.cmpi eq, %42, %c0_i32_5 : i32 loc(#loc2)
    %50 = arith.xori %49, %true : i1 loc(#loc2)
    %51 = arith.ori %39, %50 : i1 loc(#loc2)
    %52 = arith.andi %48, %51 : i1 loc(#loc2)
    %53 = arith.andi %true_0, %43 : i1 loc(#loc1)
    %54 = arith.ori %39, %53 : i1 loc(#loc1)
    %55 = arith.andi %52, %54 : i1 loc(#loc1)
    %56 = arith.andi %55, %44 : i1 loc(#loc1)
    %57 = arith.andi %56, %44 : i1 loc(#loc1)
    %58 = arith.ori %39, %true_0 : i1 loc(#loc3)
    %59 = arith.andi %57, %58 : i1 loc(#loc3)
    %c0_i32_6 = arith.constant 0 : i32 loc(#loc3)
    %60 = arith.cmpi eq, %42, %c0_i32_6 : i32 loc(#loc3)
    %61 = arith.xori %60, %true : i1 loc(#loc3)
    %62 = arith.ori %39, %61 : i1 loc(#loc3)
    %63 = arith.andi %59, %62 : i1 loc(#loc3)
    %64 = arith.andi %38, %43 : i1
    %65 = arith.xori %64, %true : i1
    %66 = arith.ori %65, %true_0 : i1
    %67 = arith.andi %63, %66 : i1
    %68 = acir.current {aspect = "value", resource = "top.right_out$count"} : i32
    %c0_i32_7 = arith.constant 0 : i32
    %69 = arith.subi %68, %c0_i32_7 : i32
    %c1_i32_8 = arith.constant 1 : i32
    %70 = arith.cmpi uge, %69, %c1_i32_8 : i32
    %71 = arith.xori %70, %true : i1
    %72 = arith.ori %65, %71 : i1
    %73 = arith.andi %67, %72 : i1
    %74 = acir.fire "top.right.transfer#1" %73 {successors = [], txn = 1 : i64}
    %75 = acir.export %true {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1
    %76 = arith.xori %75, %true : i1 loc(#loc1)
    %77 = arith.ori %76, %true_0 : i1 loc(#loc1)
    %78 = arith.andi %75, %77 : i1 loc(#loc1)
    %79 = acir.current {aspect = "value", resource = "top.third$count"} : i32 loc(#loc2)
    %c0_i32_9 = arith.constant 0 : i32 loc(#loc2)
    %80 = arith.cmpi ne, %79, %c0_i32_9 : i32 loc(#loc2)
    %81 = arith.ori %76, %80 : i1 loc(#loc2)
    %82 = arith.andi %78, %81 : i1 loc(#loc2)
    %83 = arith.andi %82, %81 : i1 loc(#loc2)
    %84 = arith.ori %76, %true_0 : i1 loc(#loc2)
    %85 = arith.andi %83, %84 : i1 loc(#loc2)
    %c0_i32_10 = arith.constant 0 : i32 loc(#loc2)
    %86 = arith.cmpi eq, %79, %c0_i32_10 : i32 loc(#loc2)
    %87 = arith.xori %86, %true : i1 loc(#loc2)
    %88 = arith.ori %76, %87 : i1 loc(#loc2)
    %89 = arith.andi %85, %88 : i1 loc(#loc2)
    %90 = arith.andi %true_0, %80 : i1 loc(#loc1)
    %91 = arith.ori %76, %90 : i1 loc(#loc1)
    %92 = arith.andi %89, %91 : i1 loc(#loc1)
    %93 = arith.andi %92, %81 : i1 loc(#loc1)
    %94 = arith.andi %93, %81 : i1 loc(#loc1)
    %95 = arith.ori %76, %true_0 : i1 loc(#loc3)
    %96 = arith.andi %94, %95 : i1 loc(#loc3)
    %c0_i32_11 = arith.constant 0 : i32 loc(#loc3)
    %97 = arith.cmpi eq, %79, %c0_i32_11 : i32 loc(#loc3)
    %98 = arith.xori %97, %true : i1 loc(#loc3)
    %99 = arith.ori %76, %98 : i1 loc(#loc3)
    %100 = arith.andi %96, %99 : i1 loc(#loc3)
    %101 = arith.andi %75, %80 : i1
    %102 = arith.xori %101, %true : i1
    %103 = arith.ori %102, %true_0 : i1
    %104 = arith.andi %100, %103 : i1
    %105 = acir.current {aspect = "value", resource = "top.independent_out$count"} : i32
    %c0_i32_12 = arith.constant 0 : i32
    %106 = arith.subi %105, %c0_i32_12 : i32
    %c1_i32_13 = arith.constant 1 : i32
    %107 = arith.cmpi uge, %106, %c1_i32_13 : i32
    %108 = arith.xori %107, %true : i1
    %109 = arith.ori %102, %108 : i1
    %110 = arith.andi %104, %109 : i1
    %111 = acir.fire "top.independent.transfer#2" %110 {successors = [], txn = 2 : i64}
    %c0_i32_14 = arith.constant 0 : i32
    %112 = arith.andi %36, %0 : i1
    %c1_i32_15 = arith.constant 1 : i32
    %c0_i32_16 = arith.constant 0 : i32
    %113 = arith.select %112, %c1_i32_15, %c0_i32_16 : i32
    %114 = arith.addi %c0_i32_14, %113 : i32
    %c0_i32_17 = arith.constant 0 : i32
    %115 = arith.cmpi eq, %114, %c0_i32_17 : i32
    %116 = acir.current {aspect = "value", resource = "top.first$storage$0"} : i32 loc(#loc2)
    %117 = acir.aggregate(%116) : (i32) -> tuple<i32> loc(#loc2)
    %c0_i32_18 = arith.constant 0 : i32
    %c0_i32_19 = arith.constant 0 : i32
    %118 = acir.extract %117[%c0_i32_19] : (tuple<i32>, i32) -> i32
    %c0_i32_20 = arith.constant 0 : i32
    %119 = arith.select %115, %118, %c0_i32_20 : i32
    %120 = acir.aggregate(%119) : (i32) -> tuple<i32>
    %c0_i32_21 = arith.constant 0 : i32
    %121 = acir.extract %120[%c0_i32_21] : (tuple<i32>, i32) -> i32
    acir.next %121 {aspect = "value", resource = "top.first$storage$0"} : i32
    %122 = arith.subi %4, %114 : i32
    acir.next %122 {aspect = "value", resource = "top.first$count"} : i32
    %123 = arith.andi %111, %101 : i1
    %c0_i32_22 = arith.constant 0 : i32
    %c0_i32_23 = arith.constant 0 : i32
    %124 = arith.cmpi eq, %c0_i32_22, %c0_i32_23 : i32
    %125 = acir.current {aspect = "value", resource = "top.independent_out$storage$0"} : i32
    %126 = acir.aggregate(%125) : (i32) -> tuple<i32>
    %c0_i32_24 = arith.constant 0 : i32
    %c0_i32_25 = arith.constant 0 : i32
    %127 = acir.extract %126[%c0_i32_25] : (tuple<i32>, i32) -> i32
    %c0_i32_26 = arith.constant 0 : i32
    %128 = arith.select %124, %127, %c0_i32_26 : i32
    %129 = acir.aggregate(%128) : (i32) -> tuple<i32>
    %130 = acir.current {aspect = "value", resource = "top.third$storage$0"} : i32 loc(#loc2)
    %131 = acir.aggregate(%130) : (i32) -> tuple<i32> loc(#loc2)
    %c0_i32_27 = arith.constant 0 : i32 loc(#loc2)
    %c0_i32_28 = arith.constant 0 : i32 loc(#loc2)
    %132 = acir.extract %131[%c0_i32_28] : (tuple<i32>, i32) -> i32 loc(#loc2)
    %133 = arith.subi %105, %c0_i32_22 : i32
    %c0_i32_29 = arith.constant 0 : i32
    %134 = arith.select %123, %133, %c0_i32_29 : i32
    %c0_i32_30 = arith.constant 0 : i32
    %135 = acir.extract %129[%c0_i32_30] : (tuple<i32>, i32) -> i32
    %c0_i32_31 = arith.constant 0 : i32
    %136 = arith.cmpi eq, %134, %c0_i32_31 : i32
    %137 = arith.select %136, %132, %135 : i32
    %138 = acir.aggregate(%137) : (i32) -> tuple<i32>
    %c0_i32_32 = arith.constant 0 : i32
    %139 = acir.extract %138[%c0_i32_32] : (tuple<i32>, i32) -> i32
    %c0_i32_33 = arith.constant 0 : i32
    %140 = acir.extract %129[%c0_i32_33] : (tuple<i32>, i32) -> i32
    %141 = arith.select %123, %139, %140 : i32
    %142 = acir.aggregate(%141) : (i32) -> tuple<i32>
    %c0_i32_34 = arith.constant 0 : i32
    %143 = acir.extract %142[%c0_i32_34] : (tuple<i32>, i32) -> i32
    acir.next %143 {aspect = "value", resource = "top.independent_out$storage$0"} : i32
    %c1_i32_35 = arith.constant 1 : i32
    %c0_i32_36 = arith.constant 0 : i32
    %144 = arith.select %123, %c1_i32_35, %c0_i32_36 : i32
    %145 = arith.addi %133, %144 : i32
    acir.next %145 {aspect = "value", resource = "top.independent_out$count"} : i32
    %146 = arith.andi %36, %26 : i1
    %c0_i32_37 = arith.constant 0 : i32
    %c0_i32_38 = arith.constant 0 : i32
    %147 = arith.cmpi eq, %c0_i32_37, %c0_i32_38 : i32
    %148 = acir.current {aspect = "value", resource = "top.left_out$storage$0"} : i32
    %149 = acir.aggregate(%148) : (i32) -> tuple<i32>
    %c0_i32_39 = arith.constant 0 : i32
    %c0_i32_40 = arith.constant 0 : i32
    %150 = acir.extract %149[%c0_i32_40] : (tuple<i32>, i32) -> i32
    %c0_i32_41 = arith.constant 0 : i32
    %151 = arith.select %147, %150, %c0_i32_41 : i32
    %152 = acir.aggregate(%151) : (i32) -> tuple<i32>
    %c0_i32_42 = arith.constant 0 : i32 loc(#loc2)
    %c0_i32_43 = arith.constant 0 : i32 loc(#loc2)
    %153 = acir.extract %117[%c0_i32_43] : (tuple<i32>, i32) -> i32 loc(#loc2)
    %154 = arith.subi %30, %c0_i32_37 : i32
    %c0_i32_44 = arith.constant 0 : i32
    %155 = arith.select %146, %154, %c0_i32_44 : i32
    %c0_i32_45 = arith.constant 0 : i32
    %156 = acir.extract %152[%c0_i32_45] : (tuple<i32>, i32) -> i32
    %c0_i32_46 = arith.constant 0 : i32
    %157 = arith.cmpi eq, %155, %c0_i32_46 : i32
    %158 = arith.select %157, %153, %156 : i32
    %159 = acir.aggregate(%158) : (i32) -> tuple<i32>
    %c0_i32_47 = arith.constant 0 : i32
    %160 = acir.extract %159[%c0_i32_47] : (tuple<i32>, i32) -> i32
    %c0_i32_48 = arith.constant 0 : i32
    %161 = acir.extract %152[%c0_i32_48] : (tuple<i32>, i32) -> i32
    %162 = arith.select %146, %160, %161 : i32
    %163 = acir.aggregate(%162) : (i32) -> tuple<i32>
    %c0_i32_49 = arith.constant 0 : i32
    %164 = acir.extract %163[%c0_i32_49] : (tuple<i32>, i32) -> i32
    acir.next %164 {aspect = "value", resource = "top.left_out$storage$0"} : i32
    %c1_i32_50 = arith.constant 1 : i32
    %c0_i32_51 = arith.constant 0 : i32
    %165 = arith.select %146, %c1_i32_50, %c0_i32_51 : i32
    %166 = arith.addi %154, %165 : i32
    acir.next %166 {aspect = "value", resource = "top.left_out$count"} : i32
    %167 = arith.andi %74, %64 : i1
    %c0_i32_52 = arith.constant 0 : i32
    %c0_i32_53 = arith.constant 0 : i32
    %168 = arith.cmpi eq, %c0_i32_52, %c0_i32_53 : i32
    %169 = acir.current {aspect = "value", resource = "top.right_out$storage$0"} : i32
    %170 = acir.aggregate(%169) : (i32) -> tuple<i32>
    %c0_i32_54 = arith.constant 0 : i32
    %c0_i32_55 = arith.constant 0 : i32
    %171 = acir.extract %170[%c0_i32_55] : (tuple<i32>, i32) -> i32
    %c0_i32_56 = arith.constant 0 : i32
    %172 = arith.select %168, %171, %c0_i32_56 : i32
    %173 = acir.aggregate(%172) : (i32) -> tuple<i32>
    %174 = acir.current {aspect = "value", resource = "top.second$storage$0"} : i32 loc(#loc2)
    %175 = acir.aggregate(%174) : (i32) -> tuple<i32> loc(#loc2)
    %c0_i32_57 = arith.constant 0 : i32 loc(#loc2)
    %c0_i32_58 = arith.constant 0 : i32 loc(#loc2)
    %176 = acir.extract %175[%c0_i32_58] : (tuple<i32>, i32) -> i32 loc(#loc2)
    %177 = arith.subi %68, %c0_i32_52 : i32
    %c0_i32_59 = arith.constant 0 : i32
    %178 = arith.select %167, %177, %c0_i32_59 : i32
    %c0_i32_60 = arith.constant 0 : i32
    %179 = acir.extract %173[%c0_i32_60] : (tuple<i32>, i32) -> i32
    %c0_i32_61 = arith.constant 0 : i32
    %180 = arith.cmpi eq, %178, %c0_i32_61 : i32
    %181 = arith.select %180, %176, %179 : i32
    %182 = acir.aggregate(%181) : (i32) -> tuple<i32>
    %c0_i32_62 = arith.constant 0 : i32
    %183 = acir.extract %182[%c0_i32_62] : (tuple<i32>, i32) -> i32
    %c0_i32_63 = arith.constant 0 : i32
    %184 = acir.extract %173[%c0_i32_63] : (tuple<i32>, i32) -> i32
    %185 = arith.select %167, %183, %184 : i32
    %186 = acir.aggregate(%185) : (i32) -> tuple<i32>
    %c0_i32_64 = arith.constant 0 : i32
    %187 = acir.extract %186[%c0_i32_64] : (tuple<i32>, i32) -> i32
    acir.next %187 {aspect = "value", resource = "top.right_out$storage$0"} : i32
    %c1_i32_65 = arith.constant 1 : i32
    %c0_i32_66 = arith.constant 0 : i32
    %188 = arith.select %167, %c1_i32_65, %c0_i32_66 : i32
    %189 = arith.addi %177, %188 : i32
    acir.next %189 {aspect = "value", resource = "top.right_out$count"} : i32
    %c0_i32_67 = arith.constant 0 : i32
    %190 = arith.andi %74, %38 : i1
    %c1_i32_68 = arith.constant 1 : i32
    %c0_i32_69 = arith.constant 0 : i32
    %191 = arith.select %190, %c1_i32_68, %c0_i32_69 : i32
    %192 = arith.addi %c0_i32_67, %191 : i32
    %c0_i32_70 = arith.constant 0 : i32
    %193 = arith.cmpi eq, %192, %c0_i32_70 : i32
    %c0_i32_71 = arith.constant 0 : i32
    %c0_i32_72 = arith.constant 0 : i32
    %194 = acir.extract %175[%c0_i32_72] : (tuple<i32>, i32) -> i32
    %c0_i32_73 = arith.constant 0 : i32
    %195 = arith.select %193, %194, %c0_i32_73 : i32
    %196 = acir.aggregate(%195) : (i32) -> tuple<i32>
    %c0_i32_74 = arith.constant 0 : i32
    %197 = acir.extract %196[%c0_i32_74] : (tuple<i32>, i32) -> i32
    acir.next %197 {aspect = "value", resource = "top.second$storage$0"} : i32
    %198 = arith.subi %42, %192 : i32
    acir.next %198 {aspect = "value", resource = "top.second$count"} : i32
    %199 = arith.andi %111, %75 : i1
    %200 = acir.current {aspect = "value", resource = "top.separate.total$value"} : i32 loc(#loc1)
    %201 = arith.addi %200, %132 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc1)
    %202 = arith.select %199, %201, %200 : i32 loc(#loc1)
    acir.next %202 {aspect = "value", resource = "top.separate.total$value"} : i32 loc(#loc1)
    %203 = acir.current {aspect = "value", resource = "top.shared.total$value"} : i32 loc(#loc1)
    %204 = arith.addi %203, %176 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc1)
    %205 = arith.addi %203, %153 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc1)
    %206 = arith.select %112, %205, %203 : i32 loc(#loc1)
    %207 = arith.select %190, %204, %206 : i32 loc(#loc1)
    acir.next %207 {aspect = "value", resource = "top.shared.total$value"} : i32 loc(#loc1)
    %c0_i32_75 = arith.constant 0 : i32 loc(#loc1)
    %c1_i32_76 = arith.constant 1 : i32 loc(#loc1)
    %c0_i32_77 = arith.constant 0 : i32 loc(#loc1)
    %208 = arith.select %199, %c1_i32_76, %c0_i32_77 : i32 loc(#loc1)
    %209 = arith.addi %c0_i32_75, %208 : i32 loc(#loc1)
    %c0_i32_78 = arith.constant 0 : i32 loc(#loc1)
    %210 = arith.cmpi eq, %209, %c0_i32_78 : i32 loc(#loc1)
    %c0_i32_79 = arith.constant 0 : i32 loc(#loc1)
    %c0_i32_80 = arith.constant 0 : i32 loc(#loc1)
    %211 = acir.extract %131[%c0_i32_80] : (tuple<i32>, i32) -> i32 loc(#loc1)
    %c0_i32_81 = arith.constant 0 : i32 loc(#loc1)
    %212 = arith.select %210, %211, %c0_i32_81 : i32 loc(#loc1)
    %213 = acir.aggregate(%212) : (i32) -> tuple<i32> loc(#loc1)
    %c0_i32_82 = arith.constant 0 : i32 loc(#loc1)
    %214 = acir.extract %213[%c0_i32_82] : (tuple<i32>, i32) -> i32 loc(#loc1)
    acir.next %214 {aspect = "value", resource = "top.third$storage$0"} : i32 loc(#loc1)
    %215 = arith.subi %79, %209 : i32 loc(#loc1)
    acir.next %215 {aspect = "value", resource = "top.third$count"} : i32 loc(#loc1)
    %false = arith.constant false
    %216 = acir.unary %false "not" {acir.source_type = "bool"} : i1 -> i1
    acir.effect %112(%216) {branch = 0 : i64, frames = ["top.left:transfer", "top.shared:add"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> ()
    acir.effect %190(%216) {branch = 3 : i64, frames = ["top.right:transfer", "top.shared:add"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> ()
    acir.effect %199(%216) {branch = 6 : i64, frames = ["top.independent:transfer", "top.separate:add"], method = "check", port = 0 : i64, resource = "", txn = 2 : i64} : (i1, i1) -> ()
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:17)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":12:25)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":13:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/shared_state.py":47:43)
