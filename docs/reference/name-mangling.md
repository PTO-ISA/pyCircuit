# Source, MLIR, and generated-code naming

This document defines the pyCircuit 6 naming contract for Agentic Circuit
artifacts. Source-readable names identify declarations and instances. Typed
records identify finite-family cases. Generated spellings never become a
second semantic identity.

## Family and case identity

Decisions 0274-0278 define one identity model:

- a source-owned module family is identified by its `ac.module` symbol;
- a case is selected structurally by the family symbol plus its complete
  ordered typed static arguments; and
- each runtime instance has its own instance path and owns independent state.

An `ac.module.case` is an ordered non-symbol region inside its family. It has no
case symbol, ordinal identity, source file, generated alias, or backend name.
Equal typed arguments reuse the same family case while repeated instances keep
independent Queues, Tables, Slots, reservations, runtime IDs, and persistent
state.

The following forms are never identity:

- a dictionary, JSON object, or flattened parameter string;
- a concrete or suffixed module symbol;
- a generated class, module, file, local, or waveform name;
- a case ordinal, caller-observed case set, content digest, or opaque token; or
- a sidecar specialization inventory.

## Naming rules

- Preserve meaningful source names for hierarchy review, diagnostics, source
  maps, interface documentation, and generated-source inspection.
- Use one readable family name in ACIR, PYC, C++, and RTL. Static values never
  enter that name.
- Use lower snake case for generated RTL families and locals. Do not repeat an
  enclosing module prefix on a local name.
- Do not emit `__`, a parameter-value suffix, an opaque suffix, or a truncated
  name. A collision or overlength name fails closed.
- Keep direction, endpoint, lane, and stage information when the author puts
  those facts in a name. For example, `input_select_valid_0` stays in that
  order; the compiler does not invent a different endpoint convention.
- Reserve the complete `compiler_` prefix for compiler-owned definitions,
  scopes, adapters, and temporaries. Python systems and modules cannot declare
  names with that prefix.

Target-language legalization may replace unsupported characters, protect a
reserved word, or prefix a digit-leading identifier. It must not add identity
information. If two source names legalize to the same target name, compilation
fails instead of appending a counter, hash, or opaque token.

## Python to ACIR

The frontend preserves case-sensitive Python spelling:

| Python entity | ACIR representation |
| --- | --- |
| `@ac.system def name` | selected `ac.system @name` |
| source-owned `@ac.module_decl` plus `@ac.module` | one `ac.module @name` family |
| one declared finite case | one non-symbol `ac.module.case` region with ordered typed arguments |
| assignment receiving a module result | readable `ac.instance` name plus family reference and typed arguments |
| typed input/output names | ordered `ModuleInterfaceAttr` ports with source provenance |
| rule and local names | case-local key plus display name and source provenance |

`@Top`, `%source_<N>`, `%result_<N>`, and other documented reserved spellings
are compiler-owned wrapper or SSA names. The selected system, source owner,
family schema, interface records, instance path, and source map preserve the
source-facing mapping.

Record-field adapters use readable compiler-owned names such as
`compiler_project_packet_header_opcode`. Equal input type and projection path
reuse one adapter implementation; each call site retains its own source
provenance and instance identity.

## ACIR and PYC family names

One source definition publishes one `ac.module` family symbol containing its
ordered `ac.module.case` regions. `ac.module.import` and `ac.instance` refer to
that family symbol and carry the complete ordered typed schema or arguments
required by their operation. They never refer to a concrete case name.

PYC preserves the same shape with one `pyc.module` family and ordered
non-symbol `pyc.module.case` regions. The verified logical-to-physical mapping
carries projection paths, packed layouts, physical roles and indices, Queue
lanes and rate, the one shared Queue-ready carrier, and explicit implicit
clock/reset origins. A backend consumes those records directly. It does not
recover identity from a function name, physical width, path string, or
`pyc.params` text.

## Generated C++ ownership and names

Every executable implementation source owns exactly one public module family.
Its normalized source stem owns one generated source group:

```text
include/generated/modules/<source_stem>.hpp
src/generated/modules/<source_stem>.cpp
```

The same source owner publishes one nominal interface shard:

```text
include/generated/interfaces/<source_stem>_interface.hpp
```

All generated C++ headers use `.hpp`; no `.h` duplicate or compatibility name
exists. Core composition and lifecycle glue also use readable `.hpp` names and
remain separate from source-owned implementation groups.

The C++ family identifier is the readable Pascal-style legalization of the
source definition:

1. treat every non-alphanumeric character as a word boundary;
2. uppercase the first alphanumeric character and the first character after a
   boundary;
3. preserve the remaining alphanumeric characters; and
4. prefix `_` only when the result is empty or digit-leading.

For example, `alu_pipeline` becomes `AluPipeline`. Typed static parameters are
template arguments or typed parameter objects. Admitted cases are explicit
materializations of that one identifier. They do not gain a class alias,
parameter-bearing class name, source file, or opaque token.

Parents own internal `SimQueue<T>` objects as concrete values and children as
`std::unique_ptr<Child>`. Child ports are non-owning typed `SimQueue<T> *`
pointers. These target-language types express ownership; they do not change
family, case, or instance identity.

## Generated RTL names

Each source family emits one readable lower-snake-case RTL module family.
Typed parameters and checked generate branches cover the complete declared
finite case set. Parameter values never enter the module name, and the emitter
never creates one RTL module per case.

Local RTL names omit repeated family prefixes. A cross-module signal names its
endpoints once. Collisions and overlength names reject before emission; the
emitter does not truncate a name or append a hash.

## Example

```text
Python declaration:    alu_pipeline
ACIR family:           @alu_pipeline
case arguments:        (lanes = 4, width = 32)
PYC family:            @alu_pipeline
C++ family:            AluPipeline
C++ source stem:       alu_pipeline.hpp / alu_pipeline.cpp
interface shard:       alu_pipeline_interface.hpp
RTL family:            alu_pipeline

Python interface:      input_select_valid_0
ACIR display name:     input_select_valid_0
C++/RTL local:         input_select_valid_0
```

The case arguments select behavior and concrete types without changing any
family, class, file, or RTL module name.

## Source traceability

Adjacent Python comments using `# ndf:` and `# ndf:requires` become
non-semantic `ac.ndf_ids` and `ac.ndf_requires` metadata. Generated C++ prints
the NDF identifiers beside module/rule and `source: path:line:column` comments.
The complete inline stack remains in the source-map artifact.

Source paths, provenance frames, display names, and NDF identifiers support
review and diagnostics. They do not participate in family, case, topology, or
release identity.

## Source-unit and package boundary

CMake invokes `acc.py` independently for every executable Python source. Each
invocation publishes one source-named `.ac` containing that source's one module
family and complete declared case set. A separate composition source compiles
the selected system and explicit links. Source-owned interface shards let
parents type-check children without consuming child implementation bodies.

Native linking verifies the complete family schema, source owner, nominal
declarations, ordered cases, and materialized signatures before backend
emission. The linked design is not a replacement whole-program authority, and
the compiler never obtains modular units by splitting a previously compiled
whole-system artifact.

The package has no `root.ac`, monolithic `types.ac`, `shared/` compatibility
tree, specialization manifest, post-compile AC split, backend-only C++ split,
or whole-core fallback. Generated C++ preserves the source boundary as one
independently compiled `.hpp`/`.cpp` group per implementation source plus its
one interface shard and separate core glue.
