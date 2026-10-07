# C3-DUT-IO: typed external boundary for a single-clock system

Revision: B. Status: draft for independent Astra review; not approved or implemented.
Architecture: dut_resource_design, Astra xhigh; PM transcription. This extension
changes C1 system authoring, C2 root bindings, and C3 I/O through a separate
optional ABI table and generated C++ helpers. Approved C1-C/C2-C/C3-C remain frozen.

## Scope and donor basis

Use the existing `@system` name for a selected externally driven root, with
ordinary finite typed constructor parameters and constant defaults. Reuse
module/rule/current-next semantics and source-owned interfaces. Keep the approved
portless `@module` entry. Add no CLI flag, ownership sidecar, public Queue API,
internal-state accessor, or second runtime.

This extension covers bool, finite 1–64-bit mathematical integers, and nonempty
nominal records recursively containing those values; one default rising-edge
domain and active-high synchronous reset. Public list/tuple/Enum, four-state
values, combinational outputs, clock selection, full source testbench actions,
and resource ports remain separate required proposals.

Donor GFSIM b852ed83fa0288d0be7406bba0ed47be4b2c0f63,
`frontend/ObjectFrontendProposal.md:737–806`, describes a closed source system;
`compiler/acir/lib/PythonLower.cpp:568–571` rejects its lowering. External system
parameters are an explicit pyCircuit extension, not existing donor functionality.

## Exact Python surface

`@system` takes no arguments and decorates one ordinary class. Its constructor,
aliases, children, rules, and static parameters follow C1. A source owns either
one public module or one public system, never both. A system is root-only:
instantiation as a child, inheritance, and runtime-selected roots reject.

Each connection parameter requires a finite annotation and explicit default
resolved by the MLIR constant evaluator to a complete matching value. System
connection defaults declare boundary reset/idle values. They are not child
constructor arguments and do not imply static classification. Ordinary
`@module` connection defaults keep C1/C2 meaning and do not acquire storage.
Defaults in this boundary profile cannot depend on constructor parameters.

After specialization and transitive effect closure:

| Exact effects | External direction and ownership |
| --- | --- |
| R only | Input: environment owns sampled value; rules may only read. |
| W or R+W | Output: boundary owns one committed DFFE; internal readers see current, host only observes. |
| Neither | Reject unused boundary parameter. |

An output's current/next endpoints share one identity through all children.
Multiple writers retain C1 checks/rejections. Names, defaults and C++ types do
not determine direction. Request and reply use separate parameters; bidirectional
host/model mutation is not admitted.

```python
# packet.py
from typing import Annotated

Word = Annotated[int, range(256)]

class Request:
    value: Word
    valid: bool

    def __init__(self, value: Word = 0, valid: bool = False):
        self.value = value
        self.valid = valid
```

```python
# accumulator.py
from pycircuit import module, rule
from .packet import Request, Word

@module
class Accumulator:
    def __init__(self, request: Request, bias: Word, result: Word):
        self.request = request
        self.bias = bias
        self.result = result
        self.total: Word = 0
        self.advance()

    @rule
    def advance(self):
        if self.request.valid:
            candidate = (self.total + self.request.value + self.bias) & 255
            self.total = candidate
            self.result = candidate
```

```python
# top.py
from pycircuit import system
from .packet import Request, Word
from .accumulator import Accumulator

@system
class Top:
    def __init__(self, request: Request = Request(),
                 bias: Word = 0, result: Word = 7):
        self.request = request
        self.bias = bias
        self.result = result
        self.accumulator = Accumulator(self.request, self.bias, self.result)
```

Compile each unit independently; select Top with existing
`pycircuit link ... --top <qualified-name>`. `--parameters` binds static parameters
only; connection values there reject. Python constructors are never executed.

## Exact common IR extension

System body/header declarations carry `ac.root_kind="system"`; ordinary modules
omit it. This attribute and connection defaults match the authoritative header.
Existing C2 Parameter/Default records hold defaults. Final `ac.module` retains
ordinary formal current/next slots, rule results, and data/enable ordering.

