# Sparse and wide enum pipeline

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

The two CTest gates execute the actual generated native DUT with workers 1/2,
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