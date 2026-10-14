#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @Owner, ast_path = []}, expansion = []}, location = {path = "sampled_admission.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Owner, ast_path = []}, expansion = []}, location = {path = "sampled_admission.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#span0 = {path = "sampled_admission.py", line = 10 : i64, column = 5 : i64, end_line = 10 : i64, end_column = 24 : i64}
#span1 = {path = "sampled_admission.py", line = 11 : i64, column = 5 : i64, end_line = 11 : i64, end_column = 24 : i64}
#state = {site = {definition = @Owner, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#registration = {site = {definition = @Owner, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}
#guard0 = {site = {definition = @Owner, ast_path = [{kind = "index", value = 8 : i64}]}, expansion = []}
#guard1 = {site = {definition = @Owner, ast_path = [{kind = "index", value = 9 : i64}]}, expansion = []}
#guard_call = {site = {definition = @Owner, ast_path = [{kind = "index", value = 10 : i64}]}, expansion = []}
#check0 = {registration = #registration, check = #guard0, obligation = 0 : i64}
#check1 = {registration = #registration, check = #guard1, obligation = 1 : i64}
#top_registration = {site = {definition = @Pure, ast_path = [{kind = "index", value = 17 : i64}]}, expansion = []}
#top_guard = {site = {definition = @Pure, ast_path = [{kind = "index", value = 18 : i64}]}, expansion = []}
#top_check = {registration = #top_registration, check = #top_guard, obligation = 0 : i64}
#top_span = {path = "sampled_admission.py", line = 30 : i64, column = 5 : i64, end_line = 30 : i64, end_column = 24 : i64}
#left = {site = {definition = @Top, ast_path = [{kind = "index", value = 20 : i64}]}, expansion = []}
#right = {site = {definition = @Top, ast_path = [{kind = "index", value = 21 : i64}]}, expansion = []}
#pure_instance = {site = {definition = @Top, ast_path = [{kind = "index", value = 22 : i64}]}, expansion = []}