Add one regionless, operandless, resultless `ac.dut` operation to the linked
builtin module. It declares boundary ownership and is the effectful intrinsic
entry-control operation that admits and invokes the selected root. It cannot
carry Pure, be speculated, duplicated or removed. Fixed ingress semantics are
part of common IR, not emitter policy. The exact schema is:

```text
ac.dut {
  entry: SpecKey,
  ac.source_owner: SourceOwner,
  ports: Array<DutPort>,
  bindings: Array<DutBinding>
}
DutPort = {
  parameter: StringAttr,
  direction: "input" | "output",
  type: LogicalType.Bool | LogicalType.Integer | LogicalType.Record,
  initial: StaticValue,
  state: StateID,
  origin: Occurrence,
  location: SourceSpan
}
DutBinding = {port: u32, boundary: u32}
```

All fields are required; unknown fields reject. A selected system has exactly
one ac.dut; ordinary portless modules have none. entry equals the C2 program
entry. Ports follow connection declaration order; each record is one logical
value. State is `{owner:{instance_path:[]}, declaration:<parameter occurrence>,
element:[]}`. This adds precise StateID authority: input rows provide sampled
external current; output rows declare boundary-owned DFFEs with reset images.
Neither is an alias of an arbitrary member.

Bindings cover every root ac.ports ordinal exactly once and map it to the same
named parameter's boundary row. Input has current only. Output has next and,
when internally read, current to the same row. Root ac.instance_bindings must
agree with these StateIDs.

Input current handles have no local DFFE, next driver, or commit target; they
resolve to the captured external sample. Output backing is one DFFE in root/interface
glue, committed from the existing root borrowed-next result. ac.dut declares its
storage/reset; the emitter cannot infer it. No extra module, source unit, relay
register or source-state copy is introduced. Child interfaces/forwarding retain C2.

The final verifier independently checks root kind, closed types/defaults,
effect-derived direction, ownership, binding coverage, actual formals, and
output commit targets. Swapped same-type bindings, input writes, hidden output
aliases, a non-root boundary, and missing reset/commit authority reject. Ingress
domain checks derive from these declarations. Both emitters consume the identical
verified program; no backend reconstructs admission or host-computed transitions.

## Intrinsic ingress authority

Before this extension C2 runtime ac.expect checks are rule-owned and use
rule-registration CheckIDs. Those rules remain unchanged. ac.dut additionally
owns intrinsic validation of external samples and admission of its selected
root. It creates no synthetic rule, CheckID, source assertion kind or
source-visible failure state.

The intrinsic derives ordered input leaves from port rows and authoritative
nominal declarations: parameter order, recursively field declaration order.
Each leaf resolves through bindings, root formals and instance bindings to the
actual external current. Logical types constrain unchecked bits; they do not
prove those bits valid.

Every leaf requires known physical bits, a two-state bool value, or an integer
whose declared-width/signedness interpretation satisfies its full interval.
Check every leaf even if source control would not read it. Ingress performs only
safe knownness/bit/range operations. No source rule, helper, assertion, dynamic
access or arithmetic evaluates before admission.

At the default-domain active edge, the intrinsic selects exactly one alternative:

```text
reset  = reset pin is known 1
run    = reset pin is known 0 AND every input leaf is valid
reject = NOT (reset OR run)
```

Predicates are two-valued. RTL reset uses case equality; knownness is equivalent
to `((^raw_leaf) !== 1'bx)` before accepting the range result.

| Alternative | Behavior |
| --- | --- |
| reset | Restore all source-owned/boundary-output reset images; ignore invalid data; no source evaluation or data-ingress diagnostic. |
| run | Invoke root with sampled inputs/current Q; C2 source/check/driver failures still prohibit every commit. |
| reject | No source evaluation, next drive, state/output commit or successful-cycle update; preserve all Q. |

Clock remains a clean known 0/1 clock; unknown-clock execution is undefined by
this extension. X/Z reset at an otherwise valid edge selects reject.

ac.dut is the sole invocation for the selected system. Replacing it by an
ordinary ac.instance, adding a bypass invocation, or directly calling root
from a backend is not a permitted final form. Admission governs every descendant
and all ordinary/boundary commits, including children that never read an input.
Per-output gating or checks performed only after source evaluation are insufficient.

