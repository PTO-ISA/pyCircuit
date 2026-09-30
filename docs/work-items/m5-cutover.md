# M5 cutover and M4 remainder audit

Status: active. Product baseline: `26e096c0`; planning baseline: `d6897e08`.
Authorization: approved C1/C2/C3, R1/M1, and the user's request to start M5.
No new source/IR/CLI/schema contract is inferred from milestone coordination.

## M4 assessment

The revision-8 M4 exit has **zero open required items** at its accepted revision:
a usable documented macOS source-tree workflow, real per-source producers,
source-owned C++ TUs, standard same-executor C++/RTL runners, external oracle,
output protection and independent acceptance. The 35 reviewed code/fixture and
17 accepted documentation blobs were rechecked against commit 26e096c0.

This is not a percentage claim about all C3 productization. Four substantial
work groups still matter to the product:

| Remaining work | Placement and prerequisite |
| --- | --- |
| Public emit and retirement of old Python/native paths | M5 atomic cutover, after the promised target bundle is ready |
| Complete CPP DUT ABI and genuine target-specific artifact delivery | M5 prerequisites; begin with the ABI thin layer below |
| Source-owned RTL organization and approved source-map ownership/payload | M5 prerequisite for promised RTL delivery; resolve the payload mapping without inventing a schema |
| Unified runtime/install and current-platform external consumer smoke | M5; wider SDK/relocation/platform matrices continue in M6 |

M3 capabilities such as memory/CDC/four-state/complex parameterization remain
explicit backlog. SYSTEM/EXPECT B remain unapproved. Full three-platform SDK
coverage and actual parallel simulation are not silently included in M4.

## First execution packet: M5-A generated model ABI

C3-C already approves dut.h, agentic_model_query_v1, the seven-callback table,
ABI version 1 and 64/16/24-byte layouts. Generate a thin opaque handle around the
same FinalSystem and SimExecutor. No new scheduler, counters, lifecycle policy,
configuration field, ABI function or public emit dispatch.

- Implementation: unit_pair_fix, Luna high, only new FinalModelAbi.h/.cpp.
- PM: existing guarded native emission, private transport, generated CPP CMake
  shared target and receipt integration; model_api.h comment correction only.
- Independent tests: decl_cpp_impl, separate Luna high, C/ctypes consumer tests.
- Scope review: decl_arch_conformance, Astra xhigh, read-only; exact existing
  C3 approval covers this packet.
- Final review: other_agent_code_review, Sol high, no authorship.

The handle owns one system and one executor; create nulls its output before any
allocation and publishes only after successful Build. Operations forward to the
executor, including unlimited configure_json("{}"). The finite-limit rule belongs
to SystemRunner, not the C ABI. Exceptions do not escape C boundaries; destroy
accepts null; parameter/state failures preserve output buffers as specified.
Generated C glue depends on Runtime headers only, never compiler developer headers.

Acceptance: real generated shared DUT, pure C include/link/run, exact layout/table,
null/layout/lifecycle/error/output-preservation cases, invalid-config retry,
unlimited config, limits/ties, quiescence/hold, failed epoch and latched errors,
Reset/replay, independent handles and buffer lifetime. Reuse current source-owned
TUs; do not copy the retired QueueGraph wrapper. Keep current public routes intact
until the whole cutover candidate is ready; this packet alone is not M5 done.

## R01–R04 completion rules

R01 retires old semantic routes after their declared-profile replacements pass.
R02 removes aliases, fallback dispatch and install payloads, not merely CLI help.
R03 closes source/build/runtime dependency ownership and external consumer use.
R04 updates decisions, AGENTS/skills, active docs and gate owners in that same
cutover candidate. Historical oracles stay reference evidence; meaningful current
semantics are retained or explicitly reassigned, not deleted to turn tests green.

The source-map payload is a real unresolved mapping: C3 fixes ownership and file
roles, but copying QueueGraph v0.1 categories or inventing a replacement public
schema is not authorized. Resolve it independently while ABI implementation proceeds.

## Retiring-route inventory at 26e096c0

Read-only Sol audit found that the new driver needs nine implementation modules
plus package init, but the current eager init expands imports to 47 of 51 package
modules and still reaches semantic-core through legacy exports. This is an
import/dispatch cleanup requirement, not evidence that the new compiler secretly
uses the old lowering during an M4 model build.

| Surface | Concrete owning files and cutover action |
| --- | --- |
| Python exports/import closure | `python/pycircuit/src/pycircuit/__init__.py`, design.py, jit.py, dsl.py, v6.py, hw.py and legacy companions; preserve approved source names and private capture/orchestration, retire old builder/CAS/structural exports |
| CLI dispatch | python/pycircuit/src/pycircuit/cli.py still owns old Python-to-PYC emit; replace only when exact C3 output is ready |
| Active new driver closure | `_driver`, `_native_verify`, `_publication`, `_publication_fs`, `_source_compile`, `_source_capture`, `_source_transport`, `_source_unit_files`, `packaged_toolchain`, plus init |
| New receipt/preview route | `_generated_bundle.py` and flows/tools/materialize_m4_preview.py; integrate reusable publication into the real emit route or retire preview wiring at cutover |
| Dormant native engines | compiler/acir/lib/{Analysis,Bindings,CodeGen,Transforms} and matching headers/tools acc and acir-queue-* are absent from current ACIR CMake but still need source/test/registration removal |
| Root native/runtime graph | CMakeLists.txt still requires LLVM, compiler/mlir and library/cpp; preserve only proven RTL-private functionality while retiring PYC C++ and old runtime/build bindings |
| Install/wheel | cmake/pycircuitConfig.cmake.in still exposes PYCC; packaging/wheel/setup.py includes both frontend namespaces and six console entries, including the retired aliases |
| Active docs/gates | Decision supersession, AGENTS/skills, language/CLI docs, CI registrations and installed-package inventory must change with the actual cutover, not with this prerequisite packet |

Direct semantic-core imports in production pycircuit are concentrated in legacy
bitfield.py, bitmask.py and dsl.py; other direct users are Agentic/tests/gates.
Do not drop shared data utilities just by directory name without verifying their
remaining owners. The old PYC/ACIR TableGen incompatibility is source-confirmed;
this audit did not run a root build and does not claim fresh failure evidence.

M5-A also reserves the C ABI's global agentic_model_query_v1 name in shared C++
naming admission (module family, declaration and namespace). Nested source names
remain valid; this is target collision detection under approved C2-DECL/C3,
not a Python/source/IR naming prohibition.

## M5-A verification

The native helper and generated shared DUT build successfully. Twelve independent
ABI cases plus the existing M4/scalar/receipt lanes pass together: **141 passed,
zero skips**. C11 consumers link using dut.h and Runtime includes only; ctypes
also exercises the function table. Constructor exceptions and failed Build are
injected only in copied generated headers, and both preserve a null output handle.

The existing build-graph assertion now verifies one compile per consumer target:
standard runner, independent reset consumer and shared DUT. It retains exact
source ownership and adds a single model_api.cpp assertion. The newly occupied
global query symbol is rejected before C++ publication; nested names remain legal.

[Evidence](../gates/logs/20260930-m5-model-abi/README.md) binds the candidate;
independent architecture and code reviews both APPROVE. **M5-A is done.**
M5 as a whole remains active: this ABI prerequisite is not the public
emit/install/deletion cutover.
