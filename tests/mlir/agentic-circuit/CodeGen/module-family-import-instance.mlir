// A split-module unit declares its siblings as `pyc.module.import`, so the
// instance has to resolve and wire through the declared interface. This used to
// fail with "callee symbol not found" because only `pyc.module` families were
// recognised.
// RUN: rm -rf %t
// RUN: %pycc %s --emit=cpp --cpp-split=module --out-dir %t/cpp
// RUN: test -f %t/cpp/top.hpp
// RUN: grep -q '#include "child.hpp"' %t/cpp/top.hpp

#prov = #ac.source_provenance<"tests/top.py", 1, 1, 1, 8>
#child_prov = #ac.source_provenance<"tests/child.py", 1, 1, 1, 8>
#owner = #ac.source_owner<"tests/top.py", "tests/top.py">
#child_owner = #ac.source_owner<"tests/child.py", "tests/child.py">
#dep = #ac.dependent_arguments<[]>
#i8 = #ac.type_expr<#ac.type_expr_concrete<i8>>
#interface = #ac.module_interface<[
  #ac.interface_port<"x", "input", #i8, #prov>,
  #ac.interface_port<"y", "output", #i8, #prov>
]>
#schema = #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #interface, #owner, []>
#child_schema = #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #interface, #child_owner, []>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk">
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst">
#layout = #pyc.layout<8, [#pyc.packed_leaf<#pyc.projection_path<[]>, #i8, 0, 8>]>
#input = #pyc.physical_port<"input", 2, i8, "value" layout #layout>
#result = #pyc.physical_port<"result", 0, i8, "value" layout #layout>
#logical_input = #pyc.logical_port_mapping<"input", 0, "x", #i8, [#input], #prov>
#logical_output = #pyc.logical_port_mapping<"output", 0, "y", #i8, [#result], #prov>
#mapping = #pyc.module_port_mapping<[#clock, #reset], [#logical_input, #logical_output], [#input], [#result]>
#signature = #pyc.module_case_signature<#dep, #interface, (!pyc.clock, !pyc.reset, i8) -> i8, #mapping>

module attributes {pyc.top = @top, pyc.frontend.contract = "pycircuit"} {
  pyc.module.import @child source #child_owner schema #child_schema

  pyc.module @top source #owner schema #schema {
    pyc.module.case signature #signature source #prov {
    ^bb0(%clk: !pyc.clock, %rst: !pyc.reset, %x: i8):
      %y = pyc.instance %clk, %rst, %x {callee = @child, name = "u0", static_args = #dep} : (!pyc.clock, !pyc.reset, i8) -> i8
      pyc.return %y : i8
    }
  }
}
