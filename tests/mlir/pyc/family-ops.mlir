// RUN: %pyc_opt %s | %FileCheck %s

#owner = #ac.source_owner<"tests/family.py", "tests/family.py">
#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#args = #ac.static_arguments<[]>
#dependent_args = #ac.dependent_arguments<[]>
#schema = #ac.module_family_schema<
  #ac.static_parameters<[]>,
  #ac.static_cases<[#args]>,
  #ac.module_interface<[]>,
  #owner,
  []>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin>
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin>
#mapping = #pyc.module_port_mapping<[#clock, #reset], [], [], []>
#signature = #pyc.module_case_signature<
  #dependent_args, #ac.module_interface<[]>, (!pyc.clock, !pyc.reset) -> (), #mapping>

builtin.module {
  ac.type_scope @types {
    ac.struct @Pair fields [{name = "lo", type = i4}, {name = "hi", type = i4}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Pair> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  pyc.module @family source #owner schema #schema {
    pyc.module.case signature #signature source #prov {
    ^bb0(%clock_arg: !pyc.clock, %reset_arg: !pyc.reset):
      %true = pyc.constant 1 : i1
      pyc.assert %true {msg = "value must be bounded", obligation_id = "range:bounded", obligation_kind = "range", severity = "error", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = []}
      pyc.instance %clock_arg, %reset_arg {
        callee = @family,
        name = "child",
        static_args = #dependent_args
      } : (!pyc.clock, !pyc.reset) -> ()
      pyc.return
    }
  }
  pyc.module.import @external source #owner schema #schema

  pyc.module @aggregate source #owner schema #ac.module_family_schema<
      #ac.static_parameters<[]>, #ac.static_cases<[#args]>,
      #ac.module_interface<[
        #ac.interface_port<"value", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.struct<@types::@Pair>>>, #prov>,
        #ac.interface_port<"result", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.struct<@types::@Pair>>>, #prov>
      ]>, #owner, [@Pair]> {
    pyc.module.case signature #pyc.module_case_signature<
        #dependent_args,
        #ac.module_interface<[
          #ac.interface_port<"value", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.struct<@types::@Pair>>>, #prov>,
          #ac.interface_port<"result", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.struct<@types::@Pair>>>, #prov>
        ]>,
        (!pyc.clock, !pyc.reset, i8) -> i8,
        #pyc.module_port_mapping<
          [#clock, #reset],
          [
            #pyc.logical_port_mapping<"input", 0, "value", #ac.type_expr<#ac.type_expr_concrete<!ac.struct<@types::@Pair>>>, [#pyc.physical_port<"input", 2, i8, "value" layout #pyc.layout<8, [
              #pyc.packed_leaf<#pyc.projection_path<[#pyc.projection_step<#pyc.projection_field<"lo">>]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 0, 4>,
              #pyc.packed_leaf<#pyc.projection_path<[#pyc.projection_step<#pyc.projection_field<"hi">>]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 4, 4>
            ]>>], #prov>,
            #pyc.logical_port_mapping<"output", 0, "result", #ac.type_expr<#ac.type_expr_concrete<!ac.struct<@types::@Pair>>>, [#pyc.physical_port<"result", 0, i8, "value" layout #pyc.layout<8, [
              #pyc.packed_leaf<#pyc.projection_path<[#pyc.projection_step<#pyc.projection_field<"lo">>]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 0, 4>,
              #pyc.packed_leaf<#pyc.projection_path<[#pyc.projection_step<#pyc.projection_field<"hi">>]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 4, 4>
            ]>>], #prov>
          ],
          [#pyc.physical_port<"input", 2, i8, "value" layout #pyc.layout<8, [
            #pyc.packed_leaf<#pyc.projection_path<[#pyc.projection_step<#pyc.projection_field<"lo">>]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 0, 4>,
            #pyc.packed_leaf<#pyc.projection_path<[#pyc.projection_step<#pyc.projection_field<"hi">>]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 4, 4>
          ]>>],
          [#pyc.physical_port<"result", 0, i8, "value" layout #pyc.layout<8, [
            #pyc.packed_leaf<#pyc.projection_path<[#pyc.projection_step<#pyc.projection_field<"lo">>]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 0, 4>,
            #pyc.packed_leaf<#pyc.projection_path<[#pyc.projection_step<#pyc.projection_field<"hi">>]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 4, 4>
          ]>>]
        >> source #prov {
    ^bb0(%clock_arg: !pyc.clock, %reset_arg: !pyc.reset, %value: i8):
      pyc.return %value : i8
    }
  }
}

// CHECK: pyc.module @family
// CHECK: pyc.module.case signature #pyc.module_case_signature<
// CHECK: pyc.assert
// CHECK-SAME: obligation_id = "range:bounded"
// CHECK: pyc.instance
// CHECK-SAME: static_args = #ac.dependent_arguments<[]>
// CHECK: pyc.return
// CHECK: pyc.module.import @external
// CHECK: pyc.module @aggregate
// CHECK: #pyc.projection_field<"lo">
// CHECK: #pyc.projection_field<"hi">
