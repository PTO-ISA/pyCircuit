# AC C++ pointer-owned module composition

**Status:** Accepted architecture contract; implementation tracked by Decisions 0274 and 0275

**Related decisions:** 0264, 0267, 0269, 0270, 0274, 0275

**Implementation checklist:**
[AC rule and SimQueue atomic lowering checklist](ac-rule-simqueue-atomic-lowering-checklist.md)

## Outcome

Structured Agentic Circuit C++ generation uses the same ownership tree as the
verified AC package:

- one resolved `ac.module.import` declaration owns one generated `.hpp`;
- one implementation source-unit `.ac` owns one generated `.cpp`;
- a parent module owns every Queue declared in its body as concrete storage;
- a parent uniquely owns each child module through `std::unique_ptr`;
- child ports receive only non-owning typed `gfsim::SimQueue<T> *` pointers;
- every nominal `ac.struct` and module interface emits a readable C++ class;
- a Queue payload whose concrete packed width exceeds 64 bits uses
  `std::shared_ptr<const PayloadClass>` as its C++ storage type;
- an instance record stores the stable child pointer after allocation;
- equal concrete specializations reuse one C++ class, while every instance has
  a distinct allocation, state, object ID, runtime path, and instance record.

This is a generated-source contract, not a stable cross-release C++ ABI.

## Motivation

The current generator already emits one header and one source for every
concrete module specialization, but a composite class embeds internal children
by value and wires ports with C++ references. For example, a parent resembles:

```cpp
class Pipeline final : public gfsim::Module {
  gfsim::SimQueue<DecodedItem> queue_0_;
  DecodeStage child_0_;
  SelectStage child_1_;
};
```

That representation executes, but it hides three architectural facts:

1. the AC module declaration is the public C++ compilation boundary;
2. the parent owns the connecting Queue storage;
3. a child instance is an allocated runtime object with a stable address and a
   separate identity from the reusable class definition.

The new representation makes these facts explicit while keeping identity
structural, ownership local, and the framework consumer-neutral.

## Scope

This design covers:

- module `.hpp` and `.cpp` ownership;
- parent-owned Queue storage;
- child allocation and lifetime;
- typed pointer ports;
- instance records and scheduler registration;
- repeated instances and specialization reuse;
- file and class naming;
- CMake compilation and linkage.

It does not change:

- Agentic Circuit Queue, rule, Table, Slot, or cycle semantics;
- the H1/H2/H3 hierarchy of any consumer design;
- the public model lifecycle ABI in `generated/model.h`;
- source-level module signatures;
- external Git/release pinning;
- specialization equality.

## Artifact ownership

The package linker resolves a declaration, its implementation owner, and the
closed set of concrete specializations before C++ emission.

| AC artifact | Generated artifact | Content authority |
| --- | --- | --- |
| source-owned `ac.module.import` | `<source>.hpp` | public class and port declarations |
| implementation `<source>.ac` | `<source>.cpp` | method bodies, rules, state, child allocation |
| source-owned nominal `ac.struct` interface shard | `<interface-source>.hpp` | payload class and enum declarations; never a monolithic type unit |
| `ac.module.import` interface | `<source>.hpp` | module class and typed `Ports` class |
| `core.ac` | `core.hpp` and `core.cpp` | selected root and external Queue ownership |
| linked package | root glue and CMake source list | validated definition/instance closure |

The source-owner key comes from the normalized implementation Python path, not
from the literal interface filename `module.ac`. For example:

```text
interface/pipeline/decode.ac -> include/generated/pipeline/decode.hpp
pipeline/decode.ac           -> src/generated/pipeline/decode.cpp
```

The header and source therefore share the readable implementation stem `decode`.
The class name still comes from the AC module definition, such as
`DecodeStage`.

One implementation source with several typed specializations still owns one
`.hpp`/`.cpp` pair. The pair declares and defines every concrete specialization
from that source unit.

## Generated module header

The module declaration determines the public class surface. The linked
implementation contributes the private member layout after declaration and
definition compatibility has been verified.

