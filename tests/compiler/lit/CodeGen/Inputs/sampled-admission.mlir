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

#conditional_state = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#conditional_registration = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}
#conditional_instance = {site = {definition = @Top, ast_path = [{kind = "index", value = 23 : i64}]}, expansion = []}
#then_call = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 30 : i64}]}, expansion = []}
#else_call = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 31 : i64}]}, expansion = []}
#conditional_site0 = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 10 : i64}]}, expansion = []}
#conditional_check0 = {registration = #conditional_registration, check = #conditional_site0, obligation = 0 : i64}
#conditional_site1 = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 11 : i64}]}, expansion = []}
#conditional_check1 = {registration = #conditional_registration, check = #conditional_site1, obligation = 1 : i64}
#conditional_site2 = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 12 : i64}]}, expansion = []}
#conditional_check2 = {registration = #conditional_registration, check = #conditional_site2, obligation = 2 : i64}
#conditional_site3 = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 13 : i64}]}, expansion = []}
#conditional_check3 = {registration = #conditional_registration, check = #conditional_site3, obligation = 3 : i64}
#conditional_site4 = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 14 : i64}]}, expansion = []}
#conditional_check4 = {registration = #conditional_registration, check = #conditional_site4, obligation = 4 : i64}
#conditional_site5 = {site = {definition = @Conditional, ast_path = [{kind = "index", value = 15 : i64}]}, expansion = []}
#conditional_check5 = {registration = #conditional_registration, check = #conditional_site5, obligation = 5 : i64}

