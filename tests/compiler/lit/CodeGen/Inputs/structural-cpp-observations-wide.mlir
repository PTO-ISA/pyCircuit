#w1 = #ac.static_expr<{kind = "literal", location = {path = "observations_wide.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w130 = #ac.static_expr<{kind = "literal", location = {path = "observations_wide.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#w1>
!b130 = !ac.bits<#w130>
module attributes {ac.source_units = [{package = "", path = "observations_wide.py"}]} {
  "ac.module"() ({
  ^bb0(%path: !b1, %value: !b130):
    "ac.observe"(%path, %value) {kind = "log", spec = {event = "wide", level = "info", items = [{kind = "value", ordinal = 0 : i32}]}, ac.location = {path = "observations_wide.py", line = 4 : i64, column = 3 : i64, end_line = 4 : i64, end_column = 20 : i64}, ac.observation_id = {registration = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, site = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, ordinal = 0 : i64, invocation = [], owner = {instance_path = []}}} : (!b1, !b130) -> ()
    "ac.yield"(%value) : (!b130) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "observations_wide.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b130) -> !b130, input_names = ["path", "value"], output_names = ["out"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