```cpp
#pragma once

#include "generated/interfaces/pipeline_items.hpp"
#include "gfsim/object.h"
#include "gfsim/queue.h"

#include <memory>

namespace ac_generated::pipeline {

using InputItemRef = std::shared_ptr<const InputItem>;
using LookupResultRef = std::shared_ptr<const LookupResult>;
using DecodedItemRef = std::shared_ptr<const DecodedItem>;

struct DecodeStagePorts final {
  gfsim::SimQueue<InputItemRef> *item_in;
  gfsim::SimQueue<LookupResultRef> *lookup_in;
  gfsim::SimQueue<gfsim::UInt<32>> *expected_table_identity_in;
  gfsim::SimQueue<DecodedItemRef> *decoded_out;
};

class DecodeStage final : public gfsim::Module {
public:
  DecodeStage(gfsim::InstanceConstructionContext context,
              DecodeStagePorts ports);
  ~DecodeStage() override;

  DecodeStage(const DecodeStage &) = delete;
  DecodeStage &operator=(const DecodeStage &) = delete;
  DecodeStage(DecodeStage &&) = delete;
  DecodeStage &operator=(DecodeStage &&) = delete;

  gfsim::DispatchRow dispatchRow(std::size_t index);

private:
  DecodeStagePorts ports_;
  gfsim::Module scope_;
  gfsim::QueueAtomicTransform<...> decode_;
};

} // namespace ac_generated::pipeline
```

Rules:

- Every runtime port is a typed Queue pointer.
- A required port must be non-null. Construction fails before scheduler
  registration when a required pointer is null.
- The generated module stores port pointers only; it never owns, deletes, or
  reallocates the pointed-to Queue.
- The class is non-copyable and non-movable so Queue and module addresses stay
  stable for the complete runtime lifetime.
- The implementation `.cpp` defines the constructor, destructor, dispatch
  rows, rules, and reset behavior.

The generator does not use PIMPL. Generated C++ is rebuilt with its exact
compiler revision, and a full private layout keeps hierarchy and ownership
visible to reviewers while avoiding an extra allocation for every module
implementation object.

## Parent ownership model

Ownership follows lexical AC structure.

```text
Pipeline instance
├── decode_to_select_ : SimQueue<DecodedItemRef> [owned Queue value]
├── select_to_commit_ : SimQueue<SelectedItemRef> [owned Queue value]
├── decode_           : unique_ptr<DecodeStage> [owned child]
├── select_           : unique_ptr<SelectStage<2>> [owned child]
├── commit_           : unique_ptr<CommitStage> [owned child]
├── decode_ref_       : InstanceRecord -> decode_.get() [non-owning]
├── select_ref_       : InstanceRecord -> select_.get() [non-owning]
└── commit_ref_       : InstanceRecord -> commit_.get() [non-owning]
```

### Queue storage

The module that declares an inter-child Queue owns its storage as a direct
member value:

```cpp
gfsim::SimQueue<DecodedItemRef> decode_to_select_;
```

The parent passes `&decode_to_select_` to the producer output and consumer input. The
producer and consumer store the same Queue pointer; neither allocates another
Queue. A wide payload allocation referenced by that Queue is a separate object
with the lifetime rules in Section 10.

The ownership rules are:

- a Queue between siblings belongs to their least common structural parent;
- a Queue private to one child belongs to that child;
- a selected-system input/output Queue belongs to the generated root wrapper;
- a compiler-generated fanout Queue and broadcast object belong to the parent
  scope in which fanout was introduced;
- a Queue is never owned by an instance record or by a port object.

### Persistent state

“The parent owns variables” means lexical ownership, not forced hoisting:

- an `ac.var`, Table, Slot, or Queue declared in the parent belongs to the
  parent class;
- state declared inside a child remains child state;
- a parent cannot access or duplicate a child's private state;
- a child sees parent-owned communication storage only through Queue pointers.

## Child allocation

Structural children use `std::unique_ptr`. Generated hierarchy does not use
`std::shared_ptr` for module ownership. `shared_ptr` is reserved for wide,
immutable Queue payloads as described in Section 10.

