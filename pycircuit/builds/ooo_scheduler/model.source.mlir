module attributes {acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, format_version = 9 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default, name = "lhs", payload_type = !acir.struct<"src_payloads_25ebcaa7.Operand">, type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", payload_type = !acir.struct<"src_payloads_25ebcaa7.Operand">, type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = false, name = "done", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "tag", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}} {
  func.func @ac_init_top.source_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.source_cap", params = [], result = "u32"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc1)
    return %c4_i32 : i32
  }
  func.func @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">> attributes {acir.function = {kind = "helper", name = "ac_init_top.source", params = [], result = "tuple<src_payloads_25ebcaa7.MicroOp,src_payloads_25ebcaa7.MicroOp,src_payloads_25ebcaa7.MicroOp,src_payloads_25ebcaa7.MicroOp>"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc2)
    %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc3)
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc4)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc5)
    %0 = acir.aggregate(%c1_i32, %c1_i32_0, %c2_i32, %c3_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc6)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc7)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc8)
    %c1_i32_1 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc9)
    %c2_i32_2 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc10)
    %1 = acir.aggregate(%c0_i32, %c4_i32, %c1_i32_1, %c2_i32_2) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc11)
    %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc12)
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32 loc(#loc13)
    %c2_i32_4 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc14)
    %c3_i32_5 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc15)
    %2 = acir.aggregate(%c0_i32_3, %c5_i32, %c2_i32_4, %c3_i32_5) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc16)
    %c0_i32_6 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc17)
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32 loc(#loc18)
    %c4_i32_7 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc19)
    %c5_i32_8 = arith.constant {acir.source_type = "u32"} 5 : i32 loc(#loc20)
    %3 = acir.aggregate(%c0_i32_6, %c6_i32, %c4_i32_7, %c5_i32_8) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc21)
    %4 = acir.aggregate(%0, %1, %2, %3) {acir.id = "v22", acir.source_type = "tuple<src_payloads_25ebcaa7.MicroOp,src_payloads_25ebcaa7.MicroOp,src_payloads_25ebcaa7.MicroOp,src_payloads_25ebcaa7.MicroOp>"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">> loc(#loc22)
    return %4 : tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
  }
  func.func @ac_init_top.add_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.add_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.add_requests() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.add_requests", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.mul_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.mul_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.mul_requests() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.mul_requests", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.add_done_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.add_done_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.add_done() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.add_done", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.mul_done_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.mul_done_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.mul_done() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.mul_done", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slots.0.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %0 = acir.aggregate(%false, %false_0, %c0_i32, %c0_i32_1, %c0_i32_2) {acir.id = "v38", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc23)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slots.1.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %0 = acir.aggregate(%false, %false_0, %c0_i32, %c0_i32_1, %c0_i32_2) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc23)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slots.2.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %0 = acir.aggregate(%false, %false_0, %c0_i32, %c0_i32_1, %c0_i32_2) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc23)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slots.3.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %0 = acir.aggregate(%false, %false_0, %c0_i32, %c0_i32_1, %c0_i32_2) {acir.id = "v59", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc23)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.head() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.head", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc24)
    return %c1_i32 : i32
  }
  func.func @ac_init_top.rob.tail() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.tail", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc25)
    return %c1_i32 : i32
  }
  func.func @ac_init_top.rf.registers() -> !acir.array<i32, [8]> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers", params = [], result = "array<u32;8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc26)
    %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc27)
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32 loc(#loc28)
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc29)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc30)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc31)
    %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc32)
    %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc33)
    %0 = acir.aggregate(%c0_i32, %c0_i32_0, %c6_i32, %c7_i32, %c0_i32_1, %c0_i32_2, %c0_i32_3, %c0_i32_4) {acir.id = "v73", acir.source_type = "array<u32;8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [8]> loc(#loc34)
    return %0 : !acir.array<i32, [8]>
  }
  func.func @ac_init_top.rf.renamed() -> !acir.array<i32, [8]> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.renamed", params = [], result = "array<u32;8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc35)
    %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc36)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc37)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc38)
    %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc39)
    %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc40)
    %c0_i32_5 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc41)
    %c0_i32_6 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc42)
    %0 = acir.aggregate(%c0_i32, %c0_i32_0, %c0_i32_1, %c0_i32_2, %c0_i32_3, %c0_i32_4, %c0_i32_5, %c0_i32_6) {acir.id = "v83", acir.source_type = "array<u32;8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [8]> loc(#loc43)
    return %0 : !acir.array<i32, [8]>
  }
  func.func @ac_init_top.rf.committed() -> !acir.array<i32, [8]> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.committed", params = [], result = "array<u32;8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc44)
    %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc45)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc46)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc47)
    %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc48)
    %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc49)
    %c0_i32_5 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc50)
    %c0_i32_6 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc51)
    %0 = acir.aggregate(%c0_i32, %c0_i32_0, %c0_i32_1, %c0_i32_2, %c0_i32_3, %c0_i32_4, %c0_i32_5, %c0_i32_6) {acir.id = "v93", acir.source_type = "array<u32;8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [8]> loc(#loc52)
    return %0 : !acir.array<i32, [8]>
  }
  func.func @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.left.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc53)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc53)
    %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc53)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc53)
    %0 = acir.aggregate() {acir.id = "v99", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc53)
    %1 = acir.aggregate() {acir.id = "v100", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc53)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32_0, %c0_i32_1, %0, %1) {acir.id = "v101", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc53)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.right.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc53)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc53)
    %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc53)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc53)
    %0 = acir.aggregate() {acir.id = "v107", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc53)
    %1 = acir.aggregate() {acir.id = "v108", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc53)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32_0, %c0_i32_1, %0, %1) {acir.id = "v109", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc53)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.multiplier.active() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.multiplier.active", params = [], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc54)
    return %false : i1
  }
  func.func @ac_init_top.multiplier.remaining() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.multiplier.remaining", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc55)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.multiplier.request", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc56)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc56)
    %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc56)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc56)
    %0 = acir.aggregate() {acir.id = "v119", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc56)
    %1 = acir.aggregate() {acir.id = "v120", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc56)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32_0, %c0_i32_1, %0, %1) {acir.id = "v121", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc56)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.commit.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.commit.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc57)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.commit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.commit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc58)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.audit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.audit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc59)
    return %c0_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_model_3cf90884.OutOfOrder", name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.source : !acir.struct<"src_payloads_25ebcaa7.MicroOp"> {capacity_fn = "ac_init_top.source_cap", initial_fn = "ac_init_top.source", name = "top.source", type = "src_payloads_25ebcaa7.MicroOp"}
    acir.queue @top.add_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity_fn = "ac_init_top.add_requests_cap", initial_fn = "ac_init_top.add_requests", name = "top.add_requests", type = "src_payloads_25ebcaa7.Entry"}
    acir.queue @top.mul_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity_fn = "ac_init_top.mul_requests_cap", initial_fn = "ac_init_top.mul_requests", name = "top.mul_requests", type = "src_payloads_25ebcaa7.Entry"}
    acir.queue @top.add_done : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity_fn = "ac_init_top.add_done_cap", initial_fn = "ac_init_top.add_done", name = "top.add_done", type = "src_payloads_25ebcaa7.Completion"}
    acir.queue @top.mul_done : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity_fn = "ac_init_top.mul_done_cap", initial_fn = "ac_init_top.mul_done", name = "top.mul_done", type = "src_payloads_25ebcaa7.Completion"}
    acir.signal @top.event : !acir.struct<"src_payloads_25ebcaa7.Completion"> {name = "top.event", type = "src_payloads_25ebcaa7.Completion"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      acir.module.call @top.scheduler loc(#loc60)
      %0 = acir.ref @top::@top.add_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.module.call @top.adder loc(#loc61)
      %1 = acir.ref @top::@top.mul_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.module.call @top.multiplier loc(#loc62)
      %2 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %3 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.module.call @top.cdb loc(#loc63)
      %4 = acir.ref @top::@top.source {acir.source_type = "queue<src_payloads_25ebcaa7.MicroOp>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
      %5 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %6 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.module.call @top.dispatch loc(#loc64)
      acir.module.call @top.commit loc(#loc65)
      %7 = acir.ref @top::@top.event {acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.module.call @top.audit loc(#loc66)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rob attributes {acir.instance = {definition = "src_state_7e811fd9.ReorderBuffer", name = "top.rob", parent = "top", rules = ["allocate", "complete", "has_room", "lookup", "retire", "room"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "has_room", "lookup", "retire", "room"]}
    acir.cell @top.rob.head : i32 {initial_fn = "ac_init_top.rob.head", name = "top.rob.head", type = "u32"}
    acir.cell @top.rob.tail : i32 {initial_fn = "ac_init_top.rob.tail", name = "top.rob.tail", type = "u32"}
    "acir.instance.array"() <{name = "top.rob.slots", shape = [4], targets = [@top.rob.slots.0, @top.rob.slots.1, @top.rob.slots.2, @top.rob.slots.3]}> : () -> ()
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17 : (i32, i32, i1, i1, i1, i1) -> () {acir.function = {contract = "Module:top.rob", definition = "$dispatch11", guard_info = {name = "", source = "variable"}, instance = "top.rob", interface = "Module:top.rob:$dispatch11", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17", output_type = "void", owner = "top.rob", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}, {id = "v3", name = "$enable0", targets = [], type = "bool"}, {id = "v4", name = "$enable1", targets = [], type = "bool"}, {id = "v5", name = "$enable2", targets = [], type = "bool"}, {id = "v6", name = "$enable3", targets = [], type = "bool"}], public = true, result = "void", rule = "$dispatch11"}} guard {
    ^bb0(%arg0: i32, %arg1: i32, %arg2: i1, %arg3: i1, %arg4: i1, %arg5: i1):
      %0 = scf.if %arg2 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg3 : i1
      }
      %1 = scf.if %0 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg4 : i1
      }
      %2 = scf.if %1 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg5 : i1
      }
      acir.guard.yield %2 {acir.id = "v20", acir.source_type = "void"} loc(#loc67)
    } body {
    ^bb0(%arg0: i32, %arg1: i32, %arg2: i1, %arg3: i1, %arg4: i1, %arg5: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = scf.if %arg2 -> (i1) {
        acir.rule.call @top.rob.slots.0::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.11.21(%arg0, %arg1) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.rob.slots.0", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc67)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        %1 = scf.if %arg3 -> (i1) {
          acir.rule.call @top.rob.slots.1::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.12.23(%arg0, %arg1) {acir.id = "v24", acir.source_type = "void", contract = "Module:top.rob.slots.1", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc67)
          %false = arith.constant {acir.source_type = "bool"} false
          scf.yield %false : i1
        } else {
          %2 = scf.if %arg4 -> (i1) {
            acir.rule.call @top.rob.slots.2::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.13.25(%arg0, %arg1) {acir.id = "v26", acir.source_type = "void", contract = "Module:top.rob.slots.2", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc67)
            %false = arith.constant {acir.source_type = "bool"} false
            scf.yield %false : i1
          } else {
            %3 = scf.if %arg5 -> (i1) {
              acir.rule.call @top.rob.slots.3::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.14.27(%arg0, %arg1) {acir.id = "v28", acir.source_type = "void", contract = "Module:top.rob.slots.3", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc67)
              %false = arith.constant {acir.source_type = "bool"} false
              scf.yield %false : i1
            } else {
              scf.yield %true : i1
            }
            scf.yield %3 : i1
          }
          scf.yield %2 : i1
        }
        scf.yield %1 : i1
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob", definition = "complete", instance = "top.rob", interface = "Module:top.rob:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16", output_type = "void", owner = "top.rob", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc68)
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc69)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc70)
      %1 = arith.andi %0, %c3_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc71)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc67)
      %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32
      %3 = arith.cmpi eq, %1, %c1_i32_0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc67)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc67)
      %c3_i32_1 = arith.constant {acir.source_type = "u32"} 3 : i32
      %5 = arith.cmpi eq, %1, %c3_i32_1 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc67)
      acir.rule.call @top.rob::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17(%arg0, %arg1, %2, %3, %4, %5) {acir.id = "v18", acir.source_type = "void", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:$dispatch11", role = "rule", rule = "$dispatch11"} : (i32, i32, i1, i1, i1, i1) -> () loc(#loc67)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30 : (i1, i1, i1, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "$dispatch20", guard_info = {name = "", source = "variable"}, instance = "top.rob", interface = "Module:top.rob:$dispatch20", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [{id = "v1", name = "$enable0", targets = [], type = "bool"}, {id = "v2", name = "$enable1", targets = [], type = "bool"}, {id = "v3", name = "$enable2", targets = [], type = "bool"}, {id = "v4", name = "$enable3", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "$dispatch20"}} guard {
    ^bb0(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1):
      %0 = scf.if %arg0 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg1 : i1
      }
      %1 = scf.if %0 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg2 : i1
      }
      %2 = scf.if %1 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg3 : i1
      }
      acir.guard.yield %2 {acir.id = "v18", acir.source_type = "void"} loc(#loc72)
    } body {
    ^bb0(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1:3 = scf.if %arg0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %3 = acir.rule.call @top.rob.slots.0::@top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.20.21() {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slots.0", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc72)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %3 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        %3:3 = scf.if %arg1 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
          %4 = acir.rule.call @top.rob.slots.1::@top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.21.24() {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slots.1", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc72)
          %true_0 = arith.constant {acir.source_type = "bool"} true
          %false_1 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_1, %true_0, %4 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        } else {
          %4:3 = scf.if %arg2 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
            %5 = acir.rule.call @top.rob.slots.2::@top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.22.27() {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slots.2", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc72)
            %true_0 = arith.constant {acir.source_type = "bool"} true
            %false_1 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_1, %true_0, %5 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          } else {
            %5:3 = scf.if %arg3 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
              %6 = acir.rule.call @top.rob.slots.3::@top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.23.30() {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slots.3", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc72)
              %true_0 = arith.constant {acir.source_type = "bool"} true
              %false_1 = arith.constant {acir.source_type = "bool"} false
              scf.yield %false_1, %true_0, %6 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
            } else {
              scf.yield %true, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
            }
            scf.yield %5#0, %5#1, %5#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          }
          scf.yield %4#0, %4#1, %4#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        }
        scf.yield %3#0, %3#1, %3#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %2 = acir.unary %1#0 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %2 {acir.source_type = "void"}
      acir.return %1#2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "lookup", instance = "top.rob", interface = "Module:top.rob:lookup", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "lookup"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc73)
      %1 = arith.subi %arg0, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc74)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc75)
      %2 = arith.andi %1, %c3_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc76)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %3 = arith.cmpi eq, %2, %c0_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc72)
      %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32
      %4 = arith.cmpi eq, %2, %c1_i32_0 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc72)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %5 = arith.cmpi eq, %2, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc72)
      %c3_i32_1 = arith.constant {acir.source_type = "u32"} 3 : i32
      %6 = arith.cmpi eq, %2, %c3_i32_1 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc72)
      %c1_i32_2 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc73)
      %7 = arith.subi %arg0, %c1_i32_2 {acir.id = "v20", acir.source_type = "u32"} : i32 loc(#loc74)
      %c3_i32_3 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc75)
      %8 = arith.andi %7, %c3_i32_3 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc76)
      %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32
      %9 = arith.cmpi eq, %8, %c0_i32_4 {acir.id = "v24", acir.source_type = "bool"} : i32 loc(#loc72)
      %c1_i32_5 = arith.constant {acir.source_type = "u32"} 1 : i32
      %10 = arith.cmpi eq, %8, %c1_i32_5 {acir.id = "v26", acir.source_type = "bool"} : i32 loc(#loc72)
      %c2_i32_6 = arith.constant {acir.source_type = "u32"} 2 : i32
      %11 = arith.cmpi eq, %8, %c2_i32_6 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc72)
      %c3_i32_7 = arith.constant {acir.source_type = "u32"} 3 : i32
      %12 = arith.cmpi eq, %8, %c3_i32_7 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc72)
      %13 = acir.rule.call @top.rob::@top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30(%9, %10, %11, %12) {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:$dispatch20", role = "rule", rule = "$dispatch20"} : (i1, i1, i1, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc72)
      %true_8 = arith.constant {acir.source_type = "bool"} true
      %false_9 = arith.constant {acir.source_type = "bool"} false
      %14 = acir.unary %false_9 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %14 {acir.source_type = "void"}
      acir.return %13 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.27.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob", definition = "room", instance = "top.rob", interface = "Module:top.rob:room", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.27.1", output_type = "bool", owner = "top.rob", params = [{id = "v1", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "room"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc77)
      %3 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %4 = acir.cell.read %3 {acir.id = "v10", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc78)
      %5 = arith.subi %2, %4 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc77)
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc79)
      %6 = arith.cmpi ult, %5, %c4_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc77)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %6 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21 : (i32, i32, i1, i1, i1, i1) -> () {acir.function = {contract = "Module:top.rob", definition = "$dispatch28", guard_info = {name = "", source = "variable"}, instance = "top.rob", interface = "Module:top.rob:$dispatch28", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21", output_type = "void", owner = "top.rob", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}, {id = "v3", name = "$enable0", targets = [], type = "bool"}, {id = "v4", name = "$enable1", targets = [], type = "bool"}, {id = "v5", name = "$enable2", targets = [], type = "bool"}, {id = "v6", name = "$enable3", targets = [], type = "bool"}], public = true, result = "void", rule = "$dispatch28"}} guard {
    ^bb0(%arg0: i32, %arg1: i32, %arg2: i1, %arg3: i1, %arg4: i1, %arg5: i1):
      %0 = scf.if %arg2 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg3 : i1
      }
      %1 = scf.if %0 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg4 : i1
      }
      %2 = scf.if %1 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg5 : i1
      }
      acir.guard.yield %2 {acir.id = "v20", acir.source_type = "void"} loc(#loc80)
    } body {
    ^bb0(%arg0: i32, %arg1: i32, %arg2: i1, %arg3: i1, %arg4: i1, %arg5: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = scf.if %arg2 -> (i1) {
        acir.rule.call @top.rob.slots.0::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.28.21(%arg0, %arg1) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.rob.slots.0", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc80)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        %1 = scf.if %arg3 -> (i1) {
          acir.rule.call @top.rob.slots.1::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.30.23(%arg0, %arg1) {acir.id = "v24", acir.source_type = "void", contract = "Module:top.rob.slots.1", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc80)
          %false = arith.constant {acir.source_type = "bool"} false
          scf.yield %false : i1
        } else {
          %2 = scf.if %arg4 -> (i1) {
            acir.rule.call @top.rob.slots.2::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.32.25(%arg0, %arg1) {acir.id = "v26", acir.source_type = "void", contract = "Module:top.rob.slots.2", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc80)
            %false = arith.constant {acir.source_type = "bool"} false
            scf.yield %false : i1
          } else {
            %3 = scf.if %arg5 -> (i1) {
              acir.rule.call @top.rob.slots.3::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.34.27(%arg0, %arg1) {acir.id = "v28", acir.source_type = "void", contract = "Module:top.rob.slots.3", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc80)
              %false = arith.constant {acir.source_type = "bool"} false
              scf.yield %false : i1
            } else {
              scf.yield %true : i1
            }
            scf.yield %3 : i1
          }
          scf.yield %2 : i1
        }
        scf.yield %1 : i1
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10 : (i32) -> i32 {acir.function = {contract = "Module:top.rob", definition = "allocate", guard_info = {name = "room", source = "function"}, instance = "top.rob", interface = "Module:top.rob:allocate", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10", output_type = "u32", owner = "top.rob", params = [{id = "v1", name = "rd", targets = [], type = "u32"}], public = true, result = "u32", rule = "allocate"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.27.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:room", role = "rule", rule = "room"} : (i32) -> i1 loc(#loc81)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc82)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc83)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc84)
      %3 = arith.subi %2, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc85)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc86)
      %4 = arith.andi %3, %c3_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc87)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %5 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc80)
      %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32
      %6 = arith.cmpi eq, %4, %c1_i32_0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc80)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %7 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc80)
      %c3_i32_1 = arith.constant {acir.source_type = "u32"} 3 : i32
      %8 = arith.cmpi eq, %4, %c3_i32_1 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc80)
      acir.rule.call @top.rob::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21(%2, %arg0, %5, %6, %7, %8) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:$dispatch28", role = "rule", rule = "$dispatch28"} : (i32, i32, i1, i1, i1, i1) -> () loc(#loc80)
      %c1_i32_2 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc88)
      %9 = arith.addi %2, %c1_i32_2 {acir.id = "v24", acir.source_type = "u32"} : i32 loc(#loc89)
      %10 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %10, %9 {acir.id = "v26", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc90)
      %true_3 = arith.constant {acir.source_type = "bool"} true
      %false_4 = arith.constant {acir.source_type = "bool"} false
      %11 = acir.unary %false_4 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %11 {acir.source_type = "void"}
      acir.return %2 : i32 {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19 : (i32, i1, i1, i1, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "$dispatch44", guard_info = {name = "", source = "variable"}, instance = "top.rob", interface = "Module:top.rob:$dispatch44", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "$enable0", targets = [], type = "bool"}, {id = "v3", name = "$enable1", targets = [], type = "bool"}, {id = "v4", name = "$enable2", targets = [], type = "bool"}, {id = "v5", name = "$enable3", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "$dispatch44"}} guard {
    ^bb0(%arg0: i32, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1):
      %0 = scf.if %arg1 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg2 : i1
      }
      %1 = scf.if %0 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg3 : i1
      }
      %2 = scf.if %1 -> (i1) {
        %true = arith.constant {acir.source_type = "bool"} true
        scf.yield %true : i1
      } else {
        scf.yield %arg4 : i1
      }
      acir.guard.yield %2 {acir.id = "v19", acir.source_type = "void"} loc(#loc91)
    } body {
    ^bb0(%arg0: i32, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1:3 = scf.if %arg1 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %3 = acir.rule.call @top.rob.slots.0::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.44.22(%arg0) {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slots.0", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc91)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %3 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        %3:3 = scf.if %arg2 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
          %4 = acir.rule.call @top.rob.slots.1::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.46.25(%arg0) {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slots.1", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc91)
          %true_0 = arith.constant {acir.source_type = "bool"} true
          %false_1 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_1, %true_0, %4 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        } else {
          %4:3 = scf.if %arg3 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
            %5 = acir.rule.call @top.rob.slots.2::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.48.28(%arg0) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slots.2", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc91)
            %true_0 = arith.constant {acir.source_type = "bool"} true
            %false_1 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_1, %true_0, %5 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          } else {
            %5:3 = scf.if %arg4 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
              %6 = acir.rule.call @top.rob.slots.3::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.50.31(%arg0) {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slots.3", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc91)
              %true_0 = arith.constant {acir.source_type = "bool"} true
              %false_1 = arith.constant {acir.source_type = "bool"} false
              scf.yield %false_1, %true_0, %6 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
            } else {
              scf.yield %true, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
            }
            scf.yield %5#0, %5#1, %5#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          }
          scf.yield %4#0, %4#1, %4#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        }
        scf.yield %3#0, %3#1, %3#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %2 = acir.unary %1#0 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %2 {acir.source_type = "void"}
      acir.return %1#2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "retire", instance = "top.rob", interface = "Module:top.rob:retire", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc92)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc93)
      %3 = arith.subi %2, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc94)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc95)
      %4 = arith.andi %3, %c3_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc96)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %5 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc91)
      %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32
      %6 = arith.cmpi eq, %4, %c1_i32_0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc91)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %7 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc91)
      %c3_i32_1 = arith.constant {acir.source_type = "u32"} 3 : i32
      %8 = arith.cmpi eq, %4, %c3_i32_1 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc91)
      %9 = acir.rule.call @top.rob::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19(%2, %5, %6, %7, %8) {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:$dispatch44", role = "rule", rule = "$dispatch44"} : (i32, i1, i1, i1, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc91)
      %c1_i32_2 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc97)
      %10 = arith.addi %2, %c1_i32_2 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc98)
      %11 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %11, %10 {acir.id = "v24", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc99)
      %true_3 = arith.constant {acir.source_type = "bool"} true
      %false_4 = arith.constant {acir.source_type = "bool"} false
      %12 = acir.unary %false_4 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %12 {acir.source_type = "void"}
      acir.return %9 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slots.0 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.0", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}
    acir.cell @top.rob.slots.0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slots.0.state", name = "top.rob.slots.0.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.11.21 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slots.0", definition = "complete", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.11.21", output_type = "void", owner = "top.rob.slots.0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.rob.slots.0::@top.rob.slots.0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc101)
      %3 = scf.if %2 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc102)
        %8 = acir.unary %7 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc103)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc104)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc104)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %5 = acir.ref @top.rob.slots.0::@top.rob.slots.0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %6 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.extract %1 {acir.id = "v21", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc106)
        %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %11 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %12 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %13 = acir.aggregate(%8, %true_0, %10, %11, %12) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
        %14 = acir.extract %13 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %15 = acir.extract %13 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
        %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
        %17 = acir.extract %13 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %18 = acir.extract %13 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %19 = acir.extract %13 {acir.id = "v34", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %20 = acir.aggregate(%15, %16, %17, %18, %arg1) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc107)
        acir.cell.write %5, %20 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc108)
        scf.yield %20 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.20.21 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slots.0", definition = "peek", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:peek", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.20.21", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slots.0", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.0::@top.rob.slots.0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc109)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.28.21.0.call.28.2 : (i32, i32) -> i1 {acir.function = {contract = "Module:top.rob.slots.0", definition = "free", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:free", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.28.21.0.call.28.2", output_type = "bool", owner = "top.rob.slots.0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.0::@top.rob.slots.0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc110)
      %3 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc110)
      %4 = acir.unary %3 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc111)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.28.21 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slots.0", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:allocate", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.28.21", output_type = "void", owner = "top.rob.slots.0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = acir.rule.call @top.rob.slots.0::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.28.21.0.call.28.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", contract = "Module:top.rob.slots.0", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:free", role = "rule", rule = "free"} : (i32, i32) -> i1 loc(#loc80)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc112)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc113)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc114)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc115)
      %0 = acir.aggregate(%true_0, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      %1 = acir.ref @top.rob.slots.0::@top.rob.slots.0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc117)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.44.22.0.call.44.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob.slots.0", definition = "completed", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:completed", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.44.22.0.call.44.1", output_type = "bool", owner = "top.rob.slots.0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "bool", rule = "completed"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.0::@top.rob.slots.0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc118)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
        scf.yield %7 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc121)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc121)
        scf.yield %8 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %6 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %6 {acir.source_type = "void"}
      acir.return %5 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.44.22 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slots.0", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:retire", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.44.22", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slots.0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob.slots.0::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.44.22.0.call.44.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob.slots.0", instance = "top.rob.slots.0", interface = "Module:top.rob.slots.0:completed", role = "rule", rule = "completed"} : (i32) -> i1 loc(#loc91)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc122)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.0::@top.rob.slots.0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc123)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc124)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc124)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %3 = acir.aggregate(%false_0, %false_1, %c0_i32, %c0_i32_2, %c0_i32_3) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc124)
      %4 = acir.ref @top.rob.slots.0::@top.rob.slots.0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %4, %3 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc125)
      %true_4 = arith.constant {acir.source_type = "bool"} true
      %false_5 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_5 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slots.1 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.1", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}
    acir.cell @top.rob.slots.1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slots.1.state", name = "top.rob.slots.1.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.12.23 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slots.1", definition = "complete", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.12.23", output_type = "void", owner = "top.rob.slots.1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.rob.slots.1::@top.rob.slots.1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc101)
      %3 = scf.if %2 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc102)
        %8 = acir.unary %7 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc103)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc104)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc104)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %5 = acir.ref @top.rob.slots.1::@top.rob.slots.1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %6 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.extract %1 {acir.id = "v21", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc106)
        %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %11 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %12 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %13 = acir.aggregate(%8, %true_0, %10, %11, %12) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
        %14 = acir.extract %13 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %15 = acir.extract %13 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
        %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
        %17 = acir.extract %13 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %18 = acir.extract %13 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %19 = acir.extract %13 {acir.id = "v34", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %20 = acir.aggregate(%15, %16, %17, %18, %arg1) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc107)
        acir.cell.write %5, %20 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc108)
        scf.yield %20 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.21.24 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slots.1", definition = "peek", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:peek", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.21.24", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slots.1", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.1::@top.rob.slots.1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc109)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.30.23.0.call.30.2 : (i32, i32) -> i1 {acir.function = {contract = "Module:top.rob.slots.1", definition = "free", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:free", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.30.23.0.call.30.2", output_type = "bool", owner = "top.rob.slots.1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.1::@top.rob.slots.1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc110)
      %3 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc110)
      %4 = acir.unary %3 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc111)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.30.23 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slots.1", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:allocate", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.30.23", output_type = "void", owner = "top.rob.slots.1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = acir.rule.call @top.rob.slots.1::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.30.23.0.call.30.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", contract = "Module:top.rob.slots.1", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:free", role = "rule", rule = "free"} : (i32, i32) -> i1 loc(#loc80)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc112)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc113)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc114)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc115)
      %0 = acir.aggregate(%true_0, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      %1 = acir.ref @top.rob.slots.1::@top.rob.slots.1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc117)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.46.25.0.call.46.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob.slots.1", definition = "completed", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:completed", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.46.25.0.call.46.1", output_type = "bool", owner = "top.rob.slots.1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "bool", rule = "completed"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.1::@top.rob.slots.1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc118)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
        scf.yield %7 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc121)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc121)
        scf.yield %8 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %6 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %6 {acir.source_type = "void"}
      acir.return %5 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.46.25 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slots.1", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:retire", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.46.25", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slots.1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob.slots.1::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.46.25.0.call.46.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob.slots.1", instance = "top.rob.slots.1", interface = "Module:top.rob.slots.1:completed", role = "rule", rule = "completed"} : (i32) -> i1 loc(#loc91)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc122)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.1::@top.rob.slots.1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc123)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc124)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc124)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %3 = acir.aggregate(%false_0, %false_1, %c0_i32, %c0_i32_2, %c0_i32_3) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc124)
      %4 = acir.ref @top.rob.slots.1::@top.rob.slots.1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %4, %3 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc125)
      %true_4 = arith.constant {acir.source_type = "bool"} true
      %false_5 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_5 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slots.2 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.2", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}
    acir.cell @top.rob.slots.2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slots.2.state", name = "top.rob.slots.2.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.13.25 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slots.2", definition = "complete", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.13.25", output_type = "void", owner = "top.rob.slots.2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.rob.slots.2::@top.rob.slots.2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc101)
      %3 = scf.if %2 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc102)
        %8 = acir.unary %7 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc103)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc104)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc104)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %5 = acir.ref @top.rob.slots.2::@top.rob.slots.2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %6 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.extract %1 {acir.id = "v21", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc106)
        %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %11 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %12 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %13 = acir.aggregate(%8, %true_0, %10, %11, %12) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
        %14 = acir.extract %13 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %15 = acir.extract %13 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
        %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
        %17 = acir.extract %13 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %18 = acir.extract %13 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %19 = acir.extract %13 {acir.id = "v34", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %20 = acir.aggregate(%15, %16, %17, %18, %arg1) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc107)
        acir.cell.write %5, %20 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc108)
        scf.yield %20 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.22.27 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slots.2", definition = "peek", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:peek", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.22.27", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slots.2", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.2::@top.rob.slots.2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc109)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.32.25.0.call.32.2 : (i32, i32) -> i1 {acir.function = {contract = "Module:top.rob.slots.2", definition = "free", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:free", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.32.25.0.call.32.2", output_type = "bool", owner = "top.rob.slots.2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.2::@top.rob.slots.2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc110)
      %3 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc110)
      %4 = acir.unary %3 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc111)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.32.25 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slots.2", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:allocate", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.32.25", output_type = "void", owner = "top.rob.slots.2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = acir.rule.call @top.rob.slots.2::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.32.25.0.call.32.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", contract = "Module:top.rob.slots.2", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:free", role = "rule", rule = "free"} : (i32, i32) -> i1 loc(#loc80)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc112)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc113)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc114)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc115)
      %0 = acir.aggregate(%true_0, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      %1 = acir.ref @top.rob.slots.2::@top.rob.slots.2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc117)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.48.28.0.call.48.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob.slots.2", definition = "completed", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:completed", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.48.28.0.call.48.1", output_type = "bool", owner = "top.rob.slots.2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "bool", rule = "completed"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.2::@top.rob.slots.2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc118)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
        scf.yield %7 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc121)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc121)
        scf.yield %8 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %6 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %6 {acir.source_type = "void"}
      acir.return %5 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.48.28 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slots.2", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:retire", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.48.28", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slots.2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob.slots.2::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.48.28.0.call.48.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob.slots.2", instance = "top.rob.slots.2", interface = "Module:top.rob.slots.2:completed", role = "rule", rule = "completed"} : (i32) -> i1 loc(#loc91)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc122)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.2::@top.rob.slots.2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc123)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc124)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc124)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %3 = acir.aggregate(%false_0, %false_1, %c0_i32, %c0_i32_2, %c0_i32_3) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc124)
      %4 = acir.ref @top.rob.slots.2::@top.rob.slots.2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %4, %3 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc125)
      %true_4 = arith.constant {acir.source_type = "bool"} true
      %false_5 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_5 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slots.3 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slots.3", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}
    acir.cell @top.rob.slots.3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slots.3.state", name = "top.rob.slots.3.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.14.27 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slots.3", definition = "complete", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.17.0.call.14.27", output_type = "void", owner = "top.rob.slots.3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.rob.slots.3::@top.rob.slots.3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc101)
      %3 = scf.if %2 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc102)
        %8 = acir.unary %7 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc103)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc104)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc104)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %5 = acir.ref @top.rob.slots.3::@top.rob.slots.3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %6 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.extract %1 {acir.id = "v21", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc106)
        %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc105)
        %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %11 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %12 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc105)
        %13 = acir.aggregate(%8, %true_0, %10, %11, %12) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
        %14 = acir.extract %13 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %15 = acir.extract %13 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
        %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
        %17 = acir.extract %13 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %18 = acir.extract %13 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %19 = acir.extract %13 {acir.id = "v34", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc107)
        %20 = acir.aggregate(%15, %16, %17, %18, %arg1) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc107)
        acir.cell.write %5, %20 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc108)
        scf.yield %20 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.23.30 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slots.3", definition = "peek", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:peek", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43.0.call.20.30.0.call.23.30", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slots.3", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.3::@top.rob.slots.3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc109)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.34.27.0.call.34.2 : (i32, i32) -> i1 {acir.function = {contract = "Module:top.rob.slots.3", definition = "free", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:free", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.34.27.0.call.34.2", output_type = "bool", owner = "top.rob.slots.3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.3::@top.rob.slots.3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc110)
      %3 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc110)
      %4 = acir.unary %3 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc111)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.34.27 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slots.3", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:allocate", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.34.27", output_type = "void", owner = "top.rob.slots.3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = acir.rule.call @top.rob.slots.3::@top.dispatch_body.0.call.20.12.0.call.27.10.0.call.28.21.0.call.34.27.0.call.34.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", contract = "Module:top.rob.slots.3", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:free", role = "rule", rule = "free"} : (i32, i32) -> i1 loc(#loc80)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc112)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc113)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc114)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc115)
      %0 = acir.aggregate(%true_0, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      %1 = acir.ref @top.rob.slots.3::@top.rob.slots.3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc117)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.50.31.0.call.50.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob.slots.3", definition = "completed", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:completed", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.50.31.0.call.50.1", output_type = "bool", owner = "top.rob.slots.3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "bool", rule = "completed"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.3::@top.rob.slots.3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc118)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc119)
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
        scf.yield %7 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc121)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc121)
        scf.yield %8 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %6 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %6 {acir.source_type = "void"}
      acir.return %5 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.50.31 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slots.3", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:retire", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.50.31", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slots.3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob.slots.3::@top.commit_body.0.call.44.1.0.call.44.3.0.call.44.19.0.call.50.31.0.call.50.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob.slots.3", instance = "top.rob.slots.3", interface = "Module:top.rob.slots.3:completed", role = "rule", rule = "completed"} : (i32) -> i1 loc(#loc91)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc122)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slots.3::@top.rob.slots.3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc123)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc124)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc124)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %3 = acir.aggregate(%false_0, %false_1, %c0_i32, %c0_i32_2, %c0_i32_3) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc124)
      %4 = acir.ref @top.rob.slots.3::@top.rob.slots.3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %4, %3 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc125)
      %true_4 = arith.constant {acir.source_type = "bool"} true
      %false_5 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_5 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rf attributes {acir.instance = {definition = "src_state_7e811fd9.RegisterFile", name = "top.rf", parent = "top", rules = ["commit", "operand", "rename"]}} {
    acir.contract {relations = [], rules = ["commit", "operand", "rename"]}
    acir.cell @top.rf.registers : !acir.array<i32, [8]> {initial_fn = "ac_init_top.rf.registers", name = "top.rf.registers", type = "array<u32;8>"}
    acir.cell @top.rf.renamed : !acir.array<i32, [8]> {initial_fn = "ac_init_top.rf.renamed", name = "top.rf.renamed", type = "array<u32;8>"}
    acir.cell @top.rf.committed : !acir.array<i32, [8]> {initial_fn = "ac_init_top.rf.committed", name = "top.rf.committed", type = "array<u32;8>"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.dispatch_body.0.call.20.12.0.call.20.6 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:top.rf", definition = "operand", instance = "top.rf", interface = "Module:top.rf:operand", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.20.6", output_type = "src_payloads_25ebcaa7.Operand", owner = "top.rf", params = [{id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Operand", rule = "operand"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %false = arith.constant {acir.source_type = "bool"} false
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc126)
      %1 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc127)
      acir.check %1 {acir.id = "v9", acir.source_type = "void"} loc(#loc128)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc129)
      %2 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc130)
      %3:3 = scf.if %2 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc131)
        %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc132)
        %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc133)
        %9 = acir.aggregate(%true_0, %c0_i32_1, %c0_i32_2) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc134)
        %true_3 = arith.constant {acir.source_type = "bool"} true
        %false_4 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_4, %true_3, %9 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
      } else {
        scf.yield %true, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
      }
      %4 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %5 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %6 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %7:3 = scf.if %3#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
        %9 = acir.cell.read %4 {acir.id = "v25", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc135)
        %10 = "acir.array.index"(%9, %arg0) {acir.id = "v26", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [8]>, i32) -> i32 loc(#loc135)
        %11 = acir.cell.read %5 {acir.id = "v28", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc136)
        %12 = "acir.array.index"(%11, %arg0) {acir.id = "v29", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [8]>, i32) -> i32 loc(#loc136)
        %13 = arith.cmpi eq, %10, %12 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc137)
        %14:3 = scf.if %13 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc138)
          %16 = acir.cell.read %6 {acir.id = "v33", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc139)
          %17 = "acir.array.index"(%16, %arg0) {acir.id = "v34", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [8]>, i32) -> i32 loc(#loc139)
          %18 = acir.aggregate(%true_0, %10, %17) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc140)
          %true_1 = arith.constant {acir.source_type = "bool"} true
          %false_2 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_2, %true_1, %18 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
        } else {
          scf.yield %3#0, %3#1, %3#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
        }
        %15:3 = scf.if %14#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
          %16 = acir.rule.call @top.rob::@top.dispatch_body.0.call.20.12.0.call.20.6.0.call.20.43(%10) {acir.id = "v44", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:lookup", role = "rule", rule = "lookup"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc141)
          %17 = acir.extract %16 {acir.id = "v45", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc142)
          %18 = scf.if %17 -> (i1) {
            %22 = acir.extract %16 {acir.id = "v46", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc143)
            scf.yield %22 : i1
          } else {
            %false_0 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_0 : i1
          }
          %19 = scf.if %18 -> (i1) {
            %22 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc144)
            %23 = arith.cmpi eq, %22, %10 {acir.id = "v52", acir.source_type = "bool"} : i32 loc(#loc144)
            scf.yield %23 : i1
          } else {
            %false_0 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_0 : i1
          }
          %20:3 = scf.if %19 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
            %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc145)
            %22 = acir.extract %16 {acir.id = "v58", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc146)
            %23 = acir.aggregate(%true_0, %10, %22) {acir.id = "v59", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc147)
            %true_1 = arith.constant {acir.source_type = "bool"} true
            %false_2 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_2, %true_1, %23 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
          } else {
            scf.yield %14#0, %14#1, %14#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
          }
          %21:3 = scf.if %20#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
            %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc148)
            %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc149)
            %22 = acir.aggregate(%false_0, %10, %c0_i32_1) {acir.id = "v70", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc150)
            %true_2 = arith.constant {acir.source_type = "bool"} true
            %false_3 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_3, %true_2, %22 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
          } else {
            scf.yield %20#0, %20#1, %20#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
          }
          scf.yield %21#0, %21#1, %21#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
        } else {
          scf.yield %14#0, %14#1, %14#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
        }
        scf.yield %15#0, %15#1, %15#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
      } else {
        scf.yield %3#0, %3#1, %3#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
      }
      %8 = acir.unary %7#0 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %8 {acir.source_type = "void"}
      acir.return %7#2 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.38.12 : (i32, i32) -> () {acir.function = {contract = "Module:top.rf", definition = "rename", instance = "top.rf", interface = "Module:top.rf:rename", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.38.12", output_type = "void", owner = "top.rf", params = [{id = "v1", name = "rd", targets = [], type = "u32"}, {id = "v2", name = "tag", targets = [], type = "u32"}], public = true, result = "void", rule = "rename"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc151)
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc152)
      acir.check %0 {acir.id = "v8", acir.source_type = "void"} loc(#loc153)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc154)
      %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc155)
      %2 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %3 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %4 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      scf.if %1 {
        %5 = acir.cell.read %4 {acir.id = "v14", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc156)
        %6 = acir.update %5, %arg1[%arg0] {acir.id = "v15", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i32) -> !acir.array<i32, [8]> loc(#loc156)
        acir.cell.write %4, %6 {acir.id = "v16", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [8]>>, !acir.array<i32, [8]> loc(#loc156)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.44.1.0.call.54.4 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () {acir.function = {contract = "Module:top.rf", definition = "commit", instance = "top.rf", interface = "Module:top.rf:commit", kind = "rule", name = "top.commit_body.0.call.44.1.0.call.54.4", output_type = "void", owner = "top.rf", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.extract %arg0 {acir.id = "v5", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc157)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc158)
      %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc157)
      %2 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %3 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %4 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %5 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %6 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %7 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      scf.if %1 {
        %8 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc159)
        %9 = acir.cell.read %4 {acir.id = "v12", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc160)
        %10 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc161)
        %11 = acir.update %9, %8[%10] {acir.id = "v14", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i32) -> !acir.array<i32, [8]> loc(#loc160)
        acir.cell.write %4, %11 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [8]>>, !acir.array<i32, [8]> loc(#loc160)
        %12 = acir.extract %arg0 {acir.id = "v17", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
        %13 = acir.cell.read %7 {acir.id = "v20", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc163)
        %14 = acir.extract %arg0 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc164)
        %15 = acir.update %13, %12[%14] {acir.id = "v22", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i32) -> !acir.array<i32, [8]> loc(#loc163)
        acir.cell.write %7, %15 {acir.id = "v23", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [8]>>, !acir.array<i32, [8]> loc(#loc163)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.left attributes {acir.instance = {definition = "src_state_7e811fd9.ReservationStation", name = "top.left", parent = "top", rules = ["free", "issuable", "peek", "place", "take", "wakeup"]}} {
    acir.contract {relations = [], rules = ["free", "issuable", "peek", "place", "take", "wakeup"]}
    acir.cell @top.left.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.left.slot", name = "top.left.slot", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.scheduler_body.0.call.0.1 : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.left", definition = "peek", instance = "top.left", interface = "Module:top.left:peek", kind = "rule", name = "top.scheduler_body.0.call.0.1", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.left", params = [], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Entry"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc165)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top.scheduler_body.0.call.2.81.0.call.2.7.0.call.2.1 : (i32) -> i1 {acir.function = {contract = "Module:top.left", definition = "issuable", instance = "top.left", interface = "Module:top.left:issuable", kind = "rule", name = "top.scheduler_body.0.call.2.81.0.call.2.7.0.call.2.1", output_type = "bool", owner = "top.left", params = [{id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "bool", rule = "issuable"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc166)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc167)
      %4 = scf.if %3 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v10", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc168)
        %9 = arith.cmpi eq, %8, %arg0 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc168)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc169)
        %9 = acir.extract %8 {acir.id = "v17", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc169)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %6 = scf.if %5 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc170)
        %9 = acir.extract %8 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc170)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %6 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.scheduler_body.0.call.2.81.0.call.2.7 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.left", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.left", interface = "Module:top.left:take", kind = "rule", name = "top.scheduler_body.0.call.2.81.0.call.2.7", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.left", params = [{id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.2.81.0.call.2.7.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:issuable", role = "rule", rule = "issuable"} : (i32) -> i1 loc(#loc171)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc172)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Entry"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc173)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc174)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc174)
      %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc174)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc174)
      %3 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc174)
      %4 = acir.aggregate() {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc174)
      %5 = acir.aggregate(%false_0, %c0_i32, %c0_i32_1, %c0_i32_2, %3, %4) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc174)
      %6 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %6, %5 {acir.id = "v18", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc175)
      %true_3 = arith.constant {acir.source_type = "bool"} true
      %false_4 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_4 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top.cdb_body.0.call.11.7.0.call.17.19 : (i32, i32) -> () {acir.function = {contract = "Module:top.left", definition = "wakeup", instance = "top.left", interface = "Module:top.left:wakeup", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.17.19", output_type = "void", owner = "top.left", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "wakeup"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc176)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc177)
      %2 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc178)
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %5 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc179)
        %6 = acir.extract %5 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc179)
        %7 = acir.unary %6 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc180)
        %8 = scf.if %7 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc181)
          %16 = acir.extract %15 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc181)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc181)
          scf.yield %17 : i1
        } else {
          %false_0 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_0 : i1
        }
        %9:2 = scf.if %8 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc182)
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc183)
          %16 = acir.aggregate(%true_0, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc184)
          %17 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc182)
          %18 = acir.extract %1 {acir.id = "v24", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc182)
          %19 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc182)
          %20 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc182)
          %21 = acir.extract %1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc182)
          %22 = acir.extract %1 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc182)
          %23 = acir.aggregate(%17, %18, %19, %20, %16, %22) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc182)
          %true_1 = arith.constant {acir.source_type = "bool"} true loc(#loc185)
          scf.yield %true_1, %23 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %10 = acir.extract %9#1 {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc186)
        %11 = acir.extract %10 {acir.id = "v37", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc186)
        %12 = acir.unary %11 "not" {acir.id = "v38", acir.source_type = "bool"} : i1 -> i1 loc(#loc187)
        %13 = scf.if %12 -> (i1) {
          %15 = acir.extract %9#1 {acir.id = "v39", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc188)
          %16 = acir.extract %15 {acir.id = "v40", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc188)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc188)
          scf.yield %17 : i1
        } else {
          %false_0 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_0 : i1
        }
        %14:2 = scf.if %13 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.extract %9#1 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc189)
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc190)
          %16 = acir.aggregate(%true_0, %arg0, %arg1) {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc191)
          %17 = acir.extract %9#1 {acir.id = "v49", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc189)
          %18 = acir.extract %9#1 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc189)
          %19 = acir.extract %9#1 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc189)
          %20 = acir.extract %9#1 {acir.id = "v52", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc189)
          %21 = acir.extract %9#1 {acir.id = "v53", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc189)
          %22 = acir.extract %9#1 {acir.id = "v54", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc189)
          %23 = acir.aggregate(%17, %18, %19, %20, %21, %16) {acir.id = "v55", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc189)
          %true_1 = arith.constant {acir.source_type = "bool"} true loc(#loc192)
          scf.yield %true_1, %23 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %9#0, %9#1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %14#0, %14#1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      %4 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      scf.if %3#0 {
        acir.cell.write %4, %3#1 {acir.id = "v68", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc193)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.39.20.0.call.39.1 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 {acir.function = {contract = "Module:top.left", definition = "free", instance = "top.left", interface = "Module:top.left:free", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.39.20.0.call.39.1", output_type = "bool", owner = "top.left", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc194)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc194)
      %4 = acir.unary %3 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc195)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.39.20 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:top.left", definition = "place", guard_info = {name = "free", source = "function"}, instance = "top.left", interface = "Module:top.left:place", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.39.20", output_type = "void", owner = "top.left", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "void", rule = "place"}} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.rule.call @top.left::@top.dispatch_body.0.call.20.12.0.call.39.20.0.call.39.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:free", role = "rule", rule = "free"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc196)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc197)
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc198)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.right attributes {acir.instance = {definition = "src_state_7e811fd9.ReservationStation", name = "top.right", parent = "top", rules = ["free", "issuable", "peek", "place", "take", "wakeup"]}} {
    acir.contract {relations = [], rules = ["free", "issuable", "peek", "place", "take", "wakeup"]}
    acir.cell @top.right.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.right.slot", name = "top.right.slot", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.scheduler_body.0.call.1.2 : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.right", definition = "peek", instance = "top.right", interface = "Module:top.right:peek", kind = "rule", name = "top.scheduler_body.0.call.1.2", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.right", params = [], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Entry"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc165)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top.scheduler_body.0.call.2.81.0.call.4.17.0.call.4.1 : (i32) -> i1 {acir.function = {contract = "Module:top.right", definition = "issuable", instance = "top.right", interface = "Module:top.right:issuable", kind = "rule", name = "top.scheduler_body.0.call.2.81.0.call.4.17.0.call.4.1", output_type = "bool", owner = "top.right", params = [{id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "bool", rule = "issuable"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc166)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc167)
      %4 = scf.if %3 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v10", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc168)
        %9 = arith.cmpi eq, %8, %arg0 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc168)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc169)
        %9 = acir.extract %8 {acir.id = "v17", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc169)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %6 = scf.if %5 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc170)
        %9 = acir.extract %8 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc170)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %6 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.scheduler_body.0.call.2.81.0.call.4.17 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.right", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.right", interface = "Module:top.right:take", kind = "rule", name = "top.scheduler_body.0.call.2.81.0.call.4.17", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.right", params = [{id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.right::@top.scheduler_body.0.call.2.81.0.call.4.17.0.call.4.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:issuable", role = "rule", rule = "issuable"} : (i32) -> i1 loc(#loc199)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc172)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Entry"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc173)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc174)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc174)
      %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc174)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc174)
      %3 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc174)
      %4 = acir.aggregate() {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc174)
      %5 = acir.aggregate(%false_0, %c0_i32, %c0_i32_1, %c0_i32_2, %3, %4) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc174)
      %6 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %6, %5 {acir.id = "v18", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc175)
      %true_3 = arith.constant {acir.source_type = "bool"} true
      %false_4 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_4 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top.cdb_body.0.call.11.7.0.call.18.22 : (i32, i32) -> () {acir.function = {contract = "Module:top.right", definition = "wakeup", instance = "top.right", interface = "Module:top.right:wakeup", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.18.22", output_type = "void", owner = "top.right", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "wakeup"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc176)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc177)
      %2 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc178)
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %5 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc179)
        %6 = acir.extract %5 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc179)
        %7 = acir.unary %6 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc180)
        %8 = scf.if %7 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc181)
          %16 = acir.extract %15 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc181)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc181)
          scf.yield %17 : i1
        } else {
          %false_0 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_0 : i1
        }
        %9:2 = scf.if %8 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc182)
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc183)
          %16 = acir.aggregate(%true_0, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc184)
          %17 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc182)
          %18 = acir.extract %1 {acir.id = "v24", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc182)
          %19 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc182)
          %20 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc182)
          %21 = acir.extract %1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc182)
          %22 = acir.extract %1 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc182)
          %23 = acir.aggregate(%17, %18, %19, %20, %16, %22) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc182)
          %true_1 = arith.constant {acir.source_type = "bool"} true loc(#loc185)
          scf.yield %true_1, %23 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %10 = acir.extract %9#1 {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc186)
        %11 = acir.extract %10 {acir.id = "v37", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc186)
        %12 = acir.unary %11 "not" {acir.id = "v38", acir.source_type = "bool"} : i1 -> i1 loc(#loc187)
        %13 = scf.if %12 -> (i1) {
          %15 = acir.extract %9#1 {acir.id = "v39", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc188)
          %16 = acir.extract %15 {acir.id = "v40", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc188)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc188)
          scf.yield %17 : i1
        } else {
          %false_0 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_0 : i1
        }
        %14:2 = scf.if %13 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.extract %9#1 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc189)
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc190)
          %16 = acir.aggregate(%true_0, %arg0, %arg1) {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc191)
          %17 = acir.extract %9#1 {acir.id = "v49", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc189)
          %18 = acir.extract %9#1 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc189)
          %19 = acir.extract %9#1 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc189)
          %20 = acir.extract %9#1 {acir.id = "v52", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc189)
          %21 = acir.extract %9#1 {acir.id = "v53", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc189)
          %22 = acir.extract %9#1 {acir.id = "v54", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc189)
          %23 = acir.aggregate(%17, %18, %19, %20, %21, %16) {acir.id = "v55", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc189)
          %true_1 = arith.constant {acir.source_type = "bool"} true loc(#loc192)
          scf.yield %true_1, %23 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %9#0, %9#1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %14#0, %14#1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      %4 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      scf.if %3#0 {
        acir.cell.write %4, %3#1 {acir.id = "v68", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc193)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.41.21.0.call.41.1 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 {acir.function = {contract = "Module:top.right", definition = "free", instance = "top.right", interface = "Module:top.right:free", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.41.21.0.call.41.1", output_type = "bool", owner = "top.right", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc194)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc194)
      %4 = acir.unary %3 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc195)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.20.12.0.call.41.21 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:top.right", definition = "place", guard_info = {name = "free", source = "function"}, instance = "top.right", interface = "Module:top.right:place", kind = "rule", name = "top.dispatch_body.0.call.20.12.0.call.41.21", output_type = "void", owner = "top.right", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "void", rule = "place"}} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.rule.call @top.right::@top.dispatch_body.0.call.20.12.0.call.41.21.0.call.41.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:free", role = "rule", rule = "free"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc200)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc197)
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc198)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.scheduler attributes {acir.instance = {definition = "src_execution_885f3671.Scheduler", name = "top.scheduler", parent = "top", rules = ["issue_add", "issue_mul"]}} {
    acir.contract {relations = [], rules = ["issue_add", "issue_mul"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.scheduler_body.0.call.2.81 : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.scheduler", definition = "issue_add", instance = "top.scheduler", interface = "Module:top.scheduler:issue_add", kind = "rule", name = "top.scheduler_body.0.call.2.81", output_type = "queue<src_payloads_25ebcaa7.Entry>", owner = "top.scheduler", params = [{id = "v1", name = "choose_left", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "issue_add"}} body {
    ^bb0(%arg0: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Entry"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1:3 = scf.if %arg0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc201)
        %4 = acir.rule.call @top.left::@top.scheduler_body.0.call.2.81.0.call.2.7(%c0_i32) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %4 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %true, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      %2:3 = scf.if %1#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc202)
        %4 = acir.rule.call @top.right::@top.scheduler_body.0.call.2.81.0.call.4.17(%c0_i32) {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc199)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %4 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %1#0, %1#1, %1#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      %3 = "acir.optional"(%2#2, %2#1) {acir.source_type = "src_payloads_25ebcaa7.Entry"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top.scheduler_body.0.call.7.82 : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.scheduler", definition = "issue_mul", instance = "top.scheduler", interface = "Module:top.scheduler:issue_mul", kind = "rule", name = "top.scheduler_body.0.call.7.82", output_type = "queue<src_payloads_25ebcaa7.Entry>", owner = "top.scheduler", params = [{id = "v1", name = "choose_left", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "issue_mul"}} body {
    ^bb0(%arg0: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Entry"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1:3 = scf.if %arg0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc203)
        %4 = acir.rule.call @top.left::@top.scheduler_body.0.call.2.81.0.call.2.7(%c1_i32) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc204)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %4 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %true, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      %2:3 = scf.if %1#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc205)
        %4 = acir.rule.call @top.right::@top.scheduler_body.0.call.2.81.0.call.4.17(%c1_i32) {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc206)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %4 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %1#0, %1#1, %1#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      %3 = "acir.optional"(%2#2, %2#1) {acir.source_type = "src_payloads_25ebcaa7.Entry"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.scheduler_body", owner = "top.scheduler", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.0.1() {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc207)
      %1 = acir.rule.call @top.right::@top.scheduler_body.0.call.1.2() {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc208)
      %2 = acir.extract %0 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc209)
      %3 = scf.if %2 -> (i1) {
        %18 = acir.extract %0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc210)
        %19 = acir.extract %18 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc210)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %4 = scf.if %3 -> (i1) {
        %18 = acir.extract %0 {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc211)
        %19 = acir.extract %18 {acir.id = "v12", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc211)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %5 = acir.extract %1 {acir.id = "v17", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc212)
      %6 = scf.if %5 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc213)
        %19 = acir.extract %18 {acir.id = "v19", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc213)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %7 = scf.if %6 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc214)
        %19 = acir.extract %18 {acir.id = "v25", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc214)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %8 = scf.if %4 -> (i1) {
        %18 = acir.extract %0 {acir.id = "v30", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc215)
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc216)
        %19 = arith.cmpi eq, %18, %c0_i32 {acir.id = "v32", acir.source_type = "bool"} : i32 loc(#loc215)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %9 = scf.if %7 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v37", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc217)
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc218)
        %19 = arith.cmpi eq, %18, %c0_i32 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc217)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %10 = scf.if %4 -> (i1) {
        %18 = acir.extract %0 {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc219)
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc220)
        %19 = arith.cmpi eq, %18, %c1_i32 {acir.id = "v46", acir.source_type = "bool"} : i32 loc(#loc219)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %11 = scf.if %7 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v51", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc221)
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc222)
        %19 = arith.cmpi eq, %18, %c1_i32 {acir.id = "v53", acir.source_type = "bool"} : i32 loc(#loc221)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %12 = scf.if %8 -> (i1) {
        %18 = acir.unary %9 "not" {acir.id = "v58", acir.source_type = "bool"} : i1 -> i1 loc(#loc223)
        %19 = scf.if %18 -> (i1) {
          %true_0 = arith.constant {acir.source_type = "bool"} true
          scf.yield %true_0 : i1
        } else {
          %20 = acir.extract %0 {acir.id = "v60", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc224)
          %21 = acir.extract %1 {acir.id = "v61", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc225)
          %22 = arith.cmpi ult, %20, %21 {acir.id = "v62", acir.source_type = "bool"} : i32 loc(#loc224)
          scf.yield %22 : i1
        }
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %13 = scf.if %10 -> (i1) {
        %18 = acir.unary %11 "not" {acir.id = "v70", acir.source_type = "bool"} : i1 -> i1 loc(#loc226)
        %19 = scf.if %18 -> (i1) {
          %true_0 = arith.constant {acir.source_type = "bool"} true
          scf.yield %true_0 : i1
        } else {
          %20 = acir.extract %0 {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc227)
          %21 = acir.extract %1 {acir.id = "v73", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc228)
          %22 = arith.cmpi ult, %20, %21 {acir.id = "v74", acir.source_type = "bool"} : i32 loc(#loc227)
          scf.yield %22 : i1
        }
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %14 = acir.rule.call @top.scheduler::@top.scheduler_body.0.call.2.81(%12) {acir.id = "v82", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.scheduler", instance = "top.scheduler", interface = "Module:top.scheduler:issue_add", output_binding = @top::@top.add_requests, role = "rule", rule = "issue_add"} : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc229)
      %15 = acir.rule.call @top.scheduler::@top.scheduler_body.0.call.7.82(%13) {acir.id = "v83", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.scheduler", instance = "top.scheduler", interface = "Module:top.scheduler:issue_mul", output_binding = @top::@top.mul_requests, role = "rule", rule = "issue_mul"} : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc230)
      %16 = acir.ref @top::@top.add_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %17 = acir.ref @top::@top.mul_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %false = arith.constant {acir.source_type = "bool"} false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.adder attributes {acir.instance = {definition = "src_execution_885f3671.Adder", name = "top.adder", parent = "top", rules = ["execute"]}} {
    acir.contract {relations = [], rules = ["execute"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.adder_body.0.call.8.2 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.adder", definition = "execute", instance = "top.adder", interface = "Module:top.adder:execute", kind = "rule", name = "top.adder_body.0.call.8.2", output_type = "queue<src_payloads_25ebcaa7.Completion>", owner = "top.adder", params = [{id = "v1", name = "message", targets = ["top.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "execute"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc231)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc232)
      %3 = acir.extract %1 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc233)
      %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc234)
      %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc234)
      %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc235)
      %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc235)
      %8 = arith.addi %5, %7 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc234)
      %9 = acir.aggregate(%2, %3, %8) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc236)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %10 = "acir.optional"(%9, %true_0) {acir.source_type = "src_payloads_25ebcaa7.Completion"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      acir.return %10 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.adder_body", owner = "top.adder", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.add_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.rule.call @top.adder::@top.adder_body.0.call.8.2(%0) {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.adder", instance = "top.adder", interface = "Module:top.adder:execute", output_binding = @top::@top.add_done, role = "rule", rule = "execute"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc237)
      %2 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %false = arith.constant {acir.source_type = "bool"} false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.multiplier attributes {acir.instance = {definition = "src_execution_885f3671.Multiplier", name = "top.multiplier", parent = "top", rules = ["accept", "advance"]}} {
    acir.contract {relations = [], rules = ["accept", "advance"]}
    acir.cell @top.multiplier.active : i1 {initial_fn = "ac_init_top.multiplier.active", name = "top.multiplier.active", type = "bool"}
    acir.cell @top.multiplier.remaining : i32 {initial_fn = "ac_init_top.multiplier.remaining", name = "top.multiplier.remaining", type = "u32"}
    acir.cell @top.multiplier.request : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.multiplier.request", name = "top.multiplier.request", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.multiplier_body.0.call.9.3 : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.multiplier", definition = "advance", instance = "top.multiplier", interface = "Module:top.multiplier:advance", kind = "rule", name = "top.multiplier_body.0.call.9.3", output_type = "queue<src_payloads_25ebcaa7.Completion>", owner = "top.multiplier", params = [], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "advance"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc238)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc239)
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc238)
      %4 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %5 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %6 = scf.if %3 -> (i1) {
        %11 = acir.cell.read %4 {acir.id = "v11", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc240)
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc241)
        %12 = arith.subi %11, %c1_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc240)
        acir.cell.write %5, %12 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc242)
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      } else {
        scf.yield %true : i1
      }
      %7 = acir.ref @top.multiplier::@top.multiplier.request {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %8 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      %9:3 = scf.if %6 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) {
        %11 = acir.cell.read %7 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc243)
        %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc244)
        acir.cell.write %8, %false_0 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc245)
        %12 = acir.extract %11 {acir.id = "v26", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc246)
        %13 = acir.extract %11 {acir.id = "v27", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc247)
        %14 = acir.extract %11 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc248)
        %15 = acir.extract %14 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc248)
        %16 = acir.extract %11 {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc249)
        %17 = acir.extract %16 {acir.id = "v31", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc249)
        %18 = arith.muli %15, %17 {acir.id = "v32", acir.source_type = "u32"} : i32 loc(#loc248)
        %19 = acir.aggregate(%12, %13, %18) {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc250)
        %true_1 = arith.constant {acir.source_type = "bool"} true
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2, %true_1, %19 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Completion">
      } else {
        scf.yield %6, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Completion">
      }
      %10 = "acir.optional"(%9#2, %9#1) {acir.source_type = "src_payloads_25ebcaa7.Completion"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      acir.return %10 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.source_type = "void"}
    }
    acir.rule @top.multiplier_body.0.call.10.7 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () {acir.function = {contract = "Module:top.multiplier", definition = "accept", instance = "top.multiplier", interface = "Module:top.multiplier:accept", kind = "rule", name = "top.multiplier_body.0.call.10.7", output_type = "void", owner = "top.multiplier", params = [{id = "v1", name = "message", targets = ["top.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], public = true, result = "void", rule = "accept"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc251)
      %1 = acir.ref @top.multiplier::@top.multiplier.request {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %1, %0 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc252)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc253)
      %2 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %c2_i32 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc254)
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc255)
      %3 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      acir.cell.write %3, %true_0 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc256)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.multiplier_body", owner = "top.multiplier", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "bool", role = "observe"} : !acir.cell<i1> -> i1 loc(#loc257)
      %2 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %3 = acir.ref @top::@top.mul_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %4 = scf.if %1 -> (i1) {
        %5 = acir.rule.call @top.multiplier::@top.multiplier_body.0.call.9.3() {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.multiplier", instance = "top.multiplier", interface = "Module:top.multiplier:advance", output_binding = @top::@top.mul_done, role = "rule", rule = "advance"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc258)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        acir.rule.call @top.multiplier::@top.multiplier_body.0.call.10.7(%3) {acir.id = "v8", acir.source_type = "void", contract = "Module:top.multiplier", instance = "top.multiplier", interface = "Module:top.multiplier:accept", role = "rule", rule = "accept"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () loc(#loc259)
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cdb attributes {acir.instance = {definition = "src_model_3cf90884.Broadcast", name = "top.cdb", parent = "top", rules = ["broadcast"]}} {
    acir.contract {relations = [], rules = ["broadcast"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.cdb_body.0.call.11.7 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.cdb", definition = "broadcast", instance = "top.cdb", interface = "Module:top.cdb:broadcast", kind = "rule", name = "top.cdb_body.0.call.11.7", output_type = "signal<src_payloads_25ebcaa7.Completion>", owner = "top.cdb", params = [{id = "v1", name = "a", targets = ["top.add_done"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {id = "v2", name = "b", targets = ["top.mul_done"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {id = "v3", name = "choose_add", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "broadcast"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg1: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg2: i1):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = scf.if %arg2 -> (!acir.struct<"src_payloads_25ebcaa7.Completion">) {
        %9 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc260)
        scf.yield %9 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      } else {
        %9 = acir.queue.peek %arg1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc261)
        scf.yield %9 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      }
      %2 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc262)
      %3 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc263)
      acir.rule.call @top.rob::@top.cdb_body.0.call.11.7.0.call.11.16(%2, %3) {acir.id = "v17", acir.source_type = "void", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc264)
      %4 = acir.extract %1 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc265)
      %5 = acir.extract %1 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc266)
      acir.rule.call @top.left::@top.cdb_body.0.call.11.7.0.call.17.19(%4, %5) {acir.id = "v20", acir.source_type = "void", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:wakeup", role = "rule", rule = "wakeup"} : (i32, i32) -> () loc(#loc267)
      %6 = acir.extract %1 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc268)
      %7 = acir.extract %1 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc269)
      acir.rule.call @top.right::@top.cdb_body.0.call.11.7.0.call.18.22(%6, %7) {acir.id = "v23", acir.source_type = "void", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:wakeup", role = "rule", rule = "wakeup"} : (i32, i32) -> () loc(#loc270)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %8 = "acir.optional"(%1, %true_0) {acir.source_type = "src_payloads_25ebcaa7.Completion"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cdb_body", owner = "top.cdb", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %2 = acir.queue.empty %1 {acir.id = "v4", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc271)
      %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc272)
      %4 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %5 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %6 = acir.rule.call @top.cdb::@top.cdb_body.0.call.11.7(%4, %5, %3) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.cdb", instance = "top.cdb", interface = "Module:top.cdb:broadcast", output_binding = @top::@top.event, role = "rule", rule = "broadcast"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc273)
      %7 = acir.ref @top::@top.event {acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %false = arith.constant {acir.source_type = "bool"} false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.dispatch attributes {acir.instance = {definition = "src_model_3cf90884.Dispatch", name = "top.dispatch", parent = "top", rules = ["dispatch"]}} {
    acir.contract {relations = [], rules = ["dispatch"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.dispatch_body.0.call.20.12 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () {acir.function = {contract = "Module:top.dispatch", definition = "dispatch", instance = "top.dispatch", interface = "Module:top.dispatch:dispatch", kind = "rule", name = "top.dispatch_body.0.call.20.12", output_type = "void", owner = "top.dispatch", params = [{id = "v1", name = "message", targets = ["top.source"], type = "queue<src_payloads_25ebcaa7.MicroOp>"}], public = true, result = "void", rule = "dispatch"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.MicroOp", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc274)
      %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc275)
      %2 = acir.rule.call @top.rf::@top.dispatch_body.0.call.20.12.0.call.20.6(%1) {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:operand", role = "rule", rule = "operand"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc276)
      %3 = acir.extract %0 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc277)
      %4 = acir.rule.call @top.rf::@top.dispatch_body.0.call.20.12.0.call.20.6(%3) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:operand", role = "rule", rule = "operand"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc278)
      %5 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc279)
      %6 = acir.rule.call @top.rob::@top.dispatch_body.0.call.20.12.0.call.27.10(%5) {acir.id = "v11", acir.source_type = "u32", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:allocate", role = "rule", rule = "allocate"} : (i32) -> i32 loc(#loc81)
      %7 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc280)
      acir.rule.call @top.rf::@top.dispatch_body.0.call.20.12.0.call.38.12(%7, %6) {acir.id = "v13", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:rename", role = "rule", rule = "rename"} : (i32, i32) -> () loc(#loc281)
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc282)
      %8 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc283)
      %9 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc284)
      %10 = acir.aggregate(%true_0, %8, %6, %9, %2, %4) {acir.id = "v17", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc285)
      %11 = acir.rule.call @top.left::@top.scheduler_body.0.call.0.1() {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc286)
      %12 = acir.extract %11 {acir.id = "v19", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc286)
      %13 = acir.unary %12 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc287)
      scf.if %13 {
        acir.rule.call @top.left::@top.dispatch_body.0.call.20.12.0.call.39.20(%10) {acir.id = "v21", acir.source_type = "void", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:place", role = "rule", rule = "place"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc196)
      } else {
        acir.rule.call @top.right::@top.dispatch_body.0.call.20.12.0.call.41.21(%10) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:place", role = "rule", rule = "place"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc200)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.dispatch_body", owner = "top.dispatch", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %2 = acir.queue.empty %1 {acir.id = "v4", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc288)
      %3 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %4 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %5 = scf.if %2 -> (i1) {
        %7 = acir.queue.empty %4 {acir.id = "v7", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc289)
        scf.yield %7 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %6 = acir.ref @top::@top.source {acir.source_type = "queue<src_payloads_25ebcaa7.MicroOp>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
      scf.if %5 {
        acir.rule.call @top.dispatch::@top.dispatch_body.0.call.20.12(%6) {acir.id = "v13", acir.source_type = "void", contract = "Module:top.dispatch", instance = "top.dispatch", interface = "Module:top.dispatch:dispatch", role = "rule", rule = "dispatch"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () loc(#loc290)
      } else {
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.commit attributes {acir.instance = {definition = "src_model_3cf90884.Commit", name = "top.commit", parent = "top", rules = ["commit"]}} {
    acir.contract {relations = [], rules = ["commit"]}
    acir.cell @top.commit.retired : i32 {initial_fn = "ac_init_top.commit.retired", name = "top.commit.retired", type = "u32"}
    acir.cell @top.commit.last_tag : i32 {initial_fn = "ac_init_top.commit.last_tag", name = "top.commit.last_tag", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.commit_body.0.call.44.1 : () -> () {acir.function = {contract = "Module:top.commit", definition = "commit", instance = "top.commit", interface = "Module:top.commit:commit", kind = "rule", name = "top.commit_body.0.call.44.1", output_type = "void", owner = "top.commit", params = [], public = true, result = "void", rule = "commit"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.rule.call @top.rob::@top.commit_body.0.call.44.1.0.call.44.3() {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:retire", role = "rule", rule = "retire"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc291)
      acir.rule.call @top.rf::@top.commit_body.0.call.44.1.0.call.54.4(%0) {acir.id = "v5", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:commit", role = "rule", rule = "commit"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc292)
      %1 = acir.ref @top.commit::@top.commit.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc293)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc294)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc293)
      %4 = acir.ref @top.commit::@top.commit.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %4, %3 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc295)
      %5 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc296)
      %6 = acir.ref @top.commit::@top.commit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %6, %5 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc297)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.commit_body", owner = "top.commit", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      acir.rule.call @top.commit::@top.commit_body.0.call.44.1() {acir.id = "v2", acir.source_type = "void", contract = "Module:top.commit", instance = "top.commit", interface = "Module:top.commit:commit", role = "rule", rule = "commit"} : () -> () loc(#loc298)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.audit attributes {acir.instance = {definition = "src_model_3cf90884.CompletionAudit", name = "top.audit", parent = "top", rules = ["observe"]}} {
    acir.contract {relations = [], rules = ["observe"]}
    acir.cell @top.audit.last_tag : i32 {initial_fn = "ac_init_top.audit.last_tag", name = "top.audit.last_tag", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> tuple<!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> tuple<>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> tuple<>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> tuple<>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> tuple<>
    func.func private @ac_init_top.rob.slots.0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slots.3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, [8]>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    acir.rule @top.audit_body.0.call.56.2 : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () {acir.function = {contract = "Module:top.audit", definition = "observe", instance = "top.audit", interface = "Module:top.audit:observe", kind = "rule", name = "top.audit_body.0.call.56.2", output_type = "void", owner = "top.audit", params = [{id = "v1", name = "value", targets = ["top.event"], type = "signal<src_payloads_25ebcaa7.Completion>"}], public = true, result = "void", rule = "observe"}} body {
    ^bb0(%arg0: !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.signal.get %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "observe"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc299)
      %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc300)
      %2 = acir.ref @top.audit::@top.audit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %1 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc301)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.audit_body", owner = "top.audit", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.event {acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.rule.call @top.audit::@top.audit_body.0.call.56.2(%0) {acir.id = "v3", acir.source_type = "void", contract = "Module:top.audit", instance = "top.audit", interface = "Module:top.audit:observe", role = "rule", rule = "observe"} : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () loc(#loc302)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:45)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:65)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:70)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:73)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:76)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:57)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:88)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:93)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:96)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:99)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:80)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:111)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:116)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:119)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:122)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:103)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:134)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:139)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:142)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:145)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:126)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:18)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:43)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":90:40)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":91:40)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:68)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:71)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:74)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:77)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:80)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:83)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:86)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:89)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:67)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:66)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:69)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:72)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:75)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:78)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:81)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:84)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:87)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:65)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:68)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:71)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:74)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:77)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:80)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:83)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:86)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:89)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:67)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":9:39)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":55:40)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":56:45)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":57:42)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":59:43)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":60:44)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":76:44)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":109:38)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":110:20)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":111:20)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":112:17)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":113:9)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":114:9)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":115:9)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:9)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:22)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:16)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:27)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:15)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:16)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:29)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:23)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:34)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:22)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:16)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:23)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:30)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:9)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:20)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:15)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:22)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:16)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:27)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:15)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:22)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:16)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:9)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:17)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:15)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:30)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:24)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:35)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:23)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:22)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:16)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:9)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":64:17)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:12)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:33)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:29)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:50)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:13)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:26)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":67:13)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:13)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:20)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:16)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":58:20)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:26)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:32)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:48)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:17)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:9)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":76:17)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:16)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:32)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:48)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":79:20)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":81:17)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:17)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:9)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:24)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:16)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:9)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:21)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:12)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:28)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:34)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:37)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:20)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":136:15)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:19)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:12)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":138:28)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":138:39)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":138:20)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":139:17)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:12)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:28)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:44)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:28)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:39)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:20)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:24)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:36)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:16)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:21)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:16)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:9)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:18)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:12)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:13)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":152:12)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":152:24)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:35)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:13)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:23)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:35)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:13)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:23)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":46:16)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":21:17)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:16)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:32)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:55)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:75)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:20)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:20)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:17)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:16)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:9)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:17)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:19)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:12)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:20)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:16)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:40)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:17)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:37)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:29)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:27)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:20)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:16)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:40)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:17)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:37)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:29)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":40:27)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":42:13)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:20)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:16)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":15:20)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":17:9)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:16)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:30)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:27)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:30)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:20)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:27)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:16)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":25:13)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:13)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:19)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:31)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:47)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:19)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:29)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:39)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:39)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:39)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:39)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:38)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:51)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:59)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:38)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:51)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:59)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:17)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:45)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":45:17)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:27)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:38)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:48)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:66)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:16)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:12)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:25)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:25)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:37)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:13)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":70:17)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:18)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:9)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:27)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:38)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:48)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:66)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:16)
#loc251 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:19)
#loc252 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:9)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:21)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:9)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:18)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:9)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:16)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:20)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":79:13)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":43:22)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:22)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:34)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:9)
#loc265 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc266 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc267 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc268 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc269 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc270 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc271 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc272 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc273 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:16)
#loc274 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc275 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc276 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc277 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc278 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc279 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc280 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc281 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc282 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:23)
#loc283 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc284 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc285 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc286 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc287 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:12)
#loc288 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc289 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc290 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc291 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":64:17)
#loc292 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:9)
#loc293 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:19)
#loc294 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:29)
#loc295 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc296 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:20)
#loc297 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc298 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":70:9)
#loc299 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":80:17)
#loc300 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:20)
#loc301 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:9)
#loc302 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":84:9)
