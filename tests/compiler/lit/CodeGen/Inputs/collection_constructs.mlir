#wm1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<-1>}}>
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b4 = !ac.bits<#w4>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b70 = !ac.bits<#w70>
!a = !ac.table<[#w2, #w3], !b8>
!at = !ac.table<[#w3, #w2], !b8>
!as = !ac.table<[#w2, #w2], !b8>
!af = !ac.table<[#w6], !b8>
!apair = !ac.table<[#w2], !b8>
#w7 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<7>}}>
#w65 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
!b7 = !ac.bits<#w7>
!input = !ac.table<[#w3], !b8>
!nested = !ac.table<[#w2], !ac.table<[#w3], !b8>>
!ordinals = !ac.table<[#w65], !b7>
#w1606938044258990275541962092341162602522202993782792835301377 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1606938044258990275541962092341162602522202993782792835301377>}}>
#wm1606938044258990275541962092341162602522202993782792835301377 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<-1606938044258990275541962092341162602522202993782792835301377>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!record = !ac.struct<"QueryInner">
!query_mark = !ac.enum<"QueryMark">
!query_payload = !ac.struct<"QueryPayload">
!query_source = !ac.table<[#w3], !query_payload>
!query_indices = !ac.table<[#w5], !b70>
!query_indices_rows = !ac.table<[#w3, #w5], !b70>
!query_indices_grid = !ac.table<[#w5, #w3], !b70>
!query_payload_grid = !ac.table<[#w5, #w3], !query_payload>
!query_guards_grid = !ac.table<[#w5, #w3], !b1>
!query_ordinals = !ac.table<[#w3], !b2>
!query_ordinals_grid = !ac.table<[#w5, #w3], !b2>
!query_pair_payload = !ac.table<[#w5, #w1], !query_payload>
!query_pair_guard = !ac.table<[#w5, #w1], !b1>
!query_outputs = !ac.table<[#w5], !query_payload>
!query_output_guards = !ac.table<[#w5], !b1>
module {
  ac.struct "QueryInner" fields [{name = "tag", type = !b4}, {name = "data", type = !b8}]
  "ac.enum"() {sym_name = "QueryMark", width = #ac.math_int<2>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}, {name = "TWO", code = #ac.math_int<2>}, {name = "THREE", code = #ac.math_int<3>}]} : () -> ()
  ac.struct "QueryPayload" fields [{name = "nested", type = !record}, {name = "mark", type = !query_mark}]
  "ac.module"() ({
  ^bb0(%data: !query_source, %indices: !query_indices):
    %payloads = "ac.table.splat"(%data) {shape = [#w5]} : (!query_source) -> !query_payload_grid
    %index_rows = "ac.table.splat"(%indices) {shape = [#w3]} : (!query_indices) -> !query_indices_rows
    %index_grid = "ac.table.view"(%index_rows) {kind = "transpose", parameters = {axes = [#w1, #w0]}} : (!query_indices_rows) -> !query_indices_grid
    %ordinals = "ac.table.map"() ({
    ^bb0(%ordinal: !b2):
      "ac.yield"(%ordinal) : (!b2) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 0, 0>} : () -> !query_ordinals
    %ordinal_grid = "ac.table.splat"(%ordinals) {shape = [#w5]} : (!query_ordinals) -> !query_ordinals_grid
    %guards, %candidates = "ac.table.map"(%index_grid, %ordinal_grid, %payloads) ({
    ^bb0(%position: !b4, %index: !b70, %ordinal: !b2, %payload: !query_payload):
      %wide = "ac.bits.resize"(%ordinal) {mode = "zext"} : (!b2) -> !b70
      %equal = "ac.bits.compare"(%index, %wide) {predicate = "eq"} : (!b70, !b70) -> !b1
      "ac.yield"(%equal, %payload) : (!b1, !query_payload) -> ()
    }) {shape = [#w5, #w3], operandSegmentSizes = array<i32: 3, 0>} : (!query_indices_grid, !query_ordinals_grid, !query_payload_grid) -> (!query_guards_grid, !query_payload_grid)
    %g0 = "ac.table.view"(%guards) {kind = "slice", parameters = {offsets = [#w0, #w0], sizes = [#w5, #w1], strides = [#w1, #w2]}} : (!query_guards_grid) -> !query_pair_guard
    %g1 = "ac.table.view"(%guards) {kind = "slice", parameters = {offsets = [#w0, #w1], sizes = [#w5, #w1], strides = [#w1, #w2]}} : (!query_guards_grid) -> !query_pair_guard
    %p0 = "ac.table.view"(%candidates) {kind = "slice", parameters = {offsets = [#w0, #w0], sizes = [#w5, #w1], strides = [#w1, #w2]}} : (!query_payload_grid) -> !query_pair_payload
    %p1 = "ac.table.view"(%candidates) {kind = "slice", parameters = {offsets = [#w0, #w1], sizes = [#w5, #w1], strides = [#w1, #w2]}} : (!query_payload_grid) -> !query_pair_payload
    %gp, %pp = "ac.table.map"(%g0, %g1, %p0, %p1) ({
    ^bb0(%ordinal: !b3, %left_valid: !b1, %right_valid: !b1, %left: !query_payload, %right: !query_payload):
      %valid = "ac.bits.binary"(%left_valid, %right_valid) {opcode = "or"} : (!b1, !b1) -> !b1
      %selected, %unused = "ac.value.merge"(%right, %left_valid, %left) {paths = [[]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!query_payload, !b1, !query_payload) -> (!query_payload, !b1)
      "ac.yield"(%valid, %selected) : (!b1, !query_payload) -> ()
    }) {shape = [#w5, #w1], operandSegmentSizes = array<i32: 4, 0>} : (!query_pair_guard, !query_pair_guard, !query_pair_payload, !query_pair_payload) -> (!query_pair_guard, !query_pair_payload)
    %gt = "ac.table.view"(%guards) {kind = "slice", parameters = {offsets = [#w0, #w2], sizes = [#w5, #w1], strides = [#w1, #w1]}} : (!query_guards_grid) -> !query_pair_guard
    %pt = "ac.table.view"(%candidates) {kind = "slice", parameters = {offsets = [#w0, #w2], sizes = [#w5, #w1], strides = [#w1, #w1]}} : (!query_payload_grid) -> !query_pair_payload
    %gr = "ac.table.view"(%gp) {kind = "reshape", parameters = {shape = [#w5]}} : (!query_pair_guard) -> !query_output_guards
    %pr = "ac.table.view"(%pp) {kind = "reshape", parameters = {shape = [#w5]}} : (!query_pair_payload) -> !query_outputs
    %tr = "ac.table.view"(%gt) {kind = "reshape", parameters = {shape = [#w5]}} : (!query_pair_guard) -> !query_output_guards
    %tp = "ac.table.view"(%pt) {kind = "reshape", parameters = {shape = [#w5]}} : (!query_pair_payload) -> !query_outputs
    %reduced = "ac.table.map"(%gr, %tr, %pr, %tp) ({
    ^bb0(%ordinal: !b3, %left_valid: !b1, %right_valid: !b1, %left: !query_payload, %right: !query_payload):
      %selected, %unused = "ac.value.merge"(%right, %left_valid, %left) {paths = [[]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!query_payload, !b1, !query_payload) -> (!query_payload, !b1)
      "ac.yield"(%selected) : (!query_payload) -> ()
    }) {shape = [#w5], operandSegmentSizes = array<i32: 4, 0>} : (!query_output_guards, !query_output_guards, !query_outputs, !query_outputs) -> !query_outputs
    %sentinel = "ac.bits.constant"() {value = #w3} : () -> !b70
    %poison, %poison_range = "ac.table.get"(%data, %sentinel) : (!query_source, !b70) -> (!query_payload, !b1)
    %poisons = "ac.table.splat"(%poison) {shape = [#w5]} : (!query_payload) -> !query_outputs
    %gathered = "ac.table.map"(%indices, %reduced, %poisons) ({
    ^bb0(%ordinal: !b3, %index: !b70, %candidate: !query_payload, %unknown: !query_payload):
      %limit = "ac.bits.constant"() {value = #w3} : () -> !b70
      %in_range = "ac.bits.compare"(%index, %limit) {predicate = "ult"} : (!b70, !b70) -> !b1
      %selected, %unused = "ac.value.merge"(%unknown, %in_range, %candidate) {paths = [[]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!query_payload, !b1, !query_payload) -> (!query_payload, !b1)
      "ac.yield"(%selected) : (!query_payload) -> ()
    }) {shape = [#w5], operandSegmentSizes = array<i32: 3, 0>} : (!query_indices, !query_outputs, !query_outputs) -> !query_outputs
    "ac.yield"(%gathered) : (!query_outputs) -> ()
  }) {sym_name = "query_gather", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!query_source, !query_indices) -> !query_outputs, input_names = ["data", "indices"], output_names = ["gathered"]} : () -> ()
  "ac.module"() ({
  ^bb0(%input: !input, %query_data: !query_source, %query_indices: !query_indices):
    %nested = "ac.table.splat"(%input) {shape = [#w2]} : (!input) -> !nested
    %ordinals = "ac.table.map"() ({
    ^bb0(%ordinal: !b7):
      "ac.yield"(%ordinal) : (!b7) -> ()
    }) {shape = [#w65], operandSegmentSizes = array<i32: 0, 0>} : () -> !ordinals
    %large = "ac.table.view"(%input) {kind = "rotate", parameters = {axis = #w0, offset = #w1606938044258990275541962092341162602522202993782792835301377}} : (!input) -> !input
    %negative = "ac.table.view"(%input) {kind = "rotate", parameters = {axis = #w0, offset = #wm1606938044258990275541962092341162602522202993782792835301377}} : (!input) -> !input
    %sum = "ac.table.fold"(%ordinals) {kind = "add"} : (!ordinals) -> !b7
    %gathered = "ac.instance"(%query_data, %query_indices) {instance_name = "gather", callee = @query_gather, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}]}, expansion = []}} : (!query_source, !query_indices) -> !query_outputs
    %idxpos0 = "ac.bits.constant"() {value = #w0} : () -> !b3
    %idx0, %idxrange0 = "ac.table.get"(%query_indices, %idxpos0) : (!query_indices, !b3) -> (!b70, !b1)
    %ref0, %range0 = "ac.table.get"(%query_data, %idx0) : (!query_source, !b70) -> (!query_payload, !b1)
    %idxpos1 = "ac.bits.constant"() {value = #w1} : () -> !b3
    %idx1, %idxrange1 = "ac.table.get"(%query_indices, %idxpos1) : (!query_indices, !b3) -> (!b70, !b1)
    %ref1, %range1 = "ac.table.get"(%query_data, %idx1) : (!query_source, !b70) -> (!query_payload, !b1)
    %idxpos2 = "ac.bits.constant"() {value = #w2} : () -> !b3
    %idx2, %idxrange2 = "ac.table.get"(%query_indices, %idxpos2) : (!query_indices, !b3) -> (!b70, !b1)
    %ref2, %range2 = "ac.table.get"(%query_data, %idx2) : (!query_source, !b70) -> (!query_payload, !b1)
    %idxpos3 = "ac.bits.constant"() {value = #w3} : () -> !b3
    %idx3, %idxrange3 = "ac.table.get"(%query_indices, %idxpos3) : (!query_indices, !b3) -> (!b70, !b1)
    %ref3, %range3 = "ac.table.get"(%query_data, %idx3) : (!query_source, !b70) -> (!query_payload, !b1)
    %idxpos4 = "ac.bits.constant"() {value = #w4} : () -> !b3
    %idx4, %idxrange4 = "ac.table.get"(%query_indices, %idxpos4) : (!query_indices, !b3) -> (!b70, !b1)
    %ref4, %range4 = "ac.table.get"(%query_data, %idx4) : (!query_source, !b70) -> (!query_payload, !b1)
    %reference = "ac.table.create"(%ref0, %ref1, %ref2, %ref3, %ref4) {shape = [#w5]} : (!query_payload, !query_payload, !query_payload, !query_payload, !query_payload) -> !query_outputs
    "ac.yield"(%nested, %ordinals, %sum, %large, %negative, %gathered, %reference) : (!nested, !ordinals, !b7, !input, !input, !query_outputs, !query_outputs) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!input, !query_source, !query_indices) -> (!nested, !ordinals, !b7, !input, !input, !query_outputs, !query_outputs), input_names = ["input", "query_data", "query_indices"], output_names = ["nested", "ordinals", "sum", "large", "negative", "query_gathered", "query_reference"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
