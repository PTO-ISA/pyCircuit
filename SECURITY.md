# Security policy

pyCircuit is an evolving compiler and simulation toolchain. Do not use it as a
security boundary or assume untrusted design, MLIR, manifest, trace, or package
inputs are safe.

## Report a vulnerability

Report vulnerabilities privately through
[GitHub Security Advisories](https://github.com/PTO-ISA/pyCircuit/security/advisories/new).
Include the affected revision, environment, reproduction steps, impact, and any
suggested mitigation. Do not open a public issue or pull request before the
maintainers coordinate disclosure.

The maintainers will acknowledge the report through the advisory thread,
triage its scope, and coordinate remediation and disclosure there. This policy
does not promise a fixed response or release timeline.

## In scope

Examples include:

- code execution or path traversal from crafted compiler inputs;
- unsafe file writes or package extraction;
- memory-safety defects in generated or runtime C++ code;
- release artifact or dependency-integrity failures;
- disclosure of sensitive data through logs, traces, or diagnostics.

Functional correctness bugs without a security impact should follow
[`CONTRIBUTING.md`](CONTRIBUTING.md).
