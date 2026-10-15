#loc = loc(unknown)
module attributes {acir.instances = [{definition = "src_demo_7dc0ad19.Demo", name = "top", parent, rules = []}, {definition = "src_model_bb8d42ed.Ripes5", name = "top.cpu", parent = "top", rules = ["observe"]}, {definition = "src_model_bb8d42ed.RegisterFile", name = "top.cpu.rf", parent = "top.cpu", rules = ["read", "write"]}, {definition = "src_model_bb8d42ed.Fetch", name = "top.cpu.fetch", parent = "top.cpu", rules = ["advance"]}, {definition = "src_model_bb8d42ed.Decode", name = "top.cpu.decode_stage", parent = "top.cpu", rules = ["advance", "decode_instruction", "inspect"]}, {definition = "src_model_bb8d42ed.HazardUnit", name = "top.cpu.hazard_unit", parent = "top.cpu", rules = ["load_use"]}, {definition = "src_model_bb8d42ed.ForwardingUnit", name = "top.cpu.forwarding_unit", parent = "top.cpu", rules = ["resolve"]}, {definition = "src_model_bb8d42ed.Execute", name = "top.cpu.execute", parent = "top.cpu", rules = ["advance", "resolve"]}, {definition = "src_model_bb8d42ed.Memory", name = "top.cpu.memory", parent = "top.cpu", rules = ["advance"]}, {definition = "src_model_bb8d42ed.Writeback", name = "top.cpu.writeback", parent = "top.cpu", rules = ["advance"]}], acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "Demo", type_order = ["src_isa_e31afbf3.Instruction", "src_isa_e31afbf3.Fetched", "src_isa_e31afbf3.Decoded", "src_isa_e31afbf3.Executed", "src_isa_e31afbf3.Completed", "src_isa_e31afbf3.Control", "src_isa_e31afbf3.ForwardedOperands", "src_isa_e31afbf3.ExecutionResult", "src_isa_e31afbf3.Retirement", "src_isa_e31afbf3.StoreEvent"], types = {src_isa_e31afbf3.Completed = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}], src_isa_e31afbf3.Control = [{default = false, name = "stall", payload_type = i1, type = "bool"}, {default = false, name = "redirect", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "target", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "forward_a", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "forward_b", payload_type = i32, type = "u32"}], src_isa_e31afbf3.Decoded = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default, name = "instruction", payload_type = !acir.struct<"src_isa_e31afbf3.Instruction">, type = "src_isa_e31afbf3.Instruction"}, {default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}], src_isa_e31afbf3.Executed = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = false, name = "load", payload_type = i1, type = "bool"}, {default = false, name = "store", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "alu", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}], src_isa_e31afbf3.ExecutionResult = [{default, name = "packet", payload_type = !acir.struct<"src_isa_e31afbf3.Executed">, type = "src_isa_e31afbf3.Executed"}, {default = false, name = "redirect", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "target", payload_type = i32, type = "u32"}], src_isa_e31afbf3.Fetched = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}], src_isa_e31afbf3.ForwardedOperands = [{default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "source_a", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "source_b", payload_type = i32, type = "u32"}], src_isa_e31afbf3.Instruction = [{default = 0 : i64, name = "opcode", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "funct3", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "funct7", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "immediate", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = false, name = "load", payload_type = i1, type = "bool"}, {default = false, name = "store", payload_type = i1, type = "bool"}], src_isa_e31afbf3.Retirement = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_isa_e31afbf3.StoreEvent = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "address", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}, acir.stage = "transaction"} {
  %c1048576_i32 = arith.constant 1048576 : i32
  %c2048_i32 = arith.constant 2048 : i32
  %c84_i32 = arith.constant 84 : i32
  %c63_i64 = arith.constant 63 : i64
  %c62_i64 = arith.constant 62 : i64
  %c61_i64 = arith.constant 61 : i64
  %c60_i64 = arith.constant 60 : i64
  %c59_i64 = arith.constant 59 : i64
  %c58_i64 = arith.constant 58 : i64
  %c57_i64 = arith.constant 57 : i64
  %c56_i64 = arith.constant 56 : i64
  %c55_i64 = arith.constant 55 : i64
  %c54_i64 = arith.constant 54 : i64
  %c53_i64 = arith.constant 53 : i64
  %c52_i64 = arith.constant 52 : i64
  %c51_i64 = arith.constant 51 : i64
  %c50_i64 = arith.constant 50 : i64
  %c49_i64 = arith.constant 49 : i64
  %c48_i64 = arith.constant 48 : i64
  %c47_i64 = arith.constant 47 : i64
  %c46_i64 = arith.constant 46 : i64
  %c45_i64 = arith.constant 45 : i64
  %c44_i64 = arith.constant 44 : i64
  %c43_i64 = arith.constant 43 : i64
  %c42_i64 = arith.constant 42 : i64
  %c41_i64 = arith.constant 41 : i64
  %c40_i64 = arith.constant 40 : i64
  %c39_i64 = arith.constant 39 : i64
  %c38_i64 = arith.constant 38 : i64
  %c37_i64 = arith.constant 37 : i64
  %c36_i64 = arith.constant 36 : i64
  %c35_i64 = arith.constant 35 : i64
  %c34_i64 = arith.constant 34 : i64
  %c33_i64 = arith.constant 33 : i64
  %c64_i64 = arith.constant 64 : i64
  %c4096_i32 = arith.constant 4096 : i32
  %c256_i32 = arith.constant 256 : i32
  %c31_i64 = arith.constant 31 : i64
  %c30_i64 = arith.constant 30 : i64
  %c29_i64 = arith.constant 29 : i64
  %c28_i64 = arith.constant 28 : i64
  %c27_i64 = arith.constant 27 : i64
  %c26_i64 = arith.constant 26 : i64
  %c25_i64 = arith.constant 25 : i64
  %c24_i64 = arith.constant 24 : i64
  %c23_i64 = arith.constant 23 : i64
  %c22_i64 = arith.constant 22 : i64
  %c21_i64 = arith.constant 21 : i64
  %c20_i64 = arith.constant 20 : i64
  %c19_i64 = arith.constant 19 : i64
  %c18_i64 = arith.constant 18 : i64
  %c17_i64 = arith.constant 17 : i64
  %c16_i64 = arith.constant 16 : i64
  %c15_i64 = arith.constant 15 : i64
  %c14_i64 = arith.constant 14 : i64
  %c13_i64 = arith.constant 13 : i64
  %c12_i64 = arith.constant 12 : i64
  %c11_i64 = arith.constant 11 : i64
  %c10_i64 = arith.constant 10 : i64
  %c9_i64 = arith.constant 9 : i64
  %c8_i64 = arith.constant 8 : i64
  %c7_i64 = arith.constant 7 : i64
  %c6_i64 = arith.constant 6 : i64
  %c5_i64 = arith.constant 5 : i64
  %c4_i64 = arith.constant 4 : i64
  %c3_i64 = arith.constant 3 : i64
  %c2_i64 = arith.constant 2 : i64
  %c1_i64 = arith.constant 1 : i64
  %c32_i64 = arith.constant 32 : i64
  %c127_i32 = arith.constant 127 : i32
  %c12_i32 = arith.constant 12 : i32
  %c25_i32 = arith.constant 25 : i32
  %c15_i32 = arith.constant 15 : i32
  %c20_i32 = arith.constant 20 : i32
  %c-4096_i32 = arith.constant -4096 : i32
  %c255_i32 = arith.constant 255 : i32
  %c11_i32 = arith.constant 11 : i32
  %c21_i32 = arith.constant 21 : i32
  %c1023_i32 = arith.constant 1023 : i32
  %c63_i32 = arith.constant 63 : i32
  %c8_i32 = arith.constant 8 : i32
  %c0_i64 = arith.constant 0 : i64
  %c256_i64 = arith.constant 256 : i64
  %c7_i32 = arith.constant 7 : i32
  %c-1_i32 = arith.constant -1 : i32
  %c-2147483648_i32 = arith.constant -2147483648 : i32
  %c55_i32 = arith.constant 55 : i32
  %c23_i32 = arith.constant 23 : i32
  %c99_i32 = arith.constant 99 : i32
  %c3_i32 = arith.constant 3 : i32
  %c35_i32 = arith.constant 35 : i32
  %c51_i32 = arith.constant 51 : i32
  %c19_i32 = arith.constant 19 : i32
  %c32_i32 = arith.constant 32 : i32
  %c31_i32 = arith.constant 31 : i32
  %c5_i32 = arith.constant 5 : i32
  %c6_i32 = arith.constant 6 : i32
  %c111_i32 = arith.constant 111 : i32
  %c103_i32 = arith.constant 103 : i32
  %c4_i32 = arith.constant 4 : i32
  %c1_i32 = arith.constant 1 : i32
  %c2_i32 = arith.constant 2 : i32
  %c0_i32 = arith.constant 0 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 5 : i64, target_branches = [0], to = 0 : i64}, {from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 5 : i64, target_branches = [0], to = 2 : i64}, {from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 5 : i64, target_branches = [0], to = 6 : i64}, {from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 5 : i64, target_branches = [13], to = 2 : i64}], schedule = [1, 0, 2, 3, 4, 5, 6], stage = "transaction"} {
    acir.state @top.cpu.decode_stage.instruction : !acir.struct<"src_isa_e31afbf3.Instruction"> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, false, false, false], kind = "signal"}
    acir.state @top.cpu.ex_mem : !acir.struct<"src_isa_e31afbf3.Executed"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, false, false, false, 0 : i32, 0 : i32, 0 : i32, false], kind = "cell"}
    acir.state @top.cpu.fetch.pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.fetch.rom : !acir.array<i32, [256]> {initial = [5431 : i32, 17825939 : i32, 1384483 : i32, 336131 : i32, 1115747 : i32, 335907 : i32, 103811603 : i32, 1114515 : i32, 2201187 : i32, 335907 : i32, 103811731 : i32, 336387 : i32, 1185379 : i32, 1180307 : i32, 8388719 : i32, 335907 : i32, 5579299 : i32, 2146435091 : i32, 111 : i32, 19 : i32, 19 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.id_ex : !acir.struct<"src_isa_e31afbf3.Decoded"> {initial = [false, 0 : i32, 0 : i32, [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, false, false, false], 0 : i32, 0 : i32, false], kind = "cell"}
    acir.state @top.cpu.if_id : !acir.struct<"src_isa_e31afbf3.Fetched"> {initial = [false, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.last_control : !acir.struct<"src_isa_e31afbf3.Control"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.mem_wb : !acir.struct<"src_isa_e31afbf3.Completed"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, false, 0 : i32, false], kind = "cell"}
    acir.state @top.cpu.memory.store_event : !acir.struct<"src_isa_e31afbf3.StoreEvent"> {initial = [false, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.memory.words.0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.10 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.11 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.12 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.13 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.14 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.15 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.16 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.17 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.18 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.19 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.20 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.21 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.22 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.23 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.24 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.25 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.26 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.27 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.28 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.29 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.30 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.31 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.32 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.33 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.34 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.35 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.36 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.37 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.38 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.39 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.40 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.41 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.42 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.43 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.44 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.45 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.46 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.47 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.48 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.49 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.5 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.50 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.51 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.52 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.53 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.54 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.55 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.56 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.57 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.58 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.59 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.6 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.60 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.61 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.62 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.63 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.7 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.8 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.memory.words.9 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.10 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.11 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.12 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.13 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.14 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.15 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.16 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.17 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.18 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.19 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.20 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.21 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.22 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.23 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.24 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.25 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.26 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.27 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.28 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.29 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.30 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.31 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.5 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.6 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.7 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.8 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.rf.registers.9 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.writeback.retired : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.writeback.retirement : !acir.struct<"src_isa_e31afbf3.Retirement"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, false, 0 : i32], kind = "cell"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64}
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.decode_stage:inspect"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.decode_stage:inspect"], method = "get", port = 0 : i64, resource = "top.cpu.decode_stage.instruction", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Instruction">, i1) loc(#loc1)
    %0 = acir.export %value : !acir.struct<"src_isa_e31afbf3.Instruction"> -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc2)
    %1 = acir.export %ready : i1 -> i1
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.hazard_unit:load_use"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value_0, %ready_1 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.hazard_unit:load_use"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Decoded">, i1) loc(#loc3)
    %2 = acir.extract %value_0 {field = "instruction"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc4)
    %3 = acir.extract %2 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc5)
    %4 = acir.extract %2 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc6)
    %5 = arith.cmpi ne, %4, %c0_i32 : i32 loc(#loc6)
    %6 = arith.andi %3, %5 : i1
    %7 = acir.extract %0 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc7)
    %8 = arith.cmpi eq, %4, %7 : i32 loc(#loc8)
    %9 = acir.extract %0 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc9)
    %10 = arith.cmpi eq, %4, %9 : i32 loc(#loc10)
    %11 = arith.ori %8, %10 : i1
    %12 = arith.ori %8, %1 : i1
    %13 = arith.andi %1, %12 : i1
    %14 = arith.xori %6, %true : i1
    %15 = arith.ori %14, %1 : i1
    %16 = arith.andi %6, %11 : i1
    %17 = arith.ori %14, %13 : i1
    %18 = arith.andi %17, %15 : i1
    %19 = acir.export %16 : i1 -> i1 loc(#loc11)
    %20 = acir.export %18 : i1 -> i1
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.forwarding_unit:resolve"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value_2, %ready_3 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.forwarding_unit:resolve"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Decoded">, i1)
    %21 = acir.bind %value_2 : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc12)
    %value_4, %ready_5 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.forwarding_unit:resolve"], method = "read", port = 0 : i64, resource = "top.cpu.ex_mem", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Executed">, i1)
    %22 = acir.bind %value_4 : (!acir.struct<"src_isa_e31afbf3.Executed">) -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc13)
    %value_6, %ready_7 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.forwarding_unit:resolve"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Completed">, i1)
    %23 = acir.bind %value_6 : (!acir.struct<"src_isa_e31afbf3.Completed">) -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc14)
    %24 = acir.extract %21 {field = "instruction"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc15)
    %25 = acir.extract %24 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc265)
    %26 = acir.aggregate() : () -> i32 loc(#loc266)
    %27 = arith.cmpi ne, %25, %c0_i32 : i32 loc(#loc267)
    %28 = acir.extract %22 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc268)
    %29 = acir.extract %22 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc269)
    %30 = arith.cmpi eq, %29, %25 : i32 loc(#loc269)
    %31 = arith.andi %28, %30 : i1 loc(#loc266)
    %32 = arith.xori %31, %true : i1 loc(#loc266)
    %33 = arith.select %31, %c1_i32, %26 : i32 loc(#loc266)
    %34 = acir.extract %23 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i1 loc(#loc270)
    %35 = acir.extract %23 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc271)
    %36 = arith.cmpi eq, %35, %25 : i32 loc(#loc271)
    %37 = arith.andi %34, %36 : i1 loc(#loc266)
    %38 = arith.select %37, %false, %32 : i1 loc(#loc266)
    %39 = arith.select %37, %c2_i32, %33 : i32 loc(#loc266)
    %40 = arith.select %31, %32, %38 : i1 loc(#loc266)
    %41 = arith.select %31, %c1_i32, %39 : i32 loc(#loc266)
    %42 = arith.cmpi eq, %25, %c0_i32 : i32 loc(#loc395)
    %43 = arith.ori %42, %40 : i1 loc(#loc266)
    %44 = arith.select %27, %41, %26 : i32 loc(#loc266)
    %45 = arith.select %43, %c0_i32, %44 : i32 loc(#loc22)
    %46 = acir.extract %24 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc265)
    %47 = arith.cmpi ne, %46, %c0_i32 : i32 loc(#loc267)
    %48 = arith.cmpi eq, %29, %46 : i32 loc(#loc269)
    %49 = arith.andi %28, %48 : i1 loc(#loc266)
    %50 = arith.xori %49, %true : i1 loc(#loc266)
    %51 = arith.select %49, %c1_i32, %26 : i32 loc(#loc266)
    %52 = arith.cmpi eq, %35, %46 : i32 loc(#loc271)
    %53 = arith.andi %34, %52 : i1 loc(#loc266)
    %54 = arith.select %53, %false, %50 : i1 loc(#loc266)
    %55 = arith.select %53, %c2_i32, %51 : i32 loc(#loc266)
    %56 = arith.select %49, %50, %54 : i1 loc(#loc266)
    %57 = arith.select %49, %c1_i32, %55 : i32 loc(#loc266)
    %58 = arith.cmpi eq, %46, %c0_i32 : i32 loc(#loc395)
    %59 = arith.ori %58, %56 : i1 loc(#loc266)
    %60 = arith.select %47, %57, %26 : i32 loc(#loc266)
    %61 = arith.select %59, %c0_i32, %60 : i32 loc(#loc23)
    %62 = acir.extract %21 {field = "lhs"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc272)
    %63 = arith.cmpi eq, %45, %c1_i32 : i32 loc(#loc273)
    %64 = arith.cmpi ne, %45, %c1_i32 : i32 loc(#loc273)
    %65 = acir.extract %22 {field = "alu"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc274)
    %66 = arith.select %63, %65, %26 : i32 loc(#loc266)
    %67 = arith.cmpi eq, %45, %c2_i32 : i32 loc(#loc275)
    %68 = arith.cmpi ne, %45, %c2_i32 : i32 loc(#loc275)
    %69 = acir.extract %23 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc276)
    %70 = arith.select %67, %69, %66 : i32 loc(#loc266)
    %71 = arith.select %68, %62, %70 : i32 loc(#loc266)
    %72 = arith.select %64, %71, %66 : i32 loc(#loc29)
    %73 = acir.extract %21 {field = "rhs"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc272)
    %74 = arith.cmpi eq, %61, %c1_i32 : i32 loc(#loc273)
    %75 = arith.cmpi ne, %61, %c1_i32 : i32 loc(#loc273)
    %76 = arith.select %74, %65, %26 : i32 loc(#loc266)
    %77 = arith.cmpi eq, %61, %c2_i32 : i32 loc(#loc275)
    %78 = arith.cmpi ne, %61, %c2_i32 : i32 loc(#loc275)
    %79 = arith.select %77, %69, %76 : i32 loc(#loc266)
    %80 = arith.select %78, %73, %79 : i32 loc(#loc266)
    %81 = arith.select %75, %80, %76 : i32 loc(#loc30)
    %82 = acir.aggregate(%72, %81, %45, %61) : (i32, i32, i32, i32) -> !acir.struct<"src_isa_e31afbf3.ForwardedOperands"> loc(#loc31)
    %83 = acir.export %82 : !acir.struct<"src_isa_e31afbf3.ForwardedOperands"> -> !acir.struct<"src_isa_e31afbf3.ForwardedOperands"> loc(#loc32)
    %84 = acir.export %true : i1 -> i1
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.execute:resolve"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value_8, %ready_9 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.execute:resolve"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Decoded">, i1)
    %85 = acir.bind %value_8 : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc33)
    %86 = acir.extract %85 {field = "instruction"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc34)
    %87 = acir.extract %83 {field = "lhs"} : (!acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> i32 loc(#loc35)
    %88 = acir.extract %83 {field = "rhs"} : (!acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> i32 loc(#loc36)
    %89 = acir.extract %85 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc277)
    %90 = acir.extract %86 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc278)
    %91 = arith.cmpi eq, %90, %c55_i32 : i32 loc(#loc279)
    %92 = arith.cmpi ne, %90, %c55_i32 : i32 loc(#loc279)
    %93 = acir.extract %86 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc280)
    %94 = arith.select %91, %93, %26 : i32 loc(#loc266)
    %95 = arith.cmpi eq, %90, %c23_i32 : i32 loc(#loc281)
    %96 = arith.cmpi eq, %90, %c111_i32 : i32 loc(#loc282)
    %97 = arith.ori %95, %96 : i1 loc(#loc266)
    %98 = arith.cmpi eq, %90, %c99_i32 : i32 loc(#loc283)
    %99 = arith.ori %97, %98 : i1 loc(#loc266)
    %100 = arith.xori %99, %true : i1 loc(#loc266)
    %101 = arith.select %99, %100, %false : i1 loc(#loc266)
    %102 = arith.cmpi eq, %90, %c3_i32 : i32 loc(#loc284)
    %103 = arith.cmpi eq, %90, %c35_i32 : i32 loc(#loc285)
    %104 = arith.ori %102, %103 : i1 loc(#loc266)
    %105 = arith.cmpi eq, %90, %c103_i32 : i32 loc(#loc286)
    %106 = arith.ori %104, %105 : i1 loc(#loc266)
    %107 = arith.select %106, %false, %100 : i1 loc(#loc266)
    %108 = arith.addi %87, %93 : i32 loc(#loc287)
    %109 = arith.select %106, %108, %94 : i32 loc(#loc266)
    %110 = arith.xori %106, %true : i1 loc(#loc266)
    %111 = arith.ori %110, %84 : i1 loc(#loc266)
    %112 = arith.cmpi eq, %90, %c51_i32 : i32 loc(#loc288)
    %113 = arith.cmpi eq, %90, %c19_i32 : i32 loc(#loc289)
    %114 = arith.ori %112, %113 : i1 loc(#loc266)
    %115 = arith.select %113, %93, %88 : i32 loc(#loc290)
    %116 = arith.ori %113, %84 : i1 loc(#loc266)
    %117 = acir.extract %86 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc291)
    %118 = arith.cmpi eq, %117, %c0_i32 : i32 loc(#loc292)
    %119 = arith.cmpi ne, %117, %c0_i32 : i32 loc(#loc292)
    %120 = acir.extract %86 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc293)
    %121 = arith.cmpi eq, %120, %c32_i32 : i32 loc(#loc293)
    %122 = arith.andi %112, %121 : i1 loc(#loc266)
    %123 = arith.andi %84, %116 : i1 loc(#loc294)
    %124 = arith.addi %87, %115 : i32 loc(#loc294)
    %125 = arith.subi %87, %115 : i32 loc(#loc295)
    %126 = arith.select %122, %125, %124 : i32 loc(#loc266)
    %127 = arith.select %118, %126, %109 : i32 loc(#loc266)
    %128 = arith.select %118, %123, %111 : i1 loc(#loc266)
    %129 = arith.cmpi eq, %117, %c1_i32 : i32 loc(#loc296)
    %130 = arith.cmpi ne, %117, %c1_i32 : i32 loc(#loc296)
    %131 = arith.andi %115, %c31_i32 : i32 loc(#loc297)
    %132 = arith.shli %87, %131 : i32 loc(#loc298)
    %133 = arith.select %129, %132, %127 : i32 loc(#loc266)
    %134 = arith.select %129, %123, %128 : i1 loc(#loc266)
    %135 = arith.cmpi eq, %117, %c2_i32 : i32 loc(#loc299)
    %136 = arith.cmpi ne, %117, %c2_i32 : i32 loc(#loc299)
    %137 = arith.xori %87, %c-2147483648_i32 : i32 loc(#loc396)
    %138 = arith.xori %115, %c-2147483648_i32 : i32 loc(#loc397)
    %139 = arith.cmpi ult, %137, %138 : i32 loc(#loc398)
    %140 = arith.extui %139 : i1 to i32 loc(#loc266)
    %141 = arith.select %135, %140, %133 : i32 loc(#loc266)
    %142 = arith.select %135, %123, %134 : i1 loc(#loc266)
    %143 = arith.cmpi eq, %117, %c3_i32 : i32 loc(#loc301)
    %144 = arith.cmpi ne, %117, %c3_i32 : i32 loc(#loc301)
    %145 = arith.cmpi ult, %87, %115 : i32 loc(#loc302)
    %146 = arith.extui %145 : i1 to i32 loc(#loc266)
    %147 = arith.select %143, %146, %141 : i32 loc(#loc266)
    %148 = arith.select %143, %123, %142 : i1 loc(#loc266)
    %149 = arith.cmpi eq, %117, %c4_i32 : i32 loc(#loc303)
    %150 = arith.cmpi ne, %117, %c4_i32 : i32 loc(#loc303)
    %151 = arith.xori %87, %115 : i32 loc(#loc304)
    %152 = arith.select %149, %151, %147 : i32 loc(#loc266)
    %153 = arith.select %149, %123, %148 : i1 loc(#loc266)
    %154 = arith.cmpi eq, %117, %c5_i32 : i32 loc(#loc305)
    %155 = arith.cmpi ne, %117, %c5_i32 : i32 loc(#loc305)
    %156 = arith.shrui %87, %131 : i32 loc(#loc306)
    %157 = arith.cmpi ne, %131, %c0_i32 : i32 loc(#loc399)
    %158 = arith.andi %87, %c-2147483648_i32 : i32 loc(#loc400)
    %159 = arith.cmpi ne, %158, %c0_i32 : i32 loc(#loc401)
    %160 = arith.andi %157, %159 : i1 loc(#loc402)
    %161 = arith.cmpi eq, %131, %c0_i32 : i32 loc(#loc421)
    %162 = arith.ori %161, %84 : i1 loc(#loc402)
    %163 = arith.andi %116, %162 : i1 loc(#loc402)
    %164 = arith.subi %c32_i32, %131 : i32 loc(#loc403)
    %165 = arith.shli %c-1_i32, %164 : i32 loc(#loc404)
    %166 = arith.ori %156, %165 : i32 loc(#loc405)
    %167 = arith.select %160, %166, %156 : i32 loc(#loc405)
    %168 = arith.andi %163, %123 : i1 loc(#loc402)
    %169 = arith.select %121, %167, %156 : i32 loc(#loc266)
    %170 = arith.select %121, %168, %123 : i1 loc(#loc266)
    %171 = arith.select %154, %169, %152 : i32 loc(#loc266)
    %172 = arith.select %154, %170, %153 : i1 loc(#loc266)
    %173 = arith.cmpi eq, %117, %c6_i32 : i32 loc(#loc308)
    %174 = arith.cmpi ne, %117, %c6_i32 : i32 loc(#loc308)
    %175 = arith.ori %87, %115 : i32 loc(#loc309)
    %176 = arith.select %173, %175, %171 : i32 loc(#loc266)
    %177 = arith.select %173, %123, %172 : i1 loc(#loc266)
    %178 = arith.andi %87, %115 : i32 loc(#loc310)
    %179 = arith.select %174, %178, %176 : i32 loc(#loc266)
    %180 = arith.select %174, %123, %177 : i1 loc(#loc266)
    %181 = arith.select %155, %179, %171 : i32 loc(#loc266)
    %182 = arith.select %155, %180, %172 : i1 loc(#loc266)
    %183 = arith.select %150, %181, %152 : i32 loc(#loc266)
    %184 = arith.select %150, %182, %153 : i1 loc(#loc266)
    %185 = arith.select %144, %183, %147 : i32 loc(#loc266)
    %186 = arith.select %144, %184, %148 : i1 loc(#loc266)
    %187 = arith.select %136, %185, %141 : i32 loc(#loc266)
    %188 = arith.select %136, %186, %142 : i1 loc(#loc266)
    %189 = arith.select %130, %187, %133 : i32 loc(#loc266)
    %190 = arith.select %130, %188, %134 : i1 loc(#loc266)
    %191 = arith.select %119, %189, %127 : i32 loc(#loc266)
    %192 = arith.select %119, %190, %128 : i1 loc(#loc266)
    %193 = arith.select %114, %192, %111 : i1 loc(#loc266)
    %194 = arith.select %114, %191, %c0_i32 : i32 loc(#loc266)
    %195 = arith.select %107, %194, %109 : i32 loc(#loc266)
    %196 = arith.select %107, %193, %111 : i1 loc(#loc266)
    %197 = arith.addi %89, %93 : i32 loc(#loc311)
    %198 = arith.select %99, %197, %195 : i32 loc(#loc266)
    %199 = arith.ori %99, %196 : i1 loc(#loc266)
    %200 = arith.andi %92, %101 : i1 loc(#loc266)
    %201 = arith.select %92, %198, %94 : i32 loc(#loc81)
    %202 = arith.ori %91, %199 : i1 loc(#loc266)
    %203 = arith.xori %200, %true : i1 loc(#loc266)
    %204 = acir.assert %203 loc(#loc266)
    %205 = arith.andi %202, %204 : i1 loc(#loc266)
    %206 = arith.ori %96, %105 : i1 loc(#loc82)
    %207 = acir.aggregate() : () -> i1 loc(#loc312)
    %208 = arith.cmpi ne, %90, %c99_i32 : i32 loc(#loc313)
    %209 = arith.select %208, %false, %207 : i1 loc(#loc312)
    %210 = arith.cmpi eq, %87, %88 : i32 loc(#loc314)
    %211 = arith.select %118, %210, %209 : i1 loc(#loc312)
    %212 = arith.ori %119, %84 : i1 loc(#loc312)
    %213 = arith.cmpi ne, %87, %88 : i32 loc(#loc315)
    %214 = arith.select %129, %213, %211 : i1 loc(#loc312)
    %215 = arith.select %129, %84, %212 : i1 loc(#loc312)
    %216 = arith.xori %88, %c-2147483648_i32 : i32 loc(#loc406)
    %217 = arith.cmpi ult, %137, %216 : i32 loc(#loc407)
    %218 = arith.select %149, %217, %214 : i1 loc(#loc312)
    %219 = arith.select %149, %84, %215 : i1 loc(#loc312)
    %220 = arith.cmpi uge, %137, %216 : i32 loc(#loc422)
    %221 = arith.select %154, %220, %218 : i1 loc(#loc312)
    %222 = arith.select %154, %84, %219 : i1 loc(#loc312)
    %223 = arith.cmpi ult, %87, %88 : i32 loc(#loc319)
    %224 = arith.select %173, %223, %221 : i1 loc(#loc312)
    %225 = arith.select %173, %84, %222 : i1 loc(#loc312)
    %226 = arith.cmpi eq, %117, %c7_i32 : i32 loc(#loc320)
    %227 = acir.assert %226 loc(#loc321)
    %228 = arith.cmpi uge, %87, %88 : i32 loc(#loc322)
    %229 = arith.andi %84, %227 : i1 loc(#loc312)
    %230 = arith.ori %173, %227 : i1 loc(#loc312)
    %231 = arith.select %174, %228, %224 : i1 loc(#loc312)
    %232 = arith.select %174, %229, %225 : i1 loc(#loc312)
    %233 = arith.andi %232, %230 : i1 loc(#loc312)
    %234 = arith.ori %154, %230 : i1 loc(#loc312)
    %235 = arith.select %155, %231, %221 : i1 loc(#loc312)
    %236 = arith.select %155, %233, %222 : i1 loc(#loc312)
    %237 = arith.andi %236, %234 : i1 loc(#loc312)
    %238 = arith.ori %149, %234 : i1 loc(#loc312)
    %239 = arith.select %150, %235, %218 : i1 loc(#loc312)
    %240 = arith.select %150, %237, %219 : i1 loc(#loc312)
    %241 = arith.andi %240, %238 : i1 loc(#loc312)
    %242 = arith.ori %129, %238 : i1 loc(#loc312)
    %243 = arith.select %130, %239, %214 : i1 loc(#loc312)
    %244 = arith.select %130, %241, %215 : i1 loc(#loc312)
    %245 = arith.andi %244, %242 : i1 loc(#loc312)
    %246 = arith.ori %118, %242 : i1 loc(#loc312)
    %247 = arith.select %119, %243, %211 : i1 loc(#loc312)
    %248 = arith.select %119, %245, %212 : i1 loc(#loc312)
    %249 = arith.andi %248, %246 : i1 loc(#loc312)
    %250 = arith.ori %208, %246 : i1 loc(#loc312)
    %251 = arith.select %98, %247, %209 : i1 loc(#loc312)
    %252 = arith.ori %208, %249 : i1 loc(#loc312)
    %253 = arith.andi %252, %250 : i1 loc(#loc312)
    %254 = arith.addi %89, %c4_i32 : i32 loc(#loc94)
    %255 = arith.ori %206, %251 : i1
    %256 = arith.ori %206, %253 : i1
    %257 = arith.select %206, %254, %201 : i32
    %258 = arith.ori %206, %205 : i1
    %259 = acir.extract %85 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i1 loc(#loc95)
    %260 = acir.extract %85 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc96)
    %261 = acir.extract %86 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc97)
    %262 = acir.extract %86 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc98)
    %263 = acir.extract %86 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc99)
    %264 = acir.extract %86 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc100)
    %265 = acir.extract %85 {field = "stalled"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i1 loc(#loc101)
    %266 = arith.andi %205, %84 : i1
    %267 = arith.andi %266, %258 : i1
    %268 = acir.aggregate(%259, %89, %260, %261, %262, %263, %264, %201, %88, %257, %265) : (i1, i32, i32, i32, i1, i1, i1, i32, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc102)
    %269 = arith.andi %256, %205 : i1
    %270 = arith.select %255, %201, %c0_i32 : i32
    %271 = arith.andi %267, %256 : i1 loc(#loc103)
    %272 = arith.andi %271, %269 : i1 loc(#loc103)
    %273 = acir.aggregate(%268, %255, %270) : (!acir.struct<"src_isa_e31afbf3.Executed">, i1, i32) -> !acir.struct<"src_isa_e31afbf3.ExecutionResult"> loc(#loc103)
    %274 = acir.export %273 : !acir.struct<"src_isa_e31afbf3.ExecutionResult"> -> !acir.struct<"src_isa_e31afbf3.ExecutionResult"> loc(#loc104)
    %275 = acir.export %272 : i1 -> i1
    %276 = acir.extract %274 {field = "redirect"} : (!acir.struct<"src_isa_e31afbf3.ExecutionResult">) -> i1 loc(#loc105)
    %277 = acir.extract %274 {field = "target"} : (!acir.struct<"src_isa_e31afbf3.ExecutionResult">) -> i32 loc(#loc106)
    %278 = acir.extract %83 {field = "source_a"} : (!acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> i32 loc(#loc107)
    %279 = acir.extract %83 {field = "source_b"} : (!acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> i32 loc(#loc108)
    %280 = arith.andi %20, %275 : i1
    %281 = arith.andi %280, %84 : i1
    %282 = acir.aggregate(%19, %276, %277, %278, %279) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_isa_e31afbf3.Control"> loc(#loc109)
    %283 = acir.export %282 {acir.id = "v21", acir.source_type = "src_isa_e31afbf3.Control"} : !acir.struct<"src_isa_e31afbf3.Control"> -> !acir.struct<"src_isa_e31afbf3.Control"> loc(#loc110)
    acir.path %true {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc111)
    acir.path %true {branch = 1 : i64, frames = ["top.cpu.fetch:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc111)
    %284 = acir.aggregate() : () -> !acir.struct<"src_isa_e31afbf3.Fetched">
    %285 = acir.extract %283 {field = "stall"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> i1 loc(#loc112)
    %286 = arith.xori %285, %true : i1
    %287 = arith.ori %285, %281 : i1
    %288 = acir.extract %283 {field = "redirect"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> i1 loc(#loc113)
    %289 = arith.select %288, %false, %286 : i1
    %290 = arith.andi %287, %281 : i1
    %291 = arith.ori %285, %290 : i1
    %292 = arith.andi %281, %291 : i1
    %293 = arith.andi %286, %288 : i1
    %294 = arith.xori %293, %true : i1 loc(#loc114)
    %295 = arith.ori %294, %290 : i1
    %296 = acir.extract %283 {field = "target"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> i32 loc(#loc115)
    %297 = arith.andi %295, %281 : i1 loc(#loc114)
    %298 = arith.ori %294, %297 : i1 loc(#loc114)
    %299 = arith.andi %292, %298 : i1 loc(#loc114)
    acir.path %293 {branch = 2 : i64, kind = "branch", txn = 0 : i64} loc(#loc114)
    %300 = acir.aggregate(%false, %c0_i32, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc116)
    %301 = arith.select %288, %300, %284 : !acir.struct<"src_isa_e31afbf3.Fetched">
    %302 = arith.select %289, %true, %288 : i1
    %303 = arith.andi %299, %291 : i1
    %304 = arith.andi %286, %289 : i1
    %305 = arith.xori %304, %true : i1 loc(#loc117)
    %306 = arith.ori %305, %290 : i1
    acir.path %304 {branch = 3 : i64, kind = "branch", txn = 0 : i64} loc(#loc117)
    %value_10, %ready_11 = acir.read(%304) {branch = 3 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc117)
    %307 = arith.ori %305, %306 : i1 loc(#loc117)
    %308 = arith.andi %303, %307 : i1 loc(#loc117)
    %309 = arith.andi %value_10, %c3_i32 : i32 loc(#loc117)
    %310 = arith.cmpi eq, %309, %c0_i32 : i32 loc(#loc117)
    %value_12, %ready_13 = acir.read(%304) {branch = 3 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, [256]>, i1) loc(#loc118)
    %311 = arith.shrui %value_10, %c2_i32 : i32 loc(#loc119)
    %312 = arith.extui %311 : i32 to i64 loc(#loc118)
    %313 = arith.cmpi ult, %312, %c256_i64 : i64 loc(#loc118)
    %314 = arith.select %313, %312, %c0_i64 : i64 loc(#loc118)
    %315 = "acir.array.index"(%value_12, %314) : (!acir.array<i32, [256]>, i64) -> i32 loc(#loc118)
    %316 = acir.aggregate(%true, %value_10, %315) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc120)
    %317 = arith.addi %value_10, %c4_i32 : i32 loc(#loc121)
    %318 = arith.select %289, %316, %301 : !acir.struct<"src_isa_e31afbf3.Fetched">
    %319 = arith.select %289, %313, %281 : i1
    %320 = arith.andi %281, %319 : i1
    %321 = arith.andi %286, %302 : i1
    %322 = arith.select %285, %284, %318 : !acir.struct<"src_isa_e31afbf3.Fetched">
    %323 = arith.ori %285, %320 : i1
    %324 = arith.andi %281, %323 : i1
    %325 = arith.andi %324, %290 : i1
    %326 = arith.andi %325, %321 : i1
    %327 = arith.andi %326, %290 : i1
    acir.path %327 {branch = 4 : i64, kind = "branch", txn = 0 : i64}
    %328 = acir.fire "top.cpu.fetch.advance#0" %308 {successors = [], txn = 0 : i64}
    %329 = arith.andi %328, %293 : i1
    acir.effect %329(%296) {branch = 2 : i64, frames = ["top.cpu.fetch:advance"], method = "write", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc114)
    %330 = arith.xori %288, %true : i1
    %331 = arith.andi %286, %330 : i1
    %332 = arith.andi %328, %331 : i1
    acir.effect %332(%310) {branch = 3 : i64, frames = ["top.cpu.fetch:advance"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> () loc(#loc122)
    acir.effect %332(%317) {branch = 3 : i64, frames = ["top.cpu.fetch:advance"], method = "write", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc123)
    %333 = arith.andi %328, %327 : i1
    acir.effect %333(%322) {branch = 4 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.cpu.if_id", txn = 0 : i64} : (i1, !acir.struct<"src_isa_e31afbf3.Fetched">) -> ()
    acir.path %true {branch = 5 : i64, kind = "branch", txn = 1 : i64} loc(#loc124)
    acir.path %true {branch = 5 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc124)
    %value_14, %ready_15 = acir.read(%true) {branch = 5 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "read", port = 0 : i64, resource = "top.cpu.if_id", txn = 1 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Fetched">, i1) loc(#loc125)
    %334 = acir.extract %value_14 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> i32 loc(#loc323)
    %335 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %false, %false, %false) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %336 = arith.cmpi ne, %334, %c0_i32 : i32 loc(#loc325)
    %337 = arith.andi %334, %c127_i32 : i32 loc(#loc326)
    %338 = acir.extract %335 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc327)
    %339 = acir.extract %335 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc327)
    %340 = acir.extract %335 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc327)
    %341 = acir.extract %335 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc327)
    %342 = acir.extract %335 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc327)
    %343 = acir.extract %335 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc327)
    %344 = acir.extract %335 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc327)
    %345 = acir.extract %335 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc327)
    %346 = acir.extract %335 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc327)
    %347 = acir.aggregate(%337, %338, %339, %340, %341, %342, %343, %344, %345, %346) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %348 = arith.shrui %334, %c12_i32 : i32 loc(#loc328)
    %349 = arith.andi %348, %c7_i32 : i32 loc(#loc328)
    %350 = acir.extract %347 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc329)
    %351 = acir.extract %347 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc329)
    %352 = acir.extract %347 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc329)
    %353 = acir.extract %347 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc329)
    %354 = acir.extract %347 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc329)
    %355 = acir.extract %347 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc329)
    %356 = acir.extract %347 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc329)
    %357 = acir.extract %347 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc329)
    %358 = acir.extract %347 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc329)
    %359 = acir.aggregate(%350, %349, %351, %352, %353, %354, %355, %356, %357, %358) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %360 = arith.shrui %334, %c25_i32 : i32 loc(#loc330)
    %361 = acir.extract %359 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc331)
    %362 = acir.extract %359 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc331)
    %363 = acir.extract %359 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc331)
    %364 = acir.extract %359 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc331)
    %365 = acir.extract %359 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc331)
    %366 = acir.extract %359 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc331)
    %367 = acir.extract %359 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc331)
    %368 = acir.extract %359 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc331)
    %369 = acir.extract %359 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc331)
    %370 = acir.aggregate(%361, %362, %360, %363, %364, %365, %366, %367, %368, %369) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %371 = arith.shrui %334, %c7_i32 : i32 loc(#loc332)
    %372 = arith.andi %371, %c31_i32 : i32 loc(#loc332)
    %373 = acir.extract %370 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc333)
    %374 = acir.extract %370 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc333)
    %375 = acir.extract %370 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc333)
    %376 = acir.extract %370 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc333)
    %377 = acir.extract %370 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc333)
    %378 = acir.extract %370 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc333)
    %379 = acir.extract %370 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc333)
    %380 = acir.extract %370 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc333)
    %381 = acir.extract %370 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc333)
    %382 = acir.aggregate(%373, %374, %375, %372, %376, %377, %378, %379, %380, %381) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %383 = arith.shrui %334, %c15_i32 : i32 loc(#loc334)
    %384 = arith.andi %383, %c31_i32 : i32 loc(#loc334)
    %385 = acir.extract %382 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc335)
    %386 = acir.extract %382 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc335)
    %387 = acir.extract %382 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc335)
    %388 = acir.extract %382 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc335)
    %389 = acir.extract %382 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc335)
    %390 = acir.extract %382 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc335)
    %391 = acir.extract %382 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc335)
    %392 = acir.extract %382 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc335)
    %393 = acir.extract %382 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc335)
    %394 = acir.aggregate(%385, %386, %387, %388, %384, %389, %390, %391, %392, %393) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %395 = arith.cmpi eq, %337, %c51_i32 : i32 loc(#loc336)
    %396 = arith.cmpi eq, %337, %c35_i32 : i32 loc(#loc337)
    %397 = arith.ori %395, %396 : i1 loc(#loc338)
    %398 = arith.cmpi eq, %337, %c99_i32 : i32 loc(#loc339)
    %399 = arith.ori %397, %398 : i1 loc(#loc338)
    %400 = arith.shrui %334, %c20_i32 : i32 loc(#loc340)
    %401 = arith.andi %400, %c31_i32 : i32 loc(#loc340)
    %402 = acir.extract %394 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc341)
    %403 = acir.extract %394 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc341)
    %404 = acir.extract %394 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc341)
    %405 = acir.extract %394 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc341)
    %406 = acir.extract %394 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc341)
    %407 = acir.extract %394 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc341)
    %408 = acir.extract %394 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc341)
    %409 = acir.extract %394 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc341)
    %410 = acir.extract %394 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc341)
    %411 = acir.aggregate(%402, %403, %404, %405, %406, %401, %407, %408, %409, %410) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %412 = arith.select %399, %411, %394 : !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %413 = arith.cmpi eq, %337, %c19_i32 : i32 loc(#loc342)
    %414 = arith.ori %395, %413 : i1 loc(#loc338)
    %415 = arith.cmpi eq, %337, %c3_i32 : i32 loc(#loc343)
    %416 = arith.ori %414, %415 : i1 loc(#loc338)
    %417 = arith.cmpi eq, %337, %c55_i32 : i32 loc(#loc344)
    %418 = arith.ori %416, %417 : i1 loc(#loc338)
    %419 = arith.cmpi eq, %337, %c23_i32 : i32 loc(#loc345)
    %420 = arith.ori %418, %419 : i1 loc(#loc338)
    %421 = arith.cmpi eq, %337, %c111_i32 : i32 loc(#loc346)
    %422 = arith.ori %420, %421 : i1 loc(#loc338)
    %423 = arith.cmpi eq, %337, %c103_i32 : i32 loc(#loc347)
    %424 = arith.ori %422, %423 : i1 loc(#loc338)
    %425 = acir.extract %412 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc348)
    %426 = acir.extract %412 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc348)
    %427 = acir.extract %412 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc348)
    %428 = acir.extract %412 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc348)
    %429 = acir.extract %412 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc348)
    %430 = acir.extract %412 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc348)
    %431 = acir.extract %412 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc348)
    %432 = acir.extract %412 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc348)
    %433 = acir.extract %412 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc348)
    %434 = acir.aggregate(%425, %426, %427, %428, %429, %430, %431, %424, %432, %433) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %435 = acir.extract %434 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc349)
    %436 = acir.extract %434 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc349)
    %437 = acir.extract %434 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc349)
    %438 = acir.extract %434 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc349)
    %439 = acir.extract %434 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc349)
    %440 = acir.extract %434 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc349)
    %441 = acir.extract %434 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc349)
    %442 = acir.extract %434 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc349)
    %443 = acir.extract %434 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc349)
    %444 = acir.aggregate(%435, %436, %437, %438, %439, %440, %441, %442, %415, %443) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %445 = acir.extract %444 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc350)
    %446 = acir.extract %444 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc350)
    %447 = acir.extract %444 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc350)
    %448 = acir.extract %444 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc350)
    %449 = acir.extract %444 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc350)
    %450 = acir.extract %444 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc350)
    %451 = acir.extract %444 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc350)
    %452 = acir.extract %444 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc350)
    %453 = acir.extract %444 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc350)
    %454 = acir.aggregate(%445, %446, %447, %448, %449, %450, %451, %452, %453, %396) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %455 = arith.ori %417, %419 : i1 loc(#loc338)
    %456 = arith.xori %455, %true : i1 loc(#loc338)
    %457 = arith.andi %336, %456 : i1 loc(#loc338)
    %458 = arith.cmpi ne, %337, %c111_i32 : i32 loc(#loc409)
    %459 = arith.andi %457, %458 : i1 loc(#loc338)
    %460 = arith.cmpi ne, %337, %c99_i32 : i32 loc(#loc410)
    %461 = arith.andi %459, %460 : i1 loc(#loc338)
    %462 = arith.cmpi ne, %337, %c35_i32 : i32 loc(#loc411)
    %463 = arith.andi %461, %462 : i1 loc(#loc338)
    %464 = arith.ori %413, %415 : i1 loc(#loc338)
    %465 = arith.ori %464, %423 : i1 loc(#loc338)
    %466 = arith.xori %465, %true : i1 loc(#loc338)
    %467 = arith.andi %463, %466 : i1 loc(#loc338)
    acir.path %467 {branch = 6 : i64, kind = "branch", txn = 1 : i64} loc(#loc354)
    %468 = arith.xori %400, %c2048_i32 : i32 loc(#loc412)
    %469 = arith.subi %468, %c2048_i32 : i32 loc(#loc413)
    %470 = acir.extract %454 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc356)
    %471 = acir.extract %454 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc356)
    %472 = acir.extract %454 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc356)
    %473 = acir.extract %454 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc356)
    %474 = acir.extract %454 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc356)
    %475 = acir.extract %454 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc356)
    %476 = acir.extract %454 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc356)
    %477 = acir.extract %454 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc356)
    %478 = acir.extract %454 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc356)
    %479 = acir.aggregate(%470, %471, %472, %473, %474, %475, %469, %476, %477, %478) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %480 = arith.select %465, %479, %454 : !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %481 = arith.shli %360, %c5_i32 : i32 loc(#loc357)
    %482 = arith.ori %481, %372 : i32 loc(#loc414)
    %483 = arith.xori %482, %c2048_i32 : i32 loc(#loc415)
    %484 = arith.subi %483, %c2048_i32 : i32 loc(#loc416)
    %485 = acir.aggregate(%470, %471, %472, %473, %474, %475, %484, %476, %477, %478) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %486 = arith.select %396, %485, %480 : !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %487 = arith.shrui %334, %c31_i32 : i32 loc(#loc359)
    %488 = arith.shli %487, %c12_i32 : i32 loc(#loc359)
    %489 = arith.andi %371, %c1_i32 : i32 loc(#loc360)
    %490 = arith.shli %489, %c11_i32 : i32 loc(#loc361)
    %491 = arith.ori %488, %490 : i32 loc(#loc362)
    %492 = arith.andi %360, %c63_i32 : i32 loc(#loc363)
    %493 = arith.shli %492, %c5_i32 : i32 loc(#loc364)
    %494 = arith.ori %491, %493 : i32 loc(#loc362)
    %495 = arith.shrui %334, %c8_i32 : i32 loc(#loc365)
    %496 = arith.andi %495, %c15_i32 : i32 loc(#loc365)
    %497 = arith.shli %496, %c1_i32 : i32 loc(#loc366)
    %498 = arith.ori %494, %497 : i32 loc(#loc367)
    %499 = arith.xori %498, %c4096_i32 : i32 loc(#loc417)
    %500 = arith.subi %499, %c4096_i32 : i32 loc(#loc418)
    %501 = acir.aggregate(%470, %471, %472, %473, %474, %475, %500, %476, %477, %478) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %502 = arith.select %398, %501, %486 : !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %503 = acir.extract %454 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc369)
    %504 = acir.aggregate(%470, %471, %472, %473, %c0_i32, %475, %503, %476, %477, %478) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %505 = arith.shli %487, %c20_i32 : i32 loc(#loc370)
    %506 = arith.andi %348, %c255_i32 : i32 loc(#loc371)
    %507 = arith.shli %506, %c12_i32 : i32 loc(#loc372)
    %508 = arith.ori %505, %507 : i32 loc(#loc373)
    %509 = arith.andi %400, %c1_i32 : i32 loc(#loc374)
    %510 = arith.shli %509, %c11_i32 : i32 loc(#loc375)
    %511 = arith.ori %508, %510 : i32 loc(#loc373)
    %512 = arith.shrui %334, %c21_i32 : i32 loc(#loc376)
    %513 = arith.andi %512, %c1023_i32 : i32 loc(#loc376)
    %514 = arith.shli %513, %c1_i32 : i32 loc(#loc377)
    %515 = arith.ori %511, %514 : i32 loc(#loc367)
    %516 = arith.xori %515, %c1048576_i32 : i32 loc(#loc419)
    %517 = arith.subi %516, %c1048576_i32 : i32 loc(#loc420)
    %518 = acir.extract %504 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc379)
    %519 = acir.extract %504 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc379)
    %520 = acir.extract %504 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc379)
    %521 = acir.extract %504 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc379)
    %522 = acir.extract %504 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc379)
    %523 = acir.extract %504 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc379)
    %524 = acir.extract %504 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc379)
    %525 = acir.extract %504 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc379)
    %526 = acir.extract %504 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc379)
    %527 = acir.aggregate(%518, %519, %520, %521, %522, %523, %517, %524, %525, %526) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %528 = arith.select %421, %527, %502 : !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %529 = arith.andi %334, %c-4096_i32 : i32 loc(#loc380)
    %530 = acir.aggregate(%518, %519, %520, %521, %522, %523, %529, %524, %525, %526) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %531 = arith.select %455, %530, %528 : !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %532 = acir.extract %531 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc381)
    %533 = acir.extract %531 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc382)
    %534 = arith.ori %532, %533 : i1 loc(#loc338)
    %535 = arith.andi %336, %534 : i1 loc(#loc338)
    %536 = acir.extract %531 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc383)
    %537 = arith.cmpi eq, %536, %c2_i32 : i32 loc(#loc383)
    acir.path %535 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc384)
    %538 = arith.andi %336, %395 : i1 loc(#loc338)
    %539 = acir.extract %531 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc385)
    %540 = arith.cmpi eq, %539, %c0_i32 : i32 loc(#loc385)
    %541 = arith.cmpi eq, %539, %c32_i32 : i32 loc(#loc386)
    %542 = arith.cmpi eq, %536, %c0_i32 : i32 loc(#loc387)
    %543 = arith.cmpi eq, %536, %c5_i32 : i32 loc(#loc388)
    %544 = arith.ori %542, %543 : i1 loc(#loc338)
    %545 = arith.andi %541, %544 : i1 loc(#loc338)
    %546 = arith.ori %540, %545 : i1 loc(#loc338)
    acir.path %538 {branch = 8 : i64, kind = "branch", txn = 1 : i64} loc(#loc389)
    %547 = arith.andi %336, %423 : i1 loc(#loc338)
    acir.path %547 {branch = 9 : i64, kind = "branch", txn = 1 : i64} loc(#loc390)
    %548 = arith.cmpi eq, %536, %c1_i32 : i32 loc(#loc391)
    %549 = arith.andi %413, %548 : i1 loc(#loc338)
    %550 = arith.andi %336, %549 : i1 loc(#loc338)
    acir.path %550 {branch = 10 : i64, kind = "branch", txn = 1 : i64} loc(#loc392)
    %551 = arith.andi %413, %543 : i1 loc(#loc338)
    %552 = arith.andi %336, %551 : i1 loc(#loc338)
    %553 = arith.ori %540, %541 : i1 loc(#loc338)
    acir.path %552 {branch = 11 : i64, kind = "branch", txn = 1 : i64} loc(#loc393)
    %554 = arith.select %336, %531, %335 : !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc324)
    %555 = acir.fire "top.cpu.decode_stage.decode_instruction#1" %true {successors = [0, 2, 6], txn = 1 : i64}
    %556 = arith.cmpi ne, %337, %c55_i32 : i32 loc(#loc344)
    %557 = arith.cmpi ne, %337, %c23_i32 : i32 loc(#loc345)
    %558 = arith.cmpi ne, %337, %c19_i32 : i32 loc(#loc342)
    %559 = arith.cmpi ne, %337, %c3_i32 : i32 loc(#loc343)
    %560 = arith.cmpi ne, %337, %c103_i32 : i32 loc(#loc347)
    %561 = arith.andi %559, %560 : i1
    %562 = arith.andi %558, %561 : i1
    %563 = arith.andi %462, %562 : i1
    %564 = arith.andi %460, %563 : i1
    %565 = arith.andi %458, %564 : i1
    %566 = arith.andi %557, %565 : i1
    %567 = arith.andi %556, %566 : i1
    %568 = arith.andi %336, %567 : i1
    %569 = arith.andi %555, %568 : i1
    acir.effect %569(%395) {branch = 6 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> () loc(#loc354)
    %570 = arith.andi %555, %535 : i1
    acir.effect %570(%537) {branch = 7 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> () loc(#loc384)
    %571 = arith.andi %555, %538 : i1
    acir.effect %571(%546) {branch = 8 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> () loc(#loc389)
    %572 = arith.andi %555, %547 : i1
    acir.effect %572(%542) {branch = 9 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> () loc(#loc390)
    %573 = arith.andi %555, %550 : i1
    acir.effect %573(%540) {branch = 10 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> () loc(#loc392)
    %574 = arith.andi %555, %552 : i1
    acir.effect %574(%553) {branch = 11 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> () loc(#loc393)
    acir.effect %555(%554) {branch = 5 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "set", port = 0 : i64, resource = "top.cpu.decode_stage.instruction", txn = 1 : i64} : (i1, !acir.struct<"src_isa_e31afbf3.Instruction">) -> () loc(#loc200)
    acir.path %true {branch = 12 : i64, kind = "branch", txn = 2 : i64} loc(#loc201)
    acir.path %true {branch = 12 : i64, frames = ["top.cpu.decode_stage:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc201)
    %575 = arith.ori %285, %288 : i1
    %576 = arith.xori %575, %true : i1
    %577 = arith.ori %575, %290 : i1
    acir.path %576 {branch = 13 : i64, kind = "branch", txn = 2 : i64}
    %value_16, %ready_17 = acir.read(%576) {branch = 13 : i64, frames = ["top.cpu.decode_stage:advance"], method = "read", port = 0 : i64, resource = "top.cpu.if_id", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Fetched">, i1)
    %578 = arith.andi %290, %577 : i1
    %579 = acir.bind %value_16 : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc202)
    %value_18, %ready_19 = acir.read(%576) {branch = 13 : i64, frames = ["top.cpu.decode_stage:advance"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Completed">, i1)
    %580 = acir.bind %value_18 : (!acir.struct<"src_isa_e31afbf3.Completed">) -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc203)
    acir.path %576 {branch = 13 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.decode_stage:inspect"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64}
    %value_20, %ready_21 = acir.read(%576) {branch = 13 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.decode_stage:inspect"], method = "get", port = 0 : i64, resource = "top.cpu.decode_stage.instruction", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Instruction">, i1) loc(#loc1)
    %581 = arith.andi %577, %ready_21 : i1 loc(#loc1)
    %582 = arith.ori %575, %581 : i1 loc(#loc1)
    %583 = arith.andi %578, %582 : i1 loc(#loc1)
    %584 = acir.bind %value_20 : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc204)
    %585 = acir.extract %584 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc205)
    acir.path %576 {branch = 13 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64}
    %586 = arith.cmpi eq, %585, %c0_i32 : i32 loc(#loc206)
    %587 = arith.cmpi ne, %585, %c0_i32 : i32 loc(#loc206)
    %588 = arith.select %586, %c0_i32, %26 : i32
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc207)
    %589 = arith.andi %576, %587 : i1
    %590 = arith.xori %589, %true : i1
    %591 = arith.ori %590, %581 : i1
    %592 = acir.extract %580 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i1 loc(#loc208)
    %593 = arith.ori %590, %591 : i1
    %594 = arith.andi %583, %593 : i1
    %595 = acir.extract %580 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc209)
    %596 = arith.cmpi eq, %595, %585 : i32 loc(#loc209)
    %597 = arith.andi %592, %596 : i1
    %598 = arith.xori %592, %true : i1
    %599 = arith.ori %598, %ready_21 : i1
    %600 = arith.xori %597, %true : i1
    %601 = arith.select %597, %600, %false : i1
    %602 = arith.andi %591, %599 : i1
    %603 = arith.ori %590, %602 : i1
    %604 = arith.andi %594, %603 : i1
    %605 = arith.andi %589, %600 : i1
    %606 = arith.xori %605, %true : i1 loc(#loc210)
    %607 = arith.ori %606, %602 : i1
    %608 = arith.extui %585 : i32 to i64 loc(#loc210)
    %609 = arith.cmpi ult, %608, %c32_i64 : i64 loc(#loc210)
    %610 = arith.andi %ready_21, %609 : i1 loc(#loc210)
    %611 = arith.andi %607, %610 : i1 loc(#loc210)
    %612 = arith.ori %606, %611 : i1 loc(#loc210)
    %613 = arith.andi %604, %612 : i1 loc(#loc210)
    acir.path %605 {branch = 14 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %value_22, %ready_23 = acir.read(%605, %608) {array = "top.cpu.rf.registers", branch = 14 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = 2 : i64} : (i1, i64) -> (i32, i1) loc(#loc210)
    %614 = arith.cmpi eq, %608, %c0_i64 : i64 loc(#loc210)
    %615 = arith.andi %605, %614 : i1 loc(#loc210)
    acir.path %615 {branch = 15 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %616 = arith.cmpi eq, %608, %c1_i64 : i64 loc(#loc210)
    %617 = arith.andi %605, %616 : i1 loc(#loc210)
    acir.path %617 {branch = 16 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %618 = arith.cmpi eq, %608, %c2_i64 : i64 loc(#loc210)
    %619 = arith.andi %605, %618 : i1 loc(#loc210)
    acir.path %619 {branch = 17 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %620 = arith.cmpi eq, %608, %c3_i64 : i64 loc(#loc210)
    %621 = arith.andi %605, %620 : i1 loc(#loc210)
    acir.path %621 {branch = 18 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %622 = arith.cmpi eq, %608, %c4_i64 : i64 loc(#loc210)
    %623 = arith.andi %605, %622 : i1 loc(#loc210)
    acir.path %623 {branch = 19 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %624 = arith.cmpi eq, %608, %c5_i64 : i64 loc(#loc210)
    %625 = arith.andi %605, %624 : i1 loc(#loc210)
    acir.path %625 {branch = 20 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %626 = arith.cmpi eq, %608, %c6_i64 : i64 loc(#loc210)
    %627 = arith.andi %605, %626 : i1 loc(#loc210)
    acir.path %627 {branch = 21 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %628 = arith.cmpi eq, %608, %c7_i64 : i64 loc(#loc210)
    %629 = arith.andi %605, %628 : i1 loc(#loc210)
    acir.path %629 {branch = 22 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %630 = arith.cmpi eq, %608, %c8_i64 : i64 loc(#loc210)
    %631 = arith.andi %605, %630 : i1 loc(#loc210)
    acir.path %631 {branch = 23 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %632 = arith.cmpi eq, %608, %c9_i64 : i64 loc(#loc210)
    %633 = arith.andi %605, %632 : i1 loc(#loc210)
    acir.path %633 {branch = 24 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %634 = arith.cmpi eq, %608, %c10_i64 : i64 loc(#loc210)
    %635 = arith.andi %605, %634 : i1 loc(#loc210)
    acir.path %635 {branch = 25 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %636 = arith.cmpi eq, %608, %c11_i64 : i64 loc(#loc210)
    %637 = arith.andi %605, %636 : i1 loc(#loc210)
    acir.path %637 {branch = 26 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %638 = arith.cmpi eq, %608, %c12_i64 : i64 loc(#loc210)
    %639 = arith.andi %605, %638 : i1 loc(#loc210)
    acir.path %639 {branch = 27 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %640 = arith.cmpi eq, %608, %c13_i64 : i64 loc(#loc210)
    %641 = arith.andi %605, %640 : i1 loc(#loc210)
    acir.path %641 {branch = 28 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %642 = arith.cmpi eq, %608, %c14_i64 : i64 loc(#loc210)
    %643 = arith.andi %605, %642 : i1 loc(#loc210)
    acir.path %643 {branch = 29 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %644 = arith.cmpi eq, %608, %c15_i64 : i64 loc(#loc210)
    %645 = arith.andi %605, %644 : i1 loc(#loc210)
    acir.path %645 {branch = 30 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %646 = arith.cmpi eq, %608, %c16_i64 : i64 loc(#loc210)
    %647 = arith.andi %605, %646 : i1 loc(#loc210)
    acir.path %647 {branch = 31 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %648 = arith.cmpi eq, %608, %c17_i64 : i64 loc(#loc210)
    %649 = arith.andi %605, %648 : i1 loc(#loc210)
    acir.path %649 {branch = 32 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %650 = arith.cmpi eq, %608, %c18_i64 : i64 loc(#loc210)
    %651 = arith.andi %605, %650 : i1 loc(#loc210)
    acir.path %651 {branch = 33 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %652 = arith.cmpi eq, %608, %c19_i64 : i64 loc(#loc210)
    %653 = arith.andi %605, %652 : i1 loc(#loc210)
    acir.path %653 {branch = 34 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %654 = arith.cmpi eq, %608, %c20_i64 : i64 loc(#loc210)
    %655 = arith.andi %605, %654 : i1 loc(#loc210)
    acir.path %655 {branch = 35 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %656 = arith.cmpi eq, %608, %c21_i64 : i64 loc(#loc210)
    %657 = arith.andi %605, %656 : i1 loc(#loc210)
    acir.path %657 {branch = 36 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %658 = arith.cmpi eq, %608, %c22_i64 : i64 loc(#loc210)
    %659 = arith.andi %605, %658 : i1 loc(#loc210)
    acir.path %659 {branch = 37 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %660 = arith.cmpi eq, %608, %c23_i64 : i64 loc(#loc210)
    %661 = arith.andi %605, %660 : i1 loc(#loc210)
    acir.path %661 {branch = 38 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %662 = arith.cmpi eq, %608, %c24_i64 : i64 loc(#loc210)
    %663 = arith.andi %605, %662 : i1 loc(#loc210)
    acir.path %663 {branch = 39 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %664 = arith.cmpi eq, %608, %c25_i64 : i64 loc(#loc210)
    %665 = arith.andi %605, %664 : i1 loc(#loc210)
    acir.path %665 {branch = 40 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %666 = arith.cmpi eq, %608, %c26_i64 : i64 loc(#loc210)
    %667 = arith.andi %605, %666 : i1 loc(#loc210)
    acir.path %667 {branch = 41 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %668 = arith.cmpi eq, %608, %c27_i64 : i64 loc(#loc210)
    %669 = arith.andi %605, %668 : i1 loc(#loc210)
    acir.path %669 {branch = 42 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %670 = arith.cmpi eq, %608, %c28_i64 : i64 loc(#loc210)
    %671 = arith.andi %605, %670 : i1 loc(#loc210)
    acir.path %671 {branch = 43 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %672 = arith.cmpi eq, %608, %c29_i64 : i64 loc(#loc210)
    %673 = arith.andi %605, %672 : i1 loc(#loc210)
    acir.path %673 {branch = 44 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %674 = arith.cmpi eq, %608, %c30_i64 : i64 loc(#loc210)
    %675 = arith.andi %605, %674 : i1 loc(#loc210)
    acir.path %675 {branch = 45 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %676 = arith.cmpi eq, %608, %c31_i64 : i64 loc(#loc210)
    %677 = arith.andi %605, %676 : i1 loc(#loc210)
    acir.path %677 {branch = 46 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %678 = acir.extract %580 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc211)
    %679 = arith.select %597, %678, %value_22 : i32
    %680 = arith.ori %597, %610 : i1
    %681 = arith.andi %599, %680 : i1
    %682 = arith.ori %586, %599 : i1
    %683 = arith.andi %ready_21, %682 : i1
    %684 = arith.andi %587, %601 : i1
    %685 = arith.select %587, %679, %588 : i32 loc(#loc212)
    %686 = arith.select %587, %681, %ready_21 : i1
    %687 = arith.andi %ready_21, %686 : i1
    %688 = arith.xori %684, %true : i1
    %689 = arith.andi %577, %683 : i1
    %690 = arith.ori %575, %689 : i1
    %691 = arith.andi %613, %690 : i1
    %692 = arith.andi %687, %683 : i1
    %693 = acir.extract %584 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc205)
    acir.path %576 {branch = 13 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64}
    %694 = arith.cmpi eq, %693, %c0_i32 : i32 loc(#loc206)
    %695 = arith.cmpi ne, %693, %c0_i32 : i32 loc(#loc206)
    %696 = arith.select %694, %c0_i32, %26 : i32
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc207)
    %697 = arith.andi %691, %582 : i1
    %698 = arith.andi %576, %695 : i1
    %699 = arith.xori %698, %true : i1
    %700 = arith.ori %699, %581 : i1
    %701 = arith.ori %699, %700 : i1
    %702 = arith.andi %697, %701 : i1
    %703 = arith.cmpi eq, %595, %693 : i32 loc(#loc209)
    %704 = arith.andi %592, %703 : i1
    %705 = arith.xori %704, %true : i1
    %706 = arith.select %704, %705, %false : i1
    %707 = arith.andi %700, %599 : i1
    %708 = arith.ori %699, %707 : i1
    %709 = arith.andi %702, %708 : i1
    %710 = arith.andi %698, %705 : i1
    %711 = arith.xori %710, %true : i1 loc(#loc210)
    %712 = arith.ori %711, %707 : i1
    %713 = arith.extui %693 : i32 to i64 loc(#loc210)
    %714 = arith.cmpi ult, %713, %c32_i64 : i64 loc(#loc210)
    %715 = arith.andi %ready_21, %714 : i1 loc(#loc210)
    %716 = arith.andi %712, %715 : i1 loc(#loc210)
    %717 = arith.ori %711, %716 : i1 loc(#loc210)
    %718 = arith.andi %709, %717 : i1 loc(#loc210)
    acir.path %710 {branch = 47 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %value_24, %ready_25 = acir.read(%710, %713) {array = "top.cpu.rf.registers", branch = 47 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = 2 : i64} : (i1, i64) -> (i32, i1) loc(#loc210)
    %719 = arith.cmpi eq, %713, %c0_i64 : i64 loc(#loc210)
    %720 = arith.andi %710, %719 : i1 loc(#loc210)
    acir.path %720 {branch = 48 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %721 = arith.cmpi eq, %713, %c1_i64 : i64 loc(#loc210)
    %722 = arith.andi %710, %721 : i1 loc(#loc210)
    acir.path %722 {branch = 49 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %723 = arith.cmpi eq, %713, %c2_i64 : i64 loc(#loc210)
    %724 = arith.andi %710, %723 : i1 loc(#loc210)
    acir.path %724 {branch = 50 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %725 = arith.cmpi eq, %713, %c3_i64 : i64 loc(#loc210)
    %726 = arith.andi %710, %725 : i1 loc(#loc210)
    acir.path %726 {branch = 51 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %727 = arith.cmpi eq, %713, %c4_i64 : i64 loc(#loc210)
    %728 = arith.andi %710, %727 : i1 loc(#loc210)
    acir.path %728 {branch = 52 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %729 = arith.cmpi eq, %713, %c5_i64 : i64 loc(#loc210)
    %730 = arith.andi %710, %729 : i1 loc(#loc210)
    acir.path %730 {branch = 53 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %731 = arith.cmpi eq, %713, %c6_i64 : i64 loc(#loc210)
    %732 = arith.andi %710, %731 : i1 loc(#loc210)
    acir.path %732 {branch = 54 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %733 = arith.cmpi eq, %713, %c7_i64 : i64 loc(#loc210)
    %734 = arith.andi %710, %733 : i1 loc(#loc210)
    acir.path %734 {branch = 55 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %735 = arith.cmpi eq, %713, %c8_i64 : i64 loc(#loc210)
    %736 = arith.andi %710, %735 : i1 loc(#loc210)
    acir.path %736 {branch = 56 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %737 = arith.cmpi eq, %713, %c9_i64 : i64 loc(#loc210)
    %738 = arith.andi %710, %737 : i1 loc(#loc210)
    acir.path %738 {branch = 57 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %739 = arith.cmpi eq, %713, %c10_i64 : i64 loc(#loc210)
    %740 = arith.andi %710, %739 : i1 loc(#loc210)
    acir.path %740 {branch = 58 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %741 = arith.cmpi eq, %713, %c11_i64 : i64 loc(#loc210)
    %742 = arith.andi %710, %741 : i1 loc(#loc210)
    acir.path %742 {branch = 59 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %743 = arith.cmpi eq, %713, %c12_i64 : i64 loc(#loc210)
    %744 = arith.andi %710, %743 : i1 loc(#loc210)
    acir.path %744 {branch = 60 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %745 = arith.cmpi eq, %713, %c13_i64 : i64 loc(#loc210)
    %746 = arith.andi %710, %745 : i1 loc(#loc210)
    acir.path %746 {branch = 61 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %747 = arith.cmpi eq, %713, %c14_i64 : i64 loc(#loc210)
    %748 = arith.andi %710, %747 : i1 loc(#loc210)
    acir.path %748 {branch = 62 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %749 = arith.cmpi eq, %713, %c15_i64 : i64 loc(#loc210)
    %750 = arith.andi %710, %749 : i1 loc(#loc210)
    acir.path %750 {branch = 63 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %751 = arith.cmpi eq, %713, %c16_i64 : i64 loc(#loc210)
    %752 = arith.andi %710, %751 : i1 loc(#loc210)
    acir.path %752 {branch = 64 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %753 = arith.cmpi eq, %713, %c17_i64 : i64 loc(#loc210)
    %754 = arith.andi %710, %753 : i1 loc(#loc210)
    acir.path %754 {branch = 65 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %755 = arith.cmpi eq, %713, %c18_i64 : i64 loc(#loc210)
    %756 = arith.andi %710, %755 : i1 loc(#loc210)
    acir.path %756 {branch = 66 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %757 = arith.cmpi eq, %713, %c19_i64 : i64 loc(#loc210)
    %758 = arith.andi %710, %757 : i1 loc(#loc210)
    acir.path %758 {branch = 67 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %759 = arith.cmpi eq, %713, %c20_i64 : i64 loc(#loc210)
    %760 = arith.andi %710, %759 : i1 loc(#loc210)
    acir.path %760 {branch = 68 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %761 = arith.cmpi eq, %713, %c21_i64 : i64 loc(#loc210)
    %762 = arith.andi %710, %761 : i1 loc(#loc210)
    acir.path %762 {branch = 69 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %763 = arith.cmpi eq, %713, %c22_i64 : i64 loc(#loc210)
    %764 = arith.andi %710, %763 : i1 loc(#loc210)
    acir.path %764 {branch = 70 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %765 = arith.cmpi eq, %713, %c23_i64 : i64 loc(#loc210)
    %766 = arith.andi %710, %765 : i1 loc(#loc210)
    acir.path %766 {branch = 71 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %767 = arith.cmpi eq, %713, %c24_i64 : i64 loc(#loc210)
    %768 = arith.andi %710, %767 : i1 loc(#loc210)
    acir.path %768 {branch = 72 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %769 = arith.cmpi eq, %713, %c25_i64 : i64 loc(#loc210)
    %770 = arith.andi %710, %769 : i1 loc(#loc210)
    acir.path %770 {branch = 73 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %771 = arith.cmpi eq, %713, %c26_i64 : i64 loc(#loc210)
    %772 = arith.andi %710, %771 : i1 loc(#loc210)
    acir.path %772 {branch = 74 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %773 = arith.cmpi eq, %713, %c27_i64 : i64 loc(#loc210)
    %774 = arith.andi %710, %773 : i1 loc(#loc210)
    acir.path %774 {branch = 75 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %775 = arith.cmpi eq, %713, %c28_i64 : i64 loc(#loc210)
    %776 = arith.andi %710, %775 : i1 loc(#loc210)
    acir.path %776 {branch = 76 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %777 = arith.cmpi eq, %713, %c29_i64 : i64 loc(#loc210)
    %778 = arith.andi %710, %777 : i1 loc(#loc210)
    acir.path %778 {branch = 77 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %779 = arith.cmpi eq, %713, %c30_i64 : i64 loc(#loc210)
    %780 = arith.andi %710, %779 : i1 loc(#loc210)
    acir.path %780 {branch = 78 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %781 = arith.cmpi eq, %713, %c31_i64 : i64 loc(#loc210)
    %782 = arith.andi %710, %781 : i1 loc(#loc210)
    acir.path %782 {branch = 79 : i64, kind = "branch", txn = 2 : i64} loc(#loc210)
    %783 = arith.select %704, %678, %value_24 : i32
    %784 = arith.ori %704, %715 : i1
    %785 = arith.andi %599, %784 : i1
    %786 = arith.ori %694, %599 : i1
    %787 = arith.andi %ready_21, %786 : i1
    %788 = arith.andi %695, %706 : i1
    %789 = arith.select %695, %783, %696 : i32 loc(#loc213)
    %790 = arith.select %695, %785, %ready_21 : i1
    %791 = arith.andi %ready_21, %790 : i1
    %792 = arith.xori %788, %true : i1
    %793 = arith.andi %577, %787 : i1
    %794 = arith.ori %575, %793 : i1
    %795 = arith.andi %718, %794 : i1
    %796 = arith.andi %791, %787 : i1
    %797 = acir.extract %579 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> i1 loc(#loc214)
    %798 = acir.extract %579 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> i32 loc(#loc215)
    %799 = acir.extract %579 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> i32 loc(#loc216)
    %800 = arith.andi %692, %796 : i1 loc(#loc217)
    %801 = acir.aggregate(%797, %798, %799, %584, %685, %789, %false) : (i1, i32, i32, !acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc217)
    %802 = acir.aggregate() : () -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc218)
    %803 = acir.aggregate(%false, %c0_i32, %c0_i32, %802, %c0_i32, %c0_i32, %false) : (i1, i32, i32, !acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc219)
    %804 = acir.extract %803 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i1 loc(#loc220)
    %805 = acir.extract %803 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc220)
    %806 = acir.extract %803 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc220)
    %807 = acir.extract %803 {field = "instruction"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc220)
    %808 = acir.extract %803 {field = "lhs"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc220)
    %809 = acir.extract %803 {field = "rhs"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc220)
    %810 = acir.aggregate(%804, %805, %806, %807, %808, %809, %285) : (i1, i32, i32, !acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc219)
    %811 = arith.select %575, %810, %801 : !acir.struct<"src_isa_e31afbf3.Decoded">
    %812 = arith.select %575, %281, %800 : i1
    %813 = arith.andi %290, %812 : i1
    acir.path %813 {branch = 80 : i64, kind = "branch", txn = 2 : i64}
    %814 = acir.fire "top.cpu.decode_stage.advance#2" %795 {successors = [], txn = 2 : i64}
    %815 = arith.andi %814, %331 : i1
    acir.effect %815(%688) {branch = 13 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "check", port = 0 : i64, resource = "", txn = 2 : i64} : (i1, i1) -> ()
    acir.effect %815(%792) {branch = 13 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "check", port = 0 : i64, resource = "", txn = 2 : i64} : (i1, i1) -> ()
    %816 = arith.andi %814, %813 : i1
    acir.effect %816(%811) {branch = 80 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.cpu.id_ex", txn = 2 : i64} : (i1, !acir.struct<"src_isa_e31afbf3.Decoded">) -> ()
    %817 = acir.extract %274 {field = "packet"} : (!acir.struct<"src_isa_e31afbf3.ExecutionResult">) -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc221)
    %818 = acir.export %817 {acir.id = "v27", acir.source_type = "src_isa_e31afbf3.Executed"} : !acir.struct<"src_isa_e31afbf3.Executed"> -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc222)
    acir.path %true {branch = 81 : i64, kind = "branch", txn = 3 : i64} loc(#loc223)
    acir.path %true {branch = 81 : i64, frames = ["top.cpu.execute:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc223)
    acir.path %275 {branch = 82 : i64, kind = "branch", txn = 3 : i64}
    %819 = arith.cmpi ne, %90, %c111_i32 : i32 loc(#loc282)
    %820 = arith.cmpi ne, %90, %c103_i32 : i32 loc(#loc286)
    %821 = arith.andi %155, %174 : i1
    %822 = arith.andi %150, %821 : i1
    %823 = arith.andi %130, %822 : i1
    %824 = arith.andi %119, %823 : i1
    %825 = arith.andi %820, %824 : i1
    %826 = arith.andi %98, %825 : i1
    %827 = arith.andi %819, %826 : i1
    %828 = arith.select %827, %227, %true : i1 loc(#loc321)
    %829 = acir.fire "top.cpu.execute.advance#3" %true checks(%204, %828) {successors = [], txn = 3 : i64}
    %830 = arith.andi %204, %227 : i1
    %831 = arith.select %174, %830, %204 : i1
    %832 = arith.select %155, %831, %204 : i1
    %833 = arith.select %150, %832, %204 : i1
    %834 = arith.select %130, %833, %204 : i1
    %835 = arith.select %119, %834, %204 : i1
    %836 = arith.select %105, %204, %835 : i1
    %837 = arith.select %98, %836, %204 : i1
    %838 = arith.select %96, %204, %837 : i1
    %839 = arith.andi %829, %838 : i1
    acir.effect %839(%818) {branch = 82 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.cpu.ex_mem", txn = 3 : i64} : (i1, !acir.struct<"src_isa_e31afbf3.Executed">) -> ()
    acir.path %true {branch = 83 : i64, kind = "branch", txn = 4 : i64} loc(#loc224)
    acir.path %true {branch = 83 : i64, frames = ["top.cpu.memory:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc224)
    %value_26, %ready_27 = acir.read(%true) {branch = 83 : i64, frames = ["top.cpu.memory:advance"], method = "read", port = 0 : i64, resource = "top.cpu.ex_mem", txn = 4 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Executed">, i1)
    %840 = acir.bind %value_26 : (!acir.struct<"src_isa_e31afbf3.Executed">) -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc225)
    %841 = acir.extract %840 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc226)
    %842 = acir.aggregate(%false, %c0_i32, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc227)
    %843 = acir.extract %840 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc228)
    %844 = acir.extract %840 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc229)
    %845 = arith.ori %843, %844 : i1
    "acir.resource.array"() <{name = "top.cpu.memory.words", resources = ["top.cpu.memory.words.0", "top.cpu.memory.words.1", "top.cpu.memory.words.2", "top.cpu.memory.words.3", "top.cpu.memory.words.4", "top.cpu.memory.words.5", "top.cpu.memory.words.6", "top.cpu.memory.words.7", "top.cpu.memory.words.8", "top.cpu.memory.words.9", "top.cpu.memory.words.10", "top.cpu.memory.words.11", "top.cpu.memory.words.12", "top.cpu.memory.words.13", "top.cpu.memory.words.14", "top.cpu.memory.words.15", "top.cpu.memory.words.16", "top.cpu.memory.words.17", "top.cpu.memory.words.18", "top.cpu.memory.words.19", "top.cpu.memory.words.20", "top.cpu.memory.words.21", "top.cpu.memory.words.22", "top.cpu.memory.words.23", "top.cpu.memory.words.24", "top.cpu.memory.words.25", "top.cpu.memory.words.26", "top.cpu.memory.words.27", "top.cpu.memory.words.28", "top.cpu.memory.words.29", "top.cpu.memory.words.30", "top.cpu.memory.words.31", "top.cpu.memory.words.32", "top.cpu.memory.words.33", "top.cpu.memory.words.34", "top.cpu.memory.words.35", "top.cpu.memory.words.36", "top.cpu.memory.words.37", "top.cpu.memory.words.38", "top.cpu.memory.words.39", "top.cpu.memory.words.40", "top.cpu.memory.words.41", "top.cpu.memory.words.42", "top.cpu.memory.words.43", "top.cpu.memory.words.44", "top.cpu.memory.words.45", "top.cpu.memory.words.46", "top.cpu.memory.words.47", "top.cpu.memory.words.48", "top.cpu.memory.words.49", "top.cpu.memory.words.50", "top.cpu.memory.words.51", "top.cpu.memory.words.52", "top.cpu.memory.words.53", "top.cpu.memory.words.54", "top.cpu.memory.words.55", "top.cpu.memory.words.56", "top.cpu.memory.words.57", "top.cpu.memory.words.58", "top.cpu.memory.words.59", "top.cpu.memory.words.60", "top.cpu.memory.words.61", "top.cpu.memory.words.62", "top.cpu.memory.words.63"], shape = [64]}> : () -> () loc(#loc230)
    %846 = acir.extract %840 {field = "alu"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc231)
    %847 = arith.andi %846, %c3_i32 : i32 loc(#loc231)
    %848 = arith.cmpi eq, %847, %c0_i32 : i32 loc(#loc231)
    %849 = arith.cmpi uge, %846, %c4096_i32 : i32 loc(#loc232)
    %850 = arith.andi %848, %849 : i1
    %851 = arith.subi %846, %c4096_i32 : i32 loc(#loc233)
    %852 = arith.cmpi ult, %851, %c256_i32 : i32 loc(#loc233)
    %853 = arith.andi %850, %852 : i1
    acir.path %845 {branch = 84 : i64, kind = "branch", txn = 4 : i64} loc(#loc234)
    %854 = arith.shrui %851, %c2_i32 : i32 loc(#loc235)
    %855 = arith.andi %845, %843 : i1
    %856 = arith.extui %854 : i32 to i64 loc(#loc236)
    %857 = arith.cmpi ult, %856, %c64_i64 : i64 loc(#loc236)
    acir.path %855 {branch = 85 : i64, kind = "branch", txn = 4 : i64}
    %value_28, %ready_29 = acir.read(%855, %856) {array = "top.cpu.memory.words", branch = 85 : i64, frames = ["top.cpu.memory:advance"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.memory.words.0", resources = ["top.cpu.memory.words.0", "top.cpu.memory.words.1", "top.cpu.memory.words.2", "top.cpu.memory.words.3", "top.cpu.memory.words.4", "top.cpu.memory.words.5", "top.cpu.memory.words.6", "top.cpu.memory.words.7", "top.cpu.memory.words.8", "top.cpu.memory.words.9", "top.cpu.memory.words.10", "top.cpu.memory.words.11", "top.cpu.memory.words.12", "top.cpu.memory.words.13", "top.cpu.memory.words.14", "top.cpu.memory.words.15", "top.cpu.memory.words.16", "top.cpu.memory.words.17", "top.cpu.memory.words.18", "top.cpu.memory.words.19", "top.cpu.memory.words.20", "top.cpu.memory.words.21", "top.cpu.memory.words.22", "top.cpu.memory.words.23", "top.cpu.memory.words.24", "top.cpu.memory.words.25", "top.cpu.memory.words.26", "top.cpu.memory.words.27", "top.cpu.memory.words.28", "top.cpu.memory.words.29", "top.cpu.memory.words.30", "top.cpu.memory.words.31", "top.cpu.memory.words.32", "top.cpu.memory.words.33", "top.cpu.memory.words.34", "top.cpu.memory.words.35", "top.cpu.memory.words.36", "top.cpu.memory.words.37", "top.cpu.memory.words.38", "top.cpu.memory.words.39", "top.cpu.memory.words.40", "top.cpu.memory.words.41", "top.cpu.memory.words.42", "top.cpu.memory.words.43", "top.cpu.memory.words.44", "top.cpu.memory.words.45", "top.cpu.memory.words.46", "top.cpu.memory.words.47", "top.cpu.memory.words.48", "top.cpu.memory.words.49", "top.cpu.memory.words.50", "top.cpu.memory.words.51", "top.cpu.memory.words.52", "top.cpu.memory.words.53", "top.cpu.memory.words.54", "top.cpu.memory.words.55", "top.cpu.memory.words.56", "top.cpu.memory.words.57", "top.cpu.memory.words.58", "top.cpu.memory.words.59", "top.cpu.memory.words.60", "top.cpu.memory.words.61", "top.cpu.memory.words.62", "top.cpu.memory.words.63"], txn = 4 : i64} : (i1, i64) -> (i32, i1)
    %858 = arith.cmpi eq, %856, %c0_i64 : i64 loc(#loc237)
    %859 = arith.andi %855, %858 : i1
    acir.path %859 {branch = 86 : i64, kind = "branch", txn = 4 : i64}
    %860 = arith.cmpi eq, %856, %c1_i64 : i64 loc(#loc237)
    %861 = arith.andi %855, %860 : i1
    acir.path %861 {branch = 87 : i64, kind = "branch", txn = 4 : i64}
    %862 = arith.cmpi eq, %856, %c2_i64 : i64 loc(#loc237)
    %863 = arith.andi %855, %862 : i1
    acir.path %863 {branch = 88 : i64, kind = "branch", txn = 4 : i64}
    %864 = arith.cmpi eq, %856, %c3_i64 : i64 loc(#loc237)
    %865 = arith.andi %855, %864 : i1
    acir.path %865 {branch = 89 : i64, kind = "branch", txn = 4 : i64}
    %866 = arith.cmpi eq, %856, %c4_i64 : i64 loc(#loc237)
    %867 = arith.andi %855, %866 : i1
    acir.path %867 {branch = 90 : i64, kind = "branch", txn = 4 : i64}
    %868 = arith.cmpi eq, %856, %c5_i64 : i64 loc(#loc237)
    %869 = arith.andi %855, %868 : i1
    acir.path %869 {branch = 91 : i64, kind = "branch", txn = 4 : i64}
    %870 = arith.cmpi eq, %856, %c6_i64 : i64 loc(#loc237)
    %871 = arith.andi %855, %870 : i1
    acir.path %871 {branch = 92 : i64, kind = "branch", txn = 4 : i64}
    %872 = arith.cmpi eq, %856, %c7_i64 : i64 loc(#loc237)
    %873 = arith.andi %855, %872 : i1
    acir.path %873 {branch = 93 : i64, kind = "branch", txn = 4 : i64}
    %874 = arith.cmpi eq, %856, %c8_i64 : i64 loc(#loc237)
    %875 = arith.andi %855, %874 : i1
    acir.path %875 {branch = 94 : i64, kind = "branch", txn = 4 : i64}
    %876 = arith.cmpi eq, %856, %c9_i64 : i64 loc(#loc237)
    %877 = arith.andi %855, %876 : i1
    acir.path %877 {branch = 95 : i64, kind = "branch", txn = 4 : i64}
    %878 = arith.cmpi eq, %856, %c10_i64 : i64 loc(#loc237)
    %879 = arith.andi %855, %878 : i1
    acir.path %879 {branch = 96 : i64, kind = "branch", txn = 4 : i64}
    %880 = arith.cmpi eq, %856, %c11_i64 : i64 loc(#loc237)
    %881 = arith.andi %855, %880 : i1
    acir.path %881 {branch = 97 : i64, kind = "branch", txn = 4 : i64}
    %882 = arith.cmpi eq, %856, %c12_i64 : i64 loc(#loc237)
    %883 = arith.andi %855, %882 : i1
    acir.path %883 {branch = 98 : i64, kind = "branch", txn = 4 : i64}
    %884 = arith.cmpi eq, %856, %c13_i64 : i64 loc(#loc237)
    %885 = arith.andi %855, %884 : i1
    acir.path %885 {branch = 99 : i64, kind = "branch", txn = 4 : i64}
    %886 = arith.cmpi eq, %856, %c14_i64 : i64 loc(#loc237)
    %887 = arith.andi %855, %886 : i1
    acir.path %887 {branch = 100 : i64, kind = "branch", txn = 4 : i64}
    %888 = arith.cmpi eq, %856, %c15_i64 : i64 loc(#loc237)
    %889 = arith.andi %855, %888 : i1
    acir.path %889 {branch = 101 : i64, kind = "branch", txn = 4 : i64}
    %890 = arith.cmpi eq, %856, %c16_i64 : i64 loc(#loc237)
    %891 = arith.andi %855, %890 : i1
    acir.path %891 {branch = 102 : i64, kind = "branch", txn = 4 : i64}
    %892 = arith.cmpi eq, %856, %c17_i64 : i64 loc(#loc237)
    %893 = arith.andi %855, %892 : i1
    acir.path %893 {branch = 103 : i64, kind = "branch", txn = 4 : i64}
    %894 = arith.cmpi eq, %856, %c18_i64 : i64 loc(#loc237)
    %895 = arith.andi %855, %894 : i1
    acir.path %895 {branch = 104 : i64, kind = "branch", txn = 4 : i64}
    %896 = arith.cmpi eq, %856, %c19_i64 : i64 loc(#loc237)
    %897 = arith.andi %855, %896 : i1
    acir.path %897 {branch = 105 : i64, kind = "branch", txn = 4 : i64}
    %898 = arith.cmpi eq, %856, %c20_i64 : i64 loc(#loc237)
    %899 = arith.andi %855, %898 : i1
    acir.path %899 {branch = 106 : i64, kind = "branch", txn = 4 : i64}
    %900 = arith.cmpi eq, %856, %c21_i64 : i64 loc(#loc237)
    %901 = arith.andi %855, %900 : i1
    acir.path %901 {branch = 107 : i64, kind = "branch", txn = 4 : i64}
    %902 = arith.cmpi eq, %856, %c22_i64 : i64 loc(#loc237)
    %903 = arith.andi %855, %902 : i1
    acir.path %903 {branch = 108 : i64, kind = "branch", txn = 4 : i64}
    %904 = arith.cmpi eq, %856, %c23_i64 : i64 loc(#loc237)
    %905 = arith.andi %855, %904 : i1
    acir.path %905 {branch = 109 : i64, kind = "branch", txn = 4 : i64}
    %906 = arith.cmpi eq, %856, %c24_i64 : i64 loc(#loc237)
    %907 = arith.andi %855, %906 : i1
    acir.path %907 {branch = 110 : i64, kind = "branch", txn = 4 : i64}
    %908 = arith.cmpi eq, %856, %c25_i64 : i64 loc(#loc237)
    %909 = arith.andi %855, %908 : i1
    acir.path %909 {branch = 111 : i64, kind = "branch", txn = 4 : i64}
    %910 = arith.cmpi eq, %856, %c26_i64 : i64 loc(#loc237)
    %911 = arith.andi %855, %910 : i1
    acir.path %911 {branch = 112 : i64, kind = "branch", txn = 4 : i64}
    %912 = arith.cmpi eq, %856, %c27_i64 : i64 loc(#loc237)
    %913 = arith.andi %855, %912 : i1
    acir.path %913 {branch = 113 : i64, kind = "branch", txn = 4 : i64}
    %914 = arith.cmpi eq, %856, %c28_i64 : i64 loc(#loc237)
    %915 = arith.andi %855, %914 : i1
    acir.path %915 {branch = 114 : i64, kind = "branch", txn = 4 : i64}
    %916 = arith.cmpi eq, %856, %c29_i64 : i64 loc(#loc237)
    %917 = arith.andi %855, %916 : i1
    acir.path %917 {branch = 115 : i64, kind = "branch", txn = 4 : i64}
    %918 = arith.cmpi eq, %856, %c30_i64 : i64 loc(#loc237)
    %919 = arith.andi %855, %918 : i1
    acir.path %919 {branch = 116 : i64, kind = "branch", txn = 4 : i64}
    %920 = arith.cmpi eq, %856, %c31_i64 : i64 loc(#loc237)
    %921 = arith.andi %855, %920 : i1
    acir.path %921 {branch = 117 : i64, kind = "branch", txn = 4 : i64}
    %922 = arith.cmpi eq, %856, %c32_i64 : i64 loc(#loc237)
    %923 = arith.andi %855, %922 : i1
    acir.path %923 {branch = 118 : i64, kind = "branch", txn = 4 : i64}
    %924 = arith.cmpi eq, %856, %c33_i64 : i64 loc(#loc237)
    %925 = arith.andi %855, %924 : i1
    acir.path %925 {branch = 119 : i64, kind = "branch", txn = 4 : i64}
    %926 = arith.cmpi eq, %856, %c34_i64 : i64 loc(#loc237)
    %927 = arith.andi %855, %926 : i1
    acir.path %927 {branch = 120 : i64, kind = "branch", txn = 4 : i64}
    %928 = arith.cmpi eq, %856, %c35_i64 : i64 loc(#loc237)
    %929 = arith.andi %855, %928 : i1
    acir.path %929 {branch = 121 : i64, kind = "branch", txn = 4 : i64}
    %930 = arith.cmpi eq, %856, %c36_i64 : i64 loc(#loc237)
    %931 = arith.andi %855, %930 : i1
    acir.path %931 {branch = 122 : i64, kind = "branch", txn = 4 : i64}
    %932 = arith.cmpi eq, %856, %c37_i64 : i64 loc(#loc237)
    %933 = arith.andi %855, %932 : i1
    acir.path %933 {branch = 123 : i64, kind = "branch", txn = 4 : i64}
    %934 = arith.cmpi eq, %856, %c38_i64 : i64 loc(#loc237)
    %935 = arith.andi %855, %934 : i1
    acir.path %935 {branch = 124 : i64, kind = "branch", txn = 4 : i64}
    %936 = arith.cmpi eq, %856, %c39_i64 : i64 loc(#loc237)
    %937 = arith.andi %855, %936 : i1
    acir.path %937 {branch = 125 : i64, kind = "branch", txn = 4 : i64}
    %938 = arith.cmpi eq, %856, %c40_i64 : i64 loc(#loc237)
    %939 = arith.andi %855, %938 : i1
    acir.path %939 {branch = 126 : i64, kind = "branch", txn = 4 : i64}
    %940 = arith.cmpi eq, %856, %c41_i64 : i64 loc(#loc237)
    %941 = arith.andi %855, %940 : i1
    acir.path %941 {branch = 127 : i64, kind = "branch", txn = 4 : i64}
    %942 = arith.cmpi eq, %856, %c42_i64 : i64 loc(#loc237)
    %943 = arith.andi %855, %942 : i1
    acir.path %943 {branch = 128 : i64, kind = "branch", txn = 4 : i64}
    %944 = arith.cmpi eq, %856, %c43_i64 : i64 loc(#loc237)
    %945 = arith.andi %855, %944 : i1
    acir.path %945 {branch = 129 : i64, kind = "branch", txn = 4 : i64}
    %946 = arith.cmpi eq, %856, %c44_i64 : i64 loc(#loc237)
    %947 = arith.andi %855, %946 : i1
    acir.path %947 {branch = 130 : i64, kind = "branch", txn = 4 : i64}
    %948 = arith.cmpi eq, %856, %c45_i64 : i64 loc(#loc237)
    %949 = arith.andi %855, %948 : i1
    acir.path %949 {branch = 131 : i64, kind = "branch", txn = 4 : i64}
    %950 = arith.cmpi eq, %856, %c46_i64 : i64 loc(#loc237)
    %951 = arith.andi %855, %950 : i1
    acir.path %951 {branch = 132 : i64, kind = "branch", txn = 4 : i64}
    %952 = arith.cmpi eq, %856, %c47_i64 : i64 loc(#loc237)
    %953 = arith.andi %855, %952 : i1
    acir.path %953 {branch = 133 : i64, kind = "branch", txn = 4 : i64}
    %954 = arith.cmpi eq, %856, %c48_i64 : i64 loc(#loc237)
    %955 = arith.andi %855, %954 : i1
    acir.path %955 {branch = 134 : i64, kind = "branch", txn = 4 : i64}
    %956 = arith.cmpi eq, %856, %c49_i64 : i64 loc(#loc237)
    %957 = arith.andi %855, %956 : i1
    acir.path %957 {branch = 135 : i64, kind = "branch", txn = 4 : i64}
    %958 = arith.cmpi eq, %856, %c50_i64 : i64 loc(#loc237)
    %959 = arith.andi %855, %958 : i1
    acir.path %959 {branch = 136 : i64, kind = "branch", txn = 4 : i64}
    %960 = arith.cmpi eq, %856, %c51_i64 : i64 loc(#loc237)
    %961 = arith.andi %855, %960 : i1
    acir.path %961 {branch = 137 : i64, kind = "branch", txn = 4 : i64}
    %962 = arith.cmpi eq, %856, %c52_i64 : i64 loc(#loc237)
    %963 = arith.andi %855, %962 : i1
    acir.path %963 {branch = 138 : i64, kind = "branch", txn = 4 : i64}
    %964 = arith.cmpi eq, %856, %c53_i64 : i64 loc(#loc237)
    %965 = arith.andi %855, %964 : i1
    acir.path %965 {branch = 139 : i64, kind = "branch", txn = 4 : i64}
    %966 = arith.cmpi eq, %856, %c54_i64 : i64 loc(#loc237)
    %967 = arith.andi %855, %966 : i1
    acir.path %967 {branch = 140 : i64, kind = "branch", txn = 4 : i64}
    %968 = arith.cmpi eq, %856, %c55_i64 : i64 loc(#loc237)
    %969 = arith.andi %855, %968 : i1
    acir.path %969 {branch = 141 : i64, kind = "branch", txn = 4 : i64}
    %970 = arith.cmpi eq, %856, %c56_i64 : i64 loc(#loc237)
    %971 = arith.andi %855, %970 : i1
    acir.path %971 {branch = 142 : i64, kind = "branch", txn = 4 : i64}
    %972 = arith.cmpi eq, %856, %c57_i64 : i64 loc(#loc237)
    %973 = arith.andi %855, %972 : i1
    acir.path %973 {branch = 143 : i64, kind = "branch", txn = 4 : i64}
    %974 = arith.cmpi eq, %856, %c58_i64 : i64 loc(#loc237)
    %975 = arith.andi %855, %974 : i1
    acir.path %975 {branch = 144 : i64, kind = "branch", txn = 4 : i64}
    %976 = arith.cmpi eq, %856, %c59_i64 : i64 loc(#loc237)
    %977 = arith.andi %855, %976 : i1
    acir.path %977 {branch = 145 : i64, kind = "branch", txn = 4 : i64}
    %978 = arith.cmpi eq, %856, %c60_i64 : i64 loc(#loc237)
    %979 = arith.andi %855, %978 : i1
    acir.path %979 {branch = 146 : i64, kind = "branch", txn = 4 : i64}
    %980 = arith.cmpi eq, %856, %c61_i64 : i64 loc(#loc237)
    %981 = arith.andi %855, %980 : i1
    acir.path %981 {branch = 147 : i64, kind = "branch", txn = 4 : i64}
    %982 = arith.cmpi eq, %856, %c62_i64 : i64 loc(#loc237)
    %983 = arith.andi %855, %982 : i1
    acir.path %983 {branch = 148 : i64, kind = "branch", txn = 4 : i64}
    %984 = arith.cmpi eq, %856, %c63_i64 : i64 loc(#loc237)
    %985 = arith.andi %855, %984 : i1
    acir.path %985 {branch = 149 : i64, kind = "branch", txn = 4 : i64}
    %986 = acir.bind %value_28 : (i32) -> i32 loc(#loc226)
    %987 = arith.select %843, %986, %841 : i32 loc(#loc226)
    %988 = arith.xori %843, %true : i1
    %989 = arith.ori %988, %857 : i1
    %990 = arith.andi %845, %844 : i1
    %991 = acir.extract %840 {field = "store_value"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc238)
    acir.path %990 {branch = 150 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %992 = arith.andi %990, %858 : i1 loc(#loc237)
    acir.path %992 {branch = 151 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %993 = arith.andi %990, %860 : i1 loc(#loc237)
    acir.path %993 {branch = 152 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %994 = arith.andi %990, %862 : i1 loc(#loc237)
    acir.path %994 {branch = 153 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %995 = arith.andi %990, %864 : i1 loc(#loc237)
    acir.path %995 {branch = 154 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %996 = arith.andi %990, %866 : i1 loc(#loc237)
    acir.path %996 {branch = 155 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %997 = arith.andi %990, %868 : i1 loc(#loc237)
    acir.path %997 {branch = 156 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %998 = arith.andi %990, %870 : i1 loc(#loc237)
    acir.path %998 {branch = 157 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %999 = arith.andi %990, %872 : i1 loc(#loc237)
    acir.path %999 {branch = 158 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1000 = arith.andi %990, %874 : i1 loc(#loc237)
    acir.path %1000 {branch = 159 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1001 = arith.andi %990, %876 : i1 loc(#loc237)
    acir.path %1001 {branch = 160 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1002 = arith.andi %990, %878 : i1 loc(#loc237)
    acir.path %1002 {branch = 161 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1003 = arith.andi %990, %880 : i1 loc(#loc237)
    acir.path %1003 {branch = 162 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1004 = arith.andi %990, %882 : i1 loc(#loc237)
    acir.path %1004 {branch = 163 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1005 = arith.andi %990, %884 : i1 loc(#loc237)
    acir.path %1005 {branch = 164 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1006 = arith.andi %990, %886 : i1 loc(#loc237)
    acir.path %1006 {branch = 165 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1007 = arith.andi %990, %888 : i1 loc(#loc237)
    acir.path %1007 {branch = 166 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1008 = arith.andi %990, %890 : i1 loc(#loc237)
    acir.path %1008 {branch = 167 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1009 = arith.andi %990, %892 : i1 loc(#loc237)
    acir.path %1009 {branch = 168 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1010 = arith.andi %990, %894 : i1 loc(#loc237)
    acir.path %1010 {branch = 169 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1011 = arith.andi %990, %896 : i1 loc(#loc237)
    acir.path %1011 {branch = 170 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1012 = arith.andi %990, %898 : i1 loc(#loc237)
    acir.path %1012 {branch = 171 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1013 = arith.andi %990, %900 : i1 loc(#loc237)
    acir.path %1013 {branch = 172 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1014 = arith.andi %990, %902 : i1 loc(#loc237)
    acir.path %1014 {branch = 173 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1015 = arith.andi %990, %904 : i1 loc(#loc237)
    acir.path %1015 {branch = 174 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1016 = arith.andi %990, %906 : i1 loc(#loc237)
    acir.path %1016 {branch = 175 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1017 = arith.andi %990, %908 : i1 loc(#loc237)
    acir.path %1017 {branch = 176 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1018 = arith.andi %990, %910 : i1 loc(#loc237)
    acir.path %1018 {branch = 177 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1019 = arith.andi %990, %912 : i1 loc(#loc237)
    acir.path %1019 {branch = 178 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1020 = arith.andi %990, %914 : i1 loc(#loc237)
    acir.path %1020 {branch = 179 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1021 = arith.andi %990, %916 : i1 loc(#loc237)
    acir.path %1021 {branch = 180 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1022 = arith.andi %990, %918 : i1 loc(#loc237)
    acir.path %1022 {branch = 181 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1023 = arith.andi %990, %920 : i1 loc(#loc237)
    acir.path %1023 {branch = 182 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1024 = arith.andi %990, %922 : i1 loc(#loc237)
    acir.path %1024 {branch = 183 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1025 = arith.andi %990, %924 : i1 loc(#loc237)
    acir.path %1025 {branch = 184 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1026 = arith.andi %990, %926 : i1 loc(#loc237)
    acir.path %1026 {branch = 185 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1027 = arith.andi %990, %928 : i1 loc(#loc237)
    acir.path %1027 {branch = 186 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1028 = arith.andi %990, %930 : i1 loc(#loc237)
    acir.path %1028 {branch = 187 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1029 = arith.andi %990, %932 : i1 loc(#loc237)
    acir.path %1029 {branch = 188 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1030 = arith.andi %990, %934 : i1 loc(#loc237)
    acir.path %1030 {branch = 189 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1031 = arith.andi %990, %936 : i1 loc(#loc237)
    acir.path %1031 {branch = 190 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1032 = arith.andi %990, %938 : i1 loc(#loc237)
    acir.path %1032 {branch = 191 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1033 = arith.andi %990, %940 : i1 loc(#loc237)
    acir.path %1033 {branch = 192 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1034 = arith.andi %990, %942 : i1 loc(#loc237)
    acir.path %1034 {branch = 193 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1035 = arith.andi %990, %944 : i1 loc(#loc237)
    acir.path %1035 {branch = 194 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1036 = arith.andi %990, %946 : i1 loc(#loc237)
    acir.path %1036 {branch = 195 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1037 = arith.andi %990, %948 : i1 loc(#loc237)
    acir.path %1037 {branch = 196 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1038 = arith.andi %990, %950 : i1 loc(#loc237)
    acir.path %1038 {branch = 197 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1039 = arith.andi %990, %952 : i1 loc(#loc237)
    acir.path %1039 {branch = 198 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1040 = arith.andi %990, %954 : i1 loc(#loc237)
    acir.path %1040 {branch = 199 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1041 = arith.andi %990, %956 : i1 loc(#loc237)
    acir.path %1041 {branch = 200 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1042 = arith.andi %990, %958 : i1 loc(#loc237)
    acir.path %1042 {branch = 201 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1043 = arith.andi %990, %960 : i1 loc(#loc237)
    acir.path %1043 {branch = 202 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1044 = arith.andi %990, %962 : i1 loc(#loc237)
    acir.path %1044 {branch = 203 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1045 = arith.andi %990, %964 : i1 loc(#loc237)
    acir.path %1045 {branch = 204 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1046 = arith.andi %990, %966 : i1 loc(#loc237)
    acir.path %1046 {branch = 205 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1047 = arith.andi %990, %968 : i1 loc(#loc237)
    acir.path %1047 {branch = 206 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1048 = arith.andi %990, %970 : i1 loc(#loc237)
    acir.path %1048 {branch = 207 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1049 = arith.andi %990, %972 : i1 loc(#loc237)
    acir.path %1049 {branch = 208 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1050 = arith.andi %990, %974 : i1 loc(#loc237)
    acir.path %1050 {branch = 209 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1051 = arith.andi %990, %976 : i1 loc(#loc237)
    acir.path %1051 {branch = 210 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1052 = arith.andi %990, %978 : i1 loc(#loc237)
    acir.path %1052 {branch = 211 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1053 = arith.andi %990, %980 : i1 loc(#loc237)
    acir.path %1053 {branch = 212 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1054 = arith.andi %990, %982 : i1 loc(#loc237)
    acir.path %1054 {branch = 213 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1055 = arith.andi %990, %984 : i1 loc(#loc237)
    acir.path %1055 {branch = 214 : i64, kind = "branch", txn = 4 : i64} loc(#loc237)
    %1056 = acir.aggregate(%true, %846, %991) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc227)
    %1057 = arith.select %844, %1056, %842 : !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc227)
    %1058 = arith.select %845, %1057, %842 : !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc227)
    %1059 = arith.select %845, %987, %841 : i32 loc(#loc226)
    %1060 = arith.xori %845, %true : i1
    %1061 = arith.ori %1060, %989 : i1
    %1062 = acir.extract %840 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc239)
    %1063 = acir.extract %840 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc240)
    %1064 = acir.extract %840 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc241)
    %1065 = acir.extract %840 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc242)
    %1066 = acir.extract %840 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc243)
    %1067 = acir.extract %840 {field = "stalled"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc244)
    %1068 = acir.aggregate(%1062, %1063, %1064, %1065, %1066, %1059, %1067) : (i1, i32, i32, i32, i1, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc245)
    acir.path %1061 {branch = 215 : i64, kind = "branch", txn = 4 : i64}
    %1069 = arith.xori %844, %true : i1 loc(#loc237)
    %1070 = arith.ori %1069, %857 : i1 loc(#loc237)
    %1071 = arith.select %843, %857, %1070 : i1 loc(#loc237)
    %1072 = acir.fire "top.cpu.memory.advance#4" %1071 {successors = [], txn = 4 : i64}
    %1073 = arith.andi %1072, %845 : i1
    acir.effect %1073(%853) {branch = 84 : i64, frames = ["top.cpu.memory:advance"], method = "check", port = 0 : i64, resource = "", txn = 4 : i64} : (i1, i1) -> () loc(#loc234)
    %1074 = arith.andi %1072, %844 : i1
    acir.effect %1074(%991, %856) {array = "top.cpu.memory.words", branch = 150 : i64, frames = ["top.cpu.memory:advance"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.memory.words.0", resources = ["top.cpu.memory.words.0", "top.cpu.memory.words.1", "top.cpu.memory.words.2", "top.cpu.memory.words.3", "top.cpu.memory.words.4", "top.cpu.memory.words.5", "top.cpu.memory.words.6", "top.cpu.memory.words.7", "top.cpu.memory.words.8", "top.cpu.memory.words.9", "top.cpu.memory.words.10", "top.cpu.memory.words.11", "top.cpu.memory.words.12", "top.cpu.memory.words.13", "top.cpu.memory.words.14", "top.cpu.memory.words.15", "top.cpu.memory.words.16", "top.cpu.memory.words.17", "top.cpu.memory.words.18", "top.cpu.memory.words.19", "top.cpu.memory.words.20", "top.cpu.memory.words.21", "top.cpu.memory.words.22", "top.cpu.memory.words.23", "top.cpu.memory.words.24", "top.cpu.memory.words.25", "top.cpu.memory.words.26", "top.cpu.memory.words.27", "top.cpu.memory.words.28", "top.cpu.memory.words.29", "top.cpu.memory.words.30", "top.cpu.memory.words.31", "top.cpu.memory.words.32", "top.cpu.memory.words.33", "top.cpu.memory.words.34", "top.cpu.memory.words.35", "top.cpu.memory.words.36", "top.cpu.memory.words.37", "top.cpu.memory.words.38", "top.cpu.memory.words.39", "top.cpu.memory.words.40", "top.cpu.memory.words.41", "top.cpu.memory.words.42", "top.cpu.memory.words.43", "top.cpu.memory.words.44", "top.cpu.memory.words.45", "top.cpu.memory.words.46", "top.cpu.memory.words.47", "top.cpu.memory.words.48", "top.cpu.memory.words.49", "top.cpu.memory.words.50", "top.cpu.memory.words.51", "top.cpu.memory.words.52", "top.cpu.memory.words.53", "top.cpu.memory.words.54", "top.cpu.memory.words.55", "top.cpu.memory.words.56", "top.cpu.memory.words.57", "top.cpu.memory.words.58", "top.cpu.memory.words.59", "top.cpu.memory.words.60", "top.cpu.memory.words.61", "top.cpu.memory.words.62", "top.cpu.memory.words.63"], txn = 4 : i64} : (i1, i32, i64) -> () loc(#loc237)
    acir.effect %1072(%1058) {branch = 83 : i64, frames = ["top.cpu.memory:advance"], method = "write", port = 0 : i64, resource = "top.cpu.memory.store_event", txn = 4 : i64} : (i1, !acir.struct<"src_isa_e31afbf3.StoreEvent">) -> () loc(#loc246)
    %1075 = arith.andi %1072, %989 : i1
    acir.effect %1075(%1068) {branch = 215 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.cpu.mem_wb", txn = 4 : i64} : (i1, !acir.struct<"src_isa_e31afbf3.Completed">) -> ()
    %1076 = acir.export %c84_i32 {acir.id = "v33", acir.source_type = "u32"} : i32 -> i32 loc(#loc247)
    acir.path %true {branch = 216 : i64, kind = "branch", txn = 5 : i64} loc(#loc248)
    acir.path %true {branch = 216 : i64, frames = ["top.cpu.writeback:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc248)
    %value_30, %ready_31 = acir.read(%true) {branch = 216 : i64, frames = ["top.cpu.writeback:advance"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = 5 : i64} : (i1) -> (!acir.struct<"src_isa_e31afbf3.Completed">, i1)
    %1077 = acir.bind %value_30 : (!acir.struct<"src_isa_e31afbf3.Completed">) -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc249)
    %1078 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %false, %c0_i32) : (i1, i32, i32, i32, i1, i32) -> !acir.struct<"src_isa_e31afbf3.Retirement"> loc(#loc250)
    %1079 = acir.extract %1077 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i1 loc(#loc251)
    %1080 = acir.extract %1077 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc252)
    %1081 = acir.extract %1077 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc253)
    acir.path %1079 {branch = 217 : i64, kind = "branch", txn = 5 : i64} loc(#loc254)
    acir.path %1079 {branch = 217 : i64, frames = ["top.cpu.writeback:advance", "top.cpu.rf:write"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc254)
    %1082 = arith.cmpi ne, %1080, %c0_i32 : i32 loc(#loc255)
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc207)
    %1083 = arith.andi %1079, %1082 : i1
    %1084 = arith.extui %1080 : i32 to i64 loc(#loc256)
    %1085 = arith.cmpi ult, %1084, %c32_i64 : i64 loc(#loc256)
    acir.path %1083 {branch = 218 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1086 = arith.cmpi eq, %1084, %c0_i64 : i64 loc(#loc256)
    %1087 = arith.andi %1083, %1086 : i1 loc(#loc256)
    acir.path %1087 {branch = 219 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1088 = arith.cmpi eq, %1084, %c1_i64 : i64 loc(#loc256)
    %1089 = arith.andi %1083, %1088 : i1 loc(#loc256)
    acir.path %1089 {branch = 220 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1090 = arith.cmpi eq, %1084, %c2_i64 : i64 loc(#loc256)
    %1091 = arith.andi %1083, %1090 : i1 loc(#loc256)
    acir.path %1091 {branch = 221 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1092 = arith.cmpi eq, %1084, %c3_i64 : i64 loc(#loc256)
    %1093 = arith.andi %1083, %1092 : i1 loc(#loc256)
    acir.path %1093 {branch = 222 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1094 = arith.cmpi eq, %1084, %c4_i64 : i64 loc(#loc256)
    %1095 = arith.andi %1083, %1094 : i1 loc(#loc256)
    acir.path %1095 {branch = 223 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1096 = arith.cmpi eq, %1084, %c5_i64 : i64 loc(#loc256)
    %1097 = arith.andi %1083, %1096 : i1 loc(#loc256)
    acir.path %1097 {branch = 224 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1098 = arith.cmpi eq, %1084, %c6_i64 : i64 loc(#loc256)
    %1099 = arith.andi %1083, %1098 : i1 loc(#loc256)
    acir.path %1099 {branch = 225 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1100 = arith.cmpi eq, %1084, %c7_i64 : i64 loc(#loc256)
    %1101 = arith.andi %1083, %1100 : i1 loc(#loc256)
    acir.path %1101 {branch = 226 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1102 = arith.cmpi eq, %1084, %c8_i64 : i64 loc(#loc256)
    %1103 = arith.andi %1083, %1102 : i1 loc(#loc256)
    acir.path %1103 {branch = 227 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1104 = arith.cmpi eq, %1084, %c9_i64 : i64 loc(#loc256)
    %1105 = arith.andi %1083, %1104 : i1 loc(#loc256)
    acir.path %1105 {branch = 228 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1106 = arith.cmpi eq, %1084, %c10_i64 : i64 loc(#loc256)
    %1107 = arith.andi %1083, %1106 : i1 loc(#loc256)
    acir.path %1107 {branch = 229 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1108 = arith.cmpi eq, %1084, %c11_i64 : i64 loc(#loc256)
    %1109 = arith.andi %1083, %1108 : i1 loc(#loc256)
    acir.path %1109 {branch = 230 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1110 = arith.cmpi eq, %1084, %c12_i64 : i64 loc(#loc256)
    %1111 = arith.andi %1083, %1110 : i1 loc(#loc256)
    acir.path %1111 {branch = 231 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1112 = arith.cmpi eq, %1084, %c13_i64 : i64 loc(#loc256)
    %1113 = arith.andi %1083, %1112 : i1 loc(#loc256)
    acir.path %1113 {branch = 232 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1114 = arith.cmpi eq, %1084, %c14_i64 : i64 loc(#loc256)
    %1115 = arith.andi %1083, %1114 : i1 loc(#loc256)
    acir.path %1115 {branch = 233 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1116 = arith.cmpi eq, %1084, %c15_i64 : i64 loc(#loc256)
    %1117 = arith.andi %1083, %1116 : i1 loc(#loc256)
    acir.path %1117 {branch = 234 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1118 = arith.cmpi eq, %1084, %c16_i64 : i64 loc(#loc256)
    %1119 = arith.andi %1083, %1118 : i1 loc(#loc256)
    acir.path %1119 {branch = 235 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1120 = arith.cmpi eq, %1084, %c17_i64 : i64 loc(#loc256)
    %1121 = arith.andi %1083, %1120 : i1 loc(#loc256)
    acir.path %1121 {branch = 236 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1122 = arith.cmpi eq, %1084, %c18_i64 : i64 loc(#loc256)
    %1123 = arith.andi %1083, %1122 : i1 loc(#loc256)
    acir.path %1123 {branch = 237 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1124 = arith.cmpi eq, %1084, %c19_i64 : i64 loc(#loc256)
    %1125 = arith.andi %1083, %1124 : i1 loc(#loc256)
    acir.path %1125 {branch = 238 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1126 = arith.cmpi eq, %1084, %c20_i64 : i64 loc(#loc256)
    %1127 = arith.andi %1083, %1126 : i1 loc(#loc256)
    acir.path %1127 {branch = 239 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1128 = arith.cmpi eq, %1084, %c21_i64 : i64 loc(#loc256)
    %1129 = arith.andi %1083, %1128 : i1 loc(#loc256)
    acir.path %1129 {branch = 240 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1130 = arith.cmpi eq, %1084, %c22_i64 : i64 loc(#loc256)
    %1131 = arith.andi %1083, %1130 : i1 loc(#loc256)
    acir.path %1131 {branch = 241 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1132 = arith.cmpi eq, %1084, %c23_i64 : i64 loc(#loc256)
    %1133 = arith.andi %1083, %1132 : i1 loc(#loc256)
    acir.path %1133 {branch = 242 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1134 = arith.cmpi eq, %1084, %c24_i64 : i64 loc(#loc256)
    %1135 = arith.andi %1083, %1134 : i1 loc(#loc256)
    acir.path %1135 {branch = 243 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1136 = arith.cmpi eq, %1084, %c25_i64 : i64 loc(#loc256)
    %1137 = arith.andi %1083, %1136 : i1 loc(#loc256)
    acir.path %1137 {branch = 244 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1138 = arith.cmpi eq, %1084, %c26_i64 : i64 loc(#loc256)
    %1139 = arith.andi %1083, %1138 : i1 loc(#loc256)
    acir.path %1139 {branch = 245 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1140 = arith.cmpi eq, %1084, %c27_i64 : i64 loc(#loc256)
    %1141 = arith.andi %1083, %1140 : i1 loc(#loc256)
    acir.path %1141 {branch = 246 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1142 = arith.cmpi eq, %1084, %c28_i64 : i64 loc(#loc256)
    %1143 = arith.andi %1083, %1142 : i1 loc(#loc256)
    acir.path %1143 {branch = 247 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1144 = arith.cmpi eq, %1084, %c29_i64 : i64 loc(#loc256)
    %1145 = arith.andi %1083, %1144 : i1 loc(#loc256)
    acir.path %1145 {branch = 248 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1146 = arith.cmpi eq, %1084, %c30_i64 : i64 loc(#loc256)
    %1147 = arith.andi %1083, %1146 : i1 loc(#loc256)
    acir.path %1147 {branch = 249 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1148 = arith.cmpi eq, %1084, %c31_i64 : i64 loc(#loc256)
    %1149 = arith.andi %1083, %1148 : i1 loc(#loc256)
    acir.path %1149 {branch = 250 : i64, kind = "branch", txn = 5 : i64} loc(#loc256)
    %1150 = acir.extract %1077 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i1 loc(#loc257)
    %1151 = acir.extract %1077 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc258)
    %1152 = arith.cmpi ult, %1151, %1076 : i32 loc(#loc258)
    %1153 = arith.andi %1150, %1152 : i1
    acir.path %1153 {branch = 251 : i64, kind = "branch", txn = 5 : i64} loc(#loc259)
    %value_32, %ready_33 = acir.read(%1153) {branch = 251 : i64, frames = ["top.cpu.writeback:advance"], method = "read", port = 0 : i64, resource = "top.cpu.writeback.retired", txn = 5 : i64} : (i1) -> (i32, i1) loc(#loc259)
    %1154 = arith.addi %value_32, %c1_i32 : i32 loc(#loc259)
    %1155 = acir.extract %1077 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc260)
    %1156 = acir.aggregate(%true, %1151, %1155, %1080, %1083, %1081) : (i1, i32, i32, i32, i1, i32) -> !acir.struct<"src_isa_e31afbf3.Retirement"> loc(#loc250)
    %1157 = arith.select %1153, %1156, %1078 : !acir.struct<"src_isa_e31afbf3.Retirement"> loc(#loc250)
    %1158 = arith.xori %1079, %true : i1 loc(#loc256)
    %1159 = arith.cmpi eq, %1080, %c0_i32 : i32 loc(#loc394)
    %1160 = arith.ori %1159, %1085 : i1 loc(#loc256)
    %1161 = arith.ori %1158, %1160 : i1 loc(#loc256)
    %1162 = acir.fire "top.cpu.writeback.advance#5" %1161 {successors = [], txn = 5 : i64}
    %1163 = arith.andi %1162, %1083 : i1
    acir.effect %1163(%1081, %1084) {array = "top.cpu.rf.registers", branch = 218 : i64, frames = ["top.cpu.writeback:advance", "top.cpu.rf:write"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = 5 : i64} : (i1, i32, i64) -> () loc(#loc256)
    %1164 = arith.andi %1162, %1153 : i1
    acir.effect %1164(%1154) {branch = 251 : i64, frames = ["top.cpu.writeback:advance"], method = "write", port = 0 : i64, resource = "top.cpu.writeback.retired", txn = 5 : i64} : (i1, i32) -> () loc(#loc261)
    acir.effect %1162(%1157) {branch = 216 : i64, frames = ["top.cpu.writeback:advance"], method = "write", port = 0 : i64, resource = "top.cpu.writeback.retirement", txn = 5 : i64} : (i1, !acir.struct<"src_isa_e31afbf3.Retirement">) -> () loc(#loc262)
    acir.path %true {branch = 252 : i64, kind = "branch", txn = 6 : i64} loc(#loc263)
    acir.path %true {branch = 252 : i64, frames = ["top.cpu:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc263)
    %1165 = acir.fire "top.cpu.observe#6" %281 {successors = [], txn = 6 : i64}
    acir.effect %1165(%282) {branch = 252 : i64, frames = ["top.cpu:observe"], method = "write", port = 0 : i64, resource = "top.cpu.last_control", txn = 6 : i64} : (i1, !acir.struct<"src_isa_e31afbf3.Control">) -> () loc(#loc264)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":66:16)
#loc2 = loc("acir.binding:top.cpu_body:instruction")
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":95:21)
#loc4 = loc("acir.binding:top.cpu_body.0.call.4.4:executing")
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":96:16)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":96:35)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":97:29)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":97:13)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":97:64)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":97:48)
#loc11 = loc("acir.binding:top.cpu_body:stall")
#loc12 = loc("acir.binding:top.cpu_body.0.call.5.9:packet")
#loc13 = loc("acir.binding:top.cpu_body.0.call.5.9:memory")
#loc14 = loc("acir.binding:top.cpu_body.0.call.5.9:writeback")
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":111:31)
#loc16 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.5.16:index")
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":210:8)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":211:12)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":211:30)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":213:12)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":213:33)
#loc22 = loc("acir.binding:top.cpu_body.0.call.5.9:source_a")
#loc23 = loc("acir.binding:top.cpu_body.0.call.5.9:source_b")
#loc24 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:original")
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":219:8)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":220:16)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":221:8)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":222:16)
#loc29 = loc("acir.binding:top.cpu_body.0.call.5.9:lhs")
#loc30 = loc("acir.binding:top.cpu_body.0.call.5.9:rhs")
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":115:16)
#loc32 = loc("acir.binding:top.cpu_body:operands")
#loc33 = loc("acir.binding:top.cpu_body.0.call.8.12:packet")
#loc34 = loc("acir.binding:top.cpu_body.0.call.8.12:executing")
#loc35 = loc("acir.binding:top.cpu_body.0.call.8.12:lhs")
#loc36 = loc("acir.binding:top.cpu_body.0.call.8.12:rhs")
#loc37 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:pc")
#loc38 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:opcode")
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":164:8)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":165:16)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":166:8)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":166:24)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":166:41)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":168:8)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":168:23)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":168:39)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":169:16)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":170:8)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":170:24)
#loc50 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:operand")
#loc51 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:function")
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":173:12)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":174:54)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":174:84)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":174:20)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":175:12)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":176:28)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":176:20)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":177:12)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":152:13)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":178:25)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":152:34)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":152:12)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":179:12)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":180:25)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":181:12)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":182:20)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":183:12)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":184:89)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":157:8)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":184:20)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":157:25)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":157:24)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":158:43)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":158:28)
#loc76 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172:result")
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":185:12)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":186:20)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":187:16)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":167:16)
#loc81 = loc("acir.binding:top.cpu_body.0.call.8.12:result")
#loc82 = loc("acir.binding:top.cpu_body.0.call.8.12:jump")
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":131:28)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":192:8)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":196:16)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":198:16)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":200:16)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":202:16)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":202:20)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":204:16)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":205:12)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":205:5)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":206:12)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":132:17)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":133:32)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":133:57)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":133:70)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":133:84)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":134:32)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":134:48)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":134:85)
#loc102 = loc("acir.binding:top.cpu_body.0.call.8.12:next_packet")
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":135:16)
#loc104 = loc("acir.binding:top.cpu_body:result")
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":222:34)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":222:51)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":222:66)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":222:85)
#loc109 = loc("acir.binding:top.cpu_body:control")
#loc110 = loc("acir.binding:top.cpu.fetch_body.0.call.13.2:control")
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":52:16)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":41:12)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":43:12)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":44:13)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":44:18)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":45:20)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":46:16)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":47:36)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":47:40)
#loc120 = loc("acir.binding:top.cpu.fetch_body.0.call.13.2:packet")
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":48:14)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":46:9)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":48:9)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":84:9)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":62:30)
#loc126 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:word")
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":62:23)
#loc128 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:instruction")
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":110:8)
#loc130 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:opcode")
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":112:9)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":113:30)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":113:9)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":114:30)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":114:9)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":115:26)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":115:9)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":116:27)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":116:9)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":117:12)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":117:28)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":117:44)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":118:31)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":118:13)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:46)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:62)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:77)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:93)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:109)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:126)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:9)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":120:9)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":121:9)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":125:14)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":129:14)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":121:29)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":137:13)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":105:13)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":135:37)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":105:12)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":135:13)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":133:50)
#loc163 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298:value")
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":133:37)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:26)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:48)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:47)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:25)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:74)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:73)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:101)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:100)
#loc173 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:immediate")
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":131:37)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":126:13)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:26)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:48)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:47)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:25)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:77)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:76)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:104)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:103)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":128:37)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":128:13)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":124:37)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":138:12)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":138:32)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":139:20)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":139:13)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:20)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:48)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:78)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:105)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:13)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":143:13)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":144:29)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":145:13)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":147:13)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":62:9)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":85:16)
#loc202 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:packet")
#loc203 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:completed")
#loc204 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:decoded")
#loc205 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6.0.call.17.49:index")
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":21:12)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":15:61)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":23:12)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":23:33)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":25:16)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":24:20)
#loc212 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:lhs")
#loc213 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:rhs")
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":80:24)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":80:38)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":80:49)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":80:16)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":72:22)
#loc219 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:bubble")
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":73:13)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":227:26)
#loc222 = loc("acir.binding:top.cpu.execute_body.0.call.19.2:packet")
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":142:16)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":170:16)
#loc225 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:packet")
#loc226 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:value")
#loc227 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:event")
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":158:12)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":158:27)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":148:57)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":159:20)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":159:44)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":159:67)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":159:13)
#loc235 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:index")
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":162:25)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":164:17)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":164:32)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:26)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:40)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:51)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:64)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:75)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:97)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:16)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":166:9)
#loc247 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3:code_size")
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":191:9)
#loc249 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3:packet")
#loc250 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3:event")
#loc251 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":183:12)
#loc252 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3.0.call.21.18:index")
#loc253 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3.0.call.21.18:value")
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":184:13)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":29:12)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":30:13)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":185:12)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":185:29)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":186:23)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":187:49)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":186:13)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":188:9)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":230:9)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":214:9)
#loc265 = loc(callsite(#loc16 at #loc))
#loc266 = loc(callsite(#loc at #loc))
#loc267 = loc(callsite(#loc17 at #loc))
#loc268 = loc(callsite(#loc18 at #loc))
#loc269 = loc(callsite(#loc19 at #loc))
#loc270 = loc(callsite(#loc20 at #loc))
#loc271 = loc(callsite(#loc21 at #loc))
#loc272 = loc(callsite(#loc24 at #loc))
#loc273 = loc(callsite(#loc25 at #loc))
#loc274 = loc(callsite(#loc26 at #loc))
#loc275 = loc(callsite(#loc27 at #loc))
#loc276 = loc(callsite(#loc28 at #loc))
#loc277 = loc(callsite(#loc37 at #loc))
#loc278 = loc(callsite(#loc38 at #loc))
#loc279 = loc(callsite(#loc39 at #loc))
#loc280 = loc(callsite(#loc40 at #loc))
#loc281 = loc(callsite(#loc41 at #loc))
#loc282 = loc(callsite(#loc42 at #loc))
#loc283 = loc(callsite(#loc43 at #loc))
#loc284 = loc(callsite(#loc44 at #loc))
#loc285 = loc(callsite(#loc45 at #loc))
#loc286 = loc(callsite(#loc46 at #loc))
#loc287 = loc(callsite(#loc47 at #loc))
#loc288 = loc(callsite(#loc48 at #loc))
#loc289 = loc(callsite(#loc49 at #loc))
#loc290 = loc(callsite(#loc50 at #loc))
#loc291 = loc(callsite(#loc51 at #loc))
#loc292 = loc(callsite(#loc52 at #loc))
#loc293 = loc(callsite(#loc53 at #loc))
#loc294 = loc(callsite(#loc54 at #loc))
#loc295 = loc(callsite(#loc55 at #loc))
#loc296 = loc(callsite(#loc56 at #loc))
#loc297 = loc(callsite(#loc57 at #loc))
#loc298 = loc(callsite(#loc58 at #loc))
#loc299 = loc(callsite(#loc59 at #loc))
#loc300 = loc(callsite(#loc61 at #loc))
#loc301 = loc(callsite(#loc64 at #loc))
#loc302 = loc(callsite(#loc65 at #loc))
#loc303 = loc(callsite(#loc66 at #loc))
#loc304 = loc(callsite(#loc67 at #loc))
#loc305 = loc(callsite(#loc68 at #loc))
#loc306 = loc(callsite(#loc69 at #loc))
#loc307 = loc(callsite(#loc71 at #loc))
#loc308 = loc(callsite(#loc77 at #loc))
#loc309 = loc(callsite(#loc78 at #loc))
#loc310 = loc(callsite(#loc79 at #loc))
#loc311 = loc(callsite(#loc80 at #loc))
#loc312 = loc(callsite(#loc at #loc83))
#loc313 = loc(callsite(#loc84 at #loc83))
#loc314 = loc(callsite(#loc85 at #loc83))
#loc315 = loc(callsite(#loc86 at #loc83))
#loc316 = loc(callsite(#loc87 at #loc83))
#loc317 = loc(callsite(#loc88 at #loc83))
#loc318 = loc(callsite(#loc89 at #loc83))
#loc319 = loc(callsite(#loc90 at #loc83))
#loc320 = loc(callsite(#loc91 at #loc83))
#loc321 = loc(callsite(#loc92 at #loc83))
#loc322 = loc(callsite(#loc93 at #loc83))
#loc323 = loc(callsite(#loc126 at #loc127))
#loc324 = loc(callsite(#loc128 at #loc127))
#loc325 = loc(callsite(#loc129 at #loc127))
#loc326 = loc(callsite(#loc130 at #loc127))
#loc327 = loc(callsite(#loc131 at #loc127))
#loc328 = loc(callsite(#loc132 at #loc127))
#loc329 = loc(callsite(#loc133 at #loc127))
#loc330 = loc(callsite(#loc134 at #loc127))
#loc331 = loc(callsite(#loc135 at #loc127))
#loc332 = loc(callsite(#loc136 at #loc127))
#loc333 = loc(callsite(#loc137 at #loc127))
#loc334 = loc(callsite(#loc138 at #loc127))
#loc335 = loc(callsite(#loc139 at #loc127))
#loc336 = loc(callsite(#loc140 at #loc127))
#loc337 = loc(callsite(#loc141 at #loc127))
#loc338 = loc(callsite(#loc at #loc127))
#loc339 = loc(callsite(#loc142 at #loc127))
#loc340 = loc(callsite(#loc143 at #loc127))
#loc341 = loc(callsite(#loc144 at #loc127))
#loc342 = loc(callsite(#loc145 at #loc127))
#loc343 = loc(callsite(#loc146 at #loc127))
#loc344 = loc(callsite(#loc147 at #loc127))
#loc345 = loc(callsite(#loc148 at #loc127))
#loc346 = loc(callsite(#loc149 at #loc127))
#loc347 = loc(callsite(#loc150 at #loc127))
#loc348 = loc(callsite(#loc151 at #loc127))
#loc349 = loc(callsite(#loc152 at #loc127))
#loc350 = loc(callsite(#loc153 at #loc127))
#loc351 = loc(callsite(#loc154 at #loc127))
#loc352 = loc(callsite(#loc155 at #loc127))
#loc353 = loc(callsite(#loc156 at #loc127))
#loc354 = loc(callsite(#loc157 at #loc127))
#loc355 = loc(callsite(#loc159 at #loc127))
#loc356 = loc(callsite(#loc161 at #loc127))
#loc357 = loc(callsite(#loc162 at #loc127))
#loc358 = loc(callsite(#loc164 at #loc127))
#loc359 = loc(callsite(#loc165 at #loc127))
#loc360 = loc(callsite(#loc166 at #loc127))
#loc361 = loc(callsite(#loc167 at #loc127))
#loc362 = loc(callsite(#loc168 at #loc127))
#loc363 = loc(callsite(#loc169 at #loc127))
#loc364 = loc(callsite(#loc170 at #loc127))
#loc365 = loc(callsite(#loc171 at #loc127))
#loc366 = loc(callsite(#loc172 at #loc127))
#loc367 = loc(callsite(#loc173 at #loc127))
#loc368 = loc(callsite(#loc174 at #loc127))
#loc369 = loc(callsite(#loc175 at #loc127))
#loc370 = loc(callsite(#loc176 at #loc127))
#loc371 = loc(callsite(#loc177 at #loc127))
#loc372 = loc(callsite(#loc178 at #loc127))
#loc373 = loc(callsite(#loc179 at #loc127))
#loc374 = loc(callsite(#loc180 at #loc127))
#loc375 = loc(callsite(#loc181 at #loc127))
#loc376 = loc(callsite(#loc182 at #loc127))
#loc377 = loc(callsite(#loc183 at #loc127))
#loc378 = loc(callsite(#loc184 at #loc127))
#loc379 = loc(callsite(#loc185 at #loc127))
#loc380 = loc(callsite(#loc186 at #loc127))
#loc381 = loc(callsite(#loc187 at #loc127))
#loc382 = loc(callsite(#loc188 at #loc127))
#loc383 = loc(callsite(#loc189 at #loc127))
#loc384 = loc(callsite(#loc190 at #loc127))
#loc385 = loc(callsite(#loc191 at #loc127))
#loc386 = loc(callsite(#loc192 at #loc127))
#loc387 = loc(callsite(#loc193 at #loc127))
#loc388 = loc(callsite(#loc194 at #loc127))
#loc389 = loc(callsite(#loc195 at #loc127))
#loc390 = loc(callsite(#loc196 at #loc127))
#loc391 = loc(callsite(#loc197 at #loc127))
#loc392 = loc(callsite(#loc198 at #loc127))
#loc393 = loc(callsite(#loc199 at #loc127))
#loc394 = loc(fused[#loc256, #loc255])
#loc395 = loc(fused[#loc266, #loc267])
#loc396 = loc(callsite(#loc60 at #loc300))
#loc397 = loc(callsite(#loc62 at #loc300))
#loc398 = loc(callsite(#loc63 at #loc300))
#loc399 = loc(callsite(#loc70 at #loc307))
#loc400 = loc(callsite(#loc72 at #loc307))
#loc401 = loc(callsite(#loc73 at #loc307))
#loc402 = loc(callsite(#loc at #loc307))
#loc403 = loc(callsite(#loc74 at #loc307))
#loc404 = loc(callsite(#loc75 at #loc307))
#loc405 = loc(callsite(#loc76 at #loc307))
#loc406 = loc(callsite(#loc62 at #loc316))
#loc407 = loc(callsite(#loc63 at #loc316))
#loc408 = loc(callsite(#loc63 at #loc318))
#loc409 = loc(fused[#loc338, #loc351])
#loc410 = loc(fused[#loc338, #loc352])
#loc411 = loc(fused[#loc338, #loc353])
#loc412 = loc(callsite(#loc158 at #loc355))
#loc413 = loc(callsite(#loc160 at #loc355))
#loc414 = loc(callsite(#loc163 at #loc358))
#loc415 = loc(callsite(#loc158 at #loc358))
#loc416 = loc(callsite(#loc160 at #loc358))
#loc417 = loc(callsite(#loc158 at #loc368))
#loc418 = loc(callsite(#loc160 at #loc368))
#loc419 = loc(callsite(#loc158 at #loc378))
#loc420 = loc(callsite(#loc160 at #loc378))
#loc421 = loc(fused[#loc402, #loc399])
#loc422 = loc(fused[#loc317, #loc408])
