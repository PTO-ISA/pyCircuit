# Sparse enum helpers

Opcode4 retains NONE=1, READ=3, WRITE=9 and ERROR=15. Command10 packs raw_opcode4
at9:6, onehot_mask2 at5:4 and nominal selector4 at3:0. All1,024 known combinations
are valid input tokens, including raw/selector codes without an enum member.

| EnumResult20 field | Packed bits | Behavior |
|---|---|---|
| decoded: Opcode | 19:16 | Checked member selection, NONE fallback |
| decoded_valid: u1 | 15 | Raw code belongs to the declared members |
| onehot: Opcode | 14:11 | READ/WRITE for low/high mask bit, NONE empty, ERROR conflict |
| onehot_present: u1 | 10 | OR of mask bits |
| onehot_conflict: u1 | 9 | popcount(mask)>1 |
| selected: u1 | 8 | Selector is READ or WRITE |
| classification: u8 | 7:0 | NONE/READ/WRITE/ERROR→10/11/12/13, invalid→255 |

Boolean fields use explicit u1 physical storage. Predicates remain
logical Boolean until the existing fresh field conversion; this does not claim
logical Boolean nominal-field roundtrips. All Enum fields are explicitly supplied
because Opcode has no zero member.

## Four-state meaning

The checked decoder and classification preserve the original balanced conditional
trees. Selecting the raw carrier when valid, or replacing the balanced tree with
a linear if/elif chain, can change partial-known bits. Onehot conflict must use
popcount, not bit-AND. The source keeps these circuits explicit and shares their
ordinary local values.

| Input | Required computed result |
|---|---|
| raw_opcode=X001 | decoded_valid=X, decoded=XXX1 |
| selector=00X1 | classification=XXXX1XXX |
| mask=0X | present=X, conflict=X, onehot=XXX1 |
| mask=X1 | present=1, conflict=X, onehot=XX11 |

Replacing X with Z produces the same computed symbols. All output fields derive
from comparisons, arithmetic or selection of known constants: computed output
contains no Z. Known/Z masks and known bits are contractual; latent values under
computed X are not. Invalid/unknown enum payloads do not fail or filter tokens.
Payload valid/present/conflict are separate from queue protocol valid.

## Queue timing

`EnumHelpers(valid, data, take)` returns Result22: input ready21, output valid20
and EnumResult20. The input Command10 and output EnumResult20 each have one
depth1/latency1 queue, for two token slots and30 logical payload storage bits.
Explicit downstream_pop permits old-head retirement and replacement on the same
edge. E0 accepts Command, E1 commits its result and E2 first retires it. There is
no empty bypass or extra queue for the returned value.

Held/falling clocks transfer nothing. Rising reset drops both occupied tokens.
Initialized invalid output remains physical packed zero, not a constructed result
with NONE members. The compiler generates hidden clock/reset pins driven by the
host/SystemRunner, and the shared Runtime manages Work/Xfer and discard.

## Build and verification

```sh
cmake -S examples/enum_helpers -B /absolute/build/enum_helpers -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/enum_helpers --parallel 4
ctest --test-dir /absolute/build/enum_helpers --output-on-failure --no-tests=error
```

Independent tests cover the full known Command domain, bounded exhaustive
four-state raw/selector fields and masks, literal distinctions above, dense cases
and both native latent alternatives. Separate 10-bit input and20-bit output
queue oracles check old-state timing, capacity two, replacement, stalls, holds,
busy reset, finite drain, conservation, discard and terminal Reset recovery.
Standalone and targeted aggregate gates each pass 2/2. Native workers1/2 and
Verilator agree on 2,089 known Work rows; native and genuine Icarus also agree
on 3,229 four-state rows. The corpus contains all1,024 known commands and1,584
four-state tokens:512 raw-field,512 selector-field,32 symbolic-mask,512 dense
crossed and16 literal-witness cases, including both native latent alternatives.

Known history accepts1,031 tokens, retires1,027 and reset-drops four;
four-state history is1596/1592/four. Both finish empty at peak occupancy two.
Three direct-owner probes check discard, failed acceptance and failed retirement
without partial commit. The discarded host-Reset probe must actually retire its
retained command's classified result exactly once before a real Reset. Separate
terminal native/RTL probes check unknown-control failure and native Reset recovery.
The finite runner bound is4,000 sampling epochs per successful history.