module attributes {ac.stage = "final", ac.source_units = [{package = "", path = "sampled_admission.py"}]} {
  "ac.module.import"() {semantic_kind = "hardware", sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()

  "ac.module"() ({
  ^bb0(%guard_first: !b1, %guard_second: !b1, %clk: !b1, %rst: !b1):
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b1
    %q = "ac.instance"(%clk, %rst, %transaction#1, %transaction#0, %zero) {instance_name = "state", callee = @storage, parameters = [], type_arguments = [!b1], occurrence = #state} : (!b1, !b1, !b1, !b1, !b1) -> !b1
    %transaction:7 = "ac.comb"(%clk, %rst, %guard_first, %guard_second, %q) ({
    ^bb0(%rule_clk: !b1, %rule_rst: !b1, %first: !b1, %second: !b1, %old_q: !b1):
      %rule_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %rule_zero = "ac.bits.constant"() {value = #w0} : () -> !b1
      %candidate = "ac.bits.unary"(%old_q) {opcode = "not"} : (!b1) -> !b1
      %known_first = "ac.bits.compare"(%first, %rule_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %known_second = "ac.bits.compare"(%second, %rule_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %fire = "ac.bits.binary"(%known_first, %known_second) {opcode = "and"} : (!b1, !b1) -> !b1
      %intent = "ac.bits.select"(%first, %rule_one, %rule_zero) : (!b1, !b1, !b1) -> !b1
      %effective_enable = "ac.bits.binary"(%fire, %intent) {opcode = "and"} : (!b1, !b1) -> !b1
      "ac.yield"(%candidate, %effective_enable, %fire, %first, %rule_one, %second, %known_first) : (!b1, !b1, !b1, !b1, !b1, !b1, !b1) -> ()
    }) {name = "toggle", occurrence = #registration, ac.required_checks = [{id = #check0, kind = "guard_known", location = #span0}, {id = #check1, kind = "guard_known", location = #span1}], ac.sampled_admission = {clock_capture = 0 : i64, reset_capture = 1 : i64, fire_result = 2 : i64, body_checks = [], observations = [], controls = [{id = #check0, invocation = [], predecessor = -1 : i64, kind = "callable_guard", path = {}}, {id = #check1, invocation = [{call = #guard_call, receiver = {instance_path = []}}], predecessor = 0 : i64, kind = "callable_guard", path = {}}]}} : (!b1, !b1, !b1, !b1, !b1) -> (!b1, !b1, !b1, !b1, !b1, !b1, !b1)
    "ac.expect"(%transaction#3, %transaction#4, %clk, %rst) {kind = "guard_known", location = #span0, ac.check_id = #check0} : (!b1, !b1, !b1, !b1) -> ()
    "ac.expect"(%transaction#5, %transaction#6, %clk, %rst) {kind = "guard_known", location = #span1, ac.check_id = #check1} : (!b1, !b1, !b1, !b1) -> ()
    "ac.yield"(%q) : (!b1) -> ()
  }) {sym_name = "Owner", source_owner = {package = "", path = "sampled_admission.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1) -> !b1, input_names = ["guard_first", "guard_second", "pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 2 : i64, reset = 3 : i64}, ac.transaction_wiring = {states = [#state], incoming = [], writers = [{registration = #registration, ordinary_results = 0 : i64, candidates = [{state = {owner = {instance_path = []}, declaration = #state, element = []}, data = 0 : i64, enable = 1 : i64}]}]}} : () -> ()

  "ac.module"() ({
  ^bb0(%guard: !b1, %clk: !b1, %rst: !b1):
    %top_transaction:3 = "ac.comb"(%clk, %rst, %guard) ({
    ^bb0(%rule_clk: !b1, %rule_rst: !b1, %rule_guard: !b1):
      %rule_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %fire = "ac.bits.compare"(%rule_guard, %rule_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      "ac.yield"(%fire, %rule_guard, %rule_one) : (!b1, !b1, !b1) -> ()
    }) {name = "statefree", occurrence = #top_registration, ac.required_checks = [{id = #top_check, kind = "guard_known", location = #top_span}], ac.sampled_admission = {clock_capture = 0 : i64, reset_capture = 1 : i64, fire_result = 0 : i64, body_checks = [], observations = [], controls = [{id = #top_check, invocation = [], predecessor = -1 : i64, kind = "callable_guard", path = {}}]}} : (!b1, !b1, !b1) -> (!b1, !b1, !b1)
    "ac.expect"(%top_transaction#1, %top_transaction#2, %clk, %rst) {kind = "guard_known", location = #top_span, ac.check_id = #top_check} : (!b1, !b1, !b1, !b1) -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Pure", source_owner = {package = "", path = "sampled_admission.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> (), input_names = ["guard", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.transaction_wiring = {states = [], incoming = [], writers = [{registration = #top_registration, ordinary_results = 0 : i64, candidates = []}]}} : () -> ()

  "ac.module"() ({
  ^bb0(%selector: !b1, %then_guard: !b1, %else_guard: !b1, %asserted: !b1, %clk: !b1, %rst: !b1):
    %initial = "ac.bits.constant"() {value = #w0} : () -> !b1
    %q = "ac.instance"(%clk, %rst, %transaction#1, %transaction#0, %initial) {instance_name = "state", callee = @storage, parameters = [], type_arguments = [!b1], occurrence = #conditional_state} : (!b1, !b1, !b1, !b1, !b1) -> !b1
    %transaction:15 = "ac.comb"(%clk, %rst, %selector, %then_guard, %else_guard, %asserted, %q) ({
    ^bb0(%rule_clk: !b1, %rule_rst: !b1, %raw_selector: !b1, %raw_then: !b1, %raw_else: !b1, %raw_assert: !b1, %old_q: !b1):
      %one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %zero = "ac.bits.constant"() {value = #w0} : () -> !b1
      %candidate = "ac.bits.unary"(%old_q) {opcode = "not"} : (!b1) -> !b1
      %root = "ac.bits.compare"(%one, %one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %k0 = "ac.bits.compare"(%raw_selector, %zero) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %k1 = "ac.bits.compare"(%raw_selector, %one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %known = "ac.bits.binary"(%k0, %k1) {opcode = "or"} : (!b1, !b1) -> !b1
      %selected = "ac.bits.binary"(%root, %known) {opcode = "and"} : (!b1, !b1) -> !b1
      %then_reached = "ac.bits.binary"(%selected, %k1) {opcode = "and"} : (!b1, !b1) -> !b1
      %then_accepted = "ac.bits.compare"(%raw_then, %one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %then_bypass = "ac.bits.select"(%k1, %then_accepted, %one) : (!b1, !b1, !b1) -> !b1
      %after_then = "ac.bits.binary"(%selected, %then_bypass) {opcode = "and"} : (!b1, !b1) -> !b1
      %else_reached = "ac.bits.binary"(%after_then, %k0) {opcode = "and"} : (!b1, !b1) -> !b1
      %else_accepted = "ac.bits.compare"(%raw_else, %one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %else_bypass = "ac.bits.select"(%k0, %else_accepted, %one) : (!b1, !b1, !b1) -> !b1
      %fire = "ac.bits.binary"(%after_then, %else_bypass) {opcode = "and"} : (!b1, !b1) -> !b1
      %enable = "ac.bits.binary"(%fire, %one) {opcode = "and"} : (!b1, !b1) -> !b1
      %then_effect = "ac.bits.binary"(%fire, %k1) {opcode = "and"} : (!b1, !b1) -> !b1
      %else_effect = "ac.bits.binary"(%fire, %k0) {opcode = "and"} : (!b1, !b1) -> !b1
      "ac.yield"(%candidate, %enable, %fire, %one, %one, %raw_selector, %root, %raw_then, %then_reached, %raw_else, %else_reached, %raw_assert, %then_effect, %raw_assert, %else_effect) : (!b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1) -> ()
    }) {name = "conditional", occurrence = #conditional_registration, ac.required_checks = [{id = #conditional_check0, kind = "guard_known", location = #span0}, {id = #conditional_check1, kind = "guard_known", location = #span0}, {id = #conditional_check2, kind = "guard_known", location = #span0}, {id = #conditional_check3, kind = "guard_known", location = #span0}, {id = #conditional_check4, kind = "assert", location = #span0}, {id = #conditional_check5, kind = "assert", location = #span0}], ac.sampled_admission = {clock_capture = 0 : i64, reset_capture = 1 : i64, fire_result = 2 : i64, controls = [{kind = "callable_guard", id = #conditional_check0, invocation = [], predecessor = -1 : i64, path = {}}, {kind = "branch_select", id = #conditional_check1, invocation = [], predecessor = 0 : i64, path = {}}, {kind = "callable_guard", id = #conditional_check2, invocation = [{call = #then_call, receiver = {instance_path = []}}], predecessor = 1 : i64, path = {selection = 1 : i64, when_true = true}}, {kind = "callable_guard", id = #conditional_check3, invocation = [{call = #else_call, receiver = {instance_path = []}}], predecessor = 2 : i64, path = {selection = 1 : i64, when_true = false}}], body_checks = [{id = #conditional_check4, invocation = [], path = {selection = 1 : i64, when_true = true}}, {id = #conditional_check5, invocation = [], path = {selection = 1 : i64, when_true = false}}], observations = []}} : (!b1, !b1, !b1, !b1, !b1, !b1, !b1) -> (!b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1)
    "ac.expect"(%transaction#3, %transaction#4, %clk, %rst) {kind = "guard_known", location = #span0, ac.check_id = #conditional_check0} : (!b1, !b1, !b1, !b1) -> ()
    "ac.expect"(%transaction#5, %transaction#6, %clk, %rst) {kind = "guard_known", location = #span0, ac.check_id = #conditional_check1} : (!b1, !b1, !b1, !b1) -> ()
    "ac.expect"(%transaction#7, %transaction#8, %clk, %rst) {kind = "guard_known", location = #span0, ac.check_id = #conditional_check2} : (!b1, !b1, !b1, !b1) -> ()
    "ac.expect"(%transaction#9, %transaction#10, %clk, %rst) {kind = "guard_known", location = #span0, ac.check_id = #conditional_check3} : (!b1, !b1, !b1, !b1) -> ()
    "ac.expect"(%transaction#11, %transaction#12) {kind = "assert", location = #span0, ac.check_id = #conditional_check4} : (!b1, !b1) -> ()
    "ac.expect"(%transaction#13, %transaction#14) {kind = "assert", location = #span0, ac.check_id = #conditional_check5} : (!b1, !b1) -> ()
    "ac.yield"(%q) : (!b1) -> ()
  }) {sym_name = "Conditional", source_owner = {package = "", path = "sampled_admission.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1, !b1, !b1) -> !b1, input_names = ["selector", "then_guard", "else_guard", "asserted", "pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 4 : i64, reset = 5 : i64}, ac.transaction_wiring = {states = [#conditional_state], incoming = [], writers = [{registration = #conditional_registration, ordinary_results = 0 : i64, candidates = [{state = {owner = {instance_path = []}, declaration = #conditional_state, element = []}, data = 0 : i64, enable = 1 : i64}]}]}} : () -> ()

  "ac.module"() ({
  ^bb0(%left_first: !b1, %left_second: !b1, %right_first: !b1, %right_second: !b1, %left_clk: !b1, %left_rst: !b1, %right_clk: !b1, %right_rst: !b1, %sample_clk: !b1, %sample_rst: !b1, %selector: !b1, %then_guard: !b1, %else_guard: !b1, %asserted: !b1, %conditional_clk: !b1, %conditional_rst: !b1, %root_clk: !b1, %root_rst: !b1):
    %left_q = "ac.instance"(%left_first, %left_second, %left_clk, %left_rst) {instance_name = "left", callee = @Owner, parameters = [], type_arguments = [], occurrence = #left} : (!b1, !b1, !b1, !b1) -> !b1
    %right_q = "ac.instance"(%right_first, %right_second, %right_clk, %right_rst) {instance_name = "right", callee = @Owner, parameters = [], type_arguments = [], occurrence = #right} : (!b1, !b1, !b1, !b1) -> !b1
    "ac.instance"(%left_first, %sample_clk, %sample_rst) {instance_name = "pure", callee = @Pure, parameters = [], type_arguments = [], occurrence = #pure_instance} : (!b1, !b1, !b1) -> ()
    %conditional_q = "ac.instance"(%selector, %then_guard, %else_guard, %asserted, %conditional_clk, %conditional_rst) {instance_name = "conditional", callee = @Conditional, parameters = [], type_arguments = [], occurrence = #conditional_instance} : (!b1, !b1, !b1, !b1, !b1, !b1) -> !b1
    "ac.yield"(%left_q, %right_q, %conditional_q) : (!b1, !b1, !b1) -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "sampled_admission.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1) -> (!b1, !b1, !b1), input_names = ["left_first", "left_second", "right_first", "right_second", "left_clk", "left_rst", "right_clk", "right_rst", "sample_clk", "sample_rst", "selector", "then_guard", "else_guard", "asserted", "conditional_clk", "conditional_rst", "pyc_clk", "pyc_rst"], output_names = ["left_q", "right_q", "conditional_q"], ac.domain_inputs = {clock = 16 : i64, reset = 17 : i64}} : () -> ()

  "ac.system"() {entry = {callee = @Top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
