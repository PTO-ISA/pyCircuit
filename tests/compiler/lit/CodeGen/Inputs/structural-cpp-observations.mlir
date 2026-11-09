#w1 = #ac.static_expr<{kind = "literal", location = {path = "observations.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "observations.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module attributes {ac.source_units = [{package = "", path = "observations.py"}]} {
  "ac.module"() ({
  ^bb0(%path: !b1, %left: !b8, %right: !b8):
    "ac.observe"(%path, %left, %right) {kind = "log", spec = {event = "pair", level = "info", items = [{kind = "value", ordinal = 0 : i32}, {kind = "value", ordinal = 1 : i32}]}, ac.location = {path = "observations.py", line = 4 : i64, column = 3 : i64, end_line = 4 : i64, end_column = 20 : i64}, ac.observation_id = {registration = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, site = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, ordinal = 0 : i64, invocation = [], owner = {instance_path = []}}} : (!b1, !b8, !b8) -> ()
    "ac.observe"(%path, %left) {kind = "report", spec = {name = "progress"}, ac.location = {path = "observations.py", line = 5 : i64, column = 3 : i64, end_line = 5 : i64, end_column = 20 : i64}, ac.observation_id = {registration = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, site = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, ordinal = 1 : i64, invocation = [], owner = {instance_path = []}}} : (!b1, !b8) -> ()
    "ac.yield"(%left) : (!b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "observations.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b8, !b8) -> !b8, input_names = ["path", "left", "right"], output_names = ["out"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
