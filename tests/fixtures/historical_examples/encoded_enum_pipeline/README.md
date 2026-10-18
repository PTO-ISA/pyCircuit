# Sparse and wide enum pipeline

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named ingress and
egress handles retain the two depth-one, latency-one `downstream_pop` queues.
One registered `transfer` Rule pops the old ingress head, computes the payload
and pushes the egress atomically; the compiler supplies availability and capacity
guards. Raw `in_valid`/`in_data` and `out_ready` bindings retain the external
handshake, and `ready`/`valid`/`data` expose the queue boundary views. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

## Payload behavior

Command is 73 bits: Opcode4, selected Opcode4, WideOpcode64, matched1.
Opcode codes remain NONE=0, READ=3 and WRITE=9. WideOpcode remains LOW=2**63-1,
HIGH=2**63 and MAX=2**64-1. The rule preserves the incoming opcode, replaces
selected with WRITE and wide with MAX, and computes matched from the incoming
opcode compared with READ. A copy avoids requiring a zero member for WideOpcode.

The Boolean matched field is represented physically as `ac.u1`.
Its comparison remains Boolean and converts at the explicit field boundary.
This does not add general logical Boolean struct fields. Matched is payload data,
independent of the outer transport valid signal.

Invalid enum codes and X/Z are ordinary payload carriers. No membership filter,
repair or runtime assertion is added. Untouched fields preserve value/known/Z;
overridden fields become their declared known constants. Equality returns known
false when a known bit conflicts, and X when unresolved. The new source uses
nominal Enum values and ordinary record copy/field updates through the existing
frontend and common IR.

`EncodedEnumPipeline(valid, data, take)` returns typed Result(ready, valid, data).
Input data has the record type above; output retains the same type.

## Original timing

The historical typed input and rule result each owned a depth1/latency1 queue.
The current module retains both with explicit `downstream_pop` ready policy,
matching historical full replacement. Returning the result adds no third queue.
E0 captures an input, E1 commits the transformed token, and E2 is its earliest
consumption. Neither pure rule evaluation nor a copied local adds storage.

## Build and verification

```sh
cmake -S examples/encoded_enum_pipeline -B /absolute/build/encoded_enum_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/encoded_enum_pipeline --parallel 4
ctest --test-dir /absolute/build/encoded_enum_pipeline --output-on-failure --no-tests=error
```

The module and four-state CTest gates execute the actual generated native DUT with workers 1/2,
Verilator and genuine Icarus. Each checks 281 known Work samples and another
51 four-state Work samples in native/Icarus. Tests cover every payload bit,
all opcode/mode codes including invalid ones, retained and overridden X/Z,
known-conflict versus unresolved equality, and transport of false/X matched data.
Full replacement, stalls, changed rejected offers, held high/low clocks, reset
and drain use a separate two-slot oracle and actual-output retirement ledger.

Known history is 129 accepted, 125 retired, 4 reset-dropped, zero outstanding, peak two slots.
The four-state history is 14 accepted, 10 retired, 4 reset-dropped, zero
outstanding, peak two. The computed unknown comparison bit has no asserted
value plane; copied wire fields retain exact value/known/Z planes. All runs
have finite limits, and test vectors are generated procedurally.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
12-field, 150-bit `Stimulus` preserves the original schema, field order and all
input/expected payloads. Row 184 is all zero, retaining the helper fallback
for every known u16 phase from 184 through 65,535. This fallback is distinct
from the system counter, which holds at 183. The run retains 184 cycles.

Stimulus remains flat Bits data. Both Opcode mappings stay `0→NONE, 3→READ,
else→WRITE`; the wide mapping stays `2^63−1→LOW, 2^63→HIGH, else→MAX`.
Thus a zero wide value in the helper fallback still constructs MAX. The two
u64 columns retain exact integer values, and all three `enum_to_bits` check/log
pairs retain raw-zero invalid expectations. Overwritten selected, wide and
matched inputs remain in the scenarios.

The complete advance and system bodies are unchanged, including unconditional
assertions, observation order and advance-before-check/exercise registration.
Independent source checks compare all 786,432 field values across the known-u16
helper domain. Twenty complete native and RTL traces match all 2,576 observations
and the terminal result after excluding only changed source-position metadata,
including native workers 1 and 2. Baseline and compact versions each pass all
three standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical tests retain 281 known and 51 four-state Work samples,
with the reset, held-clock and conservation checks described above. The
registered four-state test already executes Icarus. Regular-clock systems do not
replace physical histories, and known-u16 source proof does not establish
arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,892 lines / 55,587 bytes to
292 lines / 40,961 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.413 s | 0.996 s |
| Link system | 2.160 s | 1.865 s |
| Emit C++ | 2.142 s | 1.763 s |
| Emit Verilog | 1.617 s | 1.333 s |
| Build C++ | 1.405 s | 1.233 s |
| Build Verilog | 1.764 s | 1.588 s |
| First measured full C++ run | 0.565 s | 0.414 s |
| First measured full Verilog run | 0.300 s | 0.761 s |
| Warm C++ median, one worker | 0.262325 s | 0.031806 s |
| Warm Verilog median | 0.027299 s | 0.026660 s |
| Warm native peak RSS median | 4.00 MiB | 3.06 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 8,905,722 bytes | 7,485,967 bytes |
| Generated C++ total | 2,257,222 bytes | 2,129,814 bytes |
| Generated Verilog total | 505,261 bytes | 418,214 bytes |

The first measured RTL run increases from 0.300 to 0.761 s despite the lower
warm median. That regression is retained separately; one observation does not
identify its cause or establish steady-state behavior.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/encoded_enum_pipeline --target cpp --cycles 184
pycircuit run examples/encoded_enum_pipeline --target verilog --cycles 184
```
