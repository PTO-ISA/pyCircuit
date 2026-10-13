module attributes {acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, exports = {source = "source"}, format_version = 7 : i64, top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", type = "bool"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default, name = "lhs", type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", type = "bool"}, {default = false, name = "done", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}]}}} {
  func.func @ac_init_source_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_source_cap", params = [], result = "u32"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    return %c4_i32 : i32
  }
  func.func @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> attributes {acir.function = {kind = "helper", name = "ac_init_source", params = [], result = "vector<src_payloads_25ebcaa7.MicroOp>"}} {
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = acir.aggregate(%c1_i32, %c1_i32, %c2_i32, %c3_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %1 = acir.aggregate(%c0_i32, %c4_i32, %c1_i32, %c2_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %2 = acir.aggregate(%c0_i32, %c5_i32, %c2_i32, %c3_i32) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %3 = acir.aggregate(%c0_i32, %c6_i32, %c4_i32, %c5_i32) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %4 = acir.aggregate(%0, %1, %2, %3) {acir.id = "v22", acir.source_type = "vector<src_payloads_25ebcaa7.MicroOp>"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    return %4 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
  }
  func.func @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot0.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot1.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot2.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v43", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot3.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.head() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_rob.head", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_rob.tail() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_rob.tail", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_rf.registers() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.registers", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c6_i32, %c7_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v64", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8>
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_rf.renamed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.renamed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v74", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8>
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_rf.committed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.committed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v84", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8>
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_left.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v90", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1 = acir.aggregate() {acir.id = "v91", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v92", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_right.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v98", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1 = acir.aggregate() {acir.id = "v99", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v100", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_scheduler.add_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_scheduler.add_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">> attributes {acir.function = {kind = "helper", name = "ac_init_scheduler.add_requests", params = [], result = "vector<src_payloads_25ebcaa7.Entry>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Entry>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
  }
  func.func @ac_init_scheduler.mul_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_scheduler.mul_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">> attributes {acir.function = {kind = "helper", name = "ac_init_scheduler.mul_requests", params = [], result = "vector<src_payloads_25ebcaa7.Entry>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Entry>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
  }
  func.func @ac_init_adder.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_adder.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">> attributes {acir.function = {kind = "helper", name = "ac_init_adder.out", params = [], result = "vector<src_payloads_25ebcaa7.Completion>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Completion>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
  }
  func.func @ac_init_multiplier.active() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.active", params = [], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    return %false : i1
  }
  func.func @ac_init_multiplier.remaining() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.remaining", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.request", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v119", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1 = acir.aggregate() {acir.id = "v120", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v121", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_multiplier.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">> attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.out", params = [], result = "vector<src_payloads_25ebcaa7.Completion>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Completion>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
  }
  func.func @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion"> attributes {acir.function = {kind = "helper", name = "ac_init_cdb.event", params = [], result = "src_payloads_25ebcaa7.Completion"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32) {acir.id = "v129", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.Completion">
  }
  func.func @ac_init_commit.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_commit.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_commit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_commit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_audit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_audit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  acir.module @rob.slot0 attributes {acir.instance = {contract = "Module:rob.slot0", definition = "src_state_7e811fd9.RobSlot", inputs = {}, interfaces = {allocate = "Module:rob.slot0:allocate", complete = "Module:rob.slot0:complete", peek = "Module:rob.slot0:peek", retire = "Module:rob.slot0:retire"}, members = {state = {resources = ["rob.slot0.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}}, methods = ["allocate", "complete", "peek", "retire"], name = "rob.slot0", outputs = {resources = ["rob.slot0.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}, parent = "rob"}} {
    acir.interface @allocate {implementations = [@rob.slot0::@dispatch.dispatch.0.call.27.9.0.call.28.13, @rob.slot0::@ac_summary_rob.slot0.allocate.0.call.52.2, @rob.slot0::@ac_summary_rob.allocate.0.call.77.1.0.call.78.13], orders = []}
    acir.interface @complete {implementations = [@rob.slot0::@cdb.broadcast.0.call.8.16.0.call.8.11, @rob.slot0::@ac_summary_rob.slot0.complete.0.call.54.2, @rob.slot0::@ac_summary_rob.complete.0.call.92.2.0.call.92.11], orders = []}
    acir.interface @peek {implementations = [@rob.slot0::@dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.15.10, @rob.slot0::@dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.21.10, @rob.slot0::@ac_summary_rob.slot0.peek.0.call.55.0, @rob.slot0::@ac_summary_rob.lookup.0.call.87.1.0.call.87.10, @rob.slot0::@ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.106.10], orders = []}
    acir.interface @retire {implementations = [@rob.slot0::@commit.commit.0.call.42.2.0.call.42.11, @rob.slot0::@ac_summary_rob.slot0.retire.0.call.56.1, @rob.slot0::@ac_summary_rob.retire.0.call.97.0.0.call.97.11], orders = []}
    acir.cell @rob.slot0.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_rob.slot0.state", name = "rob.slot0.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.method @cdb.broadcast.0.call.8.16.0.call.8.11 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.16.0.call.8.11", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot0::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.15.10 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.15.10", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot0::@peek} body {
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.21.10 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.21.10", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot0::@peek} body {
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    func.func @dispatch.dispatch.0.call.27.9.0.call.28.13.0.call.28.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "dispatch.dispatch.0.call.27.9.0.call.28.13.0.call.28.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @dispatch.dispatch.0.call.27.9.0.call.28.13 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot0", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.9.0.call.28.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot0::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @dispatch.dispatch.0.call.27.9.0.call.28.13.0.call.28.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    func.func @commit.commit.0.call.42.2.0.call.42.11.0.call.42.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "commit.commit.0.call.42.2.0.call.42.11.0.call.42.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @commit.commit.0.call.42.2.0.call.42.11 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot0", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.2.0.call.42.11", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot0::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @commit.commit.0.call.42.2.0.call.42.11.0.call.42.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_rob.slot0.allocate.0.call.52.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot0", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot0.allocate.0.call.52.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob.slot0::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.slot0.complete.0.call.54.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot0.complete.0.call.54.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob.slot0::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.slot0.peek.0.call.55.0 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot0.peek.0.call.55.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob.slot0::@peek} body {
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @ac_summary_rob.slot0.retire.0.call.56.1 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot0", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot0.retire.0.call.56.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob.slot0::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.allocate.0.call.77.1.0.call.78.13.0.call.78.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.78.13.0.call.78.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_rob.allocate.0.call.77.1.0.call.78.13 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot0", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.78.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot0::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @ac_summary_rob.allocate.0.call.77.1.0.call.78.13.0.call.78.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.lookup.0.call.87.1.0.call.87.10 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.87.10", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot0::@peek} body {
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.complete.0.call.92.2.0.call.92.11 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.92.11", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot0::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    func.func @ac_summary_rob.retire.0.call.97.0.0.call.97.11.0.call.97.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.97.11.0.call.97.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @ac_summary_rob.retire.0.call.97.0.0.call.97.11 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot0", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.97.11", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot0::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.retire.0.call.97.0.0.call.97.11.0.call.97.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.106.10 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.106.10", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot0::@peek} body {
      %0 = acir.ref @rob.slot0::@rob.slot0.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
  }
  acir.module @rob.slot1 attributes {acir.instance = {contract = "Module:rob.slot1", definition = "src_state_7e811fd9.RobSlot", inputs = {}, interfaces = {allocate = "Module:rob.slot1:allocate", complete = "Module:rob.slot1:complete", peek = "Module:rob.slot1:peek", retire = "Module:rob.slot1:retire"}, members = {state = {resources = ["rob.slot1.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}}, methods = ["allocate", "complete", "peek", "retire"], name = "rob.slot1", outputs = {resources = ["rob.slot1.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}, parent = "rob"}} {
    acir.interface @allocate {implementations = [@rob.slot1::@dispatch.dispatch.0.call.27.9.0.call.30.17, @rob.slot1::@ac_summary_rob.slot1.allocate.0.call.58.2, @rob.slot1::@ac_summary_rob.allocate.0.call.77.1.0.call.80.17], orders = []}
    acir.interface @complete {implementations = [@rob.slot1::@cdb.broadcast.0.call.8.16.0.call.9.15, @rob.slot1::@ac_summary_rob.slot1.complete.0.call.60.2, @rob.slot1::@ac_summary_rob.complete.0.call.92.2.0.call.93.15], orders = []}
    acir.interface @peek {implementations = [@rob.slot1::@dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.16.15, @rob.slot1::@dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.22.15, @rob.slot1::@ac_summary_rob.slot1.peek.0.call.61.0, @rob.slot1::@ac_summary_rob.lookup.0.call.87.1.0.call.88.15, @rob.slot1::@ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.107.15], orders = []}
    acir.interface @retire {implementations = [@rob.slot1::@commit.commit.0.call.42.2.0.call.44.15, @rob.slot1::@ac_summary_rob.slot1.retire.0.call.62.1, @rob.slot1::@ac_summary_rob.retire.0.call.97.0.0.call.99.15], orders = []}
    acir.cell @rob.slot1.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_rob.slot1.state", name = "rob.slot1.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.method @cdb.broadcast.0.call.8.16.0.call.9.15 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.16.0.call.9.15", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot1::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.16.15 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.16.15", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot1::@peek} body {
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.22.15 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.22.15", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot1::@peek} body {
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    func.func @dispatch.dispatch.0.call.27.9.0.call.30.17.0.call.30.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "dispatch.dispatch.0.call.27.9.0.call.30.17.0.call.30.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @dispatch.dispatch.0.call.27.9.0.call.30.17 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot1", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.9.0.call.30.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot1::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @dispatch.dispatch.0.call.27.9.0.call.30.17.0.call.30.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    func.func @commit.commit.0.call.42.2.0.call.44.15.0.call.44.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "commit.commit.0.call.42.2.0.call.44.15.0.call.44.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @commit.commit.0.call.42.2.0.call.44.15 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot1", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.2.0.call.44.15", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot1::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @commit.commit.0.call.42.2.0.call.44.15.0.call.44.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_rob.slot1.allocate.0.call.58.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot1", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot1.allocate.0.call.58.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob.slot1::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.slot1.complete.0.call.60.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot1.complete.0.call.60.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob.slot1::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.slot1.peek.0.call.61.0 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot1.peek.0.call.61.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob.slot1::@peek} body {
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @ac_summary_rob.slot1.retire.0.call.62.1 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot1", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot1.retire.0.call.62.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob.slot1::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.allocate.0.call.77.1.0.call.80.17.0.call.80.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.80.17.0.call.80.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_rob.allocate.0.call.77.1.0.call.80.17 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot1", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.80.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot1::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @ac_summary_rob.allocate.0.call.77.1.0.call.80.17.0.call.80.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.lookup.0.call.87.1.0.call.88.15 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.88.15", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot1::@peek} body {
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.complete.0.call.92.2.0.call.93.15 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.93.15", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot1::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    func.func @ac_summary_rob.retire.0.call.97.0.0.call.99.15.0.call.99.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.99.15.0.call.99.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @ac_summary_rob.retire.0.call.97.0.0.call.99.15 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot1", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.99.15", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot1::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.retire.0.call.97.0.0.call.99.15.0.call.99.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.107.15 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.107.15", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot1::@peek} body {
      %0 = acir.ref @rob.slot1::@rob.slot1.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
  }
  acir.module @rob.slot2 attributes {acir.instance = {contract = "Module:rob.slot2", definition = "src_state_7e811fd9.RobSlot", inputs = {}, interfaces = {allocate = "Module:rob.slot2:allocate", complete = "Module:rob.slot2:complete", peek = "Module:rob.slot2:peek", retire = "Module:rob.slot2:retire"}, members = {state = {resources = ["rob.slot2.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}}, methods = ["allocate", "complete", "peek", "retire"], name = "rob.slot2", outputs = {resources = ["rob.slot2.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}, parent = "rob"}} {
    acir.interface @allocate {implementations = [@rob.slot2::@dispatch.dispatch.0.call.27.9.0.call.32.21, @rob.slot2::@ac_summary_rob.slot2.allocate.0.call.64.2, @rob.slot2::@ac_summary_rob.allocate.0.call.77.1.0.call.82.21], orders = []}
    acir.interface @complete {implementations = [@rob.slot2::@cdb.broadcast.0.call.8.16.0.call.10.19, @rob.slot2::@ac_summary_rob.slot2.complete.0.call.66.2, @rob.slot2::@ac_summary_rob.complete.0.call.92.2.0.call.94.19], orders = []}
    acir.interface @peek {implementations = [@rob.slot2::@dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.17.20, @rob.slot2::@dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.23.20, @rob.slot2::@ac_summary_rob.slot2.peek.0.call.67.0, @rob.slot2::@ac_summary_rob.lookup.0.call.87.1.0.call.89.20, @rob.slot2::@ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.108.20], orders = []}
    acir.interface @retire {implementations = [@rob.slot2::@commit.commit.0.call.42.2.0.call.46.19, @rob.slot2::@ac_summary_rob.slot2.retire.0.call.68.1, @rob.slot2::@ac_summary_rob.retire.0.call.97.0.0.call.101.19], orders = []}
    acir.cell @rob.slot2.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_rob.slot2.state", name = "rob.slot2.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.method @cdb.broadcast.0.call.8.16.0.call.10.19 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.16.0.call.10.19", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot2::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.17.20 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.17.20", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot2::@peek} body {
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.23.20 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.23.20", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot2::@peek} body {
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    func.func @dispatch.dispatch.0.call.27.9.0.call.32.21.0.call.32.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "dispatch.dispatch.0.call.27.9.0.call.32.21.0.call.32.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @dispatch.dispatch.0.call.27.9.0.call.32.21 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot2", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.9.0.call.32.21", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot2::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @dispatch.dispatch.0.call.27.9.0.call.32.21.0.call.32.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    func.func @commit.commit.0.call.42.2.0.call.46.19.0.call.46.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "commit.commit.0.call.42.2.0.call.46.19.0.call.46.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @commit.commit.0.call.42.2.0.call.46.19 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot2", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.2.0.call.46.19", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot2::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @commit.commit.0.call.42.2.0.call.46.19.0.call.46.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_rob.slot2.allocate.0.call.64.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot2", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot2.allocate.0.call.64.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob.slot2::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.slot2.complete.0.call.66.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot2.complete.0.call.66.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob.slot2::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.slot2.peek.0.call.67.0 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot2.peek.0.call.67.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob.slot2::@peek} body {
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @ac_summary_rob.slot2.retire.0.call.68.1 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot2", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot2.retire.0.call.68.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob.slot2::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.allocate.0.call.77.1.0.call.82.21.0.call.82.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.82.21.0.call.82.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_rob.allocate.0.call.77.1.0.call.82.21 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot2", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.82.21", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot2::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @ac_summary_rob.allocate.0.call.77.1.0.call.82.21.0.call.82.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.lookup.0.call.87.1.0.call.89.20 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.89.20", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot2::@peek} body {
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.complete.0.call.92.2.0.call.94.19 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.94.19", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot2::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    func.func @ac_summary_rob.retire.0.call.97.0.0.call.101.19.0.call.101.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.101.19.0.call.101.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @ac_summary_rob.retire.0.call.97.0.0.call.101.19 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot2", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.101.19", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot2::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.retire.0.call.97.0.0.call.101.19.0.call.101.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.108.20 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.108.20", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot2::@peek} body {
      %0 = acir.ref @rob.slot2::@rob.slot2.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
  }
  acir.module @rob.slot3 attributes {acir.instance = {contract = "Module:rob.slot3", definition = "src_state_7e811fd9.RobSlot", inputs = {}, interfaces = {allocate = "Module:rob.slot3:allocate", complete = "Module:rob.slot3:complete", peek = "Module:rob.slot3:peek", retire = "Module:rob.slot3:retire"}, members = {state = {resources = ["rob.slot3.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}}, methods = ["allocate", "complete", "peek", "retire"], name = "rob.slot3", outputs = {resources = ["rob.slot3.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}, parent = "rob"}} {
    acir.interface @allocate {implementations = [@rob.slot3::@dispatch.dispatch.0.call.27.9.0.call.34.22, @rob.slot3::@ac_summary_rob.slot3.allocate.0.call.70.2, @rob.slot3::@ac_summary_rob.allocate.0.call.77.1.0.call.84.22], orders = []}
    acir.interface @complete {implementations = [@rob.slot3::@cdb.broadcast.0.call.8.16.0.call.11.20, @rob.slot3::@ac_summary_rob.slot3.complete.0.call.72.2, @rob.slot3::@ac_summary_rob.complete.0.call.92.2.0.call.95.20], orders = []}
    acir.interface @peek {implementations = [@rob.slot3::@dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.18.31, @rob.slot3::@dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.24.31, @rob.slot3::@ac_summary_rob.slot3.peek.0.call.73.0, @rob.slot3::@ac_summary_rob.lookup.0.call.87.1.0.call.90.31, @rob.slot3::@ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.109.31], orders = []}
    acir.interface @retire {implementations = [@rob.slot3::@commit.commit.0.call.42.2.0.call.48.20, @rob.slot3::@ac_summary_rob.slot3.retire.0.call.74.1, @rob.slot3::@ac_summary_rob.retire.0.call.97.0.0.call.103.20], orders = []}
    acir.cell @rob.slot3.state : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {initial_fn = "ac_init_rob.slot3.state", name = "rob.slot3.state", type = "src_payloads_25ebcaa7.RobEntry"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.method @cdb.broadcast.0.call.8.16.0.call.11.20 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.16.0.call.11.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot3::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.18.31 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.18.31", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot3::@peek} body {
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.24.31 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.24.31", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot3::@peek} body {
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    func.func @dispatch.dispatch.0.call.27.9.0.call.34.22.0.call.34.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "dispatch.dispatch.0.call.27.9.0.call.34.22.0.call.34.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @dispatch.dispatch.0.call.27.9.0.call.34.22 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot3", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.9.0.call.34.22", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot3::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @dispatch.dispatch.0.call.27.9.0.call.34.22.0.call.34.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    func.func @commit.commit.0.call.42.2.0.call.48.20.0.call.48.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "commit.commit.0.call.42.2.0.call.48.20.0.call.48.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @commit.commit.0.call.42.2.0.call.48.20 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot3", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.2.0.call.48.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot3::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @commit.commit.0.call.42.2.0.call.48.20.0.call.48.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_rob.slot3.allocate.0.call.70.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot3", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot3.allocate.0.call.70.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob.slot3::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.slot3.complete.0.call.72.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot3.complete.0.call.72.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob.slot3::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.slot3.peek.0.call.73.0 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot3.peek.0.call.73.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob.slot3::@peek} body {
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @ac_summary_rob.slot3.retire.0.call.74.1 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot3", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot3.retire.0.call.74.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob.slot3::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.allocate.0.call.77.1.0.call.84.22.0.call.84.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.84.22.0.call.84.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_rob.allocate.0.call.77.1.0.call.84.22 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot3", definition = "allocate", guard_info = {name = "free", source = "function"}, instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.84.22", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot3::@allocate} guard {
    ^bb0(%arg0: i32, %arg1: i32):
      %0 = func.call @ac_summary_rob.allocate.0.call.77.1.0.call.84.22.0.call.84.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1
      acir.guard.yield %0 {acir.id = "v5", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v6", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %1 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.lookup.0.call.87.1.0.call.90.31 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.90.31", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot3::@peek} body {
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.complete.0.call.92.2.0.call.95.20 : (i32, i32) -> () {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.95.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob.slot3::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v9", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v16", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v17", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        %5 = acir.update %1, %true {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        %6 = acir.update %5, %arg1 {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        acir.cell.write %0, %6 {acir.id = "v27", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      acir.return {acir.source_type = "void"}
    }
    func.func @ac_summary_rob.retire.0.call.97.0.0.call.103.20.0.call.103.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.103.20.0.call.103.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      return %4 : i1
    }
    acir.method @ac_summary_rob.retire.0.call.97.0.0.call.103.20 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot3", definition = "retire", guard_info = {name = "completed", source = "function"}, instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.103.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot3::@retire} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.retire.0.call.97.0.0.call.103.20.0.call.103.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %2 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.cell.write %0, %2 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v14", acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.109.31 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.109.31", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob.slot3::@peek} body {
      %0 = acir.ref @rob.slot3::@rob.slot3.state {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v5", acir.source_type = "void"}
    }
  }
  acir.module @rob attributes {acir.instance = {contract = "Module:rob", definition = "src_state_7e811fd9.ReorderBuffer", inputs = {}, interfaces = {allocate = "Module:rob:allocate", complete = "Module:rob:complete", has_room = "Module:rob:has_room", lookup = "Module:rob:lookup", retire = "Module:rob:retire"}, members = {head = {resources = ["rob.head"], type = "cell<u32>", value}, slot0 = {instance = "rob.slot0"}, slot1 = {instance = "rob.slot1"}, slot2 = {instance = "rob.slot2"}, slot3 = {instance = "rob.slot3"}, tail = {resources = ["rob.tail"], type = "cell<u32>", value}}, methods = ["allocate", "complete", "has_room", "lookup", "retire"], name = "rob", outputs = [{resources = ["rob.head"], type = "cell<u32>", value}, {resources = ["rob.tail"], type = "cell<u32>", value}], parent = "top"}} {
    acir.instance @slot0 = @rob.slot0
    acir.instance @slot1 = @rob.slot1
    acir.instance @slot2 = @rob.slot2
    acir.instance @slot3 = @rob.slot3
    acir.interface @allocate {implementations = [@rob::@dispatch.dispatch.0.call.27.9, @rob::@ac_summary_rob.allocate.0.call.77.1], orders = []}
    acir.interface @complete {implementations = [@rob::@cdb.broadcast.0.call.8.16, @rob::@ac_summary_rob.complete.0.call.92.2], orders = []}
    acir.interface @has_room {implementations = [@rob::@ac_summary_rob.has_room.0.call.76.0], orders = []}
    acir.interface @lookup {implementations = [@rob::@dispatch.dispatch.0.call.15.5.0.call.15.33, @rob::@dispatch.dispatch.0.call.21.7.0.call.21.33, @rob::@ac_summary_rob.lookup.0.call.87.1, @rob::@ac_summary_rf.operand.0.call.106.1.0.call.106.33], orders = []}
    acir.interface @retire {implementations = [@rob::@commit.commit.0.call.42.2, @rob::@ac_summary_rob.retire.0.call.97.0], orders = []}
    acir.cell @rob.head : i32 {initial_fn = "ac_init_rob.head", name = "rob.head", type = "u32"}
    acir.cell @rob.tail : i32 {initial_fn = "ac_init_rob.tail", name = "rob.tail", type = "u32"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.method @cdb.broadcast.0.call.8.16 : (i32, i32) -> () {acir.function = {contract = "Module:rob", definition = "complete", instance = "rob", interface = "Module:rob:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.16", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @rob::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
      %1 = arith.andi %0, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32
      scf.if %2 {
        acir.method.call @rob.slot0::@cdb.broadcast.0.call.8.16.0.call.8.11(%arg0, %arg1) {acir.id = "v12", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:complete", method = "complete", role = "method"} : (i32, i32) -> ()
      } else {
        %3 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32
        scf.if %3 {
          acir.method.call @rob.slot1::@cdb.broadcast.0.call.8.16.0.call.9.15(%arg0, %arg1) {acir.id = "v16", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:complete", method = "complete", role = "method"} : (i32, i32) -> ()
        } else {
          %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32
          scf.if %4 {
            acir.method.call @rob.slot2::@cdb.broadcast.0.call.8.16.0.call.10.19(%arg0, %arg1) {acir.id = "v20", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:complete", method = "complete", role = "method"} : (i32, i32) -> ()
          } else {
            acir.method.call @rob.slot3::@cdb.broadcast.0.call.8.16.0.call.11.20(%arg0, %arg1) {acir.id = "v21", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:complete", method = "complete", role = "method"} : (i32, i32) -> ()
          }
        }
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.15.5.0.call.15.33 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "dispatch.dispatch.0.call.15.5.0.call.15.33", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob::@lookup} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
      %1 = arith.andi %0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.method.call @rob.slot0::@dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.15.10() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v12", acir.source_type = "void"}
    ^bb2:  // pred: ^bb0
      %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.method.call @rob.slot1::@dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.16.15() {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v17", acir.source_type = "void"}
    ^bb4:  // pred: ^bb2
      %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32
      cf.cond_br %6, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %7 = acir.method.call @rob.slot2::@dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.17.20() {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v22", acir.source_type = "void"}
    ^bb6:  // pred: ^bb4
      %8 = acir.method.call @rob.slot3::@dispatch.dispatch.0.call.15.5.0.call.15.33.0.call.18.31() {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v33", acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.21.7.0.call.21.33 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "dispatch.dispatch.0.call.21.7.0.call.21.33", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob::@lookup} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
      %1 = arith.andi %0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.method.call @rob.slot0::@dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.21.10() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v12", acir.source_type = "void"}
    ^bb2:  // pred: ^bb0
      %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.method.call @rob.slot1::@dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.22.15() {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v17", acir.source_type = "void"}
    ^bb4:  // pred: ^bb2
      %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32
      cf.cond_br %6, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %7 = acir.method.call @rob.slot2::@dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.23.20() {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v22", acir.source_type = "void"}
    ^bb6:  // pred: ^bb4
      %8 = acir.method.call @rob.slot3::@dispatch.dispatch.0.call.21.7.0.call.21.33.0.call.24.31() {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v33", acir.source_type = "void"}
    }
    func.func @dispatch.dispatch.0.call.27.9.0.call.27.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "room", instance = "rob", kind = "guard", name = "dispatch.dispatch.0.call.27.9.0.call.27.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
      %0 = acir.ref @rob::@rob.tail {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.ref @rob::@rob.head {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %3 = acir.cell.read %2 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %4 = arith.subi %1, %3 {acir.id = "v6", acir.source_type = "u32"} : i32
      %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
      return %5 : i1
    }
    acir.method @dispatch.dispatch.0.call.27.9 : (i32) -> i32 {acir.function = {contract = "Module:rob", definition = "allocate", guard_info = {name = "room", source = "function"}, instance = "rob", interface = "Module:rob:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.9", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "u32"}, interface_ref = @rob::@allocate} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @dispatch.dispatch.0.call.27.9.0.call.27.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @rob::@rob.tail {acir.id = "v5", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = arith.subi %1, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
      %3 = arith.andi %2, %c3_i32 {acir.id = "v10", acir.source_type = "u32"} : i32
      %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
      scf.if %4 {
        acir.method.call @rob.slot0::@dispatch.dispatch.0.call.27.9.0.call.28.13(%1, %arg0) {acir.id = "v14", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
      } else {
        %6 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v16", acir.source_type = "bool"} : i32
        scf.if %6 {
          acir.method.call @rob.slot1::@dispatch.dispatch.0.call.27.9.0.call.30.17(%1, %arg0) {acir.id = "v18", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
        } else {
          %7 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v20", acir.source_type = "bool"} : i32
          scf.if %7 {
            acir.method.call @rob.slot2::@dispatch.dispatch.0.call.27.9.0.call.32.21(%1, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
          } else {
            acir.method.call @rob.slot3::@dispatch.dispatch.0.call.27.9.0.call.34.22(%1, %arg0) {acir.id = "v23", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
          }
        }
      }
      %5 = arith.addi %1, %c1_i32 {acir.id = "v40", acir.source_type = "u32"} : i32
      acir.cell.write %0, %5 {acir.id = "v41", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return %1 : i32 {acir.id = "v42", acir.source_type = "void"}
    }
    acir.method @commit.commit.0.call.42.2 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob", definition = "retire", instance = "rob", interface = "Module:rob:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.2", params = [], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob::@retire} body {
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @rob::@rob.head {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = arith.subi %1, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
      %3 = arith.andi %2, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
      %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32
      %5 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.method.call @rob.slot0::@commit.commit.0.call.42.2.0.call.42.11(%1) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        %7 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
          %9 = acir.method.call @rob.slot1::@commit.commit.0.call.42.2.0.call.44.15(%1) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          scf.yield %9 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        } else {
          %9 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32
          %10 = scf.if %9 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
            %11 = acir.method.call @rob.slot2::@commit.commit.0.call.42.2.0.call.46.19(%1) {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
            scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          } else {
            %11 = acir.method.call @rob.slot3::@commit.commit.0.call.42.2.0.call.48.20(%1) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
            scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          }
          scf.yield %10 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        }
        scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %6 = arith.addi %1, %c1_i32 {acir.id = "v38", acir.source_type = "u32"} : i32
      acir.cell.write %0, %6 {acir.id = "v39", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v40", acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.has_room.0.call.76.0 : () -> i1 {acir.function = {contract = "Module:rob", definition = "has_room", instance = "rob", interface = "Module:rob:has_room", kind = "method", method = "has_room", name = "ac_summary_rob.has_room.0.call.76.0", params = [], result = "bool", summary = true}, interface_ref = @rob::@has_room} body {
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
      %0 = acir.ref @rob::@rob.tail {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.ref @rob::@rob.head {acir.id = "v5", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %3 = acir.cell.read %2 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %4 = arith.subi %1, %3 {acir.id = "v7", acir.source_type = "u32"} : i32
      %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
      acir.return %5 : i1 {acir.id = "v10", acir.source_type = "void"}
    }
    func.func @ac_summary_rob.allocate.0.call.77.1.0.call.77.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "room", instance = "rob", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.77.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
      %0 = acir.ref @rob::@rob.tail {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = acir.ref @rob::@rob.head {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %3 = acir.cell.read %2 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %4 = arith.subi %1, %3 {acir.id = "v6", acir.source_type = "u32"} : i32
      %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
      return %5 : i1
    }
    acir.method @ac_summary_rob.allocate.0.call.77.1 : (i32) -> i32 {acir.function = {contract = "Module:rob", definition = "allocate", guard_info = {name = "room", source = "function"}, instance = "rob", interface = "Module:rob:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "u32", summary = true}, interface_ref = @rob::@allocate} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_rob.allocate.0.call.77.1.0.call.77.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @rob::@rob.tail {acir.id = "v5", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = arith.subi %1, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
      %3 = arith.andi %2, %c3_i32 {acir.id = "v10", acir.source_type = "u32"} : i32
      %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
      scf.if %4 {
        acir.method.call @rob.slot0::@ac_summary_rob.allocate.0.call.77.1.0.call.78.13(%1, %arg0) {acir.id = "v14", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
      } else {
        %6 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v16", acir.source_type = "bool"} : i32
        scf.if %6 {
          acir.method.call @rob.slot1::@ac_summary_rob.allocate.0.call.77.1.0.call.80.17(%1, %arg0) {acir.id = "v18", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
        } else {
          %7 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v20", acir.source_type = "bool"} : i32
          scf.if %7 {
            acir.method.call @rob.slot2::@ac_summary_rob.allocate.0.call.77.1.0.call.82.21(%1, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
          } else {
            acir.method.call @rob.slot3::@ac_summary_rob.allocate.0.call.77.1.0.call.84.22(%1, %arg0) {acir.id = "v23", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
          }
        }
      }
      %5 = arith.addi %1, %c1_i32 {acir.id = "v40", acir.source_type = "u32"} : i32
      acir.cell.write %0, %5 {acir.id = "v41", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return %1 : i32 {acir.id = "v42", acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.lookup.0.call.87.1 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "ac_summary_rob.lookup.0.call.87.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob::@lookup} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
      %1 = arith.andi %0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.method.call @rob.slot0::@ac_summary_rob.lookup.0.call.87.1.0.call.87.10() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v12", acir.source_type = "void"}
    ^bb2:  // pred: ^bb0
      %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.method.call @rob.slot1::@ac_summary_rob.lookup.0.call.87.1.0.call.88.15() {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v17", acir.source_type = "void"}
    ^bb4:  // pred: ^bb2
      %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32
      cf.cond_br %6, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %7 = acir.method.call @rob.slot2::@ac_summary_rob.lookup.0.call.87.1.0.call.89.20() {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v22", acir.source_type = "void"}
    ^bb6:  // pred: ^bb4
      %8 = acir.method.call @rob.slot3::@ac_summary_rob.lookup.0.call.87.1.0.call.90.31() {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v33", acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.complete.0.call.92.2 : (i32, i32) -> () {acir.function = {contract = "Module:rob", definition = "complete", instance = "rob", interface = "Module:rob:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rob::@complete} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
      %1 = arith.andi %0, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32
      scf.if %2 {
        acir.method.call @rob.slot0::@ac_summary_rob.complete.0.call.92.2.0.call.92.11(%arg0, %arg1) {acir.id = "v12", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:complete", method = "complete", role = "method"} : (i32, i32) -> ()
      } else {
        %3 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32
        scf.if %3 {
          acir.method.call @rob.slot1::@ac_summary_rob.complete.0.call.92.2.0.call.93.15(%arg0, %arg1) {acir.id = "v16", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:complete", method = "complete", role = "method"} : (i32, i32) -> ()
        } else {
          %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32
          scf.if %4 {
            acir.method.call @rob.slot2::@ac_summary_rob.complete.0.call.92.2.0.call.94.19(%arg0, %arg1) {acir.id = "v20", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:complete", method = "complete", role = "method"} : (i32, i32) -> ()
          } else {
            acir.method.call @rob.slot3::@ac_summary_rob.complete.0.call.92.2.0.call.95.20(%arg0, %arg1) {acir.id = "v21", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:complete", method = "complete", role = "method"} : (i32, i32) -> ()
          }
        }
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rob.retire.0.call.97.0 : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob", definition = "retire", instance = "rob", interface = "Module:rob:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}, interface_ref = @rob::@retire} body {
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.ref @rob::@rob.head {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = arith.subi %1, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
      %3 = arith.andi %2, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
      %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32
      %5 = scf.if %4 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
        %7 = acir.method.call @rob.slot0::@ac_summary_rob.retire.0.call.97.0.0.call.97.11(%1) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      } else {
        %7 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
          %9 = acir.method.call @rob.slot1::@ac_summary_rob.retire.0.call.97.0.0.call.99.15(%1) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          scf.yield %9 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        } else {
          %9 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v18", acir.source_type = "bool"} : i32
          %10 = scf.if %9 -> (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) {
            %11 = acir.method.call @rob.slot2::@ac_summary_rob.retire.0.call.97.0.0.call.101.19(%1) {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
            scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          } else {
            %11 = acir.method.call @rob.slot3::@ac_summary_rob.retire.0.call.97.0.0.call.103.20(%1) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
            scf.yield %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
          }
          scf.yield %10 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
        }
        scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      }
      %6 = arith.addi %1, %c1_i32 {acir.id = "v38", acir.source_type = "u32"} : i32
      acir.cell.write %0, %6 {acir.id = "v39", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v40", acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.operand.0.call.106.1.0.call.106.33 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.33", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}, interface_ref = @rob::@lookup} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = arith.subi %arg0, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
      %1 = arith.andi %0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %3 = acir.method.call @rob.slot0::@ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.106.10() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v12", acir.source_type = "void"}
    ^bb2:  // pred: ^bb0
      %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v14", acir.source_type = "bool"} : i32
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.method.call @rob.slot1::@ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.107.15() {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v17", acir.source_type = "void"}
    ^bb4:  // pred: ^bb2
      %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32
      cf.cond_br %6, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %7 = acir.method.call @rob.slot2::@ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.108.20() {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v22", acir.source_type = "void"}
    ^bb6:  // pred: ^bb4
      %8 = acir.method.call @rob.slot3::@ac_summary_rf.operand.0.call.106.1.0.call.106.33.0.call.109.31() {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> {acir.id = "v33", acir.source_type = "void"}
    }
  }
  acir.module @rf attributes {acir.instance = {contract = "Module:rf", definition = "src_state_7e811fd9.RegisterFile", inputs = {rob = {instance = "rob"}}, interfaces = {commit = "Module:rf:commit", operand = "Module:rf:operand", rename = "Module:rf:rename"}, members = {committed = {resources = ["rf.committed"], type = "cell<array<u32,8>>", value}, registers = {resources = ["rf.registers"], type = "cell<array<u32,8>>", value}, renamed = {resources = ["rf.renamed"], type = "cell<array<u32,8>>", value}, rob = {instance = "rob"}}, methods = ["commit", "operand", "rename"], name = "rf", outputs = [{resources = ["rf.registers"], type = "cell<array<u32,8>>", value}, {resources = ["rf.renamed"], type = "cell<array<u32,8>>", value}, {resources = ["rf.committed"], type = "cell<array<u32,8>>", value}], parent = "top"}} {
    acir.instance @rob = @rob
    acir.interface @commit {implementations = [@rf::@commit.commit.0.call.51.3, @rf::@ac_summary_rf.commit.0.call.113.1], orders = []}
    acir.interface @operand {implementations = [@rf::@dispatch.dispatch.0.call.15.5, @rf::@dispatch.dispatch.0.call.21.7, @rf::@ac_summary_rf.operand.0.call.106.1], orders = []}
    acir.interface @rename {implementations = [@rf::@dispatch.dispatch.0.call.37.11, @rf::@ac_summary_rf.rename.0.call.112.2], orders = []}
    acir.cell @rf.registers : !acir.array<i32, 8> {initial_fn = "ac_init_rf.registers", name = "rf.registers", type = "array<u32,8>"}
    acir.cell @rf.renamed : !acir.array<i32, 8> {initial_fn = "ac_init_rf.renamed", name = "rf.renamed", type = "array<u32,8>"}
    acir.cell @rf.committed : !acir.array<i32, 8> {initial_fn = "ac_init_rf.committed", name = "rf.committed", type = "array<u32,8>"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.method @dispatch.dispatch.0.call.15.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "dispatch.dispatch.0.call.15.5", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand"}, interface_ref = @rf::@operand} body {
    ^bb0(%arg0: i32):
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32
      acir.check %0 {acir.id = "v6", acir.source_type = "void"}
      %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
      %2 = acir.ref @rf::@rf.renamed {acir.id = "v17", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %3 = acir.ref @rf::@rf.committed {acir.id = "v20", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %4 = acir.ref @rf::@rf.registers {acir.id = "v26", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v14", acir.source_type = "void"}
    ^bb2(%6: i32):  // pred: ^bb0
      %7 = acir.cell.read %2 {acir.id = "v18", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %8 = acir.extract %7[%6] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %9 = acir.cell.read %3 {acir.id = "v21", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %10 = acir.extract %9[%6] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %11 = arith.cmpi eq, %8, %10 {acir.id = "v23", acir.source_type = "bool"} : i32
      cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
    ^bb3:  // pred: ^bb2
      %12 = acir.cell.read %4 {acir.id = "v27", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %13 = acir.extract %12[%6] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %14 = acir.aggregate(%true, %8, %13) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v30", acir.source_type = "void"}
    ^bb4(%15: i32):  // pred: ^bb2
      %16 = acir.method.call @rob::@dispatch.dispatch.0.call.15.5.0.call.15.33(%15) {acir.id = "v34", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %17 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      cf.cond_br %17, ^bb5, ^bb6(%false : i1)
    ^bb5:  // pred: ^bb4
      %18 = acir.extract %16 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      cf.br ^bb6(%18 : i1)
    ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
      cf.cond_br %19, ^bb7, ^bb8(%false : i1)
    ^bb7:  // pred: ^bb6
      %20 = acir.extract %16 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      %21 = arith.cmpi eq, %20, %15 {acir.id = "v44", acir.source_type = "bool"} : i32
      cf.br ^bb8(%21 : i1)
    ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
      cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
    ^bb9:  // pred: ^bb8
      %23 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      %24 = acir.aggregate(%true, %15, %23) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v53", acir.source_type = "void"}
    ^bb10(%25: i32):  // pred: ^bb8
      %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v61", acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.21.7 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "dispatch.dispatch.0.call.21.7", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand"}, interface_ref = @rf::@operand} body {
    ^bb0(%arg0: i32):
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32
      acir.check %0 {acir.id = "v6", acir.source_type = "void"}
      %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
      %2 = acir.ref @rf::@rf.renamed {acir.id = "v17", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %3 = acir.ref @rf::@rf.committed {acir.id = "v20", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %4 = acir.ref @rf::@rf.registers {acir.id = "v26", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v14", acir.source_type = "void"}
    ^bb2(%6: i32):  // pred: ^bb0
      %7 = acir.cell.read %2 {acir.id = "v18", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %8 = acir.extract %7[%6] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %9 = acir.cell.read %3 {acir.id = "v21", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %10 = acir.extract %9[%6] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %11 = arith.cmpi eq, %8, %10 {acir.id = "v23", acir.source_type = "bool"} : i32
      cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
    ^bb3:  // pred: ^bb2
      %12 = acir.cell.read %4 {acir.id = "v27", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %13 = acir.extract %12[%6] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %14 = acir.aggregate(%true, %8, %13) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v30", acir.source_type = "void"}
    ^bb4(%15: i32):  // pred: ^bb2
      %16 = acir.method.call @rob::@dispatch.dispatch.0.call.21.7.0.call.21.33(%15) {acir.id = "v34", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %17 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      cf.cond_br %17, ^bb5, ^bb6(%false : i1)
    ^bb5:  // pred: ^bb4
      %18 = acir.extract %16 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      cf.br ^bb6(%18 : i1)
    ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
      cf.cond_br %19, ^bb7, ^bb8(%false : i1)
    ^bb7:  // pred: ^bb6
      %20 = acir.extract %16 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      %21 = arith.cmpi eq, %20, %15 {acir.id = "v44", acir.source_type = "bool"} : i32
      cf.br ^bb8(%21 : i1)
    ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
      cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
    ^bb9:  // pred: ^bb8
      %23 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      %24 = acir.aggregate(%true, %15, %23) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v53", acir.source_type = "void"}
    ^bb10(%25: i32):  // pred: ^bb8
      %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v61", acir.source_type = "void"}
    }
    acir.method @dispatch.dispatch.0.call.37.11 : (i32, i32) -> () {acir.function = {contract = "Module:rf", definition = "rename", instance = "rf", interface = "Module:rf:rename", kind = "method", method = "rename", name = "dispatch.dispatch.0.call.37.11", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}, {consume = false, id = "v2", name = "tag", targets = [], type = "u32"}], result = "void"}, interface_ref = @rf::@rename} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
      acir.check %0 {acir.id = "v7", acir.source_type = "void"}
      %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
      %2 = acir.ref @rf::@rf.renamed {acir.id = "v11", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      scf.if %1 {
        %3 = acir.cell.read %2 {acir.id = "v12", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
        %4 = acir.update %3, %arg1[%arg0] {acir.id = "v13", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
        acir.cell.write %2, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @commit.commit.0.call.51.3 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "commit.commit.0.call.51.3", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], result = "void"}, interface_ref = @rf::@commit} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
      %2 = acir.ref @rf::@rf.registers {acir.id = "v8", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %3 = acir.ref @rf::@rf.committed {acir.id = "v14", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      scf.if %1 {
        %4 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %5 = acir.extract %arg0 {acir.id = "v10", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = acir.cell.read %2 {acir.id = "v11", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
        %7 = acir.update %6, %4[%5] {acir.id = "v12", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
        acir.cell.write %2, %7 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
        %8 = acir.extract %arg0 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %9 = acir.extract %arg0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %10 = acir.cell.read %3 {acir.id = "v17", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
        %11 = acir.update %10, %8[%9] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
        acir.cell.write %3, %11 {acir.id = "v19", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.operand.0.call.106.1 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "ac_summary_rf.operand.0.call.106.1", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand", summary = true}, interface_ref = @rf::@operand} body {
    ^bb0(%arg0: i32):
      %false = arith.constant {acir.source_type = "bool"} false
      %true = arith.constant {acir.source_type = "bool"} true
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v5", acir.source_type = "bool"} : i32
      acir.check %0 {acir.id = "v6", acir.source_type = "void"}
      %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
      %2 = acir.ref @rf::@rf.renamed {acir.id = "v17", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %3 = acir.ref @rf::@rf.committed {acir.id = "v20", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %4 = acir.ref @rf::@rf.registers {acir.id = "v26", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v14", acir.source_type = "void"}
    ^bb2(%6: i32):  // pred: ^bb0
      %7 = acir.cell.read %2 {acir.id = "v18", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %8 = acir.extract %7[%6] {acir.id = "v19", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %9 = acir.cell.read %3 {acir.id = "v21", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %10 = acir.extract %9[%6] {acir.id = "v22", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %11 = arith.cmpi eq, %8, %10 {acir.id = "v23", acir.source_type = "bool"} : i32
      cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
    ^bb3:  // pred: ^bb2
      %12 = acir.cell.read %4 {acir.id = "v27", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
      %13 = acir.extract %12[%6] {acir.id = "v28", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
      %14 = acir.aggregate(%true, %8, %13) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v30", acir.source_type = "void"}
    ^bb4(%15: i32):  // pred: ^bb2
      %16 = acir.method.call @rob::@ac_summary_rf.operand.0.call.106.1.0.call.106.33(%15) {acir.id = "v34", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      %17 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      cf.cond_br %17, ^bb5, ^bb6(%false : i1)
    ^bb5:  // pred: ^bb4
      %18 = acir.extract %16 {acir.id = "v37", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
      cf.br ^bb6(%18 : i1)
    ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
      cf.cond_br %19, ^bb7, ^bb8(%false : i1)
    ^bb7:  // pred: ^bb6
      %20 = acir.extract %16 {acir.id = "v43", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      %21 = arith.cmpi eq, %20, %15 {acir.id = "v44", acir.source_type = "bool"} : i32
      cf.br ^bb8(%21 : i1)
    ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
      cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
    ^bb9:  // pred: ^bb8
      %23 = acir.extract %16 {acir.id = "v51", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      %24 = acir.aggregate(%true, %15, %23) {acir.id = "v52", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v53", acir.source_type = "void"}
    ^bb10(%25: i32):  // pred: ^bb8
      %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v60", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      acir.return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> {acir.id = "v61", acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.rename.0.call.112.2 : (i32, i32) -> () {acir.function = {contract = "Module:rf", definition = "rename", instance = "rf", interface = "Module:rf:rename", kind = "method", method = "rename", name = "ac_summary_rf.rename.0.call.112.2", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}, {consume = false, id = "v2", name = "tag", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @rf::@rename} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
      %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
      acir.check %0 {acir.id = "v7", acir.source_type = "void"}
      %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
      %2 = acir.ref @rf::@rf.renamed {acir.id = "v11", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      scf.if %1 {
        %3 = acir.cell.read %2 {acir.id = "v12", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
        %4 = acir.update %3, %arg1[%arg0] {acir.id = "v13", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
        acir.cell.write %2, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.commit.0.call.113.1 : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "ac_summary_rf.commit.0.call.113.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], result = "void", summary = true}, interface_ref = @rf::@commit} body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
      %2 = acir.ref @rf::@rf.registers {acir.id = "v8", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      %3 = acir.ref @rf::@rf.committed {acir.id = "v14", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
      scf.if %1 {
        %4 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %5 = acir.extract %arg0 {acir.id = "v10", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %6 = acir.cell.read %2 {acir.id = "v11", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
        %7 = acir.update %6, %4[%5] {acir.id = "v12", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
        acir.cell.write %2, %7 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
        %8 = acir.extract %arg0 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %9 = acir.extract %arg0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
        %10 = acir.cell.read %3 {acir.id = "v17", acir.source_type = "array<u32,8>", role = "observe"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
        %11 = acir.update %10, %8[%9] {acir.id = "v18", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
        acir.cell.write %3, %11 {acir.id = "v19", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @left attributes {acir.instance = {contract = "Module:left", definition = "src_state_7e811fd9.ReservationStation", inputs = {}, interfaces = {place = "Module:left:place", take = "Module:left:take", wakeup = "Module:left:wakeup"}, members = {slot = {resources = ["left.slot"], type = "cell<src_payloads_25ebcaa7.Entry>", value}}, methods = ["place", "take", "wakeup"], name = "left", outputs = {resources = ["left.slot"], type = "cell<src_payloads_25ebcaa7.Entry>", value}, parent = "top"}} {
    acir.interface @place {implementations = [@left::@dispatch.dispatch.0.call.38.21, @left::@ac_summary_left.place.0.call.114.1], orders = []}
    acir.interface @take {implementations = [@left::@scheduler.issue_add.0.call.0.5, @left::@scheduler.issue_mul.0.call.4.5, @left::@ac_summary_left.take.0.call.116.1], orders = []}
    acir.interface @wakeup {implementations = [@left::@cdb.broadcast.0.call.13.19, @left::@ac_summary_left.wakeup.0.call.118.2], orders = []}
    acir.cell @left.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_left.slot", name = "left.slot", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    func.func @scheduler.issue_add.0.call.0.5.0.call.0.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "scheduler.issue_add.0.call.0.5.0.call.0.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %5 = scf.if %4 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      return %5 : i1
    }
    acir.method @scheduler.issue_add.0.call.0.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:left", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "scheduler.issue_add.0.call.0.5", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}, interface_ref = @left::@take} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @scheduler.issue_add.0.call.0.5.0.call.0.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.cell.write %0, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v15", acir.source_type = "void"}
    }
    func.func @scheduler.issue_mul.0.call.4.5.0.call.4.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "scheduler.issue_mul.0.call.4.5.0.call.4.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %5 = scf.if %4 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      return %5 : i1
    }
    acir.method @scheduler.issue_mul.0.call.4.5 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:left", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "scheduler.issue_mul.0.call.4.5", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}, interface_ref = @left::@take} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @scheduler.issue_mul.0.call.4.5.0.call.4.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.cell.write %0, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v15", acir.source_type = "void"}
    }
    acir.method @cdb.broadcast.0.call.13.19 : (i32, i32) -> () {acir.function = {contract = "Module:left", definition = "wakeup", instance = "left", interface = "Module:left:wakeup", kind = "method", method = "wakeup", name = "cdb.broadcast.0.call.13.19", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @left::@wakeup} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1
        %7 = scf.if %6 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.extract %15 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.update %1, %15 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %9 = acir.extract %8 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %10 = acir.extract %9 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        %11 = acir.unary %10 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1
        %12 = scf.if %11 -> (i1) {
          %15 = acir.extract %8 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.extract %15 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v39", acir.source_type = "bool"} : i32
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %13 = arith.select %12, %true, %7 : i1
        %14 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.update %8, %15 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %13, %14 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      scf.if %3#0 {
        acir.cell.write %0, %3#1 {acir.id = "v63", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      acir.return {acir.source_type = "void"}
    }
    func.func @dispatch.dispatch.0.call.38.21.0.call.38.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "left", kind = "guard", name = "dispatch.dispatch.0.call.38.21.0.call.38.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
      %0 = acir.ref @left::@left.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @dispatch.dispatch.0.call.38.21 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:left", definition = "place", guard_info = {name = "free", source = "function"}, instance = "left", interface = "Module:left:place", kind = "method", method = "place", name = "dispatch.dispatch.0.call.38.21", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void"}, interface_ref = @left::@place} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = func.call @dispatch.dispatch.0.call.38.21.0.call.38.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.ref @left::@left.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return {acir.source_type = "void"}
    }
    func.func @ac_summary_left.place.0.call.114.1.0.call.114.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "left", kind = "guard", name = "ac_summary_left.place.0.call.114.1.0.call.114.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
      %0 = acir.ref @left::@left.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_left.place.0.call.114.1 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:left", definition = "place", guard_info = {name = "free", source = "function"}, instance = "left", interface = "Module:left:place", kind = "method", method = "place", name = "ac_summary_left.place.0.call.114.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void", summary = true}, interface_ref = @left::@place} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = func.call @ac_summary_left.place.0.call.114.1.0.call.114.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.ref @left::@left.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return {acir.source_type = "void"}
    }
    func.func @ac_summary_left.take.0.call.116.1.0.call.116.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "ac_summary_left.take.0.call.116.1.0.call.116.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %5 = scf.if %4 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      return %5 : i1
    }
    acir.method @ac_summary_left.take.0.call.116.1 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:left", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "ac_summary_left.take.0.call.116.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry", summary = true}, interface_ref = @left::@take} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_left.take.0.call.116.1.0.call.116.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.cell.write %0, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v15", acir.source_type = "void"}
    }
    acir.method @ac_summary_left.wakeup.0.call.118.2 : (i32, i32) -> () {acir.function = {contract = "Module:left", definition = "wakeup", instance = "left", interface = "Module:left:wakeup", kind = "method", method = "wakeup", name = "ac_summary_left.wakeup.0.call.118.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @left::@wakeup} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1
        %7 = scf.if %6 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.extract %15 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.update %1, %15 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %9 = acir.extract %8 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %10 = acir.extract %9 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        %11 = acir.unary %10 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1
        %12 = scf.if %11 -> (i1) {
          %15 = acir.extract %8 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.extract %15 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v39", acir.source_type = "bool"} : i32
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %13 = arith.select %12, %true, %7 : i1
        %14 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.update %8, %15 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %13, %14 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      scf.if %3#0 {
        acir.cell.write %0, %3#1 {acir.id = "v63", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @right attributes {acir.instance = {contract = "Module:right", definition = "src_state_7e811fd9.ReservationStation", inputs = {}, interfaces = {place = "Module:right:place", take = "Module:right:take", wakeup = "Module:right:wakeup"}, members = {slot = {resources = ["right.slot"], type = "cell<src_payloads_25ebcaa7.Entry>", value}}, methods = ["place", "take", "wakeup"], name = "right", outputs = {resources = ["right.slot"], type = "cell<src_payloads_25ebcaa7.Entry>", value}, parent = "top"}} {
    acir.interface @place {implementations = [@right::@dispatch.dispatch.0.call.40.22, @right::@ac_summary_right.place.0.call.119.1], orders = []}
    acir.interface @take {implementations = [@right::@scheduler.issue_add.0.call.2.10, @right::@scheduler.issue_mul.0.call.6.10, @right::@ac_summary_right.take.0.call.121.1], orders = []}
    acir.interface @wakeup {implementations = [@right::@cdb.broadcast.0.call.14.22, @right::@ac_summary_right.wakeup.0.call.123.2], orders = []}
    acir.cell @right.slot : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_right.slot", name = "right.slot", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    func.func @scheduler.issue_add.0.call.2.10.0.call.2.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "scheduler.issue_add.0.call.2.10.0.call.2.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @right::@right.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %5 = scf.if %4 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      return %5 : i1
    }
    acir.method @scheduler.issue_add.0.call.2.10 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:right", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "scheduler.issue_add.0.call.2.10", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}, interface_ref = @right::@take} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @scheduler.issue_add.0.call.2.10.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @right::@right.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.cell.write %0, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v15", acir.source_type = "void"}
    }
    func.func @scheduler.issue_mul.0.call.6.10.0.call.6.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "scheduler.issue_mul.0.call.6.10.0.call.6.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @right::@right.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %5 = scf.if %4 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      return %5 : i1
    }
    acir.method @scheduler.issue_mul.0.call.6.10 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:right", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "scheduler.issue_mul.0.call.6.10", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}, interface_ref = @right::@take} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @scheduler.issue_mul.0.call.6.10.0.call.6.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @right::@right.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.cell.write %0, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v15", acir.source_type = "void"}
    }
    acir.method @cdb.broadcast.0.call.14.22 : (i32, i32) -> () {acir.function = {contract = "Module:right", definition = "wakeup", instance = "right", interface = "Module:right:wakeup", kind = "method", method = "wakeup", name = "cdb.broadcast.0.call.14.22", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @right::@wakeup} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @right::@right.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1
        %7 = scf.if %6 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.extract %15 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.update %1, %15 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %9 = acir.extract %8 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %10 = acir.extract %9 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        %11 = acir.unary %10 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1
        %12 = scf.if %11 -> (i1) {
          %15 = acir.extract %8 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.extract %15 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v39", acir.source_type = "bool"} : i32
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %13 = arith.select %12, %true, %7 : i1
        %14 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.update %8, %15 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %13, %14 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      scf.if %3#0 {
        acir.cell.write %0, %3#1 {acir.id = "v63", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      acir.return {acir.source_type = "void"}
    }
    func.func @dispatch.dispatch.0.call.40.22.0.call.40.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "right", kind = "guard", name = "dispatch.dispatch.0.call.40.22.0.call.40.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
      %0 = acir.ref @right::@right.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @dispatch.dispatch.0.call.40.22 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:right", definition = "place", guard_info = {name = "free", source = "function"}, instance = "right", interface = "Module:right:place", kind = "method", method = "place", name = "dispatch.dispatch.0.call.40.22", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void"}, interface_ref = @right::@place} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = func.call @dispatch.dispatch.0.call.40.22.0.call.40.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.ref @right::@right.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return {acir.source_type = "void"}
    }
    func.func @ac_summary_right.place.0.call.119.1.0.call.119.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "right", kind = "guard", name = "ac_summary_right.place.0.call.119.1.0.call.119.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
      %0 = acir.ref @right::@right.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1
      return %3 : i1
    }
    acir.method @ac_summary_right.place.0.call.119.1 : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () {acir.function = {contract = "Module:right", definition = "place", guard_info = {name = "free", source = "function"}, instance = "right", interface = "Module:right:place", kind = "method", method = "place", name = "ac_summary_right.place.0.call.119.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void", summary = true}, interface_ref = @right::@place} guard {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = func.call @ac_summary_right.place.0.call.119.1.0.call.119.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">):
      %0 = acir.ref @right::@right.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return {acir.source_type = "void"}
    }
    func.func @ac_summary_right.take.0.call.121.1.0.call.121.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "ac_summary_right.take.0.call.121.1.0.call.121.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @right::@right.slot {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3 = scf.if %2 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      %5 = scf.if %4 -> (i1) {
        %6 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %7 = acir.extract %6 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      return %5 : i1
    }
    acir.method @ac_summary_right.take.0.call.121.1 : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.function = {contract = "Module:right", definition = "take", guard_info = {name = "issuable", source = "function"}, instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "ac_summary_right.take.0.call.121.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry", summary = true}, interface_ref = @right::@take} guard {
    ^bb0(%arg0: i32):
      %0 = func.call @ac_summary_right.take.0.call.121.1.0.call.121.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1
      acir.guard.yield %0 {acir.id = "v4", acir.source_type = "void"}
    } body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @right::@right.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %3 = acir.aggregate() {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %4 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %2, %3) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.cell.write %0, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v15", acir.source_type = "void"}
    }
    acir.method @ac_summary_right.wakeup.0.call.123.2 : (i32, i32) -> () {acir.function = {contract = "Module:right", definition = "wakeup", instance = "right", interface = "Module:right:wakeup", kind = "method", method = "wakeup", name = "ac_summary_right.wakeup.0.call.123.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @right::@wakeup} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @right::@right.slot {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %3:2 = scf.if %2 -> (i1, !acir.struct<"src_payloads_25ebcaa7.Entry">) {
        %4 = acir.extract %1 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %5 = acir.extract %4 {acir.id = "v11", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        %6 = acir.unary %5 "not" {acir.id = "v12", acir.source_type = "bool"} : i1 -> i1
        %7 = scf.if %6 -> (i1) {
          %15 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.extract %15 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v16", acir.source_type = "bool"} : i32
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %8 = scf.if %7 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.update %1, %15 {acir.id = "v25", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %1 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        %9 = acir.extract %8 {acir.id = "v33", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %10 = acir.extract %9 {acir.id = "v34", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        %11 = acir.unary %10 "not" {acir.id = "v35", acir.source_type = "bool"} : i1 -> i1
        %12 = scf.if %11 -> (i1) {
          %15 = acir.extract %8 {acir.id = "v37", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.extract %15 {acir.id = "v38", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
          %17 = arith.cmpi eq, %16, %arg0 {acir.id = "v39", acir.source_type = "bool"} : i32
          scf.yield %17 : i1
        } else {
          scf.yield %false : i1
        }
        %13 = arith.select %12, %true, %7 : i1
        %14 = scf.if %12 -> (!acir.struct<"src_payloads_25ebcaa7.Entry">) {
          %15 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v47", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
          %16 = acir.update %8, %15 {acir.id = "v48", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
          scf.yield %16 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        } else {
          scf.yield %8 : !acir.struct<"src_payloads_25ebcaa7.Entry">
        }
        scf.yield %13, %14 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      } else {
        scf.yield %false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      scf.if %3#0 {
        acir.cell.write %0, %3#1 {acir.id = "v63", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @scheduler attributes {acir.instance = {contract = "Module:scheduler", definition = "src_execution_885f3671.Scheduler", inputs = {left = {instance = "left"}, right = {instance = "right"}}, interfaces = {}, members = {add_requests = {resources = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}, left = {instance = "left"}, mul_requests = {resources = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}, right = {instance = "right"}}, methods = [], name = "scheduler", outputs = [{resources = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}, {resources = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}], parent = "top"}} {
    acir.instance @left = @left
    acir.instance @right = @right
    acir.queue @scheduler.add_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity_fn = "ac_init_scheduler.add_requests_cap", initial_fn = "ac_init_scheduler.add_requests", name = "scheduler.add_requests", type = "src_payloads_25ebcaa7.Entry"}
    acir.queue @scheduler.mul_requests : !acir.struct<"src_payloads_25ebcaa7.Entry"> {capacity_fn = "ac_init_scheduler.mul_requests_cap", initial_fn = "ac_init_scheduler.mul_requests", name = "scheduler.mul_requests", type = "src_payloads_25ebcaa7.Entry"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.rule @scheduler.issue_add : (i1) -> () {acir.function = {kind = "rule", name = "scheduler.issue_add", owner = "scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], result = "void", rule = "issue_add"}, output_types = [!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>]} body {
    ^bb0(%arg0: i1):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      cf.cond_br %arg0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %0 = acir.method.call @left::@scheduler.issue_add.0.call.0.5(%c0_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:left", instance = "left", interface = "Module:left:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v7", acir.source_type = "void", slots = [0]}
    ^bb2:  // pred: ^bb0
      %1 = acir.method.call @right::@scheduler.issue_add.0.call.2.10(%c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:right", instance = "right", interface = "Module:right:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v12", acir.source_type = "void", slots = [0]}
    }
    acir.rule @scheduler.issue_mul : (i1) -> () {acir.function = {kind = "rule", name = "scheduler.issue_mul", owner = "scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], result = "void", rule = "issue_mul"}, output_types = [!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>]} body {
    ^bb0(%arg0: i1):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      cf.cond_br %arg0, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %0 = acir.method.call @left::@scheduler.issue_mul.0.call.4.5(%c1_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:left", instance = "left", interface = "Module:left:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v7", acir.source_type = "void", slots = [0]}
    ^bb2:  // pred: ^bb0
      %1 = acir.method.call @right::@scheduler.issue_mul.0.call.6.10(%c1_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:right", instance = "right", interface = "Module:right:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.return %1 : !acir.struct<"src_payloads_25ebcaa7.Entry"> {acir.id = "v12", acir.source_type = "void", slots = [0]}
    }
    acir.work attributes {acir.function = {kind = "work", name = "scheduler_Work", owner = "scheduler", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @left::@left.slot {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.ref @right::@right.slot {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %3 = acir.cell.read %2 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %4 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %5 = scf.if %4 -> (i1) {
        %16 = acir.extract %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %17 = acir.extract %16 {acir.id = "v8", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %6 = scf.if %5 -> (i1) {
        %16 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %17 = acir.extract %16 {acir.id = "v15", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %7 = acir.extract %3 {acir.id = "v20", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %8 = scf.if %7 -> (i1) {
        %16 = acir.extract %3 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %17 = acir.extract %16 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %9 = scf.if %8 -> (i1) {
        %16 = acir.extract %3 {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
        %17 = acir.extract %16 {acir.id = "v30", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %10 = scf.if %6 -> (i1) {
        %16 = acir.extract %1 {acir.id = "v36", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %17 = arith.cmpi eq, %16, %c0_i32 {acir.id = "v38", acir.source_type = "bool"} : i32
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %11 = scf.if %9 -> (i1) {
        %16 = acir.extract %3 {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %17 = arith.cmpi eq, %16, %c0_i32 {acir.id = "v46", acir.source_type = "bool"} : i32
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %12 = scf.if %6 -> (i1) {
        %16 = acir.extract %1 {acir.id = "v52", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %17 = arith.cmpi eq, %16, %c1_i32 {acir.id = "v54", acir.source_type = "bool"} : i32
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %13 = scf.if %9 -> (i1) {
        %16 = acir.extract %3 {acir.id = "v60", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
        %17 = arith.cmpi eq, %16, %c1_i32 {acir.id = "v62", acir.source_type = "bool"} : i32
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %14 = scf.if %10 -> (i1) {
        %16 = acir.unary %11 "not" {acir.id = "v68", acir.source_type = "bool"} : i1 -> i1
        %17 = scf.if %16 -> (i1) {
          scf.yield %true : i1
        } else {
          %18 = acir.extract %1 {acir.id = "v71", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
          %19 = acir.extract %3 {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
          %20 = arith.cmpi ult, %18, %19 {acir.id = "v73", acir.source_type = "bool"} : i32
          scf.yield %20 : i1
        }
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      %15 = scf.if %12 -> (i1) {
        %16 = acir.unary %13 "not" {acir.id = "v82", acir.source_type = "bool"} : i1 -> i1
        %17 = scf.if %16 -> (i1) {
          scf.yield %true : i1
        } else {
          %18 = acir.extract %1 {acir.id = "v85", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
          %19 = acir.extract %3 {acir.id = "v86", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
          %20 = arith.cmpi ult, %18, %19 {acir.id = "v87", acir.source_type = "bool"} : i32
          scf.yield %20 : i1
        }
        scf.yield %17 : i1
      } else {
        scf.yield %false : i1
      }
      acir.rule.call @scheduler::@scheduler.issue_add(%14) {acir.id = "v95", acir.source_type = "void", output_bindings = [@scheduler::@scheduler.add_requests]} : (i1) -> ()
      acir.rule.call @scheduler::@scheduler.issue_mul(%15) {acir.id = "v97", acir.source_type = "void", output_bindings = [@scheduler::@scheduler.mul_requests]} : (i1) -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @adder attributes {acir.instance = {contract = "Module:adder", definition = "src_execution_885f3671.Adder", inputs = {source = {resources = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}}, interfaces = {}, members = {out = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, source = {resources = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}}, methods = [], name = "adder", outputs = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, parent = "top"}} {
    acir.queue @adder.out : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity_fn = "ac_init_adder.out_cap", initial_fn = "ac_init_adder.out", name = "adder.out", type = "src_payloads_25ebcaa7.Completion"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.rule @adder.execute : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () {acir.function = {kind = "rule", name = "adder.execute", owner = "adder", params = [{consume = true, id = "v1", name = "message", targets = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], result = "void", rule = "execute"}, output_types = [!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>]} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>):
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
      %2 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
      %3 = acir.extract %0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %4 = acir.extract %3 {acir.id = "v8", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
      %5 = acir.extract %0 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %6 = acir.extract %5 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
      %7 = arith.addi %4, %6 {acir.id = "v11", acir.source_type = "u32"} : i32
      %8 = acir.aggregate(%1, %2, %7) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      acir.return %8 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v13", acir.source_type = "void", slots = [0]}
    }
    acir.work attributes {acir.function = {kind = "work", name = "adder_Work", owner = "adder", params = [], result = "void"}} {
      %0 = acir.ref @scheduler::@scheduler.add_requests {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      acir.rule.call @adder::@adder.execute(%0) {acir.id = "v2", acir.source_type = "void", output_bindings = [@adder::@adder.out]} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @multiplier attributes {acir.instance = {contract = "Module:multiplier", definition = "src_execution_885f3671.Multiplier", inputs = {source = {resources = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}}, interfaces = {}, members = {active = {resources = ["multiplier.active"], type = "cell<bool>", value}, out = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, remaining = {resources = ["multiplier.remaining"], type = "cell<u32>", value}, request = {resources = ["multiplier.request"], type = "cell<src_payloads_25ebcaa7.Entry>", value}, source = {resources = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}}, methods = [], name = "multiplier", outputs = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, parent = "top"}} {
    acir.cell @multiplier.active : i1 {initial_fn = "ac_init_multiplier.active", name = "multiplier.active", type = "bool"}
    acir.cell @multiplier.remaining : i32 {initial_fn = "ac_init_multiplier.remaining", name = "multiplier.remaining", type = "u32"}
    acir.cell @multiplier.request : !acir.struct<"src_payloads_25ebcaa7.Entry"> {initial_fn = "ac_init_multiplier.request", name = "multiplier.request", type = "src_payloads_25ebcaa7.Entry"}
    acir.queue @multiplier.out : !acir.struct<"src_payloads_25ebcaa7.Completion"> {capacity_fn = "ac_init_multiplier.out_cap", initial_fn = "ac_init_multiplier.out", name = "multiplier.out", type = "src_payloads_25ebcaa7.Completion"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.rule @multiplier.advance : () -> () {acir.function = {kind = "rule", name = "multiplier.advance", owner = "multiplier", params = [], result = "void", rule = "advance"}, output_types = [!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>]} body {
      %false = arith.constant {acir.source_type = "bool"} false
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.ref @multiplier::@multiplier.remaining {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %2 = arith.cmpi ne, %1, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
      %3 = acir.ref @multiplier::@multiplier.request {acir.id = "v14", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %4 = acir.ref @multiplier::@multiplier.active {acir.id = "v16", acir.source_type = "cell<bool>"} : !acir.cell<i1>
      cf.cond_br %2, ^bb1, ^bb2
    ^bb1:  // pred: ^bb0
      %5 = acir.cell.read %0 {acir.id = "v8", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %6 = arith.subi %5, %c1_i32 {acir.id = "v10", acir.source_type = "u32"} : i32
      acir.cell.write %0, %6 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return {acir.id = "v12", acir.source_type = "void", slots = []}
    ^bb2:  // pred: ^bb0
      %7 = acir.cell.read %3 {acir.id = "v15", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.cell.write %4, %false {acir.id = "v18", acir.source_type = "void"} : !acir.cell<i1>, i1
      %8 = acir.extract %7 {acir.id = "v19", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
      %9 = acir.extract %7 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
      %10 = acir.extract %7 {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %11 = acir.extract %10 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
      %12 = acir.extract %7 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %13 = acir.extract %12 {acir.id = "v24", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
      %14 = arith.muli %11, %13 {acir.id = "v25", acir.source_type = "u32"} : i32
      %15 = acir.aggregate(%8, %9, %14) {acir.id = "v26", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      acir.return %15 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v27", acir.source_type = "void", slots = [0]}
    }
    acir.rule @multiplier.accept : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () {acir.function = {kind = "rule", name = "multiplier.accept", owner = "multiplier", params = [{consume = true, id = "v1", name = "message", targets = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], result = "void", rule = "accept"}, output_types = []} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %0 = acir.ref @multiplier::@multiplier.request {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %1 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      acir.cell.write %0, %1 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
      %2 = acir.ref @multiplier::@multiplier.remaining {acir.id = "v7", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %2, %c2_i32 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32
      %3 = acir.ref @multiplier::@multiplier.active {acir.id = "v10", acir.source_type = "cell<bool>"} : !acir.cell<i1>
      acir.cell.write %3, %true {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i1>, i1
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "multiplier_Work", owner = "multiplier", params = [], result = "void"}} {
      %0 = acir.ref @multiplier::@multiplier.active {acir.id = "v1", acir.source_type = "cell<bool>"} : !acir.cell<i1>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "bool", role = "observe"} : !acir.cell<i1> -> i1
      %2 = acir.ref @scheduler::@scheduler.mul_requests {acir.id = "v6", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      scf.if %1 {
        acir.rule.call @multiplier::@multiplier.advance() {acir.id = "v4", acir.source_type = "void", output_bindings = [@multiplier::@multiplier.out]} : () -> ()
      } else {
        acir.rule.call @multiplier::@multiplier.accept(%2) {acir.id = "v7", acir.source_type = "void", output_bindings = []} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> ()
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @cdb attributes {acir.instance = {contract = "Module:cdb", definition = "src_model_3cf90884.Broadcast", inputs = {add_done = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, left = {instance = "left"}, mul_done = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, right = {instance = "right"}, rob = {instance = "rob"}}, interfaces = {}, members = {add_done = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, event = {resources = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>", value}, left = {instance = "left"}, mul_done = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, right = {instance = "right"}, rob = {instance = "rob"}}, methods = [], name = "cdb", outputs = {resources = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>", value}, parent = "top"}} {
    acir.instance @rob = @rob
    acir.instance @left = @left
    acir.instance @right = @right
    acir.signal @cdb.event : !acir.struct<"src_payloads_25ebcaa7.Completion"> {initial_fn = "ac_init_cdb.event", name = "cdb.event", type = "src_payloads_25ebcaa7.Completion"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.rule @cdb.broadcast : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> () {acir.function = {kind = "rule", name = "cdb.broadcast", owner = "cdb", params = [{consume = true, id = "v1", name = "a", targets = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = true, id = "v2", name = "b", targets = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = false, id = "v3", name = "choose_add", targets = [], type = "bool"}], result = "void", rule = "broadcast"}, output_types = [!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>]} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg1: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg2: i1):
      %0 = scf.if %arg2 -> (!acir.struct<"src_payloads_25ebcaa7.Completion">) {
        %7 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion">
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      } else {
        %7 = acir.queue.peek %arg1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion">
        scf.yield %7 : !acir.struct<"src_payloads_25ebcaa7.Completion">
      }
      %1 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
      %2 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
      acir.method.call @rob::@cdb.broadcast.0.call.8.16(%1, %2) {acir.id = "v17", acir.source_type = "void", contract = "Module:rob", instance = "rob", interface = "Module:rob:complete", method = "complete", role = "method"} : (i32, i32) -> ()
      %3 = acir.extract %0 {acir.id = "v18", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
      %4 = acir.extract %0 {acir.id = "v19", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
      acir.method.call @left::@cdb.broadcast.0.call.13.19(%3, %4) {acir.id = "v20", acir.source_type = "void", contract = "Module:left", instance = "left", interface = "Module:left:wakeup", method = "wakeup", role = "method"} : (i32, i32) -> ()
      %5 = acir.extract %0 {acir.id = "v21", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
      %6 = acir.extract %0 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
      acir.method.call @right::@cdb.broadcast.0.call.14.22(%5, %6) {acir.id = "v23", acir.source_type = "void", contract = "Module:right", instance = "right", interface = "Module:right:wakeup", method = "wakeup", role = "method"} : (i32, i32) -> ()
      acir.return %0 : !acir.struct<"src_payloads_25ebcaa7.Completion"> {acir.id = "v24", acir.source_type = "void", slots = [0]}
    }
    acir.work attributes {acir.function = {kind = "work", name = "cdb_Work", owner = "cdb", params = [], result = "void"}} {
      %0 = acir.ref @adder::@adder.out {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.queue.empty %0 {acir.id = "v2", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1
      %2 = acir.unary %1 "not" {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
      %3 = acir.ref @multiplier::@multiplier.out {acir.id = "v4", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.rule.call @cdb::@cdb.broadcast(%0, %3, %2) {acir.id = "v5", acir.source_type = "void", output_bindings = [@cdb::@cdb.event]} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @dispatch attributes {acir.instance = {contract = "Module:dispatch", definition = "src_model_3cf90884.Dispatch", inputs = {add_done = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, left = {instance = "left"}, mul_done = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, rf = {instance = "rf"}, right = {instance = "right"}, rob = {instance = "rob"}, source = {resources = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>", value}}, interfaces = {}, members = {add_done = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, left = {instance = "left"}, mul_done = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, rf = {instance = "rf"}, right = {instance = "right"}, rob = {instance = "rob"}, source = {resources = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>", value}}, methods = [], name = "dispatch", outputs, parent = "top"}} {
    acir.instance @rob = @rob
    acir.instance @rf = @rf
    acir.instance @left = @left
    acir.instance @right = @right
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.rule @dispatch.dispatch : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () {acir.function = {kind = "rule", name = "dispatch.dispatch", owner = "dispatch", params = [{consume = true, id = "v1", name = "message", targets = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>"}], result = "void", rule = "dispatch"}, output_types = []} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.MicroOp", role = "consume"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
      %2 = acir.method.call @rf::@dispatch.dispatch.0.call.15.5(%1) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:rf", instance = "rf", interface = "Module:rf:operand", method = "operand", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %3 = acir.extract %0 {acir.id = "v7", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
      %4 = acir.method.call @rf::@dispatch.dispatch.0.call.21.7(%3) {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:rf", instance = "rf", interface = "Module:rf:operand", method = "operand", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
      %5 = acir.extract %0 {acir.id = "v9", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
      %6 = acir.method.call @rob::@dispatch.dispatch.0.call.27.9(%5) {acir.id = "v10", acir.source_type = "u32", contract = "Module:rob", instance = "rob", interface = "Module:rob:allocate", method = "allocate", role = "method"} : (i32) -> i32
      %7 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
      acir.method.call @rf::@dispatch.dispatch.0.call.37.11(%7, %6) {acir.id = "v12", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:rename", method = "rename", role = "method"} : (i32, i32) -> ()
      %8 = acir.extract %0 {acir.id = "v14", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
      %9 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
      %10 = acir.aggregate(%true, %8, %6, %9, %2, %4) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %11 = acir.ref @left::@left.slot {acir.id = "v17", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
      %12 = acir.cell.read %11 {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
      %13 = acir.extract %12 {acir.id = "v19", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
      %14 = acir.unary %13 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1
      scf.if %14 {
        acir.method.call @left::@dispatch.dispatch.0.call.38.21(%10) {acir.id = "v22", acir.source_type = "void", contract = "Module:left", instance = "left", interface = "Module:left:place", method = "place", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> ()
      } else {
        acir.method.call @right::@dispatch.dispatch.0.call.40.22(%10) {acir.id = "v23", acir.source_type = "void", contract = "Module:right", instance = "right", interface = "Module:right:place", method = "place", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> ()
      }
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "dispatch_Work", owner = "dispatch", params = [], result = "void"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @adder::@adder.out {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %1 = acir.queue.empty %0 {acir.id = "v2", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1
      %2 = acir.ref @multiplier::@multiplier.out {acir.id = "v4", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      %3 = acir.ref @top::@source {acir.id = "v11", acir.source_type = "queue<src_payloads_25ebcaa7.MicroOp>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
      %4 = scf.if %1 -> (i1) {
        %5 = acir.queue.empty %2 {acir.id = "v5", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %4 {
        acir.rule.call @dispatch::@dispatch.dispatch(%3) {acir.id = "v12", acir.source_type = "void", output_bindings = []} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> ()
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @commit attributes {acir.instance = {contract = "Module:commit", definition = "src_model_3cf90884.Commit", inputs = {rf = {instance = "rf"}, rob = {instance = "rob"}}, interfaces = {}, members = {last_tag = {resources = ["commit.last_tag"], type = "cell<u32>", value}, retired = {resources = ["commit.retired"], type = "cell<u32>", value}, rf = {instance = "rf"}, rob = {instance = "rob"}}, methods = [], name = "commit", outputs = [{resources = ["commit.retired"], type = "cell<u32>", value}, {resources = ["commit.last_tag"], type = "cell<u32>", value}], parent = "top"}} {
    acir.instance @rob = @rob
    acir.instance @rf = @rf
    acir.cell @commit.retired : i32 {initial_fn = "ac_init_commit.retired", name = "commit.retired", type = "u32"}
    acir.cell @commit.last_tag : i32 {initial_fn = "ac_init_commit.last_tag", name = "commit.last_tag", type = "u32"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.rule @commit.commit : () -> () {acir.function = {kind = "rule", name = "commit.commit", owner = "commit", params = [], result = "void", rule = "commit"}, output_types = []} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
      %0 = acir.method.call @rob::@commit.commit.0.call.42.2() {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:retire", method = "retire", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
      acir.method.call @rf::@commit.commit.0.call.51.3(%0) {acir.id = "v4", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:commit", method = "commit", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> ()
      %1 = acir.ref @commit::@commit.retired {acir.id = "v5", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32
      %3 = arith.addi %2, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
      acir.cell.write %1, %3 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32
      %4 = acir.ref @commit::@commit.last_tag {acir.id = "v10", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %5 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
      acir.cell.write %4, %5 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "commit_Work", owner = "commit", params = [], result = "void"}} {
      acir.rule.call @commit::@commit.commit() {acir.id = "v1", acir.source_type = "void", output_bindings = []} : () -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @audit attributes {acir.instance = {contract = "Module:audit", definition = "src_model_3cf90884.CompletionAudit", inputs = {event = {resources = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>", value}}, interfaces = {}, members = {event = {resources = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>", value}, last_tag = {resources = ["audit.last_tag"], type = "cell<u32>", value}}, methods = [], name = "audit", outputs = {resources = ["audit.last_tag"], type = "cell<u32>", value}, parent = "top"}} {
    acir.cell @audit.last_tag : i32 {initial_fn = "ac_init_audit.last_tag", name = "audit.last_tag", type = "u32"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
    acir.rule @audit.observe : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () {acir.function = {kind = "rule", name = "audit.observe", owner = "audit", params = [{consume = false, id = "v1", name = "value", targets = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>"}], result = "void", rule = "observe"}, output_types = []} body {
    ^bb0(%arg0: !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>):
      %0 = acir.signal.get %arg0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "observe"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion">
      %1 = acir.ref @audit::@audit.last_tag {acir.id = "v5", acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
      acir.cell.write %1, %2 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<i32>, i32
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "audit_Work", owner = "audit", params = [], result = "void"}} {
      %0 = acir.ref @cdb::@cdb.event {acir.id = "v1", acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
      acir.rule.call @audit::@audit.observe(%0) {acir.id = "v2", acir.source_type = "void", output_bindings = []} : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> ()
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_model_3cf90884.OutOfOrder", inputs = {}, interfaces = {}, members = {adder = {instance = "adder"}, audit = {instance = "audit"}, cdb = {instance = "cdb"}, commit = {instance = "commit"}, dispatch = {instance = "dispatch"}, left = {instance = "left"}, multiplier = {instance = "multiplier"}, rf = {instance = "rf"}, right = {instance = "right"}, rob = {instance = "rob"}, scheduler = {instance = "scheduler"}, source = {resources = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>", value}}, methods = [], name = "top", outputs = [[{resources = ["rf.registers"], type = "cell<array<u32,8>>", value}, {resources = ["rf.renamed"], type = "cell<array<u32,8>>", value}, {resources = ["rf.committed"], type = "cell<array<u32,8>>", value}], [{resources = ["rob.head"], type = "cell<u32>", value}, {resources = ["rob.tail"], type = "cell<u32>", value}], [{resources = ["commit.retired"], type = "cell<u32>", value}, {resources = ["commit.last_tag"], type = "cell<u32>", value}], {resources = ["audit.last_tag"], type = "cell<u32>", value}], parent}} {
    acir.instance @rob = @rob
    acir.instance @rf = @rf
    acir.instance @left = @left
    acir.instance @right = @right
    acir.instance @scheduler = @scheduler
    acir.instance @adder = @adder
    acir.instance @multiplier = @multiplier
    acir.instance @cdb = @cdb
    acir.instance @dispatch = @dispatch
    acir.instance @commit = @commit
    acir.instance @audit = @audit
    acir.queue @source : !acir.struct<"src_payloads_25ebcaa7.MicroOp"> {capacity_fn = "ac_init_source_cap", initial_fn = "ac_init_source", name = "source", type = "src_payloads_25ebcaa7.MicroOp"}
    func.func private @ac_init_source_cap() -> i32
    func.func private @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    func.func private @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    func.func private @ac_init_rob.head() -> i32
    func.func private @ac_init_rob.tail() -> i32
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.renamed() -> !acir.array<i32, 8>
    func.func private @ac_init_rf.committed() -> !acir.array<i32, 8>
    func.func private @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_scheduler.add_requests_cap() -> i32
    func.func private @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_scheduler.mul_requests_cap() -> i32
    func.func private @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    func.func private @ac_init_adder.out_cap() -> i32
    func.func private @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_multiplier.active() -> i1
    func.func private @ac_init_multiplier.remaining() -> i32
    func.func private @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    func.func private @ac_init_multiplier.out_cap() -> i32
    func.func private @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    func.func private @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    func.func private @ac_init_commit.retired() -> i32
    func.func private @ac_init_commit.last_tag() -> i32
    func.func private @ac_init_audit.last_tag() -> i32
  }
}