The final verifier reconstructs the leaf/binding inventory and rejects missing
or duplicate intrinsic, ordinary-instance substitution or extra ungated entry,
omitted/redirected/duplicate bindings, domain/formal mismatches, attributes that
disable checks/admission, or root/commit owners outside its complete instance
closure. There is no checks-already-performed flag. Both real emit entries reject
removing or redirecting this obligation. RTL-private verification additionally
checks materialized root-evaluation and every commit control against admission;
this does not create another serialized scheduler schema.

### Ingress diagnostics and recovery

Each rejected RTL edge emits exactly one simulation `$error`, selecting reset
first, otherwise the first invalid leaf:

```text
pycircuit_ingress_invalid: <reset-or-parameter.field-path>: <unknown|range>
```

The diagnostic itself neither terminates simulation nor adds a failure latch.
Corrected valid pins can execute next edge without Reset. Known asserted reset
wins over invalid data and restores reset state.

| Case | Continuation |
| --- | --- |
| Invalid C++ drive before admission | Reject entire batch; preserve I/Q/snapshot/time and healthy lifecycle. |
| Invalid RTL sample at edge | Reject that edge, preserve Q; corrected next edge may run. |
| Source/check failure after admission | Existing C2/C3 failure and whole-tree Reset requirements. |
| C++ step finds invalid retained input despite API validation | Internal invariant failure before source evaluation: RUNTIME_FAILURE/FAILED, Reset required. |

The last case uses code=runtime_failure, phase=evaluate, root/parameter source
location and null check_id; it is not normal malformed-drive behavior. No new
C3 diagnostic field or source-check kind is introduced.

Independent ingress tests use an externally constrained denominator and an
unrelated free-running child counter. Invalid range/X/Z must emit only the
boundary diagnostic, avoid source arithmetic/assertion diagnostics and preserve
both children's outputs. Corrected pins advance both on the next valid edge.
Repeat with asserted reset over invalid data and X/Z reset.

## Sampling and reset schedule

Let I be the retained external input batch and Q the committed DUT state.

1. Successful Reset installs input defaults in I, resets every output/owned
   state, and publishes an output snapshot at time zero. It discards old retained
   inputs and temporary samples.
2. Successful drive validates and copies the entire batch into I atomically.
   It does not change Q, execute rules, publish outputs, or advance time.
   A second drive replaces the whole batch.
3. An accepted step captures immutable I[t] before EvaluateNext. Every rule in
   the complete tree reads that sample and Q[t]. There is no extra input cycle.
4. Whole-tree EvaluateNext → CheckNext → DriveNext → Xfer produces Q[t+1]. After
   complete successful publication, output snapshot and success count publish.
5. Without another drive the same input repeats. A valid field is a level;
   the framework never clears or consumes it. Handshake/backpressure and
   repeated-command avoidance are authored model behavior.

Sampling returns the last complete reset/commit snapshot without consuming it.
Source/check failure leaves Q and snapshot unchanged. Internal Drive/Xfer failure
retains C3 Failed/Reset requirements and possible partial internal publication;
public output remains the last complete snapshot. This adds no rollback promise
for unrelated internal Xfer exceptions.

Snapshot copying uses preallocated nonthrowing storage. Failed Reset never
publishes a partial snapshot. If no reset/step has succeeded, sampling in Failed
returns INVALID_STATE.

Independent oracle: Reset result=7. Default invalid-request step holds 7. Drive
request=(3,true), bias=0; sample still 7. Two steps without another drive produce
3 then 6. Reject the entire batch request=(5,true), bias=300; the next step
produces 9, proving old request remained. Drive request=(7,false), bias=0; step
holds 9. Reset restores result=7 and default invalid request; undriven step holds 7.

## Optional ABI and generated typed C++ helpers

Existing lifecycle ABI stays unchanged. Add runtime header `gfsim/model_dut_io.h`:

