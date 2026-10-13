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
  func.func @ac_init_top.rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot0.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot1.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot2.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v43", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_top.rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.rob.slot3.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
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
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c6_i32, %c7_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v64", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc13)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_top.rf.renamed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.renamed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v74", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc14)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_top.rf.committed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.committed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v84", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc15)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_top.left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.left.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc16)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v90", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %1 = acir.aggregate() {acir.id = "v91", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v92", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc16)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_top.right.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc16)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v98", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %1 = acir.aggregate() {acir.id = "v99", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v100", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc16)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.scheduler.add_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.scheduler.add_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">> attributes {acir.function = {kind = "helper", name = "ac_init_top.scheduler.add_requests", params = [], result = "vector<src_payloads_25ebcaa7.Entry>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Entry>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
  }
  func.func @ac_init_top.scheduler.mul_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.scheduler.mul_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">> attributes {acir.function = {kind = "helper", name = "ac_init_top.scheduler.mul_requests", params = [], result = "vector<src_payloads_25ebcaa7.Entry>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Entry>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
  }
  func.func @ac_init_top.adder.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.adder.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">> attributes {acir.function = {kind = "helper", name = "ac_init_top.adder.out", params = [], result = "vector<src_payloads_25ebcaa7.Completion>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Completion>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
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
    %0 = acir.aggregate() {acir.id = "v116", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc19)
    %1 = acir.aggregate() {acir.id = "v117", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc19)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v118", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc19)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_top.multiplier.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.multiplier.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">> attributes {acir.function = {kind = "helper", name = "ac_init_top.multiplier.out", params = [], result = "vector<src_payloads_25ebcaa7.Completion>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Completion>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
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
  func.func @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "top.left", kind = "guard", name = "top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
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
  func.func @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "top.right", kind = "guard", name = "top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
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
  func.func @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "top.left", kind = "guard", name = "top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
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
  func.func @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "top.right", kind = "guard", name = "top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
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
  func.func @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "room", instance = "top.rob", kind = "guard", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc29)
    %0 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc30)
    %2 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %3 = acir.cell.read %2 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc31)
    %4 = arith.subi %1, %3 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc30)
    %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc30)
    return %5 : i1 loc(#loc32)
  }
  func.func @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.rob.slot0", kind = "guard", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc33)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc33)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc34)
    return %3 : i1 loc(#loc35)
  }
  func.func @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.rob.slot1", kind = "guard", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc33)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc33)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc34)
    return %3 : i1 loc(#loc35)
  }
  func.func @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.rob.slot2", kind = "guard", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc33)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc33)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc34)
    return %3 : i1 loc(#loc35)
  }
  func.func @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "top.rob.slot3", kind = "guard", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc33)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc33)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc34)
    return %3 : i1 loc(#loc35)
  }
  func.func @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "top.left", kind = "guard", name = "top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc36)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc37)
    return %3 : i1 loc(#loc38)
  }
  func.func @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "top.right", kind = "guard", name = "top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc36)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc37)
    return %3 : i1 loc(#loc38)
  }
  func.func @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "top.rob.slot0", kind = "guard", name = "top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
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
  func.func @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "top.rob.slot1", kind = "guard", name = "top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
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
  func.func @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "top.rob.slot2", kind = "guard", name = "top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
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
  func.func @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "top.rob.slot3", kind = "guard", name = "top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
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
  acir.module @top attributes {acir.instance = {definition = "src_model_3cf90884.OutOfOrder", inputs = {}, members = {}, name = "top", outputs, parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.source : !acir.struct<"src_payloads_25ebcaa7.MicroOp"> {capacity_fn = "ac_init_top.source_cap", initial_fn = "ac_init_top.source", name = "top.source", type = "src_payloads_25ebcaa7.MicroOp"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc44)
      %2 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %3 = acir.cell.read %2 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc45)
      %4 = acir.ref @top.adder::@top.adder.out {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %5 = acir.queue.peek %4 {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc46)
      %6 = acir.ref @top.multiplier::@top.multiplier.out {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %7 = acir.queue.peek %6 {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc47)
      %8 = acir.ref @top.cdb::@top.cdb.event {acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %9 = acir.signal.get %8 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "observe"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc48)
      %10 = acir.ref @top.audit::@top.audit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %11 = acir.cell.read %10 {acir.id = "v33", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc49)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rob attributes {acir.instance = {definition = "src_state_7e811fd9.ReorderBuffer", inputs = {}, members = {}, name = "top.rob", outputs, parent = "top", rules = ["allocate", "complete", "has_room", "lookup", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "has_room", "lookup", "retire"]}
    acir.cell @top.rob.head : i32 {initial_fn = "ac_init_top.rob.head", name = "top.rob.head", type = "u32"}
    acir.cell @top.rob.tail : i32 {initial_fn = "ac_init_top.rob.tail", name = "top.rob.tail", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.13.6.0.call.13.16 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob", definition = "complete", instance = "top.rob", interface = "Module:top.rob:complete", kind = "rule", name = "top.cdb_body.0.call.13.6.0.call.13.16", output_type = "void", owner = "top.rob", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc50)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc51)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc52)
      %1 = arith.andi %0, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc52)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc53)
      scf.if %2 {
        acir.rule.call @top.rob.slot0::@top.cdb_body.0.call.13.6.0.call.13.16.0.call.13.11(%arg0, %arg1) {acir.id = "v12", acir.source_type = "void", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc54)
      } else {
        %3 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc55)
        scf.if %3 {
          acir.rule.call @top.rob.slot1::@top.cdb_body.0.call.13.6.0.call.13.16.0.call.14.15(%arg0, %arg1) {acir.id = "v16", acir.source_type = "void", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc56)
        } else {
          %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc57)
          scf.if %4 {
            acir.rule.call @top.rob.slot2::@top.cdb_body.0.call.13.6.0.call.13.16.0.call.15.19(%arg0, %arg1) {acir.id = "v20", acir.source_type = "void", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc58)
          } else {
            acir.rule.call @top.rob.slot3::@top.cdb_body.0.call.13.6.0.call.13.16.0.call.16.20(%arg0, %arg1) {acir.id = "v21", acir.source_type = "void", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc59)
          }
        }
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "lookup", instance = "top.rob", interface = "Module:top.rob:lookup", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "lookup"}} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc60)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc61)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc62)
      %1 = arith.andi %0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc62)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc63)
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.21.10() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc64)
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v12", acir.source_type = "void"} loc(#loc65)
    ^bb2:  // pred: ^bb0
      %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc66)
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.22.15() {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc67)
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v17", acir.source_type = "void"} loc(#loc68)
    ^bb4:  // pred: ^bb2
      %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc69)
      cf.cond_br %6, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %7 = acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.23.20() {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc70)
      acir.return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v22", acir.source_type = "void"} loc(#loc71)
    ^bb6:  // pred: ^bb4
      %8 = acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.24.31() {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc72)
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v33", acir.source_type = "void"} loc(#loc73)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "lookup", instance = "top.rob", interface = "Module:top.rob:lookup", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "lookup"}} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc60)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc61)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc62)
      %1 = arith.andi %0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc62)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc63)
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.27.10() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc64)
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v12", acir.source_type = "void"} loc(#loc65)
    ^bb2:  // pred: ^bb0
      %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc66)
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.28.15() {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc67)
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v17", acir.source_type = "void"} loc(#loc68)
    ^bb4:  // pred: ^bb2
      %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc69)
      cf.cond_br %6, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %7 = acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.29.20() {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc70)
      acir.return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v22", acir.source_type = "void"} loc(#loc71)
    ^bb6:  // pred: ^bb4
      %8 = acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.30.31() {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", role = "rule", rule = "peek"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc72)
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v33", acir.source_type = "void"} loc(#loc73)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.33.9 : (i32) -> i32 {acir.function = {contract = "Module:top.rob", definition = "allocate", guard_info = {name = "room", source = "function"}, instance = "top.rob", interface = "Module:top.rob:allocate", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.33.9", output_type = "u32", owner = "top.rob", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], public = true, result = "u32", rule = "allocate"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob", role = "guard"} : (i32) -> i1 loc(#loc74)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc75)
    } body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc76)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc77)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @top.rob::@top.rob.tail {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc78)
      %2 = arith.subi %1, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc79)
      %3 = arith.andi %2, %c3_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc79)
      %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc80)
      scf.if %4 {
        acir.rule.call @top.rob.slot0::@top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13(%1, %arg0) {acir.id = "v14", acir.source_type = "void", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc81)
      } else {
        %6 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc82)
        scf.if %6 {
          acir.rule.call @top.rob.slot1::@top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17(%1, %arg0) {acir.id = "v18", acir.source_type = "void", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc83)
        } else {
          %7 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v20", acir.source_type = "bool"} : i32 loc(#loc84)
          scf.if %7 {
            acir.rule.call @top.rob.slot2::@top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21(%1, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc85)
          } else {
            acir.rule.call @top.rob.slot3::@top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22(%1, %arg0) {acir.id = "v23", acir.source_type = "void", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:allocate", role = "rule", rule = "allocate"} : (i32, i32) -> () loc(#loc86)
          }
        }
      }
      %5 = arith.addi %1, %c1_i32 {acir.id = "v40", acir.source_type = "u32"} : i32 loc(#loc87)
      acir.cell.write %0, %5 {acir.id = "v42", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc88)
      acir.return %1 : i32 {acir.id = "v43", acir.source_type = "void"} loc(#loc89)
    }
    acir.rule @top.commit_body.0.call.49.0.0.call.49.2 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob", definition = "retire", instance = "top.rob", interface = "Module:top.rob:retire", kind = "rule", name = "top.commit_body.0.call.49.0.0.call.49.2", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} body {
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc90)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc91)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @top.rob::@top.rob.head {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc92)
      %2 = arith.subi %1, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc93)
      %3 = arith.andi %2, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc93)
      %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc94)
      %5 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.rule.call @top.rob.slot0::@top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11(%1) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot0", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc95)
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        %7 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc96)
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
          %9 = acir.rule.call @top.rob.slot1::@top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15(%1) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot1", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc97)
          scf.yield %9 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        } else {
          %9 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32 loc(#loc98)
          %10 = scf.if %9 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
            %11 = acir.rule.call @top.rob.slot2::@top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19(%1) {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot2", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc99)
            scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          } else {
            %11 = acir.rule.call @top.rob.slot3::@top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20(%1) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob.slot3", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:retire", role = "rule", rule = "retire"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc100)
            scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          }
          scf.yield %10 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        }
        scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %6 = arith.addi %1, %c1_i32 {acir.id = "v38", acir.source_type = "u32"} : i32 loc(#loc101)
      acir.cell.write %0, %6 {acir.id = "v40", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc102)
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v41", acir.source_type = "void"} loc(#loc103)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.rob_body", owner = "top.rob", params = [], result = "void"}} {
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc104)
      %2 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %3 = acir.cell.read %2 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
      %4 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %5 = acir.cell.read %4 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc106)
      %6 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %7 = acir.cell.read %6 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc107)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slot0 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", inputs = {}, members = {}, name = "top.rob.slot0", outputs, parent = "top.rob", rules = ["allocate", "complete", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "peek", "retire"]}
    acir.cell @top.rob.slot0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot0.state", name = "top.rob.slot0.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.13.6.0.call.13.16.0.call.13.11 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot0", definition = "complete", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:complete", kind = "rule", name = "top.cdb_body.0.call.13.6.0.call.13.16.0.call.13.11", output_type = "void", owner = "top.rob.slot0", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc108)
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc109)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc110)
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc111)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc112)
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc112)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc113)
        %6 = acir.update %5, %arg1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc114)
        acir.cell.write %0, %6 {acir.id = "v29", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc115)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.21.10 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot0", definition = "peek", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.21.10", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot0", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc117)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.27.10 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot0", definition = "peek", instance = "top.rob.slot0", interface = "Module:top.rob.slot0:peek", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.27.10", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot0", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc117)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot0", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot0", interface = "Module:top.rob.slot0:allocate", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13", output_type = "void", owner = "top.rob.slot0", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "top.rob.slot0", role = "guard"} : (i32, i32) -> i1 loc(#loc81)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc118)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc119)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc120)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc121)
      %0 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc122)
      %1 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc123)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot0", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot0", interface = "Module:top.rob.slot0:retire", kind = "rule", name = "top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot0", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob.slot0", role = "guard"} : (i32) -> i1 loc(#loc95)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc124)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot0::@top.rob.slot0.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc125)
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc126)
      acir.cell.write %0, %2 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc127)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v15", acir.source_type = "void"} loc(#loc128)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.rob.slot0_body", owner = "top.rob.slot0", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slot1 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", inputs = {}, members = {}, name = "top.rob.slot1", outputs, parent = "top.rob", rules = ["allocate", "complete", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "peek", "retire"]}
    acir.cell @top.rob.slot1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot1.state", name = "top.rob.slot1.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.13.6.0.call.13.16.0.call.14.15 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot1", definition = "complete", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:complete", kind = "rule", name = "top.cdb_body.0.call.13.6.0.call.13.16.0.call.14.15", output_type = "void", owner = "top.rob.slot1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc108)
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc109)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc110)
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc111)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc112)
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc112)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc113)
        %6 = acir.update %5, %arg1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc114)
        acir.cell.write %0, %6 {acir.id = "v29", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc115)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.22.15 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot1", definition = "peek", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.22.15", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot1", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc117)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.28.15 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot1", definition = "peek", instance = "top.rob.slot1", interface = "Module:top.rob.slot1:peek", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.28.15", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot1", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc117)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot1", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot1", interface = "Module:top.rob.slot1:allocate", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17", output_type = "void", owner = "top.rob.slot1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "top.rob.slot1", role = "guard"} : (i32, i32) -> i1 loc(#loc83)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc118)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc119)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc120)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc121)
      %0 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc122)
      %1 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc123)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot1", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot1", interface = "Module:top.rob.slot1:retire", kind = "rule", name = "top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob.slot1", role = "guard"} : (i32) -> i1 loc(#loc97)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc124)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot1::@top.rob.slot1.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc125)
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc126)
      acir.cell.write %0, %2 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc127)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v15", acir.source_type = "void"} loc(#loc128)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.rob.slot1_body", owner = "top.rob.slot1", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slot2 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", inputs = {}, members = {}, name = "top.rob.slot2", outputs, parent = "top.rob", rules = ["allocate", "complete", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "peek", "retire"]}
    acir.cell @top.rob.slot2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot2.state", name = "top.rob.slot2.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.13.6.0.call.13.16.0.call.15.19 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot2", definition = "complete", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:complete", kind = "rule", name = "top.cdb_body.0.call.13.6.0.call.13.16.0.call.15.19", output_type = "void", owner = "top.rob.slot2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc108)
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc109)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc110)
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc111)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc112)
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc112)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc113)
        %6 = acir.update %5, %arg1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc114)
        acir.cell.write %0, %6 {acir.id = "v29", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc115)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.23.20 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot2", definition = "peek", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.23.20", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot2", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc117)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.29.20 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot2", definition = "peek", instance = "top.rob.slot2", interface = "Module:top.rob.slot2:peek", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.29.20", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot2", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc117)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot2", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot2", interface = "Module:top.rob.slot2:allocate", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21", output_type = "void", owner = "top.rob.slot2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "top.rob.slot2", role = "guard"} : (i32, i32) -> i1 loc(#loc85)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc118)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc119)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc120)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc121)
      %0 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc122)
      %1 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc123)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot2", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot2", interface = "Module:top.rob.slot2:retire", kind = "rule", name = "top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob.slot2", role = "guard"} : (i32) -> i1 loc(#loc99)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc124)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot2::@top.rob.slot2.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc125)
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc126)
      acir.cell.write %0, %2 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc127)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v15", acir.source_type = "void"} loc(#loc128)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.rob.slot2_body", owner = "top.rob.slot2", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rob.slot3 attributes {acir.instance = {definition = "src_state_7e811fd9.RobSlot", inputs = {}, members = {}, name = "top.rob.slot3", outputs, parent = "top.rob", rules = ["allocate", "complete", "peek", "retire"]}} {
    acir.contract {relations = [], rules = ["allocate", "complete", "peek", "retire"]}
    acir.cell @top.rob.slot3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_top.rob.slot3.state", name = "top.rob.slot3.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.13.6.0.call.13.16.0.call.16.20 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot3", definition = "complete", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:complete", kind = "rule", name = "top.cdb_body.0.call.13.6.0.call.13.16.0.call.16.20", output_type = "void", owner = "top.rob.slot3", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "complete"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc108)
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc109)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc110)
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc111)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc112)
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc112)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc113)
        %6 = acir.update %5, %arg1 {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc114)
        acir.cell.write %0, %6 {acir.id = "v29", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc115)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.24.31 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot3", definition = "peek", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33.0.call.24.31", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot3", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc117)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.30.31 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot3", definition = "peek", instance = "top.rob.slot3", interface = "Module:top.rob.slot3:peek", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33.0.call.30.31", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot3", params = [], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "peek"}} body {
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc116)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"} loc(#loc117)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22 : (i32, i32) -> () {acir.function = {contract = "Module:top.rob.slot3", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "top.rob.slot3", interface = "Module:top.rob.slot3:allocate", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22", output_type = "void", owner = "top.rob.slot3", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], public = true, result = "void", rule = "allocate"}} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "top.rob.slot3", role = "guard"} : (i32, i32) -> i1 loc(#loc86)
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc118)
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc119)
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc120)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc121)
      %0 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc122)
      %1 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      acir.cell.write %1, %0 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc123)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:top.rob.slot3", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "top.rob.slot3", interface = "Module:top.rob.slot3:retire", kind = "rule", name = "top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20", output_type = "src_payloads_25ebcaa7.RobEntry", owner = "top.rob.slot3", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.RobEntry", rule = "retire"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.rob.slot3", role = "guard"} : (i32) -> i1 loc(#loc100)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc124)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rob.slot3::@top.rob.slot3.state {acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc125)
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc126)
      acir.cell.write %0, %2 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc127)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v15", acir.source_type = "void"} loc(#loc128)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.rob.slot3_body", owner = "top.rob.slot3", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rf attributes {acir.instance = {definition = "src_state_7e811fd9.RegisterFile", inputs = {}, members = {}, name = "top.rf", outputs, parent = "top", rules = ["commit", "operand", "rename"]}} {
    acir.contract {relations = [], rules = ["commit", "operand", "rename"]}
    acir.cell @top.rf.registers : !acir.array<i32, 8> {initial_fn = "ac_init_top.rf.registers", name = "top.rf.registers", type = "array<u32,8>"}
    acir.cell @top.rf.renamed : !acir.array<i32, 8> {initial_fn = "ac_init_top.rf.renamed", name = "top.rf.renamed", type = "array<u32,8>"}
    acir.cell @top.rf.committed : !acir.array<i32, 8> {initial_fn = "ac_init_top.rf.committed", name = "top.rf.committed", type = "array<u32,8>"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.dispatch_body.0.call.21.13.0.call.21.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:top.rf", definition = "operand", instance = "top.rf", interface = "Module:top.rf:operand", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.21.5", output_type = "src_payloads_25ebcaa7.Operand", owner = "top.rf", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Operand", rule = "operand"}} body {
    ^bb0(%arg0: i32):
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc129)
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc130)
      acir.check %0 {acir.id = "v6", acir.source_type = "void"} loc(#loc131)
      %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc132)
      cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      %2 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc133)
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v14", acir.source_type = "void"} loc(#loc134)
    ^bb2(%3: i32):  // pred: ^bb0
      %4 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %5 = acir.cell.read %4 {acir.id = "v18", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc135)
      %6 = acir.extract %5[%3] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc135)
      %7 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %8 = acir.cell.read %7 {acir.id = "v21", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc136)
      %9 = acir.extract %8[%3] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc136)
      %10 = arith.cmpi eq, %6, %9 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc137)
      cf.cond_br %10, ^bb3, ^bb4(%6 : i32)
    ^bb3:  // pred: ^bb2
      %11 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %12 = acir.cell.read %11 {acir.id = "v27", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc138)
      %13 = acir.extract %12[%3] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc138)
      %14 = acir.aggregate(%true, %6, %13) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc139)
      acir.return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v30", acir.source_type = "void"} loc(#loc140)
    ^bb4(%15: i32):  // pred: ^bb2
      %16 = acir.rule.call @top.rob::@top.dispatch_body.0.call.21.13.0.call.21.5.0.call.21.33(%15) {acir.id = "v34", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:lookup", role = "rule", rule = "lookup"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc141)
      %17 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc142)
      cf.cond_br %17, ^bb5, ^bb6(%false : i1)
    ^bb5:  // pred: ^bb4
      %18 = acir.extract %16 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc143)
      cf.br ^bb6(%18 : i1)
    ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
      cf.cond_br %19, ^bb7, ^bb8(%false : i1)
    ^bb7:  // pred: ^bb6
      %20 = acir.extract %16 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc144)
      %21 = arith.cmpi eq, %20, %15 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc144)
      cf.br ^bb8(%21 : i1)
    ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
      cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
    ^bb9:  // pred: ^bb8
      %23 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc145)
      %24 = acir.aggregate(%true, %15, %23) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc146)
      acir.return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v53", acir.source_type = "void"} loc(#loc147)
    ^bb10(%25: i32):  // pred: ^bb8
      %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc148)
      acir.return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v61", acir.source_type = "void"} loc(#loc149)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.27.7 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:top.rf", definition = "operand", instance = "top.rf", interface = "Module:top.rf:operand", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.27.7", output_type = "src_payloads_25ebcaa7.Operand", owner = "top.rf", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Operand", rule = "operand"}} body {
    ^bb0(%arg0: i32):
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc129)
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc130)
      acir.check %0 {acir.id = "v6", acir.source_type = "void"} loc(#loc131)
      %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc132)
      cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      %2 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc133)
      acir.return %2 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v14", acir.source_type = "void"} loc(#loc134)
    ^bb2(%3: i32):  // pred: ^bb0
      %4 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %5 = acir.cell.read %4 {acir.id = "v18", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc135)
      %6 = acir.extract %5[%3] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc135)
      %7 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %8 = acir.cell.read %7 {acir.id = "v21", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc136)
      %9 = acir.extract %8[%3] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc136)
      %10 = arith.cmpi eq, %6, %9 {acir.id = "v23", acir.source_type = "bool"} : i32 loc(#loc137)
      cf.cond_br %10, ^bb3, ^bb4(%6 : i32)
    ^bb3:  // pred: ^bb2
      %11 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %12 = acir.cell.read %11 {acir.id = "v27", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc138)
      %13 = acir.extract %12[%3] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc138)
      %14 = acir.aggregate(%true, %6, %13) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc139)
      acir.return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v30", acir.source_type = "void"} loc(#loc140)
    ^bb4(%15: i32):  // pred: ^bb2
      %16 = acir.rule.call @top.rob::@top.dispatch_body.0.call.21.13.0.call.27.7.0.call.27.33(%15) {acir.id = "v34", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:lookup", role = "rule", rule = "lookup"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc141)
      %17 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc142)
      cf.cond_br %17, ^bb5, ^bb6(%false : i1)
    ^bb5:  // pred: ^bb4
      %18 = acir.extract %16 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc143)
      cf.br ^bb6(%18 : i1)
    ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
      cf.cond_br %19, ^bb7, ^bb8(%false : i1)
    ^bb7:  // pred: ^bb6
      %20 = acir.extract %16 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc144)
      %21 = arith.cmpi eq, %20, %15 {acir.id = "v44", acir.source_type = "bool"} : i32 loc(#loc144)
      cf.br ^bb8(%21 : i1)
    ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
      cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
    ^bb9:  // pred: ^bb8
      %23 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc145)
      %24 = acir.aggregate(%true, %15, %23) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc146)
      acir.return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v53", acir.source_type = "void"} loc(#loc147)
    ^bb10(%25: i32):  // pred: ^bb8
      %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc148)
      acir.return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v61", acir.source_type = "void"} loc(#loc149)
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.43.11 : (i32, i32) -> () {acir.function = {contract = "Module:top.rf", definition = "rename", instance = "top.rf", interface = "Module:top.rf:rename", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.43.11", output_type = "void", owner = "top.rf", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}, {consume = false, id = "v2", name = "tag", targets = [], type = "u32"}], public = true, result = "void", rule = "rename"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc150)
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc151)
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc152)
      acir.check %0 {acir.id = "v7", acir.source_type = "void"} loc(#loc153)
      %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc154)
      scf.if %1 {
        %2 = acir.ref @top.rf::@top.rf.renamed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
        %3 = acir.cell.read %2 {acir.id = "v13", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc155)
        %4 = acir.cell.read %2 {acir.id = "v17", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc155)
        %5 = acir.update %4, %arg1[%arg0] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc155)
        acir.cell.write %2, %5 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc155)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.commit_body.0.call.49.0.0.call.58.3 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () {acir.function = {contract = "Module:top.rf", definition = "commit", instance = "top.rf", interface = "Module:top.rf:commit", kind = "rule", name = "top.commit_body.0.call.49.0.0.call.58.3", output_type = "void", owner = "top.rf", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc156)
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc157)
      %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc157)
      scf.if %1 {
        %2 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc158)
        %3 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
        %4 = acir.cell.read %3 {acir.id = "v12", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc159)
        %5 = acir.cell.read %3 {acir.id = "v16", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc159)
        %6 = acir.extract %arg0 {acir.id = "v17", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc160)
        %7 = acir.update %5, %2[%6] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc159)
        acir.cell.write %3, %7 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc159)
        %8 = acir.extract %arg0 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc161)
        %9 = acir.ref @top.rf::@top.rf.committed {acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
        %10 = acir.cell.read %9 {acir.id = "v25", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc162)
        %11 = acir.cell.read %9 {acir.id = "v29", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc162)
        %12 = acir.extract %arg0 {acir.id = "v30", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc163)
        %13 = acir.update %11, %8[%12] {acir.id = "v31", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc162)
        acir.cell.write %9, %13 {acir.id = "v33", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc162)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.rf_body", owner = "top.rf", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.left attributes {acir.instance = {definition = "src_state_7e811fd9.ReservationStation", inputs = {}, members = {}, name = "top.left", outputs, parent = "top", rules = ["place", "take", "wakeup"]}} {
    acir.contract {relations = [], rules = ["place", "take", "wakeup"]}
    acir.cell @top.left.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.left.slot", name = "top.left.slot", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.scheduler_body.0.call.0.94.0.call.0.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.left", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.left", interface = "Module:top.left:take", kind = "rule", name = "top.scheduler_body.0.call.0.94.0.call.0.5", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.left", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.left", role = "guard"} : (i32) -> i1 loc(#loc164)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc165)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc166)
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc167)
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc166)
      acir.cell.write %0, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc168)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v16", acir.source_type = "void"} loc(#loc169)
    }
    acir.rule @top.scheduler_body.0.call.5.96.0.call.5.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.left", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.left", interface = "Module:top.left:take", kind = "rule", name = "top.scheduler_body.0.call.5.96.0.call.5.5", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.left", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.left", role = "guard"} : (i32) -> i1 loc(#loc170)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc165)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc166)
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc167)
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc166)
      acir.cell.write %0, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc168)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v16", acir.source_type = "void"} loc(#loc169)
    }
    acir.rule @top.cdb_body.0.call.13.6.0.call.18.19 : (i32, i32) -> () {acir.function = {contract = "Module:top.left", definition = "wakeup", instance = "top.left", interface = "Module:top.left:wakeup", kind = "rule", name = "top.cdb_body.0.call.13.6.0.call.18.19", output_type = "void", owner = "top.left", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "wakeup"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc172)
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc173)
        %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc173)
        %6 = acir.unary %5 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc174)
        %7 = scf.if %6 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc175)
          %16 = acir.extract %15 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc175)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc175)
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc176)
          %16 = acir.update %1, %15 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc177)
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %9 = acir.extract %8 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc178)
        %10 = acir.extract %9 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc178)
        %11 = acir.unary %10 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc179)
        %12 = scf.if %11 -> (i1) {
          %15 = acir.extract %8 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc180)
          %16 = acir.extract %15 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc180)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc180)
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %13 = arith.select %12, %true, %7 : i1
        %14 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc181)
          %16 = acir.update %8, %15 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc182)
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %13, %14 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      scf.if %3#0 {
        acir.cell.write %0, %3#1 {acir.id = "v64", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc183)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.44.21 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:top.left", definition = "place", guard_info = {name = "free", source = "function"}, instance = "top.left", interface = "Module:top.left:place", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.44.21", output_type = "void", owner = "top.left", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "void", rule = "place"}} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = func.call @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.left", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc184)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc185)
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc186)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.left_body", owner = "top.left", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.right attributes {acir.instance = {definition = "src_state_7e811fd9.ReservationStation", inputs = {}, members = {}, name = "top.right", outputs, parent = "top", rules = ["place", "take", "wakeup"]}} {
    acir.contract {relations = [], rules = ["place", "take", "wakeup"]}
    acir.cell @top.right.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.right.slot", name = "top.right.slot", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.scheduler_body.0.call.0.94.0.call.2.10 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.right", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.right", interface = "Module:top.right:take", kind = "rule", name = "top.scheduler_body.0.call.0.94.0.call.2.10", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.right", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.right", role = "guard"} : (i32) -> i1 loc(#loc187)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc165)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc166)
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc167)
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc166)
      acir.cell.write %0, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc168)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v16", acir.source_type = "void"} loc(#loc169)
    }
    acir.rule @top.scheduler_body.0.call.5.96.0.call.7.10 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.right", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "top.right", interface = "Module:top.right:take", kind = "rule", name = "top.scheduler_body.0.call.5.96.0.call.7.10", output_type = "src_payloads_25ebcaa7.Entry", owner = "top.right", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "take"}} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.right", role = "guard"} : (i32) -> i1 loc(#loc188)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc165)
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false loc(#loc166)
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc167)
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc166)
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc166)
      acir.cell.write %0, %4 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc168)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v16", acir.source_type = "void"} loc(#loc169)
    }
    acir.rule @top.cdb_body.0.call.13.6.0.call.19.22 : (i32, i32) -> () {acir.function = {contract = "Module:top.right", definition = "wakeup", instance = "top.right", interface = "Module:top.right:wakeup", kind = "rule", name = "top.cdb_body.0.call.13.6.0.call.19.22", output_type = "void", owner = "top.right", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "wakeup"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc171)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc172)
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc173)
        %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc173)
        %6 = acir.unary %5 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1 loc(#loc174)
        %7 = scf.if %6 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc175)
          %16 = acir.extract %15 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc175)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc175)
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc176)
          %16 = acir.update %1, %15 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc177)
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %9 = acir.extract %8 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc178)
        %10 = acir.extract %9 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc178)
        %11 = acir.unary %10 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1 loc(#loc179)
        %12 = scf.if %11 -> (i1) {
          %15 = acir.extract %8 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc180)
          %16 = acir.extract %15 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc180)
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc180)
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %13 = arith.select %12, %true, %7 : i1
        %14 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc181)
          %16 = acir.update %8, %15 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc182)
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %13, %14 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      scf.if %3#0 {
        acir.cell.write %0, %3#1 {acir.id = "v64", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc183)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.dispatch_body.0.call.21.13.0.call.46.22 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:top.right", definition = "place", guard_info = {name = "free", source = "function"}, instance = "top.right", interface = "Module:top.right:place", kind = "rule", name = "top.dispatch_body.0.call.21.13.0.call.46.22", output_type = "void", owner = "top.right", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], public = true, result = "void", rule = "place"}} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = func.call @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "top.right", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc189)
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc185)
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc186)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.right_body", owner = "top.right", params = [], result = "void"}} {
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.scheduler attributes {acir.instance = {definition = "src_execution_885f3671.Scheduler", inputs = {}, members = {}, name = "top.scheduler", outputs, parent = "top", rules = ["issue_add", "issue_mul"]}} {
    acir.contract {relations = [], rules = ["issue_add", "issue_mul"]}
    acir.queue @top.scheduler.add_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity_fn = "ac_init_top.scheduler.add_requests_cap", initial_fn = "ac_init_top.scheduler.add_requests", name = "top.scheduler.add_requests", type = "src_payloads_25ebcaa7.Entry"}
    acir.queue @top.scheduler.mul_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity_fn = "ac_init_top.scheduler.mul_requests_cap", initial_fn = "ac_init_top.scheduler.mul_requests", name = "top.scheduler.mul_requests", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.scheduler_body.0.call.0.94 : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.scheduler", definition = "issue_add", instance = "top.scheduler", interface = "Module:top.scheduler:issue_add", kind = "rule", name = "top.scheduler_body.0.call.0.94", output_type = "queue<src_payloads_25ebcaa7.Entry>", owner = "top.scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "issue_add"}} body {
    ^bb0(%arg0: i1):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      cf.cond_br %arg0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.0.94.0.call.0.5(%c0_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc164)
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v7", acir.source_type = "void"} loc(#loc190)
    ^bb2:  // pred: ^bb0
      %1 = acir.rule.call @top.right::@top.scheduler_body.0.call.0.94.0.call.2.10(%c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc187)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v12", acir.source_type = "void"} loc(#loc191)
    }
    acir.rule @top.scheduler_body.0.call.5.96 : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:top.scheduler", definition = "issue_mul", instance = "top.scheduler", interface = "Module:top.scheduler:issue_mul", kind = "rule", name = "top.scheduler_body.0.call.5.96", output_type = "queue<src_payloads_25ebcaa7.Entry>", owner = "top.scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Entry", rule = "issue_mul"}} body {
    ^bb0(%arg0: i1):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      cf.cond_br %arg0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %0 = acir.rule.call @top.left::@top.scheduler_body.0.call.5.96.0.call.5.5(%c1_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc170)
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v7", acir.source_type = "void"} loc(#loc192)
    ^bb2:  // pred: ^bb0
      %1 = acir.rule.call @top.right::@top.scheduler_body.0.call.5.96.0.call.7.10(%c1_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:take", role = "rule", rule = "take"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc188)
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v12", acir.source_type = "void"} loc(#loc193)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.scheduler_body", owner = "top.scheduler", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc194)
      %2 = acir.ref @top.right::@top.right.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %3 = acir.cell.read %2 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc195)
      %4 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc196)
      %5 = scf.if %4 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc197)
        %19 = acir.extract %18 {acir.id = "v8", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc197)
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %6 = scf.if %5 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc198)
        %19 = acir.extract %18 {acir.id = "v15", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc198)
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %7 = acir.extract %3 {acir.id = "v20", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc199)
      %8 = scf.if %7 -> (i1) {
        %18 = acir.extract %3 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc200)
        %19 = acir.extract %18 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc200)
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %9 = scf.if %8 -> (i1) {
        %18 = acir.extract %3 {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc201)
        %19 = acir.extract %18 {acir.id = "v30", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc201)
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %10 = scf.if %6 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v36", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc202)
        %19 = arith.cmpi eq, %18, %c0_i32 {acir.id = "v38", acir.source_type = "bool"} : i32 loc(#loc202)
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %11 = scf.if %9 -> (i1) {
        %18 = acir.extract %3 {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc203)
        %19 = arith.cmpi eq, %18, %c0_i32 {acir.id = "v46", acir.source_type = "bool"} : i32 loc(#loc203)
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %12 = scf.if %6 -> (i1) {
        %18 = acir.extract %1 {acir.id = "v52", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc204)
        %19 = arith.cmpi eq, %18, %c1_i32 {acir.id = "v54", acir.source_type = "bool"} : i32 loc(#loc204)
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %13 = scf.if %9 -> (i1) {
        %18 = acir.extract %3 {acir.id = "v60", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc205)
        %19 = arith.cmpi eq, %18, %c1_i32 {acir.id = "v62", acir.source_type = "bool"} : i32 loc(#loc205)
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %14 = scf.if %10 -> (i1) {
        %18 = acir.unary %11 "not" {acir.id = "v68", acir.source_type = "bool"} : i1 -> i1 loc(#loc206)
        %19 = scf.if %18 -> (i1) {
          scf.yield %true : i1
        } else {
          %20 = acir.extract %1 {acir.id = "v71", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc207)
          %21 = acir.extract %3 {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc208)
          %22 = arith.cmpi ult, %20, %21 {acir.id = "v73", acir.source_type = "bool"} : i32 loc(#loc207)
          scf.yield %22 : i1
        }
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %15 = scf.if %12 -> (i1) {
        %18 = acir.unary %13 "not" {acir.id = "v82", acir.source_type = "bool"} : i1 -> i1 loc(#loc209)
        %19 = scf.if %18 -> (i1) {
          scf.yield %true : i1
        } else {
          %20 = acir.extract %1 {acir.id = "v85", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc210)
          %21 = acir.extract %3 {acir.id = "v86", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc211)
          %22 = arith.cmpi ult, %20, %21 {acir.id = "v87", acir.source_type = "bool"} : i32 loc(#loc210)
          scf.yield %22 : i1
        }
        scf.yield %19 : i1
      } else {
        scf.yield %false : i1
      }
      %16 = acir.rule.call @top.scheduler::@top.scheduler_body.0.call.0.94(%14) {acir.id = "v95", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.scheduler", instance = "top.scheduler", interface = "Module:top.scheduler:issue_add", output_binding = @top.scheduler::@top.scheduler.add_requests, role = "rule", rule = "issue_add"} : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc212)
      %17 = acir.rule.call @top.scheduler::@top.scheduler_body.0.call.5.96(%15) {acir.id = "v97", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:top.scheduler", instance = "top.scheduler", interface = "Module:top.scheduler:issue_mul", output_binding = @top.scheduler::@top.scheduler.mul_requests, role = "rule", rule = "issue_mul"} : (i1) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc213)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.adder attributes {acir.instance = {definition = "src_execution_885f3671.Adder", inputs = {}, members = {}, name = "top.adder", outputs, parent = "top", rules = ["execute"]}} {
    acir.contract {relations = [], rules = ["execute"]}
    acir.queue @top.adder.out : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity_fn = "ac_init_top.adder.out_cap", initial_fn = "ac_init_top.adder.out", name = "top.adder.out", type = "src_payloads_25ebcaa7.Completion"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.adder_body.0.call.10.1 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.adder", definition = "execute", instance = "top.adder", interface = "Module:top.adder:execute", kind = "rule", name = "top.adder_body.0.call.10.1", output_type = "queue<src_payloads_25ebcaa7.Completion>", owner = "top.adder", params = [{consume = false, id = "v1", name = "message", targets = ["top.scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "execute"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc214)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc215)
      %2 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc216)
      %3 = acir.extract %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc217)
      %4 = acir.extract %3 {acir.id = "v8", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc217)
      %5 = acir.extract %0 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc218)
      %6 = acir.extract %5 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc218)
      %7 = arith.addi %4, %6 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc217)
      %8 = acir.aggregate(%1, %2, %7) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc219)
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v13", acir.source_type = "void"} loc(#loc220)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.adder_body", owner = "top.adder", params = [], result = "void"}} {
      %0 = acir.ref @top.scheduler::@top.scheduler.add_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.rule.call @top.adder::@top.adder_body.0.call.10.1(%0) {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.adder", instance = "top.adder", interface = "Module:top.adder:execute", output_binding = @top.adder::@top.adder.out, role = "rule", rule = "execute"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc221)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.multiplier attributes {acir.instance = {definition = "src_execution_885f3671.Multiplier", inputs = {}, members = {}, name = "top.multiplier", outputs, parent = "top", rules = ["accept", "advance"]}} {
    acir.contract {relations = [], rules = ["accept", "advance"]}
    acir.cell @top.multiplier.active : i1 {initial_fn = "ac_init_top.multiplier.active", name = "top.multiplier.active", type = "bool"}
    acir.cell @top.multiplier.remaining : i32 {initial_fn = "ac_init_top.multiplier.remaining", name = "top.multiplier.remaining", type = "u32"}
    acir.cell @top.multiplier.request : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_top.multiplier.request", name = "top.multiplier.request", type = "src_payloads_25ebcaa7.Entry"}
    acir.queue @top.multiplier.out : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity_fn = "ac_init_top.multiplier.out_cap", initial_fn = "ac_init_top.multiplier.out", name = "top.multiplier.out", type = "src_payloads_25ebcaa7.Completion"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.multiplier_body.0.call.11.3 : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.multiplier", definition = "advance", instance = "top.multiplier", interface = "Module:top.multiplier:advance", kind = "rule", name = "top.multiplier_body.0.call.11.3", output_type = "queue<src_payloads_25ebcaa7.Completion>", owner = "top.multiplier", params = [], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "advance"}} body {
      %false = arith.constant {acir.source_type = "bool"} false
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc222)
      %0 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc223)
      %2 = arith.cmpi ne, %1, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc223)
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.cell.read %0 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc224)
      %4 = arith.subi %3, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc224)
      acir.cell.write %0, %4 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc225)
      %5 = acir.aggregate() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Completion"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc226)
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v15", acir.source_type = "void", omitted = true} loc(#loc226)
    ^bb2:  // pred: ^bb0
      %6 = acir.ref @top.multiplier::@top.multiplier.request {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %7 = acir.cell.read %6 {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc227)
      %8 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      acir.cell.write %8, %false {acir.id = "v21", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc228)
      %9 = acir.extract %7 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc229)
      %10 = acir.extract %7 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc230)
      %11 = acir.extract %7 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc231)
      %12 = acir.extract %11 {acir.id = "v25", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc231)
      %13 = acir.extract %7 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc232)
      %14 = acir.extract %13 {acir.id = "v27", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc232)
      %15 = arith.muli %12, %14 {acir.id = "v28", acir.source_type = "u32"} : i32 loc(#loc231)
      %16 = acir.aggregate(%9, %10, %15) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc233)
      acir.return %16 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v30", acir.source_type = "void"} loc(#loc234)
    }
    acir.rule @top.multiplier_body.0.call.12.6 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () {acir.function = {contract = "Module:top.multiplier", definition = "accept", instance = "top.multiplier", interface = "Module:top.multiplier:accept", kind = "rule", name = "top.multiplier_body.0.call.12.6", output_type = "void", owner = "top.multiplier", params = [{consume = false, id = "v1", name = "message", targets = ["top.scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], public = true, result = "void", rule = "accept"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc235)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc236)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc237)
      %1 = acir.ref @top.multiplier::@top.multiplier.request {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %1, %0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc238)
      %2 = acir.ref @top.multiplier::@top.multiplier.remaining {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %c2_i32 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc239)
      %3 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      acir.cell.write %3, %true {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc240)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.multiplier_body", owner = "top.multiplier", params = [], result = "void"}} {
      %0 = acir.ref @top.multiplier::@top.multiplier.active {acir.source_type = "cell<bool>"} : !acir.cell<i1>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "bool", role = "observe"} : !acir.cell<i1> -> i1 loc(#loc241)
      scf.if %1 {
        %2 = acir.rule.call @top.multiplier::@top.multiplier_body.0.call.11.3() {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.multiplier", instance = "top.multiplier", interface = "Module:top.multiplier:advance", output_binding = @top.multiplier::@top.multiplier.out, role = "rule", rule = "advance"} : () -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc242)
      } else {
        %2 = acir.ref @top.scheduler::@top.scheduler.mul_requests {acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
        acir.rule.call @top.multiplier::@top.multiplier_body.0.call.12.6(%2) {acir.id = "v7", acir.source_type = "void", contract = "Module:top.multiplier", instance = "top.multiplier", interface = "Module:top.multiplier:accept", role = "rule", rule = "accept"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () loc(#loc243)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cdb attributes {acir.instance = {definition = "src_model_3cf90884.Broadcast", inputs = {}, members = {}, name = "top.cdb", outputs, parent = "top", rules = ["broadcast"]}} {
    acir.contract {relations = [], rules = ["broadcast"]}
    acir.signal @top.cdb.event : !acir.struct<"src_payloads_25ebcaa7.Completion"> {name = "top.cdb.event", type = "src_payloads_25ebcaa7.Completion"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.cdb_body.0.call.13.6 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.function = {contract = "Module:top.cdb", definition = "broadcast", instance = "top.cdb", interface = "Module:top.cdb:broadcast", kind = "rule", name = "top.cdb_body.0.call.13.6", output_type = "signal<src_payloads_25ebcaa7.Completion>", owner = "top.cdb", params = [{consume = false, id = "v1", name = "a", targets = ["top.adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = false, id = "v2", name = "b", targets = ["top.multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = false, id = "v3", name = "choose_add", targets = [], type = "bool"}], public = true, result = "src_payloads_25ebcaa7.Completion", rule = "broadcast"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg1: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg2: i1):
      %0 = scf.if %arg2 -> (!acir.struct<"src_payloads_25ebcaa7.Completion">) {
        %7 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc244)
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      } else {
        %7 = acir.queue.peek %arg1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc245)
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      }
      %1 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc246)
      %2 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc247)
      acir.rule.call @top.rob::@top.cdb_body.0.call.13.6.0.call.13.16(%1, %2) {acir.id = "v17", acir.source_type = "void", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:complete", role = "rule", rule = "complete"} : (i32, i32) -> () loc(#loc248)
      %3 = acir.extract %0 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc249)
      %4 = acir.extract %0 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc250)
      acir.rule.call @top.left::@top.cdb_body.0.call.13.6.0.call.18.19(%3, %4) {acir.id = "v20", acir.source_type = "void", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:wakeup", role = "rule", rule = "wakeup"} : (i32, i32) -> () loc(#loc251)
      %5 = acir.extract %0 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc252)
      %6 = acir.extract %0 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc253)
      acir.rule.call @top.right::@top.cdb_body.0.call.13.6.0.call.19.22(%5, %6) {acir.id = "v23", acir.source_type = "void", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:wakeup", role = "rule", rule = "wakeup"} : (i32, i32) -> () loc(#loc254)
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v24", acir.source_type = "void"} loc(#loc255)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cdb_body", owner = "top.cdb", params = [], result = "void"}} {
      %0 = acir.ref @top.adder::@top.adder.out {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.queue.empty %0 {acir.id = "v3", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc256)
      %2 = acir.unary %1 "not" {acir.id = "v4", acir.source_type = "bool"} : i1 -> i1 loc(#loc257)
      %3 = acir.ref @top.multiplier::@top.multiplier.out {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %4 = acir.rule.call @top.cdb::@top.cdb_body.0.call.13.6(%0, %3, %2) {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Completion", contract = "Module:top.cdb", instance = "top.cdb", interface = "Module:top.cdb:broadcast", output_binding = @top.cdb::@top.cdb.event, role = "rule", rule = "broadcast"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc258)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.dispatch attributes {acir.instance = {definition = "src_model_3cf90884.Dispatch", inputs = {}, members = {}, name = "top.dispatch", outputs, parent = "top", rules = ["dispatch"]}} {
    acir.contract {relations = [], rules = ["dispatch"]}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.dispatch_body.0.call.21.13 : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () {acir.function = {contract = "Module:top.dispatch", definition = "dispatch", instance = "top.dispatch", interface = "Module:top.dispatch:dispatch", kind = "rule", name = "top.dispatch_body.0.call.21.13", output_type = "void", owner = "top.dispatch", params = [{consume = false, id = "v1", name = "message", targets = ["top.source"], type = "queue<src_payloads_25ebcaa7.MicroOp>"}], public = true, result = "void", rule = "dispatch"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>):
      %true = arith.constant {acir.source_type = "bool"} true loc(#loc259)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.MicroOp", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc260)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc261)
      %2 = acir.rule.call @top.rf::@top.dispatch_body.0.call.21.13.0.call.21.5(%1) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:operand", role = "rule", rule = "operand"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc262)
      %3 = acir.extract %0 {acir.id = "v7", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc263)
      %4 = acir.rule.call @top.rf::@top.dispatch_body.0.call.21.13.0.call.27.7(%3) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:operand", role = "rule", rule = "operand"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc264)
      %5 = acir.extract %0 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc265)
      %6 = acir.rule.call @top.rob::@top.dispatch_body.0.call.21.13.0.call.33.9(%5) {acir.id = "v10", acir.source_type = "u32", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:allocate", role = "rule", rule = "allocate"} : (i32) -> i32 loc(#loc74)
      %7 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc266)
      acir.rule.call @top.rf::@top.dispatch_body.0.call.21.13.0.call.43.11(%7, %6) {acir.id = "v12", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:rename", role = "rule", rule = "rename"} : (i32, i32) -> () loc(#loc267)
      %8 = acir.extract %0 {acir.id = "v14", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc268)
      %9 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc269)
      %10 = acir.aggregate(%true, %8, %6, %9, %2, %4) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc270)
      %11 = acir.ref @top.left::@top.left.slot {acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %12 = acir.cell.read %11 {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc271)
      %13 = acir.extract %12 {acir.id = "v19", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc271)
      %14 = acir.unary %13 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc272)
      scf.if %14 {
        acir.rule.call @top.left::@top.dispatch_body.0.call.21.13.0.call.44.21(%10) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.left", instance = "top.left", interface = "Module:top.left:place", role = "rule", rule = "place"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc184)
      } else {
        acir.rule.call @top.right::@top.dispatch_body.0.call.21.13.0.call.46.22(%10) {acir.id = "v23", acir.source_type = "void", contract = "Module:top.right", instance = "top.right", interface = "Module:top.right:place", role = "rule", rule = "place"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc189)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.dispatch_body", owner = "top.dispatch", params = [], result = "void"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.adder::@top.adder.out {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.queue.empty %0 {acir.id = "v3", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc273)
      %2 = scf.if %1 -> (i1) {
        %3 = acir.ref @top.multiplier::@top.multiplier.out {acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
        %4 = acir.queue.empty %3 {acir.id = "v7", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc274)
        scf.yield %4 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %2 {
        %3 = acir.ref @top::@top.source {acir.source_type = "queue<src_payloads_25ebcaa7.MicroOp>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
        acir.rule.call @top.dispatch::@top.dispatch_body.0.call.21.13(%3) {acir.id = "v14", acir.source_type = "void", contract = "Module:top.dispatch", instance = "top.dispatch", interface = "Module:top.dispatch:dispatch", role = "rule", rule = "dispatch"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () loc(#loc275)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.commit attributes {acir.instance = {definition = "src_model_3cf90884.Commit", inputs = {}, members = {}, name = "top.commit", outputs, parent = "top", rules = ["commit"]}} {
    acir.contract {relations = [], rules = ["commit"]}
    acir.cell @top.commit.retired : i32 {initial_fn = "ac_init_top.commit.retired", name = "top.commit.retired", type = "u32"}
    acir.cell @top.commit.last_tag : i32 {initial_fn = "ac_init_top.commit.last_tag", name = "top.commit.last_tag", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.commit_body.0.call.49.0 : () -> () {acir.function = {contract = "Module:top.commit", definition = "commit", instance = "top.commit", interface = "Module:top.commit:commit", kind = "rule", name = "top.commit_body.0.call.49.0", output_type = "void", owner = "top.commit", params = [], public = true, result = "void", rule = "commit"}} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc276)
      %0 = acir.rule.call @top.rob::@top.commit_body.0.call.49.0.0.call.49.2() {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:top.rob", instance = "top.rob", interface = "Module:top.rob:retire", role = "rule", rule = "retire"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc277)
      acir.rule.call @top.rf::@top.commit_body.0.call.49.0.0.call.58.3(%0) {acir.id = "v4", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:commit", role = "rule", rule = "commit"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc278)
      %1 = acir.ref @top.commit::@top.commit.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc279)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc279)
      acir.cell.write %1, %3 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc280)
      %4 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc281)
      %5 = acir.ref @top.commit::@top.commit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc282)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.commit_body", owner = "top.commit", params = [], result = "void"}} {
      acir.rule.call @top.commit::@top.commit_body.0.call.49.0() {acir.id = "v1", acir.source_type = "void", contract = "Module:top.commit", instance = "top.commit", interface = "Module:top.commit:commit", role = "rule", rule = "commit"} : () -> () loc(#loc283)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.audit attributes {acir.instance = {definition = "src_model_3cf90884.CompletionAudit", inputs = {}, members = {}, name = "top.audit", outputs, parent = "top", rules = ["observe"]}} {
    acir.contract {relations = [], rules = ["observe"]}
    acir.cell @top.audit.last_tag : i32 {initial_fn = "ac_init_top.audit.last_tag", name = "top.audit.last_tag", type = "u32"}
    func.func private @ac_init_top.source_cap() -> i32
    func.func private @ac_init_top.source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
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
    func.func private @ac_init_top.scheduler.add_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_top.scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_top.adder.out_cap() -> i32
    func.func private @ac_init_top.adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.multiplier.active() -> i1
    func.func private @ac_init_top.multiplier.remaining() -> i32
    func.func private @ac_init_top.multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_top.multiplier.out_cap() -> i32
    func.func private @ac_init_top.multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_top.commit.retired() -> i32
    func.func private @ac_init_top.commit.last_tag() -> i32
    func.func private @ac_init_top.audit.last_tag() -> i32
    func.func private @top.scheduler_body.0.call.0.94.0.call.0.5.0.call.0.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.0.94.0.call.2.10.0.call.2.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.5.5.0.call.5.1(i32) -> i1
    func.func private @top.scheduler_body.0.call.5.96.0.call.7.10.0.call.7.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.33.1(i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.34.13.0.call.34.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.36.17.0.call.36.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.38.21.0.call.38.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.33.9.0.call.40.22.0.call.40.2(i32, i32) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.44.21.0.call.44.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.dispatch_body.0.call.21.13.0.call.46.22.0.call.46.1(!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.49.11.0.call.49.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.51.15.0.call.51.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.53.19.0.call.53.1(i32) -> i1
    func.func private @top.commit_body.0.call.49.0.0.call.49.2.0.call.55.20.0.call.55.1(i32) -> i1
    acir.rule @top.audit_body.0.call.60.1 : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () {acir.function = {contract = "Module:top.audit", definition = "observe", instance = "top.audit", interface = "Module:top.audit:observe", kind = "rule", name = "top.audit_body.0.call.60.1", output_type = "void", owner = "top.audit", params = [{consume = false, id = "v1", name = "value", targets = ["top.cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>"}], public = true, result = "void", rule = "observe"}} body {
    ^bb0(%arg0: !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>):
      %0 = acir.signal.get %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "observe"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc284)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc285)
      %2 = acir.ref @top.audit::@top.audit.last_tag {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %1 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc286)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.audit_body", owner = "top.audit", params = [], result = "void"}} {
      %0 = acir.ref @top.cdb::@top.cdb.event {acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.rule.call @top.audit::@top.audit_body.0.call.60.1(%0) {acir.id = "v2", acir.source_type = "void", contract = "Module:top.audit", instance = "top.audit", interface = "Module:top.audit:observe", role = "rule", rule = "observe"} : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () loc(#loc287)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":93:45)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":93:139)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":93:57)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":93:80)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":93:103)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":93:126)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":93:56)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":49:43)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":91:40)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":92:40)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":165:65)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":165:68)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":165:58)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":166:56)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:58)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":9:39)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":59:40)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":60:45)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":61:42)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":60:43)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":61:44)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":78:44)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":19:17)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:16)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:32)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:55)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:75)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:9)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:30)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:16)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:23)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:9)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:20)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:16)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:9)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:20)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:16)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":12:9)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":71:17)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:16)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:32)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:48)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:9)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":109:9)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":110:9)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":112:9)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":113:9)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":114:9)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":117:9)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":130:21)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:27)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:17)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":130:12)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":131:13)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:14)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":133:13)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:14)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":135:13)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":137:13)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:21)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:27)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:17)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:12)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:20)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:13)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:14)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:20)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:13)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":123:14)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:20)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":124:13)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":125:16)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":125:9)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:15)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:20)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:21)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:27)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:15)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:17)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:12)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":106:13)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:14)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":108:13)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":109:14)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:13)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":112:13)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":113:16)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":113:9)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":114:9)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:21)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:27)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:15)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:17)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:12)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:21)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:14)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":146:21)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:14)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:21)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":150:21)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":151:16)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":151:9)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":152:9)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":155:9)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:9)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":157:9)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":158:9)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:17)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:12)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:33)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:29)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:50)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:13)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":63:13)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":64:13)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:16)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":68:9)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":54:20)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:26)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:32)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:48)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:17)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":56:9)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":74:20)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":76:17)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:17)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":77:9)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":78:9)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:24)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:16)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:9)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":172:12)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":173:20)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":173:13)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":174:15)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:19)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:12)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:39)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:20)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:13)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":177:17)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":178:12)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":178:28)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":178:44)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:39)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:20)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":179:13)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:16)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":180:9)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":185:18)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":184:21)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":184:16)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":184:9)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":185:12)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":186:13)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":190:24)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":190:12)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:35)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:13)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":191:23)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:35)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:13)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":192:23)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:20)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:20)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:16)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:17)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:9)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:9)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:20)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":30:17)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:12)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:20)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:16)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:40)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:29)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:17)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:20)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:16)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:40)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:29)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:17)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":40:13)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":14:20)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":16:9)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":17:16)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":23:16)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":28:13)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":16:13)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":17:9)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":22:13)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":23:9)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:13)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:13)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:19)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:31)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:47)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:19)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:31)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:47)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:29)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:29)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:29)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":33:29)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:38)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:51)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":34:59)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:38)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:51)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:59)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":36:24)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":37:24)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":48:17)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:27)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:38)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:48)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:66)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:16)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:9)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":52:15)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:25)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:12)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:25)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:13)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":74:13)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:17)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":76:9)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:27)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:38)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:48)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:66)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:16)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:9)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:18)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:21)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":66:19)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":66:9)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":67:9)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:9)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":80:16)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":82:19)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":84:13)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":43:22)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:22)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:34)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":46:9)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:21)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:33)
#loc251 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:9)
#loc252 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:22)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:34)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":48:9)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":49:9)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:26)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:17)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:23)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:16)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:26)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:26)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:15)
#loc265 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:28)
#loc266 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:19)
#loc267 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:9)
#loc268 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:29)
#loc269 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:45)
#loc270 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:17)
#loc271 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:16)
#loc272 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":25:12)
#loc273 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:12)
#loc274 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":31:33)
#loc275 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:13)
#loc276 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:29)
#loc277 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":65:17)
#loc278 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:9)
#loc279 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:19)
#loc280 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc281 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":68:20)
#loc282 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":68:9)
#loc283 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":71:9)
#loc284 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":82:17)
#loc285 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":83:20)
#loc286 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":83:9)
#loc287 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":86:9)
