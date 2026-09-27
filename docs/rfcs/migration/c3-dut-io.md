# C3-DUT-IO: typed external boundary for a single-clock system

Revision: A. Status: draft for independent Astra review; not approved or implemented.
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

Add one regionless, operandless, resultless structural `ac.dut` operation to the
linked builtin module. It declares a boundary, not a scheduler or arbitrary
internal observations:

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

```cpp
typedef struct AgenticModelDutIoApiV1 {
  uint32_t struct_size;  /* exactly 40 on the C3 64-bit host profile */
  uint32_t abi_version;  /* exactly 1 */
  uint64_t input_size;
  uint64_t output_size;
  AgenticModelStatusV1 (*drive_inputs)(
      AgenticModelV1*, const uint8_t*, uint64_t size);
  AgenticModelStatusV1 (*sample_outputs)(
      AgenticModelV1*, uint8_t*, uint64_t size);
} AgenticModelDutIoApiV1;

AGENTIC_MODEL_EXPORT const AgenticModelDutIoApiV1*
agentic_model_dut_io_query_v1(void);
```

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
AgenticModelStatusV1 drive_inputs(
    AgenticModelV1* model, const inputs& value) noexcept;
AgenticModelStatusV1 sample_outputs(
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

Both functions serialize with all model calls. Ordinary validation/copies allocate
nothing and cannot throw. Unexpected internal failures are caught and return
RUNTIME_FAILURE without publishing a batch or modifying caller output. Diagnostics
use existing code=invalid_argument|abi_mismatch|invalid_state|runtime_failure,
phase=api, root instance/source when available, null check_id. Preserve a Failed
model's first execution diagnostic. Success does not clear diagnostics; existing
C3 configure/reset/step clearing rules remain. Calls invalidate prior borrowed
diagnostic/statistics buffers.

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

Out-of-domain bits and X/Z at this two-state boundary are invalid stimulus:
simulation diagnoses them and inhibits commit. C++ drive rejects invalid host
values before installation. Legal stimulus timing/value parity and invalid
stimulus no-commit behavior are separately tested; this is not general four-state
support. Shared verification/lowering must represent ingress checks/global commit
inhibition; they cannot exist solely in a C++ adapter.

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