```cpp
template <unsigned LaneCount>
class Pipeline final : public gfsim::Module {
private:
  gfsim::SimQueue<DecodedItemRef> decode_to_select_;
  gfsim::SimQueue<SelectedItemRef> select_to_commit_;

  std::unique_ptr<DecodeStage> decode_;
  std::unique_ptr<SelectStage<LaneCount>> select_;
  std::unique_ptr<CommitStage> commit_;

  gfsim::InstanceRecord decode_ref_;
  gfsim::InstanceRecord select_ref_;
  gfsim::InstanceRecord commit_ref_;
};
```

The constructor creates Queue storage before allocating children:

```cpp
template <unsigned LaneCount>
Pipeline<LaneCount>::Pipeline(
    gfsim::InstanceConstructionContext context,
    PipelinePorts ports)
    : gfsim::Module(context.localName(), context.objectId(), context.parent()),
      decode_to_select_("decode_to_select", context.takeObjectId(), this, 1),
      select_to_commit_("select_to_commit", context.takeObjectId(), this, 1) {
  decode_ = std::make_unique<DecodeStage>(
      context.child("decode"),
      DecodeStagePorts{
          .item_in = ports.item_in,
          .lookup_in = ports.lookup_in,
          .expected_table_identity_in = ports.expected_table_identity_in,
          .decoded_out = &decode_to_select_,
      });
  decode_ref_ = registerInstance("decode", decode_.get());
  attachChild(*decode_);

  // Select and commit follow the same pattern.
}
```

Member declaration order places Queue values before child pointers. C++ then
constructs Queues before children and destroys children before Queues.

`std::shared_ptr` is rejected for structural module ownership because it hides the
single owning parent, permits cycles, and makes destruction order dependent on
unrelated references. A service used by several modules is elevated to their
least common parent, uniquely owned there, and connected through Queue
pointers. Wide payload sharing does not change this rule because a payload is
data, not a structural module instance.

## Instance record

An instance record is a non-owning runtime description of one allocation:

```cpp
struct InstanceRecord final {
  gfsim::Module *module = nullptr;
  gfsim::ObjectId objectId = gfsim::kInvalidObjectId;
  std::string_view localName;
  std::string_view moduleSymbol;
  std::string_view sourceUnit;
};
```

Properties:

- `module` equals `child_unique_ptr.get()` after successful allocation;
- the record never deletes the module;
- `objectId`, `localName`, and hierarchy path describe the instance, not the
  reusable class;
- repeated instances of one specialization have different records and object
  IDs but the same `moduleSymbol` and C++ class;
- records are populated only after a child constructor succeeds;
- the parent is not attached to the runtime tree until all required children,
  Queues, records, and port checks succeed.

Scheduler configuration, reset traversal, statistics, source mapping, and
debug hierarchy consume instance records or their stable module pointers. They
do not infer children from C++ member offsets.

## Port contract

Each module header contains one generated `Ports` aggregate. Field order
follows the verified AC declaration order.

```cpp
struct CommitStagePorts final {
  gfsim::SimQueue<SelectedItemRef> *item_in;
  gfsim::SimQueue<ResourceSnapshotRef> *resources_in;
  gfsim::SimQueue<CommitResultRef> *result_out;
};
```

The generator validates:

- every required pointer is non-null;
- payload types match exactly;
- one input endpoint has one consuming child unless an admitted fanout block
  owns the split;
- one output endpoint has one producing child;
- direction metadata matches producer/consumer roles;
- a child cannot receive a pointer to a sibling's private state;
- all pointers target objects whose lifetime dominates the child lifetime.

Raw pointers are intentional here: they express non-owning structural links.
References are not used because instance construction and records need an
explicit nullable-before-validation representation. Smart pointers are not
used for ports because ports do not own Queue storage.

## Nominal payload classes and wide Queue storage

Each source-owned interface shard emits one `.hpp` containing the readable
`final class` declarations for every nominal `ac.struct` that source owns. No
per-type header or `generated/types/` tree becomes a second authority. Each
class preserves nominal type identity and exposes the operations required by
generated rules, tracing, equality, packing, and debug output.