```c
#ifndef GFSIM_MODEL_DUT_IO_V1_H
#define GFSIM_MODEL_DUT_IO_V1_H
#include <stddef.h>
#include "gfsim/model_api.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AGENTIC_MODEL_DUT_IO_ABI_V1 1u
typedef struct AgenticModelDutIoApiV1 {
  uint32_t struct_size;
  uint32_t abi_version;
  uint64_t input_size;
  uint64_t output_size;
  AgenticModelStatusV1 (*drive_inputs)(
      AgenticModelV1*, const uint8_t*, uint64_t size);
  AgenticModelStatusV1 (*sample_outputs)(
      AgenticModelV1*, uint8_t*, uint64_t size);
} AgenticModelDutIoApiV1;
typedef const AgenticModelDutIoApiV1* (*AgenticModelDutIoQueryV1)(void);
AGENTIC_MODEL_EXPORT const AgenticModelDutIoApiV1*
agentic_model_dut_io_query_v1(void);
#ifdef __cplusplus
}
static_assert(sizeof(AgenticModelDutIoApiV1) == 40);
static_assert(offsetof(AgenticModelDutIoApiV1, input_size) == 8);
static_assert(offsetof(AgenticModelDutIoApiV1, output_size) == 16);
static_assert(offsetof(AgenticModelDutIoApiV1, drive_inputs) == 24);
static_assert(offsetof(AgenticModelDutIoApiV1, sample_outputs) == 32);
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(AgenticModelDutIoApiV1) == 40, "I/O table size");
_Static_assert(offsetof(AgenticModelDutIoApiV1, input_size) == 8, "input size");
_Static_assert(offsetof(AgenticModelDutIoApiV1, output_size) == 16, "output size");
_Static_assert(offsetof(AgenticModelDutIoApiV1, drive_inputs) == 24, "drive");
_Static_assert(offsetof(AgenticModelDutIoApiV1, sample_outputs) == 32, "sample");
#endif
#endif
```

This is a standalone installed C/C++ header. Lifecycle types, the 64-bit host
restriction and export policy come from model_api.h. The query definition uses
matching `extern "C" AGENTIC_MODEL_EXPORT` linkage. Table implementations use C
linkage and the lifecycle platform-default calling convention; they need not be
separately exported. Generated-library compilation defines AGENTIC_MODEL_BUILD
before including either header.

The two typed helpers are out-of-line generated C++ definitions. Declarations
and definitions carry AGENTIC_MODEL_EXPORT, including Windows; they retain C++
linkage. Clients use the matching SDK C++ ABI and generated model/import library.
No header-only local error return, public error-setter or lifecycle-vtable tail
is introduced.

The immutable table lives for library lifetime and accepts only handles from
that bundle's lifecycle API. Missing symbol/null return means unavailable.
Callers verify size/version/function pointers before use; incompatibility is
ABI_MISMATCH, not permission to assume layout.

Byte layout concatenates input or output parameters in declaration order and
record fields recursively without padding. Bool is one byte, 0 or 1. Integer
minimal storage width N occupies ceil(N/8) little-endian bytes: unsigned magnitude
or signed two's complement sign-extended to byte width. The resulting mathematical
value must fit the full declared range. No lengths, pointers, identities, trace,
JSON or model tags occur in payloads. Exact sizes use checked arithmetic;
UINT64_MAX/SIZE_MAX overflow rejects before emission. Any smaller implementation
capacity limit requires an explicit compile-limit diagnostic.

Null data is allowed only for a zero-length batch. Wrong length returns
ABI_MISMATCH before state/destination changes. Invalid bytes or null nonempty
buffer returns INVALID_ARGUMENT. Callers provide accessible nonoverlapping
storage. Equal byte lengths do not establish nominal type compatibility; this
low-level ABI requires the generated schema. Typed helpers provide nominal types
without runtime schema discovery.

`generated/dut.h` remains the include entry. Typed systems define
`PYCIRCUIT_GENERATED_DUT_IO_AVAILABLE 1` and this C++20 surface (illustrative
source-qualified names after C3 legalization):

```cpp
namespace pycircuit::io::demo::top::top {
struct inputs {
  pycircuit::io::demo::packet::request request;
  std::uint64_t bias;
};
struct outputs {
  std::uint64_t result;
};
AGENTIC_MODEL_EXPORT AgenticModelStatusV1 drive_inputs(
    AgenticModelV1* model, const inputs& value) noexcept;
AGENTIC_MODEL_EXPORT AgenticModelStatusV1 sample_outputs(
    AgenticModelV1* model, outputs& value) noexcept;
}
```

