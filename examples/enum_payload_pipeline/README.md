# Nested enum payload pipeline

## Payload behavior

Packet is 26 bits: nested Header(opcode6, Mode2), payload17 and matched1.
Explicit Mode width2 preserves the historical codes IDLE=0, RUN=1 and WAIT=2.
The rule copies the input, sets header.mode to RUN, and compares the original
input mode with WAIT for matched. Opcode and payload remain unchanged. The
original input alias is an immutable value snapshot; the nested update does not
change the later comparison.

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

`EnumPayloadPipeline(valid, data, take)` returns typed Result(ready, valid, data).
Input data has the record type above; output retains the same type.

## Original timing

The historical typed input and rule result each owned a depth1/latency1 queue.
The current module retains both with explicit `downstream_pop` ready policy,
matching historical full replacement. Returning the result adds no third queue.
E0 captures an input, E1 commits the transformed token, and E2 is its earliest
consumption. Neither pure rule evaluation nor a copied local adds storage.

## Build and verification

```sh
cmake -S examples/enum_payload_pipeline -B /absolute/build/enum_payload_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/enum_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/enum_payload_pipeline --output-on-failure --no-tests=error
```

The two CTest gates execute the actual generated native DUT with workers 1/2,
Verilator and genuine Icarus. Each checks 603 known Work samples and another
51 four-state Work samples in native/Icarus. Tests cover every payload bit,
all opcode/mode codes including invalid ones, retained and overridden X/Z,
known-conflict versus unresolved equality, and transport of false/X matched data.
Full replacement, stalls, changed rejected offers, held high/low clocks, reset
and drain use a separate two-slot oracle and actual-output retirement ledger.

Known history is 290 accepted, 286 retired, 4 reset-dropped, zero outstanding, peak two slots.
The four-state history is 14 accepted, 10 retired, 4 reset-dropped, zero
outstanding, peak two. The computed unknown comparison bit has no asserted
value plane; copied wire fields retain exact value/known/Z planes. All runs
have finite limits, and test vectors are generated procedurally.
The separate `bench.py` system checks a finite regular-clock known-state scenario with exact old-state output checks. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/enum_payload_pipeline --target cpp --cycles 184 --build-dir .pycircuit_out/enum_payload_pipeline/system-cpp
pycircuit run examples/enum_payload_pipeline --target verilog --cycles 184 --build-dir .pycircuit_out/enum_payload_pipeline/system-verilog
```
