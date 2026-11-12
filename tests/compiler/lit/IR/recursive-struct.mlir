// RUN: %pycircuit_opt %s --ac-verify-hardware --verify-diagnostics
module {
  // expected-error @+1 {{recursive packed struct layout}}
  ac.struct "Recursive" fields [{name = "loop", type = !ac.struct<"Recursive">}]
}