Nominal transport structs are emitted once in source-owned headers under
pycircuit::io plus source namespace. Fields preserve declaration order and C3
legalized names. Leaves use bool, int64_t for signed values, uint64_t for unsigned.
Nested records are by-value; no pointers, nullable values, callbacks, dynamic
containers, model handles or user-defined copy/destructor. Full logical domains
are checked before narrowing. Equal-layout nominal records remain distinct C++
structs. These C++ types carry no cross-release layout promise.

Handles come from the bundle's existing agentic_model_query_v1()->create.
Lifetime/non-reentrancy are unchanged. Both APIs share the same handle/state/error
buffer. Typed drive checks original 64-bit host fields before encoding: Word=300
rejects rather than truncating to 44. Typed sample decodes locally and assigns
caller output only after complete success. No second model/runtime is created;
payloads are never interpreted as instructions.

| Operation | Legal state | Success | Rejection |
| --- | --- | --- | --- |
| drive_inputs | Ready | Validate all leaves, atomically replace I | Null handle/domain: INVALID_ARGUMENT; byte size: ABI_MISMATCH; lifecycle: INVALID_STATE; I/Q/time unchanged |
| sample_outputs | Ready/Completed; Failed with prior complete snapshot | Copy last complete snapshot | Null handle: INVALID_ARGUMENT; byte size: ABI_MISMATCH; unavailable snapshot/lifecycle: INVALID_STATE; destination unchanged |
| step | Ready | Existing C3 rules plus input sampling | Existing C3 API/execution errors |
| reset | Configured/Ready/Completed/Failed | C3 reset plus defaults/snapshot | Existing reset failure behavior |

### I/O entry, rejection and unexpected failure

Byte functions and typed helpers use one internal serialized API-entry guard and
diagnostic machinery. For a valid handle, entry invalidates prior borrowed buffers
before lifecycle/argument validation. Null handles return INVALID_ARGUMENT without
a per-model diagnostic.

Helpers run in the generated bundle and enter the guard before checking original
host fields. They cannot narrow 300 to an 8-bit byte merely to trigger raw API
rejection. Accepted typed/byte forms converge on the same internal publication/
snapshot-copy implementation without recursive public API entry.

Ordinary domain/size/null-buffer/capability/layout/lifecycle rejection preserves
lifecycle, I/Q/snapshot/time/statistics and caller destination. Record the
appropriate existing diagnostic unless Failed already holds its first execution
failure, which is preserved.

| Unexpected I/O exception | Resulting state |
| --- | --- |
| drive_inputs from Ready | Failed |
| sample_outputs from Ready or Completed | Failed |
| sample_outputs from Failed | Remains Failed; preserves first failure |

Return RUNTIME_FAILURE with code=runtime_failure, phase=api, available root source
information and null check_id. Preserve I/Q/snapshot/time/statistics/destination.
Subsequent step/drive returns INVALID_STATE; prior complete snapshot remains
sampleable. Successful whole-tree Reset restores Ready/defaults/zero time.

All possibly failing work precedes publication/destination copy. The suffix uses
preallocated trivial storage and is nonthrowing. Fault injection targets preparation,
validation or encoding before that suffix, never inventing a legal mid-copy failure.
Invalid caller memory and asynchronous process failures remain outside C buffer
guarantees. Ordinary copies/validation allocate nothing.

Successful drive/sample do not clear diagnostics. C3 configure/reset/step clearing
and reset-failure behavior remain unchanged. Diagnostic construction failure uses
C3's preallocated fallback. Revision B makes unexpected I/O failure fail-stop;
ordinary argument rejection leaves a healthy model usable and no internal-Xfer
rollback promise changes.

Fault oracles require old-input retention, unchanged destination/snapshot/time,
subsequent step rejection, permitted Failed sampling, first-failure retention,
and successful Reset/rerun.

The 64-byte lifecycle vtable, 24-byte step result, 16-byte buffer, ABI version,
query symbol and status values are unchanged. Query the I/O extension separately;
statistics/trace buffers are not input/output transport.

