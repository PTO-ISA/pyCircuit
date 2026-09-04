// Small compatibility shim for the standalone OpenTitan primitive subset.
//
// The upstream tree includes a large assertion macro stack.  The runtime
// package keeps the functional RTL and uses this shim for the two combinational
// trees so Verilator and Yosys can consume the dependency closure without the
// full OpenTitan verification environment.  Parameter checks remain active;
// concurrent assertions are intentionally disabled in this structural package.
`ifndef PYC_RUNTIME_PRIM_ASSERT_SV
`define PYC_RUNTIME_PRIM_ASSERT_SV
`define ASSERT_INIT(name, property) \
  initial begin if (!(property)) $error("OpenTitan primitive parameter check failed: %s", `"name`"); end
`define ASSERT(name, property)
`endif
