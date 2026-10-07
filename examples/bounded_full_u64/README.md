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