Portless modules define availability macro 0, no typed drive/sample surface, and
return null from optional query; a prior bundle may omit the symbol. Typed consumers
reject unavailable capability and never access private fields. Add capability
`pycircuit-typed-dut-io` consistently to SDK/platform/release-index/consumer-lock
sets only after validation. Generator ABI 2/runtime ABI 1 remain C3 values; this
proposal approves a capability extension and separate 40-byte table, not a
lifecycle-vtable tail.

## RTL mapping and parity

Emit an input/output port per scalar leaf in parameter/field order using C3
name/collision rules. LogicalType determines minimal packed width/signedness;
bool is one bit. Source maps retain exact parameter/field paths. Outputs observe
boundary DFFE current; inputs are external current wires.

Testbench holds a complete batch before active edge, evaluates combinational
logic and observes after edge/NBA settle. This is the same I[t],Q[t]→Q[t+1]
relation as C++. Inputs cannot change during a step. RTL reset resets DUT state;
testbench separately drives declared input defaults to reproduce the C++ reset
environment. No extra sampling edge is added.

Invalid bits, X/Z, reset precedence, global inhibition and next-edge recovery
follow the intrinsic ingress contract above. C++ drive rejects invalid host
values before installation. Legal-stimulus parity and rejected-edge observations
are separate tests; this is not general four-state source-value support. Real
four-state simulation is required for X/Z evidence.

## Independent acceptance and hard break

Independent tests derive expectations from this proposal, not either backend.
Cover the example sequence, atomic batch validation, signed/narrow ranges,
nested nominal types, input persistence, R+W feedback, reset/rerun, observation
before/after step, source/check failure, injected internal Xfer failure retaining
last complete snapshot, lifecycle, absent/null/incompatible query, byte sizes,
malformed bool/signed/narrow encodings, nominal helper rejection, source/default/
header/binding tampering, no child forwarding cycle and input-domain failures.

One final program and independently authored oracle/stimulus must execute on
C++ and Verilog. The three Python units require independent producers and C++
groups. Planned gate: `python3 -m pytest tests/system/test_migration_dut_io.py`.
This test is not claimed to exist; it must execute real compile/link/emit and
both backends, with fresh evidence.

No old Agentic entry, generated sink array, traversal-order scheduling, trace
transport or private-state accessor returns. Exact approval scope is system
source defaults, C2 boundary declaration, optional table/byte layout, generated
header APIs, and SDK capability. This draft does not alter frozen contracts or
authorize implementation.

## External callers and acceptance lanes

Before this extension the foundation portless model supports lifecycle, steps,
statistics and errors only. It exposes no host stimulus/payload observation.
Afterward this complete Runtime-only client uses only generated/dut.h; its
independent expected byte sizes are input=3 and output=1:

```cpp
#include "generated/dut.h"
#if !PYCIRCUIT_GENERATED_DUT_IO_AVAILABLE
#error "This client requires a typed system"
#endif
namespace dut = pycircuit::io::demo::top::top;
struct ModelOwner {
  const AgenticModelApiV1* api;
  AgenticModelV1* model = nullptr;
  ~ModelOwner() { api->destroy(model); }
};
int main() {
  const auto* api = agentic_model_query_v1();
  if (!api || api->struct_size != sizeof(AgenticModelApiV1) ||
      api->abi_version != AGENTIC_MODEL_ABI_V1 ||
      !api->create || !api->destroy || !api->configure_json ||
      !api->reset || !api->step || !api->statistics_json || !api->last_error)
    return 1;
  const auto* io = agentic_model_dut_io_query_v1();
  if (!io || io->struct_size != sizeof(AgenticModelDutIoApiV1) ||
      io->abi_version != AGENTIC_MODEL_DUT_IO_ABI_V1 ||
      !io->drive_inputs || !io->sample_outputs ||
      io->input_size != 3 || io->output_size != 1)
    return 2;
  ModelOwner owner{api};
  if (api->create(&owner.model) != AGENTIC_MODEL_STATUS_V1_OK)
    return 3;
  const uint8_t config[] = {'{', '}'};
  if (api->configure_json(owner.model, config, sizeof(config)) !=
          AGENTIC_MODEL_STATUS_V1_OK ||
      api->reset(owner.model) != AGENTIC_MODEL_STATUS_V1_OK)
    return 4;
  dut::outputs observed{};
  if (dut::sample_outputs(owner.model, observed) !=
          AGENTIC_MODEL_STATUS_V1_OK || observed.result != 7)
    return 5;
  dut::inputs input{{3, true}, 0};
  if (dut::drive_inputs(owner.model, input) != AGENTIC_MODEL_STATUS_V1_OK)
    return 6;
  AgenticModelStepResultV1 step{};
  step.struct_size = sizeof(step);
  if (api->step(owner.model, &step) != AGENTIC_MODEL_STATUS_V1_OK ||
      step.state != AGENTIC_MODEL_STEP_V1_RUNNING || step.epoch_time != 1)
    return 7;
  if (dut::sample_outputs(owner.model, observed) !=
          AGENTIC_MODEL_STATUS_V1_OK || observed.result != 3)
    return 8;
  return 0;
}
```

