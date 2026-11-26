#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "select_group.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "select_group.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w24 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<24>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "select_group.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!b24 = !ac.bits<#w24>
!Data = !ac.struct<"Data">

module attributes {gfir.contract = 1 : i64} {
  ac.struct "Data" fields [{name = "first", type = !b8}, {name = "second", type = !b8}, {name = "third", type = !b8}]
  gfir.module @Choose(%pick: !b1, %yes: !Data, %no: !Data) -> (!Data)
      attributes {source_owner = {package = "", path = "select_group.py"}, input_names = ["pick", "yes", "no"], output_names = ["chosen"]} {
    %yes_first = ac.struct.get %yes["first"] : (!Data) -> !b8
    %no_first = ac.struct.get %no["first"] : (!Data) -> !b8
    %first = "ac.bits.select"(%pick, %yes_first, %no_first) : (!b1, !b8, !b8) -> !b8
    %yes_second = ac.struct.get %yes["second"] : (!Data) -> !b8
    %no_second = ac.struct.get %no["second"] : (!Data) -> !b8
    %second = "ac.bits.select"(%pick, %yes_second, %no_second) : (!b1, !b8, !b8) -> !b8
    %yes_third = ac.struct.get %yes["third"] : (!Data) -> !b8
    %no_third = ac.struct.get %no["third"] : (!Data) -> !b8
    %third = "ac.bits.select"(%pick, %yes_third, %no_third) : (!b1, !b8, !b8) -> !b8
    %chosen = ac.struct.create(%first, %second, %third) : (!b8, !b8, !b8) -> !Data
    gfir.yield %chosen : !Data
  }
  gfir.module @Top(%pick: !b1, %yes: !Data, %no: !Data) -> (!b24)
      attributes {source_owner = {package = "", path = "select_group.py"}, input_names = ["pick", "yes", "no"], output_names = ["chosen"]} {
    gfir.instance @choose of @Choose
    %chosen = gfir.call @choose(%pick, %yes, %no) : (!b1, !Data, !Data) -> !Data
    %first = ac.struct.get %chosen["first"] : (!Data) -> !b8
    %second = ac.struct.get %chosen["second"] : (!Data) -> !b8
    %third = ac.struct.get %chosen["third"] : (!Data) -> !b8
    %packed = "ac.bits.concat"(%first, %second, %third) : (!b8, !b8, !b8) -> !b24
    gfir.yield %packed : !b24
  }
  gfir.root @Top kind "module" {domains = []}
}
