# Public IR status

PYC is no longer a public compile or authoring route. The current public
workflow accepts Python source units, links them into one verified final
hardware design, and emits C++ or Verilog with the same compiler driver. The
internal IR is an implementation boundary, not a user-facing interchange
format in this profile.

The retired `pycc` command and PYC textual recipes are not supported. For the
active commands and support boundary, see the
[M5 migration guide](../development/m5-migration.md).