generated/dut.h includes both runtime headers and source-owned transport types,
without compiler/MLIR headers. A separate generic loader uses dlsym/GetProcAddress
to test absent optional symbols; the directly linked client does not claim that.

### Affected surfaces and deletion boundary

| Surface | Required work |
| --- | --- |
| simulator/gfsim/include/gfsim/model_api.h | Preserve existing layout, values and query. |
| New simulator/gfsim/include/gfsim/model_dut_io.h | Install exact standalone extension header/linkage/layout. |
| Generated root/interface glue and generated/dut.h | Optional query, two exported out-of-line helpers, shared entry machinery and source-owned transport declarations. |
| cmake/pycircuitConfig.cmake.in and installation inventory | Include new header in Runtime-only component; no second runtime. |
| schemas/agentic-circuit/sdk-manifest.schema.json, release-index.schema.json, consumer-lock.schema.json | Add exact capability to C3 sets while preserving its ABI/schema versions. |
| packaging/sdk/create_platform_manifest.py, release_candidate.py, examples and validators | Produce and cross-check identical capability/ABI tuples. |
| packaging/sdk/verify_platform_candidate.py | Replace exactly-one-export assumption with the precise new inventory. |
| C/C++ API smokes and new I/O cases | Preserve lifecycle checks, add C compilation, real symbols/linkage and typed client execution. |
| SSM generated DUT runner/tests | Consumer-owned replacement of sink/private-object access with source ports; no SSM implementation here. |

Typed models export exactly lifecycle query, I/O query and their two source-qualified
C++ helper symbols. Newly emitted portless models export both queries, with null
I/O query and no helpers. Earlier bundles may lack optional query and remain
unsupported for typed I/O. Export tests derive expected C++ decorations using the
matching compiler/header; no hardcoded Itanium spelling on Windows or unrestricted
extra exports. Internal table functions/model classes remain unexported.

No additional prior-route deletion is required beyond frozen M5 retirement.
Update affected validators rather than relax export checks. Reject sink_N_values,
QueueSink histories, private members, trace/statistics as payload transport,
manual activation schedules and old compiler fallback as implementations of this
boundary. Consumer-specific call sites migrate in their own repository.

Capability handshake first requires SDK/platform/release-index/consumer-lock
agreement on required capabilities and generator ABI2/runtime ABI1. Missing or
inconsistent required capability fails before typed consumer build/run. Then the
selected model requires availability=1 and a compatible non-null table with
expected source-derived sizes. Toolchain support alone does not give portless
models typed I/O.

### Planned execution lanes

These tests/paths are planned and do not assert present execution evidence.

| Lane | Required evidence |
| --- | --- |
| IO-SOURCE | Three producers, header-only parent with child source/body inaccessible, defaults/directions, child-system/invalid-default rejection. |
| IO-IR | Both emit entries reject removed/replaced intrinsic, redirected bindings/domains, duplicate/bypass root; RTL-private verifier rejects missing/redirected admission control. |
| IO-ABI | C11/C++20 layout compilation, real unmangled query lookup, Windows helper import/export linking, absent/null/bad-size/bad-version/null-function cases. |
| IO-RUNTIME | Independent value sequence, input persistence, atomic invalid batch, complete snapshots, lifecycle and injected I/O failures. |
| IO-RTL4 | Real Icarus invalid range, X/Z input/reset, reset precedence, global inhibition and next-edge recovery. |
| IO-INSTALLED | Relocated Runtime-only build/run, exact exports, capability mismatch, independent source/TU ownership. |