```cpp
class DecodedItem final {
public:
  DecodedItem(...);

  const ItemIdentity &identity() const;
  const OperationRecipe &recipe() const;
  bool valid() const;

  bool operator==(const DecodedItem &) const = default;

private:
  ItemIdentity identity_;
  OperationRecipe recipe_;
  bool valid_ = false;
};
```

Module interface declarations also emit C++ classes:

- `<Module>Ports` describes typed Queue pointer ports;
- `<Module>` derives from `gfsim::Module` and represents one runtime instance;
- payload interfaces referenced by ports use their nominal generated classes.

The Queue element storage policy is selected after all static parameters have
been concretized:

```cpp
template <typename T>
using QueueStorage = std::conditional_t<
    (PackedBitWidth<T> > 64), std::shared_ptr<const T>, T>;
```

The generator normally prints the resolved type directly rather than exposing
this alias in user-facing generated code.

### Allocation and push

A new payload wider than 64 bits is allocated only after the Queue has reserved
capacity for the push. A blocked push performs no allocation.

```cpp
if (output->preparePush(group)) {
  auto payload = std::make_shared<const DecodedItem>(...);
  output->publishPush(group, std::move(payload));
}
```

Forwarding an unchanged wide payload reuses the existing shared pointer. It
does not allocate or deep-copy another payload object. A rule that changes a
field creates a new immutable payload object, matching AC SSA semantics.

### Pop and release

A prepared pop retains the Queue's reference until the Xfer commit. On a
successful pop commit, the Queue removes its entry and releases its reference.
The consumer receives or already holds another `shared_ptr<const T>`.

Physical deallocation occurs when the final reference is released. It may
happen at the pop commit when no consumer retains the value, or later after the
last consumer, fanout branch, retained Slot, or downstream Queue releases it.
This is the safe meaning of “pop frees the payload” under shared ownership.

Cancelled pushes, cancelled pops, recovery pruning, reset, and Queue
destruction release every reference they own. A proposal that does not commit
cannot leak a reference into committed state.

### Fanout and immutability

Compiler-generated fanout copies only `shared_ptr`, so every branch observes
the same immutable payload object. No branch may mutate the object after
publication. If a branch needs a changed value, it constructs a new payload.

Nested fields remain ordinary class members. The storage policy applies to the
top-level Queue element; it does not recursively wrap every nested struct in a
separate pointer.

Payloads with a concrete packed width of 64 bits or less remain value Queue
elements. Nominal structs still generate C++ classes at those widths; only the
Queue storage representation changes.

## Specialization and reuse

Decision 0275 is the normative static-family schema. Family emission is blocked
until the frontend, ACIR/link verifier, and QueueGraph represent its ordered
`StaticParameterDecl` records, closed parameter types, required/default state,
`one_of`/`integer_range` constraints, source-declared `finite_cases`, and closed
dependent expressions as typed records. Observed callers, concrete symbol
spellings, dictionaries, and identifier suffixes cannot supply missing family
information.

Specialization equality remains:

```text
(MLIR definition symbol, ordered typed static arguments)
```

Only this structural tuple participates in specialization equality.

For a source `select.py` with `lane_count=2` and `lane_count=4`, the output
remains one readable source pair:

```text
include/generated/pipeline/select.hpp
src/generated/pipeline/select.cpp
```

The header declares one readable template. Static parameters remain explicit
template arguments rather than becoming identifier fragments:

```cpp
template <unsigned LaneCount>
class SelectStage;
```

The source explicitly instantiates the admitted parameter values. Ten instances
of `SelectStage<2>` allocate ten independent objects of the same C++ type. The
readable class identifier stays `SelectStage`; parameter data is carried
separately in template arguments, construction metadata, and source maps.
Generated convenience aliases that encode specialization values in identifiers
are forbidden.

The RTL projection follows the same grouping: one readable parameterized RTL
module family per source definition, with typed static parameters and admitted
generate branches. Unsupported parameter/family shapes reject before RTL
emission; they do not produce per-specialization module names or suffixes.

