#loc = loc(unknown)
module attributes {acir.instances = [{definition = "src_demo_1feb5b94.Demo", name = "top", parent, rules = []}, {definition = "src_model_d0912efa.Ripes5", name = "top.cpu", parent = "top", rules = ["environment", "observe"]}, {definition = "src_model_d0912efa.RegisterFile", name = "top.cpu.rf", parent = "top.cpu", rules = ["read", "write"]}, {definition = "src_model_d0912efa.Fetch", name = "top.cpu.fetch", parent = "top.cpu", rules = ["advance"]}, {definition = "src_model_d0912efa.Decode", name = "top.cpu.decode_stage", parent = "top.cpu", rules = ["advance", "decode_instruction", "inspect"]}, {definition = "src_model_d0912efa.HazardUnit", name = "top.cpu.hazard_unit", parent = "top.cpu", rules = ["ecall_wait", "load_use"]}, {definition = "src_model_d0912efa.ForwardingUnit", name = "top.cpu.forwarding_unit", parent = "top.cpu", rules = ["resolve"]}, {definition = "src_model_d0912efa.Execute", name = "top.cpu.execute", parent = "top.cpu", rules = ["advance", "resolve"]}, {definition = "src_model_d0912efa.Memory", name = "top.cpu.memory", parent = "top.cpu", rules = ["advance", "read", "read_word"]}, {definition = "src_model_d0912efa.Writeback", name = "top.cpu.writeback", parent = "top.cpu", rules = ["advance"]}], acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "Demo", type_order = ["src_isa_ae2f66cd.Instruction", "src_isa_ae2f66cd.Fetched", "src_isa_ae2f66cd.Decoded", "src_isa_ae2f66cd.Executed", "src_isa_ae2f66cd.Completed", "src_isa_ae2f66cd.Control", "src_isa_ae2f66cd.ForwardedOperands", "src_isa_ae2f66cd.ExecutionResult", "src_isa_ae2f66cd.Retirement", "src_isa_ae2f66cd.StoreEvent", "src_isa_ae2f66cd.SystemEvent", "src_isa_ae2f66cd.Expansion"], types = {src_isa_ae2f66cd.Completed = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}, {default = false, name = "illegal", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "system", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "expanded", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "next_pc", payload_type = i32, type = "u32"}], src_isa_ae2f66cd.Control = [{default = false, name = "stall", payload_type = i1, type = "bool"}, {default = false, name = "redirect", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "target", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "forward_a", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "forward_b", payload_type = i32, type = "u32"}, {default = false, name = "ecall_stall", payload_type = i1, type = "bool"}, {default = false, name = "stop", payload_type = i1, type = "bool"}], src_isa_ae2f66cd.Decoded = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default, name = "instruction", payload_type = !acir.struct<"src_isa_ae2f66cd.Instruction">, type = "src_isa_ae2f66cd.Instruction"}, {default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "next_pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "expanded", payload_type = i32, type = "u32"}, {default = false, name = "fetch_fault", payload_type = i1, type = "bool"}], src_isa_ae2f66cd.Executed = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = false, name = "load", payload_type = i1, type = "bool"}, {default = false, name = "store", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "alu", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "funct3", payload_type = i32, type = "u32"}, {default = false, name = "illegal", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "system", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "expanded", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "next_pc", payload_type = i32, type = "u32"}], src_isa_ae2f66cd.ExecutionResult = [{default, name = "packet", payload_type = !acir.struct<"src_isa_ae2f66cd.Executed">, type = "src_isa_ae2f66cd.Executed"}, {default = false, name = "redirect", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "target", payload_type = i32, type = "u32"}], src_isa_ae2f66cd.Expansion = [{default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 4 : i64, name = "length", payload_type = i32, type = "u32"}], src_isa_ae2f66cd.Fetched = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "expanded", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "next_pc", payload_type = i32, type = "u32"}, {default = false, name = "fetch_fault", payload_type = i1, type = "bool"}], src_isa_ae2f66cd.ForwardedOperands = [{default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "source_a", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "source_b", payload_type = i32, type = "u32"}], src_isa_ae2f66cd.Instruction = [{default = 0 : i64, name = "opcode", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "funct3", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "funct7", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "immediate", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = false, name = "load", payload_type = i1, type = "bool"}, {default = false, name = "store", payload_type = i1, type = "bool"}, {default = false, name = "illegal", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "system", payload_type = i32, type = "u32"}], src_isa_ae2f66cd.Retirement = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_isa_ae2f66cd.StoreEvent = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "address", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "size", payload_type = i32, type = "u32"}], src_isa_ae2f66cd.SystemEvent = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "number", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "argument", payload_type = i32, type = "u32"}]}}, acir.stage = "transaction"} {
  %c-4092_i32 = arith.constant -4092 : i32
  %c1048576_i32 = arith.constant 1048576 : i32
  %c2048_i32 = arith.constant 2048 : i32
  %c512_i32 = arith.constant 512 : i32
  %c20480_i32 = arith.constant 20480 : i32
  %c28672_i32 = arith.constant 28672 : i32
  %c256_i32 = arith.constant 256 : i32
  %c128_i32 = arith.constant 128 : i32
  %c65536_i32 = arith.constant 65536 : i32
  %c84_i32 = arith.constant 84 : i32
  %c-4_i32 = arith.constant -4 : i32
  %c-4096_i32 = arith.constant -4096 : i32
  %c255_i32 = arith.constant 255 : i32
  %c115_i32 = arith.constant 115 : i32
  %c1023_i32 = arith.constant 1023 : i32
  %c21_i32 = arith.constant 21 : i32
  %c1044480_i32 = arith.constant 1044480 : i32
  %c63_i32 = arith.constant 63 : i32
  %c4095_i32 = arith.constant 4095 : i32
  %c127_i32 = arith.constant 127 : i32
  %c25_i32 = arith.constant 25 : i32
  %c20_i32 = arith.constant 20 : i32
  %c8192_i32 = arith.constant 8192 : i32
  %c13_i32 = arith.constant 13 : i32
  %c12_i32 = arith.constant 12 : i32
  %c15_i32 = arith.constant 15 : i32
  %c9_i32 = arith.constant 9 : i32
  %c1024_i32 = arith.constant 1024 : i32
  %c1048691_i32 = arith.constant 1048691 : i32
  %c1024_i64 = arith.constant 1024 : i64
  %c4096_i32 = arith.constant 4096 : i32
  %c8_i32 = arith.constant 8 : i32
  %c16_i32 = arith.constant 16 : i32
  %c65535_i32 = arith.constant 65535 : i32
  %c11_i32 = arith.constant 11 : i32
  %c93_i32 = arith.constant 93 : i32
  %c10_i32 = arith.constant 10 : i32
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
  %c0_i64 = arith.constant 0 : i64
  %c1_i64 = arith.constant 1 : i64
  %c17_i32 = arith.constant 17 : i32
  %c32_i64 = arith.constant 32 : i64
  %c-2147483648_i32 = arith.constant -2147483648 : i32
  %c-1_i32 = arith.constant -1 : i32
  %c7_i32 = arith.constant 7 : i32
  %c55_i32 = arith.constant 55 : i32
  %c23_i32 = arith.constant 23 : i32
  %c99_i32 = arith.constant 99 : i32
  %c-2_i32 = arith.constant -2 : i32
  %c3_i32 = arith.constant 3 : i32
  %c35_i32 = arith.constant 35 : i32
  %c51_i32 = arith.constant 51 : i32
  %c19_i32 = arith.constant 19 : i32
  %c32_i32 = arith.constant 32 : i32
  %c31_i32 = arith.constant 31 : i32
  %c4_i32 = arith.constant 4 : i32
  %c5_i32 = arith.constant 5 : i32
  %c6_i32 = arith.constant 6 : i32
  %c111_i32 = arith.constant 111 : i32
  %c103_i32 = arith.constant 103 : i32
  %c1_i32 = arith.constant 1 : i32
  %c2_i32 = arith.constant 2 : i32
  %c0_i32 = arith.constant 0 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 122 : i64, target_branches = [0], to = 0 : i64}, {from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 122 : i64, target_branches = [0], to = 2 : i64}, {from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 122 : i64, target_branches = [0], to = 3 : i64}, {from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 122 : i64, target_branches = [0], to = 6 : i64}, {from = 1 : i64, relation = "Signal.set \E2\86\92 get", resource = "top.cpu.decode_stage.instruction", source_branch = 122 : i64, target_branches = [124], to = 2 : i64}], schedule = [1, 0, 2, 3, 4, 5, 6, 7], stage = "transaction"} {
    acir.state @top.cpu.decode_stage.instruction : !acir.struct<"src_isa_ae2f66cd.Instruction"> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, false, false, false, false, 0 : i32], kind = "signal"}
    acir.state @top.cpu.ex_mem : !acir.struct<"src_isa_ae2f66cd.Executed"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, false, false, false, 0 : i32, 0 : i32, 0 : i32, false, 0 : i32, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.exit_status : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.fetch.pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.fetch.rom : !acir.array<i32, [1024]> {initial = [5431 : i32, 17825939 : i32, 1384483 : i32, 336131 : i32, 1115747 : i32, 335907 : i32, 103811603 : i32, 1114515 : i32, 2201187 : i32, 335907 : i32, 103811731 : i32, 336387 : i32, 1185379 : i32, 1180307 : i32, 8388719 : i32, 335907 : i32, 5579299 : i32, 2146435091 : i32, 111 : i32, 19 : i32, 19 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.halted : i1 {initial = false, kind = "cell"}
    acir.state @top.cpu.id_ex : !acir.struct<"src_isa_ae2f66cd.Decoded"> {initial = [false, 0 : i32, 0 : i32, [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, false, false, false, false, 0 : i32], 0 : i32, 0 : i32, false, 0 : i32, 0 : i32, false], kind = "cell"}
    acir.state @top.cpu.if_id : !acir.struct<"src_isa_ae2f66cd.Fetched"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, 0 : i32, false], kind = "cell"}
    acir.state @top.cpu.last_control : !acir.struct<"src_isa_ae2f66cd.Control"> {initial = [false, false, 0 : i32, 0 : i32, 0 : i32, false, false], kind = "cell"}
    acir.state @top.cpu.mem_wb : !acir.struct<"src_isa_ae2f66cd.Completed"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, false, 0 : i32, false, false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.memory.load_event : !acir.struct<"src_isa_ae2f66cd.StoreEvent"> {initial = [false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.memory.store_event : !acir.struct<"src_isa_ae2f66cd.StoreEvent"> {initial = [false, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.memory.text : !acir.array<i32, [1024]> {initial = [5431 : i32, 17825939 : i32, 1384483 : i32, 336131 : i32, 1115747 : i32, 335907 : i32, 103811603 : i32, 1114515 : i32, 2201187 : i32, 335907 : i32, 103811731 : i32, 336387 : i32, 1185379 : i32, 1180307 : i32, 8388719 : i32, 335907 : i32, 5579299 : i32, 2146435091 : i32, 111 : i32, 19 : i32, 19 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.memory.words : !acir.array<i32, [1024]> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
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
    acir.state @top.cpu.system_event : !acir.struct<"src_isa_ae2f66cd.SystemEvent"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.cpu.writeback.retired : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.cpu.writeback.retirement : !acir.struct<"src_isa_ae2f66cd.Retirement"> {initial = [false, 0 : i32, 0 : i32, 0 : i32, false, 0 : i32], kind = "cell"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64}
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.decode_stage:inspect"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.decode_stage:inspect"], method = "get", port = 0 : i64, resource = "top.cpu.decode_stage.instruction", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Instruction">, i1) loc(#loc1)
    %0 = acir.export %value : !acir.struct<"src_isa_ae2f66cd.Instruction"> -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc2)
    %1 = acir.export %ready : i1 -> i1
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.hazard_unit:load_use"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value_0, %ready_1 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.hazard_unit:load_use"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1) loc(#loc3)
    %2 = acir.extract %value_0 {field = "instruction"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc4)
    %3 = acir.extract %value_0 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i1 loc(#loc5)
    %4 = acir.extract %2 {field = "load"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i1 loc(#loc6)
    %5 = arith.andi %3, %4 : i1
    %6 = acir.extract %2 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc7)
    %7 = arith.cmpi ne, %6, %c0_i32 : i32 loc(#loc7)
    %8 = arith.andi %5, %7 : i1
    %9 = acir.extract %value {field = "rs1"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc8)
    %10 = arith.cmpi eq, %6, %9 : i32 loc(#loc9)
    %11 = acir.extract %value {field = "rs2"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc10)
    %12 = arith.cmpi eq, %6, %11 : i32 loc(#loc11)
    %13 = arith.cmpi ne, %6, %9 : i32 loc(#loc9)
    %14 = arith.ori %10, %12 : i1
    %15 = arith.ori %10, %13 : i1
    %16 = arith.ori %10, %1 : i1
    %17 = arith.andi %1, %16 : i1
    %18 = arith.andi %15, %1 : i1
    %19 = arith.xori %8, %true : i1
    %20 = arith.ori %19, %1 : i1
    %21 = arith.andi %8, %14 : i1
    %22 = arith.andi %8, %18 : i1
    %23 = arith.ori %22, %19 : i1
    %24 = arith.ori %19, %17 : i1
    %25 = arith.andi %24, %20 : i1
    %26 = arith.andi %23, %20 : i1
    %27 = acir.export %21 : i1 -> i1 loc(#loc12)
    %28 = acir.export %25 : i1 -> i1
    %29 = arith.andi %28, %26 : i1
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.forwarding_unit:resolve"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value_2, %ready_3 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.forwarding_unit:resolve"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1)
    %value_4, %ready_5 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.forwarding_unit:resolve"], method = "read", port = 0 : i64, resource = "top.cpu.ex_mem", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Executed">, i1)
    %value_6, %ready_7 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.forwarding_unit:resolve"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1)
    %30 = acir.extract %value_2 {field = "instruction"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc13)
    %31 = acir.extract %30 {field = "rs1"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc561)
    %32 = acir.aggregate() : () -> i32 loc(#loc562)
    %33 = arith.cmpi ne, %31, %c0_i32 : i32 loc(#loc563)
    %34 = acir.extract %value_4 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc564)
    %35 = acir.extract %value_4 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc565)
    %36 = arith.andi %34, %35 : i1 loc(#loc562)
    %37 = acir.extract %value_4 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc566)
    %38 = arith.cmpi eq, %37, %31 : i32 loc(#loc566)
    %39 = arith.andi %36, %38 : i1 loc(#loc562)
    %40 = arith.xori %39, %true : i1 loc(#loc562)
    %41 = arith.select %39, %c1_i32, %32 : i32 loc(#loc562)
    %42 = acir.extract %value_6 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc567)
    %43 = acir.extract %value_6 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc568)
    %44 = arith.andi %42, %43 : i1 loc(#loc562)
    %45 = acir.extract %value_6 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc569)
    %46 = arith.cmpi eq, %45, %31 : i32 loc(#loc569)
    %47 = arith.andi %44, %46 : i1 loc(#loc562)
    %48 = arith.select %47, %false, %40 : i1 loc(#loc562)
    %49 = arith.select %47, %c2_i32, %41 : i32 loc(#loc562)
    %50 = arith.select %39, %40, %48 : i1 loc(#loc562)
    %51 = arith.select %39, %c1_i32, %49 : i32 loc(#loc562)
    %52 = arith.cmpi eq, %31, %c0_i32 : i32 loc(#loc797)
    %53 = arith.ori %52, %50 : i1 loc(#loc562)
    %54 = arith.select %33, %51, %32 : i32 loc(#loc562)
    %55 = arith.select %53, %c0_i32, %54 : i32 loc(#loc22)
    %56 = acir.extract %30 {field = "rs2"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc561)
    %57 = arith.cmpi ne, %56, %c0_i32 : i32 loc(#loc563)
    %58 = arith.cmpi eq, %37, %56 : i32 loc(#loc566)
    %59 = arith.andi %36, %58 : i1 loc(#loc562)
    %60 = arith.xori %59, %true : i1 loc(#loc562)
    %61 = arith.select %59, %c1_i32, %32 : i32 loc(#loc562)
    %62 = arith.cmpi eq, %45, %56 : i32 loc(#loc569)
    %63 = arith.andi %44, %62 : i1 loc(#loc562)
    %64 = arith.select %63, %false, %60 : i1 loc(#loc562)
    %65 = arith.select %63, %c2_i32, %61 : i32 loc(#loc562)
    %66 = arith.select %59, %60, %64 : i1 loc(#loc562)
    %67 = arith.select %59, %c1_i32, %65 : i32 loc(#loc562)
    %68 = arith.cmpi eq, %56, %c0_i32 : i32 loc(#loc797)
    %69 = arith.ori %68, %66 : i1 loc(#loc562)
    %70 = arith.select %57, %67, %32 : i32 loc(#loc562)
    %71 = arith.select %69, %c0_i32, %70 : i32 loc(#loc23)
    %72 = acir.extract %value_2 {field = "lhs"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i32 loc(#loc570)
    %73 = arith.cmpi eq, %55, %c1_i32 : i32 loc(#loc571)
    %74 = arith.cmpi ne, %55, %c1_i32 : i32 loc(#loc571)
    %75 = acir.extract %value_4 {field = "alu"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc572)
    %76 = arith.select %73, %75, %32 : i32 loc(#loc562)
    %77 = arith.ori %73, %74 : i1 loc(#loc562)
    %78 = arith.cmpi eq, %55, %c2_i32 : i32 loc(#loc573)
    %79 = arith.cmpi ne, %55, %c2_i32 : i32 loc(#loc573)
    %80 = acir.extract %value_6 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc574)
    %81 = arith.select %78, %80, %76 : i32 loc(#loc562)
    %82 = arith.select %79, %72, %81 : i32 loc(#loc562)
    %83 = arith.select %74, %82, %76 : i32 loc(#loc29)
    %84 = arith.andi %73, %77 : i1 loc(#loc562)
    %85 = arith.ori %74, %84 : i1 loc(#loc562)
    %86 = acir.extract %value_2 {field = "rhs"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i32 loc(#loc570)
    %87 = arith.cmpi eq, %71, %c1_i32 : i32 loc(#loc571)
    %88 = arith.cmpi ne, %71, %c1_i32 : i32 loc(#loc571)
    %89 = arith.select %87, %75, %32 : i32 loc(#loc562)
    %90 = arith.ori %87, %88 : i1 loc(#loc562)
    %91 = arith.cmpi eq, %71, %c2_i32 : i32 loc(#loc573)
    %92 = arith.cmpi ne, %71, %c2_i32 : i32 loc(#loc573)
    %93 = arith.select %91, %80, %89 : i32 loc(#loc562)
    %94 = arith.select %92, %86, %93 : i32 loc(#loc562)
    %95 = arith.select %88, %94, %89 : i32 loc(#loc30)
    %96 = arith.andi %87, %90 : i1 loc(#loc562)
    %97 = arith.ori %88, %96 : i1 loc(#loc562)
    %98 = arith.andi %85, %97 : i1 loc(#loc31)
    %99 = acir.aggregate(%83, %95, %55, %71) : (i32, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.ForwardedOperands"> loc(#loc31)
    %100 = acir.export %99 : !acir.struct<"src_isa_ae2f66cd.ForwardedOperands"> -> !acir.struct<"src_isa_ae2f66cd.ForwardedOperands"> loc(#loc32)
    %101 = acir.export %98 : i1 -> i1
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.execute:resolve"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value_8, %ready_9 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.execute:resolve"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1)
    %102 = acir.extract %value_8 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i1 loc(#loc33)
    %103 = acir.extract %value_8 {field = "fetch_fault"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i1 loc(#loc34)
    %104 = arith.andi %102, %103 : i1
    %105 = acir.assert %false loc(#loc35)
    %106 = arith.xori %104, %true : i1
    %107 = arith.ori %106, %105 : i1
    %108 = acir.extract %value_8 {field = "instruction"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc36)
    %109 = acir.extract %value_8 {field = "pc"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i32 loc(#loc575)
    %110 = acir.extract %108 {field = "opcode"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc576)
    %111 = arith.cmpi eq, %110, %c55_i32 : i32 loc(#loc577)
    %112 = arith.cmpi ne, %110, %c55_i32 : i32 loc(#loc577)
    %113 = acir.extract %108 {field = "immediate"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc578)
    %114 = arith.select %111, %113, %32 : i32 loc(#loc562)
    %115 = arith.ori %111, %112 : i1 loc(#loc562)
    %116 = arith.cmpi eq, %110, %c23_i32 : i32 loc(#loc579)
    %117 = arith.cmpi eq, %110, %c111_i32 : i32 loc(#loc580)
    %118 = arith.ori %116, %117 : i1 loc(#loc562)
    %119 = arith.cmpi eq, %110, %c99_i32 : i32 loc(#loc581)
    %120 = arith.ori %118, %119 : i1 loc(#loc562)
    %121 = arith.xori %120, %true : i1 loc(#loc562)
    %122 = arith.cmpi eq, %110, %c103_i32 : i32 loc(#loc582)
    %123 = arith.select %122, %false, %121 : i1 loc(#loc562)
    %124 = arith.addi %83, %113 : i32 loc(#loc583)
    %125 = arith.andi %124, %c-2_i32 : i32 loc(#loc584)
    %126 = arith.cmpi ne, %110, %c103_i32 : i32 loc(#loc798)
    %127 = arith.select %122, %125, %114 : i32 loc(#loc562)
    %128 = arith.andi %126, %115 : i1 loc(#loc562)
    %129 = arith.ori %122, %128 : i1 loc(#loc562)
    %130 = arith.ori %126, %98 : i1 loc(#loc562)
    %131 = arith.cmpi eq, %110, %c3_i32 : i32 loc(#loc585)
    %132 = arith.cmpi eq, %110, %c35_i32 : i32 loc(#loc586)
    %133 = arith.ori %131, %132 : i1 loc(#loc562)
    %134 = arith.xori %133, %true : i1 loc(#loc562)
    %135 = arith.select %133, %134, %false : i1 loc(#loc562)
    %136 = arith.cmpi eq, %110, %c51_i32 : i32 loc(#loc587)
    %137 = acir.extract %108 {field = "funct7"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc588)
    %138 = arith.cmpi eq, %137, %c1_i32 : i32 loc(#loc588)
    %139 = arith.andi %136, %138 : i1 loc(#loc562)
    %140 = arith.select %139, %false, %134 : i1 loc(#loc562)
    %141 = acir.extract %108 {field = "funct3"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc799)
    %142 = arith.cmpi ule, %141, %c3_i32 : i32 loc(#loc800)
    %143 = arith.cmpi ugt, %141, %c3_i32 : i32 loc(#loc800)
    %144 = arith.extui %83 : i32 to i64 loc(#loc801)
    %145 = arith.extui %95 : i32 to i64 loc(#loc802)
    %146 = arith.muli %144, %145 : i64 loc(#loc803)
    %147 = arith.shrui %146, %c32_i64 : i64 loc(#loc804)
    %148 = arith.trunci %147 : i64 to i32 loc(#loc805)
    %149 = arith.cmpi eq, %141, %c0_i32 : i32 loc(#loc806)
    %150 = arith.cmpi ne, %141, %c0_i32 : i32 loc(#loc806)
    %151 = arith.trunci %146 : i64 to i32 loc(#loc807)
    %152 = arith.select %149, %151, %32 : i32 loc(#loc808)
    %153 = arith.ori %149, %150 : i1 loc(#loc808)
    %154 = arith.ori %150, %98 : i1 loc(#loc808)
    %155 = arith.cmpi eq, %141, %c1_i32 : i32 loc(#loc809)
    %156 = arith.cmpi eq, %141, %c2_i32 : i32 loc(#loc810)
    %157 = arith.ori %155, %156 : i1 loc(#loc808)
    %158 = arith.andi %83, %c-2147483648_i32 : i32 loc(#loc811)
    %159 = arith.cmpi ne, %158, %c0_i32 : i32 loc(#loc812)
    %160 = arith.andi %157, %159 : i1 loc(#loc808)
    %161 = arith.xori %157, %true : i1 loc(#loc808)
    %162 = arith.ori %161, %98 : i1 loc(#loc808)
    %163 = arith.subi %148, %95 : i32 loc(#loc805)
    %164 = arith.select %160, %163, %148 : i32 loc(#loc805)
    %165 = arith.andi %162, %98 : i1 loc(#loc808)
    %166 = arith.andi %95, %c-2147483648_i32 : i32 loc(#loc813)
    %167 = arith.cmpi ne, %166, %c0_i32 : i32 loc(#loc814)
    %168 = arith.andi %155, %167 : i1 loc(#loc808)
    %169 = arith.cmpi ne, %141, %c1_i32 : i32 loc(#loc927)
    %170 = arith.ori %169, %98 : i1 loc(#loc808)
    %171 = arith.subi %164, %83 : i32 loc(#loc805)
    %172 = arith.andi %162, %170 : i1 loc(#loc808)
    %173 = arith.xori %168, %true : i1 loc(#loc808)
    %174 = arith.select %168, %171, %164 : i32 loc(#loc805)
    %175 = arith.andi %173, %162 : i1 loc(#loc808)
    %176 = arith.ori %168, %175 : i1 loc(#loc808)
    %177 = arith.andi %170, %165 : i1 loc(#loc808)
    %178 = arith.andi %176, %170 : i1 loc(#loc808)
    %179 = arith.andi %177, %172 : i1 loc(#loc808)
    %180 = arith.ori %149, %172 : i1 loc(#loc808)
    %181 = arith.select %150, %174, %152 : i32 loc(#loc808)
    %182 = arith.andi %150, %178 : i1 loc(#loc808)
    %183 = arith.andi %149, %153 : i1 loc(#loc808)
    %184 = arith.ori %182, %183 : i1 loc(#loc808)
    %185 = arith.select %150, %179, %154 : i1 loc(#loc808)
    %186 = arith.andi %185, %180 : i1 loc(#loc808)
    %187 = arith.ori %143, %180 : i1 loc(#loc808)
    %188 = arith.select %142, %181, %32 : i32 loc(#loc808)
    %189 = arith.andi %142, %184 : i1 loc(#loc808)
    %190 = arith.ori %189, %143 : i1 loc(#loc808)
    %191 = arith.ori %143, %186 : i1 loc(#loc808)
    %192 = arith.cmpi eq, %95, %c0_i32 : i32 loc(#loc815)
    %193 = arith.cmpi ne, %95, %c0_i32 : i32 loc(#loc815)
    %194 = arith.cmpi eq, %141, %c4_i32 : i32 loc(#loc816)
    %195 = arith.cmpi eq, %141, %c5_i32 : i32 loc(#loc817)
    %196 = arith.ori %194, %195 : i1 loc(#loc808)
    %197 = arith.select %196, %c-1_i32, %83 : i32 loc(#loc808)
    %198 = arith.select %192, %197, %188 : i32 loc(#loc808)
    %199 = arith.andi %193, %190 : i1 loc(#loc808)
    %200 = arith.ori %192, %199 : i1 loc(#loc808)
    %201 = arith.select %192, %98, %191 : i1 loc(#loc808)
    %202 = arith.andi %98, %201 : i1 loc(#loc808)
    %203 = arith.andi %200, %98 : i1 loc(#loc808)
    %204 = arith.cmpi ne, %141, %c5_i32 : i32 loc(#loc818)
    %205 = arith.divui %83, %95 : i32 loc(#loc819)
    %206 = arith.select %195, %205, %198 : i32 loc(#loc808)
    %207 = arith.andi %204, %203 : i1 loc(#loc808)
    %208 = arith.ori %195, %207 : i1 loc(#loc808)
    %209 = arith.select %195, %98, %202 : i1 loc(#loc808)
    %210 = arith.cmpi eq, %141, %c7_i32 : i32 loc(#loc820)
    %211 = arith.cmpi ne, %141, %c7_i32 : i32 loc(#loc820)
    %212 = arith.remui %83, %95 : i32 loc(#loc821)
    %213 = arith.select %210, %212, %206 : i32 loc(#loc808)
    %214 = arith.andi %211, %208 : i1 loc(#loc808)
    %215 = arith.ori %210, %214 : i1 loc(#loc808)
    %216 = arith.select %210, %98, %209 : i1 loc(#loc808)
    %217 = arith.subi %c0_i32, %83 : i32 loc(#loc822)
    %218 = arith.select %159, %217, %83 : i32 loc(#loc823)
    %219 = arith.subi %c0_i32, %95 : i32 loc(#loc824)
    %220 = arith.select %167, %219, %95 : i32 loc(#loc825)
    %221 = arith.cmpi ne, %141, %c4_i32 : i32 loc(#loc826)
    %222 = arith.divui %218, %220 : i32 loc(#loc827)
    %223 = arith.cmpi ne, %159, %167 : i1 loc(#loc828)
    %224 = arith.subi %c0_i32, %222 : i32 loc(#loc829)
    %225 = arith.cmpi eq, %159, %167 : i1 loc(#loc928)
    %226 = arith.select %223, %224, %222 : i32 loc(#loc808)
    %227 = arith.ori %223, %225 : i1 loc(#loc808)
    %228 = arith.andi %227, %98 : i1 loc(#loc808)
    %229 = arith.ori %221, %98 : i1 loc(#loc808)
    %230 = arith.andi %98, %229 : i1 loc(#loc808)
    %231 = arith.select %194, %226, %213 : i32 loc(#loc808)
    %232 = arith.andi %194, %228 : i1 loc(#loc808)
    %233 = arith.andi %221, %215 : i1 loc(#loc808)
    %234 = arith.ori %232, %233 : i1 loc(#loc808)
    %235 = arith.select %194, %98, %216 : i1 loc(#loc808)
    %236 = arith.remui %218, %220 : i32 loc(#loc830)
    %237 = arith.subi %c0_i32, %236 : i32 loc(#loc831)
    %238 = arith.cmpi eq, %158, %c0_i32 : i32 loc(#loc929)
    %239 = arith.select %159, %237, %236 : i32 loc(#loc808)
    %240 = arith.ori %159, %238 : i1 loc(#loc808)
    %241 = arith.andi %240, %98 : i1 loc(#loc808)
    %242 = arith.ori %194, %98 : i1 loc(#loc808)
    %243 = arith.andi %230, %242 : i1 loc(#loc808)
    %244 = arith.select %221, %239, %231 : i32 loc(#loc808)
    %245 = arith.andi %221, %241 : i1 loc(#loc808)
    %246 = arith.andi %194, %234 : i1 loc(#loc808)
    %247 = arith.ori %245, %246 : i1 loc(#loc808)
    %248 = arith.select %221, %98, %235 : i1 loc(#loc808)
    %249 = arith.andi %248, %243 : i1 loc(#loc808)
    %250 = arith.ori %210, %243 : i1 loc(#loc808)
    %251 = arith.select %211, %244, %213 : i32 loc(#loc808)
    %252 = arith.andi %211, %247 : i1 loc(#loc808)
    %253 = arith.andi %210, %215 : i1 loc(#loc808)
    %254 = arith.ori %252, %253 : i1 loc(#loc808)
    %255 = arith.select %211, %249, %216 : i1 loc(#loc808)
    %256 = arith.andi %255, %250 : i1 loc(#loc808)
    %257 = arith.ori %195, %250 : i1 loc(#loc808)
    %258 = arith.select %204, %251, %206 : i32 loc(#loc808)
    %259 = arith.andi %204, %254 : i1 loc(#loc808)
    %260 = arith.andi %195, %208 : i1 loc(#loc808)
    %261 = arith.ori %259, %260 : i1 loc(#loc808)
    %262 = arith.select %204, %256, %209 : i1 loc(#loc808)
    %263 = arith.andi %262, %257 : i1 loc(#loc808)
    %264 = arith.ori %192, %257 : i1 loc(#loc808)
    %265 = arith.andi %98, %264 : i1 loc(#loc808)
    %266 = arith.select %193, %258, %198 : i32 loc(#loc808)
    %267 = arith.andi %193, %261 : i1 loc(#loc808)
    %268 = arith.andi %192, %203 : i1 loc(#loc808)
    %269 = arith.ori %267, %268 : i1 loc(#loc808)
    %270 = arith.select %193, %263, %202 : i1 loc(#loc808)
    %271 = arith.andi %98, %270 : i1 loc(#loc808)
    %272 = arith.andi %269, %98 : i1 loc(#loc808)
    %273 = arith.andi %271, %265 : i1 loc(#loc808)
    %274 = arith.ori %142, %265 : i1 loc(#loc808)
    %275 = arith.andi %187, %274 : i1 loc(#loc808)
    %276 = arith.select %143, %266, %188 : i32 loc(#loc808)
    %277 = arith.andi %143, %272 : i1 loc(#loc808)
    %278 = arith.andi %142, %190 : i1 loc(#loc808)
    %279 = arith.ori %277, %278 : i1 loc(#loc808)
    %280 = arith.select %143, %273, %191 : i1 loc(#loc808)
    %281 = arith.andi %280, %275 : i1 loc(#loc808)
    %282 = arith.andi %279, %275 : i1 loc(#loc808)
    %283 = arith.andi %281, %282 : i1 loc(#loc808)
    %284 = arith.xori %139, %true : i1 loc(#loc562)
    %285 = arith.select %139, %276, %127 : i32 loc(#loc562)
    %286 = arith.andi %139, %282 : i1 loc(#loc562)
    %287 = arith.andi %284, %129 : i1 loc(#loc562)
    %288 = arith.ori %286, %287 : i1 loc(#loc562)
    %289 = arith.select %139, %283, %130 : i1 loc(#loc562)
    %290 = arith.cmpi eq, %110, %c19_i32 : i32 loc(#loc590)
    %291 = arith.ori %136, %290 : i1 loc(#loc562)
    %292 = arith.select %290, %113, %95 : i32 loc(#loc591)
    %293 = arith.ori %290, %98 : i1 loc(#loc562)
    %294 = arith.cmpi eq, %137, %c32_i32 : i32 loc(#loc592)
    %295 = arith.andi %136, %294 : i1 loc(#loc562)
    %296 = arith.andi %98, %293 : i1 loc(#loc593)
    %297 = arith.addi %83, %292 : i32 loc(#loc593)
    %298 = arith.subi %83, %292 : i32 loc(#loc594)
    %299 = arith.select %295, %298, %297 : i32 loc(#loc562)
    %300 = arith.select %149, %299, %285 : i32 loc(#loc562)
    %301 = arith.select %149, %296, %289 : i1 loc(#loc562)
    %302 = arith.andi %292, %c31_i32 : i32 loc(#loc595)
    %303 = arith.shli %83, %302 : i32 loc(#loc596)
    %304 = arith.select %155, %303, %300 : i32 loc(#loc562)
    %305 = arith.select %155, %296, %301 : i1 loc(#loc562)
    %306 = arith.cmpi ne, %141, %c2_i32 : i32 loc(#loc597)
    %307 = arith.xori %83, %c-2147483648_i32 : i32 loc(#loc833)
    %308 = arith.xori %292, %c-2147483648_i32 : i32 loc(#loc834)
    %309 = arith.cmpi ult, %307, %308 : i32 loc(#loc835)
    %310 = arith.extui %309 : i1 to i32 loc(#loc562)
    %311 = arith.select %156, %310, %304 : i32 loc(#loc562)
    %312 = arith.select %156, %296, %305 : i1 loc(#loc562)
    %313 = arith.cmpi eq, %141, %c3_i32 : i32 loc(#loc599)
    %314 = arith.cmpi ne, %141, %c3_i32 : i32 loc(#loc599)
    %315 = arith.cmpi ult, %83, %292 : i32 loc(#loc600)
    %316 = arith.extui %315 : i1 to i32 loc(#loc562)
    %317 = arith.select %313, %316, %311 : i32 loc(#loc562)
    %318 = arith.select %313, %296, %312 : i1 loc(#loc562)
    %319 = arith.xori %83, %292 : i32 loc(#loc601)
    %320 = arith.select %194, %319, %317 : i32 loc(#loc562)
    %321 = arith.select %194, %296, %318 : i1 loc(#loc562)
    %322 = arith.shrui %83, %302 : i32 loc(#loc602)
    %323 = arith.cmpi ne, %302, %c0_i32 : i32 loc(#loc836)
    %324 = arith.andi %323, %159 : i1 loc(#loc837)
    %325 = arith.cmpi eq, %302, %c0_i32 : i32 loc(#loc930)
    %326 = arith.ori %325, %98 : i1 loc(#loc837)
    %327 = arith.andi %293, %326 : i1 loc(#loc837)
    %328 = arith.subi %c32_i32, %302 : i32 loc(#loc838)
    %329 = arith.shli %c-1_i32, %328 : i32 loc(#loc839)
    %330 = arith.ori %322, %329 : i32 loc(#loc840)
    %331 = arith.select %324, %330, %322 : i32 loc(#loc840)
    %332 = arith.andi %327, %296 : i1 loc(#loc837)
    %333 = arith.select %294, %331, %322 : i32 loc(#loc562)
    %334 = arith.select %294, %332, %296 : i1 loc(#loc562)
    %335 = arith.select %195, %333, %320 : i32 loc(#loc562)
    %336 = arith.select %195, %334, %321 : i1 loc(#loc562)
    %337 = arith.cmpi eq, %141, %c6_i32 : i32 loc(#loc604)
    %338 = arith.cmpi ne, %141, %c6_i32 : i32 loc(#loc604)
    %339 = arith.ori %83, %292 : i32 loc(#loc605)
    %340 = arith.select %337, %339, %335 : i32 loc(#loc562)
    %341 = arith.select %337, %296, %336 : i1 loc(#loc562)
    %342 = arith.andi %83, %292 : i32 loc(#loc606)
    %343 = arith.select %338, %342, %340 : i32 loc(#loc562)
    %344 = arith.select %338, %296, %341 : i1 loc(#loc562)
    %345 = arith.select %204, %343, %335 : i32 loc(#loc562)
    %346 = arith.select %204, %344, %336 : i1 loc(#loc562)
    %347 = arith.select %221, %345, %320 : i32 loc(#loc562)
    %348 = arith.select %221, %346, %321 : i1 loc(#loc562)
    %349 = arith.select %314, %347, %317 : i32 loc(#loc562)
    %350 = arith.select %314, %348, %318 : i1 loc(#loc562)
    %351 = arith.select %306, %349, %311 : i32 loc(#loc562)
    %352 = arith.select %306, %350, %312 : i1 loc(#loc562)
    %353 = arith.select %169, %351, %304 : i32 loc(#loc562)
    %354 = arith.select %169, %352, %305 : i1 loc(#loc562)
    %355 = arith.select %150, %353, %300 : i32 loc(#loc562)
    %356 = arith.select %150, %354, %301 : i1 loc(#loc562)
    %357 = arith.select %291, %356, %289 : i1 loc(#loc562)
    %358 = arith.select %291, %355, %c0_i32 : i32 loc(#loc562)
    %359 = arith.xori %140, %true : i1 loc(#loc562)
    %360 = arith.select %140, %358, %285 : i32 loc(#loc562)
    %361 = arith.andi %359, %288 : i1 loc(#loc562)
    %362 = arith.ori %140, %361 : i1 loc(#loc562)
    %363 = arith.select %140, %357, %289 : i1 loc(#loc562)
    %364 = arith.select %133, %124, %360 : i32 loc(#loc562)
    %365 = arith.andi %134, %362 : i1 loc(#loc562)
    %366 = arith.ori %133, %365 : i1 loc(#loc562)
    %367 = arith.select %133, %98, %363 : i1 loc(#loc562)
    %368 = arith.andi %123, %135 : i1 loc(#loc562)
    %369 = arith.xori %123, %true : i1 loc(#loc562)
    %370 = arith.select %123, %364, %127 : i32 loc(#loc562)
    %371 = arith.andi %123, %366 : i1 loc(#loc562)
    %372 = arith.andi %369, %129 : i1 loc(#loc562)
    %373 = arith.ori %371, %372 : i1 loc(#loc562)
    %374 = arith.select %123, %367, %130 : i1 loc(#loc562)
    %375 = arith.addi %109, %113 : i32 loc(#loc607)
    %376 = arith.select %120, %121, %368 : i1 loc(#loc562)
    %377 = arith.select %120, %375, %370 : i32 loc(#loc562)
    %378 = arith.andi %121, %373 : i1 loc(#loc562)
    %379 = arith.ori %120, %378 : i1 loc(#loc562)
    %380 = arith.ori %120, %374 : i1 loc(#loc562)
    %381 = arith.andi %112, %376 : i1 loc(#loc562)
    %382 = arith.select %112, %377, %114 : i32 loc(#loc110)
    %383 = arith.andi %112, %379 : i1 loc(#loc562)
    %384 = arith.andi %111, %115 : i1 loc(#loc562)
    %385 = arith.ori %383, %384 : i1 loc(#loc562)
    %386 = arith.ori %111, %380 : i1 loc(#loc562)
    %387 = arith.xori %381, %true : i1 loc(#loc562)
    %388 = acir.assert %387 {check_kind = "return_coverage"} loc(#loc562)
    %389 = arith.andi %386, %388 : i1 loc(#loc562)
    %390 = arith.andi %385, %388 : i1 loc(#loc562)
    %391 = arith.andi %389, %390 : i1 loc(#loc562)
    %392 = acir.extract %108 {field = "illegal"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i1 loc(#loc111)
    %393 = arith.ori %117, %122 : i1
    %394 = arith.xori %392, %true : i1 loc(#loc112)
    %395 = arith.andi %394, %393 : i1 loc(#loc113)
    %396 = arith.andi %102, %394 : i1
    %397 = acir.aggregate() : () -> i1 loc(#loc608)
    %398 = arith.cmpi ne, %110, %c99_i32 : i32 loc(#loc609)
    %399 = arith.select %398, %false, %397 : i1 loc(#loc608)
    %400 = arith.cmpi eq, %83, %95 : i32 loc(#loc610)
    %401 = arith.select %149, %400, %399 : i1 loc(#loc608)
    %402 = arith.cmpi ne, %83, %95 : i32 loc(#loc611)
    %403 = arith.select %155, %402, %401 : i1 loc(#loc608)
    %404 = arith.andi %169, %153 : i1 loc(#loc608)
    %405 = arith.ori %155, %404 : i1 loc(#loc608)
    %406 = arith.select %155, %98, %154 : i1 loc(#loc608)
    %407 = arith.xori %95, %c-2147483648_i32 : i32 loc(#loc841)
    %408 = arith.cmpi ult, %307, %407 : i32 loc(#loc842)
    %409 = arith.select %194, %408, %403 : i1 loc(#loc608)
    %410 = arith.andi %221, %405 : i1 loc(#loc608)
    %411 = arith.ori %194, %410 : i1 loc(#loc608)
    %412 = arith.select %194, %98, %406 : i1 loc(#loc608)
    %413 = arith.cmpi uge, %307, %407 : i32 loc(#loc931)
    %414 = arith.select %195, %413, %409 : i1 loc(#loc608)
    %415 = arith.andi %204, %411 : i1 loc(#loc608)
    %416 = arith.ori %195, %415 : i1 loc(#loc608)
    %417 = arith.select %195, %98, %412 : i1 loc(#loc608)
    %418 = arith.cmpi ult, %83, %95 : i32 loc(#loc615)
    %419 = arith.select %337, %418, %414 : i1 loc(#loc608)
    %420 = arith.andi %338, %416 : i1 loc(#loc608)
    %421 = arith.ori %337, %420 : i1 loc(#loc608)
    %422 = arith.select %337, %98, %417 : i1 loc(#loc608)
    %423 = acir.assert %210 loc(#loc616)
    %424 = arith.cmpi uge, %83, %95 : i32 loc(#loc617)
    %425 = arith.andi %98, %423 : i1 loc(#loc608)
    %426 = arith.ori %337, %423 : i1 loc(#loc608)
    %427 = arith.select %338, %424, %419 : i1 loc(#loc608)
    %428 = arith.andi %337, %421 : i1 loc(#loc608)
    %429 = arith.ori %338, %428 : i1 loc(#loc608)
    %430 = arith.select %338, %425, %422 : i1 loc(#loc608)
    %431 = arith.andi %430, %426 : i1 loc(#loc608)
    %432 = arith.ori %195, %426 : i1 loc(#loc608)
    %433 = arith.select %204, %427, %414 : i1 loc(#loc608)
    %434 = arith.andi %204, %429 : i1 loc(#loc608)
    %435 = arith.andi %195, %416 : i1 loc(#loc608)
    %436 = arith.ori %434, %435 : i1 loc(#loc608)
    %437 = arith.select %204, %431, %417 : i1 loc(#loc608)
    %438 = arith.andi %437, %432 : i1 loc(#loc608)
    %439 = arith.ori %194, %432 : i1 loc(#loc608)
    %440 = arith.select %221, %433, %409 : i1 loc(#loc608)
    %441 = arith.andi %221, %436 : i1 loc(#loc608)
    %442 = arith.andi %194, %411 : i1 loc(#loc608)
    %443 = arith.ori %441, %442 : i1 loc(#loc608)
    %444 = arith.select %221, %438, %412 : i1 loc(#loc608)
    %445 = arith.andi %444, %439 : i1 loc(#loc608)
    %446 = arith.ori %155, %439 : i1 loc(#loc608)
    %447 = arith.select %169, %440, %403 : i1 loc(#loc608)
    %448 = arith.andi %169, %443 : i1 loc(#loc608)
    %449 = arith.andi %155, %405 : i1 loc(#loc608)
    %450 = arith.ori %448, %449 : i1 loc(#loc608)
    %451 = arith.select %169, %445, %406 : i1 loc(#loc608)
    %452 = arith.andi %451, %446 : i1 loc(#loc608)
    %453 = arith.ori %149, %446 : i1 loc(#loc608)
    %454 = arith.select %150, %447, %401 : i1 loc(#loc608)
    %455 = arith.andi %150, %450 : i1 loc(#loc608)
    %456 = arith.ori %455, %183 : i1 loc(#loc608)
    %457 = arith.select %150, %452, %154 : i1 loc(#loc608)
    %458 = arith.andi %457, %453 : i1 loc(#loc608)
    %459 = arith.ori %398, %453 : i1 loc(#loc608)
    %460 = arith.select %119, %454, %399 : i1 loc(#loc608)
    %461 = arith.andi %119, %456 : i1 loc(#loc608)
    %462 = arith.ori %461, %398 : i1 loc(#loc608)
    %463 = arith.ori %398, %458 : i1 loc(#loc608)
    %464 = arith.andi %463, %459 : i1 loc(#loc608)
    %465 = arith.andi %462, %459 : i1 loc(#loc608)
    %466 = arith.andi %464, %465 : i1 loc(#loc608)
    %467 = arith.xori %395, %true : i1
    %468 = arith.ori %395, %460 : i1
    %469 = arith.andi %467, %465 : i1
    %470 = arith.ori %395, %469 : i1
    %471 = arith.ori %395, %466 : i1
    %472 = arith.xori %396, %true : i1
    %473 = arith.andi %396, %468 : i1 loc(#loc124)
    %474 = arith.andi %396, %470 : i1
    %475 = arith.ori %474, %472 : i1
    %476 = arith.ori %472, %471 : i1
    %477 = acir.extract %value_8 {field = "next_pc"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i32 loc(#loc125)
    %478 = arith.select %395, %477, %382 : i32 loc(#loc126)
    %479 = arith.ori %395, %391 : i1
    %480 = acir.extract %value_8 {field = "word"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i32 loc(#loc127)
    %481 = acir.extract %108 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc128)
    %482 = acir.extract %108 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i1 loc(#loc129)
    %483 = acir.extract %108 {field = "load"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i1 loc(#loc130)
    %484 = acir.extract %108 {field = "store"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i1 loc(#loc131)
    %485 = acir.extract %value_8 {field = "stalled"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i1 loc(#loc132)
    %486 = acir.extract %108 {field = "system"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc133)
    %487 = acir.extract %value_8 {field = "expanded"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i32 loc(#loc134)
    %488 = arith.andi %391, %98 : i1
    %489 = arith.andi %488, %479 : i1
    %490 = acir.aggregate(%102, %109, %480, %481, %482, %483, %484, %382, %95, %478, %485, %141, %392, %486, %487, %477) : (i1, i32, i32, i32, i1, i1, i1, i32, i32, i32, i1, i32, i1, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.Executed"> loc(#loc135)
    %491 = arith.andi %476, %391 : i1
    %492 = arith.select %473, %382, %c0_i32 : i32
    %493 = arith.andi %491, %475 : i1
    %494 = arith.andi %489, %476 : i1 loc(#loc136)
    %495 = arith.andi %494, %493 : i1 loc(#loc136)
    %496 = acir.aggregate(%490, %473, %492) : (!acir.struct<"src_isa_ae2f66cd.Executed">, i1, i32) -> !acir.struct<"src_isa_ae2f66cd.ExecutionResult"> loc(#loc136)
    %497 = arith.andi %489, %475 : i1 loc(#loc136)
    %498 = arith.andi %476, %475 : i1 loc(#loc136)
    %499 = arith.andi %495, %107 : i1
    %500 = arith.andi %475, %107 : i1
    %501 = arith.andi %497, %107 : i1
    %502 = arith.andi %498, %107 : i1
    %503 = arith.andi %493, %107 : i1
    %504 = arith.andi %501, %499 : i1
    %505 = arith.andi %502, %499 : i1
    %506 = arith.andi %503, %499 : i1
    %507 = acir.export %496 : !acir.struct<"src_isa_ae2f66cd.ExecutionResult"> -> !acir.struct<"src_isa_ae2f66cd.ExecutionResult"> loc(#loc137)
    %508 = acir.export %499 : i1 -> i1
    %509 = arith.andi %504, %500 : i1
    %510 = arith.andi %505, %500 : i1
    %511 = arith.andi %506, %500 : i1
    acir.path %true {branch = 0 : i64, frames = ["top.cpu.hazard_unit:ecall_wait"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64}
    %value_10, %ready_11 = acir.read(%true) {branch = 0 : i64, frames = ["top.cpu.hazard_unit:ecall_wait"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1) loc(#loc138)
    %512 = acir.extract %value_10 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i1 loc(#loc138)
    acir.path %512 {branch = 1 : i64, kind = "branch", txn = -1 : i64} loc(#loc139)
    %value_12, %ready_13 = acir.read(%512) {branch = 1 : i64, frames = ["top.cpu.hazard_unit:ecall_wait"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1) loc(#loc139)
    %513 = acir.extract %value_12 {field = "instruction"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc139)
    %514 = acir.extract %513 {field = "system"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc139)
    %515 = arith.cmpi eq, %514, %c1_i32 : i32 loc(#loc139)
    %516 = arith.andi %512, %515 : i1
    acir.path %516 {branch = 2 : i64, kind = "branch", txn = -1 : i64} loc(#loc140)
    %value_14, %ready_15 = acir.read(%516) {branch = 2 : i64, frames = ["top.cpu.hazard_unit:ecall_wait"], method = "read", port = 0 : i64, resource = "top.cpu.ex_mem", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Executed">, i1) loc(#loc140)
    %517 = acir.extract %value_14 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc140)
    %518 = arith.xori %517, %true : i1
    %519 = arith.andi %516, %518 : i1
    acir.path %519 {branch = 3 : i64, kind = "branch", txn = -1 : i64} loc(#loc141)
    %value_16, %ready_17 = acir.read(%519) {branch = 3 : i64, frames = ["top.cpu.hazard_unit:ecall_wait"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1) loc(#loc141)
    %520 = acir.extract %value_16 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc141)
    %521 = arith.ori %517, %520 : i1
    %522 = arith.andi %516, %521 : i1
    %523 = acir.export %522 : i1 -> i1 loc(#loc142)
    %524 = acir.export %true : i1 -> i1
    %value_18, %ready_19 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1) loc(#loc143)
    %525 = acir.extract %value_18 {field = "instruction"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc144)
    %value_20, %ready_21 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cpu.halted", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc145)
    %526 = arith.xori %value_20, %true : i1
    acir.path %526 {branch = 4 : i64, kind = "branch", txn = -1 : i64} loc(#loc146)
    %value_22, %ready_23 = acir.read(%526) {branch = 4 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1) loc(#loc146)
    %527 = acir.extract %value_22 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i1 loc(#loc146)
    %528 = arith.andi %526, %527 : i1
    %529 = acir.extract %525 {field = "illegal"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i1 loc(#loc147)
    %530 = acir.extract %525 {field = "system"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc148)
    %531 = arith.cmpi eq, %530, %c2_i32 : i32 loc(#loc148)
    %532 = arith.ori %529, %531 : i1
    %533 = arith.xori %532, %true : i1
    %534 = arith.andi %528, %533 : i1
    %535 = arith.cmpi eq, %530, %c1_i32 : i32 loc(#loc149)
    %536 = arith.xori %523, %true : i1 loc(#loc150)
    %537 = arith.andi %535, %536 : i1
    %538 = arith.cmpi ne, %530, %c1_i32 : i32 loc(#loc149)
    %539 = arith.ori %538, %524 : i1
    %540 = arith.andi %534, %537 : i1
    acir.path %540 {branch = 5 : i64, kind = "branch", txn = -1 : i64} loc(#loc151)
    %value_24, %ready_25 = acir.read(%540) {branch = 5 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1) loc(#loc151)
    acir.path %540 {branch = 5 : i64, frames = ["top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc152)
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc153)
    %541 = acir.extract %value_24 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc154)
    %542 = acir.extract %value_24 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc155)
    %543 = arith.cmpi eq, %542, %c17_i32 : i32 loc(#loc155)
    %544 = arith.andi %541, %543 : i1
    %545 = arith.xori %544, %true : i1
    %546 = arith.select %544, %545, %false : i1
    %547 = arith.andi %540, %545 : i1
    acir.path %547 {branch = 6 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    %value_26, %ready_27 = acir.read(%547, %c17_i64) {array = "top.cpu.rf.registers", branch = 6 : i64, frames = ["top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = -1 : i64} : (i1, i64) -> (i32, i1) loc(#loc156)
    acir.path %false {branch = 7 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 8 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 9 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 10 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 11 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 12 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 13 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 14 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 15 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 16 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 17 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 18 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 19 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 20 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 21 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 22 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 23 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %547 {branch = 24 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 25 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 26 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 27 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 28 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 29 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 30 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 31 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 32 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 33 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 34 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 35 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 36 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 37 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 38 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    %548 = acir.extract %value_24 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc157)
    %549 = arith.select %544, %548, %value_26 : i32
    %550 = arith.xori %546, %true : i1
    %551 = acir.assert %550 {check_kind = "return_coverage"}
    %552 = acir.export %549 : i32 -> i32
    %553 = acir.export %551 : i1 -> i1
    %554 = arith.andi %553, %551 : i1
    %555 = arith.cmpi eq, %552, %c10_i32 : i32 loc(#loc152)
    %556 = arith.cmpi ne, %552, %c10_i32 : i32 loc(#loc152)
    %557 = arith.andi %540, %556 : i1
    acir.path %557 {branch = 39 : i64, kind = "branch", txn = -1 : i64} loc(#loc158)
    %value_28, %ready_29 = acir.read(%557) {branch = 39 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1) loc(#loc158)
    acir.path %557 {branch = 39 : i64, frames = ["top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc159)
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc153)
    %558 = acir.extract %value_28 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc154)
    %559 = acir.extract %value_28 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc155)
    %560 = arith.cmpi eq, %559, %c17_i32 : i32 loc(#loc155)
    %561 = arith.andi %558, %560 : i1
    %562 = arith.xori %561, %true : i1
    %563 = arith.select %561, %562, %false : i1
    %564 = arith.andi %557, %562 : i1
    acir.path %564 {branch = 40 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    %value_30, %ready_31 = acir.read(%564, %c17_i64) {array = "top.cpu.rf.registers", branch = 40 : i64, frames = ["top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = -1 : i64} : (i1, i64) -> (i32, i1) loc(#loc156)
    acir.path %false {branch = 41 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 42 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 43 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 44 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 45 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 46 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 47 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 48 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 49 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 50 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 51 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 52 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 53 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 54 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 55 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 56 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 57 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %564 {branch = 58 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 59 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 60 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 61 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 62 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 63 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 64 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 65 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 66 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 67 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 68 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 69 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 70 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 71 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 72 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    %565 = acir.extract %value_28 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc157)
    %566 = arith.select %561, %565, %value_30 : i32
    %567 = arith.xori %563, %true : i1
    %568 = acir.assert %567 {check_kind = "return_coverage"}
    %569 = acir.export %566 : i32 -> i32
    %570 = acir.export %568 : i1 -> i1
    %571 = arith.andi %570, %568 : i1
    %572 = arith.cmpi eq, %569, %c93_i32 : i32 loc(#loc159)
    %573 = arith.ori %555, %572 : i1
    %574 = arith.ori %555, %571 : i1
    %575 = arith.andi %554, %574 : i1
    %576 = arith.xori %573, %true : i1
    %577 = arith.andi %540, %576 : i1
    acir.path %577 {branch = 73 : i64, kind = "branch", txn = -1 : i64} loc(#loc160)
    %value_32, %ready_33 = acir.read(%577) {branch = 73 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1) loc(#loc160)
    acir.path %577 {branch = 73 : i64, frames = ["top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc161)
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc153)
    %578 = acir.extract %value_32 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc154)
    %579 = acir.extract %value_32 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc155)
    %580 = arith.cmpi eq, %579, %c17_i32 : i32 loc(#loc155)
    %581 = arith.andi %578, %580 : i1
    %582 = arith.xori %581, %true : i1
    %583 = arith.select %581, %582, %false : i1
    %584 = arith.andi %577, %582 : i1
    acir.path %584 {branch = 74 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    %value_34, %ready_35 = acir.read(%584, %c17_i64) {array = "top.cpu.rf.registers", branch = 74 : i64, frames = ["top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = -1 : i64} : (i1, i64) -> (i32, i1) loc(#loc156)
    acir.path %false {branch = 75 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 76 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 77 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 78 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 79 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 80 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 81 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 82 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 83 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 84 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 85 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 86 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 87 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 88 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 89 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 90 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 91 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %584 {branch = 92 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 93 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 94 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 95 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 96 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 97 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 98 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 99 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 100 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 101 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 102 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 103 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 104 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 105 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    acir.path %false {branch = 106 : i64, kind = "branch", txn = -1 : i64} loc(#loc156)
    %585 = acir.extract %value_32 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc157)
    %586 = arith.select %581, %585, %value_34 : i32
    %587 = arith.xori %583, %true : i1
    %588 = acir.assert %587 {check_kind = "return_coverage"}
    %589 = acir.export %586 : i32 -> i32 loc(#loc618)
    %590 = acir.export %588 : i1 -> i1
    %591 = arith.andi %590, %588 : i1
    %592 = arith.cmpi eq, %589, %c1_i32 : i32 loc(#loc619)
    %593 = arith.cmpi eq, %589, %c4_i32 : i32 loc(#loc620)
    %594 = arith.ori %592, %593 : i1 loc(#loc621)
    %595 = arith.ori %592, %591 : i1 loc(#loc621)
    %596 = arith.andi %591, %595 : i1 loc(#loc621)
    %597 = arith.cmpi eq, %589, %c11_i32 : i32 loc(#loc622)
    %598 = arith.ori %594, %597 : i1 loc(#loc621)
    %599 = arith.ori %594, %591 : i1 loc(#loc621)
    %600 = arith.andi %596, %599 : i1 loc(#loc621)
    %601 = arith.cmpi eq, %589, %c10_i32 : i32 loc(#loc623)
    %602 = arith.ori %598, %601 : i1 loc(#loc621)
    %603 = arith.ori %598, %591 : i1 loc(#loc621)
    %604 = arith.andi %600, %603 : i1 loc(#loc621)
    %605 = arith.cmpi eq, %589, %c93_i32 : i32 loc(#loc624)
    %606 = arith.ori %602, %605 : i1 loc(#loc621)
    %607 = arith.ori %602, %591 : i1 loc(#loc621)
    %608 = arith.andi %604, %607 : i1 loc(#loc621)
    %609 = arith.xori %606, %true : i1 loc(#loc169)
    %610 = arith.ori %573, %609 : i1
    %611 = arith.ori %573, %608 : i1
    %612 = arith.andi %575, %611 : i1
    %613 = arith.xori %537, %true : i1
    %614 = arith.ori %613, %575 : i1
    %615 = arith.andi %539, %614 : i1
    %616 = arith.andi %537, %610 : i1
    %617 = arith.andi %537, %575 : i1
    %618 = arith.ori %617, %613 : i1
    %619 = arith.ori %613, %612 : i1
    %620 = arith.andi %539, %619 : i1
    %621 = arith.andi %618, %539 : i1
    %622 = arith.andi %620, %615 : i1
    %623 = arith.ori %532, %615 : i1
    %624 = arith.ori %532, %616 : i1
    %625 = arith.andi %533, %621 : i1
    %626 = arith.ori %532, %625 : i1
    %627 = arith.ori %532, %622 : i1
    %628 = arith.andi %627, %623 : i1
    %629 = arith.xori %527, %true : i1
    %630 = arith.ori %629, %623 : i1
    %631 = arith.andi %527, %624 : i1
    %632 = arith.andi %527, %626 : i1
    %633 = arith.ori %632, %629 : i1
    %634 = arith.ori %629, %628 : i1
    %635 = arith.andi %634, %630 : i1
    %636 = arith.ori %value_20, %631 : i1 loc(#loc170)
    %637 = arith.andi %526, %633 : i1
    %638 = arith.ori %value_20, %637 : i1
    %639 = arith.ori %value_20, %635 : i1
    %640 = arith.andi %29, %510 : i1
    %641 = arith.andi %640, %511 : i1
    %642 = arith.andi %641, %524 : i1
    %643 = arith.andi %642, %639 : i1
    %644 = acir.aggregate(%27, %473, %492, %55, %71, %523, %636) : (i1, i1, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_ae2f66cd.Control"> loc(#loc171)
    %645 = arith.andi %29, %500 : i1
    %646 = arith.andi %524, %500 : i1
    %647 = arith.andi %639, %500 : i1
    %648 = acir.export %644 {acir.id = "v94", acir.source_type = "src_isa_ae2f66cd.Control"} : !acir.struct<"src_isa_ae2f66cd.Control"> -> !acir.struct<"src_isa_ae2f66cd.Control"> loc(#loc172)
    acir.path %true {branch = 107 : i64, kind = "branch", txn = 0 : i64} loc(#loc173)
    acir.path %true {branch = 107 : i64, frames = ["top.cpu.fetch:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc173)
    %649 = acir.aggregate() : () -> !acir.struct<"src_isa_ae2f66cd.Fetched">
    %650 = arith.andi %638, %500 : i1 loc(#loc174)
    %651 = arith.andi %647, %650 : i1 loc(#loc174)
    %652 = arith.andi %643, %651 : i1
    %653 = arith.xori %636, %true : i1 loc(#loc175)
    %654 = arith.ori %653, %651 : i1
    acir.path %636 {branch = 108 : i64, kind = "branch", txn = 0 : i64} loc(#loc175)
    %value_36, %ready_37 = acir.read(%636) {branch = 108 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc175)
    %655 = arith.ori %653, %654 : i1 loc(#loc175)
    %656 = arith.andi %652, %655 : i1 loc(#loc175)
    %657 = arith.cmpi ult, %value_36, %c4096_i32 : i32 loc(#loc175)
    %658 = arith.andi %636, %657 : i1
    %659 = arith.xori %658, %true : i1 loc(#loc176)
    %660 = arith.ori %659, %654 : i1
    acir.path %658 {branch = 109 : i64, kind = "branch", txn = 0 : i64} loc(#loc176)
    %value_38, %ready_39 = acir.read(%658) {branch = 109 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc176)
    %661 = arith.ori %659, %660 : i1 loc(#loc176)
    %662 = arith.andi %656, %661 : i1 loc(#loc176)
    %value_40, %ready_41 = acir.read(%658) {branch = 109 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc177)
    %663 = arith.shrui %value_40, %c2_i32 : i32 loc(#loc177)
    %664 = arith.extui %663 : i32 to i64 loc(#loc176)
    %665 = arith.cmpi ult, %664, %c1024_i64 : i64 loc(#loc176)
    %666 = arith.select %665, %664, %c0_i64 : i64 loc(#loc176)
    %667 = "acir.array.index"(%value_38, %666) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc176)
    %668 = arith.andi %value_40, %c3_i32 : i32 loc(#loc178)
    %669 = arith.muli %668, %c8_i32 : i32 loc(#loc179)
    %670 = arith.shrui %667, %669 : i32 loc(#loc180)
    %671 = arith.andi %value_40, %c2_i32 : i32 loc(#loc181)
    %672 = arith.cmpi ne, %671, %c0_i32 : i32 loc(#loc182)
    %673 = arith.andi %658, %672 : i1
    %674 = arith.xori %673, %true : i1 loc(#loc183)
    %675 = arith.ori %674, %660 : i1
    acir.path %673 {branch = 110 : i64, kind = "branch", txn = 0 : i64} loc(#loc183)
    %value_42, %ready_43 = acir.read(%673) {branch = 110 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc183)
    %676 = arith.ori %674, %675 : i1 loc(#loc183)
    %677 = arith.andi %662, %676 : i1 loc(#loc183)
    %678 = arith.addi %value_42, %c3_i32 : i32 loc(#loc183)
    %679 = arith.cmpi ult, %678, %c4096_i32 : i32 loc(#loc183)
    %680 = arith.andi %672, %679 : i1
    %681 = arith.andi %677, %661 : i1
    %682 = arith.andi %658, %680 : i1
    %683 = arith.xori %682, %true : i1 loc(#loc184)
    %684 = arith.ori %683, %660 : i1
    acir.path %682 {branch = 111 : i64, kind = "branch", txn = 0 : i64} loc(#loc184)
    %value_44, %ready_45 = acir.read(%682) {branch = 111 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc184)
    %685 = arith.ori %683, %684 : i1 loc(#loc184)
    %686 = arith.andi %681, %685 : i1 loc(#loc184)
    %value_46, %ready_47 = acir.read(%682) {branch = 111 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc185)
    %687 = arith.shrui %value_46, %c2_i32 : i32 loc(#loc185)
    %688 = arith.addi %687, %c1_i32 : i32 loc(#loc186)
    %689 = arith.extui %688 : i32 to i64 loc(#loc184)
    %690 = arith.cmpi ult, %689, %c1024_i64 : i64 loc(#loc184)
    %691 = arith.select %690, %689, %c0_i64 : i64 loc(#loc184)
    %692 = "acir.array.index"(%value_44, %691) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc184)
    %693 = arith.shli %692, %c16_i32 : i32 loc(#loc184)
    %694 = arith.andi %665, %690 : i1
    %695 = arith.ori %670, %693 : i32 loc(#loc180)
    %696 = arith.select %680, %695, %670 : i32 loc(#loc180)
    %697 = arith.select %680, %694, %665 : i1
    %698 = arith.cmpi eq, %696, %c0_i32 : i32 loc(#loc187)
    %699 = arith.andi %660, %697 : i1
    %700 = arith.ori %659, %699 : i1
    %701 = arith.andi %686, %700 : i1
    %702 = arith.andi %696, %c3_i32 : i32 loc(#loc188)
    %703 = arith.cmpi eq, %702, %c3_i32 : i32 loc(#loc189)
    %704 = arith.ori %698, %703 : i1
    %705 = arith.ori %698, %697 : i1
    %706 = arith.andi %697, %705 : i1
    %707 = arith.select %704, %c4_i32, %c2_i32 : i32 loc(#loc190)
    %value_48, %ready_49 = acir.read(%658) {branch = 109 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc191)
    %708 = arith.andi %701, %661 : i1 loc(#loc191)
    %709 = arith.addi %value_48, %707 : i32 loc(#loc191)
    %710 = arith.andi %660, %706 : i1 loc(#loc192)
    %711 = arith.ori %659, %710 : i1 loc(#loc192)
    %712 = arith.andi %708, %711 : i1 loc(#loc192)
    %713 = arith.cmpi uge, %value_36, %c4096_i32 : i32 loc(#loc175)
    %714 = arith.ori %713, %697 : i1
    %715 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %false) : (i1, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_ae2f66cd.Fetched"> loc(#loc193)
    %716 = arith.ori %653, %714 : i1
    %717 = arith.andi %651, %716 : i1
    %718 = arith.select %636, %715, %649 : !acir.struct<"src_isa_ae2f66cd.Fetched">
    %719 = arith.ori %636, %651 : i1
    %720 = arith.andi %26, %500 : i1 loc(#loc194)
    %721 = arith.andi %645, %720 : i1 loc(#loc194)
    %722 = arith.andi %719, %721 : i1
    %723 = arith.ori %636, %722 : i1
    %724 = arith.andi %712, %723 : i1
    %725 = arith.ori %27, %523 : i1
    %726 = arith.ori %27, %646 : i1
    %727 = arith.andi %721, %726 : i1
    %728 = arith.andi %719, %727 : i1
    %729 = arith.ori %636, %728 : i1
    %730 = arith.andi %724, %729 : i1
    %731 = arith.xori %725, %true : i1
    %732 = arith.andi %653, %731 : i1
    %733 = arith.xori %732, %true : i1
    %734 = arith.ori %733, %728 : i1
    %735 = arith.andi %734, %510 : i1
    %736 = arith.ori %733, %735 : i1
    %737 = arith.andi %730, %736 : i1
    %738 = arith.xori %473, %true : i1
    %739 = arith.andi %732, %738 : i1
    %740 = arith.xori %739, %true : i1 loc(#loc195)
    %741 = arith.ori %740, %735 : i1
    acir.path %739 {branch = 112 : i64, kind = "branch", txn = 0 : i64} loc(#loc195)
    %value_50, %ready_51 = acir.read(%739) {branch = 112 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc195)
    %742 = arith.ori %740, %741 : i1 loc(#loc195)
    %743 = arith.andi %737, %742 : i1 loc(#loc195)
    %744 = arith.cmpi uge, %value_50, %c4096_i32 : i32 loc(#loc195)
    %745 = arith.cmpi ult, %value_50, %c4096_i32 : i32 loc(#loc195)
    %746 = arith.andi %739, %745 : i1
    %747 = arith.xori %746, %true : i1 loc(#loc196)
    %748 = arith.ori %747, %741 : i1
    acir.path %746 {branch = 113 : i64, kind = "branch", txn = 0 : i64} loc(#loc196)
    %value_52, %ready_53 = acir.read(%746) {branch = 113 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc196)
    %749 = arith.ori %747, %748 : i1 loc(#loc196)
    %750 = arith.andi %743, %749 : i1 loc(#loc196)
    %751 = arith.andi %value_52, %c1_i32 : i32 loc(#loc196)
    %752 = arith.cmpi ne, %751, %c0_i32 : i32 loc(#loc197)
    %753 = arith.ori %744, %752 : i1
    %754 = arith.andi %750, %742 : i1
    %755 = arith.xori %753, %true : i1
    %756 = arith.andi %739, %755 : i1
    %757 = arith.xori %756, %true : i1 loc(#loc198)
    %758 = arith.ori %757, %741 : i1
    acir.path %756 {branch = 114 : i64, kind = "branch", txn = 0 : i64} loc(#loc198)
    %value_54, %ready_55 = acir.read(%756) {branch = 114 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc198)
    %759 = arith.ori %757, %758 : i1 loc(#loc198)
    %760 = arith.andi %754, %759 : i1 loc(#loc198)
    %value_56, %ready_57 = acir.read(%756) {branch = 114 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc199)
    %761 = arith.shrui %value_56, %c2_i32 : i32 loc(#loc199)
    %762 = arith.extui %761 : i32 to i64 loc(#loc198)
    %763 = arith.cmpi ult, %762, %c1024_i64 : i64 loc(#loc198)
    %764 = arith.select %763, %762, %c0_i64 : i64 loc(#loc198)
    %765 = "acir.array.index"(%value_54, %764) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc198)
    %766 = arith.andi %value_56, %c3_i32 : i32 loc(#loc200)
    %767 = arith.muli %766, %c8_i32 : i32 loc(#loc201)
    %768 = arith.shrui %765, %767 : i32 loc(#loc180)
    %769 = arith.andi %value_56, %c2_i32 : i32 loc(#loc202)
    %770 = arith.cmpi ne, %769, %c0_i32 : i32 loc(#loc203)
    %771 = arith.andi %768, %c3_i32 : i32 loc(#loc204)
    %772 = arith.cmpi eq, %771, %c3_i32 : i32 loc(#loc205)
    %773 = arith.andi %770, %772 : i1
    %774 = arith.cmpi eq, %769, %c0_i32 : i32 loc(#loc203)
    %775 = arith.ori %774, %763 : i1
    %776 = arith.andi %758, %775 : i1
    %777 = arith.ori %757, %776 : i1
    %778 = arith.andi %760, %777 : i1
    %779 = arith.andi %756, %773 : i1
    %780 = arith.xori %779, %true : i1 loc(#loc206)
    %781 = arith.ori %780, %776 : i1
    acir.path %779 {branch = 115 : i64, kind = "branch", txn = 0 : i64} loc(#loc206)
    %value_58, %ready_59 = acir.read(%779) {branch = 115 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc206)
    %782 = arith.ori %780, %781 : i1 loc(#loc206)
    %783 = arith.andi %778, %782 : i1 loc(#loc206)
    %784 = arith.addi %value_58, %c3_i32 : i32 loc(#loc206)
    %785 = arith.cmpi uge, %784, %c4096_i32 : i32 loc(#loc206)
    %786 = arith.cmpi ult, %784, %c4096_i32 : i32 loc(#loc206)
    %787 = arith.andi %779, %785 : i1
    %788 = arith.xori %787, %true : i1 loc(#loc207)
    %789 = arith.ori %788, %781 : i1
    acir.path %787 {branch = 116 : i64, kind = "branch", txn = 0 : i64} loc(#loc207)
    %value_60, %ready_61 = acir.read(%787) {branch = 116 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc207)
    %790 = arith.ori %788, %789 : i1 loc(#loc207)
    %791 = arith.andi %783, %790 : i1 loc(#loc207)
    %792 = arith.andi %768, %c65535_i32 : i32 loc(#loc208)
    %793 = arith.addi %value_60, %c4_i32 : i32 loc(#loc209)
    %794 = acir.aggregate(%true, %value_60, %792, %792, %793, %true) : (i1, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_ae2f66cd.Fetched"> loc(#loc210)
    %795 = arith.select %785, %794, %718 : !acir.struct<"src_isa_ae2f66cd.Fetched">
    %796 = arith.andi %786, %651 : i1
    %797 = arith.ori %785, %796 : i1
    %798 = arith.select %785, %763, %717 : i1
    %799 = arith.andi %791, %782 : i1
    %800 = arith.andi %779, %786 : i1
    %801 = arith.xori %800, %true : i1 loc(#loc211)
    %802 = arith.ori %801, %781 : i1
    acir.path %800 {branch = 117 : i64, kind = "branch", txn = 0 : i64} loc(#loc211)
    %value_62, %ready_63 = acir.read(%800) {branch = 117 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc211)
    %803 = arith.ori %801, %802 : i1 loc(#loc211)
    %804 = arith.andi %799, %803 : i1 loc(#loc211)
    %value_64, %ready_65 = acir.read(%800) {branch = 117 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc212)
    %805 = arith.shrui %value_64, %c2_i32 : i32 loc(#loc212)
    %806 = arith.addi %805, %c1_i32 : i32 loc(#loc213)
    %807 = arith.extui %806 : i32 to i64 loc(#loc211)
    %808 = arith.cmpi ult, %807, %c1024_i64 : i64 loc(#loc211)
    %809 = arith.select %808, %807, %c0_i64 : i64 loc(#loc211)
    %810 = "acir.array.index"(%value_62, %809) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc211)
    %811 = arith.shli %810, %c16_i32 : i32 loc(#loc211)
    %812 = arith.andi %763, %808 : i1
    %813 = arith.ori %768, %811 : i32 loc(#loc180)
    %814 = arith.select %786, %813, %768 : i32 loc(#loc180)
    %815 = arith.select %786, %812, %763 : i1
    %816 = arith.xori %773, %true : i1
    %817 = arith.ori %816, %786 : i1
    %818 = arith.andi %773, %785 : i1
    %819 = arith.select %773, %795, %718 : !acir.struct<"src_isa_ae2f66cd.Fetched">
    %820 = arith.andi %773, %797 : i1
    %821 = arith.andi %816, %651 : i1
    %822 = arith.ori %820, %821 : i1
    %823 = arith.select %773, %798, %717 : i1
    %824 = arith.andi %775, %823 : i1
    %825 = arith.andi %822, %775 : i1
    %826 = arith.select %773, %814, %768 : i32 loc(#loc180)
    %827 = arith.select %773, %815, %763 : i1
    %828 = arith.andi %775, %827 : i1
    %829 = arith.select %817, %true, %818 : i1
    %830 = arith.andi %804, %777 : i1
    %831 = arith.andi %756, %817 : i1
    %832 = arith.xori %831, %true : i1 loc(#loc562)
    %833 = arith.ori %832, %776 : i1
    %834 = arith.ori %832, %833 : i1 loc(#loc214)
    %835 = arith.andi %826, %c3_i32 : i32 loc(#loc625)
    %836 = arith.cmpi eq, %835, %c3_i32 : i32 loc(#loc626)
    %837 = arith.andi %834, %828 : i1 loc(#loc562)
    %838 = arith.ori %832, %837 : i1 loc(#loc562)
    %839 = arith.andi %830, %838 : i1 loc(#loc562)
    %840 = arith.cmpi ne, %835, %c3_i32 : i32 loc(#loc844)
    %841 = arith.select %836, %840, %false : i1 loc(#loc562)
    %842 = arith.andi %831, %840 : i1 loc(#loc562)
    %843 = arith.xori %842, %true : i1 loc(#loc562)
    %844 = arith.ori %843, %837 : i1 loc(#loc562)
    %845 = arith.andi %826, %c65535_i32 : i32 loc(#loc627)
    %846 = arith.andi %845, %c3_i32 : i32 loc(#loc628)
    %847 = arith.shrui %845, %c13_i32 : i32 loc(#loc629)
    %848 = arith.shrui %845, %c7_i32 : i32 loc(#loc630)
    %849 = arith.andi %848, %c31_i32 : i32 loc(#loc631)
    %850 = arith.shrui %845, %c2_i32 : i32 loc(#loc632)
    %851 = arith.andi %850, %c31_i32 : i32 loc(#loc633)
    %852 = arith.andi %848, %c7_i32 : i32 loc(#loc634)
    %853 = arith.addi %852, %c8_i32 : i32 loc(#loc635)
    %854 = arith.andi %850, %c7_i32 : i32 loc(#loc636)
    %855 = arith.addi %854, %c8_i32 : i32 loc(#loc637)
    %856 = arith.shrui %845, %c12_i32 : i32 loc(#loc638)
    %857 = arith.andi %856, %c1_i32 : i32 loc(#loc638)
    %858 = arith.shli %857, %c5_i32 : i32 loc(#loc639)
    %859 = arith.ori %858, %851 : i32 loc(#loc845)
    %860 = arith.xori %859, %c32_i32 : i32 loc(#loc846)
    %861 = arith.subi %860, %c32_i32 : i32 loc(#loc640)
    %862 = arith.cmpi eq, %846, %c0_i32 : i32 loc(#loc641)
    %863 = arith.andi %844, %828 : i1 loc(#loc562)
    %864 = arith.ori %843, %863 : i1 loc(#loc562)
    %865 = arith.andi %839, %864 : i1 loc(#loc562)
    %866 = arith.cmpi ne, %846, %c0_i32 : i32 loc(#loc847)
    %867 = arith.andi %842, %866 : i1 loc(#loc562)
    %868 = arith.xori %867, %true : i1 loc(#loc562)
    %869 = arith.ori %868, %863 : i1 loc(#loc562)
    %870 = arith.cmpi eq, %846, %c1_i32 : i32 loc(#loc642)
    %871 = arith.andi %869, %828 : i1 loc(#loc562)
    %872 = arith.ori %868, %871 : i1 loc(#loc562)
    %873 = arith.andi %865, %872 : i1 loc(#loc562)
    %874 = arith.cmpi ne, %846, %c1_i32 : i32 loc(#loc848)
    %875 = arith.andi %867, %874 : i1 loc(#loc562)
    %876 = arith.xori %875, %true : i1 loc(#loc562)
    %877 = arith.ori %876, %871 : i1 loc(#loc562)
    %878 = arith.cmpi eq, %846, %c2_i32 : i32 loc(#loc643)
    %879 = arith.andi %877, %828 : i1 loc(#loc562)
    %880 = arith.ori %876, %879 : i1 loc(#loc562)
    %881 = arith.andi %873, %880 : i1 loc(#loc562)
    %882 = arith.andi %875, %878 : i1 loc(#loc562)
    %883 = arith.xori %882, %true : i1 loc(#loc562)
    %884 = arith.ori %883, %879 : i1 loc(#loc562)
    %885 = arith.cmpi eq, %847, %c0_i32 : i32 loc(#loc644)
    %886 = arith.andi %884, %828 : i1 loc(#loc562)
    %887 = arith.ori %883, %886 : i1 loc(#loc562)
    %888 = arith.andi %881, %887 : i1 loc(#loc562)
    %889 = arith.cmpi ne, %847, %c0_i32 : i32 loc(#loc849)
    %890 = arith.andi %882, %889 : i1 loc(#loc562)
    %891 = arith.xori %890, %true : i1 loc(#loc562)
    %892 = arith.ori %891, %886 : i1 loc(#loc562)
    %893 = arith.cmpi eq, %847, %c2_i32 : i32 loc(#loc645)
    %894 = arith.andi %892, %828 : i1 loc(#loc562)
    %895 = arith.ori %891, %894 : i1 loc(#loc562)
    %896 = arith.andi %888, %895 : i1 loc(#loc562)
    %897 = arith.cmpi ne, %847, %c2_i32 : i32 loc(#loc850)
    %898 = arith.andi %890, %897 : i1 loc(#loc562)
    %899 = arith.xori %898, %true : i1 loc(#loc562)
    %900 = arith.ori %899, %894 : i1 loc(#loc562)
    %901 = arith.cmpi eq, %847, %c4_i32 : i32 loc(#loc646)
    %902 = arith.andi %900, %828 : i1 loc(#loc562)
    %903 = arith.ori %899, %902 : i1 loc(#loc562)
    %904 = arith.andi %896, %903 : i1 loc(#loc562)
    %905 = arith.cmpi ne, %847, %c4_i32 : i32 loc(#loc851)
    %906 = arith.andi %898, %905 : i1 loc(#loc562)
    %907 = arith.xori %906, %true : i1 loc(#loc562)
    %908 = arith.ori %907, %902 : i1 loc(#loc562)
    %909 = arith.cmpi eq, %847, %c6_i32 : i32 loc(#loc647)
    %910 = arith.andi %908, %828 : i1 loc(#loc562)
    %911 = arith.ori %907, %910 : i1 loc(#loc562)
    %912 = arith.andi %904, %911 : i1 loc(#loc562)
    %913 = arith.shrui %845, %c9_i32 : i32 loc(#loc648)
    %914 = arith.andi %913, %c15_i32 : i32 loc(#loc648)
    %915 = arith.shli %914, %c2_i32 : i32 loc(#loc649)
    %916 = arith.andi %848, %c3_i32 : i32 loc(#loc650)
    %917 = arith.shli %916, %c6_i32 : i32 loc(#loc651)
    %918 = arith.ori %915, %917 : i32 loc(#loc652)
    %919 = arith.shrui %918, %c5_i32 : i32 loc(#loc852)
    %920 = arith.andi %919, %c127_i32 : i32 loc(#loc852)
    %921 = arith.shli %920, %c25_i32 : i32 loc(#loc853)
    %922 = arith.shli %851, %c20_i32 : i32 loc(#loc854)
    %923 = arith.ori %921, %922 : i32 loc(#loc855)
    %924 = arith.ori %923, %c65536_i32 : i32 loc(#loc855)
    %925 = arith.ori %924, %c8192_i32 : i32 loc(#loc855)
    %926 = arith.andi %918, %c31_i32 : i32 loc(#loc856)
    %927 = arith.shli %926, %c7_i32 : i32 loc(#loc857)
    %928 = arith.ori %925, %927 : i32 loc(#loc855)
    %929 = arith.ori %928, %c35_i32 : i32 loc(#loc653)
    %930 = arith.cmpi ne, %847, %c6_i32 : i32 loc(#loc858)
    %931 = arith.select %909, %929, %c0_i32 : i32 loc(#loc653)
    %932 = arith.ori %909, %930 : i1 loc(#loc562)
    %933 = arith.ori %930, %828 : i1 loc(#loc562)
    %934 = arith.andi %828, %933 : i1 loc(#loc562)
    %935 = arith.andi %932, %828 : i1 loc(#loc562)
    %936 = arith.andi %898, %901 : i1 loc(#loc562)
    %937 = arith.xori %936, %true : i1 loc(#loc562)
    %938 = arith.ori %937, %902 : i1 loc(#loc562)
    %939 = arith.andi %845, %c4096_i32 : i32 loc(#loc654)
    %940 = arith.cmpi eq, %939, %c0_i32 : i32 loc(#loc655)
    %941 = arith.andi %938, %828 : i1 loc(#loc562)
    %942 = arith.ori %937, %941 : i1 loc(#loc562)
    %943 = arith.andi %912, %942 : i1 loc(#loc562)
    %944 = arith.cmpi ne, %939, %c0_i32 : i32 loc(#loc859)
    %945 = arith.andi %936, %944 : i1 loc(#loc562)
    %946 = arith.xori %945, %true : i1 loc(#loc562)
    %947 = arith.ori %946, %941 : i1 loc(#loc562)
    %948 = arith.cmpi eq, %851, %c0_i32 : i32 loc(#loc656)
    %949 = arith.andi %947, %828 : i1 loc(#loc562)
    %950 = arith.ori %946, %949 : i1 loc(#loc562)
    %951 = arith.andi %943, %950 : i1 loc(#loc562)
    %952 = arith.shli %849, %c15_i32 : i32 loc(#loc860)
    %953 = arith.ori %922, %952 : i32 loc(#loc861)
    %954 = arith.shli %849, %c7_i32 : i32 loc(#loc862)
    %955 = arith.ori %953, %954 : i32 loc(#loc861)
    %956 = arith.ori %955, %c51_i32 : i32 loc(#loc653)
    %957 = arith.andi %945, %948 : i1 loc(#loc562)
    %958 = arith.xori %957, %true : i1 loc(#loc562)
    %959 = arith.ori %958, %949 : i1 loc(#loc562)
    %960 = arith.cmpi eq, %849, %c0_i32 : i32 loc(#loc657)
    %961 = arith.andi %959, %828 : i1 loc(#loc562)
    %962 = arith.ori %958, %961 : i1 loc(#loc562)
    %963 = arith.andi %951, %962 : i1 loc(#loc562)
    %964 = arith.ori %952, %c128_i32 : i32 loc(#loc863)
    %965 = arith.ori %964, %c103_i32 : i32 loc(#loc863)
    %966 = arith.cmpi ne, %849, %c0_i32 : i32 loc(#loc864)
    %967 = arith.select %960, %c1048691_i32, %965 : i32 loc(#loc653)
    %968 = arith.ori %960, %966 : i1 loc(#loc562)
    %969 = arith.ori %960, %828 : i1 loc(#loc562)
    %970 = arith.andi %828, %969 : i1 loc(#loc562)
    %971 = arith.andi %968, %828 : i1 loc(#loc562)
    %972 = arith.cmpi ne, %851, %c0_i32 : i32 loc(#loc865)
    %973 = arith.ori %972, %828 : i1 loc(#loc562)
    %974 = arith.andi %828, %973 : i1 loc(#loc562)
    %975 = arith.select %948, %967, %956 : i32 loc(#loc653)
    %976 = arith.andi %948, %971 : i1 loc(#loc562)
    %977 = arith.ori %976, %972 : i1 loc(#loc562)
    %978 = arith.select %948, %970, %828 : i1 loc(#loc562)
    %979 = arith.andi %828, %978 : i1 loc(#loc562)
    %980 = arith.andi %977, %828 : i1 loc(#loc562)
    %981 = arith.andi %979, %974 : i1 loc(#loc562)
    %982 = arith.andi %936, %940 : i1 loc(#loc562)
    %983 = arith.xori %982, %true : i1 loc(#loc562)
    %984 = arith.ori %983, %941 : i1 loc(#loc562)
    %985 = arith.andi %984, %828 : i1 loc(#loc562)
    %986 = arith.ori %983, %985 : i1 loc(#loc562)
    %987 = arith.andi %963, %986 : i1 loc(#loc562)
    %988 = arith.ori %922, %954 : i32 loc(#loc861)
    %989 = arith.ori %988, %c51_i32 : i32 loc(#loc653)
    %990 = arith.andi %982, %948 : i1 loc(#loc562)
    %991 = arith.xori %990, %true : i1 loc(#loc562)
    %992 = arith.ori %991, %985 : i1 loc(#loc562)
    %993 = arith.andi %992, %828 : i1 loc(#loc562)
    %994 = arith.ori %991, %993 : i1 loc(#loc562)
    %995 = arith.andi %987, %994 : i1 loc(#loc562)
    %996 = arith.ori %952, %c103_i32 : i32 loc(#loc653)
    %997 = arith.select %966, %996, %c0_i32 : i32 loc(#loc653)
    %998 = arith.select %948, %997, %989 : i32 loc(#loc653)
    %999 = arith.select %940, %998, %975 : i32 loc(#loc653)
    %1000 = arith.andi %940, %980 : i1 loc(#loc562)
    %1001 = arith.andi %944, %980 : i1 loc(#loc562)
    %1002 = arith.ori %1000, %1001 : i1 loc(#loc562)
    %1003 = arith.andi %828, %981 : i1 loc(#loc562)
    %1004 = arith.andi %1002, %828 : i1 loc(#loc562)
    %1005 = arith.andi %1003, %974 : i1 loc(#loc562)
    %1006 = arith.select %901, %974, %828 : i1 loc(#loc562)
    %1007 = arith.andi %828, %1006 : i1 loc(#loc562)
    %1008 = arith.select %901, %999, %931 : i32 loc(#loc653)
    %1009 = arith.andi %901, %1004 : i1 loc(#loc562)
    %1010 = arith.andi %905, %935 : i1 loc(#loc562)
    %1011 = arith.ori %1009, %1010 : i1 loc(#loc562)
    %1012 = arith.select %901, %1005, %934 : i1 loc(#loc562)
    %1013 = arith.andi %828, %1012 : i1 loc(#loc562)
    %1014 = arith.andi %1011, %828 : i1 loc(#loc562)
    %1015 = arith.andi %1013, %1007 : i1 loc(#loc562)
    %1016 = arith.andi %890, %893 : i1 loc(#loc562)
    %1017 = arith.xori %1016, %true : i1 loc(#loc562)
    %1018 = arith.ori %1017, %894 : i1 loc(#loc562)
    %1019 = arith.andi %1018, %828 : i1 loc(#loc562)
    %1020 = arith.ori %1017, %1019 : i1 loc(#loc562)
    %1021 = arith.andi %995, %1020 : i1 loc(#loc562)
    %1022 = arith.shrui %845, %c4_i32 : i32 loc(#loc659)
    %1023 = arith.andi %1022, %c7_i32 : i32 loc(#loc659)
    %1024 = arith.shli %1023, %c2_i32 : i32 loc(#loc660)
    %1025 = arith.ori %858, %1024 : i32 loc(#loc661)
    %1026 = arith.andi %850, %c3_i32 : i32 loc(#loc662)
    %1027 = arith.shli %1026, %c6_i32 : i32 loc(#loc663)
    %1028 = arith.ori %1025, %1027 : i32 loc(#loc652)
    %1029 = arith.andi %1028, %c4095_i32 : i32 loc(#loc866)
    %1030 = arith.shli %1029, %c20_i32 : i32 loc(#loc867)
    %1031 = arith.ori %1030, %c65536_i32 : i32 loc(#loc868)
    %1032 = arith.ori %1031, %c8192_i32 : i32 loc(#loc868)
    %1033 = arith.ori %1032, %954 : i32 loc(#loc868)
    %1034 = arith.ori %1033, %c3_i32 : i32 loc(#loc653)
    %1035 = arith.select %966, %1034, %c0_i32 : i32 loc(#loc653)
    %1036 = arith.select %893, %828, %1007 : i1 loc(#loc562)
    %1037 = arith.andi %828, %1036 : i1 loc(#loc562)
    %1038 = arith.select %893, %1035, %1008 : i32 loc(#loc653)
    %1039 = arith.andi %893, %971 : i1 loc(#loc562)
    %1040 = arith.andi %897, %1014 : i1 loc(#loc562)
    %1041 = arith.ori %1039, %1040 : i1 loc(#loc562)
    %1042 = arith.select %893, %970, %1015 : i1 loc(#loc562)
    %1043 = arith.andi %828, %1042 : i1 loc(#loc562)
    %1044 = arith.andi %1041, %828 : i1 loc(#loc562)
    %1045 = arith.andi %1043, %1037 : i1 loc(#loc562)
    %1046 = arith.andi %882, %885 : i1 loc(#loc562)
    %1047 = arith.xori %1046, %true : i1 loc(#loc562)
    %1048 = arith.ori %1047, %886 : i1 loc(#loc562)
    %1049 = arith.andi %1048, %828 : i1 loc(#loc562)
    %1050 = arith.ori %1047, %1049 : i1 loc(#loc562)
    %1051 = arith.andi %1021, %1050 : i1 loc(#loc562)
    %1052 = arith.andi %851, %c4095_i32 : i32 loc(#loc866)
    %1053 = arith.shli %1052, %c20_i32 : i32 loc(#loc867)
    %1054 = arith.ori %1053, %952 : i32 loc(#loc868)
    %1055 = arith.ori %1054, %c4096_i32 : i32 loc(#loc868)
    %1056 = arith.ori %1055, %954 : i32 loc(#loc868)
    %1057 = arith.ori %1056, %c19_i32 : i32 loc(#loc653)
    %1058 = arith.select %940, %1057, %c0_i32 : i32 loc(#loc653)
    %1059 = arith.ori %940, %944 : i1 loc(#loc562)
    %1060 = arith.ori %944, %828 : i1 loc(#loc562)
    %1061 = arith.andi %828, %1060 : i1 loc(#loc562)
    %1062 = arith.andi %1059, %828 : i1 loc(#loc562)
    %1063 = arith.select %885, %828, %1037 : i1 loc(#loc562)
    %1064 = arith.andi %828, %1063 : i1 loc(#loc562)
    %1065 = arith.select %885, %1058, %1038 : i32 loc(#loc653)
    %1066 = arith.andi %885, %1062 : i1 loc(#loc562)
    %1067 = arith.andi %889, %1044 : i1 loc(#loc562)
    %1068 = arith.ori %1066, %1067 : i1 loc(#loc562)
    %1069 = arith.select %885, %1061, %1045 : i1 loc(#loc562)
    %1070 = arith.andi %828, %1069 : i1 loc(#loc562)
    %1071 = arith.andi %1068, %828 : i1 loc(#loc562)
    %1072 = arith.andi %1070, %1064 : i1 loc(#loc562)
    %1073 = arith.cmpi ne, %846, %c2_i32 : i32 loc(#loc869)
    %1074 = arith.ori %1073, %1064 : i1 loc(#loc562)
    %1075 = arith.andi %828, %1074 : i1 loc(#loc562)
    %1076 = arith.select %878, %1065, %c0_i32 : i32 loc(#loc653)
    %1077 = arith.andi %878, %1071 : i1 loc(#loc562)
    %1078 = arith.ori %1077, %1073 : i1 loc(#loc562)
    %1079 = arith.ori %1073, %1072 : i1 loc(#loc562)
    %1080 = arith.andi %828, %1079 : i1 loc(#loc562)
    %1081 = arith.andi %1078, %828 : i1 loc(#loc562)
    %1082 = arith.andi %1080, %1075 : i1 loc(#loc562)
    %1083 = arith.andi %867, %870 : i1 loc(#loc562)
    %1084 = arith.xori %1083, %true : i1 loc(#loc562)
    %1085 = arith.ori %1084, %871 : i1 loc(#loc562)
    %1086 = arith.andi %1085, %828 : i1 loc(#loc562)
    %1087 = arith.ori %1084, %1086 : i1 loc(#loc562)
    %1088 = arith.andi %1051, %1087 : i1 loc(#loc562)
    %1089 = arith.andi %1083, %889 : i1 loc(#loc562)
    %1090 = arith.xori %1089, %true : i1 loc(#loc562)
    %1091 = arith.ori %1090, %1086 : i1 loc(#loc562)
    %1092 = arith.cmpi eq, %847, %c1_i32 : i32 loc(#loc664)
    %1093 = arith.andi %1091, %828 : i1 loc(#loc562)
    %1094 = arith.ori %1090, %1093 : i1 loc(#loc562)
    %1095 = arith.andi %1088, %1094 : i1 loc(#loc562)
    %1096 = arith.cmpi eq, %847, %c5_i32 : i32 loc(#loc665)
    %1097 = arith.ori %1092, %1096 : i1 loc(#loc562)
    %1098 = arith.ori %1092, %828 : i1 loc(#loc562)
    %1099 = arith.andi %828, %1098 : i1 loc(#loc562)
    %1100 = arith.andi %1091, %1099 : i1 loc(#loc562)
    %1101 = arith.ori %1090, %1100 : i1 loc(#loc562)
    %1102 = arith.andi %1095, %1101 : i1 loc(#loc562)
    %1103 = arith.xori %1097, %true : i1 loc(#loc562)
    %1104 = arith.andi %1089, %1103 : i1 loc(#loc562)
    %1105 = arith.xori %1104, %true : i1 loc(#loc562)
    %1106 = arith.ori %1105, %1100 : i1 loc(#loc562)
    %1107 = arith.andi %1106, %828 : i1 loc(#loc562)
    %1108 = arith.ori %1105, %1107 : i1 loc(#loc562)
    %1109 = arith.andi %1102, %1108 : i1 loc(#loc562)
    %1110 = arith.andi %1104, %897 : i1 loc(#loc562)
    %1111 = arith.xori %1110, %true : i1 loc(#loc562)
    %1112 = arith.ori %1111, %1107 : i1 loc(#loc562)
    %1113 = arith.cmpi eq, %847, %c3_i32 : i32 loc(#loc666)
    %1114 = arith.andi %1112, %828 : i1 loc(#loc562)
    %1115 = arith.ori %1111, %1114 : i1 loc(#loc562)
    %1116 = arith.andi %1109, %1115 : i1 loc(#loc562)
    %1117 = arith.cmpi ne, %847, %c3_i32 : i32 loc(#loc870)
    %1118 = arith.andi %1110, %1117 : i1 loc(#loc562)
    %1119 = arith.xori %1118, %true : i1 loc(#loc562)
    %1120 = arith.ori %1119, %1114 : i1 loc(#loc562)
    %1121 = arith.andi %1120, %828 : i1 loc(#loc562)
    %1122 = arith.ori %1119, %1121 : i1 loc(#loc562)
    %1123 = arith.andi %1116, %1122 : i1 loc(#loc562)
    %1124 = arith.andi %1118, %905 : i1 loc(#loc562)
    %1125 = arith.xori %1124, %true : i1 loc(#loc562)
    %1126 = arith.ori %1125, %1121 : i1 loc(#loc562)
    %1127 = arith.andi %1126, %828 : i1 loc(#loc562)
    %1128 = arith.ori %1125, %1127 : i1 loc(#loc562)
    %1129 = arith.andi %1123, %1128 : i1 loc(#loc562)
    %1130 = arith.cmpi eq, %847, %c7_i32 : i32 loc(#loc667)
    %1131 = arith.ori %909, %1130 : i1 loc(#loc562)
    %1132 = arith.ori %909, %828 : i1 loc(#loc562)
    %1133 = arith.andi %828, %1132 : i1 loc(#loc562)
    %1134 = arith.andi %1126, %1133 : i1 loc(#loc562)
    %1135 = arith.ori %1125, %1134 : i1 loc(#loc562)
    %1136 = arith.andi %1129, %1135 : i1 loc(#loc562)
    %1137 = arith.shli %857, %c8_i32 : i32 loc(#loc668)
    %1138 = arith.shrui %845, %c10_i32 : i32 loc(#loc669)
    %1139 = arith.andi %1138, %c3_i32 : i32 loc(#loc669)
    %1140 = arith.shli %1139, %c3_i32 : i32 loc(#loc670)
    %1141 = arith.ori %1137, %1140 : i32 loc(#loc671)
    %1142 = arith.shrui %845, %c5_i32 : i32 loc(#loc672)
    %1143 = arith.andi %1142, %c3_i32 : i32 loc(#loc672)
    %1144 = arith.shli %1143, %c6_i32 : i32 loc(#loc673)
    %1145 = arith.ori %1141, %1144 : i32 loc(#loc671)
    %1146 = arith.shrui %845, %c3_i32 : i32 loc(#loc674)
    %1147 = arith.andi %1146, %c3_i32 : i32 loc(#loc674)
    %1148 = arith.shli %1147, %c1_i32 : i32 loc(#loc675)
    %1149 = arith.ori %1145, %1148 : i32 loc(#loc671)
    %1150 = arith.andi %850, %c1_i32 : i32 loc(#loc676)
    %1151 = arith.shli %1150, %c5_i32 : i32 loc(#loc677)
    %1152 = arith.ori %1149, %1151 : i32 loc(#loc652)
    %1153 = arith.extui %930 : i1 to i32 loc(#loc871)
    %1154 = arith.xori %1152, %c256_i32 : i32 loc(#loc872)
    %1155 = arith.subi %1154, %c256_i32 : i32 loc(#loc873)
    %1156 = arith.shrui %1155, %c12_i32 : i32 loc(#loc874)
    %1157 = arith.andi %1156, %c1_i32 : i32 loc(#loc874)
    %1158 = arith.shli %1157, %c31_i32 : i32 loc(#loc875)
    %1159 = arith.shrui %1155, %c5_i32 : i32 loc(#loc876)
    %1160 = arith.andi %1159, %c63_i32 : i32 loc(#loc876)
    %1161 = arith.shli %1160, %c25_i32 : i32 loc(#loc877)
    %1162 = arith.ori %1158, %1161 : i32 loc(#loc878)
    %1163 = arith.shli %853, %c15_i32 : i32 loc(#loc879)
    %1164 = arith.ori %1162, %1163 : i32 loc(#loc878)
    %1165 = arith.shli %1153, %c12_i32 : i32 loc(#loc880)
    %1166 = arith.ori %1164, %1165 : i32 loc(#loc878)
    %1167 = arith.shrui %1155, %c1_i32 : i32 loc(#loc881)
    %1168 = arith.andi %1167, %c15_i32 : i32 loc(#loc881)
    %1169 = arith.shli %1168, %c8_i32 : i32 loc(#loc882)
    %1170 = arith.ori %1166, %1169 : i32 loc(#loc878)
    %1171 = arith.shrui %1155, %c11_i32 : i32 loc(#loc883)
    %1172 = arith.andi %1171, %c1_i32 : i32 loc(#loc883)
    %1173 = arith.shli %1172, %c7_i32 : i32 loc(#loc884)
    %1174 = arith.ori %1170, %1173 : i32 loc(#loc878)
    %1175 = arith.ori %1174, %c99_i32 : i32 loc(#loc653)
    %1176 = arith.select %1131, %1175, %c0_i32 : i32 loc(#loc653)
    %1177 = arith.xori %1131, %true : i1 loc(#loc562)
    %1178 = arith.ori %1177, %828 : i1 loc(#loc562)
    %1179 = arith.andi %1133, %1178 : i1 loc(#loc562)
    %1180 = arith.andi %1118, %901 : i1 loc(#loc562)
    %1181 = arith.xori %1180, %true : i1 loc(#loc562)
    %1182 = arith.ori %1181, %1121 : i1 loc(#loc562)
    %1183 = arith.cmpi eq, %1139, %c0_i32 : i32 loc(#loc679)
    %1184 = arith.andi %1182, %828 : i1 loc(#loc562)
    %1185 = arith.ori %1181, %1184 : i1 loc(#loc562)
    %1186 = arith.andi %1136, %1185 : i1 loc(#loc562)
    %1187 = arith.cmpi eq, %1139, %c1_i32 : i32 loc(#loc680)
    %1188 = arith.ori %1183, %1187 : i1 loc(#loc562)
    %1189 = arith.ori %1183, %828 : i1 loc(#loc562)
    %1190 = arith.andi %828, %1189 : i1 loc(#loc562)
    %1191 = arith.andi %1182, %1190 : i1 loc(#loc562)
    %1192 = arith.ori %1181, %1191 : i1 loc(#loc562)
    %1193 = arith.andi %1186, %1192 : i1 loc(#loc562)
    %1194 = arith.xori %1188, %true : i1 loc(#loc562)
    %1195 = arith.andi %1180, %1194 : i1 loc(#loc562)
    %1196 = arith.xori %1195, %true : i1 loc(#loc562)
    %1197 = arith.ori %1196, %1191 : i1 loc(#loc562)
    %1198 = arith.cmpi eq, %1139, %c2_i32 : i32 loc(#loc681)
    %1199 = arith.andi %1197, %828 : i1 loc(#loc562)
    %1200 = arith.ori %1196, %1199 : i1 loc(#loc562)
    %1201 = arith.andi %1193, %1200 : i1 loc(#loc562)
    %1202 = arith.cmpi ne, %1139, %c2_i32 : i32 loc(#loc885)
    %1203 = arith.andi %1195, %1202 : i1 loc(#loc562)
    %1204 = arith.xori %1203, %true : i1 loc(#loc562)
    %1205 = arith.ori %1204, %1199 : i1 loc(#loc562)
    %1206 = arith.andi %1205, %828 : i1 loc(#loc562)
    %1207 = arith.ori %1204, %1206 : i1 loc(#loc562)
    %1208 = arith.andi %1201, %1207 : i1 loc(#loc562)
    %1209 = arith.andi %1203, %940 : i1 loc(#loc562)
    %1210 = arith.xori %1209, %true : i1 loc(#loc562)
    %1211 = arith.ori %1210, %1206 : i1 loc(#loc562)
    %1212 = arith.cmpi eq, %1143, %c0_i32 : i32 loc(#loc682)
    %1213 = arith.andi %1211, %828 : i1 loc(#loc562)
    %1214 = arith.ori %1210, %1213 : i1 loc(#loc562)
    %1215 = arith.andi %1208, %1214 : i1 loc(#loc562)
    %1216 = arith.cmpi ne, %1143, %c0_i32 : i32 loc(#loc886)
    %1217 = arith.andi %1209, %1216 : i1 loc(#loc562)
    %1218 = arith.xori %1217, %true : i1 loc(#loc562)
    %1219 = arith.ori %1218, %1213 : i1 loc(#loc562)
    %1220 = arith.cmpi eq, %1143, %c1_i32 : i32 loc(#loc683)
    %1221 = arith.andi %1219, %828 : i1 loc(#loc562)
    %1222 = arith.ori %1218, %1221 : i1 loc(#loc562)
    %1223 = arith.andi %1215, %1222 : i1 loc(#loc562)
    %1224 = arith.cmpi eq, %1143, %c2_i32 : i32 loc(#loc684)
    %1225 = arith.select %1224, %c6_i32, %c7_i32 : i32 loc(#loc562)
    %1226 = arith.select %1220, %c4_i32, %1225 : i32 loc(#loc562)
    %1227 = arith.ori %1220, %828 : i1 loc(#loc562)
    %1228 = arith.andi %828, %1227 : i1 loc(#loc562)
    %1229 = arith.ori %1212, %828 : i1 loc(#loc562)
    %1230 = arith.andi %828, %1229 : i1 loc(#loc562)
    %1231 = arith.select %1212, %c0_i32, %1226 : i32 loc(#loc685)
    %1232 = arith.ori %1212, %1228 : i1 loc(#loc562)
    %1233 = arith.andi %828, %1232 : i1 loc(#loc562)
    %1234 = arith.select %1212, %c32_i32, %c0_i32 : i32 loc(#loc887)
    %1235 = arith.shli %1234, %c25_i32 : i32 loc(#loc888)
    %1236 = arith.shli %855, %c20_i32 : i32 loc(#loc889)
    %1237 = arith.ori %1235, %1236 : i32 loc(#loc861)
    %1238 = arith.ori %1237, %1163 : i32 loc(#loc861)
    %1239 = arith.shli %1231, %c12_i32 : i32 loc(#loc890)
    %1240 = arith.ori %1238, %1239 : i32 loc(#loc861)
    %1241 = arith.shli %853, %c7_i32 : i32 loc(#loc862)
    %1242 = arith.ori %1240, %1241 : i32 loc(#loc861)
    %1243 = arith.ori %1242, %c51_i32 : i32 loc(#loc653)
    %1244 = arith.andi %1233, %1230 : i1 loc(#loc562)
    %1245 = arith.ori %944, %1230 : i1 loc(#loc562)
    %1246 = arith.andi %828, %1245 : i1 loc(#loc562)
    %1247 = arith.select %940, %1243, %c0_i32 : i32 loc(#loc653)
    %1248 = arith.ori %944, %1244 : i1 loc(#loc562)
    %1249 = arith.andi %828, %1248 : i1 loc(#loc562)
    %1250 = arith.andi %1249, %1246 : i1 loc(#loc562)
    %1251 = arith.andi %861, %c4095_i32 : i32 loc(#loc866)
    %1252 = arith.shli %1251, %c20_i32 : i32 loc(#loc867)
    %1253 = arith.ori %1252, %1163 : i32 loc(#loc868)
    %1254 = arith.ori %1253, %c28672_i32 : i32 loc(#loc868)
    %1255 = arith.ori %1254, %1241 : i32 loc(#loc868)
    %1256 = arith.ori %1255, %c19_i32 : i32 loc(#loc653)
    %1257 = arith.ori %1198, %1246 : i1 loc(#loc562)
    %1258 = arith.andi %828, %1257 : i1 loc(#loc562)
    %1259 = arith.select %1198, %1256, %1247 : i32 loc(#loc653)
    %1260 = arith.andi %1202, %1062 : i1 loc(#loc562)
    %1261 = arith.ori %1198, %1260 : i1 loc(#loc562)
    %1262 = arith.select %1198, %828, %1250 : i1 loc(#loc562)
    %1263 = arith.andi %828, %1262 : i1 loc(#loc562)
    %1264 = arith.andi %1261, %828 : i1 loc(#loc562)
    %1265 = arith.andi %1263, %1258 : i1 loc(#loc562)
    %1266 = arith.andi %1180, %1188 : i1 loc(#loc562)
    %1267 = arith.xori %1266, %true : i1 loc(#loc562)
    %1268 = arith.ori %1267, %1191 : i1 loc(#loc562)
    %1269 = arith.andi %1268, %828 : i1 loc(#loc562)
    %1270 = arith.ori %1267, %1269 : i1 loc(#loc562)
    %1271 = arith.andi %1223, %1270 : i1 loc(#loc562)
    %1272 = arith.select %1187, %c1024_i32, %c0_i32 : i32 loc(#loc562)
    %1273 = arith.ori %851, %1272 : i32 loc(#loc891)
    %1274 = arith.andi %1273, %c4095_i32 : i32 loc(#loc866)
    %1275 = arith.shli %1274, %c20_i32 : i32 loc(#loc867)
    %1276 = arith.ori %1275, %1163 : i32 loc(#loc868)
    %1277 = arith.ori %1276, %c20480_i32 : i32 loc(#loc868)
    %1278 = arith.ori %1277, %1241 : i32 loc(#loc868)
    %1279 = arith.ori %1278, %c19_i32 : i32 loc(#loc653)
    %1280 = arith.select %940, %1279, %c0_i32 : i32 loc(#loc653)
    %1281 = arith.select %1188, %828, %1258 : i1 loc(#loc562)
    %1282 = arith.andi %1190, %1281 : i1 loc(#loc562)
    %1283 = arith.select %1188, %1280, %1259 : i32 loc(#loc653)
    %1284 = arith.andi %1188, %1062 : i1 loc(#loc562)
    %1285 = arith.andi %1194, %1264 : i1 loc(#loc562)
    %1286 = arith.ori %1284, %1285 : i1 loc(#loc562)
    %1287 = arith.select %1188, %1061, %1265 : i1 loc(#loc562)
    %1288 = arith.andi %1190, %1287 : i1 loc(#loc562)
    %1289 = arith.andi %1286, %1190 : i1 loc(#loc562)
    %1290 = arith.andi %1288, %1282 : i1 loc(#loc562)
    %1291 = arith.select %901, %1282, %1133 : i1 loc(#loc562)
    %1292 = arith.andi %828, %1291 : i1 loc(#loc562)
    %1293 = arith.select %901, %1283, %1176 : i32 loc(#loc653)
    %1294 = arith.andi %901, %1289 : i1 loc(#loc562)
    %1295 = arith.andi %905, %1133 : i1 loc(#loc562)
    %1296 = arith.ori %1294, %1295 : i1 loc(#loc562)
    %1297 = arith.select %901, %1290, %1179 : i1 loc(#loc562)
    %1298 = arith.andi %828, %1297 : i1 loc(#loc562)
    %1299 = arith.andi %1296, %828 : i1 loc(#loc562)
    %1300 = arith.andi %1298, %1292 : i1 loc(#loc562)
    %1301 = arith.andi %1110, %1113 : i1 loc(#loc562)
    %1302 = arith.xori %1301, %true : i1 loc(#loc562)
    %1303 = arith.ori %1302, %1114 : i1 loc(#loc562)
    %1304 = arith.cmpi eq, %849, %c2_i32 : i32 loc(#loc686)
    %1305 = arith.andi %1303, %828 : i1 loc(#loc562)
    %1306 = arith.ori %1302, %1305 : i1 loc(#loc562)
    %1307 = arith.andi %1271, %1306 : i1 loc(#loc562)
    %1308 = arith.cmpi ne, %849, %c2_i32 : i32 loc(#loc892)
    %1309 = arith.andi %1301, %1308 : i1 loc(#loc562)
    %1310 = arith.xori %1309, %true : i1 loc(#loc562)
    %1311 = arith.ori %1310, %1305 : i1 loc(#loc562)
    %1312 = arith.cmpi ne, %861, %c0_i32 : i32 loc(#loc687)
    %1313 = arith.andi %1311, %828 : i1 loc(#loc562)
    %1314 = arith.ori %1310, %1313 : i1 loc(#loc562)
    %1315 = arith.andi %1307, %1314 : i1 loc(#loc562)
    %1316 = arith.shli %861, %c12_i32 : i32 loc(#loc688)
    %1317 = arith.ori %1316, %954 : i32 loc(#loc689)
    %1318 = arith.ori %1317, %c55_i32 : i32 loc(#loc653)
    %1319 = arith.cmpi eq, %861, %c0_i32 : i32 loc(#loc893)
    %1320 = arith.select %1312, %1318, %c0_i32 : i32 loc(#loc653)
    %1321 = arith.ori %1312, %1319 : i1 loc(#loc562)
    %1322 = arith.ori %1319, %828 : i1 loc(#loc562)
    %1323 = arith.andi %828, %1322 : i1 loc(#loc562)
    %1324 = arith.andi %1321, %828 : i1 loc(#loc562)
    %1325 = arith.andi %1301, %1304 : i1 loc(#loc562)
    %1326 = arith.xori %1325, %true : i1 loc(#loc562)
    %1327 = arith.ori %1326, %1305 : i1 loc(#loc562)
    %1328 = arith.shli %857, %c9_i32 : i32 loc(#loc690)
    %1329 = arith.shrui %845, %c6_i32 : i32 loc(#loc691)
    %1330 = arith.andi %1329, %c1_i32 : i32 loc(#loc691)
    %1331 = arith.shli %1330, %c4_i32 : i32 loc(#loc692)
    %1332 = arith.ori %1328, %1331 : i32 loc(#loc693)
    %1333 = arith.andi %1142, %c1_i32 : i32 loc(#loc694)
    %1334 = arith.shli %1333, %c6_i32 : i32 loc(#loc695)
    %1335 = arith.ori %1332, %1334 : i32 loc(#loc693)
    %1336 = arith.shli %1147, %c7_i32 : i32 loc(#loc696)
    %1337 = arith.ori %1335, %1336 : i32 loc(#loc693)
    %1338 = arith.ori %1337, %1151 : i32 loc(#loc652)
    %1339 = arith.cmpi ne, %1338, %c0_i32 : i32 loc(#loc697)
    %1340 = arith.andi %1327, %828 : i1 loc(#loc562)
    %1341 = arith.ori %1326, %1340 : i1 loc(#loc562)
    %1342 = arith.andi %1315, %1341 : i1 loc(#loc562)
    %1343 = arith.xori %1338, %c512_i32 : i32 loc(#loc894)
    %1344 = arith.subi %1343, %c512_i32 : i32 loc(#loc891)
    %1345 = arith.andi %1344, %c4095_i32 : i32 loc(#loc866)
    %1346 = arith.shli %1345, %c20_i32 : i32 loc(#loc867)
    %1347 = arith.ori %1346, %c65536_i32 : i32 loc(#loc868)
    %1348 = arith.ori %1347, %c256_i32 : i32 loc(#loc868)
    %1349 = arith.ori %1348, %c19_i32 : i32 loc(#loc653)
    %1350 = arith.cmpi eq, %1338, %c0_i32 : i32 loc(#loc895)
    %1351 = arith.select %1339, %1349, %c0_i32 : i32 loc(#loc653)
    %1352 = arith.ori %1339, %1350 : i1 loc(#loc562)
    %1353 = arith.ori %1350, %828 : i1 loc(#loc562)
    %1354 = arith.andi %828, %1353 : i1 loc(#loc562)
    %1355 = arith.andi %1352, %828 : i1 loc(#loc562)
    %1356 = arith.select %1304, %1351, %1320 : i32 loc(#loc653)
    %1357 = arith.andi %1304, %1355 : i1 loc(#loc562)
    %1358 = arith.andi %1308, %1324 : i1 loc(#loc562)
    %1359 = arith.ori %1357, %1358 : i1 loc(#loc562)
    %1360 = arith.select %1304, %1354, %1323 : i1 loc(#loc562)
    %1361 = arith.andi %828, %1360 : i1 loc(#loc562)
    %1362 = arith.andi %1359, %828 : i1 loc(#loc562)
    %1363 = arith.select %1113, %828, %1292 : i1 loc(#loc562)
    %1364 = arith.andi %828, %1363 : i1 loc(#loc562)
    %1365 = arith.select %1113, %1356, %1293 : i32 loc(#loc653)
    %1366 = arith.andi %1113, %1362 : i1 loc(#loc562)
    %1367 = arith.andi %1117, %1299 : i1 loc(#loc562)
    %1368 = arith.ori %1366, %1367 : i1 loc(#loc562)
    %1369 = arith.select %1113, %1361, %1300 : i1 loc(#loc562)
    %1370 = arith.andi %828, %1369 : i1 loc(#loc562)
    %1371 = arith.andi %1368, %828 : i1 loc(#loc562)
    %1372 = arith.andi %1370, %1364 : i1 loc(#loc562)
    %1373 = arith.ori %1252, %954 : i32 loc(#loc868)
    %1374 = arith.ori %1373, %c19_i32 : i32 loc(#loc653)
    %1375 = arith.ori %893, %1364 : i1 loc(#loc562)
    %1376 = arith.andi %828, %1375 : i1 loc(#loc562)
    %1377 = arith.select %893, %1374, %1365 : i32 loc(#loc653)
    %1378 = arith.andi %897, %1371 : i1 loc(#loc562)
    %1379 = arith.ori %893, %1378 : i1 loc(#loc562)
    %1380 = arith.select %893, %828, %1372 : i1 loc(#loc562)
    %1381 = arith.andi %828, %1380 : i1 loc(#loc562)
    %1382 = arith.andi %1379, %828 : i1 loc(#loc562)
    %1383 = arith.andi %1381, %1376 : i1 loc(#loc562)
    %1384 = arith.shli %857, %c11_i32 : i32 loc(#loc699)
    %1385 = arith.shrui %845, %c11_i32 : i32 loc(#loc700)
    %1386 = arith.andi %1385, %c1_i32 : i32 loc(#loc700)
    %1387 = arith.shli %1386, %c4_i32 : i32 loc(#loc701)
    %1388 = arith.ori %1384, %1387 : i32 loc(#loc702)
    %1389 = arith.andi %913, %c3_i32 : i32 loc(#loc703)
    %1390 = arith.shli %1389, %c8_i32 : i32 loc(#loc704)
    %1391 = arith.ori %1388, %1390 : i32 loc(#loc702)
    %1392 = arith.shrui %845, %c8_i32 : i32 loc(#loc705)
    %1393 = arith.andi %1392, %c1_i32 : i32 loc(#loc705)
    %1394 = arith.shli %1393, %c10_i32 : i32 loc(#loc706)
    %1395 = arith.ori %1391, %1394 : i32 loc(#loc702)
    %1396 = arith.andi %848, %c1_i32 : i32 loc(#loc707)
    %1397 = arith.shli %1396, %c6_i32 : i32 loc(#loc708)
    %1398 = arith.ori %1395, %1397 : i32 loc(#loc702)
    %1399 = arith.shli %1330, %c7_i32 : i32 loc(#loc709)
    %1400 = arith.ori %1398, %1399 : i32 loc(#loc702)
    %1401 = arith.andi %1146, %c7_i32 : i32 loc(#loc710)
    %1402 = arith.shli %1401, %c1_i32 : i32 loc(#loc711)
    %1403 = arith.ori %1400, %1402 : i32 loc(#loc702)
    %1404 = arith.ori %1403, %1151 : i32 loc(#loc652)
    %1405 = arith.extui %1092 : i1 to i32 loc(#loc896)
    %1406 = arith.xori %1404, %c2048_i32 : i32 loc(#loc897)
    %1407 = arith.subi %1406, %c2048_i32 : i32 loc(#loc898)
    %1408 = arith.shrui %1407, %c20_i32 : i32 loc(#loc899)
    %1409 = arith.andi %1408, %c1_i32 : i32 loc(#loc899)
    %1410 = arith.shli %1409, %c31_i32 : i32 loc(#loc900)
    %1411 = arith.shrui %1407, %c1_i32 : i32 loc(#loc901)
    %1412 = arith.andi %1411, %c1023_i32 : i32 loc(#loc901)
    %1413 = arith.shli %1412, %c21_i32 : i32 loc(#loc902)
    %1414 = arith.ori %1410, %1413 : i32 loc(#loc903)
    %1415 = arith.shrui %1407, %c11_i32 : i32 loc(#loc904)
    %1416 = arith.andi %1415, %c1_i32 : i32 loc(#loc904)
    %1417 = arith.shli %1416, %c20_i32 : i32 loc(#loc905)
    %1418 = arith.ori %1414, %1417 : i32 loc(#loc903)
    %1419 = arith.andi %1407, %c1044480_i32 : i32 loc(#loc906)
    %1420 = arith.ori %1418, %1419 : i32 loc(#loc903)
    %1421 = arith.shli %1405, %c7_i32 : i32 loc(#loc907)
    %1422 = arith.ori %1420, %1421 : i32 loc(#loc903)
    %1423 = arith.ori %1422, %c111_i32 : i32 loc(#loc653)
    %1424 = arith.ori %1097, %1376 : i1 loc(#loc562)
    %1425 = arith.andi %1099, %1424 : i1 loc(#loc562)
    %1426 = arith.select %1097, %1423, %1377 : i32 loc(#loc653)
    %1427 = arith.andi %1103, %1382 : i1 loc(#loc562)
    %1428 = arith.ori %1097, %1427 : i1 loc(#loc562)
    %1429 = arith.select %1097, %828, %1383 : i1 loc(#loc562)
    %1430 = arith.andi %1099, %1429 : i1 loc(#loc562)
    %1431 = arith.andi %1428, %1099 : i1 loc(#loc562)
    %1432 = arith.andi %1430, %1425 : i1 loc(#loc562)
    %1433 = arith.ori %1252, %952 : i32 loc(#loc868)
    %1434 = arith.ori %1433, %954 : i32 loc(#loc868)
    %1435 = arith.ori %1434, %c19_i32 : i32 loc(#loc653)
    %1436 = arith.ori %885, %1425 : i1 loc(#loc562)
    %1437 = arith.andi %828, %1436 : i1 loc(#loc562)
    %1438 = arith.select %885, %1435, %1426 : i32 loc(#loc653)
    %1439 = arith.andi %889, %1431 : i1 loc(#loc562)
    %1440 = arith.ori %885, %1439 : i1 loc(#loc562)
    %1441 = arith.select %885, %828, %1432 : i1 loc(#loc562)
    %1442 = arith.andi %828, %1441 : i1 loc(#loc562)
    %1443 = arith.andi %1440, %828 : i1 loc(#loc562)
    %1444 = arith.andi %1442, %1437 : i1 loc(#loc562)
    %1445 = arith.select %870, %1437, %1075 : i1 loc(#loc562)
    %1446 = arith.andi %828, %1445 : i1 loc(#loc562)
    %1447 = arith.select %870, %1438, %1076 : i32 loc(#loc653)
    %1448 = arith.andi %870, %1443 : i1 loc(#loc562)
    %1449 = arith.andi %874, %1081 : i1 loc(#loc562)
    %1450 = arith.ori %1448, %1449 : i1 loc(#loc562)
    %1451 = arith.select %870, %1444, %1082 : i1 loc(#loc562)
    %1452 = arith.andi %828, %1451 : i1 loc(#loc562)
    %1453 = arith.andi %1450, %828 : i1 loc(#loc562)
    %1454 = arith.andi %1452, %1446 : i1 loc(#loc562)
    %1455 = arith.andi %842, %862 : i1 loc(#loc562)
    %1456 = arith.xori %1455, %true : i1 loc(#loc562)
    %1457 = arith.ori %1456, %863 : i1 loc(#loc562)
    %1458 = arith.andi %1457, %828 : i1 loc(#loc562)
    %1459 = arith.ori %1456, %1458 : i1 loc(#loc562)
    %1460 = arith.andi %1342, %1459 : i1 loc(#loc562)
    %1461 = arith.andi %1455, %889 : i1 loc(#loc562)
    %1462 = arith.xori %1461, %true : i1 loc(#loc562)
    %1463 = arith.ori %1462, %1458 : i1 loc(#loc562)
    %1464 = arith.andi %1463, %828 : i1 loc(#loc562)
    %1465 = arith.ori %1462, %1464 : i1 loc(#loc562)
    %1466 = arith.andi %1460, %1465 : i1 loc(#loc562)
    %1467 = arith.ori %893, %909 : i1 loc(#loc562)
    %1468 = arith.ori %893, %828 : i1 loc(#loc562)
    %1469 = arith.andi %828, %1468 : i1 loc(#loc562)
    %1470 = arith.andi %1463, %1469 : i1 loc(#loc562)
    %1471 = arith.ori %1462, %1470 : i1 loc(#loc562)
    %1472 = arith.andi %1466, %1471 : i1 loc(#loc562)
    %1473 = arith.andi %1461, %1467 : i1 loc(#loc562)
    %1474 = arith.xori %1473, %true : i1 loc(#loc562)
    %1475 = arith.ori %1474, %1470 : i1 loc(#loc562)
    %1476 = arith.andi %1138, %c7_i32 : i32 loc(#loc713)
    %1477 = arith.shli %1476, %c3_i32 : i32 loc(#loc714)
    %1478 = arith.ori %1334, %1477 : i32 loc(#loc715)
    %1479 = arith.shli %1330, %c2_i32 : i32 loc(#loc716)
    %1480 = arith.ori %1478, %1479 : i32 loc(#loc652)
    %1481 = arith.andi %1475, %828 : i1 loc(#loc562)
    %1482 = arith.ori %1474, %1481 : i1 loc(#loc562)
    %1483 = arith.andi %1472, %1482 : i1 loc(#loc562)
    %1484 = arith.shrui %1480, %c5_i32 : i32 loc(#loc908)
    %1485 = arith.andi %1484, %c127_i32 : i32 loc(#loc908)
    %1486 = arith.shli %1485, %c25_i32 : i32 loc(#loc909)
    %1487 = arith.ori %1486, %1236 : i32 loc(#loc910)
    %1488 = arith.ori %1487, %1163 : i32 loc(#loc910)
    %1489 = arith.ori %1488, %c8192_i32 : i32 loc(#loc910)
    %1490 = arith.andi %1480, %c31_i32 : i32 loc(#loc911)
    %1491 = arith.shli %1490, %c7_i32 : i32 loc(#loc912)
    %1492 = arith.ori %1489, %1491 : i32 loc(#loc910)
    %1493 = arith.ori %1492, %c35_i32 : i32 loc(#loc910)
    %1494 = arith.andi %1480, %c4095_i32 : i32 loc(#loc913)
    %1495 = arith.shli %1494, %c20_i32 : i32 loc(#loc914)
    %1496 = arith.ori %1495, %1163 : i32 loc(#loc915)
    %1497 = arith.ori %1496, %c8192_i32 : i32 loc(#loc915)
    %1498 = arith.shli %855, %c7_i32 : i32 loc(#loc916)
    %1499 = arith.ori %1497, %1498 : i32 loc(#loc915)
    %1500 = arith.ori %1499, %c3_i32 : i32 loc(#loc915)
    %1501 = arith.select %893, %1500, %1493 : i32 loc(#loc653)
    %1502 = arith.ori %893, %897 : i1 loc(#loc562)
    %1503 = arith.andi %1502, %828 : i1 loc(#loc562)
    %1504 = arith.xori %1467, %true : i1 loc(#loc562)
    %1505 = arith.ori %1504, %828 : i1 loc(#loc562)
    %1506 = arith.andi %1469, %1505 : i1 loc(#loc562)
    %1507 = arith.select %1467, %1501, %c0_i32 : i32 loc(#loc653)
    %1508 = arith.andi %1467, %1503 : i1 loc(#loc562)
    %1509 = arith.ori %1508, %1504 : i1 loc(#loc562)
    %1510 = arith.andi %1509, %1469 : i1 loc(#loc562)
    %1511 = arith.andi %1455, %885 : i1 loc(#loc562)
    %1512 = arith.xori %1511, %true : i1 loc(#loc562)
    %1513 = arith.ori %1512, %1458 : i1 loc(#loc562)
    %1514 = arith.andi %848, %c15_i32 : i32 loc(#loc719)
    %1515 = arith.shli %1514, %c6_i32 : i32 loc(#loc720)
    %1516 = arith.andi %1385, %c3_i32 : i32 loc(#loc721)
    %1517 = arith.shli %1516, %c4_i32 : i32 loc(#loc722)
    %1518 = arith.ori %1515, %1517 : i32 loc(#loc723)
    %1519 = arith.shli %1333, %c3_i32 : i32 loc(#loc724)
    %1520 = arith.ori %1518, %1519 : i32 loc(#loc723)
    %1521 = arith.ori %1520, %1479 : i32 loc(#loc652)
    %1522 = arith.cmpi ne, %1521, %c0_i32 : i32 loc(#loc725)
    %1523 = arith.andi %1513, %828 : i1 loc(#loc562)
    %1524 = arith.ori %1512, %1523 : i1 loc(#loc562)
    %1525 = arith.andi %1483, %1524 : i1 loc(#loc562)
    %1526 = arith.andi %1521, %c4095_i32 : i32 loc(#loc866)
    %1527 = arith.shli %1526, %c20_i32 : i32 loc(#loc867)
    %1528 = arith.ori %1527, %c65536_i32 : i32 loc(#loc868)
    %1529 = arith.ori %1528, %1498 : i32 loc(#loc868)
    %1530 = arith.ori %1529, %c19_i32 : i32 loc(#loc653)
    %1531 = arith.cmpi eq, %1521, %c0_i32 : i32 loc(#loc917)
    %1532 = arith.select %1522, %1530, %c0_i32 : i32 loc(#loc653)
    %1533 = arith.ori %1522, %1531 : i1 loc(#loc562)
    %1534 = arith.ori %1531, %828 : i1 loc(#loc562)
    %1535 = arith.andi %828, %1534 : i1 loc(#loc562)
    %1536 = arith.andi %1533, %828 : i1 loc(#loc562)
    %1537 = arith.select %885, %828, %1506 : i1 loc(#loc562)
    %1538 = arith.andi %828, %1537 : i1 loc(#loc562)
    %1539 = arith.select %885, %1532, %1507 : i32 loc(#loc653)
    %1540 = arith.andi %885, %1536 : i1 loc(#loc562)
    %1541 = arith.andi %889, %1510 : i1 loc(#loc562)
    %1542 = arith.ori %1540, %1541 : i1 loc(#loc562)
    %1543 = arith.select %885, %1535, %1506 : i1 loc(#loc562)
    %1544 = arith.andi %828, %1543 : i1 loc(#loc562)
    %1545 = arith.andi %1542, %828 : i1 loc(#loc562)
    %1546 = arith.andi %1544, %1538 : i1 loc(#loc562)
    %1547 = arith.select %862, %1538, %1446 : i1 loc(#loc562)
    %1548 = arith.andi %828, %1547 : i1 loc(#loc562)
    %1549 = arith.select %862, %1539, %1447 : i32 loc(#loc653)
    %1550 = arith.andi %862, %1545 : i1 loc(#loc562)
    %1551 = arith.andi %866, %1453 : i1 loc(#loc562)
    %1552 = arith.ori %1550, %1551 : i1 loc(#loc562)
    %1553 = arith.select %862, %1546, %1454 : i1 loc(#loc562)
    %1554 = arith.andi %828, %1553 : i1 loc(#loc562)
    %1555 = arith.andi %1552, %828 : i1 loc(#loc562)
    %1556 = arith.ori %836, %1548 : i1 loc(#loc562)
    %1557 = arith.andi %828, %1556 : i1 loc(#loc562)
    %1558 = arith.andi %836, %828 : i1 loc(#loc562)
    %1559 = arith.andi %840, %1554 : i1 loc(#loc562)
    %1560 = arith.ori %1558, %1559 : i1 loc(#loc562)
    %1561 = arith.andi %840, %1555 : i1 loc(#loc562)
    %1562 = arith.ori %836, %1561 : i1 loc(#loc562)
    %1563 = arith.ori %836, %840 : i1 loc(#loc562)
    %1564 = arith.andi %1560, %828 : i1 loc(#loc562)
    %1565 = arith.andi %1563, %828 : i1 loc(#loc562)
    %1566 = arith.xori %841, %true : i1 loc(#loc562)
    %1567 = arith.andi %1525, %838 : i1 loc(#loc562)
    acir.path %831 {branch = 118 : i64, kind = "branch", txn = 0 : i64} loc(#loc562)
    %1568 = arith.andi %1565, %1557 : i1 loc(#loc562)
    %1569 = arith.andi %1564, %1557 : i1 loc(#loc562)
    %1570 = arith.andi %1569, %1568 : i1 loc(#loc562)
    %value_66, %ready_67 = acir.read(%831) {branch = 118 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc214)
    %1571 = arith.andi %1567, %834 : i1 loc(#loc214)
    %1572 = arith.select %836, %c4_i32, %c2_i32 : i32 loc(#loc360)
    %1573 = arith.andi %1568, %1563 : i1 loc(#loc360)
    %1574 = arith.cmpi eq, %1572, %c2_i32 : i32 loc(#loc360)
    %1575 = arith.andi %833, %1573 : i1 loc(#loc361)
    %1576 = arith.ori %832, %1575 : i1 loc(#loc361)
    %1577 = arith.andi %1571, %1576 : i1
    %1578 = arith.select %1574, %845, %826 : i32
    %1579 = arith.select %836, %826, %1549 : i32 loc(#loc362)
    %1580 = arith.andi %1570, %1562 : i1 loc(#loc362)
    %1581 = arith.andi %1577, %834 : i1 loc(#loc363)
    %1582 = arith.addi %value_66, %1572 : i32 loc(#loc363)
    %1583 = arith.andi %1573, %1580 : i1
    %1584 = acir.aggregate(%true, %value_66, %1578, %1579, %1582, %false) : (i1, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_ae2f66cd.Fetched"> loc(#loc210)
    %1585 = arith.andi %1581, %1576 : i1 loc(#loc361)
    %1586 = arith.xori %817, %true : i1
    %1587 = arith.ori %1586, %1573 : i1
    %1588 = arith.andi %775, %1587 : i1
    %1589 = arith.select %817, %1584, %819 : !acir.struct<"src_isa_ae2f66cd.Fetched">
    %1590 = arith.andi %1586, %825 : i1
    %1591 = arith.ori %817, %1590 : i1
    %1592 = arith.select %817, %1583, %824 : i1
    %1593 = arith.andi %775, %1592 : i1
    %1594 = arith.andi %1591, %775 : i1
    %1595 = arith.andi %1593, %1588 : i1
    %1596 = arith.andi %739, %753 : i1
    %1597 = arith.xori %1596, %true : i1 loc(#loc364)
    %1598 = arith.ori %1597, %741 : i1
    acir.path %1596 {branch = 119 : i64, kind = "branch", txn = 0 : i64} loc(#loc364)
    %value_68, %ready_69 = acir.read(%1596) {branch = 119 : i64, frames = ["top.cpu.fetch:advance"], method = "read", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1) -> (i32, i1) loc(#loc364)
    %1599 = arith.ori %1597, %1598 : i1 loc(#loc364)
    %1600 = arith.andi %1585, %1599 : i1 loc(#loc364)
    %1601 = arith.addi %value_68, %c4_i32 : i32 loc(#loc365)
    %1602 = acir.aggregate(%true, %value_68, %c0_i32, %c0_i32, %1601, %true) : (i1, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_ae2f66cd.Fetched"> loc(#loc210)
    %1603 = arith.ori %753, %1588 : i1
    %1604 = arith.ori %753, %829 : i1
    %1605 = arith.select %753, %1602, %1589 : !acir.struct<"src_isa_ae2f66cd.Fetched">
    %1606 = arith.andi %755, %1594 : i1
    %1607 = arith.ori %753, %1606 : i1
    %1608 = arith.ori %753, %1595 : i1
    %1609 = arith.andi %1608, %1603 : i1
    %1610 = arith.andi %732, %473 : i1
    %1611 = arith.xori %1610, %true : i1 loc(#loc366)
    %1612 = arith.ori %1611, %735 : i1
    %1613 = arith.andi %1612, %511 : i1 loc(#loc366)
    %1614 = arith.ori %1611, %1613 : i1 loc(#loc366)
    %1615 = arith.andi %1600, %1614 : i1 loc(#loc366)
    acir.path %1610 {branch = 120 : i64, kind = "branch", txn = 0 : i64} loc(#loc366)
    %1616 = arith.ori %473, %1603 : i1
    %1617 = arith.andi %510, %1616 : i1
    %1618 = arith.ori %473, %1604 : i1
    %1619 = arith.select %473, %715, %1605 : !acir.struct<"src_isa_ae2f66cd.Fetched">
    %1620 = arith.andi %738, %1607 : i1
    %1621 = arith.ori %473, %1620 : i1
    %1622 = arith.ori %473, %1609 : i1
    %1623 = arith.andi %510, %1622 : i1
    %1624 = arith.andi %1621, %510 : i1
    %1625 = arith.andi %1617, %1617 : i1
    %1626 = arith.andi %1623, %1617 : i1
    %1627 = arith.ori %725, %1617 : i1
    %1628 = arith.andi %727, %1627 : i1
    %1629 = arith.andi %731, %1618 : i1
    %1630 = arith.ori %725, %1625 : i1
    %1631 = arith.andi %727, %1630 : i1
    %1632 = arith.select %725, %718, %1619 : !acir.struct<"src_isa_ae2f66cd.Fetched">
    %1633 = arith.andi %725, %651 : i1
    %1634 = arith.andi %731, %1624 : i1
    %1635 = arith.ori %1633, %1634 : i1
    %1636 = arith.select %725, %717, %1626 : i1
    %1637 = arith.andi %727, %1636 : i1
    %1638 = arith.andi %1635, %727 : i1
    %1639 = arith.andi %1631, %1628 : i1
    %1640 = arith.andi %1637, %1628 : i1
    %1641 = arith.ori %636, %1628 : i1
    %1642 = arith.andi %651, %1641 : i1
    %1643 = arith.andi %717, %1642 : i1
    %1644 = arith.ori %636, %1629 : i1
    %1645 = arith.ori %636, %1639 : i1
    %1646 = arith.andi %651, %1645 : i1
    %1647 = arith.select %636, %715, %1632 : !acir.struct<"src_isa_ae2f66cd.Fetched">
    %1648 = arith.andi %636, %651 : i1
    %1649 = arith.andi %653, %1638 : i1
    %1650 = arith.ori %1648, %1649 : i1
    %1651 = arith.select %636, %717, %1640 : i1
    %1652 = arith.andi %651, %1651 : i1
    %1653 = arith.andi %1650, %651 : i1
    %1654 = arith.andi %1652, %1646 : i1
    %1655 = arith.andi %1654, %1644 : i1
    %1656 = arith.andi %1655, %1653 : i1
    %1657 = arith.andi %1656, %1643 : i1
    acir.path %1657 {branch = 121 : i64, kind = "branch", txn = 0 : i64}
    %1658 = acir.fire "top.cpu.fetch.advance#0" %1615 {successors = [], txn = 0 : i64}
    %1659 = arith.xori %520, %true : i1
    %1660 = arith.cmpi ne, %589, %c1_i32 : i32 loc(#loc619)
    %1661 = arith.cmpi ne, %589, %c4_i32 : i32 loc(#loc620)
    %1662 = arith.cmpi ne, %589, %c11_i32 : i32 loc(#loc622)
    %1663 = arith.cmpi ne, %589, %c10_i32 : i32 loc(#loc623)
    %1664 = arith.cmpi ne, %589, %c93_i32 : i32 loc(#loc624)
    %1665 = arith.andi %1664, %657 : i1
    %1666 = arith.andi %1663, %1665 : i1
    %1667 = arith.andi %1662, %1666 : i1
    %1668 = arith.andi %1661, %1667 : i1
    %1669 = arith.andi %1660, %1668 : i1
    %1670 = arith.select %572, %657, %1669 : i1
    %1671 = arith.select %555, %657, %1670 : i1
    %1672 = arith.andi %1659, %1671 : i1
    %1673 = arith.andi %518, %1672 : i1
    %1674 = arith.select %515, %1673, %1671 : i1
    %1675 = arith.select %512, %1674, %1671 : i1
    %1676 = arith.andi %535, %1675 : i1
    %1677 = arith.select %531, %657, %1676 : i1
    %1678 = arith.select %529, %657, %1677 : i1
    %1679 = arith.andi %527, %1678 : i1
    %1680 = arith.select %value_20, %657, %1679 : i1
    %1681 = arith.andi %1658, %1680 : i1
    acir.effect %1681(%709) {branch = 109 : i64, frames = ["top.cpu.fetch:advance"], method = "write", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc192)
    %1682 = arith.andi %1658, %787 : i1
    acir.effect %1682(%793) {branch = 116 : i64, frames = ["top.cpu.fetch:advance"], method = "write", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc367)
    %1683 = arith.andi %1658, %831 : i1
    acir.effect %1683(%1566) {branch = 118 : i64, frames = ["top.cpu.fetch:advance"], method = "check", port = 0 : i64, resource = "", txn = 0 : i64} : (i1, i1) -> () loc(#loc562)
    acir.effect %1683(%1582) {branch = 118 : i64, frames = ["top.cpu.fetch:advance"], method = "write", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc361)
    %1684 = arith.andi %1658, %1596 : i1
    acir.effect %1684(%1601) {branch = 119 : i64, frames = ["top.cpu.fetch:advance"], method = "write", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc368)
    %1685 = arith.andi %1658, %1610 : i1
    acir.effect %1685(%492) {branch = 120 : i64, frames = ["top.cpu.fetch:advance"], method = "write", port = 0 : i64, resource = "top.cpu.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc366)
    %1686 = arith.andi %1658, %1657 : i1
    acir.effect %1686(%1647) {branch = 121 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.cpu.if_id", txn = 0 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.Fetched">) -> ()
    acir.path %true {branch = 122 : i64, kind = "branch", txn = 1 : i64} loc(#loc369)
    acir.path %true {branch = 122 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc369)
    %value_70, %ready_71 = acir.read(%true) {branch = 122 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "read", port = 0 : i64, resource = "top.cpu.if_id", txn = 1 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Fetched">, i1) loc(#loc370)
    %1687 = acir.extract %value_70 {field = "expanded"} : (!acir.struct<"src_isa_ae2f66cd.Fetched">) -> i32 loc(#loc726)
    %1688 = arith.andi %1687, %c127_i32 : i32 loc(#loc727)
    %1689 = arith.shrui %1687, %c12_i32 : i32 loc(#loc728)
    %1690 = arith.andi %1689, %c7_i32 : i32 loc(#loc729)
    %1691 = arith.shrui %1687, %c25_i32 : i32 loc(#loc730)
    %1692 = arith.shrui %1687, %c7_i32 : i32 loc(#loc731)
    %1693 = arith.andi %1692, %c31_i32 : i32 loc(#loc731)
    %1694 = arith.shrui %1687, %c15_i32 : i32 loc(#loc732)
    %1695 = arith.andi %1694, %c31_i32 : i32 loc(#loc732)
    %1696 = arith.cmpi eq, %1688, %c51_i32 : i32 loc(#loc733)
    %1697 = arith.cmpi eq, %1688, %c35_i32 : i32 loc(#loc734)
    %1698 = arith.ori %1696, %1697 : i1 loc(#loc735)
    %1699 = arith.cmpi eq, %1688, %c99_i32 : i32 loc(#loc736)
    %1700 = arith.ori %1698, %1699 : i1 loc(#loc735)
    %1701 = arith.shrui %1687, %c20_i32 : i32 loc(#loc737)
    %1702 = arith.andi %1701, %c31_i32 : i32 loc(#loc737)
    %1703 = arith.cmpi eq, %1688, %c19_i32 : i32 loc(#loc738)
    %1704 = arith.cmpi eq, %1688, %c3_i32 : i32 loc(#loc739)
    %1705 = arith.cmpi eq, %1688, %c103_i32 : i32 loc(#loc740)
    %1706 = arith.ori %1704, %1705 : i1 loc(#loc735)
    %1707 = arith.cmpi eq, %1688, %c55_i32 : i32 loc(#loc741)
    %1708 = arith.cmpi eq, %1688, %c23_i32 : i32 loc(#loc742)
    %1709 = arith.ori %1707, %1708 : i1 loc(#loc735)
    %1710 = arith.cmpi eq, %1688, %c111_i32 : i32 loc(#loc743)
    %1711 = arith.cmpi eq, %1688, %c15_i32 : i32 loc(#loc744)
    %1712 = arith.cmpi eq, %1688, %c115_i32 : i32 loc(#loc745)
    %1713 = arith.cmpi eq, %1687, %c115_i32 : i32 loc(#loc746)
    %1714 = arith.cmpi eq, %1687, %c1048691_i32 : i32 loc(#loc747)
    %1715 = arith.ori %1713, %1714 : i1 loc(#loc748)
    %1716 = arith.select %1700, %1702, %c0_i32 : i32 loc(#loc749)
    %1717 = arith.select %1713, %c1_i32, %c2_i32 : i32 loc(#loc735)
    %1718 = arith.andi %1712, %1715 : i1 loc(#loc748)
    %1719 = arith.cmpi eq, %1690, %c0_i32 : i32 loc(#loc748)
    %1720 = arith.select %1711, %1719, %1718 : i1 loc(#loc748)
    %1721 = arith.shrui %1687, %c31_i32 : i32 loc(#loc750)
    %1722 = arith.shli %1721, %c20_i32 : i32 loc(#loc750)
    %1723 = arith.andi %1689, %c255_i32 : i32 loc(#loc751)
    %1724 = arith.shli %1723, %c12_i32 : i32 loc(#loc752)
    %1725 = arith.ori %1722, %1724 : i32 loc(#loc753)
    %1726 = arith.andi %1701, %c1_i32 : i32 loc(#loc754)
    %1727 = arith.shli %1726, %c11_i32 : i32 loc(#loc755)
    %1728 = arith.ori %1725, %1727 : i32 loc(#loc753)
    %1729 = arith.shrui %1687, %c21_i32 : i32 loc(#loc756)
    %1730 = arith.andi %1729, %c1023_i32 : i32 loc(#loc756)
    %1731 = arith.shli %1730, %c1_i32 : i32 loc(#loc757)
    %1732 = arith.ori %1728, %1731 : i32 loc(#loc758)
    %1733 = arith.xori %1732, %c1048576_i32 : i32 loc(#loc918)
    %1734 = arith.subi %1733, %c1048576_i32 : i32 loc(#loc919)
    %1735 = arith.ori %1710, %1720 : i1 loc(#loc748)
    %1736 = arith.andi %1687, %c-4096_i32 : i32 loc(#loc760)
    %1737 = arith.ori %1709, %1735 : i1 loc(#loc748)
    %1738 = arith.cmpi eq, %1690, %c1_i32 : i32 loc(#loc761)
    %1739 = arith.ori %1719, %1738 : i1 loc(#loc735)
    %1740 = arith.cmpi uge, %1690, %c4_i32 : i32 loc(#loc762)
    %1741 = arith.ori %1739, %1740 : i1 loc(#loc748)
    %1742 = arith.shli %1721, %c12_i32 : i32 loc(#loc763)
    %1743 = arith.andi %1692, %c1_i32 : i32 loc(#loc764)
    %1744 = arith.shli %1743, %c11_i32 : i32 loc(#loc765)
    %1745 = arith.ori %1742, %1744 : i32 loc(#loc766)
    %1746 = arith.andi %1691, %c63_i32 : i32 loc(#loc767)
    %1747 = arith.shli %1746, %c5_i32 : i32 loc(#loc768)
    %1748 = arith.ori %1745, %1747 : i32 loc(#loc766)
    %1749 = arith.shrui %1687, %c8_i32 : i32 loc(#loc769)
    %1750 = arith.andi %1749, %c15_i32 : i32 loc(#loc769)
    %1751 = arith.shli %1750, %c1_i32 : i32 loc(#loc770)
    %1752 = arith.ori %1748, %1751 : i32 loc(#loc758)
    %1753 = arith.xori %1752, %c4096_i32 : i32 loc(#loc920)
    %1754 = arith.subi %1753, %c4096_i32 : i32 loc(#loc921)
    %1755 = arith.select %1699, %1741, %1737 : i1 loc(#loc748)
    %1756 = arith.cmpi ule, %1690, %c2_i32 : i32 loc(#loc748)
    %1757 = arith.shli %1691, %c5_i32 : i32 loc(#loc772)
    %1758 = arith.ori %1757, %1693 : i32 loc(#loc922)
    %1759 = arith.xori %1758, %c2048_i32 : i32 loc(#loc923)
    %1760 = arith.subi %1759, %c2048_i32 : i32 loc(#loc924)
    %1761 = arith.select %1697, %1756, %1755 : i1 loc(#loc748)
    %1762 = arith.cmpi eq, %1690, %c2_i32 : i32 loc(#loc774)
    %1763 = arith.ori %1739, %1762 : i1 loc(#loc735)
    %1764 = arith.cmpi eq, %1690, %c4_i32 : i32 loc(#loc775)
    %1765 = arith.ori %1763, %1764 : i1 loc(#loc735)
    %1766 = arith.cmpi eq, %1690, %c5_i32 : i32 loc(#loc776)
    %1767 = arith.ori %1765, %1766 : i1 loc(#loc735)
    %1768 = arith.select %1704, %1767, %1719 : i1 loc(#loc748)
    %1769 = arith.xori %1701, %c2048_i32 : i32 loc(#loc925)
    %1770 = arith.subi %1769, %c2048_i32 : i32 loc(#loc926)
    %1771 = arith.select %1706, %1768, %1761 : i1 loc(#loc748)
    %1772 = arith.cmpi ne, %1690, %c1_i32 : i32 loc(#loc778)
    %1773 = arith.cmpi ne, %1690, %c5_i32 : i32 loc(#loc779)
    %1774 = arith.andi %1772, %1773 : i1 loc(#loc735)
    %1775 = arith.cmpi eq, %1691, %c0_i32 : i32 loc(#loc780)
    %1776 = arith.andi %1738, %1775 : i1 loc(#loc735)
    %1777 = arith.ori %1774, %1776 : i1 loc(#loc735)
    %1778 = arith.cmpi eq, %1691, %c32_i32 : i32 loc(#loc781)
    %1779 = arith.ori %1775, %1778 : i1 loc(#loc735)
    %1780 = arith.andi %1766, %1779 : i1 loc(#loc735)
    %1781 = arith.ori %1777, %1780 : i1 loc(#loc748)
    %1782 = arith.select %1703, %1781, %1771 : i1 loc(#loc748)
    %1783 = arith.ori %1719, %1766 : i1 loc(#loc735)
    %1784 = arith.andi %1778, %1783 : i1 loc(#loc735)
    %1785 = arith.ori %1775, %1784 : i1 loc(#loc735)
    %1786 = arith.cmpi eq, %1691, %c1_i32 : i32 loc(#loc782)
    %1787 = arith.ori %1785, %1786 : i1 loc(#loc748)
    %1788 = arith.select %1696, %1787, %1782 : i1 loc(#loc748)
    %1789 = arith.xori %1788, %true : i1 loc(#loc783)
    %1790 = arith.select %1712, %c0_i32, %1695 : i32 loc(#loc784)
    %1791 = arith.select %1711, %c0_i32, %1790 : i32 loc(#loc784)
    %1792 = arith.select %1710, %c0_i32, %1791 : i32 loc(#loc784)
    %1793 = arith.select %1709, %c0_i32, %1792 : i32 loc(#loc784)
    %1794 = arith.select %1699, %1695, %1793 : i32 loc(#loc784)
    %1795 = arith.select %1697, %1695, %1794 : i32 loc(#loc784)
    %1796 = arith.select %1706, %1695, %1795 : i32 loc(#loc784)
    %1797 = arith.select %1703, %1695, %1796 : i32 loc(#loc784)
    %1798 = arith.select %1696, %1695, %1797 : i32 loc(#loc784)
    %1799 = arith.select %1710, %1734, %c0_i32 : i32 loc(#loc784)
    %1800 = arith.select %1709, %1736, %1799 : i32 loc(#loc784)
    %1801 = arith.select %1699, %1754, %1800 : i32 loc(#loc784)
    %1802 = arith.select %1697, %1760, %1801 : i32 loc(#loc784)
    %1803 = arith.select %1706, %1770, %1802 : i32 loc(#loc784)
    %1804 = arith.select %1703, %1770, %1803 : i32 loc(#loc784)
    %1805 = arith.select %1696, %c0_i32, %1804 : i32 loc(#loc784)
    %1806 = arith.select %1712, %1717, %c0_i32 : i32 loc(#loc784)
    %1807 = arith.select %1711, %c0_i32, %1806 : i32 loc(#loc784)
    %1808 = arith.select %1710, %c0_i32, %1807 : i32 loc(#loc784)
    %1809 = arith.select %1709, %c0_i32, %1808 : i32 loc(#loc784)
    %1810 = arith.select %1699, %c0_i32, %1809 : i32 loc(#loc784)
    %1811 = arith.select %1697, %c0_i32, %1810 : i32 loc(#loc784)
    %1812 = arith.select %1706, %c0_i32, %1811 : i32 loc(#loc784)
    %1813 = arith.select %1703, %c0_i32, %1812 : i32 loc(#loc784)
    %1814 = arith.select %1696, %c0_i32, %1813 : i32 loc(#loc784)
    %1815 = arith.ori %1696, %1703 : i1 loc(#loc735)
    %1816 = arith.ori %1815, %1704 : i1 loc(#loc735)
    %1817 = arith.ori %1816, %1707 : i1 loc(#loc735)
    %1818 = arith.ori %1817, %1708 : i1 loc(#loc735)
    %1819 = arith.ori %1818, %1710 : i1 loc(#loc735)
    %1820 = arith.ori %1819, %1705 : i1 loc(#loc735)
    %1821 = arith.andi %1788, %1820 : i1 loc(#loc735)
    %1822 = arith.andi %1788, %1704 : i1 loc(#loc735)
    %1823 = arith.andi %1788, %1697 : i1 loc(#loc735)
    %1824 = acir.aggregate(%1688, %1690, %1691, %1693, %1798, %1716, %1805, %1821, %1822, %1823, %1789, %1814) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1, i1, i32) -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc785)
    %1825 = acir.fire "top.cpu.decode_stage.decode_instruction#1" %true {successors = [0, 2, 3, 6], txn = 1 : i64}
    acir.effect %1825(%1824) {branch = 122 : i64, frames = ["top.cpu.decode_stage:decode_instruction"], method = "set", port = 0 : i64, resource = "top.cpu.decode_stage.instruction", txn = 1 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.Instruction">) -> () loc(#loc432)
    acir.path %true {branch = 123 : i64, kind = "branch", txn = 2 : i64} loc(#loc433)
    acir.path %true {branch = 123 : i64, frames = ["top.cpu.decode_stage:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc433)
    %1826 = acir.aggregate() : () -> !acir.struct<"src_isa_ae2f66cd.Decoded">
    %1827 = arith.andi %643, %646 : i1
    %1828 = arith.ori %523, %646 : i1
    %1829 = arith.andi %1828, %721 : i1
    %1830 = arith.ori %523, %1829 : i1
    %1831 = arith.andi %1827, %1830 : i1
    %1832 = arith.ori %27, %473 : i1
    %1833 = arith.ori %27, %510 : i1
    %1834 = arith.andi %721, %1833 : i1
    %1835 = arith.andi %1828, %1834 : i1
    %1836 = arith.ori %523, %1835 : i1
    %1837 = arith.andi %1831, %1836 : i1
    %1838 = arith.xori %1832, %true : i1
    %1839 = arith.ori %1832, %636 : i1
    %1840 = arith.andi %1838, %650 : i1
    %1841 = arith.ori %1832, %1840 : i1
    %1842 = arith.ori %1832, %651 : i1
    %1843 = arith.andi %1834, %1842 : i1
    %1844 = arith.andi %1841, %1834 : i1
    %1845 = arith.andi %1843, %646 : i1
    %1846 = arith.select %1839, %false, %536 : i1
    %1847 = arith.andi %1844, %500 : i1
    %1848 = arith.andi %1845, %1847 : i1
    %1849 = arith.andi %1828, %1843 : i1
    %1850 = arith.ori %523, %1849 : i1
    %1851 = arith.andi %1837, %1850 : i1
    %1852 = acir.aggregate() : () -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc434)
    %1853 = acir.aggregate(%false, %c0_i32, %c0_i32, %1852, %c0_i32, %c0_i32, %27, %c0_i32, %c0_i32, %false) : (i1, i32, i32, !acir.struct<"src_isa_ae2f66cd.Instruction">, i32, i32, i1, i32, i32, i1) -> !acir.struct<"src_isa_ae2f66cd.Decoded"> loc(#loc435)
    %1854 = arith.xori %1839, %true : i1
    %1855 = arith.select %1839, %1853, %1826 : !acir.struct<"src_isa_ae2f66cd.Decoded">
    %1856 = arith.andi %1839, %720 : i1
    %1857 = arith.ori %1856, %1854 : i1
    %1858 = arith.ori %1854, %721 : i1
    %1859 = arith.andi %1843, %1858 : i1
    %1860 = arith.andi %1857, %1843 : i1
    %1861 = arith.select %1846, %true, %1839 : i1
    %1862 = arith.andi %1828, %1848 : i1
    %1863 = arith.ori %523, %1862 : i1
    %1864 = arith.andi %1851, %1863 : i1
    %1865 = arith.andi %536, %1846 : i1
    %1866 = arith.xori %1865, %true : i1 loc(#loc1)
    %1867 = arith.ori %1866, %1862 : i1
    acir.path %1865 {branch = 124 : i64, kind = "branch", txn = 2 : i64}
    %value_72, %ready_73 = acir.read(%1865) {branch = 124 : i64, frames = ["top.cpu.decode_stage:advance"], method = "read", port = 0 : i64, resource = "top.cpu.if_id", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Fetched">, i1)
    %1868 = arith.ori %1866, %1867 : i1
    %1869 = arith.andi %1864, %1868 : i1
    %value_74, %ready_75 = acir.read(%1865) {branch = 124 : i64, frames = ["top.cpu.decode_stage:advance"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1)
    acir.path %1865 {branch = 124 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.decode_stage:inspect"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64}
    %value_76, %ready_77 = acir.read(%1865) {branch = 124 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.decode_stage:inspect"], method = "get", port = 0 : i64, resource = "top.cpu.decode_stage.instruction", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Instruction">, i1) loc(#loc1)
    %1870 = arith.andi %1868, %ready_77 : i1 loc(#loc1)
    %1871 = arith.ori %1866, %1870 : i1 loc(#loc1)
    %1872 = arith.andi %1869, %1871 : i1 loc(#loc1)
    %1873 = acir.bind %value_76 : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc436)
    %1874 = acir.extract %value_76 {field = "rs1"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc437)
    acir.path %1865 {branch = 124 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64}
    %1875 = arith.cmpi eq, %1874, %c0_i32 : i32 loc(#loc438)
    %1876 = arith.cmpi ne, %1874, %c0_i32 : i32 loc(#loc438)
    %1877 = arith.select %1875, %c0_i32, %32 : i32
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc153)
    %1878 = arith.andi %1865, %1876 : i1
    %1879 = arith.xori %1878, %true : i1
    %1880 = arith.ori %1879, %1870 : i1
    %1881 = acir.extract %value_74 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc154)
    %1882 = arith.ori %1879, %1880 : i1
    %1883 = arith.andi %1872, %1882 : i1
    %1884 = acir.extract %value_74 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc155)
    %1885 = arith.cmpi eq, %1884, %1874 : i32 loc(#loc155)
    %1886 = arith.andi %1881, %1885 : i1
    %1887 = arith.xori %1881, %true : i1
    %1888 = arith.ori %1887, %ready_77 : i1
    %1889 = arith.xori %1886, %true : i1
    %1890 = arith.select %1886, %1889, %false : i1
    %1891 = arith.andi %1880, %1888 : i1
    %1892 = arith.ori %1879, %1891 : i1
    %1893 = arith.andi %1883, %1892 : i1
    %1894 = arith.andi %1878, %1889 : i1
    %1895 = arith.xori %1894, %true : i1 loc(#loc156)
    %1896 = arith.ori %1895, %1891 : i1
    %1897 = arith.extui %1874 : i32 to i64 loc(#loc156)
    %1898 = arith.cmpi ult, %1897, %c32_i64 : i64 loc(#loc156)
    %1899 = arith.andi %ready_77, %1898 : i1 loc(#loc156)
    %1900 = arith.andi %1896, %1899 : i1 loc(#loc156)
    %1901 = arith.ori %1895, %1900 : i1 loc(#loc156)
    %1902 = arith.andi %1893, %1901 : i1 loc(#loc156)
    acir.path %1894 {branch = 125 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %value_78, %ready_79 = acir.read(%1894, %1897) {array = "top.cpu.rf.registers", branch = 125 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = 2 : i64} : (i1, i64) -> (i32, i1) loc(#loc156)
    %1903 = arith.cmpi eq, %1897, %c0_i64 : i64 loc(#loc156)
    %1904 = arith.andi %1894, %1903 : i1 loc(#loc156)
    acir.path %1904 {branch = 126 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1905 = arith.cmpi eq, %1897, %c1_i64 : i64 loc(#loc156)
    %1906 = arith.andi %1894, %1905 : i1 loc(#loc156)
    acir.path %1906 {branch = 127 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1907 = arith.cmpi eq, %1897, %c2_i64 : i64 loc(#loc156)
    %1908 = arith.andi %1894, %1907 : i1 loc(#loc156)
    acir.path %1908 {branch = 128 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1909 = arith.cmpi eq, %1897, %c3_i64 : i64 loc(#loc156)
    %1910 = arith.andi %1894, %1909 : i1 loc(#loc156)
    acir.path %1910 {branch = 129 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1911 = arith.cmpi eq, %1897, %c4_i64 : i64 loc(#loc156)
    %1912 = arith.andi %1894, %1911 : i1 loc(#loc156)
    acir.path %1912 {branch = 130 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1913 = arith.cmpi eq, %1897, %c5_i64 : i64 loc(#loc156)
    %1914 = arith.andi %1894, %1913 : i1 loc(#loc156)
    acir.path %1914 {branch = 131 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1915 = arith.cmpi eq, %1897, %c6_i64 : i64 loc(#loc156)
    %1916 = arith.andi %1894, %1915 : i1 loc(#loc156)
    acir.path %1916 {branch = 132 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1917 = arith.cmpi eq, %1897, %c7_i64 : i64 loc(#loc156)
    %1918 = arith.andi %1894, %1917 : i1 loc(#loc156)
    acir.path %1918 {branch = 133 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1919 = arith.cmpi eq, %1897, %c8_i64 : i64 loc(#loc156)
    %1920 = arith.andi %1894, %1919 : i1 loc(#loc156)
    acir.path %1920 {branch = 134 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1921 = arith.cmpi eq, %1897, %c9_i64 : i64 loc(#loc156)
    %1922 = arith.andi %1894, %1921 : i1 loc(#loc156)
    acir.path %1922 {branch = 135 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1923 = arith.cmpi eq, %1897, %c10_i64 : i64 loc(#loc156)
    %1924 = arith.andi %1894, %1923 : i1 loc(#loc156)
    acir.path %1924 {branch = 136 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1925 = arith.cmpi eq, %1897, %c11_i64 : i64 loc(#loc156)
    %1926 = arith.andi %1894, %1925 : i1 loc(#loc156)
    acir.path %1926 {branch = 137 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1927 = arith.cmpi eq, %1897, %c12_i64 : i64 loc(#loc156)
    %1928 = arith.andi %1894, %1927 : i1 loc(#loc156)
    acir.path %1928 {branch = 138 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1929 = arith.cmpi eq, %1897, %c13_i64 : i64 loc(#loc156)
    %1930 = arith.andi %1894, %1929 : i1 loc(#loc156)
    acir.path %1930 {branch = 139 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1931 = arith.cmpi eq, %1897, %c14_i64 : i64 loc(#loc156)
    %1932 = arith.andi %1894, %1931 : i1 loc(#loc156)
    acir.path %1932 {branch = 140 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1933 = arith.cmpi eq, %1897, %c15_i64 : i64 loc(#loc156)
    %1934 = arith.andi %1894, %1933 : i1 loc(#loc156)
    acir.path %1934 {branch = 141 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1935 = arith.cmpi eq, %1897, %c16_i64 : i64 loc(#loc156)
    %1936 = arith.andi %1894, %1935 : i1 loc(#loc156)
    acir.path %1936 {branch = 142 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1937 = arith.cmpi eq, %1897, %c17_i64 : i64 loc(#loc156)
    %1938 = arith.andi %1894, %1937 : i1 loc(#loc156)
    acir.path %1938 {branch = 143 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1939 = arith.cmpi eq, %1897, %c18_i64 : i64 loc(#loc156)
    %1940 = arith.andi %1894, %1939 : i1 loc(#loc156)
    acir.path %1940 {branch = 144 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1941 = arith.cmpi eq, %1897, %c19_i64 : i64 loc(#loc156)
    %1942 = arith.andi %1894, %1941 : i1 loc(#loc156)
    acir.path %1942 {branch = 145 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1943 = arith.cmpi eq, %1897, %c20_i64 : i64 loc(#loc156)
    %1944 = arith.andi %1894, %1943 : i1 loc(#loc156)
    acir.path %1944 {branch = 146 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1945 = arith.cmpi eq, %1897, %c21_i64 : i64 loc(#loc156)
    %1946 = arith.andi %1894, %1945 : i1 loc(#loc156)
    acir.path %1946 {branch = 147 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1947 = arith.cmpi eq, %1897, %c22_i64 : i64 loc(#loc156)
    %1948 = arith.andi %1894, %1947 : i1 loc(#loc156)
    acir.path %1948 {branch = 148 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1949 = arith.cmpi eq, %1897, %c23_i64 : i64 loc(#loc156)
    %1950 = arith.andi %1894, %1949 : i1 loc(#loc156)
    acir.path %1950 {branch = 149 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1951 = arith.cmpi eq, %1897, %c24_i64 : i64 loc(#loc156)
    %1952 = arith.andi %1894, %1951 : i1 loc(#loc156)
    acir.path %1952 {branch = 150 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1953 = arith.cmpi eq, %1897, %c25_i64 : i64 loc(#loc156)
    %1954 = arith.andi %1894, %1953 : i1 loc(#loc156)
    acir.path %1954 {branch = 151 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1955 = arith.cmpi eq, %1897, %c26_i64 : i64 loc(#loc156)
    %1956 = arith.andi %1894, %1955 : i1 loc(#loc156)
    acir.path %1956 {branch = 152 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1957 = arith.cmpi eq, %1897, %c27_i64 : i64 loc(#loc156)
    %1958 = arith.andi %1894, %1957 : i1 loc(#loc156)
    acir.path %1958 {branch = 153 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1959 = arith.cmpi eq, %1897, %c28_i64 : i64 loc(#loc156)
    %1960 = arith.andi %1894, %1959 : i1 loc(#loc156)
    acir.path %1960 {branch = 154 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1961 = arith.cmpi eq, %1897, %c29_i64 : i64 loc(#loc156)
    %1962 = arith.andi %1894, %1961 : i1 loc(#loc156)
    acir.path %1962 {branch = 155 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1963 = arith.cmpi eq, %1897, %c30_i64 : i64 loc(#loc156)
    %1964 = arith.andi %1894, %1963 : i1 loc(#loc156)
    acir.path %1964 {branch = 156 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1965 = arith.cmpi eq, %1897, %c31_i64 : i64 loc(#loc156)
    %1966 = arith.andi %1894, %1965 : i1 loc(#loc156)
    acir.path %1966 {branch = 157 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %1967 = acir.extract %value_74 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc157)
    %1968 = arith.select %1886, %1967, %value_78 : i32
    %1969 = arith.ori %1886, %1899 : i1
    %1970 = arith.andi %1888, %1969 : i1
    %1971 = arith.ori %1875, %1888 : i1
    %1972 = arith.andi %ready_77, %1971 : i1
    %1973 = arith.andi %1876, %1890 : i1
    %1974 = arith.select %1876, %1968, %1877 : i32 loc(#loc439)
    %1975 = arith.andi %1876, %1888 : i1
    %1976 = arith.ori %1975, %1875 : i1
    %1977 = arith.select %1876, %1970, %ready_77 : i1
    %1978 = arith.andi %ready_77, %1977 : i1
    %1979 = arith.andi %1976, %ready_77 : i1
    %1980 = arith.xori %1973, %true : i1
    %1981 = arith.andi %1868, %1972 : i1
    %1982 = arith.ori %1866, %1981 : i1
    %1983 = arith.andi %1902, %1982 : i1
    %1984 = arith.andi %1978, %1972 : i1
    %1985 = arith.andi %1979, %1972 : i1
    %1986 = arith.andi %1984, %1985 : i1
    %1987 = acir.extract %value_76 {field = "rs2"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc437)
    acir.path %1865 {branch = 124 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64}
    %1988 = arith.cmpi eq, %1987, %c0_i32 : i32 loc(#loc438)
    %1989 = arith.cmpi ne, %1987, %c0_i32 : i32 loc(#loc438)
    %1990 = arith.select %1988, %c0_i32, %32 : i32
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc153)
    %1991 = arith.andi %1983, %1871 : i1
    %1992 = arith.andi %1865, %1989 : i1
    %1993 = arith.xori %1992, %true : i1
    %1994 = arith.ori %1993, %1870 : i1
    %1995 = arith.ori %1993, %1994 : i1
    %1996 = arith.andi %1991, %1995 : i1
    %1997 = arith.cmpi eq, %1884, %1987 : i32 loc(#loc155)
    %1998 = arith.andi %1881, %1997 : i1
    %1999 = arith.xori %1998, %true : i1
    %2000 = arith.select %1998, %1999, %false : i1
    %2001 = arith.andi %1994, %1888 : i1
    %2002 = arith.ori %1993, %2001 : i1
    %2003 = arith.andi %1996, %2002 : i1
    %2004 = arith.andi %1992, %1999 : i1
    %2005 = arith.xori %2004, %true : i1 loc(#loc156)
    %2006 = arith.ori %2005, %2001 : i1
    %2007 = arith.extui %1987 : i32 to i64 loc(#loc156)
    %2008 = arith.cmpi ult, %2007, %c32_i64 : i64 loc(#loc156)
    %2009 = arith.andi %ready_77, %2008 : i1 loc(#loc156)
    %2010 = arith.andi %2006, %2009 : i1 loc(#loc156)
    %2011 = arith.ori %2005, %2010 : i1 loc(#loc156)
    %2012 = arith.andi %2003, %2011 : i1 loc(#loc156)
    acir.path %2004 {branch = 158 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %value_80, %ready_81 = acir.read(%2004, %2007) {array = "top.cpu.rf.registers", branch = 158 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = 2 : i64} : (i1, i64) -> (i32, i1) loc(#loc156)
    %2013 = arith.cmpi eq, %2007, %c0_i64 : i64 loc(#loc156)
    %2014 = arith.andi %2004, %2013 : i1 loc(#loc156)
    acir.path %2014 {branch = 159 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2015 = arith.cmpi eq, %2007, %c1_i64 : i64 loc(#loc156)
    %2016 = arith.andi %2004, %2015 : i1 loc(#loc156)
    acir.path %2016 {branch = 160 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2017 = arith.cmpi eq, %2007, %c2_i64 : i64 loc(#loc156)
    %2018 = arith.andi %2004, %2017 : i1 loc(#loc156)
    acir.path %2018 {branch = 161 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2019 = arith.cmpi eq, %2007, %c3_i64 : i64 loc(#loc156)
    %2020 = arith.andi %2004, %2019 : i1 loc(#loc156)
    acir.path %2020 {branch = 162 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2021 = arith.cmpi eq, %2007, %c4_i64 : i64 loc(#loc156)
    %2022 = arith.andi %2004, %2021 : i1 loc(#loc156)
    acir.path %2022 {branch = 163 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2023 = arith.cmpi eq, %2007, %c5_i64 : i64 loc(#loc156)
    %2024 = arith.andi %2004, %2023 : i1 loc(#loc156)
    acir.path %2024 {branch = 164 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2025 = arith.cmpi eq, %2007, %c6_i64 : i64 loc(#loc156)
    %2026 = arith.andi %2004, %2025 : i1 loc(#loc156)
    acir.path %2026 {branch = 165 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2027 = arith.cmpi eq, %2007, %c7_i64 : i64 loc(#loc156)
    %2028 = arith.andi %2004, %2027 : i1 loc(#loc156)
    acir.path %2028 {branch = 166 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2029 = arith.cmpi eq, %2007, %c8_i64 : i64 loc(#loc156)
    %2030 = arith.andi %2004, %2029 : i1 loc(#loc156)
    acir.path %2030 {branch = 167 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2031 = arith.cmpi eq, %2007, %c9_i64 : i64 loc(#loc156)
    %2032 = arith.andi %2004, %2031 : i1 loc(#loc156)
    acir.path %2032 {branch = 168 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2033 = arith.cmpi eq, %2007, %c10_i64 : i64 loc(#loc156)
    %2034 = arith.andi %2004, %2033 : i1 loc(#loc156)
    acir.path %2034 {branch = 169 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2035 = arith.cmpi eq, %2007, %c11_i64 : i64 loc(#loc156)
    %2036 = arith.andi %2004, %2035 : i1 loc(#loc156)
    acir.path %2036 {branch = 170 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2037 = arith.cmpi eq, %2007, %c12_i64 : i64 loc(#loc156)
    %2038 = arith.andi %2004, %2037 : i1 loc(#loc156)
    acir.path %2038 {branch = 171 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2039 = arith.cmpi eq, %2007, %c13_i64 : i64 loc(#loc156)
    %2040 = arith.andi %2004, %2039 : i1 loc(#loc156)
    acir.path %2040 {branch = 172 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2041 = arith.cmpi eq, %2007, %c14_i64 : i64 loc(#loc156)
    %2042 = arith.andi %2004, %2041 : i1 loc(#loc156)
    acir.path %2042 {branch = 173 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2043 = arith.cmpi eq, %2007, %c15_i64 : i64 loc(#loc156)
    %2044 = arith.andi %2004, %2043 : i1 loc(#loc156)
    acir.path %2044 {branch = 174 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2045 = arith.cmpi eq, %2007, %c16_i64 : i64 loc(#loc156)
    %2046 = arith.andi %2004, %2045 : i1 loc(#loc156)
    acir.path %2046 {branch = 175 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2047 = arith.cmpi eq, %2007, %c17_i64 : i64 loc(#loc156)
    %2048 = arith.andi %2004, %2047 : i1 loc(#loc156)
    acir.path %2048 {branch = 176 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2049 = arith.cmpi eq, %2007, %c18_i64 : i64 loc(#loc156)
    %2050 = arith.andi %2004, %2049 : i1 loc(#loc156)
    acir.path %2050 {branch = 177 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2051 = arith.cmpi eq, %2007, %c19_i64 : i64 loc(#loc156)
    %2052 = arith.andi %2004, %2051 : i1 loc(#loc156)
    acir.path %2052 {branch = 178 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2053 = arith.cmpi eq, %2007, %c20_i64 : i64 loc(#loc156)
    %2054 = arith.andi %2004, %2053 : i1 loc(#loc156)
    acir.path %2054 {branch = 179 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2055 = arith.cmpi eq, %2007, %c21_i64 : i64 loc(#loc156)
    %2056 = arith.andi %2004, %2055 : i1 loc(#loc156)
    acir.path %2056 {branch = 180 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2057 = arith.cmpi eq, %2007, %c22_i64 : i64 loc(#loc156)
    %2058 = arith.andi %2004, %2057 : i1 loc(#loc156)
    acir.path %2058 {branch = 181 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2059 = arith.cmpi eq, %2007, %c23_i64 : i64 loc(#loc156)
    %2060 = arith.andi %2004, %2059 : i1 loc(#loc156)
    acir.path %2060 {branch = 182 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2061 = arith.cmpi eq, %2007, %c24_i64 : i64 loc(#loc156)
    %2062 = arith.andi %2004, %2061 : i1 loc(#loc156)
    acir.path %2062 {branch = 183 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2063 = arith.cmpi eq, %2007, %c25_i64 : i64 loc(#loc156)
    %2064 = arith.andi %2004, %2063 : i1 loc(#loc156)
    acir.path %2064 {branch = 184 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2065 = arith.cmpi eq, %2007, %c26_i64 : i64 loc(#loc156)
    %2066 = arith.andi %2004, %2065 : i1 loc(#loc156)
    acir.path %2066 {branch = 185 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2067 = arith.cmpi eq, %2007, %c27_i64 : i64 loc(#loc156)
    %2068 = arith.andi %2004, %2067 : i1 loc(#loc156)
    acir.path %2068 {branch = 186 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2069 = arith.cmpi eq, %2007, %c28_i64 : i64 loc(#loc156)
    %2070 = arith.andi %2004, %2069 : i1 loc(#loc156)
    acir.path %2070 {branch = 187 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2071 = arith.cmpi eq, %2007, %c29_i64 : i64 loc(#loc156)
    %2072 = arith.andi %2004, %2071 : i1 loc(#loc156)
    acir.path %2072 {branch = 188 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2073 = arith.cmpi eq, %2007, %c30_i64 : i64 loc(#loc156)
    %2074 = arith.andi %2004, %2073 : i1 loc(#loc156)
    acir.path %2074 {branch = 189 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2075 = arith.cmpi eq, %2007, %c31_i64 : i64 loc(#loc156)
    %2076 = arith.andi %2004, %2075 : i1 loc(#loc156)
    acir.path %2076 {branch = 190 : i64, kind = "branch", txn = 2 : i64} loc(#loc156)
    %2077 = arith.select %1998, %1967, %value_80 : i32
    %2078 = arith.ori %1998, %2009 : i1
    %2079 = arith.andi %1888, %2078 : i1
    %2080 = arith.ori %1988, %1888 : i1
    %2081 = arith.andi %ready_77, %2080 : i1
    %2082 = arith.andi %1989, %2000 : i1
    %2083 = arith.select %1989, %2077, %1990 : i32 loc(#loc440)
    %2084 = arith.andi %1989, %1888 : i1
    %2085 = arith.ori %2084, %1988 : i1
    %2086 = arith.select %1989, %2079, %ready_77 : i1
    %2087 = arith.andi %ready_77, %2086 : i1
    %2088 = arith.andi %2085, %ready_77 : i1
    %2089 = arith.xori %2082, %true : i1
    %2090 = arith.andi %1868, %2081 : i1
    %2091 = arith.ori %1866, %2090 : i1
    %2092 = arith.andi %2012, %2091 : i1
    %2093 = arith.andi %2087, %2081 : i1
    %2094 = arith.andi %2088, %2081 : i1
    %2095 = arith.andi %2093, %2094 : i1
    %2096 = acir.extract %value_72 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Fetched">) -> i1 loc(#loc441)
    %2097 = acir.extract %value_72 {field = "pc"} : (!acir.struct<"src_isa_ae2f66cd.Fetched">) -> i32 loc(#loc442)
    %2098 = acir.extract %value_72 {field = "word"} : (!acir.struct<"src_isa_ae2f66cd.Fetched">) -> i32 loc(#loc443)
    %2099 = acir.extract %value_72 {field = "next_pc"} : (!acir.struct<"src_isa_ae2f66cd.Fetched">) -> i32 loc(#loc444)
    %2100 = acir.extract %value_72 {field = "expanded"} : (!acir.struct<"src_isa_ae2f66cd.Fetched">) -> i32 loc(#loc445)
    %2101 = acir.extract %value_72 {field = "fetch_fault"} : (!acir.struct<"src_isa_ae2f66cd.Fetched">) -> i1 loc(#loc446)
    %2102 = arith.andi %1986, %2095 : i1 loc(#loc447)
    %2103 = acir.aggregate(%2096, %2097, %2098, %1873, %1974, %2083, %false, %2099, %2100, %2101) : (i1, i32, i32, !acir.struct<"src_isa_ae2f66cd.Instruction">, i32, i32, i1, i32, i32, i1) -> !acir.struct<"src_isa_ae2f66cd.Decoded"> loc(#loc447)
    %2104 = arith.xori %1846, %true : i1
    %2105 = arith.select %1846, %2103, %1855 : !acir.struct<"src_isa_ae2f66cd.Decoded">
    %2106 = arith.andi %2104, %1860 : i1
    %2107 = arith.ori %1846, %2106 : i1
    %2108 = arith.select %1846, %2102, %1859 : i1
    %2109 = arith.andi %1848, %2108 : i1
    %2110 = arith.andi %2107, %1848 : i1
    %2111 = arith.ori %523, %1848 : i1
    %2112 = arith.andi %646, %2111 : i1
    %2113 = arith.andi %536, %1861 : i1
    %2114 = arith.andi %536, %1847 : i1
    %2115 = arith.ori %523, %2114 : i1
    %2116 = arith.andi %2115, %646 : i1
    %2117 = arith.select %523, %1826, %2105 : !acir.struct<"src_isa_ae2f66cd.Decoded">
    %2118 = arith.andi %536, %2110 : i1
    %2119 = arith.ori %523, %2118 : i1
    %2120 = arith.ori %523, %2109 : i1
    %2121 = arith.andi %646, %2120 : i1
    %2122 = arith.andi %2119, %646 : i1
    %2123 = arith.andi %2121, %2112 : i1
    %2124 = arith.andi %2123, %2113 : i1
    %2125 = arith.andi %2122, %2116 : i1
    %2126 = arith.andi %2124, %2125 : i1
    %2127 = arith.andi %2126, %2112 : i1
    acir.path %2127 {branch = 191 : i64, kind = "branch", txn = 2 : i64}
    %2128 = acir.fire "top.cpu.decode_stage.advance#2" %2092 {successors = [], txn = 2 : i64}
    %2129 = arith.cmpi ne, %6, %11 : i32 loc(#loc11)
    %2130 = arith.xori %529, %true : i1
    %2131 = arith.cmpi ne, %530, %c2_i32 : i32 loc(#loc148)
    %2132 = arith.cmpi ne, %569, %c93_i32 : i32 loc(#loc159)
    %2133 = arith.ori %601, %605 : i1
    %2134 = arith.ori %597, %2133 : i1
    %2135 = arith.ori %593, %2134 : i1
    %2136 = arith.ori %592, %2135 : i1
    %2137 = arith.andi %2132, %2136 : i1
    %2138 = arith.andi %556, %2137 : i1
    %2139 = arith.ori %538, %2138 : i1
    %2140 = arith.andi %2131, %2139 : i1
    %2141 = arith.andi %2130, %2140 : i1
    %2142 = arith.ori %629, %2141 : i1
    %2143 = arith.andi %526, %2142 : i1
    %2144 = arith.cmpi ne, %110, %c111_i32 : i32 loc(#loc580)
    %2145 = arith.andi %418, %2143 : i1
    %2146 = arith.andi %424, %2143 : i1
    %2147 = arith.select %338, %2145, %2146 : i1
    %2148 = arith.andi %408, %2143 : i1
    %2149 = arith.select %204, %2147, %2148 : i1
    %2150 = arith.andi %413, %2143 : i1
    %2151 = arith.select %221, %2149, %2150 : i1
    %2152 = arith.andi %400, %2143 : i1
    %2153 = arith.select %169, %2151, %2152 : i1
    %2154 = arith.andi %402, %2143 : i1
    %2155 = arith.select %150, %2153, %2154 : i1
    %2156 = arith.select %119, %2155, %2143 : i1
    %2157 = arith.andi %126, %2156 : i1
    %2158 = arith.andi %2144, %2157 : i1
    %2159 = arith.select %392, %2143, %2158 : i1
    %2160 = arith.select %102, %2159, %2143 : i1
    %2161 = arith.andi %2129, %2160 : i1
    %2162 = arith.andi %13, %2161 : i1
    %2163 = arith.select %7, %2162, %2160 : i1
    %2164 = arith.select %4, %2163, %2160 : i1
    %2165 = arith.select %3, %2164, %2160 : i1
    %2166 = arith.andi %1659, %2165 : i1
    %2167 = arith.andi %518, %2166 : i1
    %2168 = arith.select %515, %2167, %2165 : i1
    %2169 = arith.select %512, %2168, %2165 : i1
    %2170 = arith.andi %2128, %2169 : i1
    acir.effect %2170(%1980) {branch = 124 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "check", port = 0 : i64, resource = "", txn = 2 : i64} : (i1, i1) -> ()
    acir.effect %2170(%2089) {branch = 124 : i64, frames = ["top.cpu.decode_stage:advance", "top.cpu.rf:read"], method = "check", port = 0 : i64, resource = "", txn = 2 : i64} : (i1, i1) -> ()
    %2171 = arith.andi %2128, %2127 : i1
    acir.effect %2171(%2117) {branch = 191 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.cpu.id_ex", txn = 2 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.Decoded">) -> ()
    %2172 = acir.export %490 {acir.id = "v100", acir.source_type = "src_isa_ae2f66cd.Executed"} : !acir.struct<"src_isa_ae2f66cd.Executed"> -> !acir.struct<"src_isa_ae2f66cd.Executed"> loc(#loc448)
    %2173 = arith.andi %509, %643 : i1 loc(#loc449)
    acir.path %true {branch = 192 : i64, kind = "branch", txn = 3 : i64} loc(#loc449)
    acir.path %true {branch = 192 : i64, frames = ["top.cpu.execute:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc449)
    %2174 = arith.andi %2173, %646 : i1
    %2175 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %false, %false, %false, %c0_i32, %c0_i32, %c0_i32, %true, %c0_i32, %false, %c0_i32, %c0_i32, %c0_i32) : (i1, i32, i32, i32, i1, i1, i1, i32, i32, i32, i1, i32, i1, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.Executed"> loc(#loc450)
    %2176 = arith.select %523, %2175, %2172 : !acir.struct<"src_isa_ae2f66cd.Executed">
    %2177 = arith.andi %536, %500 : i1
    %2178 = arith.ori %523, %2177 : i1
    %2179 = arith.ori %523, %509 : i1
    %2180 = arith.andi %646, %2179 : i1
    %2181 = arith.andi %2178, %646 : i1
    %2182 = arith.andi %2180, %2181 : i1
    acir.path %2182 {branch = 193 : i64, kind = "branch", txn = 3 : i64}
    %2183 = acir.fire "top.cpu.execute.advance#3" %2174 {successors = [], txn = 3 : i64}
    %2184 = arith.ori %520, %388 : i1
    %2185 = arith.ori %517, %2184 : i1
    %2186 = arith.select %515, %2185, %388 : i1
    %2187 = arith.select %512, %2186, %388 : i1
    %2188 = arith.andi %105, %2187 : i1
    %2189 = arith.select %103, %2188, %2187 : i1
    %2190 = arith.andi %423, %2189 : i1
    %2191 = arith.select %338, %2190, %2189 : i1
    %2192 = arith.select %204, %2191, %2189 : i1
    %2193 = arith.select %221, %2192, %2189 : i1
    %2194 = arith.select %169, %2193, %2189 : i1
    %2195 = arith.select %150, %2194, %2189 : i1
    %2196 = arith.select %119, %2195, %2189 : i1
    %2197 = arith.select %122, %2189, %2196 : i1
    %2198 = arith.select %117, %2189, %2197 : i1
    %2199 = arith.select %392, %2189, %2198 : i1
    %2200 = arith.select %102, %2199, %2187 : i1
    %2201 = arith.andi %2183, %2200 : i1
    acir.effect %2201(%2176) {branch = 193 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.cpu.ex_mem", txn = 3 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.Executed">) -> ()
    acir.path %true {branch = 194 : i64, kind = "branch", txn = 4 : i64} loc(#loc451)
    acir.path %true {branch = 194 : i64, frames = ["top.cpu.memory:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc451)
    %value_82, %ready_83 = acir.read(%true) {branch = 194 : i64, frames = ["top.cpu.memory:advance"], method = "read", port = 0 : i64, resource = "top.cpu.ex_mem", txn = 4 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Executed">, i1)
    %2202 = acir.extract %value_82 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc452)
    %2203 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32) : (i1, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.StoreEvent"> loc(#loc453)
    %2204 = acir.extract %value_82 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc454)
    %2205 = acir.extract %value_82 {field = "load"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc455)
    %2206 = acir.extract %value_82 {field = "store"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc456)
    %2207 = arith.ori %2205, %2206 : i1
    %2208 = arith.andi %2204, %2207 : i1
    %2209 = acir.extract %value_82 {field = "funct3"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc457)
    %2210 = arith.andi %2209, %c3_i32 : i32 loc(#loc457)
    %2211 = arith.shli %c1_i32, %2210 : i32 loc(#loc458)
    %2212 = acir.extract %value_82 {field = "alu"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc459)
    %2213 = arith.andi %2208, %2205 : i1
    acir.path %2213 {branch = 195 : i64, kind = "branch", txn = 4 : i64}
    acir.path %2213 {branch = 195 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64}
    %2214 = arith.cmpi ult, %2212, %c4096_i32 : i32 loc(#loc460)
    %2215 = arith.subi %c4096_i32, %2212 : i32 loc(#loc461)
    %2216 = arith.cmpi ule, %2211, %2215 : i32 loc(#loc462)
    %2217 = arith.andi %2214, %2216 : i1
    %2218 = arith.cmpi uge, %2212, %c4096_i32 : i32 loc(#loc463)
    %2219 = arith.subi %2212, %c4096_i32 : i32 loc(#loc464)
    %2220 = arith.cmpi ult, %2219, %c4096_i32 : i32 loc(#loc464)
    %2221 = arith.andi %2218, %2220 : i1
    %2222 = arith.subi %c8192_i32, %2212 : i32 loc(#loc465)
    %2223 = arith.cmpi ule, %2211, %2222 : i32 loc(#loc466)
    %2224 = arith.andi %2221, %2223 : i1
    %2225 = arith.ori %2217, %2224 : i1
    %2226 = arith.xori %2225, %true : i1
    %2227 = arith.select %2225, %2226, %false : i1
    %2228 = arith.andi %2213, %2226 : i1
    acir.path %2228 {branch = 196 : i64, kind = "branch", txn = 4 : i64} loc(#loc467)
    %2229 = arith.andi %2213, %2225 : i1
    %2230 = arith.andi %2212, %c3_i32 : i32 loc(#loc468)
    %2231 = arith.muli %2230, %c8_i32 : i32 loc(#loc469)
    acir.path %2229 {branch = 197 : i64, kind = "branch", txn = 4 : i64} loc(#loc470)
    acir.path %2229 {branch = 197 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc470)
    %2232 = arith.andi %2229, %2214 : i1
    acir.path %2232 {branch = 198 : i64, kind = "branch", txn = 4 : i64} loc(#loc471)
    %value_84, %ready_85 = acir.read(%2232) {branch = 198 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "read", port = 0 : i64, resource = "top.cpu.memory.text", txn = 4 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc471)
    %2233 = arith.shrui %2212, %c2_i32 : i32 loc(#loc472)
    %2234 = arith.extui %2233 : i32 to i64 loc(#loc471)
    %2235 = arith.cmpi ult, %2234, %c1024_i64 : i64 loc(#loc471)
    %2236 = arith.select %2235, %2234, %c0_i64 : i64 loc(#loc471)
    %2237 = "acir.array.index"(%value_84, %2236) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc471)
    %2238 = arith.select %2214, %2237, %32 : i32
    %2239 = arith.ori %2214, %2218 : i1
    %2240 = arith.ori %2218, %2235 : i1
    %2241 = arith.andi %2229, %2218 : i1
    %2242 = arith.xori %2221, %true : i1
    %2243 = arith.select %2221, %2242, %false : i1
    %2244 = arith.andi %2241, %2242 : i1
    acir.path %2244 {branch = 199 : i64, kind = "branch", txn = 4 : i64} loc(#loc473)
    %2245 = arith.andi %2241, %2221 : i1
    acir.path %2245 {branch = 200 : i64, kind = "branch", txn = 4 : i64} loc(#loc474)
    %value_86, %ready_87 = acir.read(%2245) {branch = 200 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "read", port = 0 : i64, resource = "top.cpu.memory.words", txn = 4 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc474)
    %2246 = arith.shrui %2219, %c2_i32 : i32 loc(#loc475)
    %2247 = arith.extui %2246 : i32 to i64 loc(#loc474)
    %2248 = arith.cmpi ult, %2247, %c1024_i64 : i64 loc(#loc474)
    %2249 = arith.select %2248, %2247, %c0_i64 : i64 loc(#loc474)
    %2250 = "acir.array.index"(%value_86, %2249) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc474)
    %2251 = arith.select %2221, %2250, %c0_i32 : i32
    %2252 = arith.ori %2242, %2248 : i1
    %2253 = arith.andi %2218, %2243 : i1
    %2254 = arith.select %2218, %2251, %2238 : i32
    %2255 = arith.andi %2214, %2239 : i1
    %2256 = arith.ori %2218, %2255 : i1
    %2257 = arith.select %2218, %2252, %2240 : i1
    %2258 = arith.xori %2253, %true : i1
    %2259 = arith.andi %2257, %2256 : i1
    %2260 = arith.shrui %2254, %2231 : i32 loc(#loc476)
    %2261 = arith.addi %2230, %2211 : i32 loc(#loc477)
    %2262 = arith.cmpi ugt, %2261, %c4_i32 : i32 loc(#loc477)
    %2263 = arith.andi %2229, %2262 : i1
    %2264 = arith.andi %2212, %c-4_i32 : i32 loc(#loc478)
    %2265 = arith.addi %2264, %c4_i32 : i32 loc(#loc479)
    acir.path %2263 {branch = 201 : i64, kind = "branch", txn = 4 : i64} loc(#loc480)
    acir.path %2263 {branch = 201 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc480)
    %2266 = arith.cmpi ult, %2265, %c4096_i32 : i32 loc(#loc481)
    %2267 = arith.cmpi uge, %2265, %c4096_i32 : i32 loc(#loc481)
    %2268 = arith.andi %2263, %2266 : i1
    acir.path %2268 {branch = 202 : i64, kind = "branch", txn = 4 : i64} loc(#loc471)
    %value_88, %ready_89 = acir.read(%2268) {branch = 202 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "read", port = 0 : i64, resource = "top.cpu.memory.text", txn = 4 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc471)
    %2269 = arith.shrui %2265, %c2_i32 : i32 loc(#loc472)
    %2270 = arith.extui %2269 : i32 to i64 loc(#loc471)
    %2271 = arith.cmpi ult, %2270, %c1024_i64 : i64 loc(#loc471)
    %2272 = arith.select %2271, %2270, %c0_i64 : i64 loc(#loc471)
    %2273 = "acir.array.index"(%value_88, %2272) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc471)
    %2274 = arith.select %2266, %2273, %32 : i32
    %2275 = arith.ori %2266, %2267 : i1
    %2276 = arith.ori %2267, %2271 : i1
    %2277 = arith.andi %2263, %2267 : i1
    %2278 = arith.addi %2264, %c-4092_i32 : i32 loc(#loc482)
    %2279 = arith.cmpi ult, %2278, %c4096_i32 : i32 loc(#loc482)
    %2280 = arith.andi %2267, %2279 : i1
    %2281 = arith.xori %2280, %true : i1
    %2282 = arith.select %2280, %2281, %false : i1
    %2283 = arith.andi %2277, %2281 : i1
    acir.path %2283 {branch = 203 : i64, kind = "branch", txn = 4 : i64} loc(#loc473)
    %2284 = arith.andi %2277, %2280 : i1
    acir.path %2284 {branch = 204 : i64, kind = "branch", txn = 4 : i64} loc(#loc474)
    %value_90, %ready_91 = acir.read(%2284) {branch = 204 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "read", port = 0 : i64, resource = "top.cpu.memory.words", txn = 4 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc474)
    %2285 = arith.shrui %2278, %c2_i32 : i32 loc(#loc475)
    %2286 = arith.extui %2285 : i32 to i64 loc(#loc474)
    %2287 = arith.cmpi ult, %2286, %c1024_i64 : i64 loc(#loc474)
    %2288 = arith.select %2287, %2286, %c0_i64 : i64 loc(#loc474)
    %2289 = "acir.array.index"(%value_90, %2288) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc474)
    %2290 = arith.select %2280, %2289, %c0_i32 : i32
    %2291 = arith.ori %2281, %2287 : i1
    %2292 = arith.andi %2267, %2282 : i1
    %2293 = arith.select %2267, %2290, %2274 : i32
    %2294 = arith.andi %2266, %2275 : i1
    %2295 = arith.ori %2267, %2294 : i1
    %2296 = arith.select %2267, %2291, %2276 : i1
    %2297 = arith.xori %2292, %true : i1
    %2298 = arith.andi %2296, %2295 : i1
    %2299 = arith.subi %c32_i32, %2231 : i32 loc(#loc483)
    %2300 = arith.shli %2293, %2299 : i32 loc(#loc480)
    %2301 = arith.andi %2259, %2298 : i1
    %2302 = arith.ori %2260, %2300 : i32 loc(#loc476)
    %2303 = arith.select %2262, %2302, %2260 : i32 loc(#loc476)
    %2304 = arith.select %2262, %2301, %2259 : i1
    %2305 = arith.cmpi eq, %2211, %c1_i32 : i32 loc(#loc484)
    %2306 = arith.cmpi eq, %2211, %c2_i32 : i32 loc(#loc485)
    %2307 = arith.select %2306, %c65535_i32, %c-1_i32 : i32
    %2308 = arith.select %2305, %c255_i32, %2307 : i32 loc(#loc486)
    %2309 = arith.andi %2303, %2308 : i32 loc(#loc487)
    %2310 = arith.select %2225, %2309, %c0_i32 : i32 loc(#loc452)
    %2311 = arith.ori %2226, %2304 : i1
    %2312 = arith.xori %2227, %true : i1
    %2313 = acir.aggregate(%true, %2212, %2310, %2211) : (i1, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.StoreEvent"> loc(#loc488)
    %2314 = arith.cmpi eq, %2209, %c0_i32 : i32 loc(#loc489)
    %2315 = arith.cmpi eq, %2209, %c1_i32 : i32 loc(#loc490)
    %2316 = arith.ori %2314, %2315 : i1
    %2317 = arith.muli %2211, %c8_i32 : i32 loc(#loc786)
    %2318 = arith.subi %2317, %c1_i32 : i32 loc(#loc787)
    %2319 = arith.shli %c1_i32, %2318 : i32 loc(#loc788)
    %2320 = arith.xori %2310, %2319 : i32 loc(#loc789)
    %2321 = arith.subi %2320, %2319 : i32 loc(#loc452)
    %2322 = arith.select %2316, %2321, %2310 : i32 loc(#loc452)
    %2323 = arith.select %2205, %2313, %2203 : !acir.struct<"src_isa_ae2f66cd.StoreEvent"> loc(#loc488)
    %2324 = arith.xori %2205, %true : i1
    %2325 = arith.ori %2324, %2311 : i1
    %2326 = arith.select %2205, %2322, %2202 : i32 loc(#loc452)
    %2327 = arith.andi %2208, %2206 : i1
    %2328 = arith.xori %2224, %true : i1
    %2329 = arith.andi %2327, %2328 : i1
    acir.path %2329 {branch = 205 : i64, kind = "branch", txn = 4 : i64} loc(#loc494)
    %2330 = arith.andi %2327, %2224 : i1
    %2331 = arith.andi %2219, %c3_i32 : i32 loc(#loc495)
    %2332 = arith.muli %2331, %c8_i32 : i32 loc(#loc496)
    acir.path %2330 {branch = 206 : i64, kind = "branch", txn = 4 : i64}
    %value_92, %ready_93 = acir.read(%2330) {branch = 206 : i64, frames = ["top.cpu.memory:advance"], method = "read", port = 0 : i64, resource = "top.cpu.memory.words", txn = 4 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc497)
    %2333 = acir.bind %value_92 : (!acir.array<i32, [1024]>) -> !acir.array<i32, [1024]> loc(#loc498)
    %2334 = "acir.array.index"(%value_92, %2249) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc497)
    %2335 = arith.shli %2308, %2332 : i32 loc(#loc499)
    %2336 = arith.xori %2335, %c-1_i32 : i32 loc(#loc500)
    %2337 = arith.andi %2334, %2336 : i32 loc(#loc497)
    %2338 = acir.extract %value_82 {field = "store_value"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc501)
    %2339 = arith.andi %2338, %2308 : i32 loc(#loc501)
    %2340 = arith.shli %2339, %2332 : i32 loc(#loc502)
    %2341 = arith.ori %2337, %2340 : i32 loc(#loc503)
    %2342 = acir.update %2333, %2341[%2249] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [1024]>, i32, i64) -> !acir.array<i32, [1024]> loc(#loc498)
    %2343 = arith.addi %2331, %2211 : i32 loc(#loc504)
    %2344 = arith.cmpi ugt, %2343, %c4_i32 : i32 loc(#loc504)
    %2345 = arith.andi %2330, %2344 : i1
    %2346 = arith.subi %c32_i32, %2332 : i32 loc(#loc505)
    acir.path %2345 {branch = 207 : i64, kind = "branch", txn = 4 : i64} loc(#loc506)
    %value_94, %ready_95 = acir.read(%2345) {branch = 207 : i64, frames = ["top.cpu.memory:advance"], method = "read", port = 0 : i64, resource = "top.cpu.memory.words", txn = 4 : i64} : (i1) -> (!acir.array<i32, [1024]>, i1) loc(#loc506)
    %2347 = arith.addi %2246, %c1_i32 : i32 loc(#loc507)
    %2348 = arith.extui %2347 : i32 to i64 loc(#loc506)
    %2349 = arith.cmpi ult, %2348, %c1024_i64 : i64 loc(#loc506)
    %2350 = arith.select %2349, %2348, %c0_i64 : i64 loc(#loc506)
    %2351 = "acir.array.index"(%value_94, %2350) : (!acir.array<i32, [1024]>, i64) -> i32 loc(#loc506)
    %2352 = arith.shrui %2308, %2346 : i32 loc(#loc508)
    %2353 = arith.xori %2352, %c-1_i32 : i32 loc(#loc509)
    %2354 = arith.andi %2351, %2353 : i32 loc(#loc506)
    %2355 = arith.shrui %2339, %2346 : i32 loc(#loc510)
    %2356 = arith.ori %2354, %2355 : i32 loc(#loc511)
    %2357 = arith.andi %2248, %2349 : i1
    %2358 = acir.update %2342, %2356[%2350] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [1024]>, i32, i64) -> !acir.array<i32, [1024]> loc(#loc498)
    %2359 = arith.select %2344, %2358, %2342 : !acir.array<i32, [1024]> loc(#loc498)
    %2360 = arith.select %2344, %2357, %2248 : i1
    %2361 = arith.xori %2330, %true : i1 loc(#loc512)
    %2362 = arith.ori %2361, %2360 : i1 loc(#loc512)
    %2363 = acir.aggregate(%true, %2212, %2339, %2211) : (i1, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.StoreEvent"> loc(#loc453)
    %2364 = arith.select %2224, %2363, %2203 : !acir.struct<"src_isa_ae2f66cd.StoreEvent"> loc(#loc453)
    %2365 = arith.select %2206, %2364, %2203 : !acir.struct<"src_isa_ae2f66cd.StoreEvent"> loc(#loc453)
    %2366 = arith.select %2208, %2365, %2203 : !acir.struct<"src_isa_ae2f66cd.StoreEvent"> loc(#loc453)
    %2367 = arith.select %2208, %2323, %2203 : !acir.struct<"src_isa_ae2f66cd.StoreEvent"> loc(#loc488)
    %2368 = arith.xori %2208, %true : i1
    %2369 = arith.ori %2368, %2325 : i1
    %2370 = arith.select %2208, %2326, %2202 : i32 loc(#loc452)
    %2371 = arith.andi %2362, %2369 : i1 loc(#loc513)
    %2372 = acir.extract %value_82 {field = "pc"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc514)
    %2373 = acir.extract %value_82 {field = "word"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc515)
    %2374 = acir.extract %value_82 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc516)
    %2375 = acir.extract %value_82 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc517)
    %2376 = acir.extract %value_82 {field = "stalled"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc518)
    %2377 = acir.extract %value_82 {field = "illegal"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i1 loc(#loc519)
    %2378 = acir.extract %value_82 {field = "system"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc520)
    %2379 = acir.extract %value_82 {field = "expanded"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc521)
    %2380 = acir.extract %value_82 {field = "next_pc"} : (!acir.struct<"src_isa_ae2f66cd.Executed">) -> i32 loc(#loc522)
    %2381 = acir.aggregate(%2204, %2372, %2373, %2374, %2375, %2370, %2376, %2377, %2378, %2379, %2380) : (i1, i32, i32, i32, i1, i32, i1, i1, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.Completed"> loc(#loc523)
    acir.path %2369 {branch = 208 : i64, kind = "branch", txn = 4 : i64}
    %2382 = acir.fire "top.cpu.memory.advance#4" %2371 {successors = [], txn = 4 : i64}
    %2383 = arith.cmpi ugt, %2211, %2215 : i32 loc(#loc462)
    %2384 = arith.cmpi uge, %2219, %c4096_i32 : i32 loc(#loc464)
    %2385 = arith.cmpi ugt, %2211, %2222 : i32 loc(#loc466)
    %2386 = arith.ori %2384, %2385 : i1
    %2387 = arith.select %2214, %2383, %2386 : i1
    %2388 = arith.andi %2205, %2387 : i1
    %2389 = arith.andi %2204, %2388 : i1
    %2390 = arith.andi %2382, %2389 : i1
    acir.effect %2390(%false) {branch = 196 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read"], method = "check", port = 0 : i64, resource = "", txn = 4 : i64} : (i1, i1) -> () loc(#loc467)
    %2391 = arith.andi %2382, %2244 : i1
    acir.effect %2391(%false) {branch = 199 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "check", port = 0 : i64, resource = "", txn = 4 : i64} : (i1, i1) -> () loc(#loc473)
    %2392 = arith.andi %2220, %2223 : i1
    %2393 = arith.select %2214, %2216, %2392 : i1
    %2394 = arith.andi %2205, %2393 : i1
    %2395 = arith.andi %2204, %2394 : i1
    %2396 = arith.andi %2382, %2395 : i1
    acir.effect %2396(%2258) {branch = 197 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "check", port = 0 : i64, resource = "", txn = 4 : i64} : (i1, i1) -> ()
    %2397 = arith.cmpi uge, %2278, %c4096_i32 : i32 loc(#loc482)
    %2398 = arith.andi %2267, %2397 : i1
    %2399 = arith.andi %2262, %2398 : i1
    %2400 = arith.andi %2216, %2399 : i1
    %2401 = arith.andi %2223, %2399 : i1
    %2402 = arith.andi %2220, %2401 : i1
    %2403 = arith.select %2214, %2400, %2402 : i1
    %2404 = arith.andi %2205, %2403 : i1
    %2405 = arith.andi %2204, %2404 : i1
    %2406 = arith.andi %2382, %2405 : i1
    acir.effect %2406(%false) {branch = 203 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "check", port = 0 : i64, resource = "", txn = 4 : i64} : (i1, i1) -> () loc(#loc473)
    %2407 = arith.andi %2216, %2262 : i1
    %2408 = arith.andi %2223, %2262 : i1
    %2409 = arith.andi %2220, %2408 : i1
    %2410 = arith.select %2214, %2407, %2409 : i1
    %2411 = arith.andi %2205, %2410 : i1
    %2412 = arith.andi %2204, %2411 : i1
    %2413 = arith.andi %2382, %2412 : i1
    acir.effect %2413(%2297) {branch = 201 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read", "top.cpu.memory:read_word"], method = "check", port = 0 : i64, resource = "", txn = 4 : i64} : (i1, i1) -> ()
    %2414 = arith.andi %2204, %2205 : i1
    %2415 = arith.andi %2382, %2414 : i1
    acir.effect %2415(%2312) {branch = 195 : i64, frames = ["top.cpu.memory:advance", "top.cpu.memory:read"], method = "check", port = 0 : i64, resource = "", txn = 4 : i64} : (i1, i1) -> ()
    %2416 = arith.ori %2214, %2386 : i1
    %2417 = arith.andi %2206, %2416 : i1
    %2418 = arith.andi %2204, %2417 : i1
    %2419 = arith.andi %2382, %2418 : i1
    acir.effect %2419(%false) {branch = 205 : i64, frames = ["top.cpu.memory:advance"], method = "check", port = 0 : i64, resource = "", txn = 4 : i64} : (i1, i1) -> () loc(#loc494)
    %2420 = arith.andi %2218, %2392 : i1
    %2421 = arith.andi %2206, %2420 : i1
    %2422 = arith.andi %2204, %2421 : i1
    %2423 = arith.andi %2382, %2422 : i1
    acir.effect %2423(%2359) {branch = 206 : i64, frames = ["top.cpu.memory:advance"], method = "write", port = 0 : i64, resource = "top.cpu.memory.words", txn = 4 : i64} : (i1, !acir.array<i32, [1024]>) -> () loc(#loc512)
    acir.effect %2382(%2366) {branch = 194 : i64, frames = ["top.cpu.memory:advance"], method = "write", port = 0 : i64, resource = "top.cpu.memory.store_event", txn = 4 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.StoreEvent">) -> () loc(#loc524)
    acir.effect %2382(%2367) {branch = 194 : i64, frames = ["top.cpu.memory:advance"], method = "write", port = 0 : i64, resource = "top.cpu.memory.load_event", txn = 4 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.StoreEvent">) -> () loc(#loc513)
    %2424 = arith.xori %2204, %true : i1
    %2425 = arith.ori %2397, %2287 : i1
    %2426 = arith.select %2267, %2425, %2271 : i1
    %2427 = arith.andi %2235, %2426 : i1
    %2428 = arith.select %2262, %2427, %2235 : i1
    %2429 = arith.ori %2383, %2428 : i1
    %2430 = arith.andi %2248, %2426 : i1
    %2431 = arith.select %2262, %2430, %2248 : i1
    %2432 = arith.ori %2385, %2431 : i1
    %2433 = arith.ori %2384, %2432 : i1
    %2434 = arith.select %2214, %2429, %2433 : i1
    %2435 = arith.ori %2324, %2434 : i1
    %2436 = arith.ori %2424, %2435 : i1
    %2437 = arith.andi %2382, %2436 : i1
    acir.effect %2437(%2381) {branch = 208 : i64, frames = [], method = "write", port = 0 : i64, resource = "top.cpu.mem_wb", txn = 4 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.Completed">) -> ()
    %2438 = acir.export %c84_i32 {acir.id = "v106", acir.source_type = "u32"} : i32 -> i32 loc(#loc525)
    acir.path %true {branch = 209 : i64, kind = "branch", txn = 5 : i64} loc(#loc526)
    acir.path %true {branch = 209 : i64, frames = ["top.cpu.writeback:advance"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc526)
    %value_96, %ready_97 = acir.read(%true) {branch = 209 : i64, frames = ["top.cpu.writeback:advance"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = 5 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1)
    %2439 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %false, %c0_i32) : (i1, i32, i32, i32, i1, i32) -> !acir.struct<"src_isa_ae2f66cd.Retirement"> loc(#loc527)
    %2440 = acir.extract %value_96 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc528)
    %2441 = acir.extract %value_96 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc529)
    %2442 = arith.andi %2440, %2441 : i1
    %2443 = acir.extract %value_96 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc530)
    %2444 = acir.extract %value_96 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc531)
    acir.path %2442 {branch = 210 : i64, kind = "branch", txn = 5 : i64} loc(#loc532)
    acir.path %2442 {branch = 210 : i64, frames = ["top.cpu.writeback:advance", "top.cpu.rf:write"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 5 : i64} loc(#loc532)
    %2445 = arith.cmpi ne, %2443, %c0_i32 : i32 loc(#loc533)
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc153)
    %2446 = arith.andi %2442, %2445 : i1
    %2447 = arith.extui %2443 : i32 to i64 loc(#loc534)
    %2448 = arith.cmpi ult, %2447, %c32_i64 : i64 loc(#loc534)
    acir.path %2446 {branch = 211 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2449 = arith.cmpi eq, %2447, %c0_i64 : i64 loc(#loc534)
    %2450 = arith.andi %2446, %2449 : i1 loc(#loc534)
    acir.path %2450 {branch = 212 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2451 = arith.cmpi eq, %2447, %c1_i64 : i64 loc(#loc534)
    %2452 = arith.andi %2446, %2451 : i1 loc(#loc534)
    acir.path %2452 {branch = 213 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2453 = arith.cmpi eq, %2447, %c2_i64 : i64 loc(#loc534)
    %2454 = arith.andi %2446, %2453 : i1 loc(#loc534)
    acir.path %2454 {branch = 214 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2455 = arith.cmpi eq, %2447, %c3_i64 : i64 loc(#loc534)
    %2456 = arith.andi %2446, %2455 : i1 loc(#loc534)
    acir.path %2456 {branch = 215 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2457 = arith.cmpi eq, %2447, %c4_i64 : i64 loc(#loc534)
    %2458 = arith.andi %2446, %2457 : i1 loc(#loc534)
    acir.path %2458 {branch = 216 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2459 = arith.cmpi eq, %2447, %c5_i64 : i64 loc(#loc534)
    %2460 = arith.andi %2446, %2459 : i1 loc(#loc534)
    acir.path %2460 {branch = 217 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2461 = arith.cmpi eq, %2447, %c6_i64 : i64 loc(#loc534)
    %2462 = arith.andi %2446, %2461 : i1 loc(#loc534)
    acir.path %2462 {branch = 218 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2463 = arith.cmpi eq, %2447, %c7_i64 : i64 loc(#loc534)
    %2464 = arith.andi %2446, %2463 : i1 loc(#loc534)
    acir.path %2464 {branch = 219 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2465 = arith.cmpi eq, %2447, %c8_i64 : i64 loc(#loc534)
    %2466 = arith.andi %2446, %2465 : i1 loc(#loc534)
    acir.path %2466 {branch = 220 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2467 = arith.cmpi eq, %2447, %c9_i64 : i64 loc(#loc534)
    %2468 = arith.andi %2446, %2467 : i1 loc(#loc534)
    acir.path %2468 {branch = 221 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2469 = arith.cmpi eq, %2447, %c10_i64 : i64 loc(#loc534)
    %2470 = arith.andi %2446, %2469 : i1 loc(#loc534)
    acir.path %2470 {branch = 222 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2471 = arith.cmpi eq, %2447, %c11_i64 : i64 loc(#loc534)
    %2472 = arith.andi %2446, %2471 : i1 loc(#loc534)
    acir.path %2472 {branch = 223 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2473 = arith.cmpi eq, %2447, %c12_i64 : i64 loc(#loc534)
    %2474 = arith.andi %2446, %2473 : i1 loc(#loc534)
    acir.path %2474 {branch = 224 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2475 = arith.cmpi eq, %2447, %c13_i64 : i64 loc(#loc534)
    %2476 = arith.andi %2446, %2475 : i1 loc(#loc534)
    acir.path %2476 {branch = 225 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2477 = arith.cmpi eq, %2447, %c14_i64 : i64 loc(#loc534)
    %2478 = arith.andi %2446, %2477 : i1 loc(#loc534)
    acir.path %2478 {branch = 226 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2479 = arith.cmpi eq, %2447, %c15_i64 : i64 loc(#loc534)
    %2480 = arith.andi %2446, %2479 : i1 loc(#loc534)
    acir.path %2480 {branch = 227 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2481 = arith.cmpi eq, %2447, %c16_i64 : i64 loc(#loc534)
    %2482 = arith.andi %2446, %2481 : i1 loc(#loc534)
    acir.path %2482 {branch = 228 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2483 = arith.cmpi eq, %2447, %c17_i64 : i64 loc(#loc534)
    %2484 = arith.andi %2446, %2483 : i1 loc(#loc534)
    acir.path %2484 {branch = 229 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2485 = arith.cmpi eq, %2447, %c18_i64 : i64 loc(#loc534)
    %2486 = arith.andi %2446, %2485 : i1 loc(#loc534)
    acir.path %2486 {branch = 230 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2487 = arith.cmpi eq, %2447, %c19_i64 : i64 loc(#loc534)
    %2488 = arith.andi %2446, %2487 : i1 loc(#loc534)
    acir.path %2488 {branch = 231 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2489 = arith.cmpi eq, %2447, %c20_i64 : i64 loc(#loc534)
    %2490 = arith.andi %2446, %2489 : i1 loc(#loc534)
    acir.path %2490 {branch = 232 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2491 = arith.cmpi eq, %2447, %c21_i64 : i64 loc(#loc534)
    %2492 = arith.andi %2446, %2491 : i1 loc(#loc534)
    acir.path %2492 {branch = 233 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2493 = arith.cmpi eq, %2447, %c22_i64 : i64 loc(#loc534)
    %2494 = arith.andi %2446, %2493 : i1 loc(#loc534)
    acir.path %2494 {branch = 234 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2495 = arith.cmpi eq, %2447, %c23_i64 : i64 loc(#loc534)
    %2496 = arith.andi %2446, %2495 : i1 loc(#loc534)
    acir.path %2496 {branch = 235 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2497 = arith.cmpi eq, %2447, %c24_i64 : i64 loc(#loc534)
    %2498 = arith.andi %2446, %2497 : i1 loc(#loc534)
    acir.path %2498 {branch = 236 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2499 = arith.cmpi eq, %2447, %c25_i64 : i64 loc(#loc534)
    %2500 = arith.andi %2446, %2499 : i1 loc(#loc534)
    acir.path %2500 {branch = 237 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2501 = arith.cmpi eq, %2447, %c26_i64 : i64 loc(#loc534)
    %2502 = arith.andi %2446, %2501 : i1 loc(#loc534)
    acir.path %2502 {branch = 238 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2503 = arith.cmpi eq, %2447, %c27_i64 : i64 loc(#loc534)
    %2504 = arith.andi %2446, %2503 : i1 loc(#loc534)
    acir.path %2504 {branch = 239 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2505 = arith.cmpi eq, %2447, %c28_i64 : i64 loc(#loc534)
    %2506 = arith.andi %2446, %2505 : i1 loc(#loc534)
    acir.path %2506 {branch = 240 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2507 = arith.cmpi eq, %2447, %c29_i64 : i64 loc(#loc534)
    %2508 = arith.andi %2446, %2507 : i1 loc(#loc534)
    acir.path %2508 {branch = 241 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2509 = arith.cmpi eq, %2447, %c30_i64 : i64 loc(#loc534)
    %2510 = arith.andi %2446, %2509 : i1 loc(#loc534)
    acir.path %2510 {branch = 242 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2511 = arith.cmpi eq, %2447, %c31_i64 : i64 loc(#loc534)
    %2512 = arith.andi %2446, %2511 : i1 loc(#loc534)
    acir.path %2512 {branch = 243 : i64, kind = "branch", txn = 5 : i64} loc(#loc534)
    %2513 = acir.extract %value_96 {field = "pc"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc535)
    %2514 = arith.cmpi ult, %2513, %2438 : i32 loc(#loc535)
    %2515 = arith.andi %2440, %2514 : i1
    acir.path %2515 {branch = 244 : i64, kind = "branch", txn = 5 : i64} loc(#loc536)
    %value_98, %ready_99 = acir.read(%2515) {branch = 244 : i64, frames = ["top.cpu.writeback:advance"], method = "read", port = 0 : i64, resource = "top.cpu.writeback.retired", txn = 5 : i64} : (i1) -> (i32, i1) loc(#loc536)
    %2516 = arith.addi %value_98, %c1_i32 : i32 loc(#loc536)
    %2517 = acir.extract %value_96 {field = "word"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc537)
    %2518 = arith.andi %2441, %2445 : i1
    %2519 = acir.aggregate(%true, %2513, %2517, %2443, %2518, %2444) : (i1, i32, i32, i32, i1, i32) -> !acir.struct<"src_isa_ae2f66cd.Retirement"> loc(#loc527)
    %2520 = arith.select %2515, %2519, %2439 : !acir.struct<"src_isa_ae2f66cd.Retirement"> loc(#loc527)
    %2521 = arith.xori %2440, %true : i1 loc(#loc534)
    %2522 = arith.xori %2441, %true : i1 loc(#loc534)
    %2523 = arith.cmpi eq, %2443, %c0_i32 : i32 loc(#loc790)
    %2524 = arith.ori %2523, %2448 : i1 loc(#loc534)
    %2525 = arith.ori %2522, %2524 : i1 loc(#loc534)
    %2526 = arith.ori %2521, %2525 : i1 loc(#loc534)
    %2527 = acir.fire "top.cpu.writeback.advance#5" %2526 {successors = [], txn = 5 : i64}
    %2528 = arith.andi %2440, %2518 : i1
    %2529 = arith.andi %2527, %2528 : i1
    acir.effect %2529(%2444, %2447) {array = "top.cpu.rf.registers", branch = 211 : i64, frames = ["top.cpu.writeback:advance", "top.cpu.rf:write"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = 5 : i64} : (i1, i32, i64) -> () loc(#loc534)
    %2530 = arith.andi %2527, %2515 : i1
    acir.effect %2530(%2516) {branch = 244 : i64, frames = ["top.cpu.writeback:advance"], method = "write", port = 0 : i64, resource = "top.cpu.writeback.retired", txn = 5 : i64} : (i1, i32) -> () loc(#loc538)
    acir.effect %2527(%2520) {branch = 209 : i64, frames = ["top.cpu.writeback:advance"], method = "write", port = 0 : i64, resource = "top.cpu.writeback.retirement", txn = 5 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.Retirement">) -> () loc(#loc539)
    acir.path %true {branch = 245 : i64, kind = "branch", txn = 6 : i64} loc(#loc540)
    acir.path %true {branch = 245 : i64, frames = ["top.cpu:observe"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 6 : i64} loc(#loc540)
    %2531 = acir.fire "top.cpu.observe#6" %643 {successors = [], txn = 6 : i64}
    acir.effect %2531(%644) {branch = 245 : i64, frames = ["top.cpu:observe"], method = "write", port = 0 : i64, resource = "top.cpu.last_control", txn = 6 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.Control">) -> () loc(#loc541)
    acir.path %true {branch = 246 : i64, kind = "branch", txn = 7 : i64} loc(#loc542)
    acir.path %true {branch = 246 : i64, frames = ["top.cpu:environment"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64} loc(#loc542)
    %2532 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %c0_i32) : (i1, i32, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.SystemEvent"> loc(#loc543)
    %value_100, %ready_101 = acir.read(%true) {branch = 246 : i64, frames = ["top.cpu:environment"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = 7 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1) loc(#loc544)
    %2533 = acir.extract %value_100 {field = "valid"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i1 loc(#loc544)
    %2534 = arith.andi %2533, %536 : i1
    acir.path %2534 {branch = 247 : i64, kind = "branch", txn = 7 : i64} loc(#loc545)
    %value_102, %ready_103 = acir.read(%2534) {branch = 247 : i64, frames = ["top.cpu:environment"], method = "read", port = 0 : i64, resource = "top.cpu.halted", txn = 7 : i64} : (i1) -> (i1, i1) loc(#loc545)
    %2535 = arith.xori %value_102, %true : i1 loc(#loc546)
    %2536 = arith.andi %2534, %2535 : i1
    acir.path %2536 {branch = 248 : i64, kind = "branch", txn = 7 : i64} loc(#loc547)
    %value_104, %ready_105 = acir.read(%2536) {branch = 248 : i64, frames = ["top.cpu:environment"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = 7 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1) loc(#loc547)
    %2537 = acir.extract %value_104 {field = "instruction"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> !acir.struct<"src_isa_ae2f66cd.Instruction"> loc(#loc548)
    %2538 = acir.extract %2537 {field = "system"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i32 loc(#loc549)
    %2539 = arith.cmpi ne, %2538, %c0_i32 : i32 loc(#loc549)
    %2540 = acir.extract %2537 {field = "illegal"} : (!acir.struct<"src_isa_ae2f66cd.Instruction">) -> i1 loc(#loc550)
    %2541 = arith.ori %2539, %2540 : i1
    %2542 = arith.andi %2536, %2541 : i1
    acir.path %2542 {branch = 249 : i64, kind = "branch", txn = 7 : i64} loc(#loc551)
    %value_106, %ready_107 = acir.read(%2542) {branch = 249 : i64, frames = ["top.cpu:environment"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = 7 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1) loc(#loc551)
    acir.path %2542 {branch = 249 : i64, frames = ["top.cpu:environment", "top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64}
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc153)
    %2543 = acir.extract %value_106 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc154)
    %2544 = acir.extract %value_106 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc155)
    %2545 = arith.cmpi eq, %2544, %c17_i32 : i32 loc(#loc155)
    %2546 = arith.andi %2543, %2545 : i1
    %2547 = arith.xori %2546, %true : i1
    %2548 = arith.select %2546, %2547, %false : i1
    %2549 = arith.andi %2542, %2547 : i1
    acir.path %2549 {branch = 250 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    %value_108, %ready_109 = acir.read(%2549, %c17_i64) {array = "top.cpu.rf.registers", branch = 250 : i64, frames = ["top.cpu:environment", "top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = 7 : i64} : (i1, i64) -> (i32, i1) loc(#loc156)
    acir.path %false {branch = 251 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 252 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 253 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 254 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 255 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 256 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 257 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 258 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 259 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 260 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 261 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 262 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 263 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 264 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 265 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 266 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 267 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %2549 {branch = 268 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 269 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 270 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 271 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 272 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 273 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 274 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 275 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 276 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 277 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 278 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 279 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 280 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 281 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 282 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    %2550 = acir.extract %value_106 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc157)
    %2551 = arith.select %2546, %2550, %value_108 : i32
    %2552 = arith.xori %2548, %true : i1
    %value_110, %ready_111 = acir.read(%2542) {branch = 249 : i64, frames = ["top.cpu:environment"], method = "read", port = 0 : i64, resource = "top.cpu.mem_wb", txn = 7 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Completed">, i1) loc(#loc552)
    acir.path %2542 {branch = 249 : i64, frames = ["top.cpu:environment", "top.cpu.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 7 : i64}
    "acir.resource.array"() <{name = "top.cpu.rf.registers", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], shape = [32]}> : () -> () loc(#loc153)
    %2553 = acir.extract %value_110 {field = "writes"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i1 loc(#loc154)
    %2554 = acir.extract %value_110 {field = "rd"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc155)
    %2555 = arith.cmpi eq, %2554, %c10_i32 : i32 loc(#loc155)
    %2556 = arith.andi %2553, %2555 : i1
    %2557 = arith.xori %2556, %true : i1
    %2558 = arith.select %2556, %2557, %false : i1
    %2559 = arith.andi %2542, %2557 : i1
    acir.path %2559 {branch = 283 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    %value_112, %ready_113 = acir.read(%2559, %c10_i64) {array = "top.cpu.rf.registers", branch = 283 : i64, frames = ["top.cpu:environment", "top.cpu.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.cpu.rf.registers.0", resources = ["top.cpu.rf.registers.0", "top.cpu.rf.registers.1", "top.cpu.rf.registers.2", "top.cpu.rf.registers.3", "top.cpu.rf.registers.4", "top.cpu.rf.registers.5", "top.cpu.rf.registers.6", "top.cpu.rf.registers.7", "top.cpu.rf.registers.8", "top.cpu.rf.registers.9", "top.cpu.rf.registers.10", "top.cpu.rf.registers.11", "top.cpu.rf.registers.12", "top.cpu.rf.registers.13", "top.cpu.rf.registers.14", "top.cpu.rf.registers.15", "top.cpu.rf.registers.16", "top.cpu.rf.registers.17", "top.cpu.rf.registers.18", "top.cpu.rf.registers.19", "top.cpu.rf.registers.20", "top.cpu.rf.registers.21", "top.cpu.rf.registers.22", "top.cpu.rf.registers.23", "top.cpu.rf.registers.24", "top.cpu.rf.registers.25", "top.cpu.rf.registers.26", "top.cpu.rf.registers.27", "top.cpu.rf.registers.28", "top.cpu.rf.registers.29", "top.cpu.rf.registers.30", "top.cpu.rf.registers.31"], txn = 7 : i64} : (i1, i64) -> (i32, i1) loc(#loc156)
    acir.path %false {branch = 284 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 285 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 286 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 287 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 288 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 289 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 290 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 291 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 292 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 293 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %2559 {branch = 294 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 295 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 296 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 297 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 298 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 299 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 300 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 301 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 302 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 303 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 304 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 305 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 306 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 307 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 308 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 309 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 310 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 311 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 312 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 313 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 314 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    acir.path %false {branch = 315 : i64, kind = "branch", txn = 7 : i64} loc(#loc156)
    %2560 = acir.extract %value_110 {field = "value"} : (!acir.struct<"src_isa_ae2f66cd.Completed">) -> i32 loc(#loc157)
    %2561 = arith.select %2556, %2560, %value_112 : i32
    %2562 = arith.xori %2558, %true : i1
    %value_114, %ready_115 = acir.read(%2542) {branch = 249 : i64, frames = ["top.cpu:environment"], method = "read", port = 0 : i64, resource = "top.cpu.id_ex", txn = 7 : i64} : (i1) -> (!acir.struct<"src_isa_ae2f66cd.Decoded">, i1) loc(#loc553)
    %2563 = acir.extract %value_114 {field = "pc"} : (!acir.struct<"src_isa_ae2f66cd.Decoded">) -> i32 loc(#loc553)
    %2564 = arith.cmpi eq, %2538, %c1_i32 : i32 loc(#loc554)
    %2565 = arith.cmpi eq, %2551, %c1_i32 : i32 loc(#loc791)
    %2566 = arith.cmpi eq, %2551, %c4_i32 : i32 loc(#loc792)
    %2567 = arith.ori %2565, %2566 : i1 loc(#loc793)
    %2568 = arith.cmpi eq, %2551, %c11_i32 : i32 loc(#loc794)
    %2569 = arith.ori %2567, %2568 : i1 loc(#loc793)
    %2570 = arith.cmpi eq, %2551, %c10_i32 : i32 loc(#loc795)
    %2571 = arith.ori %2569, %2570 : i1 loc(#loc793)
    %2572 = arith.cmpi eq, %2551, %c93_i32 : i32 loc(#loc796)
    %2573 = arith.ori %2571, %2572 : i1 loc(#loc793)
    %2574 = arith.xori %2573, %true : i1 loc(#loc556)
    %2575 = arith.andi %2564, %2574 : i1
    %2576 = arith.select %2575, %c4_i32, %2538 : i32
    %2577 = arith.select %2540, %c3_i32, %2576 : i32
    %2578 = acir.aggregate(%true, %2563, %2577, %2551, %2561) : (i1, i32, i32, i32, i32) -> !acir.struct<"src_isa_ae2f66cd.SystemEvent"> loc(#loc543)
    %2579 = arith.cmpi eq, %2538, %c2_i32 : i32 loc(#loc557)
    %2580 = arith.ori %2540, %2579 : i1
    %2581 = arith.ori %2580, %2570 : i1
    %2582 = arith.ori %2581, %2572 : i1
    %2583 = arith.ori %2582, %2574 : i1
    %2584 = arith.andi %2542, %2583 : i1
    acir.path %2584 {branch = 316 : i64, kind = "branch", txn = 7 : i64} loc(#loc558)
    %2585 = arith.andi %2572, %2564 : i1
    %2586 = arith.select %2585, %2561, %c0_i32 : i32
    %2587 = arith.select %2541, %2578, %2532 : !acir.struct<"src_isa_ae2f66cd.SystemEvent"> loc(#loc543)
    %2588 = arith.select %2536, %2587, %2532 : !acir.struct<"src_isa_ae2f66cd.SystemEvent"> loc(#loc543)
    %2589 = acir.fire "top.cpu.environment#7" %true {successors = [], txn = 7 : i64}
    %2590 = arith.andi %2535, %2541 : i1
    %2591 = arith.andi %1659, %2590 : i1
    %2592 = arith.andi %518, %2591 : i1
    %2593 = arith.select %515, %2592, %2590 : i1
    %2594 = arith.select %512, %2593, %2590 : i1
    %2595 = arith.andi %2533, %2594 : i1
    %2596 = arith.andi %2589, %2595 : i1
    acir.effect %2596(%2552) {branch = 249 : i64, frames = ["top.cpu:environment", "top.cpu.rf:read"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    acir.effect %2596(%2562) {branch = 249 : i64, frames = ["top.cpu:environment", "top.cpu.rf:read"], method = "check", port = 0 : i64, resource = "", txn = 7 : i64} : (i1, i1) -> ()
    %2597 = arith.cmpi ne, %2551, %c1_i32 : i32 loc(#loc791)
    %2598 = arith.cmpi ne, %2551, %c4_i32 : i32 loc(#loc792)
    %2599 = arith.cmpi ne, %2551, %c11_i32 : i32 loc(#loc794)
    %2600 = arith.andi %2598, %2599 : i1
    %2601 = arith.andi %2597, %2600 : i1
    %2602 = arith.ori %2572, %2601 : i1
    %2603 = arith.ori %2570, %2602 : i1
    %2604 = arith.ori %2579, %2603 : i1
    %2605 = arith.ori %2540, %2604 : i1
    %2606 = arith.select %2539, %2605, %2540 : i1
    %2607 = arith.andi %2535, %2606 : i1
    %2608 = arith.andi %1659, %2607 : i1
    %2609 = arith.andi %518, %2608 : i1
    %2610 = arith.select %515, %2609, %2607 : i1
    %2611 = arith.select %512, %2610, %2607 : i1
    %2612 = arith.andi %2533, %2611 : i1
    %2613 = arith.andi %2589, %2612 : i1
    acir.effect %2613(%true) {branch = 316 : i64, frames = ["top.cpu:environment"], method = "write", port = 0 : i64, resource = "top.cpu.halted", txn = 7 : i64} : (i1, i1) -> () loc(#loc558)
    acir.effect %2613(%2586) {branch = 316 : i64, frames = ["top.cpu:environment"], method = "write", port = 0 : i64, resource = "top.cpu.exit_status", txn = 7 : i64} : (i1, i32) -> () loc(#loc559)
    acir.effect %2589(%2588) {branch = 246 : i64, frames = ["top.cpu:environment"], method = "write", port = 0 : i64, resource = "top.cpu.system_event", txn = 7 : i64} : (i1, !acir.struct<"src_isa_ae2f66cd.SystemEvent">) -> () loc(#loc560)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":89:16)
#loc2 = loc("acir.binding:top.cpu_body:instruction")
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":120:21)
#loc4 = loc("acir.binding:top.cpu_body.0.call.4.4:executing")
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":121:16)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":121:33)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":121:52)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":122:29)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":122:13)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":122:64)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":122:48)
#loc12 = loc("acir.binding:top.cpu_body:stall")
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":141:31)
#loc14 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.5.16:index")
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":252:8)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":253:12)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":253:29)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":253:47)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":255:12)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":255:32)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":255:53)
#loc22 = loc("acir.binding:top.cpu_body.0.call.5.9:source_a")
#loc23 = loc("acir.binding:top.cpu_body.0.call.5.9:source_b")
#loc24 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:original")
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":261:8)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":262:16)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":263:8)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":264:16)
#loc29 = loc("acir.binding:top.cpu_body.0.call.5.9:lhs")
#loc30 = loc("acir.binding:top.cpu_body.0.call.5.9:rhs")
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":145:16)
#loc32 = loc("acir.binding:top.cpu_body:operands")
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":156:12)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":156:29)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":157:13)
#loc36 = loc("acir.binding:top.cpu_body.0.call.8.12:executing")
#loc37 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27:pc")
#loc38 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27:opcode")
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":202:8)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":203:16)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":204:8)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":204:24)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":204:41)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":206:8)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":207:17)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":207:16)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":208:8)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":208:23)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":210:8)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":210:25)
#loc51 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.8.86:function")
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":211:16)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":269:8)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":270:16)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":270:30)
#loc56 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.8.86:wide")
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":271:23)
#loc58 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.8.86:high")
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":272:12)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":273:20)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":274:13)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":274:30)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":274:50)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":274:49)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":276:31)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":276:30)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":279:8)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":280:30)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":280:47)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":281:8)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":282:16)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":283:8)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":284:16)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":289:9)
#loc75 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.8.86:a")
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":290:9)
#loc77 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.8.86:b")
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":291:8)
#loc79 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.8.86:quotient")
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":293:32)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":293:16)
#loc82 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.8.86:remainder")
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":295:12)
#loc84 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.8.86:negative_a")
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":212:24)
#loc86 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27:operand")
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":216:54)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":216:84)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":216:20)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":218:28)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":218:20)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":219:12)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":190:13)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":220:25)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":190:34)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":190:12)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":221:12)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":222:25)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":224:20)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":226:89)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":195:8)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":226:20)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":196:43)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":196:28)
#loc105 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.27.0.call.10.199:result")
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":227:12)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":228:20)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":229:16)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":205:16)
#loc110 = loc("acir.binding:top.cpu_body.0.call.8.12:result")
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":162:20)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":163:37)
#loc113 = loc("acir.binding:top.cpu_body.0.call.8.12:jump")
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":163:72)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":234:8)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":238:16)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":240:16)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":242:16)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":244:16)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":244:20)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":246:16)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":247:5)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":248:12)
#loc124 = loc("acir.binding:top.cpu_body.0.call.8.12:redirect")
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":164:17)
#loc126 = loc("acir.binding:top.cpu_body.0.call.8.12:value")
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":165:57)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":165:70)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":165:84)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":166:32)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":166:48)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":166:85)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":166:138)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":166:156)
#loc135 = loc("acir.binding:top.cpu_body.0.call.8.12:next_packet")
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":167:16)
#loc137 = loc("acir.binding:top.cpu_body:result")
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":127:16)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":127:33)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":127:69)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":127:86)
#loc142 = loc("acir.binding:top.cpu_body:wait")
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":313:26)
#loc144 = loc("acir.binding:top.cpu_body:instruction_ex")
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:20)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:31)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:48)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:74)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:105)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:136)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:162)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:150)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":15:61)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":23:12)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":23:33)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":25:16)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":24:20)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:191)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:179)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:240)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:228)
#loc162 = loc("acir.binding:top.cpu_body.0.call.16.67:number")
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:212)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":393:12)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":393:27)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":393:42)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":393:58)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":393:74)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":314:208)
#loc170 = loc("acir.binding:top.cpu_body:stopping")
#loc171 = loc("acir.binding:top.cpu_body:control")
#loc172 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2:control")
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":75:16)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":41:12)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":44:16)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":45:23)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":45:27)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":45:41)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":45:40)
#loc180 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2:raw")
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":46:21)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":46:20)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":46:38)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":47:34)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":47:39)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":47:38)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":48:49)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":48:62)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":48:61)
#loc190 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2:length")
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":49:22)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":49:17)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":50:20)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":51:12)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":56:12)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":56:27)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":56:26)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":62:15)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":62:19)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":62:33)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":62:32)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":63:13)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":63:12)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":63:31)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":63:30)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":64:16)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":65:40)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":65:44)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":65:70)
#loc210 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2:packet")
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":68:26)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":68:31)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":68:30)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":70:32)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":319:9)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":319:8)
#loc217 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:h")
#loc218 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:quadrant")
#loc219 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:function")
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":324:10)
#loc221 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:rd")
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":325:10)
#loc223 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:rt")
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":326:21)
#loc225 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:rs_prime")
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":327:21)
#loc227 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:rt_prime")
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":328:31)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":328:30)
#loc230 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.17.69:value")
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":134:13)
#loc232 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:immediate")
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":330:8)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":338:10)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":367:10)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":368:12)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":371:14)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":375:14)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":386:14)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":387:21)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":387:20)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":387:44)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":387:43)
#loc244 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:imm")
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":307:14)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":307:13)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":307:40)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":307:12)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":307:74)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":307:73)
#loc251 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:word")
#loc252 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":376:17)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":376:16)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":382:18)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":303:42)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":303:12)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":303:67)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":383:35)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":299:12)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":383:48)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":373:48)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":373:47)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":373:23)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":373:70)
#loc265 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":373:69)
#loc266 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":299:14)
#loc267 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":299:13)
#loc268 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":341:14)
#loc269 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":341:31)
#loc270 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":346:14)
#loc271 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":364:31)
#loc272 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:20)
#loc273 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:44)
#loc274 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:43)
#loc275 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:19)
#loc276 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:67)
#loc277 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:66)
#loc278 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:89)
#loc279 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:88)
#loc280 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:111)
#loc281 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":365:110)
#loc282 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.22.481:f")
#loc283 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":366:66)
#loc284 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.22.481:imm")
#loc285 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:14)
#loc286 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:13)
#loc287 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:40)
#loc288 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:39)
#loc289 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:12)
#loc290 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:65)
#loc291 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:78)
#loc292 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:91)
#loc293 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:90)
#loc294 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:116)
#loc295 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":315:115)
#loc296 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":355:16)
#loc297 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":355:34)
#loc298 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":358:18)
#loc299 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":362:26)
#loc300 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":362:45)
#loc301 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":362:64)
#loc302 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268:f")
#loc303 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.21.412:upper")
#loc304 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":303:13)
#loc305 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":303:29)
#loc306 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":303:55)
#loc307 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.18.110:imm")
#loc308 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":347:16)
#loc309 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":351:18)
#loc310 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":352:25)
#loc311 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":352:24)
#loc312 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":348:24)
#loc313 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":348:48)
#loc314 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":348:47)
#loc315 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":348:23)
#loc316 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":348:70)
#loc317 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":348:69)
#loc318 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":348:91)
#loc319 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":349:20)
#loc320 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":350:50)
#loc321 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:20)
#loc322 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:45)
#loc323 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:44)
#loc324 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:19)
#loc325 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:68)
#loc326 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:67)
#loc327 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:90)
#loc328 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:89)
#loc329 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:113)
#loc330 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:112)
#loc331 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:134)
#loc332 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:157)
#loc333 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":342:156)
#loc334 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.20.247:rd")
#loc335 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":343:56)
#loc336 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.20.247:imm")
#loc337 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:14)
#loc338 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:13)
#loc339 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:40)
#loc340 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:39)
#loc341 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:12)
#loc342 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:68)
#loc343 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:67)
#loc344 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:93)
#loc345 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":311:111)
#loc346 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":336:43)
#loc347 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":336:42)
#loc348 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":336:19)
#loc349 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":336:65)
#loc350 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":337:82)
#loc351 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":337:20)
#loc352 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":299:61)
#loc353 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":332:21)
#loc354 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":332:20)
#loc355 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":332:44)
#loc356 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":332:43)
#loc357 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":332:19)
#loc358 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":332:66)
#loc359 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":333:16)
#loc360 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":70:51)
#loc361 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":71:9)
#loc362 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":70:82)
#loc363 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":70:97)
#loc364 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":59:36)
#loc365 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":59:46)
#loc366 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":54:13)
#loc367 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":66:17)
#loc368 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":60:13)
#loc369 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":109:9)
#loc370 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":85:30)
#loc371 = loc("acir.binding:top.cpu.decode_stage_body.0.call.25.2.0.call.25.7:word")
#loc372 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":85:23)
#loc373 = loc("acir.binding:top.cpu.decode_stage_body.0.call.25.2.0.call.25.7:opcode")
#loc374 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":140:16)
#loc375 = loc("acir.binding:top.cpu.decode_stage_body.0.call.25.2.0.call.25.7:function")
#loc376 = loc("acir.binding:top.cpu.decode_stage_body.0.call.25.2.0.call.25.7:upper")
#loc377 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":145:22)
#loc378 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":146:23)
#loc379 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":147:8)
#loc380 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":147:24)
#loc381 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":147:40)
#loc382 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":148:27)
#loc383 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":152:10)
#loc384 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":155:10)
#loc385 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":155:25)
#loc386 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":165:10)
#loc387 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":165:26)
#loc388 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":169:10)
#loc389 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":174:10)
#loc390 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":178:10)
#loc391 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":179:17)
#loc392 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":179:32)
#loc393 = loc("acir.binding:top.cpu.decode_stage_body.0.call.25.2.0.call.25.7:legal")
#loc394 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":180:9)
#loc395 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":172:22)
#loc396 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":172:44)
#loc397 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":172:43)
#loc398 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":172:21)
#loc399 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":172:73)
#loc400 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":172:72)
#loc401 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":172:100)
#loc402 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":172:99)
#loc403 = loc("acir.binding:top.cpu.decode_stage_body.0.call.25.2.0.call.25.7:immediate")
#loc404 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":173:33)
#loc405 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":134:12)
#loc406 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":168:33)
#loc407 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":162:34)
#loc408 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":162:51)
#loc409 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":163:22)
#loc410 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":163:44)
#loc411 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":163:43)
#loc412 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":163:21)
#loc413 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":163:70)
#loc414 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":163:69)
#loc415 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":163:97)
#loc416 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":163:96)
#loc417 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":164:33)
#loc418 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":160:46)
#loc419 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":160:33)
#loc420 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":156:52)
#loc421 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":156:69)
#loc422 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":156:86)
#loc423 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":157:33)
#loc424 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":153:18)
#loc425 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":153:36)
#loc426 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":153:73)
#loc427 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":153:122)
#loc428 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":151:86)
#loc429 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":182:27)
#loc430 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":182:5)
#loc431 = loc("acir.binding:top.cpu.decode_stage_body.0.call.25.2.0.call.25.7:instruction")
#loc432 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":85:9)
#loc433 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":110:16)
#loc434 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":97:22)
#loc435 = loc("acir.binding:top.cpu.decode_stage_body.0.call.27.6:bubble")
#loc436 = loc("acir.binding:top.cpu.decode_stage_body.0.call.27.6:decoded")
#loc437 = loc("acir.binding:top.cpu_body.0.call.15.49:index")
#loc438 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":21:12)
#loc439 = loc("acir.binding:top.cpu.decode_stage_body.0.call.27.6:lhs")
#loc440 = loc("acir.binding:top.cpu.decode_stage_body.0.call.27.6:rhs")
#loc441 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":105:24)
#loc442 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":105:38)
#loc443 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":105:49)
#loc444 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":105:88)
#loc445 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":105:104)
#loc446 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":105:121)
#loc447 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":105:16)
#loc448 = loc("acir.binding:top.cpu.execute_body.0.call.28.3:packet")
#loc449 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":178:16)
#loc450 = loc("acir.binding:top.cpu.execute_body.0.call.28.3:bubble")
#loc451 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":243:16)
#loc452 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:value")
#loc453 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:event")
#loc454 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":215:12)
#loc455 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":215:30)
#loc456 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":215:45)
#loc457 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":216:26)
#loc458 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:size")
#loc459 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:address")
#loc460 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":200:13)
#loc461 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":200:40)
#loc462 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":200:32)
#loc463 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":200:60)
#loc464 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":200:80)
#loc465 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":200:114)
#loc466 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":200:106)
#loc467 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":206:9)
#loc468 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":201:22)
#loc469 = loc("acir.binding:top.cpu.memory_body.0.call.29.2.0.call.29.42:shift")
#loc470 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":202:21)
#loc471 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":192:20)
#loc472 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":192:25)
#loc473 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":195:9)
#loc474 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":194:20)
#loc475 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":194:26)
#loc476 = loc("acir.binding:top.cpu.memory_body.0.call.29.2.0.call.29.42:value")
#loc477 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":203:16)
#loc478 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":204:45)
#loc479 = loc("acir.binding:top.cpu.memory_body.0.call.29.2.0.call.29.42.0.call.29.42:address")
#loc480 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":204:34)
#loc481 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":191:12)
#loc482 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":193:32)
#loc483 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":204:76)
#loc484 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":205:36)
#loc485 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":205:60)
#loc486 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:mask")
#loc487 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":205:20)
#loc488 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:loaded")
#loc489 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":221:20)
#loc490 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":221:42)
#loc491 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.17.69:bits")
#loc492 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/isa.py":133:18)
#loc493 = loc("acir.binding:top.cpu.fetch_body.0.call.17.2.0.call.17.268.0.call.17.69:sign")
#loc494 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":237:21)
#loc495 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":227:30)
#loc496 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:shift")
#loc497 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":230:39)
#loc498 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:updated")
#loc499 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":230:69)
#loc500 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":230:55)
#loc501 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":230:90)
#loc502 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":230:89)
#loc503 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":230:38)
#loc504 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":231:24)
#loc505 = loc("acir.binding:top.cpu.memory_body.0.call.29.2:high_shift")
#loc506 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":233:47)
#loc507 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":233:53)
#loc508 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":233:81)
#loc509 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":233:67)
#loc510 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":233:106)
#loc511 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":233:46)
#loc512 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":234:21)
#loc513 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":239:9)
#loc514 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:40)
#loc515 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:51)
#loc516 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:64)
#loc517 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:75)
#loc518 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:97)
#loc519 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:113)
#loc520 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:129)
#loc521 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:144)
#loc522 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:161)
#loc523 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":240:16)
#loc524 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":238:9)
#loc525 = loc("acir.binding:top.cpu.writeback_body.0.call.32.3:code_size")
#loc526 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":264:9)
#loc527 = loc("acir.binding:top.cpu.writeback_body.0.call.32.3:event")
#loc528 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":256:12)
#loc529 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":256:29)
#loc530 = loc("acir.binding:top.cpu.writeback_body.0.call.32.3.0.call.32.23:index")
#loc531 = loc("acir.binding:top.cpu.writeback_body.0.call.32.3.0.call.32.23:value")
#loc532 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":257:13)
#loc533 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":29:12)
#loc534 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":30:13)
#loc535 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":258:29)
#loc536 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":259:23)
#loc537 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":260:49)
#loc538 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":259:13)
#loc539 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":261:9)
#loc540 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":323:9)
#loc541 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":290:9)
#loc542 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":324:9)
#loc543 = loc("acir.binding:top.cpu_body.0.call.35.108:event")
#loc544 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":295:12)
#loc545 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":295:45)
#loc546 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":295:41)
#loc547 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":296:27)
#loc548 = loc("acir.binding:top.cpu_body.0.call.35.108:instruction")
#loc549 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":297:16)
#loc550 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":297:43)
#loc551 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":298:38)
#loc552 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":299:40)
#loc553 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":300:43)
#loc554 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":300:88)
#loc555 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":300:120)
#loc556 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":300:116)
#loc557 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":301:43)
#loc558 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":302:21)
#loc559 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":303:21)
#loc560 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model/model.py":304:9)
#loc561 = loc(callsite(#loc14 at #loc))
#loc562 = loc(callsite(#loc at #loc))
#loc563 = loc(callsite(#loc15 at #loc))
#loc564 = loc(callsite(#loc16 at #loc))
#loc565 = loc(callsite(#loc17 at #loc))
#loc566 = loc(callsite(#loc18 at #loc))
#loc567 = loc(callsite(#loc19 at #loc))
#loc568 = loc(callsite(#loc20 at #loc))
#loc569 = loc(callsite(#loc21 at #loc))
#loc570 = loc(callsite(#loc24 at #loc))
#loc571 = loc(callsite(#loc25 at #loc))
#loc572 = loc(callsite(#loc26 at #loc))
#loc573 = loc(callsite(#loc27 at #loc))
#loc574 = loc(callsite(#loc28 at #loc))
#loc575 = loc(callsite(#loc37 at #loc))
#loc576 = loc(callsite(#loc38 at #loc))
#loc577 = loc(callsite(#loc39 at #loc))
#loc578 = loc(callsite(#loc40 at #loc))
#loc579 = loc(callsite(#loc41 at #loc))
#loc580 = loc(callsite(#loc42 at #loc))
#loc581 = loc(callsite(#loc43 at #loc))
#loc582 = loc(callsite(#loc44 at #loc))
#loc583 = loc(callsite(#loc45 at #loc))
#loc584 = loc(callsite(#loc46 at #loc))
#loc585 = loc(callsite(#loc47 at #loc))
#loc586 = loc(callsite(#loc48 at #loc))
#loc587 = loc(callsite(#loc49 at #loc))
#loc588 = loc(callsite(#loc50 at #loc))
#loc589 = loc(callsite(#loc52 at #loc))
#loc590 = loc(callsite(#loc85 at #loc))
#loc591 = loc(callsite(#loc86 at #loc))
#loc592 = loc(callsite(#loc87 at #loc))
#loc593 = loc(callsite(#loc88 at #loc))
#loc594 = loc(callsite(#loc89 at #loc))
#loc595 = loc(callsite(#loc90 at #loc))
#loc596 = loc(callsite(#loc91 at #loc))
#loc597 = loc(callsite(#loc92 at #loc))
#loc598 = loc(callsite(#loc94 at #loc))
#loc599 = loc(callsite(#loc97 at #loc))
#loc600 = loc(callsite(#loc98 at #loc))
#loc601 = loc(callsite(#loc99 at #loc))
#loc602 = loc(callsite(#loc100 at #loc))
#loc603 = loc(callsite(#loc102 at #loc))
#loc604 = loc(callsite(#loc106 at #loc))
#loc605 = loc(callsite(#loc107 at #loc))
#loc606 = loc(callsite(#loc108 at #loc))
#loc607 = loc(callsite(#loc109 at #loc))
#loc608 = loc(callsite(#loc at #loc114))
#loc609 = loc(callsite(#loc115 at #loc114))
#loc610 = loc(callsite(#loc116 at #loc114))
#loc611 = loc(callsite(#loc117 at #loc114))
#loc612 = loc(callsite(#loc118 at #loc114))
#loc613 = loc(callsite(#loc119 at #loc114))
#loc614 = loc(callsite(#loc120 at #loc114))
#loc615 = loc(callsite(#loc121 at #loc114))
#loc616 = loc(callsite(#loc122 at #loc114))
#loc617 = loc(callsite(#loc123 at #loc114))
#loc618 = loc(callsite(#loc162 at #loc163))
#loc619 = loc(callsite(#loc164 at #loc163))
#loc620 = loc(callsite(#loc165 at #loc163))
#loc621 = loc(callsite(#loc at #loc163))
#loc622 = loc(callsite(#loc166 at #loc163))
#loc623 = loc(callsite(#loc167 at #loc163))
#loc624 = loc(callsite(#loc168 at #loc163))
#loc625 = loc(callsite(#loc215 at #loc))
#loc626 = loc(callsite(#loc216 at #loc))
#loc627 = loc(callsite(#loc217 at #loc))
#loc628 = loc(callsite(#loc218 at #loc))
#loc629 = loc(callsite(#loc219 at #loc))
#loc630 = loc(callsite(#loc220 at #loc))
#loc631 = loc(callsite(#loc221 at #loc))
#loc632 = loc(callsite(#loc222 at #loc))
#loc633 = loc(callsite(#loc223 at #loc))
#loc634 = loc(callsite(#loc224 at #loc))
#loc635 = loc(callsite(#loc225 at #loc))
#loc636 = loc(callsite(#loc226 at #loc))
#loc637 = loc(callsite(#loc227 at #loc))
#loc638 = loc(callsite(#loc228 at #loc))
#loc639 = loc(callsite(#loc229 at #loc))
#loc640 = loc(callsite(#loc232 at #loc))
#loc641 = loc(callsite(#loc233 at #loc))
#loc642 = loc(callsite(#loc234 at #loc))
#loc643 = loc(callsite(#loc235 at #loc))
#loc644 = loc(callsite(#loc236 at #loc))
#loc645 = loc(callsite(#loc237 at #loc))
#loc646 = loc(callsite(#loc238 at #loc))
#loc647 = loc(callsite(#loc239 at #loc))
#loc648 = loc(callsite(#loc240 at #loc))
#loc649 = loc(callsite(#loc241 at #loc))
#loc650 = loc(callsite(#loc242 at #loc))
#loc651 = loc(callsite(#loc243 at #loc))
#loc652 = loc(callsite(#loc244 at #loc))
#loc653 = loc(callsite(#loc251 at #loc))
#loc654 = loc(callsite(#loc252 at #loc))
#loc655 = loc(callsite(#loc253 at #loc))
#loc656 = loc(callsite(#loc254 at #loc))
#loc657 = loc(callsite(#loc258 at #loc))
#loc658 = loc(callsite(#loc260 at #loc))
#loc659 = loc(callsite(#loc261 at #loc))
#loc660 = loc(callsite(#loc262 at #loc))
#loc661 = loc(callsite(#loc263 at #loc))
#loc662 = loc(callsite(#loc264 at #loc))
#loc663 = loc(callsite(#loc265 at #loc))
#loc664 = loc(callsite(#loc268 at #loc))
#loc665 = loc(callsite(#loc269 at #loc))
#loc666 = loc(callsite(#loc270 at #loc))
#loc667 = loc(callsite(#loc271 at #loc))
#loc668 = loc(callsite(#loc272 at #loc))
#loc669 = loc(callsite(#loc273 at #loc))
#loc670 = loc(callsite(#loc274 at #loc))
#loc671 = loc(callsite(#loc275 at #loc))
#loc672 = loc(callsite(#loc276 at #loc))
#loc673 = loc(callsite(#loc277 at #loc))
#loc674 = loc(callsite(#loc278 at #loc))
#loc675 = loc(callsite(#loc279 at #loc))
#loc676 = loc(callsite(#loc280 at #loc))
#loc677 = loc(callsite(#loc281 at #loc))
#loc678 = loc(callsite(#loc283 at #loc))
#loc679 = loc(callsite(#loc296 at #loc))
#loc680 = loc(callsite(#loc297 at #loc))
#loc681 = loc(callsite(#loc298 at #loc))
#loc682 = loc(callsite(#loc299 at #loc))
#loc683 = loc(callsite(#loc300 at #loc))
#loc684 = loc(callsite(#loc301 at #loc))
#loc685 = loc(callsite(#loc302 at #loc))
#loc686 = loc(callsite(#loc308 at #loc))
#loc687 = loc(callsite(#loc309 at #loc))
#loc688 = loc(callsite(#loc310 at #loc))
#loc689 = loc(callsite(#loc311 at #loc))
#loc690 = loc(callsite(#loc312 at #loc))
#loc691 = loc(callsite(#loc313 at #loc))
#loc692 = loc(callsite(#loc314 at #loc))
#loc693 = loc(callsite(#loc315 at #loc))
#loc694 = loc(callsite(#loc316 at #loc))
#loc695 = loc(callsite(#loc317 at #loc))
#loc696 = loc(callsite(#loc318 at #loc))
#loc697 = loc(callsite(#loc319 at #loc))
#loc698 = loc(callsite(#loc320 at #loc))
#loc699 = loc(callsite(#loc321 at #loc))
#loc700 = loc(callsite(#loc322 at #loc))
#loc701 = loc(callsite(#loc323 at #loc))
#loc702 = loc(callsite(#loc324 at #loc))
#loc703 = loc(callsite(#loc325 at #loc))
#loc704 = loc(callsite(#loc326 at #loc))
#loc705 = loc(callsite(#loc327 at #loc))
#loc706 = loc(callsite(#loc328 at #loc))
#loc707 = loc(callsite(#loc329 at #loc))
#loc708 = loc(callsite(#loc330 at #loc))
#loc709 = loc(callsite(#loc331 at #loc))
#loc710 = loc(callsite(#loc332 at #loc))
#loc711 = loc(callsite(#loc333 at #loc))
#loc712 = loc(callsite(#loc335 at #loc))
#loc713 = loc(callsite(#loc346 at #loc))
#loc714 = loc(callsite(#loc347 at #loc))
#loc715 = loc(callsite(#loc348 at #loc))
#loc716 = loc(callsite(#loc349 at #loc))
#loc717 = loc(callsite(#loc350 at #loc))
#loc718 = loc(callsite(#loc351 at #loc))
#loc719 = loc(callsite(#loc353 at #loc))
#loc720 = loc(callsite(#loc354 at #loc))
#loc721 = loc(callsite(#loc355 at #loc))
#loc722 = loc(callsite(#loc356 at #loc))
#loc723 = loc(callsite(#loc357 at #loc))
#loc724 = loc(callsite(#loc358 at #loc))
#loc725 = loc(callsite(#loc359 at #loc))
#loc726 = loc(callsite(#loc371 at #loc372))
#loc727 = loc(callsite(#loc373 at #loc372))
#loc728 = loc(callsite(#loc374 at #loc372))
#loc729 = loc(callsite(#loc375 at #loc372))
#loc730 = loc(callsite(#loc376 at #loc372))
#loc731 = loc(callsite(#loc377 at #loc372))
#loc732 = loc(callsite(#loc378 at #loc372))
#loc733 = loc(callsite(#loc379 at #loc372))
#loc734 = loc(callsite(#loc380 at #loc372))
#loc735 = loc(callsite(#loc at #loc372))
#loc736 = loc(callsite(#loc381 at #loc372))
#loc737 = loc(callsite(#loc382 at #loc372))
#loc738 = loc(callsite(#loc383 at #loc372))
#loc739 = loc(callsite(#loc384 at #loc372))
#loc740 = loc(callsite(#loc385 at #loc372))
#loc741 = loc(callsite(#loc386 at #loc372))
#loc742 = loc(callsite(#loc387 at #loc372))
#loc743 = loc(callsite(#loc388 at #loc372))
#loc744 = loc(callsite(#loc389 at #loc372))
#loc745 = loc(callsite(#loc390 at #loc372))
#loc746 = loc(callsite(#loc391 at #loc372))
#loc747 = loc(callsite(#loc392 at #loc372))
#loc748 = loc(callsite(#loc393 at #loc372))
#loc749 = loc(callsite(#loc394 at #loc372))
#loc750 = loc(callsite(#loc395 at #loc372))
#loc751 = loc(callsite(#loc396 at #loc372))
#loc752 = loc(callsite(#loc397 at #loc372))
#loc753 = loc(callsite(#loc398 at #loc372))
#loc754 = loc(callsite(#loc399 at #loc372))
#loc755 = loc(callsite(#loc400 at #loc372))
#loc756 = loc(callsite(#loc401 at #loc372))
#loc757 = loc(callsite(#loc402 at #loc372))
#loc758 = loc(callsite(#loc403 at #loc372))
#loc759 = loc(callsite(#loc404 at #loc372))
#loc760 = loc(callsite(#loc406 at #loc372))
#loc761 = loc(callsite(#loc407 at #loc372))
#loc762 = loc(callsite(#loc408 at #loc372))
#loc763 = loc(callsite(#loc409 at #loc372))
#loc764 = loc(callsite(#loc410 at #loc372))
#loc765 = loc(callsite(#loc411 at #loc372))
#loc766 = loc(callsite(#loc412 at #loc372))
#loc767 = loc(callsite(#loc413 at #loc372))
#loc768 = loc(callsite(#loc414 at #loc372))
#loc769 = loc(callsite(#loc415 at #loc372))
#loc770 = loc(callsite(#loc416 at #loc372))
#loc771 = loc(callsite(#loc417 at #loc372))
#loc772 = loc(callsite(#loc418 at #loc372))
#loc773 = loc(callsite(#loc419 at #loc372))
#loc774 = loc(callsite(#loc420 at #loc372))
#loc775 = loc(callsite(#loc421 at #loc372))
#loc776 = loc(callsite(#loc422 at #loc372))
#loc777 = loc(callsite(#loc423 at #loc372))
#loc778 = loc(callsite(#loc424 at #loc372))
#loc779 = loc(callsite(#loc425 at #loc372))
#loc780 = loc(callsite(#loc426 at #loc372))
#loc781 = loc(callsite(#loc427 at #loc372))
#loc782 = loc(callsite(#loc428 at #loc372))
#loc783 = loc(callsite(#loc429 at #loc372))
#loc784 = loc(callsite(#loc430 at #loc372))
#loc785 = loc(callsite(#loc431 at #loc372))
#loc786 = loc(callsite(#loc491 at #loc))
#loc787 = loc(callsite(#loc492 at #loc))
#loc788 = loc(callsite(#loc493 at #loc))
#loc789 = loc(callsite(#loc231 at #loc))
#loc790 = loc(fused[#loc534, #loc533])
#loc791 = loc(callsite(#loc164 at #loc555))
#loc792 = loc(callsite(#loc165 at #loc555))
#loc793 = loc(callsite(#loc at #loc555))
#loc794 = loc(callsite(#loc166 at #loc555))
#loc795 = loc(callsite(#loc167 at #loc555))
#loc796 = loc(callsite(#loc168 at #loc555))
#loc797 = loc(fused[#loc562, #loc563])
#loc798 = loc(fused[#loc562, #loc582])
#loc799 = loc(callsite(#loc51 at #loc589))
#loc800 = loc(callsite(#loc53 at #loc589))
#loc801 = loc(callsite(#loc54 at #loc589))
#loc802 = loc(callsite(#loc55 at #loc589))
#loc803 = loc(callsite(#loc56 at #loc589))
#loc804 = loc(callsite(#loc57 at #loc589))
#loc805 = loc(callsite(#loc58 at #loc589))
#loc806 = loc(callsite(#loc59 at #loc589))
#loc807 = loc(callsite(#loc60 at #loc589))
#loc808 = loc(callsite(#loc at #loc589))
#loc809 = loc(callsite(#loc61 at #loc589))
#loc810 = loc(callsite(#loc62 at #loc589))
#loc811 = loc(callsite(#loc63 at #loc589))
#loc812 = loc(callsite(#loc64 at #loc589))
#loc813 = loc(callsite(#loc65 at #loc589))
#loc814 = loc(callsite(#loc66 at #loc589))
#loc815 = loc(callsite(#loc67 at #loc589))
#loc816 = loc(callsite(#loc68 at #loc589))
#loc817 = loc(callsite(#loc69 at #loc589))
#loc818 = loc(callsite(#loc70 at #loc589))
#loc819 = loc(callsite(#loc71 at #loc589))
#loc820 = loc(callsite(#loc72 at #loc589))
#loc821 = loc(callsite(#loc73 at #loc589))
#loc822 = loc(callsite(#loc74 at #loc589))
#loc823 = loc(callsite(#loc75 at #loc589))
#loc824 = loc(callsite(#loc76 at #loc589))
#loc825 = loc(callsite(#loc77 at #loc589))
#loc826 = loc(callsite(#loc78 at #loc589))
#loc827 = loc(callsite(#loc79 at #loc589))
#loc828 = loc(callsite(#loc80 at #loc589))
#loc829 = loc(callsite(#loc81 at #loc589))
#loc830 = loc(callsite(#loc82 at #loc589))
#loc831 = loc(callsite(#loc83 at #loc589))
#loc832 = loc(callsite(#loc84 at #loc589))
#loc833 = loc(callsite(#loc93 at #loc598))
#loc834 = loc(callsite(#loc95 at #loc598))
#loc835 = loc(callsite(#loc96 at #loc598))
#loc836 = loc(callsite(#loc101 at #loc603))
#loc837 = loc(callsite(#loc at #loc603))
#loc838 = loc(callsite(#loc103 at #loc603))
#loc839 = loc(callsite(#loc104 at #loc603))
#loc840 = loc(callsite(#loc105 at #loc603))
#loc841 = loc(callsite(#loc95 at #loc612))
#loc842 = loc(callsite(#loc96 at #loc612))
#loc843 = loc(callsite(#loc96 at #loc614))
#loc844 = loc(fused[#loc562, #loc626])
#loc845 = loc(callsite(#loc230 at #loc562))
#loc846 = loc(callsite(#loc231 at #loc562))
#loc847 = loc(fused[#loc562, #loc641])
#loc848 = loc(fused[#loc562, #loc642])
#loc849 = loc(fused[#loc562, #loc644])
#loc850 = loc(fused[#loc562, #loc645])
#loc851 = loc(fused[#loc562, #loc646])
#loc852 = loc(callsite(#loc245 at #loc562))
#loc853 = loc(callsite(#loc246 at #loc562))
#loc854 = loc(callsite(#loc247 at #loc562))
#loc855 = loc(callsite(#loc248 at #loc562))
#loc856 = loc(callsite(#loc249 at #loc562))
#loc857 = loc(callsite(#loc250 at #loc562))
#loc858 = loc(fused[#loc562, #loc647])
#loc859 = loc(fused[#loc562, #loc655])
#loc860 = loc(callsite(#loc255 at #loc562))
#loc861 = loc(callsite(#loc256 at #loc562))
#loc862 = loc(callsite(#loc257 at #loc562))
#loc863 = loc(callsite(#loc259 at #loc658))
#loc864 = loc(fused[#loc562, #loc657])
#loc865 = loc(fused[#loc562, #loc656])
#loc866 = loc(callsite(#loc266 at #loc562))
#loc867 = loc(callsite(#loc267 at #loc562))
#loc868 = loc(callsite(#loc259 at #loc562))
#loc869 = loc(fused[#loc562, #loc643])
#loc870 = loc(fused[#loc562, #loc666])
#loc871 = loc(callsite(#loc282 at #loc562))
#loc872 = loc(callsite(#loc231 at #loc678))
#loc873 = loc(callsite(#loc284 at #loc562))
#loc874 = loc(callsite(#loc285 at #loc562))
#loc875 = loc(callsite(#loc286 at #loc562))
#loc876 = loc(callsite(#loc287 at #loc562))
#loc877 = loc(callsite(#loc288 at #loc562))
#loc878 = loc(callsite(#loc289 at #loc562))
#loc879 = loc(callsite(#loc290 at #loc562))
#loc880 = loc(callsite(#loc291 at #loc562))
#loc881 = loc(callsite(#loc292 at #loc562))
#loc882 = loc(callsite(#loc293 at #loc562))
#loc883 = loc(callsite(#loc294 at #loc562))
#loc884 = loc(callsite(#loc295 at #loc562))
#loc885 = loc(fused[#loc562, #loc681])
#loc886 = loc(fused[#loc562, #loc682])
#loc887 = loc(callsite(#loc303 at #loc562))
#loc888 = loc(callsite(#loc304 at #loc562))
#loc889 = loc(callsite(#loc305 at #loc562))
#loc890 = loc(callsite(#loc306 at #loc562))
#loc891 = loc(callsite(#loc307 at #loc562))
#loc892 = loc(fused[#loc562, #loc686])
#loc893 = loc(fused[#loc562, #loc687])
#loc894 = loc(callsite(#loc231 at #loc698))
#loc895 = loc(fused[#loc562, #loc697])
#loc896 = loc(callsite(#loc334 at #loc562))
#loc897 = loc(callsite(#loc231 at #loc712))
#loc898 = loc(callsite(#loc336 at #loc562))
#loc899 = loc(callsite(#loc337 at #loc562))
#loc900 = loc(callsite(#loc338 at #loc562))
#loc901 = loc(callsite(#loc339 at #loc562))
#loc902 = loc(callsite(#loc340 at #loc562))
#loc903 = loc(callsite(#loc341 at #loc562))
#loc904 = loc(callsite(#loc342 at #loc562))
#loc905 = loc(callsite(#loc343 at #loc562))
#loc906 = loc(callsite(#loc344 at #loc562))
#loc907 = loc(callsite(#loc345 at #loc562))
#loc908 = loc(callsite(#loc245 at #loc717))
#loc909 = loc(callsite(#loc246 at #loc717))
#loc910 = loc(callsite(#loc248 at #loc717))
#loc911 = loc(callsite(#loc249 at #loc717))
#loc912 = loc(callsite(#loc250 at #loc717))
#loc913 = loc(callsite(#loc266 at #loc718))
#loc914 = loc(callsite(#loc267 at #loc718))
#loc915 = loc(callsite(#loc259 at #loc718))
#loc916 = loc(callsite(#loc352 at #loc718))
#loc917 = loc(fused[#loc562, #loc725])
#loc918 = loc(callsite(#loc231 at #loc759))
#loc919 = loc(callsite(#loc405 at #loc759))
#loc920 = loc(callsite(#loc231 at #loc771))
#loc921 = loc(callsite(#loc405 at #loc771))
#loc922 = loc(callsite(#loc230 at #loc773))
#loc923 = loc(callsite(#loc231 at #loc773))
#loc924 = loc(callsite(#loc405 at #loc773))
#loc925 = loc(callsite(#loc231 at #loc777))
#loc926 = loc(callsite(#loc405 at #loc777))
#loc927 = loc(fused[#loc808, #loc809])
#loc928 = loc(fused[#loc808, #loc828])
#loc929 = loc(fused[#loc808, #loc832])
#loc930 = loc(fused[#loc837, #loc836])
#loc931 = loc(fused[#loc613, #loc843])
