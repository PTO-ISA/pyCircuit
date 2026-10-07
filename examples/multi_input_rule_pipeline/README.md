# Atomic buffered input sum

Original `int` resolves to unsigned fixed 64-bit hardware. The inline sum of
the old left and right heads therefore wraps modulo 2^64, including MAX+1 to
zero. The source retains two D2/L1 input queues and one inferred D1/L1 result
queue, now explicit. All three use `downstream_pop` readiness, old committed
head reads and no empty bypass; the sink adds no storage.

The join consumes one left and one right head atomically when both are
available and result room exists. The left queue's take is
`right_available & stage_ready`, the right queue's take is
`left_available & stage_ready`, and result valid is
`left_available & right_available`. These expressions use eligible forward
queue-result Names directly, without an unsupported forward ordinary local
wire, scalar helper rule or extra instance. Compilation verifies the complete dependencies through the existing shared analysis.

The nth accepted left pairs with the nth accepted right. Inputs can arrive
independently; an absent peer or stalled result prevents both input pops. A
newly captured peer cannot bypass its queue into a same-edge sum. When both
contributors arrive at E0, the result commits at E1 and can retire at E2.
Full result retirement permits atomic replacement while either full input
queue may also replace. Held clock levels commit nothing despite changed
offers/take. Host-driven rising reset clears all three owners; Python adds
no clock/reset or transaction proposals.

There are five physical scalar storage slots: left two, right two, sum one.
A sum retains two contributor identities, so the fully occupied state contains
six accepted contributors. For each stream, accepted contributors equal retired
pairs plus reset-dropped contributors plus its input occupancy plus result
occupancy. Reset drops left-input-plus-result identities on the left and
right-input-plus-result identities on the right. A ledger that counts a sum
only once cannot prove contributor conservation.

`JoinResult` packs sum at bits 0 through 63, valid at 64, right-ready at 65 and
left-ready at 66, for 67 bits total. Any X/Z in either operand makes all 64 sum
bits X with no Z and unspecified hidden value plane. Data uncertainty never
changes the atomic control; zero and unknown sums remain valid tokens.

## Build route

From the repository root, use the accepted installed toolchain:

```sh
cmake -S examples/multi_input_rule_pipeline -B /absolute/build/multi_input_rule_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/multi_input_rule_pipeline --parallel 4
ctest --test-dir /absolute/build/multi_input_rule_pipeline --output-on-failure --no-tests=error
```

The public example helper compiles this source independently, links the complete
explicit unit closure and emits both targets from one verified final artifact.
Keep outputs outside the source tree. Both gates also run through the aggregate examples build.

## Verification

Generated native workers 1/2 and Verilator agree on 8,751 known Work samples;
genuine full-DUT Icarus also agrees with 1,173 four-state samples. The independent
oracle uses ripple addition, two depth-two input deques, one result slot and
separate left/right contributor identities. Distinct asymmetric operands,
carry boundaries, independent arrival skew, blocked results, held clocks,
full replacement, input pointer wrap, resets and recovery are exercised.
Every input bit sees X/Z with both latent values, separately and alongside
uncertainty in its peer. All 67 output bits are checked; only computed unknown
sum latent values are unconstrained.

Actual-DUT ready/valid signals qualify accepted contributors and retired pairs.
Known runs accept 4347 contributors on each side, retire 4341 pairs and reset-drop
six contributors per side. Four-state runs accept 558 per side, retire 552 pairs
and reset-drop six per side. Every run finishes empty, reaches five physical
slots and six contributors, and checks independent per-stream conservation.
Limits are finite at 12,000 ticks. Final IR checks exactly the original
D2/D2/D1 owners and complete 64/67-bit interfaces, without extra instances or
collections. No framework change was needed.