For F4, every admitted case is declared by the source independently of callers.
The C++ template family explicitly instantiates those cases, RTL covers exactly
those cases through typed parameters/generate branches, and Queue storage is
selected after per-case dependent-type concretization. Open-domain families,
richer cross-parameter constraints, and family emission from the current
concrete-symbol baseline remain rejected until a later decision or the Decision
0275 implementation is verified.

## Names and traceability

Names follow `docs/reference/name-mangling.md`:

- namespace path: normalized Python source ownership path;
- header/source stem: Python implementation source stem;
- class: readable AC definition name without enclosing-module or
  specialization suffixes;
- static parameters: explicit C++ template arguments or a typed `Params`
  object, never flattened into the identifier;
- instance local name: Python assignment/callsite name when present, otherwise
  a short lower-snake role plus deterministic single-underscore ordinal;
- runtime path: parent path plus local instance name;
- Queue/interface member: short lower-snake source-facing role. The compiler
  does not prefix it with the enclosing module name.

Generated identifiers never contain `__`. C++/Verilog local identifiers use a
single underscore as the only separator. The `*` character in the pattern
below denotes optional qualifier tokens; it is not emitted into an identifier.

Cross-module interface signals use:

```text
<name>_<from_module>_<to_module>_<stage_name>*_<valid|ready|purpose>
```

The explicit `from_module` and `to_module` fields are the only module names in
the signal. Internal module signals omit endpoint fields and remain short
lower-snake names. Generated Verilog preserves the same readable leaf names so
Verilator waveforms can be inspected without a reverse-name database.

Generated `.hpp` and `.cpp` files contain adjacent comments for:

- implementation Python path and line;
- declaration Python path and line;
- implementation `.ac` unit;
- declaration `.ac` unit;
- AC definition symbol and typed specialization arguments;
- NDF direct and required IDs.

Comments provide traceability only. They are not identities.

## Build flow

The frontend flow remains independently compiled:

```text
module.py --acc.py--> module.ac + module declaration.ac
parent.py --acc.py--> parent.ac + parent declaration.ac
core.py   --acc.py--> core.ac
```

Native ACC then:

1. links the package and resolves every declaration to exactly one definition;
2. verifies signatures, specialization arguments, hierarchy, Queue types, and
   source ownership;
3. emits one `.hpp` for every resolved module declaration;
4. emits one `.cpp` for every implementation `.ac` source unit;
5. emits root/runtime glue;
6. emits a CMake source list containing each `.cpp` exactly once.

Code generation may run as one transactional `acc -emit-cpp-bundle` command in
the first implementation. C++ compilation is per source and parallel:

```cmake
add_library(ac_decode OBJECT src/generated/pipeline/decode.cpp)
add_library(ac_select OBJECT src/generated/pipeline/select.cpp)
add_library(ac_pipeline OBJECT src/generated/pipeline/pipeline.cpp)

target_link_libraries(gfsim-pyc PRIVATE
  $<TARGET_OBJECTS:ac_decode>
  $<TARGET_OBJECTS:ac_select>
  $<TARGET_OBJECTS:ac_pipeline>)
```

A later measured optimization may add per-unit backend emission, but it cannot
introduce persistent link metadata or weaken package verification.

## Construction and destruction invariants

1. Parent Queue values are constructed first.
2. Required external Queue pointers are validated.
3. Child object IDs are allocated deterministically from the linked plan.
4. Children are allocated with `std::make_unique`.
5. Instance records capture `unique_ptr.get()` after successful construction.
6. Children are attached to the runtime hierarchy.
7. The completed parent becomes visible to scheduler configuration.
8. On destruction, child pointers are destroyed before parent Queue values.

Construction failure publishes no partial instance record or scheduler entry.
Normal runtime never reallocates a parent module, child module, or owned Queue.

## Verification

### Generated-source shape

- every resolved module declaration has exactly one `.hpp`;
- every nominal payload class appears exactly once in its source-owned
  interface-shard `.hpp`, with no per-type header authority;
