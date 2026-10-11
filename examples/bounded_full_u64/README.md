# Full unsigned 64-bit bounded results

For every known `raw: ac.u64`, `0 <= raw < 2^64`. Wrapping into `[0, 2^64)`
therefore gives `raw mod 2^64 = raw`; saturation leaves an already in-range
value unchanged; the checked value is `raw`, and its membership flag is true.
This proof includes zero, the high-bit boundary and UINT64_MAX. The source
expresses those identities by transporting the complete 64-bit value to three
output queues and storing one in the flag queue. It uses existing fixed bits
and queues, without introducing a bounded-integer constructor, conversion API
or compiler opcode. This concrete full-range identity does not establish
general `wrap`, `saturate` or `checked` source support.

The old input head advances only when it is available and all four output
queues are ready. That same group condition pushes all four results together.
Each output then retires under its own `take_*` input, so asymmetric consumers
can leave different output-valid states. A consumed output cannot receive a
partial copy of the next input while another output blocks the group.
The check flag is a queued payload: it is one when valid, distinct from its
separate `checked_flag_valid` handshake signal.

All five queues explicitly use `ready_policy="downstream_pop"`, depth one and
availability latency one. They read old committed heads and have no empty
flow-through. Full queues can retire an old head and replace it on the same
edge. The declared payload storage is 257 bits: 64 input bits, three 64-bit
output slots and one flag slot, in addition to queue control metadata. From
empty state, E0 captures the external input, E1 can capture all four results,
and E2 is their earliest independent external retirement. Stalls retain
pending tokens; held clock levels do not transfer them, and rising synchronous
reset empties every queue.

`BoundedFullU64(valid, raw, take_wrapped, take_saturated, take_checked_value,
take_checked_flag)` returns a 198-bit `FullResult`, with fields in declaration
order: ready, wrapped-valid/value, saturated-valid/value, checked-value-valid/
value, and checked-flag-valid/value. Python contains no clock/reset or
current/next API; the host drives generated physical pins. Existing
whole-system Work/Xfer checks govern commit and discard. Forward connections
use ordinary queue result wires through the same common IR as the accepted
[broadcast pipeline](../broadcast_pipeline/README.md).

## Current public flow

Compile `bounded_full_u64.py` independently with `pycircuit compile`, link its
complete unit closure selecting `BoundedFullU64`, and emit C++ or Verilog from
the same verified final artifact with `pycircuit emit --target cpp|verilog`.
The shared example helper builds the actual generated models, with separate
host testbenches and finite runner limits. Keep generated artifacts outside
the source tree.

## Verified implementation

The full public flow passed 299 Work frames with native workers 1 and 2 and Verilator. Independent queues and per-output histories check full-width boundary values, atomic publication, independent stalls, replacement, reset and drain. This is focused known-value validation, without an exhaustive or four-state claim.

See [actual generated excerpts](GENERATED.md) and [verification inputs](GENERATED.json).

## Scenario bench

`bench.py` replaces the original 158-case if/elif chain with an immutable typed
Table of 159 rows. The 15-field, 267-bit `Stimulus` keeps four complete u64
values and eleven one-bit fields. Row 158 is all zero, preserving the helper's
fallback for every known u16 phase from 158 through 65,535. The system counter
instead holds at phase 157; its last active row has raw=4,194,304, all four takes
set and ready=1, so it is not interchangeable with the zero fallback.

Every u64 field retains values through UINT64_MAX using exact integers. All
48 rows with asymmetric takes and 36 rows with asymmetric output-valid flags
remain intact. The checked-flag payload stays distinct from its valid signal.
The complete system keeps six ordered DUT inputs, nine unconditional assertions,
ten observations and advance-before-exercise registration. The run still lasts
158 cycles.

Independent source checks compare all 983,040 field values across the complete
known-u16 helper domain. Twenty complete native and RTL traces match all 3,160
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Both versions pass the two standalone
CTests. See the module [excerpts and receipt](GENERATED.md).

The unchanged 299-sample physical driver retains separate channel histories,
atomic publication, asymmetric consumption, held clocks, reset and drain. This
example's acceptance remains focused on known values; it has no independent
four-state gate. The compact helper does not establish arbitrary X/Z-phase
equivalence or complete physical-scenario migration.

## Local cost comparison

The source shrinks from 2,016 lines / 63,512 bytes to
262 lines / 46,843 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run figures are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Compact source |
| --- | ---: | ---: |
| Compile bench | 5.063 s | 1.060 s |
| Link system | 8.565 s | 1.926 s |
| Emit C++ | 6.289 s | 1.924 s |
| Emit Verilog | 5.859 s | 1.439 s |
| Build C++ | 1.450 s | 1.119 s |
| Build Verilog | 1.532 s | 1.496 s |
| First measured full C++ run | 0.559 s | 0.323 s |
| First measured full Verilog run | 0.328 s | 0.278 s |
| Warm C++ median, one worker | 0.254303 s | 0.033493 s |
| Warm Verilog median | 0.032546 s | 0.031340 s |
| Warm native peak RSS median | 4.45 MiB | 3.14 MiB |

| Artifact size | Expanded cases | Compact source |
| --- | ---: | ---: |
| Final IR | 43,295,343 bytes | 8,045,102 bytes |
| Generated C++ total | 2,660,656 bytes | 2,324,652 bytes |
| Generated Verilog total | 588,129 bytes | 441,282 bytes |

Three warm pairs do not establish statistical significance for small RTL changes;
no universal runtime speedup is claimed.

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/bounded_full_u64 --target cpp --cycles 158
pycircuit run examples/bounded_full_u64 --target verilog --cycles 158
```
