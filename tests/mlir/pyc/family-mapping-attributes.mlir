// RUN: %pyc_opt %s | %FileCheck %s

#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#args = #ac.dependent_arguments<[]>
#iface = #ac.module_interface<[]>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk">
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst">
#mapping = #pyc.module_port_mapping<[#clock, #reset], [], [], []>

module attributes {
  pyc.case_signature = #pyc.module_case_signature<
      #args, #iface, (!pyc.clock, !pyc.reset) -> (), #mapping>
} {
}

// CHECK: #pyc.module_case_signature<
// CHECK-SAME: !pyc.clock
// CHECK-SAME: !pyc.reset