- every implementation `.ac` has exactly one `.cpp`;
- header and source stems match the Python implementation source stem;
- parent headers contain Queue values and `unique_ptr` child members;
- child constructors accept Queue pointers, not Queue values or owning smart
  pointers;
- generated structural child ownership contains no `shared_ptr`;
- every wide Queue payload uses `shared_ptr<const PayloadClass>`;
- class and file names contain only readable source spellings;
- specialization data appears only as explicit parameters;
- internal and waveform-visible signal names are lower-snake, bounded in
  length, and contain no double underscore.

### Ownership and lifetime

- producer and consumer observe the exact same Queue address;
- forwarding and fanout of a wide payload preserve the payload address;
- a blocked push performs no wide-payload allocation;
- pop commit releases the Queue reference and final-reference release frees the
  payload;
- recovery, cancellation, and reset leave no retained payload references;
- repeated child instances have different addresses and object IDs;
- equal specializations have the same C++ type;
- child destruction precedes parent Queue destruction under AddressSanitizer;
- null required ports fail before hierarchy publication;
- failed child construction leaves no instance record or registered object.

### Behavioral parity

- by-value baseline and pointer-owned implementation produce identical
  TICK-OBS and XFER-OBS traces;
- reset and invalidate ordering remain unchanged;
- activation scheduler rows reference the recorded child pointers and stable
  object IDs;
- nested H1→H2→H3, repeated instances, fanout, multi-input, multi-output,
  stateful modules, and backpressure execute through the normal scheduler.

### Build graph

- every `.cpp` compiles as an independent object;
- touching one implementation `.ac` regenerates its `.cpp` and affected parent
  headers/sources without renaming unrelated files;
- CMake and emitted source inventories agree exactly;
- the final DUT links and runs without a whole-core source fallback.

## One-stage hard-break migration

Implementation may be developed on non-published branches, but the accepted
repository state changes in one coherent stage. That stage must:

1. land nominal payload/module-interface classes and source-owned interface
   shards, concrete `>64 bit` Queue storage, `Ports`, and `InstanceRecord`;
2. switch every generated structural child to `unique_ptr`, every parent-owned
   Queue to a value, and every child Queue port to a typed raw pointer;
3. switch generated headers to `.hpp`, source stems to the implementation
   Python/AC stem, static values to explicit typed parameters, and C++/RTL name
   families to the readable no-suffix contract;
4. update all bundle/CMake consumers and add source-shape, lifetime, parity,
   AddressSanitizer, and legacy-absence gates in that same stage; and
5. publish a framework revision only after all gates pass.

No intermediate dual emitter or published mixed state is admitted. No
compatibility typedef, duplicate `.h` output, reference-port mode, by-value
child fallback, whole-core fallback, specialization alias/suffix, `__` name,
opaque suffix, shim, or compatibility flag remains at the cutover.

## Accepted F0 choices

Decision 0274 accepts these choices:

1. **Header/source stem:** implementation Python/AC source stem (`decode.hpp`,
   `decode.cpp`), while the class remains `DecodeStage`.
2. **Child ownership:** `std::unique_ptr` only for generated structural modules.
3. **Queue ownership:** concrete value in the lexical parent; typed raw pointer
   in every child port.
4. **Instance record:** non-owning raw module pointer plus object ID and readable
   source/runtime names.
5. **Header layout:** full generated class layout, not PIMPL.
6. **Backend scheduling:** transactional package emission first; parallel C++
   object compilation immediately; per-unit backend processes only after a
   measured need.
7. **Payload representation:** every nominal struct/interface is a C++ class;
   Queue payloads wider than 64 bits use immutable shared ownership, allocate
   after push reservation, and release Queue ownership at pop commit.
8. **Readable names:** class and instance identifiers exclude enclosing-module
   and specialization suffixes; static parameters are passed separately;
   internal and waveform names are short lower-snake identifiers with single
   underscores.

These eight choices are accepted by Decision 0274. Any change requires a new
decision; implementation agents do not choose alternatives locally.
