# Digital clock

This example implements the `applications/digital_clock` design's
fixed 50,000,000-edge divider, 24-hour time and raw setting buttons. Its generated native and RTL models execute the original divider timing;
a short performance pilot alone does not establish historical-root acceptance.

`DigitalClock(btn_set: ac.u1, btn_plus: ac.u1, btn_minus: ac.u1)` owns six
zero-initialized variables: prescaler u26, seconds u6, minutes u6, hours u5,
mode u2 and blink u1. One rule writes these owners. Three direct stateless
`EncodeBcd(value: ac.u6)` calls encode old hours, minutes and seconds; the hour
is zero-extended at the encoder's typed input boundary. Each encoder divides
and takes remainder by literal 10, selects each digit's low four bits, then
widens to u8 before shifting and combining them. Any input X/Z makes all eight
BCD output bits X through the arithmetic operations.

The ordered `ClockResult` fields are `hours_bcd`, `minutes_bcd`, `seconds_bcd`
(u8), `setting_mode` (u2) and `colon_blink` (u1). Mode 0 runs the clock; modes
1, 2 and 3 edit hours, minutes and seconds. Set advances the mode on every
rising edge for which it is held. Plus/minus also repeat on each rising edge;
there is no debounce circuit. Minus wins when both edit buttons are asserted.
Button edits use the mode before the edge, including an edge that also presses
set. Hours wrap at 23 and minutes/seconds wrap at 59; decrementing zero wraps
to the corresponding maximum.

A tick occurs when the old prescaler equals 49,999,999. It resets the prescaler
and toggles blink in every mode. Time advances only when the old mode is RUN,
with seconds-to-minutes-to-hours carry and midnight rollover. Setting modes
hold time without pausing or restarting the prescaler. Thus a 50 MHz physical
clock gives one tick per second. Validation must execute the actual 50,000,000
rising edges per tick; the historical emulator's 1000 Hz assumption is not the
hardware behavior.

All state writes use data selection, preserving the original X/Z bit merging.
In particular, mode is assigned unconditionally from the set-button selection;
an unknown set button is not a register-enable failure. The rule constructs its
result before state assignments: a successful Work sample observes old Q, and
the following Work observes the Xfer commit. Generated `pyc_clk` and `pyc_rst`
are driven through the typed DUT; the Python source declares neither pin.
Clocked reset restores all six variables to zero.

## Validation boundary

The public flow compiles this source once, links `DigitalClock`, and emits C++
and Verilog from the same verified `digital_clock.ac`. An auxiliary gate selects
`EncodeBcd` from the same published source unit without exposing clock state.

Acceptance requires independent native worker-1/worker-2 and RTL checks of the
original behavior, including two actual full-divider periods, rollover,
setting-mode tick suppression, raw-button priority, reset and four-state
selection. Short pilots measure execution budgets only. Sparse printed `WORK`
checkpoints count printed samples; the full checker must separately assert
every executed epoch and its exact final rising-edge count. Generated artifacts
and execution receipts are maintained separately from this source description.

## Verified execution

The full native worker1, native worker2 and Verilator runs each check
200,000,333epochs and100,000,000real rising edges: two original50M-edge periods.
They verify midnight rollover, setting-mode tick suppression and blink toggling,
including leaving setting mode on the second tick. Each trace contains545numbered
checkpoints and one final count summary(546WORK rows), not merely546checkedepochs.
Recorded full command durations are33.86s,32.78s and26.93s on this machine.

The same published source unit supplies the independent encoder gate:64known
values and4,032X/Z patterns plus4,032known recoveries in native and Icarus;
Verilator checks64known values. The main short Icarus test checks332known epochs
and10X/Z cases. Native checks include discard/retry, failure without sample and
clock/reset recovery. The historical design has no debug state injection.

Build Runtime/toolchain and this DUT in Release. The shared verifier budget626s
comes from conservative100K/1M-edge pilots with margin; no full run actually hit
the former180s limit. The generic TIMEOUT_SECONDS option preserves180s by default
for other examples and retains failure logs if a command times out.

```sh
cmake -S examples/digital_clock -B /absolute/build/digital_clock -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/digital_clock --parallel 4
ctest --test-dir /absolute/build/digital_clock --output-on-failure --no-tests=error
```

See [generated output](GENERATED.md) for excerpts from the generated artifacts.
