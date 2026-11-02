# pyCircuit design gallery

Each directory is a small, self-contained hardware project using the public
pyCircuit SDK. Projects own their hardware, NDF specification, dependency pin,
independent verification and results. Build each project on its own; membership
in this gallery does not register it in the compiler's example smoke gates.

| Project | Design | Start here |
| --- | --- | --- |
| [RV32I core](rv32i_core/README.md) | PyCircuit 项目 demo：以五级 CPU 展示 NDF → Spec → PyCircuit → C++/RTL 开发与验证流程；[中文实验报告](rv32i_core/EXPERIMENT.md) | [NDF specification](rv32i_core/spec/README.md), [verification tools](rv32i_core/verification/TOOLS.md) |
| [New project template](template/README.md) | Generate compute, stateful or backpressure models with L0–L3 NDF documents | `pycircuit init designs/my_design --template stateful` |

## Add a design

Use the installed `pycircuit init` command to create a new directory, then adapt
its source, specification and independent expectations together. Keep the
project name and NDF clause identities stable once published. Add one gallery
row linking its README and verification instructions.

Each project should contain:

- A README with purpose, supported interfaces, build/run commands and limitations.
- Hardware source and a standalone CMake/project configuration using the public SDK.
- `ndf.yaml`, `ndf.lock` and `spec/` with intent (L0), requirements (L1), implementation
  specification (L2) and verification criteria (L3).
- `verification/` with independent expectations and reproducible test entrypoints.
- An exact pyCircuit dependency record and separately scoped validation results.

Keep build outputs under ignored `.pycircuit_out/`. The RV32I project also
publishes generated C++/RTL and a mapped netlist as a reproducible
deliverable; its SDK, caches, executables and cell-library dependencies remain
external. Record actual tested
revisions and commands; a draft specification, successful graph check or
successful compile is not behavioral validation. The [teaching examples](../examples/README.md)
remain the place for focused language demonstrations and compiler smoke tests.