module attributes {ac.stage = "final", ac.source_units = [{package = "", path = "sampled_admission.py"}]} {
  "ac.module.import"() {semantic_kind = "hardware", sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()

  "ac.module"() ({
  ^bb0(%guard_first: !b1, %guard_second: !b1, %clk: !b1, %rst: !b1):
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b1
    %q = "ac.instance"(%clk, %rst, %transaction#1, %transaction#0, %zero) {instance_name = "state", callee = @storage, parameters = [], type_arguments = [!b1], occurrence = #state} : (!b1, !b1, !b1, !b1, !b1) -> !b1
    %transaction:6 = "ac.rule"(%clk, %rst, %guard_first, %guard_second, %q) ({
    ^bb0(%rule_clk: !b1, %rule_rst: !b1, %first: !b1, %second: !b1, %old_q: !b1):
      %rule_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %candidate = "ac.bits.unary"(%old_q) {opcode = "not"} : (!b1) -> !b1
      %known_first = "ac.bits.compare"(%first, %rule_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %known_second = "ac.bits.compare"(%second, %rule_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %fire = "ac.bits.binary"(%known_first, %known_second) {opcode = "and"} : (!b1, !b1) -> !b1
      "ac.yield"(%candidate, %fire, %first, %rule_one, %second, %known_first) : (!b1, !b1, !b1, !b1, !b1, !b1) -> ()
    }) {name = "toggle", occurrence = #registration, ac.required_checks = [{id = #check0, kind = "guard_known", location = #span0}, {id = #check1, kind = "guard_known", location = #span1}], ac.sampled_admission = {clock_capture = 0 : i64, reset_capture = 1 : i64, fire_result = 1 : i64, body_checks = [], observations = [], guards = [{id = #check0, invocation = [], predecessor = -1 : i64}, {id = #check1, invocation = [{call = #guard_call, receiver = {instance_path = []}}], predecessor = 0 : i64}]}} : (!b1, !b1, !b1, !b1, !b1) -> (!b1, !b1, !b1, !b1, !b1, !b1)
    "ac.expect"(%transaction#2, %transaction#3, %clk, %rst) {kind = "guard_known", location = #span0, ac.check_id = #check0} : (!b1, !b1, !b1, !b1) -> ()
    "ac.expect"(%transaction#4, %transaction#5, %clk, %rst) {kind = "guard_known", location = #span1, ac.check_id = #check1} : (!b1, !b1, !b1, !b1) -> ()
    "ac.yield"(%q) : (!b1) -> ()
  }) {sym_name = "Owner", source_owner = {package = "", path = "sampled_admission.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1) -> !b1, input_names = ["guard_first", "guard_second", "pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 2 : i64, reset = 3 : i64}, ac.transaction_wiring = {states = [#state], incoming = [], writers = [{registration = #registration, ordinary_results = 0 : i64, candidates = [{state = {owner = {instance_path = []}, declaration = #state, element = []}, result = 0 : i64}]}]}} : () -> ()

  "ac.module"() ({
  ^bb0(%guard: !b1, %clk: !b1, %rst: !b1):
    %top_transaction:3 = "ac.rule"(%clk, %rst, %guard) ({
    ^bb0(%rule_clk: !b1, %rule_rst: !b1, %rule_guard: !b1):
      %rule_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %fire = "ac.bits.compare"(%rule_guard, %rule_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      "ac.yield"(%fire, %rule_guard, %rule_one) : (!b1, !b1, !b1) -> ()
    }) {name = "statefree", occurrence = #top_registration, ac.required_checks = [{id = #top_check, kind = "guard_known", location = #top_span}], ac.sampled_admission = {clock_capture = 0 : i64, reset_capture = 1 : i64, fire_result = 0 : i64, body_checks = [], observations = [], guards = [{id = #top_check, invocation = [], predecessor = -1 : i64}]}} : (!b1, !b1, !b1) -> (!b1, !b1, !b1)
    "ac.expect"(%top_transaction#1, %top_transaction#2, %clk, %rst) {kind = "guard_known", location = #top_span, ac.check_id = #top_check} : (!b1, !b1, !b1, !b1) -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Pure", source_owner = {package = "", path = "sampled_admission.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> (), input_names = ["guard", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.transaction_wiring = {states = [], incoming = [], writers = [{registration = #top_registration, ordinary_results = 0 : i64, candidates = []}]}} : () -> ()

  "ac.module"() ({
  ^bb0(%left_first: !b1, %left_second: !b1, %right_first: !b1, %right_second: !b1, %left_clk: !b1, %left_rst: !b1, %right_clk: !b1, %right_rst: !b1, %sample_clk: !b1, %sample_rst: !b1, %root_clk: !b1, %root_rst: !b1):
    %left_q = "ac.instance"(%left_first, %left_second, %left_clk, %left_rst) {instance_name = "left", callee = @Owner, parameters = [], type_arguments = [], occurrence = #left} : (!b1, !b1, !b1, !b1) -> !b1
    %right_q = "ac.instance"(%right_first, %right_second, %right_clk, %right_rst) {instance_name = "right", callee = @Owner, parameters = [], type_arguments = [], occurrence = #right} : (!b1, !b1, !b1, !b1) -> !b1
    "ac.instance"(%left_first, %sample_clk, %sample_rst) {instance_name = "pure", callee = @Pure, parameters = [], type_arguments = [], occurrence = #pure_instance} : (!b1, !b1, !b1) -> ()
    "ac.yield"(%left_q, %right_q) : (!b1, !b1) -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "sampled_admission.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1) -> (!b1, !b1), input_names = ["left_first", "left_second", "right_first", "right_second", "left_clk", "left_rst", "right_clk", "right_rst", "sample_clk", "sample_rst", "pyc_clk", "pyc_rst"], output_names = ["left_q", "right_q"], ac.domain_inputs = {clock = 10 : i64, reset = 11 : i64}} : () -> ()

  "ac.system"() {entry = {callee = @Top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
