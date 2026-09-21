// RUN: %pyc_opt %s | %FileCheck %s

#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#args = #ac.dependent_arguments<[]>
#iface = #ac.module_interface<[]>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock_a = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk_a">
#reset_a = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst_a">
#clock_b = #pyc.control_port_mapping<"clock", 2, !pyc.clock, #clock_origin, "clk_b">
#reset_b = #pyc.control_port_mapping<"reset", 3, !pyc.reset, #reset_origin, "rst_b">
#mapping = #pyc.module_port_mapping<[#clock_a, #reset_a, #clock_b, #reset_b], [], [], []>

module attributes {
  pyc.case_signature = #pyc.module_case_signature<
      #args, #iface, (!pyc.clock, !pyc.reset, !pyc.clock, !pyc.reset) -> (), #mapping>
} {
}

// Decision 0126: a module carries one control pair per clock domain, in source
// order, and every control keeps the source port name so the emitted module
// exposes `clk_a`/`clk_b` rather than a generic `clk`/`clk_2`.
// CHECK: pyc.case_signature = #pyc.module_case_signature<
// CHECK-SAME: "clk_a">
// CHECK-SAME: "rst_a">
// CHECK-SAME: "clk_b">
// CHECK-SAME: "rst_b">
