// RUN: %pycircuit_opt %s --verify-diagnostics
module {
  // expected-error @+1 {{recursive packed struct layout}}
  ac.struct "Recursive" fields [{name = "loop", type = !ac.struct<"Recursive">}]
}
