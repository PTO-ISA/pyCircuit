// Common-IR hierarchy with real storage and independently driven domains.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @execution.domains.Top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @execution.domains.Top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!result = !ac.struct<"execution.domains.Flag">
module {
  ac.struct "execution.domains.Flag" fields [{name = "value", type = !b1}] {ac.source_owner = {package = "execution", path = "domains.py"}}
  "ac.module.import"() {sym_name = "native.dffe", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}], function_type = (!b1, !b1, !b1, !ac.type_param<@native.dffe, "T">, !ac.type_param<@native.dffe, "T">) -> !ac.type_param<@native.dffe, "T">} : () -> ()
  "ac.module"() ({
  ^bb0(%allow: !b1):
    %one = "ac.bits.constant"() {value = #w1} : () -> !b1
    %output = ac.struct.create(%allow) : (!b1) -> !result
    %check:2 = "ac.rule"(%allow, %one) ({
    ^bb0(%c: !b1, %p: !b1):
      "ac.yield"(%c, %p) : (!b1, !b1) -> ()
    }) {name = "inspect", occurrence = {site = {definition = @execution.domains.Pure, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}, ac.required_checks = [{id = {registration = {site = {definition = @execution.domains.Pure, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}, check = {site = {definition = @execution.domains.Pure, ast_path = [{kind = "index", value = 13 : i64}]}, expansion = []}, obligation = 0 : i64}, kind = "assert", location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}]} : (!b1, !b1) -> (!b1, !b1)
    "ac.expect"(%check#0, %check#1) {kind = "assert", location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, ac.check_id = {registration = {site = {definition = @execution.domains.Pure, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}, check = {site = {definition = @execution.domains.Pure, ast_path = [{kind = "index", value = 13 : i64}]}, expansion = []}, obligation = 0 : i64}, ac.message = "grandchild"} : (!b1, !b1) -> ()
    "ac.yield"(%output) : (!result) -> ()
  }) {sym_name = "execution.domains.Pure", source_owner = {package = "execution", path = "domains.py"}, parameters = [], type_parameters = [], input_names = ["allow"], output_names = ["result"], function_type = (!b1) -> !result} : () -> ()
  "ac.module"() ({
  ^bb0(%data: !b1, %allow: !b1, %clk: !b1, %rst: !b1):
    %one = "ac.bits.constant"() {value = #w1} : () -> !b1
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b1
    %q = "ac.instance"(%clk, %rst, %one, %data, %zero) {callee = @native.dffe, instance_name = "state", parameters = [], type_arguments = [!b1], occurrence = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b1, !b1) -> !b1
    %child = "ac.instance"(%allow) {callee = @execution.domains.Pure, instance_name = "pure", parameters = [], type_arguments = [], occurrence = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}} : (!b1) -> !result
    %output = ac.struct.create(%q) : (!b1) -> !result
    %check:2 = "ac.rule"(%allow, %one) ({
    ^bb0(%c: !b1, %p: !b1):
      "ac.yield"(%c, %p) : (!b1, !b1) -> ()
    }) {name = "inspect", occurrence = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}, ac.required_checks = [{id = {registration = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}, check = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 13 : i64}]}, expansion = []}, obligation = 0 : i64}, kind = "assert", location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}]} : (!b1, !b1) -> (!b1, !b1)
    "ac.expect"(%check#0, %check#1) {kind = "assert", location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, ac.check_id = {registration = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}, check = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 13 : i64}]}, expansion = []}, obligation = 0 : i64}, ac.message = "cell"} : (!b1, !b1) -> ()
    "ac.yield"(%output) : (!result) -> ()
  }) {sym_name = "execution.domains.Cell", source_owner = {package = "execution", path = "domains.py"}, parameters = [], type_parameters = [], input_names = ["data", "allow", "pyc_clk", "pyc_rst"], output_names = ["result"], function_type = (!b1, !b1, !b1, !b1) -> !result, ac.return_form = "single", ac.parameters = [{name = "data", binding = "positional_or_keyword", default = {present = false}, constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}, origin = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}, {name = "allow", binding = "positional_or_keyword", default = {present = false}, constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}, origin = {site = {definition = @execution.domains.Cell, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}], ac.result_constraints = [{kind = "hardware", type = !result, source_kind = "nominal"}], ac.domain_inputs = {clock = 2 : i64, reset = 3 : i64}} : () -> ()
  "ac.module"() ({
  ^bb0(%clk_a: !b1, %rst_a: !b1, %clk_b: !b1, %rst_b: !b1, %data: !b1, %allow_a: !b1, %allow_b: !b1):
    %a = "ac.instance"(%data, %allow_a, %clk_a, %rst_a) {callee = @execution.domains.Cell, instance_name = "zzz", parameters = [], type_arguments = [], occurrence = {site = {definition = @execution.domains.Top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b1) -> !result
    %b = "ac.instance"(%data, %allow_b, %clk_b, %rst_b) {callee = @execution.domains.Cell, instance_name = "aaa", parameters = [], type_arguments = [], occurrence = {site = {definition = @execution.domains.Top, ast_path = [{kind = "index", value = 19 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b1) -> !result
    %left = ac.struct.get %a["value"] : (!result) -> !b1
    %right = ac.struct.get %b["value"] : (!result) -> !b1
    "ac.yield"(%left, %right) : (!b1, !b1) -> ()
  }) {sym_name = "execution.domains.Top", source_owner = {package = "execution", path = "domains.py"}, parameters = [], type_parameters = [], input_names = ["clk_a", "rst_a", "clk_b", "rst_b", "data", "allow_a", "allow_b"], output_names = ["left", "right"], function_type = (!b1, !b1, !b1, !b1, !b1, !b1, !b1) -> (!b1, !b1)} : () -> ()
  "ac.system"() {entry = {callee = @execution.domains.Top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
