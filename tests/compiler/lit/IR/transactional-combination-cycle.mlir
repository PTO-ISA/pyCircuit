// RUN: not %pycircuit_opt %s --ac-route-transactions='top=Cycle' -o /dev/null 2>&1 | %FileCheck %s
// CHECK: unresolved or cyclic dependencies
#w8 = #ac.static_expr<{kind = "literal", location = {path = "cycle.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Cycle, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
    %left = "ac.bits.binary"(%right, %one) {opcode = "add"} : (!b8, !b8) -> !b8
    %right = "ac.bits.binary"(%left, %one) {opcode = "add"} : (!b8, !b8) -> !b8
    %state = ac.resource.alloc "state" initial(%one : !b8) : !ac.state<!b8>
    "ac.yield"(%state) : (!ac.state<!b8>) -> ()
  }) {sym_name = "Cycle", source_owner = {package = "", path = "cycle.py"}, parameters = [], type_parameters = [], function_type = () -> !ac.state<!b8>, input_names = [], output_names = ["value"], output_kinds = ["export"]} : () -> ()
}
