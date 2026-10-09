# Review and merge

Review the change against its approved contract and its exact candidate. The
active product route is `pycircuit compile` → `pycircuit link` →
`pycircuit emit`; the current source profile is intentionally bounded.

## Review checklist

- Does the change stay within the approved Python, IR, CLI, runtime, and
  packaging contracts?
- Are typed module definitions, separate instances, stateless rules, explicit
  registration and standard storage leaves described consistently? Do Work
  outputs observe old Q and whole-system checks precede Xfer commits?
- Are unsupported capabilities rejected without a fallback route?
- Does each source compile independently, with parent compilation consuming
  published interfaces and link receiving the complete closure?
- Do C++ and Verilog use the same verified final artifact and preserve source
  ownership?
- Does Runtime-only CMake discovery avoid LLVM/MLIR, while CompilerDev pins
  exact 22.1.8?
- Are generated output protections, diagnostics, and package inventories
  covered by candidate-bound evidence?
- Do active docs distinguish current support from remaining capability
  backlog and historical callers?

## Evidence and status

A reviewer should be able to reproduce the relevant gate from the stated
checkout and inspect raw output. A plan, fixture, test name, or previous
revision's evidence is not a current pass. Record skipped checks and unresolved
callers explicitly. A docs-only migration pass must not claim source-unit cutover implementation,
verification, or completion.

See [testing and gates](testing-and-gates.md), the
[source-unit workflow](source-unit-workflow.md), and the active
[language reference](../reference/language.md).