The planned umbrella tests/system/test_migration_dut_io.py must run every lane
with nonempty assertions, no skipped required cases, expanded commands, exit codes
and observations. PYC_DRIVER is the current candidate's installed driver;
PYC_FIXTURE is planned tests/integration/pycircuit/fixtures/migration_dut_io;
PYC_RUN is a fresh ignored output directory. Never copy another checkout's tools.

```sh
"$PYC_DRIVER" compile -c "$PYC_FIXTURE/packet.py" \
  --source-root "$PYC_FIXTURE" --package-prefix demo -o "$PYC_RUN/packet"
"$PYC_DRIVER" compile -c "$PYC_FIXTURE/accumulator.py" \
  --source-root "$PYC_FIXTURE" --package-prefix demo \
  -I "$PYC_RUN/packet" -o "$PYC_RUN/accumulator"
"$PYC_DRIVER" compile -c "$PYC_FIXTURE/top.py" \
  --source-root "$PYC_FIXTURE" --package-prefix demo \
  -I "$PYC_RUN/packet" -I "$PYC_RUN/accumulator" -o "$PYC_RUN/top"
"$PYC_DRIVER" link "$PYC_RUN/packet" "$PYC_RUN/accumulator" \
  "$PYC_RUN/top" --top demo.top.Top -o "$PYC_RUN/program.ac"
"$PYC_DRIVER" emit "$PYC_RUN/program.ac" --target cpp -o "$PYC_RUN/cpp"
"$PYC_DRIVER" emit "$PYC_RUN/program.ac" --target verilog -o "$PYC_RUN/rtl"
```

The planned host harness compiles all receipt-listed generated TUs independently
into dut_model, uses only find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime),
and links client with dut_model and pycircuit::pyc6_runtime:

```sh
cmake -S "$PYC_FIXTURE/host" -B "$PYC_RUN/host-build" \
  -DCMAKE_PREFIX_PATH="$PYC_RUN/relocated-sdk" -DDUT_GENERATED_DIR="$PYC_RUN/cpp"
cmake --build "$PYC_RUN/host-build" --parallel 8
ctest --test-dir "$PYC_RUN/host-build" --no-tests=error \
  --output-on-failure -R '^dut_io_'
```

Create rtl_files.f exclusively from the verified generated receipt:

```sh
iverilog -g2012 -s tb_dut_io_four_state -o "$PYC_RUN/io-four-state.vvp" \
  -f "$PYC_RUN/rtl_files.f" "$PYC_FIXTURE/rtl/io_four_state_tb.sv"
vvp "$PYC_RUN/io-four-state.vvp"
```

For rejected edges verify exact diagnostic count/path, unchanged observations,
recovery and a nonempty oracle-completion marker. Explicitly account for expected
$error, process exit and log; launching a simulator or ignoring errors is not
validation. Verilator two-state execution cannot replace IO-RTL4. C3 publication,
crash recovery, source-unit and installed-package obligations continue by reference.

## Consumer boundary and review condition

SSM owns ELF, immutable decode facts, request/response schemas, PC/block control,
register/readiness state, ALU/BRU, completion and retirement. Host may answer
model-issued fetch/decode requests, transport external responses, drive control,
and report observations. It must not compute next PC, operands, execution,
branch results or retirement, or use a software register dictionary as core state.

Typed I/O alone does not close SSM or resources. Observed SSM
fd0fcf9f21cc5e030af732b4952deed794554dbd has an empty SPE H1 and disconnected OOO/IEX.
Its eight ELF, RAW/WAW, PC/retirement/register, 1–4 in-flight with real overlap,
and init/run/drain oracles remain required in the consumer repository.

Independent review must resolve system defaults, external-current mapping,
C++ surface and ingress rejection as one packet. No implementation authority
exists until an approval-ready review and user approval of the precise revision.
