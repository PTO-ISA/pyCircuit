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
  func.func @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot0.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %0 = acir.aggregate(%false, %false_0, %c0_i32, %c0_i32_1, %c0_i32_2) {acir.id = "v38", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc23)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot1.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %0 = acir.aggregate(%false, %false_0, %c0_i32, %c0_i32_1, %c0_i32_2) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc23)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot2.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc23)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %0 = acir.aggregate(%false, %false_0, %c0_i32, %c0_i32_1, %c0_i32_2) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc23)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot3.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob", definition = "complete", instance = "top.rob", interface = "Module:top.rob:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16", output_type = "void", owner = "top.rob", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc67)
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc68)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc69)
      %1 = arith.andi %0, %c3_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc68)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc70)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc71)
      scf.if %2 {
        acir.rule.call @top.rob.slot0::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.11(%arg0, %arg1) {acir.id = "v12", acir.source_type = "void", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc72)
      } else {
        %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc73)
        %3 = arith.cmpi eq, %1, %c1_i32_0 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc74)
        scf.if %3 {
          acir.rule.call @top.rob.slot1::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.12.14(%arg0, %arg1) {acir.id = "v15", acir.source_type = "void", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc75)
        } else {
          %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc76)
          %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc77)
          scf.if %4 {
            acir.rule.call @top.rob.slot2::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.13.17(%arg0, %arg1) {acir.id = "v18", acir.source_type = "void", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc78)
          } else {
            acir.rule.call @top.rob.slot3::@top.cdb_body.0.call.11.7.0.call.11.16.0.call.14.18(%arg0, %arg1) {acir.id = "v19", acir.source_type = "void", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc79)
          }
        }
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "lookup", instance = "top.rob", interface = "Module:top.rob:lookup", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "lookup"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc80)
      %1 = arith.subi %arg0, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc81)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc82)
      %2 = arith.andi %1, %c3_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc81)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc83)
      %3 = arith.cmpi eq, %2, %c0_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc84)
      %4:3 = scf.if %3 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.19.12() {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc85)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %7 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc86)
        %7 = arith.cmpi eq, %2, %c1_i32_0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc87)
        %8:3 = scf.if %7 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
          %9 = acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.20.17() {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc88)
          %true_1 = arith.constant {acir.source_type = "bool"} true
          %false_2 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_2, %true_1, %9 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        } else {
          %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc89)
          %9 = arith.cmpi eq, %2, %c2_i32 {acir.id = "v22", acir.source_type = "bool"} : i32 loc(#loc90)
          %10:3 = scf.if %9 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
            %11 = acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.21.22() {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc91)
            %true_1 = arith.constant {acir.source_type = "bool"} true
            %false_2 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_2, %true_1, %11 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          } else {
            scf.yield %true, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          }
          scf.yield %10#0, %10#1, %10#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        }
        scf.yield %8#0, %8#1, %8#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %5:3 = scf.if %4#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.22.43() {acir.id = "v44", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc92)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %7 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %4#0, %4#1, %4#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %6 = acir.unary %5#0 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %6 {acir.source_type = "void"}
      acir.return %5#2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.25.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob", definition = "room", instance = "top.rob", interface = "Module:top.rob:room", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.25.1", output_type = "bool", owner = "top.rob", params = [{id = "v1", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "room"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc93)
      %3 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %4 = acir.cell.read %3 {acir.id = "v10", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc94)
      %5 = arith.subi %2, %4 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc93)
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc95)
      %6 = arith.cmpi ult, %5, %c4_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc93)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %6 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10 : (i32) -> i32 {acir.function = {contract = "Module:top.rob", definition = "allocate", guard_info = {name = "room", source = "function"}, instance = "top.rob", interface = "Module:top.rob:allocate", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10", output_type = "u32", owner = "top.rob", params = [{id = "v1", name = "rd", targets = [], type = "u32"}], public = true, result = "u32", rule = "allocate"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.25.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:room", role = "rule", rule = "room"} : (i32) -> i1 loc(#loc96)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc97)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc98)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc99)
      %3 = arith.subi %2, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc100)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc101)
      %4 = arith.andi %3, %c3_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc100)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc102)
      %5 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc103)
      scf.if %5 {
        acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.26.15(%2, %arg0) {acir.id = "v16", acir.source_type = "void", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc104)
      } else {
        %c1_i32_3 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc105)
        %9 = arith.cmpi eq, %4, %c1_i32_3 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc106)
        scf.if %9 {
          acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.28.18(%2, %arg0) {acir.id = "v19", acir.source_type = "void", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc107)
        } else {
          %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc108)
          %10 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc109)
          scf.if %10 {
            acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.30.21(%2, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc110)
          } else {
            acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.32.22(%2, %arg0) {acir.id = "v23", acir.source_type = "void", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc111)
          }
        }
      }
      %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc112)
      %6 = arith.addi %2, %c1_i32_0 {acir.id = "v34", acir.source_type = "u32"} : i32 loc(#loc113)
      %7 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %7, %6 {acir.id = "v36", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc114)
      %true_1 = arith.constant {acir.source_type = "bool"} true
      %false_2 = arith.constant {acir.source_type = "bool"} false
      %8 = acir.unary %false_2 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %8 {acir.source_type = "void"}
      acir.return %2 : i32 {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "retire", instance = "top.rob", interface = "Module:top.rob:retire", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc115)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc116)
      %3 = arith.subi %2, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc117)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc118)
      %4 = arith.andi %3, %c3_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc117)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc119)
      %5 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc120)
      %6 = scf.if %5 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %10 = acir.rule.call @top.rob.slot0::@top.commit_body.0.call.41.1.0.call.41.3.0.call.41.13(%2) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc121)
        scf.yield %10 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        %c1_i32_3 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc122)
        %10 = arith.cmpi eq, %4, %c1_i32_3 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc123)
        %11 = scf.if %10 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
          %12 = acir.rule.call @top.rob.slot1::@top.commit_body.0.call.41.1.0.call.41.3.0.call.43.16(%2) {acir.id = "v17", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc124)
          scf.yield %12 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        } else {
          %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc125)
          %12 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc126)
          %13 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
            %14 = acir.rule.call @top.rob.slot2::@top.commit_body.0.call.41.1.0.call.41.3.0.call.45.19(%2) {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc127)
            scf.yield %14 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          } else {
            %14 = acir.rule.call @top.rob.slot3::@top.commit_body.0.call.41.1.0.call.41.3.0.call.47.20(%2) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
            scf.yield %14 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          }
          scf.yield %13 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        }
        scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %c1_i32_0 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc129)
      %7 = arith.addi %2, %c1_i32_0 {acir.id = "v35", acir.source_type = "u32"} : i32 loc(#loc130)
      %8 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %8, %7 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc131)
      %true_1 = arith.constant {acir.source_type = "bool"} true
      %false_2 = arith.constant {acir.source_type = "bool"} false
      %9 = acir.unary %false_2 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %9 {acir.source_type = "void"}
      acir.return %6 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slot0 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slot0", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}
    acir.cell @top.rob.slot0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot0.state", name = "top.rob.slot0.state", type = "src_payloads_25ebcaa7.RobEntry"}
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.11 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot0", definition = "complete", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.11.11", output_type = "void", owner = "top.rob.slot0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc132)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc133)
      %3 = scf.if %2 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc134)
        %8 = acir.unary %7 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc136)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc136)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %5 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %6 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.extract %1 {acir.id = "v21", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc138)
        %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %11 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %12 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %13 = acir.aggregate(%8, %true_0, %10, %11, %12) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc137)
        %14 = acir.extract %13 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %15 = acir.extract %13 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc139)
        %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc139)
        %17 = acir.extract %13 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %18 = acir.extract %13 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %19 = acir.extract %13 {acir.id = "v34", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %20 = acir.aggregate(%15, %16, %17, %18, %arg1) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc139)
        acir.cell.write %5, %20 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc140)
        scf.yield %20 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.19.12 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot0", definition = "peek", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.19.12", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot0", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc141)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.26.15.0.call.26.2 : (i32, i32) -> i1 {acir.function = {contract = "Module:top.rob.slot0", definition = "free", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:free", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.26.15.0.call.26.2", output_type = "bool", owner = "top.rob.slot0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc142)
      %3 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc142)
      %4 = acir.unary %3 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc143)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.26.15 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot0", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot0", interface = "Module:top.rob.slot0:allocate", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.26.15", output_type = "void", owner = "top.rob.slot0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.26.15.0.call.26.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:free", role = "rule", rule = "free"} : (i32, i32) -> i1 loc(#loc104)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc144)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc145)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc146)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc147)
      %0 = acir.aggregate(%true_0, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
      %1 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc149)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3.0.call.41.13.0.call.41.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob.slot0", definition = "completed", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:completed", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3.0.call.41.13.0.call.41.1", output_type = "bool", owner = "top.rob.slot0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "bool", rule = "completed"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc150)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc151)
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
        scf.yield %7 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc153)
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
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3.0.call.41.13 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot0", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot0", interface = "Module:top.rob.slot0:retire", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3.0.call.41.13", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot0", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob.slot0::@top.commit_body.0.call.41.1.0.call.41.3.0.call.41.13.0.call.41.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:completed", role = "rule", rule = "completed"} : (i32) -> i1 loc(#loc121)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc154)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc155)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc156)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc156)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %3 = acir.aggregate(%false_0, %false_1, %c0_i32, %c0_i32_2, %c0_i32_3) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
      %4 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %4, %3 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc157)
      %true_4 = arith.constant {acir.source_type = "bool"} true
      %false_5 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_5 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slot1 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slot1", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}
    acir.cell @top.rob.slot1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot1.state", name = "top.rob.slot1.state", type = "src_payloads_25ebcaa7.RobEntry"}
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.12.14 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot1", definition = "complete", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.12.14", output_type = "void", owner = "top.rob.slot1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc132)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc133)
      %3 = scf.if %2 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc134)
        %8 = acir.unary %7 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc136)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc136)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %5 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %6 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.extract %1 {acir.id = "v21", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc138)
        %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %11 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %12 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %13 = acir.aggregate(%8, %true_0, %10, %11, %12) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc137)
        %14 = acir.extract %13 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %15 = acir.extract %13 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc139)
        %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc139)
        %17 = acir.extract %13 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %18 = acir.extract %13 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %19 = acir.extract %13 {acir.id = "v34", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %20 = acir.aggregate(%15, %16, %17, %18, %arg1) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc139)
        acir.cell.write %5, %20 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc140)
        scf.yield %20 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.20.17 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot1", definition = "peek", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.20.17", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot1", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc141)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.28.18.0.call.28.2 : (i32, i32) -> i1 {acir.function = {contract = "Module:top.rob.slot1", definition = "free", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:free", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.28.18.0.call.28.2", output_type = "bool", owner = "top.rob.slot1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc142)
      %3 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc142)
      %4 = acir.unary %3 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc143)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.28.18 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot1", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot1", interface = "Module:top.rob.slot1:allocate", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.28.18", output_type = "void", owner = "top.rob.slot1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.28.18.0.call.28.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:free", role = "rule", rule = "free"} : (i32, i32) -> i1 loc(#loc107)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc144)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc145)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc146)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc147)
      %0 = acir.aggregate(%true_0, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
      %1 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc149)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3.0.call.43.16.0.call.43.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob.slot1", definition = "completed", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:completed", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3.0.call.43.16.0.call.43.1", output_type = "bool", owner = "top.rob.slot1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "bool", rule = "completed"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc150)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc151)
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
        scf.yield %7 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc153)
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
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3.0.call.43.16 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot1", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot1", interface = "Module:top.rob.slot1:retire", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3.0.call.43.16", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot1", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob.slot1::@top.commit_body.0.call.41.1.0.call.41.3.0.call.43.16.0.call.43.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:completed", role = "rule", rule = "completed"} : (i32) -> i1 loc(#loc124)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc154)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc155)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc156)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc156)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %3 = acir.aggregate(%false_0, %false_1, %c0_i32, %c0_i32_2, %c0_i32_3) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
      %4 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %4, %3 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc157)
      %true_4 = arith.constant {acir.source_type = "bool"} true
      %false_5 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_5 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slot2 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slot2", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}
    acir.cell @top.rob.slot2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot2.state", name = "top.rob.slot2.state", type = "src_payloads_25ebcaa7.RobEntry"}
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.13.17 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot2", definition = "complete", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.13.17", output_type = "void", owner = "top.rob.slot2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc132)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc133)
      %3 = scf.if %2 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc134)
        %8 = acir.unary %7 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc136)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc136)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %5 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %6 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.extract %1 {acir.id = "v21", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc138)
        %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %11 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %12 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %13 = acir.aggregate(%8, %true_0, %10, %11, %12) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc137)
        %14 = acir.extract %13 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %15 = acir.extract %13 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc139)
        %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc139)
        %17 = acir.extract %13 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %18 = acir.extract %13 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %19 = acir.extract %13 {acir.id = "v34", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %20 = acir.aggregate(%15, %16, %17, %18, %arg1) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc139)
        acir.cell.write %5, %20 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc140)
        scf.yield %20 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.21.22 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot2", definition = "peek", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.21.22", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot2", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc141)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.30.21.0.call.30.2 : (i32, i32) -> i1 {acir.function = {contract = "Module:top.rob.slot2", definition = "free", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:free", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.30.21.0.call.30.2", output_type = "bool", owner = "top.rob.slot2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc142)
      %3 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc142)
      %4 = acir.unary %3 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc143)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.30.21 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot2", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot2", interface = "Module:top.rob.slot2:allocate", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.30.21", output_type = "void", owner = "top.rob.slot2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.30.21.0.call.30.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:free", role = "rule", rule = "free"} : (i32, i32) -> i1 loc(#loc110)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc144)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc145)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc146)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc147)
      %0 = acir.aggregate(%true_0, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
      %1 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc149)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3.0.call.45.19.0.call.45.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob.slot2", definition = "completed", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:completed", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3.0.call.45.19.0.call.45.1", output_type = "bool", owner = "top.rob.slot2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "bool", rule = "completed"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc150)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc151)
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
        scf.yield %7 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc153)
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
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3.0.call.45.19 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot2", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot2", interface = "Module:top.rob.slot2:retire", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3.0.call.45.19", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot2", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob.slot2::@top.commit_body.0.call.41.1.0.call.41.3.0.call.45.19.0.call.45.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:completed", role = "rule", rule = "completed"} : (i32) -> i1 loc(#loc127)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc154)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc155)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc156)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc156)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %3 = acir.aggregate(%false_0, %false_1, %c0_i32, %c0_i32_2, %c0_i32_3) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
      %4 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %4, %3 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc157)
      %true_4 = arith.constant {acir.source_type = "bool"} true
      %false_5 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_5 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slot3 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", name = "top.rob.slot3", parent = "top.rob", rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "completed", "free", "peek", "retire"]}
    acir.cell @top.rob.slot3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot3.state", name = "top.rob.slot3.state", type = "src_payloads_25ebcaa7.RobEntry"}
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.cdb_body.0.call.11.7.0.call.11.16.0.call.14.18 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot3", definition = "complete", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:complete", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.11.16.0.call.14.18", output_type = "void", owner = "top.rob.slot3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc132)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc133)
      %3 = scf.if %2 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc134)
        %8 = acir.unary %7 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc135)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc136)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc136)
        scf.yield %8 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %5 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %6 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.extract %1 {acir.id = "v21", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc138)
        %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc137)
        %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %11 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %12 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc137)
        %13 = acir.aggregate(%8, %true_0, %10, %11, %12) {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc137)
        %14 = acir.extract %13 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %15 = acir.extract %13 {acir.id = "v30", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc139)
        %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc139)
        %17 = acir.extract %13 {acir.id = "v32", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %18 = acir.extract %13 {acir.id = "v33", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %19 = acir.extract %13 {acir.id = "v34", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc139)
        %20 = acir.aggregate(%15, %16, %17, %18, %arg1) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc139)
        acir.cell.write %5, %20 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc140)
        scf.yield %20 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.22.43 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot3", definition = "peek", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43.0.call.22.43", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot3", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc141)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.32.22.0.call.32.2 : (i32, i32) -> i1 {acir.function = {contract = "Module:top.rob.slot3", definition = "free", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:free", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.32.22.0.call.32.2", output_type = "bool", owner = "top.rob.slot3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc142)
      %3 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc142)
      %4 = acir.unary %3 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc143)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.25.10.0.call.32.22 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot3", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot3", interface = "Module:top.rob.slot3:allocate", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.25.10.0.call.32.22", output_type = "void", owner = "top.rob.slot3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.19.12.0.call.25.10.0.call.32.22.0.call.32.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:free", role = "rule", rule = "free"} : (i32, i32) -> i1 loc(#loc111)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc144)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc145)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc146)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc147)
      %0 = acir.aggregate(%true_0, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc148)
      %1 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc149)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3.0.call.47.20.0.call.47.1 : (i32) -> i1 {acir.function = {contract = "Module:top.rob.slot3", definition = "completed", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:completed", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3.0.call.47.20.0.call.47.1", output_type = "bool", owner = "top.rob.slot3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "bool", rule = "completed"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc150)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc151)
      %4 = scf.if %3 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v10", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc152)
        scf.yield %7 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %2 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
        %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc153)
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
    acir.rule @top.commit_body.0.call.41.1.0.call.41.3.0.call.47.20 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot3", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot3", interface = "Module:top.rob.slot3:retire", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.41.3.0.call.47.20", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot3", params = [{id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = acir.rule.call @top.rob.slot3::@top.commit_body.0.call.41.1.0.call.41.3.0.call.47.20.0.call.47.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:completed", role = "rule", rule = "completed"} : (i32) -> i1 loc(#loc128)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc154)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc155)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc156)
      %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc156)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %3 = acir.aggregate(%false_0, %false_1, %c0_i32, %c0_i32_2, %c0_i32_3) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc156)
      %4 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %4, %3 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc157)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.dispatch_body.0.call.19.12.0.call.19.6 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:top.rf", definition = "operand", instance = "top.rf", interface = "Module:top.rf:operand", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.19.6", output_type = "src_payloads_25ebcaa7.Operand", owner = "top.rf", params = [{id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Operand", rule = "operand"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %false = arith.constant {acir.source_type = "bool"} false
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc158)
      %1 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc159)
      acir.check %1 {acir.id = "v9", acir.source_type = "void"} loc(#loc160)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc161)
      %2 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc162)
      %3:3 = scf.if %2 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc163)
        %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc164)
        %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc165)
        %9 = acir.aggregate(%true_0, %c0_i32_1, %c0_i32_2) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
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
        %9 = acir.cell.read %4 {acir.id = "v25", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc167)
        %10 = "acir.array.index"(%9, %arg0) {acir.id = "v26", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [8]>, i32) -> i32 loc(#loc167)
        %11 = acir.cell.read %5 {acir.id = "v28", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc168)
        %12 = "acir.array.index"(%11, %arg0) {acir.id = "v29", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [8]>, i32) -> i32 loc(#loc168)
        %13 = arith.cmpi eq, %10, %12 {acir.id = "v30", acir.source_type = "bool"} : i32 loc(#loc169)
        %14:3 = scf.if %13 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc170)
          %16 = acir.cell.read %6 {acir.id = "v33", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc171)
          %17 = "acir.array.index"(%16, %arg0) {acir.id = "v34", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [8]>, i32) -> i32 loc(#loc171)
          %18 = acir.aggregate(%true_0, %10, %17) {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc172)
          %true_1 = arith.constant {acir.source_type = "bool"} true
          %false_2 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_2, %true_1, %18 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
        } else {
          scf.yield %3#0, %3#1, %3#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
        }
        %15:3 = scf.if %14#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
          %16 = acir.rule.call @top.rob::@top.dispatch_body.0.call.19.12.0.call.19.6.0.call.19.43(%10) {acir.id = "v44", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:lookup", role = "rule", rule = "lookup"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc173)
          %17 = acir.extract %16 {acir.id = "v45", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc174)
          %18 = scf.if %17 -> (i1) {
            %22 = acir.extract %16 {acir.id = "v46", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc175)
            scf.yield %22 : i1
          } else {
            %false_0 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_0 : i1
          }
          %19 = scf.if %18 -> (i1) {
            %22 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc176)
            %23 = arith.cmpi eq, %22, %10 {acir.id = "v52", acir.source_type = "bool"} : i32 loc(#loc176)
            scf.yield %23 : i1
          } else {
            %false_0 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_0 : i1
          }
          %20:3 = scf.if %19 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
            %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc177)
            %22 = acir.extract %16 {acir.id = "v58", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc178)
            %23 = acir.aggregate(%true_0, %10, %22) {acir.id = "v59", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc179)
            %true_1 = arith.constant {acir.source_type = "bool"} true
            %false_2 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_2, %true_1, %23 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
          } else {
            scf.yield %14#0, %14#1, %14#2 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">
          }
          %21:3 = scf.if %20#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Operand">) {
            %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc180)
            %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc181)
            %22 = acir.aggregate(%false_0, %10, %c0_i32_1) {acir.id = "v70", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc182)
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
    acir.rule @top.dispatch_body.0.call.19.12.0.call.35.12 : (i32, i32) -> () {acir.function = {contract = "Module:top.rf", definition = "rename", instance = "top.rf", interface = "Module:top.rf:rename", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.35.12", output_type = "void", owner = "top.rf", params = [{id = "v1", name = "rd", targets = [], type = "u32"}, {id = "v2", name = "tag", targets = [], type = "u32"}], public = true, result = "void", rule = "rename"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc183)
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc184)
      acir.check %0 {acir.id = "v8", acir.source_type = "void"} loc(#loc185)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc186)
      %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc187)
      %2 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %3 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %4 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      scf.if %1 {
        %5 = acir.cell.read %4 {acir.id = "v14", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc188)
        %6 = acir.update %5, %arg1[%arg0] {acir.id = "v15", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i32) -> !acir.array<i32, [8]> loc(#loc188)
        acir.cell.write %4, %6 {acir.id = "v16", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [8]>>, !acir.array<i32, [8]> loc(#loc188)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.41.1.0.call.50.4 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () {acir.function = {contract = "Module:top.rf", definition = "commit", instance = "top.rf", interface = "Module:top.rf:commit", kind = "rule", name = "top.commit_body.0.call.41.1.0.call.50.4", output_type = "void", owner = "top.rf", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.extract %arg0 {acir.id = "v5", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc189)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc190)
      %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc189)
      %2 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %3 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %4 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %5 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %6 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      %7 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32;8>>"} : !acir.cell<!acir.array<i32, [8]>>
      scf.if %1 {
        %8 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc191)
        %9 = acir.cell.read %4 {acir.id = "v12", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc192)
        %10 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc193)
        %11 = acir.update %9, %8[%10] {acir.id = "v14", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i32) -> !acir.array<i32, [8]> loc(#loc192)
        acir.cell.write %4, %11 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [8]>>, !acir.array<i32, [8]> loc(#loc192)
        %12 = acir.extract %arg0 {acir.id = "v17", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc194)
        %13 = acir.cell.read %7 {acir.id = "v20", acir.source_type = "array<u32;8>", role = "observe"} : !acir.cell<!acir.array<i32, [8]>> -> !acir.array<i32, [8]> loc(#loc195)
        %14 = acir.extract %arg0 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc196)
        %15 = acir.update %13, %12[%14] {acir.id = "v22", acir.source_type = "array<u32;8>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [8]>, i32, i32) -> !acir.array<i32, [8]> loc(#loc195)
        acir.cell.write %7, %15 {acir.id = "v23", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [8]>>, !acir.array<i32, [8]> loc(#loc195)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc197)
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
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc198)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc199)
      %4 = scf.if %3 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v10", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc200)
        %9 = arith.cmpi eq, %8, %arg0 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc200)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc201)
        %9 = acir.extract %8 {acir.id = "v17", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc201)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %6 = scf.if %5 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc202)
        %9 = acir.extract %8 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc202)
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
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.2.81.0.call.2.7.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:issuable", role = "rule", rule = "issuable"} : (i32) -> i1 loc(#loc203)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc204)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Entry"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc205)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc206)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc206)
      %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc206)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc206)
      %3 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc206)
      %4 = acir.aggregate() {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc206)
      %5 = acir.aggregate(%false_0, %c0_i32, %c0_i32_1, %c0_i32_2, %3, %4) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc206)
      %6 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %6, %5 {acir.id = "v18", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc207)
      %true_3 = arith.constant {acir.source_type = "bool"} true
      %false_4 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_4 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top.cdb_body.0.call.11.7.0.call.16.19 : (i32, i32) -> () {acir.function = {contract = "Module:top.left", definition = "wakeup", instance = "top.left", interface = "Module:top.left:wakeup", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.16.19", output_type = "void", owner = "top.left", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "wakeup"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc208)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc209)
      %2 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc210)
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %5 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc211)
        %6 = acir.extract %5 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc211)
        %7 = acir.unary %6 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc212)
        %8 = scf.if %7 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc213)
          %16 = acir.extract %15 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc213)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc213)
          scf.yield %17 : i1
        } else {
          %false_0 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_0 : i1
        }
        %9:2 = scf.if %8 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc214)
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc215)
          %16 = acir.aggregate(%true_0, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc216)
          %17 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc214)
          %18 = acir.extract %1 {acir.id = "v24", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc214)
          %19 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc214)
          %20 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc214)
          %21 = acir.extract %1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc214)
          %22 = acir.extract %1 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc214)
          %23 = acir.aggregate(%17, %18, %19, %20, %16, %22) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc214)
          %true_1 = arith.constant {acir.source_type = "bool"} true loc(#loc217)
          scf.yield %true_1, %23 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %10 = acir.extract %9#1 {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc218)
        %11 = acir.extract %10 {acir.id = "v37", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc218)
        %12 = acir.unary %11 "not" {acir.id = "v38", acir.source_type = "bool"} : i1 -> i1 loc(#loc219)
        %13 = scf.if %12 -> (i1) {
          %15 = acir.extract %9#1 {acir.id = "v39", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc220)
          %16 = acir.extract %15 {acir.id = "v40", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc220)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc220)
          scf.yield %17 : i1
        } else {
          %false_0 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_0 : i1
        }
        %14:2 = scf.if %13 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.extract %9#1 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc221)
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc222)
          %16 = acir.aggregate(%true_0, %arg0, %arg1) {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc223)
          %17 = acir.extract %9#1 {acir.id = "v49", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc221)
          %18 = acir.extract %9#1 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc221)
          %19 = acir.extract %9#1 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc221)
          %20 = acir.extract %9#1 {acir.id = "v52", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc221)
          %21 = acir.extract %9#1 {acir.id = "v53", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc221)
          %22 = acir.extract %9#1 {acir.id = "v54", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc221)
          %23 = acir.aggregate(%17, %18, %19, %20, %21, %16) {acir.id = "v55", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc221)
          %true_1 = arith.constant {acir.source_type = "bool"} true loc(#loc224)
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
        acir.cell.write %4, %3#1 {acir.id = "v68", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc225)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.36.20.0.call.36.1 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 {acir.function = {contract = "Module:top.left", definition = "free", instance = "top.left", interface = "Module:top.left:free", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.36.20.0.call.36.1", output_type = "bool", owner = "top.left", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc226)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc226)
      %4 = acir.unary %3 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc227)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.36.20 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:top.left", definition = "place", guard_info = {name = "free", source = "function"}, instance = "top.left", interface = "Module:top.left:place", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.36.20", output_type = "void", owner = "top.left", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "void", rule = "place"}} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.rule.call @top.left::@top.dispatch_body.0.call.19.12.0.call.36.20.0.call.36.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:free", role = "rule", rule = "free"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc228)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc229)
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc230)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc197)
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
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc198)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc199)
      %4 = scf.if %3 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v10", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc200)
        %9 = arith.cmpi eq, %8, %arg0 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc200)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %5 = scf.if %4 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc201)
        %9 = acir.extract %8 {acir.id = "v17", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc201)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %6 = scf.if %5 -> (i1) {
        %8 = acir.extract %2 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc202)
        %9 = acir.extract %8 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc202)
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
      %0 = acir.rule.call @top.right::@top.scheduler_body.0.call.2.81.0.call.4.17.0.call.4.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:issuable", role = "rule", rule = "issuable"} : (i32) -> i1 loc(#loc231)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc204)
    } body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_payloads_25ebcaa7.Entry"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc205)
      %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc206)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc206)
      %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc206)
      %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc206)
      %3 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc206)
      %4 = acir.aggregate() {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc206)
      %5 = acir.aggregate(%false_0, %c0_i32, %c0_i32_1, %c0_i32_2, %3, %4) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc206)
      %6 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %6, %5 {acir.id = "v18", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc207)
      %true_3 = arith.constant {acir.source_type = "bool"} true
      %false_4 = arith.constant {acir.source_type = "bool"} false
      %7 = acir.unary %false_4 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %7 {acir.source_type = "void"}
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.source_type = "void"}
    }
    acir.rule @top.cdb_body.0.call.11.7.0.call.17.22 : (i32, i32) -> () {acir.function = {contract = "Module:top.right", definition = "wakeup", instance = "top.right", interface = "Module:top.right:wakeup", kind = "rule", name = "top.cdb_body.0.call.11.7.0.call.17.22", output_type = "void", owner = "top.right", params = [{id = "v1", name = "tag", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "wakeup"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc208)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc209)
      %2 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc210)
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %5 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc211)
        %6 = acir.extract %5 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc211)
        %7 = acir.unary %6 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc212)
        %8 = scf.if %7 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc213)
          %16 = acir.extract %15 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc213)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc213)
          scf.yield %17 : i1
        } else {
          %false_0 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_0 : i1
        }
        %9:2 = scf.if %8 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc214)
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc215)
          %16 = acir.aggregate(%true_0, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc216)
          %17 = acir.extract %1 {acir.id = "v23", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc214)
          %18 = acir.extract %1 {acir.id = "v24", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc214)
          %19 = acir.extract %1 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc214)
          %20 = acir.extract %1 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc214)
          %21 = acir.extract %1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc214)
          %22 = acir.extract %1 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc214)
          %23 = acir.aggregate(%17, %18, %19, %20, %16, %22) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc214)
          %true_1 = arith.constant {acir.source_type = "bool"} true loc(#loc217)
          scf.yield %true_1, %23 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %10 = acir.extract %9#1 {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc218)
        %11 = acir.extract %10 {acir.id = "v37", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc218)
        %12 = acir.unary %11 "not" {acir.id = "v38", acir.source_type = "bool"} : i1 -> i1 loc(#loc219)
        %13 = scf.if %12 -> (i1) {
          %15 = acir.extract %9#1 {acir.id = "v39", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc220)
          %16 = acir.extract %15 {acir.id = "v40", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc220)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc220)
          scf.yield %17 : i1
        } else {
          %false_0 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_0 : i1
        }
        %14:2 = scf.if %13 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.extract %9#1 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc221)
          %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc222)
          %16 = acir.aggregate(%true_0, %arg0, %arg1) {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc223)
          %17 = acir.extract %9#1 {acir.id = "v49", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc221)
          %18 = acir.extract %9#1 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc221)
          %19 = acir.extract %9#1 {acir.id = "v51", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc221)
          %20 = acir.extract %9#1 {acir.id = "v52", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc221)
          %21 = acir.extract %9#1 {acir.id = "v53", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc221)
          %22 = acir.extract %9#1 {acir.id = "v54", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc221)
          %23 = acir.aggregate(%17, %18, %19, %20, %21, %16) {acir.id = "v55", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc221)
          %true_1 = arith.constant {acir.source_type = "bool"} true loc(#loc224)
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
        acir.cell.write %4, %3#1 {acir.id = "v68", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc225)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.38.21.0.call.38.1 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 {acir.function = {contract = "Module:top.right", definition = "free", instance = "top.right", interface = "Module:top.right:free", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.38.21.0.call.38.1", output_type = "bool", owner = "top.right", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "bool", rule = "free"}} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc226)
      %3 = acir.extract %2 {acir.id = "v9", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc226)
      %4 = acir.unary %3 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc227)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %5 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.19.12.0.call.38.21 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:top.right", definition = "place", guard_info = {name = "free", source = "function"}, instance = "top.right", interface = "Module:top.right:place", kind = "rule", name = "top.dispatch_body.0.call.19.12.0.call.38.21", output_type = "void", owner = "top.right", params = [{id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "void", rule = "place"}} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.rule.call @top.right::@top.dispatch_body.0.call.19.12.0.call.38.21.0.call.38.1(%arg0) {acir.id = "v2", acir.source_type = "bool", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:free", role = "rule", rule = "free"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc232)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc229)
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc230)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc233)
        %4 = acir.rule.call @top.left::@top.scheduler_body.0.call.2.81.0.call.2.7(%c0_i32) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc203)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %4 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %true, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      %2:3 = scf.if %1#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc234)
        %4 = acir.rule.call @top.right::@top.scheduler_body.0.call.2.81.0.call.4.17(%c0_i32) {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc231)
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
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc235)
        %4 = acir.rule.call @top.left::@top.scheduler_body.0.call.2.81.0.call.2.7(%c1_i32) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc236)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %4 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %true, %false, %0 : i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      %2:3 = scf.if %1#0 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc237)
        %4 = acir.rule.call @top.right::@top.scheduler_body.0.call.2.81.0.call.4.17(%c1_i32) {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc238)
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
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.0.1() {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc239)
      %1 = acir.rule.call @top.right::@top.scheduler_body.0.call.1.2() {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc240)
      %2 = acir.extract %0 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc241)
      %3 = scf.if %2 -> (i1) {
        %18 = acir.extract %0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc242)
        %19 = acir.extract %18 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc242)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %4 = scf.if %3 -> (i1) {
        %18 = acir.extract %0 {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc243)
        %19 = acir.extract %18 {acir.id = "v12", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc243)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %5 = acir.extract %1 {acir.id = "v17", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc244)
      %6 = scf.if %5 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc245)
        %19 = acir.extract %18 {acir.id = "v19", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc245)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %7 = scf.if %6 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc246)
        %19 = acir.extract %18 {acir.id = "v25", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc246)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %8 = scf.if %4 -> (i1) {
        %18 = acir.extract %0 {acir.id = "v30", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc247)
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc248)
        %19 = arith.cmpi eq, %18, %c0_i32 {acir.id = "v32", acir.source_type = "bool"} : i32 loc(#loc247)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %9 = scf.if %7 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v37", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc249)
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc250)
        %19 = arith.cmpi eq, %18, %c0_i32 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc249)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %10 = scf.if %4 -> (i1) {
        %18 = acir.extract %0 {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc251)
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc252)
        %19 = arith.cmpi eq, %18, %c1_i32 {acir.id = "v46", acir.source_type = "bool"} : i32 loc(#loc251)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %11 = scf.if %7 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v51", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc253)
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc254)
        %19 = arith.cmpi eq, %18, %c1_i32 {acir.id = "v53", acir.source_type = "bool"} : i32 loc(#loc253)
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %12 = scf.if %8 -> (i1) {
        %18 = acir.unary %9 "not" {acir.id = "v58", acir.source_type = "bool"} : i1 -> i1 loc(#loc255)
        %19 = scf.if %18 -> (i1) {
          %true_0 = arith.constant {acir.source_type = "bool"} true
          scf.yield %true_0 : i1
        } else {
          %20 = acir.extract %0 {acir.id = "v60", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc256)
          %21 = acir.extract %1 {acir.id = "v61", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc257)
          %22 = arith.cmpi ult, %20, %21 {acir.id = "v62", acir.source_type = "bool"} : i32 loc(#loc256)
          scf.yield %22 : i1
        }
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %13 = scf.if %10 -> (i1) {
        %18 = acir.unary %11 "not" {acir.id = "v70", acir.source_type = "bool"} : i1 -> i1 loc(#loc258)
        %19 = scf.if %18 -> (i1) {
          %true_0 = arith.constant {acir.source_type = "bool"} true
          scf.yield %true_0 : i1
        } else {
          %20 = acir.extract %0 {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc259)
          %21 = acir.extract %1 {acir.id = "v73", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc260)
          %22 = arith.cmpi ult, %20, %21 {acir.id = "v74", acir.source_type = "bool"} : i32 loc(#loc259)
          scf.yield %22 : i1
        }
        scf.yield %19 : i1
      } else {
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      }
      %14 = acir.rule.call @top.scheduler::@top.scheduler_body.0.call.2.81(%12) {acir.id = "v82", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.scheduler", instance = "top.scheduler", interface = "Module:top.scheduler:issue_add", output_binding = @top::@top.add_requests, role = "rule", rule = "issue_add"} : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc261)
      %15 = acir.rule.call @top.scheduler::@top.scheduler_body.0.call.7.82(%13) {acir.id = "v83", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.scheduler", instance = "top.scheduler", interface = "Module:top.scheduler:issue_mul", output_binding = @top::@top.mul_requests, role = "rule", rule = "issue_mul"} : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc262)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
      %1 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc263)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc264)
      %3 = acir.extract %1 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc265)
      %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc266)
      %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc266)
      %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc267)
      %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc267)
      %8 = arith.addi %5, %7 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc266)
      %9 = acir.aggregate(%2, %3, %8) {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc268)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %10 = "acir.optional"(%9, %true_0) {acir.source_type = "src_payloads_25ebcaa7.Completion"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      acir.return %10 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.adder_body", owner = "top.adder", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.add_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.rule.call @top.adder::@top.adder_body.0.call.8.2(%0) {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.adder", instance = "top.adder", interface = "Module:top.adder:execute", output_binding = @top::@top.add_done, role = "rule", rule = "execute"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc269)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc270)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc271)
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc270)
      %4 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %5 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %6 = scf.if %3 -> (i1) {
        %11 = acir.cell.read %4 {acir.id = "v11", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc272)
        %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc273)
        %12 = arith.subi %11, %c1_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc272)
        acir.cell.write %5, %12 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc274)
        %false_0 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_0 : i1
      } else {
        scf.yield %true : i1
      }
      %7 = acir.ref @top.multiplier::@top.multiplier.request {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %8 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      %9:3 = scf.if %6 -> (i1, i1, !acir.struct<"src_payloads_25ebcaa7.Completion">) {
        %11 = acir.cell.read %7 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc275)
        %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc276)
        acir.cell.write %8, %false_0 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc277)
        %12 = acir.extract %11 {acir.id = "v26", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc278)
        %13 = acir.extract %11 {acir.id = "v27", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc279)
        %14 = acir.extract %11 {acir.id = "v28", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc280)
        %15 = acir.extract %14 {acir.id = "v29", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc280)
        %16 = acir.extract %11 {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc281)
        %17 = acir.extract %16 {acir.id = "v31", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc281)
        %18 = arith.muli %15, %17 {acir.id = "v32", acir.source_type = "u32"} : i32 loc(#loc280)
        %19 = acir.aggregate(%12, %13, %18) {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc282)
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
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc283)
      %1 = acir.ref @top.multiplier::@top.multiplier.request {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %1, %0 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc284)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc285)
      %2 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %c2_i32 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc286)
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc287)
      %3 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      acir.cell.write %3, %true_0 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc288)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.multiplier_body", owner = "top.multiplier", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "bool", role = "observe"} : !acir.cell<i1> -> i1 loc(#loc289)
      %2 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %3 = acir.ref @top::@top.mul_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %4 = scf.if %1 -> (i1) {
        %5 = acir.rule.call @top.multiplier::@top.multiplier_body.0.call.9.3() {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.multiplier", instance = "top.multiplier", interface = "Module:top.multiplier:advance", output_binding = @top::@top.mul_done, role = "rule", rule = "advance"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc290)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        acir.rule.call @top.multiplier::@top.multiplier_body.0.call.10.7(%3) {acir.id = "v8", acir.source_type = "void", contract = "Module:top.multiplier", instance = "top.multiplier", interface = "Module:top.multiplier:accept", role = "rule", rule = "accept"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () loc(#loc291)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
        %9 = acir.queue.peek %arg0 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc292)
        scf.yield %9 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      } else {
        %9 = acir.queue.peek %arg1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc293)
        scf.yield %9 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      }
      %2 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc294)
      %3 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc295)
      acir.rule.call @top.rob::@top.cdb_body.0.call.11.7.0.call.11.16(%2, %3) {acir.id = "v17", acir.source_type = "void", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc296)
      %4 = acir.extract %1 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc297)
      %5 = acir.extract %1 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc298)
      acir.rule.call @top.left::@top.cdb_body.0.call.11.7.0.call.16.19(%4, %5) {acir.id = "v20", acir.source_type = "void", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:wakeup", role = "rule", rule = "wakeup"} : (i32, i32) -> () loc(#loc299)
      %6 = acir.extract %1 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc300)
      %7 = acir.extract %1 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc301)
      acir.rule.call @top.right::@top.cdb_body.0.call.11.7.0.call.17.22(%6, %7) {acir.id = "v23", acir.source_type = "void", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:wakeup", role = "rule", rule = "wakeup"} : (i32, i32) -> () loc(#loc302)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %8 = "acir.optional"(%1, %true_0) {acir.source_type = "src_payloads_25ebcaa7.Completion"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cdb_body", owner = "top.cdb", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %2 = acir.queue.empty %1 {acir.id = "v4", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc303)
      %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc304)
      %4 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %5 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %6 = acir.rule.call @top.cdb::@top.cdb_body.0.call.11.7(%4, %5, %3) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.cdb", instance = "top.cdb", interface = "Module:top.cdb:broadcast", output_binding = @top::@top.event, role = "rule", rule = "broadcast"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc305)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.dispatch_body.0.call.19.12 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () {acir.function = {contract = "Module:top.dispatch", definition = "dispatch", instance = "top.dispatch", interface = "Module:top.dispatch:dispatch", kind = "rule", name = "top.dispatch_body.0.call.19.12", output_type = "void", owner = "top.dispatch", params = [{id = "v1", name = "message", targets = ["top.source"], type = "queue<src_payloads_25ebcaa7.MicroOp>"}], public = true, result = "void", rule = "dispatch"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.MicroOp", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc306)
      %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc307)
      %2 = acir.rule.call @top.rf::@top.dispatch_body.0.call.19.12.0.call.19.6(%1) {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:operand", role = "rule", rule = "operand"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc308)
      %3 = acir.extract %0 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc309)
      %4 = acir.rule.call @top.rf::@top.dispatch_body.0.call.19.12.0.call.19.6(%3) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:operand", role = "rule", rule = "operand"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc310)
      %5 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc311)
      %6 = acir.rule.call @top.rob::@top.dispatch_body.0.call.19.12.0.call.25.10(%5) {acir.id = "v11", acir.source_type = "u32", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:allocate", role = "rule", rule = "allocate"} : (i32) -> i32 loc(#loc96)
      %7 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc312)
      acir.rule.call @top.rf::@top.dispatch_body.0.call.19.12.0.call.35.12(%7, %6) {acir.id = "v13", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:rename", role = "rule", rule = "rename"} : (i32, i32) -> () loc(#loc313)
      %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc314)
      %8 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc315)
      %9 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc316)
      %10 = acir.aggregate(%true_0, %8, %6, %9, %2, %4) {acir.id = "v17", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc317)
      %11 = acir.rule.call @top.left::@top.scheduler_body.0.call.0.1() {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc318)
      %12 = acir.extract %11 {acir.id = "v19", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc318)
      %13 = acir.unary %12 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc319)
      scf.if %13 {
        acir.rule.call @top.left::@top.dispatch_body.0.call.19.12.0.call.36.20(%10) {acir.id = "v21", acir.source_type = "void", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:place", role = "rule", rule = "place"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc228)
      } else {
        acir.rule.call @top.right::@top.dispatch_body.0.call.19.12.0.call.38.21(%10) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:place", role = "rule", rule = "place"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc232)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.dispatch_body", owner = "top.dispatch", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %2 = acir.queue.empty %1 {acir.id = "v4", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc320)
      %3 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %4 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %5 = scf.if %2 -> (i1) {
        %7 = acir.queue.empty %4 {acir.id = "v7", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc321)
        scf.yield %7 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %6 = acir.ref @top::@top.source {acir.source_type = "queue<src_payloads_25ebcaa7.MicroOp>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
      scf.if %5 {
        acir.rule.call @top.dispatch::@top.dispatch_body.0.call.19.12(%6) {acir.id = "v13", acir.source_type = "void", contract = "Module:top.dispatch", instance = "top.dispatch", interface = "Module:top.dispatch:dispatch", role = "rule", rule = "dispatch"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () loc(#loc322)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.commit_body.0.call.41.1 : () -> () {acir.function = {contract = "Module:top.commit", definition = "commit", instance = "top.commit", interface = "Module:top.commit:commit", kind = "rule", name = "top.commit_body.0.call.41.1", output_type = "void", owner = "top.commit", params = [], public = true, result = "void", rule = "commit"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.rule.call @top.rob::@top.commit_body.0.call.41.1.0.call.41.3() {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:retire", role = "rule", rule = "retire"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc323)
      acir.rule.call @top.rf::@top.commit_body.0.call.41.1.0.call.50.4(%0) {acir.id = "v5", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:commit", role = "rule", rule = "commit"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc324)
      %1 = acir.ref @top.commit::@top.commit.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc325)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc326)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc325)
      %4 = acir.ref @top.commit::@top.commit.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %4, %3 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc327)
      %5 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc328)
      %6 = acir.ref @top.commit::@top.commit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %6, %5 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc329)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.commit_body", owner = "top.commit", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      acir.rule.call @top.commit::@top.commit_body.0.call.41.1() {acir.id = "v2", acir.source_type = "void", contract = "Module:top.commit", instance = "top.commit", interface = "Module:top.commit:commit", role = "rule", rule = "commit"} : () -> () loc(#loc330)
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
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
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
    acir.rule @top.audit_body.0.call.52.2 : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () {acir.function = {contract = "Module:top.audit", definition = "observe", instance = "top.audit", interface = "Module:top.audit:observe", kind = "rule", name = "top.audit_body.0.call.52.2", output_type = "void", owner = "top.audit", params = [{id = "v1", name = "value", targets = ["top.event"], type = "signal<src_payloads_25ebcaa7.Completion>"}], public = true, result = "void", rule = "observe"}} body {
    ^bb0(%arg0: !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.signal.get %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "observe"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc331)
      %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc332)
      %2 = acir.ref @top.audit::@top.audit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %1 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc333)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.audit_body", owner = "top.audit", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.event {acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.rule.call @top.audit::@top.audit_body.0.call.52.2(%0) {acir.id = "v3", acir.source_type = "void", contract = "Module:top.audit", instance = "top.audit", interface = "Module:top.audit:observe", role = "rule", rule = "observe"} : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () loc(#loc334)
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
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":93:40)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":94:40)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:68)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:71)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:74)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:77)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:80)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:83)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:86)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:89)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":161:67)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:66)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:69)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:72)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:75)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:78)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:81)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:84)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:87)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:65)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:68)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:71)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:74)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:77)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:80)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:83)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:86)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:89)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:67)
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
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:23)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:17)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:27)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:21)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:12)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:13)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:23)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:14)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":136:13)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:23)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:14)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":138:13)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:13)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:23)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:17)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:27)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:21)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:12)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":123:20)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:23)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:14)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":125:20)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":126:23)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":126:14)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:20)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:16)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:16)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:23)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:30)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:20)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":106:15)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:23)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:17)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:27)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":108:21)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":108:12)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":109:13)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:23)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:14)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":111:13)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":112:23)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":112:14)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":113:13)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":115:13)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:22)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:16)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:9)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:15)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:23)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:17)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:27)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:21)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:12)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:21)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:23)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:14)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:21)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":150:23)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":150:14)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":151:21)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":153:21)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:22)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:16)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":154:9)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":64:17)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:12)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:33)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:29)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:50)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:13)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:26)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":67:13)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:13)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:20)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:16)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":58:20)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:26)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:32)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:48)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:17)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:9)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":76:17)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:16)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:32)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:48)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":79:20)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":81:17)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:17)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":82:9)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:24)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:16)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:9)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":168:21)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":168:12)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:28)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:34)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:37)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:20)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:15)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:19)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:12)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":172:28)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":172:39)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":172:20)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":173:17)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:12)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:28)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:44)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:28)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:39)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:20)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:24)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:36)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:16)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:21)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:16)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:9)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:18)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:12)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:13)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":186:12)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":186:24)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":187:35)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":187:13)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":187:23)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":188:35)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":188:13)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":188:23)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":46:16)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":21:17)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:16)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:32)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:55)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:75)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:20)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:20)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:17)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:16)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":27:9)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:17)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:19)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:12)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:20)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:16)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":35:40)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:17)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:37)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:29)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:27)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:20)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:16)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":38:40)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:17)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:37)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":39:29)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":40:27)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":42:13)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:20)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:16)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":15:20)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":17:9)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:16)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:30)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:27)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:30)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:20)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:27)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:16)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":25:13)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:13)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:19)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:31)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:47)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:19)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:29)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:39)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:39)
#loc251 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc252 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:39)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:39)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:38)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:51)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:59)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:38)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:51)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:59)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:17)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:45)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":45:17)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:27)
#loc265 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:38)
#loc266 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:48)
#loc267 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:66)
#loc268 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:16)
#loc269 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc270 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:12)
#loc271 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:25)
#loc272 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:25)
#loc273 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:37)
#loc274 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:13)
#loc275 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":70:17)
#loc276 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:18)
#loc277 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:9)
#loc278 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:27)
#loc279 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:38)
#loc280 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:48)
#loc281 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:66)
#loc282 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:16)
#loc283 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:19)
#loc284 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:9)
#loc285 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:21)
#loc286 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:9)
#loc287 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:18)
#loc288 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:9)
#loc289 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:16)
#loc290 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:20)
#loc291 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":79:13)
#loc292 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":43:22)
#loc293 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc294 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:22)
#loc295 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:34)
#loc296 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:9)
#loc297 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc298 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc299 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc300 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc301 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc302 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc303 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc304 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc305 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:16)
#loc306 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc307 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc308 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc309 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc310 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc311 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc312 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc313 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc314 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:23)
#loc315 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc316 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc317 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc318 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc319 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:12)
#loc320 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc321 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc322 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc323 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":64:17)
#loc324 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:9)
#loc325 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:19)
#loc326 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:29)
#loc327 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc328 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:20)
#loc329 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc330 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":70:9)
#loc331 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":80:17)
#loc332 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:20)
#loc333 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:9)
#loc334 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":84:9)
