module attributes {acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, exports = {}, format_version = 8 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", type = "bool"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default, name = "lhs", type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", type = "bool"}, {default = false, name = "done", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}]}}} {
  func.func @ac_init_top.source_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.source_cap", params = [], result = "u32"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc1)
    return %c4_i32 : i32
  }
  func.func @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> attributes {acir.function = {kind = "helper", name = "ac_init_top.source", params = [], result = "vector<src_payloads_25ebcaa7.MicroOp>"}} {
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32 loc(#loc2)
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = acir.aggregate(%c1_i32, %c1_i32, %c2_i32, %c3_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc3)
    %1 = acir.aggregate(%c0_i32, %c4_i32, %c1_i32, %c2_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc4)
    %2 = acir.aggregate(%c0_i32, %c5_i32, %c2_i32, %c3_i32) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc5)
    %3 = acir.aggregate(%c0_i32, %c6_i32, %c4_i32, %c5_i32) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc6)
    %4 = acir.aggregate(%0, %1, %2, %3) {acir.id = "v22", acir.source_type = "vector<src_payloads_25ebcaa7.MicroOp>"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> loc(#loc7)
    return %4 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
  }
  func.func @ac_init_top.add_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.add_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">> attributes {acir.function = {kind = "helper", name = "ac_init_top.add_requests", params = [], result = "vector<src_payloads_25ebcaa7.Entry>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Entry>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
  }
  func.func @ac_init_top.mul_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.mul_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">> attributes {acir.function = {kind = "helper", name = "ac_init_top.mul_requests", params = [], result = "vector<src_payloads_25ebcaa7.Entry>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Entry>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
  }
  func.func @ac_init_top.add_done_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.add_done_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">> attributes {acir.function = {kind = "helper", name = "ac_init_top.add_done", params = [], result = "vector<src_payloads_25ebcaa7.Completion>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Completion>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
  }
  func.func @ac_init_top.mul_done_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.mul_done_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">> attributes {acir.function = {kind = "helper", name = "ac_init_top.mul_done", params = [], result = "vector<src_payloads_25ebcaa7.Completion>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Completion>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
  }
  func.func @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot0.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v38", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot1.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot2.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot3.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v59", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.head() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.head", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc9)
    return %c1_i32 : i32
  }
  func.func @ac_init_top.rob.tail() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.tail", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc10)
    return %c1_i32 : i32
  }
  func.func @ac_init_top.rf.registers() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32 loc(#loc11)
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc12)
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c6_i32, %c7_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v73", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc13)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_top.rf.renamed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.renamed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v83", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc14)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_top.rf.committed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.committed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v93", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc15)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.left.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc16)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v99", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %1 = acir.aggregate() {acir.id = "v100", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v101", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc16)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.right.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc16)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v107", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %1 = acir.aggregate() {acir.id = "v108", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v109", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc16)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.multiplier.active() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.multiplier.active", params = [], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc17)
    return %false : i1
  }
  func.func @ac_init_top.multiplier.remaining() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.multiplier.remaining", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc18)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.multiplier.request", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc19)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v119", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc19)
    %1 = acir.aggregate() {acir.id = "v120", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc19)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v121", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc19)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.commit.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.commit.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc20)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.commit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.commit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc21)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.audit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.audit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc22)
    return %c0_i32 : i32
  }
  func.func @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "top.left", kind = "guard", name = "top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc23)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc24)
    %3 = scf.if %2 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc25)
      %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc25)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    %4 = scf.if %3 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
      %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    %5 = scf.if %4 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc27)
      %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc27)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    return %5 : i1 loc(#loc28)
  }
  func.func @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "top.right", kind = "guard", name = "top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc23)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc24)
    %3 = scf.if %2 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc25)
      %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc25)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    %4 = scf.if %3 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
      %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    %5 = scf.if %4 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc27)
      %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc27)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    return %5 : i1 loc(#loc28)
  }
  func.func @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "top.left", kind = "guard", name = "top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc23)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc24)
    %3 = scf.if %2 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc25)
      %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc25)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    %4 = scf.if %3 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
      %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    %5 = scf.if %4 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc27)
      %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc27)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    return %5 : i1 loc(#loc28)
  }
  func.func @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "top.right", kind = "guard", name = "top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc23)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc24)
    %3 = scf.if %2 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc25)
      %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc25)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    %4 = scf.if %3 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc26)
      %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc26)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    %5 = scf.if %4 -> (i1) {
      %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc27)
      %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc27)
      scf.yield %7 : i1
    } else {
      scf.yield %false : i1
    }
    return %5 : i1 loc(#loc28)
  }
  func.func @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "room", instance = "top.rob", kind = "guard", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc29)
    %0 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc30)
    %2 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %3 = acir.cell.read %2 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc31)
    %4 = arith.subi %1, %3 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc30)
    %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc30)
    return %5 : i1 loc(#loc32)
  }
  func.func @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.rob.slot0", kind = "guard", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc33)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc33)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc34)
    return %3 : i1 loc(#loc35)
  }
  func.func @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.rob.slot1", kind = "guard", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc33)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc33)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc34)
    return %3 : i1 loc(#loc35)
  }
  func.func @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.rob.slot2", kind = "guard", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc33)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc33)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc34)
    return %3 : i1 loc(#loc35)
  }
  func.func @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.rob.slot3", kind = "guard", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc33)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc33)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc34)
    return %3 : i1 loc(#loc35)
  }
  func.func @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "top.left", kind = "guard", name = "top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc36)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc37)
    return %3 : i1 loc(#loc38)
  }
  func.func @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "top.right", kind = "guard", name = "top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc36)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc37)
    return %3 : i1 loc(#loc38)
  }
  func.func @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "top.rob.slot0", kind = "guard", name = "top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc39)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc40)
    %3 = scf.if %2 -> (i1) {
      %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc41)
      scf.yield %5 : i1
    } else {
      scf.yield %false : i1
    }
    %4 = scf.if %3 -> (i1) {
      %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc42)
      %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc42)
      scf.yield %6 : i1
    } else {
      scf.yield %false : i1
    }
    return %4 : i1 loc(#loc43)
  }
  func.func @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "top.rob.slot1", kind = "guard", name = "top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc39)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc40)
    %3 = scf.if %2 -> (i1) {
      %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc41)
      scf.yield %5 : i1
    } else {
      scf.yield %false : i1
    }
    %4 = scf.if %3 -> (i1) {
      %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc42)
      %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc42)
      scf.yield %6 : i1
    } else {
      scf.yield %false : i1
    }
    return %4 : i1 loc(#loc43)
  }
  func.func @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "top.rob.slot2", kind = "guard", name = "top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc39)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc40)
    %3 = scf.if %2 -> (i1) {
      %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc41)
      scf.yield %5 : i1
    } else {
      scf.yield %false : i1
    }
    %4 = scf.if %3 -> (i1) {
      %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc42)
      %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc42)
      scf.yield %6 : i1
    } else {
      scf.yield %false : i1
    }
    return %4 : i1 loc(#loc43)
  }
  func.func @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "top.rob.slot3", kind = "guard", name = "top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc39)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc40)
    %3 = scf.if %2 -> (i1) {
      %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc41)
      scf.yield %5 : i1
    } else {
      scf.yield %false : i1
    }
    %4 = scf.if %3 -> (i1) {
      %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc42)
      %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc42)
      scf.yield %6 : i1
    } else {
      scf.yield %false : i1
    }
    return %4 : i1 loc(#loc43)
  }
  acir.module @top attributes {acir.instance = {definition = "src_model_3cf90884.OutOfOrder", inputs = {}, members = {}, name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.source : !acir.struct<"src_payloads_25ebcaa7.MicroOp"> {capacity_fn = "ac_init_top.source_cap", initial_fn = "ac_init_top.source", name = "top.source", type = "src_payloads_25ebcaa7.MicroOp"}
    acir.queue @top.add_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity_fn = "ac_init_top.add_requests_cap", initial_fn = "ac_init_top.add_requests", name = "top.add_requests", type = "src_payloads_25ebcaa7.Entry"}
    acir.queue @top.mul_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity_fn = "ac_init_top.mul_requests_cap", initial_fn = "ac_init_top.mul_requests", name = "top.mul_requests", type = "src_payloads_25ebcaa7.Entry"}
    acir.queue @top.add_done : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity_fn = "ac_init_top.add_done_cap", initial_fn = "ac_init_top.add_done", name = "top.add_done", type = "src_payloads_25ebcaa7.Completion"}
    acir.queue @top.mul_done : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity_fn = "ac_init_top.mul_done_cap", initial_fn = "ac_init_top.mul_done", name = "top.mul_done", type = "src_payloads_25ebcaa7.Completion"}
    acir.signal @top.event : !acir.struct<"src_payloads_25ebcaa7.Completion"> {name = "top.event", type = "src_payloads_25ebcaa7.Completion"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rob attributes {acir.instance = {definition = "src_state_7e811fd9.ReorderBuffer", inputs = {}, members = {}, name = "top.rob", parent = "top", rules = ["allocate", "complete", "has_room", "lookup", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "has_room", "lookup", "retire"]}
    acir.cell @top.rob.head : i32 {initial_fn = "ac_init_top.rob.head", name = "top.rob.head", type = "u32"}
    acir.cell @top.rob.tail : i32 {initial_fn = "ac_init_top.rob.tail", name = "top.rob.tail", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.15.6.0.call.15.16 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob", definition = "complete", instance = "top.rob", interface = "Module:top.rob:complete", kind = "rule", name = "top.cdb_body.0.call.15.6.0.call.15.16", output_type = "void", owner = "top.rob", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc44)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc45)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc46)
      %1 = arith.andi %0, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc46)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc47)
      scf.if %2 {
        acir.rule.call @top.rob.slot0::@top.cdb_body.0.call.15.6.0.call.15.16.0.call.15.11(%arg0, %arg1) {acir.id = "v12", acir.source_type = "void", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc48)
      } else {
        %3 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc49)
        scf.if %3 {
          acir.rule.call @top.rob.slot1::@top.cdb_body.0.call.15.6.0.call.15.16.0.call.16.15(%arg0, %arg1) {acir.id = "v16", acir.source_type = "void", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc50)
        } else {
          %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc51)
          scf.if %4 {
            acir.rule.call @top.rob.slot2::@top.cdb_body.0.call.15.6.0.call.15.16.0.call.17.19(%arg0, %arg1) {acir.id = "v20", acir.source_type = "void", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc52)
          } else {
            acir.rule.call @top.rob.slot3::@top.cdb_body.0.call.15.6.0.call.15.16.0.call.18.20(%arg0, %arg1) {acir.id = "v21", acir.source_type = "void", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc53)
          }
        }
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "lookup", instance = "top.rob", interface = "Module:top.rob:lookup", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "lookup"}} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc54)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc55)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc56)
      %1 = arith.andi %0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc56)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc57)
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.23.10() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc58)
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v12", acir.source_type = "void"} loc(#loc59)
    ^bb2:  // pred: ^bb0
      %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc60)
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.24.15() {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc61)
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v17", acir.source_type = "void"} loc(#loc62)
    ^bb4:  // pred: ^bb2
      %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc63)
      cf.cond_br %6, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %7 = acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.25.20() {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc64)
      acir.return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v22", acir.source_type = "void"} loc(#loc65)
    ^bb6:  // pred: ^bb4
      %8 = acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.26.31() {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc66)
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v33", acir.source_type = "void"} loc(#loc67)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "lookup", instance = "top.rob", interface = "Module:top.rob:lookup", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "lookup"}} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc54)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc55)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc56)
      %1 = arith.andi %0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc56)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc57)
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.29.10() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc58)
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v12", acir.source_type = "void"} loc(#loc59)
    ^bb2:  // pred: ^bb0
      %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc60)
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.30.15() {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc61)
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v17", acir.source_type = "void"} loc(#loc62)
    ^bb4:  // pred: ^bb2
      %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc63)
      cf.cond_br %6, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %7 = acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.31.20() {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc64)
      acir.return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v22", acir.source_type = "void"} loc(#loc65)
    ^bb6:  // pred: ^bb4
      %8 = acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.32.31() {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc66)
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v33", acir.source_type = "void"} loc(#loc67)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.35.9 : (i32) -> i32 {acir.function = {contract = "Module:top.rob", definition = "allocate", guard_info = {name = "room", source = "function"}, instance = "top.rob", interface = "Module:top.rob:allocate", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.35.9", output_type = "u32", owner = "top.rob", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], public = true, result = "u32", rule = "allocate"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob", role = "guard"} : (i32) -> i1 loc(#loc68)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc69)
    } body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc70)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc71)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc72)
      %2 = arith.subi %1, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc73)
      %3 = arith.andi %2, %c3_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc73)
      %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc74)
      scf.if %4 {
        acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13(%1, %arg0) {acir.id = "v14", acir.source_type = "void", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc75)
      } else {
        %6 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc76)
        scf.if %6 {
          acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17(%1, %arg0) {acir.id = "v18", acir.source_type = "void", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc77)
        } else {
          %7 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v20", acir.source_type = "bool"} : i32 loc(#loc78)
          scf.if %7 {
            acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21(%1, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc79)
          } else {
            acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22(%1, %arg0) {acir.id = "v23", acir.source_type = "void", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc80)
          }
        }
      }
      %5 = arith.addi %1, %c1_i32 {acir.id = "v40", acir.source_type = "u32"} : i32 loc(#loc81)
      acir.cell.write %0, %5 {acir.id = "v42", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc82)
      acir.return %1 : i32 {acir.id = "v43", acir.source_type = "void"} loc(#loc83)
    }
    acir.rule @top.commit_body.0.call.52.0.0.call.52.2 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "retire", instance = "top.rob", interface = "Module:top.rob:retire", kind = "rule", name = "top.commit_body.0.call.52.0.0.call.52.2", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} body {
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc84)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc85)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc86)
      %2 = arith.subi %1, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc87)
      %3 = arith.andi %2, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc87)
      %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc88)
      %5 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.rule.call @top.rob.slot0::@top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11(%1) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc89)
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        %7 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc90)
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
          %9 = acir.rule.call @top.rob.slot1::@top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15(%1) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc91)
          scf.yield %9 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        } else {
          %9 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc92)
          %10 = scf.if %9 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
            %11 = acir.rule.call @top.rob.slot2::@top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19(%1) {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc93)
            scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          } else {
            %11 = acir.rule.call @top.rob.slot3::@top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20(%1) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc94)
            scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          }
          scf.yield %10 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        }
        scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %6 = arith.addi %1, %c1_i32 {acir.id = "v38", acir.source_type = "u32"} : i32 loc(#loc95)
      acir.cell.write %0, %6 {acir.id = "v40", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc96)
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v41", acir.source_type = "void"} loc(#loc97)
    }
  }
  acir.module @top.rob.slot0 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", inputs = {}, members = {}, name = "top.rob.slot0", parent = "top.rob", rules = ["allocate", "complete", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "peek", "retire"]}
    acir.cell @top.rob.slot0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot0.state", name = "top.rob.slot0.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.15.6.0.call.15.16.0.call.15.11 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot0", definition = "complete", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:complete", kind = "rule", name = "top.cdb_body.0.call.15.6.0.call.15.16.0.call.15.11", output_type = "void", owner = "top.rob.slot0", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc98)
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc99)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc100)
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc101)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc102)
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc102)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc103)
        %6 = acir.update %5, %arg1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc104)
        acir.cell.write %0, %6 {acir.id = "v29", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.23.10 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot0", definition = "peek", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.23.10", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot0", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc107)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.29.10 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot0", definition = "peek", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.29.10", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot0", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc107)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot0", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot0", interface = "Module:top.rob.slot0:allocate", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13", output_type = "void", owner = "top.rob.slot0", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "top.rob.slot0", role = "guard"} : (i32, i32) -> i1 loc(#loc75)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc108)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc109)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc110)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc111)
      %0 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc112)
      %1 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc113)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot0", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot0", interface = "Module:top.rob.slot0:retire", kind = "rule", name = "top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot0", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob.slot0", role = "guard"} : (i32) -> i1 loc(#loc89)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc114)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc115)
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.cell.write %0, %2 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc117)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v15", acir.source_type = "void"} loc(#loc118)
    }
  }
  acir.module @top.rob.slot1 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", inputs = {}, members = {}, name = "top.rob.slot1", parent = "top.rob", rules = ["allocate", "complete", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "peek", "retire"]}
    acir.cell @top.rob.slot1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot1.state", name = "top.rob.slot1.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.15.6.0.call.15.16.0.call.16.15 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot1", definition = "complete", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:complete", kind = "rule", name = "top.cdb_body.0.call.15.6.0.call.15.16.0.call.16.15", output_type = "void", owner = "top.rob.slot1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc98)
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc99)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc100)
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc101)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc102)
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc102)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc103)
        %6 = acir.update %5, %arg1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc104)
        acir.cell.write %0, %6 {acir.id = "v29", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.24.15 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot1", definition = "peek", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.24.15", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot1", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc107)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.30.15 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot1", definition = "peek", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.30.15", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot1", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc107)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot1", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot1", interface = "Module:top.rob.slot1:allocate", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17", output_type = "void", owner = "top.rob.slot1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "top.rob.slot1", role = "guard"} : (i32, i32) -> i1 loc(#loc77)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc108)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc109)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc110)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc111)
      %0 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc112)
      %1 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc113)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot1", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot1", interface = "Module:top.rob.slot1:retire", kind = "rule", name = "top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob.slot1", role = "guard"} : (i32) -> i1 loc(#loc91)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc114)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc115)
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.cell.write %0, %2 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc117)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v15", acir.source_type = "void"} loc(#loc118)
    }
  }
  acir.module @top.rob.slot2 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", inputs = {}, members = {}, name = "top.rob.slot2", parent = "top.rob", rules = ["allocate", "complete", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "peek", "retire"]}
    acir.cell @top.rob.slot2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot2.state", name = "top.rob.slot2.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.15.6.0.call.15.16.0.call.17.19 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot2", definition = "complete", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:complete", kind = "rule", name = "top.cdb_body.0.call.15.6.0.call.15.16.0.call.17.19", output_type = "void", owner = "top.rob.slot2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc98)
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc99)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc100)
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc101)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc102)
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc102)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc103)
        %6 = acir.update %5, %arg1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc104)
        acir.cell.write %0, %6 {acir.id = "v29", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.25.20 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot2", definition = "peek", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.25.20", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot2", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc107)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.31.20 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot2", definition = "peek", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.31.20", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot2", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc107)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot2", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot2", interface = "Module:top.rob.slot2:allocate", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21", output_type = "void", owner = "top.rob.slot2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "top.rob.slot2", role = "guard"} : (i32, i32) -> i1 loc(#loc79)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc108)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc109)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc110)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc111)
      %0 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc112)
      %1 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc113)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot2", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot2", interface = "Module:top.rob.slot2:retire", kind = "rule", name = "top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob.slot2", role = "guard"} : (i32) -> i1 loc(#loc93)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc114)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc115)
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.cell.write %0, %2 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc117)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v15", acir.source_type = "void"} loc(#loc118)
    }
  }
  acir.module @top.rob.slot3 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", inputs = {}, members = {}, name = "top.rob.slot3", parent = "top.rob", rules = ["allocate", "complete", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "peek", "retire"]}
    acir.cell @top.rob.slot3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot3.state", name = "top.rob.slot3.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.15.6.0.call.15.16.0.call.18.20 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot3", definition = "complete", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:complete", kind = "rule", name = "top.cdb_body.0.call.15.6.0.call.15.16.0.call.18.20", output_type = "void", owner = "top.rob.slot3", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc98)
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc99)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc100)
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc101)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc102)
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc102)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc103)
        %6 = acir.update %5, %arg1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc104)
        acir.cell.write %0, %6 {acir.id = "v29", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.26.31 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot3", definition = "peek", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33.0.call.26.31", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot3", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc107)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.32.31 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot3", definition = "peek", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33.0.call.32.31", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot3", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc107)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot3", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot3", interface = "Module:top.rob.slot3:allocate", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22", output_type = "void", owner = "top.rob.slot3", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "top.rob.slot3", role = "guard"} : (i32, i32) -> i1 loc(#loc80)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc108)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc109)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc110)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc111)
      %0 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc112)
      %1 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc113)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot3", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot3", interface = "Module:top.rob.slot3:retire", kind = "rule", name = "top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot3", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob.slot3", role = "guard"} : (i32) -> i1 loc(#loc94)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc114)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc115)
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.cell.write %0, %2 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc117)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v15", acir.source_type = "void"} loc(#loc118)
    }
  }
  acir.module @top.rf attributes {acir.instance = {definition = "src_state_7e811fd9.RegisterFile", inputs = {}, members = {}, name = "top.rf", parent = "top", rules = ["commit", "operand", "rename"]}} {
    acir.contract {relations = [], rules = ["commit", "operand", "rename"]}
    acir.cell @top.rf.registers : !acir.array<i32, 8> {initial_fn = "ac_init_top.rf.registers", name = "top.rf.registers", type = "array<u32,8>"}
    acir.cell @top.rf.renamed : !acir.array<i32, 8> {initial_fn = "ac_init_top.rf.renamed", name = "top.rf.renamed", type = "array<u32,8>"}
    acir.cell @top.rf.committed : !acir.array<i32, 8> {initial_fn = "ac_init_top.rf.committed", name = "top.rf.committed", type = "array<u32,8>"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.dispatch_body.0.call.23.13.0.call.23.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:top.rf", definition = "operand", instance = "top.rf", interface = "Module:top.rf:operand", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.23.5", output_type = "src_payloads_25ebcaa7.Operand", owner = "top.rf", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Operand", rule = "operand"}} body {
    ^bb0(%arg0: i32):
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc119)
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc120)
      acir.check %0 {acir.id = "v6", acir.source_type = "void"} loc(#loc121)
      %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc122)
      cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      %2 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc123)
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v14", acir.source_type = "void"} loc(#loc124)
    ^bb2(%3: i32):  // pred: ^bb0
      %4 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %5 = acir.cell.read %4 {acir.id = "v18", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc125)
      %6 = acir.extract %5[%3] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc125)
      %7 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %8 = acir.cell.read %7 {acir.id = "v21", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc126)
      %9 = acir.extract %8[%3] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc126)
      %10 = arith.cmpi eq, %6, %9 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc127)
      cf.cond_br %10, ^bb3, ^bb4(%6 : i32)
    ^bb3:  // pred: ^bb2
      %11 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %12 = acir.cell.read %11 {acir.id = "v27", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc128)
      %13 = acir.extract %12[%3] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc128)
      %14 = acir.aggregate(%true, %6, %13) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc129)
      acir.return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v30", acir.source_type = "void"} loc(#loc130)
    ^bb4(%15: i32):  // pred: ^bb2
      %16 = acir.rule.call @top.rob::@top.dispatch_body.0.call.23.13.0.call.23.5.0.call.23.33(%15) {acir.id = "v34", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:lookup", role = "rule", rule = "lookup"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc131)
      %17 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc132)
      cf.cond_br %17, ^bb5, ^bb6(%false : i1)
    ^bb5:  // pred: ^bb4
      %18 = acir.extract %16 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc133)
      cf.br ^bb6(%18 : i1)
    ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
      cf.cond_br %19, ^bb7, ^bb8(%false : i1)
    ^bb7:  // pred: ^bb6
      %20 = acir.extract %16 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc134)
      %21 = arith.cmpi eq, %20, %15 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc134)
      cf.br ^bb8(%21 : i1)
    ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
      cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
    ^bb9:  // pred: ^bb8
      %23 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc135)
      %24 = acir.aggregate(%true, %15, %23) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc136)
      acir.return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v53", acir.source_type = "void"} loc(#loc137)
    ^bb10(%25: i32):  // pred: ^bb8
      %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc138)
      acir.return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v61", acir.source_type = "void"} loc(#loc139)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.29.7 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:top.rf", definition = "operand", instance = "top.rf", interface = "Module:top.rf:operand", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.29.7", output_type = "src_payloads_25ebcaa7.Operand", owner = "top.rf", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Operand", rule = "operand"}} body {
    ^bb0(%arg0: i32):
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc119)
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc120)
      acir.check %0 {acir.id = "v6", acir.source_type = "void"} loc(#loc121)
      %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc122)
      cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      %2 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc123)
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v14", acir.source_type = "void"} loc(#loc124)
    ^bb2(%3: i32):  // pred: ^bb0
      %4 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %5 = acir.cell.read %4 {acir.id = "v18", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc125)
      %6 = acir.extract %5[%3] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc125)
      %7 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %8 = acir.cell.read %7 {acir.id = "v21", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc126)
      %9 = acir.extract %8[%3] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc126)
      %10 = arith.cmpi eq, %6, %9 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc127)
      cf.cond_br %10, ^bb3, ^bb4(%6 : i32)
    ^bb3:  // pred: ^bb2
      %11 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %12 = acir.cell.read %11 {acir.id = "v27", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc128)
      %13 = acir.extract %12[%3] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc128)
      %14 = acir.aggregate(%true, %6, %13) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc129)
      acir.return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v30", acir.source_type = "void"} loc(#loc130)
    ^bb4(%15: i32):  // pred: ^bb2
      %16 = acir.rule.call @top.rob::@top.dispatch_body.0.call.23.13.0.call.29.7.0.call.29.33(%15) {acir.id = "v34", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:lookup", role = "rule", rule = "lookup"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc131)
      %17 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc132)
      cf.cond_br %17, ^bb5, ^bb6(%false : i1)
    ^bb5:  // pred: ^bb4
      %18 = acir.extract %16 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc133)
      cf.br ^bb6(%18 : i1)
    ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
      cf.cond_br %19, ^bb7, ^bb8(%false : i1)
    ^bb7:  // pred: ^bb6
      %20 = acir.extract %16 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc134)
      %21 = arith.cmpi eq, %20, %15 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc134)
      cf.br ^bb8(%21 : i1)
    ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
      cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
    ^bb9:  // pred: ^bb8
      %23 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc135)
      %24 = acir.aggregate(%true, %15, %23) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc136)
      acir.return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v53", acir.source_type = "void"} loc(#loc137)
    ^bb10(%25: i32):  // pred: ^bb8
      %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc138)
      acir.return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v61", acir.source_type = "void"} loc(#loc139)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.45.11 : (i32, i32) -> () {acir.function = {contract = "Module:top.rf", definition = "rename", instance = "top.rf", interface = "Module:top.rf:rename", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.45.11", output_type = "void", owner = "top.rf", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}, {consume = false, id = "v2", name = "tag", targets = [], type = "u32"}], public = true, result = "void", rule = "rename"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc140)
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc141)
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc142)
      acir.check %0 {acir.id = "v7", acir.source_type = "void"} loc(#loc143)
      %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc144)
      scf.if %1 {
        %2 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
        %3 = acir.cell.read %2 {acir.id = "v13", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc145)
        %4 = acir.cell.read %2 {acir.id = "v17", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc145)
        %5 = acir.update %4, %arg1[%arg0] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc145)
        acir.cell.write %2, %5 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc145)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.52.0.0.call.61.3 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () {acir.function = {contract = "Module:top.rf", definition = "commit", instance = "top.rf", interface = "Module:top.rf:commit", kind = "rule", name = "top.commit_body.0.call.52.0.0.call.61.3", output_type = "void", owner = "top.rf", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc146)
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc147)
      %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc147)
      scf.if %1 {
        %2 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc148)
        %3 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
        %4 = acir.cell.read %3 {acir.id = "v12", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc149)
        %5 = acir.cell.read %3 {acir.id = "v16", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc149)
        %6 = acir.extract %arg0 {acir.id = "v17", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc150)
        %7 = acir.update %5, %2[%6] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc149)
        acir.cell.write %3, %7 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc149)
        %8 = acir.extract %arg0 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc151)
        %9 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
        %10 = acir.cell.read %9 {acir.id = "v25", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc152)
        %11 = acir.cell.read %9 {acir.id = "v29", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc152)
        %12 = acir.extract %arg0 {acir.id = "v30", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc153)
        %13 = acir.update %11, %8[%12] {acir.id = "v31", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc152)
        acir.cell.write %9, %13 {acir.id = "v33", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc152)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.rf_body", owner = "top.rf", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.left attributes {acir.instance = {definition = "src_state_7e811fd9.ReservationStation", inputs = {}, members = {}, name = "top.left", parent = "top", rules = ["peek", "place", "take", "wakeup"]}} {
    acir.contract {relations = [], rules = ["peek", "place", "take", "wakeup"]}
    acir.cell @top.left.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.left.slot", name = "top.left.slot", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.scheduler_body.0.call.0.0 : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.left", definition = "peek", instance = "top.left", interface = "Module:top.left:peek", kind = "rule", name = "top.scheduler_body.0.call.0.0", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.left", params = [], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "peek"}} body {
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc154)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc155)
    }
    acir.rule @top.scheduler_body.0.call.2.92.0.call.2.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.left", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.left", interface = "Module:top.left:take", kind = "rule", name = "top.scheduler_body.0.call.2.92.0.call.2.5", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.left", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.left", role = "guard"} : (i32) -> i1 loc(#loc156)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc157)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc158)
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc159)
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc158)
      acir.cell.write %0, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc160)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v16", acir.source_type = "void"} loc(#loc161)
    }
    acir.rule @top.scheduler_body.0.call.7.93.0.call.7.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.left", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.left", interface = "Module:top.left:take", kind = "rule", name = "top.scheduler_body.0.call.7.93.0.call.7.5", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.left", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.left", role = "guard"} : (i32) -> i1 loc(#loc162)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc157)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc158)
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc159)
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc158)
      acir.cell.write %0, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc160)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v16", acir.source_type = "void"} loc(#loc161)
    }
    acir.rule @top.cdb_body.0.call.15.6.0.call.20.19 : (i32, i32) -> () {acir.function = {contract = "Module:top.left", definition = "wakeup", instance = "top.left", interface = "Module:top.left:wakeup", kind = "rule", name = "top.cdb_body.0.call.15.6.0.call.20.19", output_type = "void", owner = "top.left", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "wakeup"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc163)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc164)
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc165)
        %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc165)
        %6 = acir.unary %5 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc166)
        %7 = scf.if %6 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc167)
          %16 = acir.extract %15 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc167)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc167)
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc168)
          %16 = acir.update %1, %15 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc169)
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %9 = acir.extract %8 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc170)
        %10 = acir.extract %9 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc170)
        %11 = acir.unary %10 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc171)
        %12 = scf.if %11 -> (i1) {
          %15 = acir.extract %8 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc172)
          %16 = acir.extract %15 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc172)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc172)
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %13 = arith.select %12, %true, %7 : i1
        %14 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc173)
          %16 = acir.update %8, %15 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc174)
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %13, %14 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      scf.if %3#0 {
        acir.cell.write %0, %3#1 {acir.id = "v64", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc175)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.46.16 : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.left", definition = "peek", instance = "top.left", interface = "Module:top.left:peek", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.46.16", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.left", params = [], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "peek"}} body {
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc154)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc155)
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.47.20 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:top.left", definition = "place", guard_info = {name = "free", source = "function"}, instance = "top.left", interface = "Module:top.left:place", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.47.20", output_type = "void", owner = "top.left", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "void", rule = "place"}} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = func.call @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.left", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc176)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc177)
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc178)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.right attributes {acir.instance = {definition = "src_state_7e811fd9.ReservationStation", inputs = {}, members = {}, name = "top.right", parent = "top", rules = ["peek", "place", "take", "wakeup"]}} {
    acir.contract {relations = [], rules = ["peek", "place", "take", "wakeup"]}
    acir.cell @top.right.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.right.slot", name = "top.right.slot", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.scheduler_body.0.call.1.1 : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.right", definition = "peek", instance = "top.right", interface = "Module:top.right:peek", kind = "rule", name = "top.scheduler_body.0.call.1.1", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.right", params = [], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "peek"}} body {
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc154)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc155)
    }
    acir.rule @top.scheduler_body.0.call.2.92.0.call.4.10 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.right", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.right", interface = "Module:top.right:take", kind = "rule", name = "top.scheduler_body.0.call.2.92.0.call.4.10", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.right", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.right", role = "guard"} : (i32) -> i1 loc(#loc179)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc157)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc158)
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc159)
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc158)
      acir.cell.write %0, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc160)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v16", acir.source_type = "void"} loc(#loc161)
    }
    acir.rule @top.scheduler_body.0.call.7.93.0.call.9.10 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.right", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.right", interface = "Module:top.right:take", kind = "rule", name = "top.scheduler_body.0.call.7.93.0.call.9.10", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.right", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.right", role = "guard"} : (i32) -> i1 loc(#loc180)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc157)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc158)
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc159)
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc158)
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc158)
      acir.cell.write %0, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc160)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v16", acir.source_type = "void"} loc(#loc161)
    }
    acir.rule @top.cdb_body.0.call.15.6.0.call.21.22 : (i32, i32) -> () {acir.function = {contract = "Module:top.right", definition = "wakeup", instance = "top.right", interface = "Module:top.right:wakeup", kind = "rule", name = "top.cdb_body.0.call.15.6.0.call.21.22", output_type = "void", owner = "top.right", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "wakeup"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc163)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc164)
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc165)
        %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc165)
        %6 = acir.unary %5 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc166)
        %7 = scf.if %6 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc167)
          %16 = acir.extract %15 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc167)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc167)
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc168)
          %16 = acir.update %1, %15 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc169)
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %9 = acir.extract %8 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc170)
        %10 = acir.extract %9 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc170)
        %11 = acir.unary %10 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc171)
        %12 = scf.if %11 -> (i1) {
          %15 = acir.extract %8 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc172)
          %16 = acir.extract %15 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc172)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc172)
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %13 = arith.select %12, %true, %7 : i1
        %14 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc173)
          %16 = acir.update %8, %15 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc174)
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %13, %14 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      scf.if %3#0 {
        acir.cell.write %0, %3#1 {acir.id = "v64", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc175)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.23.13.0.call.49.21 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:top.right", definition = "place", guard_info = {name = "free", source = "function"}, instance = "top.right", interface = "Module:top.right:place", kind = "rule", name = "top.dispatch_body.0.call.23.13.0.call.49.21", output_type = "void", owner = "top.right", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "void", rule = "place"}} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = func.call @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.right", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc181)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc177)
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc178)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.scheduler attributes {acir.instance = {definition = "src_execution_885f3671.Scheduler", inputs = {}, members = {}, name = "top.scheduler", parent = "top", rules = ["issue_add", "issue_mul"]}} {
    acir.contract {relations = [], rules = ["issue_add", "issue_mul"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.scheduler_body.0.call.2.92 : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.scheduler", definition = "issue_add", instance = "top.scheduler", interface = "Module:top.scheduler:issue_add", kind = "rule", name = "top.scheduler_body.0.call.2.92", output_type = "queue<src_payloads_25ebcaa7.Entry>", owner = "top.scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "issue_add"}} body {
    ^bb0(%arg0: i1):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      cf.cond_br %arg0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.2.92.0.call.2.5(%c0_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc156)
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v7", acir.source_type = "void"} loc(#loc182)
    ^bb2:  // pred: ^bb0
      %1 = acir.rule.call @top.right::@top.scheduler_body.0.call.2.92.0.call.4.10(%c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc179)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v12", acir.source_type = "void"} loc(#loc183)
    }
    acir.rule @top.scheduler_body.0.call.7.93 : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.scheduler", definition = "issue_mul", instance = "top.scheduler", interface = "Module:top.scheduler:issue_mul", kind = "rule", name = "top.scheduler_body.0.call.7.93", output_type = "queue<src_payloads_25ebcaa7.Entry>", owner = "top.scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "issue_mul"}} body {
    ^bb0(%arg0: i1):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      cf.cond_br %arg0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.7.93.0.call.7.5(%c1_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc162)
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v7", acir.source_type = "void"} loc(#loc184)
    ^bb2:  // pred: ^bb0
      %1 = acir.rule.call @top.right::@top.scheduler_body.0.call.7.93.0.call.9.10(%c1_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc180)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v12", acir.source_type = "void"} loc(#loc185)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.scheduler_body", owner = "top.scheduler", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.0.0() {acir.id = "v1", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc186)
      %1 = acir.rule.call @top.right::@top.scheduler_body.0.call.1.1() {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc187)
      %2 = acir.extract %0 {acir.id = "v3", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc188)
      %3 = scf.if %2 -> (i1) {
        %16 = acir.extract %0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc189)
        %17 = acir.extract %16 {acir.id = "v6", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc189)
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %16 = acir.extract %0 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc190)
        %17 = acir.extract %16 {acir.id = "v13", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc190)
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %5 = acir.extract %1 {acir.id = "v18", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc191)
      %6 = scf.if %5 -> (i1) {
        %16 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc192)
        %17 = acir.extract %16 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc192)
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %7 = scf.if %6 -> (i1) {
        %16 = acir.extract %1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc193)
        %17 = acir.extract %16 {acir.id = "v28", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc193)
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %8 = scf.if %4 -> (i1) {
        %16 = acir.extract %0 {acir.id = "v34", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc194)
        %17 = arith.cmpi eq, %16, %c0_i32 {acir.id = "v36", acir.source_type = "bool"} : i32 loc(#loc194)
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %9 = scf.if %7 -> (i1) {
        %16 = acir.extract %1 {acir.id = "v42", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc195)
        %17 = arith.cmpi eq, %16, %c0_i32 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc195)
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %10 = scf.if %4 -> (i1) {
        %16 = acir.extract %0 {acir.id = "v50", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc196)
        %17 = arith.cmpi eq, %16, %c1_i32 {acir.id = "v52", acir.source_type = "bool"} : i32 loc(#loc196)
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %11 = scf.if %7 -> (i1) {
        %16 = acir.extract %1 {acir.id = "v58", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc197)
        %17 = arith.cmpi eq, %16, %c1_i32 {acir.id = "v60", acir.source_type = "bool"} : i32 loc(#loc197)
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %12 = scf.if %8 -> (i1) {
        %16 = acir.unary %9 "not" {acir.id = "v66", acir.source_type = "bool"} : i1 -> i1 loc(#loc198)
        %17 = scf.if %16 -> (i1) {
          scf.yield %true : i1
        } else {
          %18 = acir.extract %0 {acir.id = "v69", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc199)
          %19 = acir.extract %1 {acir.id = "v70", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc200)
          %20 = arith.cmpi ult, %18, %19 {acir.id = "v71", acir.source_type = "bool"} : i32 loc(#loc199)
          scf.yield %20 : i1
        }
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %13 = scf.if %10 -> (i1) {
        %16 = acir.unary %11 "not" {acir.id = "v80", acir.source_type = "bool"} : i1 -> i1 loc(#loc201)
        %17 = scf.if %16 -> (i1) {
          scf.yield %true : i1
        } else {
          %18 = acir.extract %0 {acir.id = "v83", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc202)
          %19 = acir.extract %1 {acir.id = "v84", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc203)
          %20 = arith.cmpi ult, %18, %19 {acir.id = "v85", acir.source_type = "bool"} : i32 loc(#loc202)
          scf.yield %20 : i1
        }
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %14 = acir.rule.call @top.scheduler::@top.scheduler_body.0.call.2.92(%12) {acir.id = "v93", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.scheduler", instance = "top.scheduler", interface = "Module:top.scheduler:issue_add", output_binding = @top::@top.add_requests, role = "rule", rule = "issue_add"} : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc204)
      %15 = acir.rule.call @top.scheduler::@top.scheduler_body.0.call.7.93(%13) {acir.id = "v94", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.scheduler", instance = "top.scheduler", interface = "Module:top.scheduler:issue_mul", output_binding = @top::@top.mul_requests, role = "rule", rule = "issue_mul"} : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc205)
      acir.body.end {acir.id = "v97", acir.source_type = "void"} loc(#loc206)
    }
  }
  acir.module @top.adder attributes {acir.instance = {definition = "src_execution_885f3671.Adder", inputs = {}, members = {}, name = "top.adder", parent = "top", rules = ["execute"]}} {
    acir.contract {relations = [], rules = ["execute"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.adder_body.0.call.12.1 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.adder", definition = "execute", instance = "top.adder", interface = "Module:top.adder:execute", kind = "rule", name = "top.adder_body.0.call.12.1", output_type = "queue<src_payloads_25ebcaa7.Completion>", owner = "top.adder", params = [{consume = false, id = "v1", name = "message", targets = ["top.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "execute"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc207)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc208)
      %2 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc209)
      %3 = acir.extract %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc210)
      %4 = acir.extract %3 {acir.id = "v8", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc210)
      %5 = acir.extract %0 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc211)
      %6 = acir.extract %5 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc211)
      %7 = arith.addi %4, %6 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc210)
      %8 = acir.aggregate(%1, %2, %7) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc212)
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v13", acir.source_type = "void"} loc(#loc213)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.adder_body", owner = "top.adder", params = [], result = "void"}} {
      %0 = acir.ref @top::@top.add_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.rule.call @top.adder::@top.adder_body.0.call.12.1(%0) {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.adder", instance = "top.adder", interface = "Module:top.adder:execute", output_binding = @top::@top.add_done, role = "rule", rule = "execute"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc214)
      acir.body.end {acir.id = "v4", acir.source_type = "void"} loc(#loc215)
    }
  }
  acir.module @top.multiplier attributes {acir.instance = {definition = "src_execution_885f3671.Multiplier", inputs = {}, members = {}, name = "top.multiplier", parent = "top", rules = ["accept", "advance"]}} {
    acir.contract {relations = [], rules = ["accept", "advance"]}
    acir.cell @top.multiplier.active : i1 {initial_fn = "ac_init_top.multiplier.active", name = "top.multiplier.active", type = "bool"}
    acir.cell @top.multiplier.remaining : i32 {initial_fn = "ac_init_top.multiplier.remaining", name = "top.multiplier.remaining", type = "u32"}
    acir.cell @top.multiplier.request : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.multiplier.request", name = "top.multiplier.request", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.multiplier_body.0.call.13.3 : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.multiplier", definition = "advance", instance = "top.multiplier", interface = "Module:top.multiplier:advance", kind = "rule", name = "top.multiplier_body.0.call.13.3", output_type = "queue<src_payloads_25ebcaa7.Completion>", owner = "top.multiplier", params = [], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "advance"}} body {
      %false = arith.constant {acir.source_type = "bool"} false
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc216)
      %0 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc217)
      %2 = arith.cmpi ne, %1, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc217)
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.cell.read %0 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc218)
      %4 = arith.subi %3, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc218)
      acir.cell.write %0, %4 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc219)
      %5 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc220)
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v15", acir.source_type = "void", omitted = true} loc(#loc220)
    ^bb2:  // pred: ^bb0
      %6 = acir.ref @top.multiplier::@top.multiplier.request {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %7 = acir.cell.read %6 {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc221)
      %8 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      acir.cell.write %8, %false {acir.id = "v21", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc222)
      %9 = acir.extract %7 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc223)
      %10 = acir.extract %7 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc224)
      %11 = acir.extract %7 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc225)
      %12 = acir.extract %11 {acir.id = "v25", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc225)
      %13 = acir.extract %7 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc226)
      %14 = acir.extract %13 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc226)
      %15 = arith.muli %12, %14 {acir.id = "v28", acir.source_type = "u32"} : i32 loc(#loc225)
      %16 = acir.aggregate(%9, %10, %15) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc227)
      acir.return %16 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v30", acir.source_type = "void"} loc(#loc228)
    }
    acir.rule @top.multiplier_body.0.call.14.7 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () {acir.function = {contract = "Module:top.multiplier", definition = "accept", instance = "top.multiplier", interface = "Module:top.multiplier:accept", kind = "rule", name = "top.multiplier_body.0.call.14.7", output_type = "void", owner = "top.multiplier", params = [{consume = false, id = "v1", name = "message", targets = ["top.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], public = true, result = "void", rule = "accept"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc229)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc230)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc231)
      %1 = acir.ref @top.multiplier::@top.multiplier.request {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %1, %0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc232)
      %2 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %c2_i32 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc233)
      %3 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      acir.cell.write %3, %true {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc234)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.multiplier_body", owner = "top.multiplier", params = [], result = "void"}} {
      %0 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "bool", role = "observe"} : !acir.cell<i1> -> i1 loc(#loc235)
      cf.cond_br %1, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %2 = acir.rule.call @top.multiplier::@top.multiplier_body.0.call.13.3() {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.multiplier", instance = "top.multiplier", interface = "Module:top.multiplier:advance", output_binding = @top::@top.mul_done, role = "rule", rule = "advance"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc236)
      acir.body.end {acir.id = "v6", acir.source_type = "void"} loc(#loc237)
    ^bb2:  // pred: ^bb0
      %3 = acir.ref @top::@top.mul_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.rule.call @top.multiplier::@top.multiplier_body.0.call.14.7(%3) {acir.id = "v8", acir.source_type = "void", contract = "Module:top.multiplier", instance = "top.multiplier", interface = "Module:top.multiplier:accept", role = "rule", rule = "accept"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () loc(#loc238)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cdb attributes {acir.instance = {definition = "src_model_3cf90884.Broadcast", inputs = {}, members = {}, name = "top.cdb", parent = "top", rules = ["broadcast"]}} {
    acir.contract {relations = [], rules = ["broadcast"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.15.6 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.cdb", definition = "broadcast", instance = "top.cdb", interface = "Module:top.cdb:broadcast", kind = "rule", name = "top.cdb_body.0.call.15.6", output_type = "signal<src_payloads_25ebcaa7.Completion>", owner = "top.cdb", params = [{consume = false, id = "v1", name = "a", targets = ["top.add_done"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = false, id = "v2", name = "b", targets = ["top.mul_done"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = false, id = "v3", name = "choose_add", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "broadcast"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg1: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg2: i1):
      %0 = scf.if %arg2 -> (!acir.struct<"src_payloads_25ebcaa7.Completion">) {
        %7 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc239)
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      } else {
        %7 = acir.queue.peek %arg1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc240)
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      }
      %1 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc241)
      %2 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc242)
      acir.rule.call @top.rob::@top.cdb_body.0.call.15.6.0.call.15.16(%1, %2) {acir.id = "v17", acir.source_type = "void", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc243)
      %3 = acir.extract %0 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc244)
      %4 = acir.extract %0 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc245)
      acir.rule.call @top.left::@top.cdb_body.0.call.15.6.0.call.20.19(%3, %4) {acir.id = "v20", acir.source_type = "void", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:wakeup", role = "rule", rule = "wakeup"} : (i32, i32) -> () loc(#loc246)
      %5 = acir.extract %0 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc247)
      %6 = acir.extract %0 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc248)
      acir.rule.call @top.right::@top.cdb_body.0.call.15.6.0.call.21.22(%5, %6) {acir.id = "v23", acir.source_type = "void", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:wakeup", role = "rule", rule = "wakeup"} : (i32, i32) -> () loc(#loc249)
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v24", acir.source_type = "void"} loc(#loc250)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cdb_body", owner = "top.cdb", params = [], result = "void"}} {
      %0 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.queue.empty %0 {acir.id = "v3", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc251)
      %2 = acir.unary %1 "not" {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1 loc(#loc252)
      %3 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %4 = acir.rule.call @top.cdb::@top.cdb_body.0.call.15.6(%0, %3, %2) {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.cdb", instance = "top.cdb", interface = "Module:top.cdb:broadcast", output_binding = @top::@top.event, role = "rule", rule = "broadcast"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc253)
      acir.body.end {acir.id = "v9", acir.source_type = "void"} loc(#loc254)
    }
  }
  acir.module @top.dispatch attributes {acir.instance = {definition = "src_model_3cf90884.Dispatch", inputs = {}, members = {}, name = "top.dispatch", parent = "top", rules = ["dispatch"]}} {
    acir.contract {relations = [], rules = ["dispatch"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.dispatch_body.0.call.23.13 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () {acir.function = {contract = "Module:top.dispatch", definition = "dispatch", instance = "top.dispatch", interface = "Module:top.dispatch:dispatch", kind = "rule", name = "top.dispatch_body.0.call.23.13", output_type = "void", owner = "top.dispatch", params = [{consume = false, id = "v1", name = "message", targets = ["top.source"], type = "queue<src_payloads_25ebcaa7.MicroOp>"}], public = true, result = "void", rule = "dispatch"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc255)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.MicroOp", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc256)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc257)
      %2 = acir.rule.call @top.rf::@top.dispatch_body.0.call.23.13.0.call.23.5(%1) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:operand", role = "rule", rule = "operand"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc258)
      %3 = acir.extract %0 {acir.id = "v7", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc259)
      %4 = acir.rule.call @top.rf::@top.dispatch_body.0.call.23.13.0.call.29.7(%3) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:operand", role = "rule", rule = "operand"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc260)
      %5 = acir.extract %0 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc261)
      %6 = acir.rule.call @top.rob::@top.dispatch_body.0.call.23.13.0.call.35.9(%5) {acir.id = "v10", acir.source_type = "u32", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:allocate", role = "rule", rule = "allocate"} : (i32) -> i32 loc(#loc68)
      %7 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc262)
      acir.rule.call @top.rf::@top.dispatch_body.0.call.23.13.0.call.45.11(%7, %6) {acir.id = "v12", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:rename", role = "rule", rule = "rename"} : (i32, i32) -> () loc(#loc263)
      %8 = acir.extract %0 {acir.id = "v14", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc264)
      %9 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc265)
      %10 = acir.aggregate(%true, %8, %6, %9, %2, %4) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc266)
      %11 = acir.rule.call @top.left::@top.dispatch_body.0.call.23.13.0.call.46.16() {acir.id = "v17", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc267)
      %12 = acir.extract %11 {acir.id = "v18", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc267)
      %13 = acir.unary %12 "not" {acir.id = "v19", acir.source_type = "bool"} : i1 -> i1 loc(#loc268)
      scf.if %13 {
        acir.rule.call @top.left::@top.dispatch_body.0.call.23.13.0.call.47.20(%10) {acir.id = "v21", acir.source_type = "void", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:place", role = "rule", rule = "place"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc176)
      } else {
        acir.rule.call @top.right::@top.dispatch_body.0.call.23.13.0.call.49.21(%10) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:place", role = "rule", rule = "place"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc181)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.dispatch_body", owner = "top.dispatch", params = [], result = "void"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top::@top.add_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.queue.empty %0 {acir.id = "v3", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc269)
      %2 = scf.if %1 -> (i1) {
        %3 = acir.ref @top::@top.mul_done {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
        %4 = acir.queue.empty %3 {acir.id = "v7", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc270)
        scf.yield %4 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %2 {
        %3 = acir.ref @top::@top.source {acir.source_type = "queue<src_payloads_25ebcaa7.MicroOp>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
        acir.rule.call @top.dispatch::@top.dispatch_body.0.call.23.13(%3) {acir.id = "v14", acir.source_type = "void", contract = "Module:top.dispatch", instance = "top.dispatch", interface = "Module:top.dispatch:dispatch", role = "rule", rule = "dispatch"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () loc(#loc271)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.commit attributes {acir.instance = {definition = "src_model_3cf90884.Commit", inputs = {}, members = {}, name = "top.commit", parent = "top", rules = ["commit"]}} {
    acir.contract {relations = [], rules = ["commit"]}
    acir.cell @top.commit.retired : i32 {initial_fn = "ac_init_top.commit.retired", name = "top.commit.retired", type = "u32"}
    acir.cell @top.commit.last_tag : i32 {initial_fn = "ac_init_top.commit.last_tag", name = "top.commit.last_tag", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.commit_body.0.call.52.0 : () -> () {acir.function = {contract = "Module:top.commit", definition = "commit", instance = "top.commit", interface = "Module:top.commit:commit", kind = "rule", name = "top.commit_body.0.call.52.0", output_type = "void", owner = "top.commit", params = [], public = true, result = "void", rule = "commit"}} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc272)
      %0 = acir.rule.call @top.rob::@top.commit_body.0.call.52.0.0.call.52.2() {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:retire", role = "rule", rule = "retire"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc273)
      acir.rule.call @top.rf::@top.commit_body.0.call.52.0.0.call.61.3(%0) {acir.id = "v4", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:commit", role = "rule", rule = "commit"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc274)
      %1 = acir.ref @top.commit::@top.commit.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc275)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc275)
      acir.cell.write %1, %3 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc276)
      %4 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc277)
      %5 = acir.ref @top.commit::@top.commit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc278)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.commit_body", owner = "top.commit", params = [], result = "void"}} {
      acir.rule.call @top.commit::@top.commit_body.0.call.52.0() {acir.id = "v1", acir.source_type = "void", contract = "Module:top.commit", instance = "top.commit", interface = "Module:top.commit:commit", role = "rule", rule = "commit"} : () -> () loc(#loc279)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.audit attributes {acir.instance = {definition = "src_model_3cf90884.CompletionAudit", inputs = {}, members = {}, name = "top.audit", parent = "top", rules = ["observe"]}} {
    acir.contract {relations = [], rules = ["observe"]}
    acir.cell @top.audit.last_tag : i32 {initial_fn = "ac_init_top.audit.last_tag", name = "top.audit.last_tag", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_top.add_requests_cap() -> i32
    func.func private @ac_init_top.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.mul_requests_cap() -> i32
    func.func private @ac_init_top.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.add_done_cap() -> i32
    func.func private @ac_init_top.add_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.mul_done_cap() -> i32
    func.func private @ac_init_top.mul_done() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_top.rob.head() -> i32
    func.func private @ac_init_top.rob.tail() -> i32
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.2.92.0.call.2.5.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.2.92.0.call.4.10.0.call.4.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.7.5.0.call.7.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.7.93.0.call.9.10.0.call.9.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.35.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.36.13.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.38.17.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.40.21.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.35.9.0.call.42.22.0.call.42.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.47.20.0.call.47.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.23.13.0.call.49.21.0.call.49.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.52.11.0.call.52.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.54.15.0.call.54.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.56.19.0.call.56.1(i32) -> i1
    func.func private @top.commit_body.0.call.52.0.0.call.52.2.0.call.58.20.0.call.58.1(i32) -> i1
    acir.rule @top.audit_body.0.call.63.1 : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () {acir.function = {contract = "Module:top.audit", definition = "observe", instance = "top.audit", interface = "Module:top.audit:observe", kind = "rule", name = "top.audit_body.0.call.63.1", output_type = "void", owner = "top.audit", params = [{consume = false, id = "v1", name = "value", targets = ["top.event"], type = "signal<src_payloads_25ebcaa7.Completion>"}], public = true, result = "void", rule = "observe"}} body {
    ^bb0(%arg0: !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>):
      %0 = acir.signal.get %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "observe"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc280)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc281)
      %2 = acir.ref @top.audit::@top.audit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %1 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc282)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.audit_body", owner = "top.audit", params = [], result = "void"}} {
      %0 = acir.ref @top::@top.event {acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.rule.call @top.audit::@top.audit_body.0.call.63.1(%0) {acir.id = "v2", acir.source_type = "void", contract = "Module:top.audit", instance = "top.audit", interface = "Module:top.audit:observe", role = "rule", rule = "observe"} : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () loc(#loc283)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:45)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:139)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:57)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:80)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:103)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:126)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:56)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":50:43)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":89:40)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":90:40)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:65)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:68)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:58)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":157:56)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":158:58)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":9:39)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":55:40)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":56:45)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":57:42)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":59:43)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":60:44)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":76:44)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":19:17)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:16)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:32)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:55)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:75)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:9)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":97:30)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":97:16)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":97:23)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":97:9)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":53:20)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":53:16)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":53:9)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:20)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:16)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:9)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:17)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:16)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:32)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:48)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:9)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:21)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:27)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:17)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:12)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:13)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":130:14)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":131:13)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:14)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:13)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:13)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":117:21)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:27)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:17)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":117:12)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:20)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:13)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:14)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:20)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:13)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:14)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:20)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:13)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":123:16)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":123:9)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:20)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:21)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:27)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:15)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:17)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:12)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:13)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:14)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":106:13)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:14)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":108:13)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:13)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":111:16)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":111:9)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":112:9)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:21)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:27)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":139:15)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:17)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:12)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:21)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:14)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:21)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:14)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:21)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:21)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:16)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:9)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":150:9)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:17)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:12)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:33)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:29)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:50)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":63:13)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":64:13)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":65:13)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":69:16)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":69:9)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":55:20)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:26)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:32)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:48)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:17)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":57:9)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":75:20)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:17)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":78:17)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":78:9)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":79:9)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:24)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:16)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:9)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:12)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":164:20)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":164:13)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":165:15)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":166:19)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":166:12)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:39)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:20)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:13)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":168:17)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:12)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:28)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:44)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:39)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:20)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:13)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:16)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:9)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:18)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:21)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:16)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:9)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:12)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":177:13)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:24)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:12)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:35)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:13)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:23)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:35)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:13)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:23)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":44:16)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":44:9)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:20)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:20)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:16)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:17)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:9)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:9)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:20)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":30:17)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:12)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:20)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:16)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:40)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:29)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:17)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:20)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:16)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:40)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:29)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:17)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":40:13)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":14:20)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":16:9)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:16)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:16)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:13)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:9)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:13)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:9)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":25:13)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:13)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:19)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:31)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:47)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:19)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:29)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:38)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:51)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:59)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:38)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:51)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:59)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:17)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:45)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:9)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":45:17)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:27)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:38)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:48)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:66)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:16)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:9)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:9)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:25)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:12)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:25)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:13)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":69:13)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":70:17)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:9)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:27)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:38)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:48)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:66)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:16)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:9)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:18)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:21)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:19)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:9)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:9)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:9)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:16)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:20)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:13)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":79:13)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":43:22)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:22)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:34)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:9)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":49:9)
#loc251 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc252 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:16)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:9)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:23)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc265 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc266 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc267 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc268 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:12)
#loc269 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc270 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc271 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc272 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:29)
#loc273 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":64:17)
#loc274 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:9)
#loc275 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:19)
#loc276 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc277 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:20)
#loc278 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc279 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":70:9)
#loc280 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":80:17)
#loc281 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:20)
#loc282 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:9)
#loc283 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":84:9)
