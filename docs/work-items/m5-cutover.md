# M5 active: cutover prerequisites and retirement

Status: active. First prerequisite M5-A is accepted in product commit `a21bb596`
on `codex/gfsim-source-units`.

## M4 reassessment

At its accepted revision 26e096c0, all 35 code/fixture and 17 documentation hashes
match. Revision-8 M4 has zero open required items for the documented macOS
source-tree preview. That does not mean full C3 productization is finished.

Four work groups remain: public emit/export retirement; complete target bundle
and ABI delivery; source-owned RTL/source-map mapping; unified install/runtime
and current-platform external consumer use. Broader SDK/platform/performance
coverage remains M6. SYSTEM/EXPECT B remain unapproved.

## Delivered M5-A

The compiler generates dut.h and a thin opaque-handle wrapper around FinalSystem
and the existing SimExecutor. Seven v1 functions, enums, 64/16/24-byte layouts
and agentic_model_query_v1 remain unchanged. Source-owned CPP translation units
link into a shared DUT. No scheduler, lifecycle policy, new config field or public
CLI route is added. Unlimited ABI configure_json("{}") stays distinct from the
standalone runner's finite-limit requirement.

Twelve independent C11/ctypes/fault/name tests plus M4/scalar/receipt regressions
pass together: **141 passed, no skips**. Sol code and Astra architecture reviews
both APPROVE. Docs/lint gates pass; current-checkout native build succeeds.

- [Detailed implementation scope and deletion map](https://github.com/PTO-ISA/pyCircuit/blob/a21bb596/docs/work-items/m5-cutover.md)
- [Candidate-bound evidence](https://github.com/PTO-ISA/pyCircuit/blob/a21bb596/docs/gates/logs/20260930-m5-model-abi/README.md)

## Next cutover dependencies

Old eager package init expands the new driver's roughly ten-module closure into
47/51 modules. CLI, root CMake, runtime targets, wheel aliases/install payloads,
decisions and active gates must change together. Complete target delivery must
precede public emit replacement; preview must not merely be renamed.

The source-map payload is not fully specified by C3 ownership/file-role rules.
Confirm an approved reusable mapping or freeze the missing payload before
implementation; do not copy retired QueueGraph categories into a new schema.
M5-A does not complete M5, install a release or retire old routes yet.
