# Digital clock

This clock uses a fixed 50,000,000-edge divider, 24-hour time and raw setting buttons. Its generated native and RTL models execute the original divider timing;
a short performance pilot alone does not establish historical-root acceptance.

`DigitalClock(btn_set: ac.u1, btn_plus: ac.u1, btn_minus: ac.u1)` owns six
zero-initialized variables: prescaler u26, seconds u6, minutes u6, hours u5,
mode u2 and blink u1. One nested rule writes these lexical owners as one atomic
transaction. Three direct stateless
`EncodeBcd(value: ac.u6) -> ac.u8` calls encode old hours, minutes and seconds; the hour
is zero-extended at the encoder's typed input boundary. Each encoder divides
and takes remainder by literal 10, selects each digit's low four bits, then
widens to u8 before shifting and combining them. Any input X/Z makes all eight
BCD output bits X through the arithmetic operations. The encoder arithmetic is
an ordinary typed helper, so the independently linkable encoder Module remains
pure combinational hardware rather than registering a transaction.

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

The nested rule saves all old-Q values before serial candidate updates. State
writes use data selection, preserving the original X/Z bit merging.
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

## Scenario systems

`bench.py` contains two closed systems:

| Root | Purpose | Complete run |
| --- | --- | --- |
| `ExerciseDigitalClockSettings` | Raw button editing, wrapping and simultaneous-button priority | 160 cycles / 320 sampling epochs |
| `ExerciseDigitalClock` | Two full divider periods, midnight rollover and a tick held in setting mode | 100,000,001 cycles / 200,000,002 sampling epochs |

The minute and second checks share one immutable 64-element `bits[8]` Table.
The original minute expectations at phases 30–93 and second expectations at
phases 94–157 are identical literal sequences. Each window has a zero prefix
and a constant tail of 88 (`0x58`). The table stores those original raw BCD
patterns directly; it does not recompute the expected values from the DUT.

Full-width phase guards select each prefix, active window and tail. The two
indices, `(phase - 30)[:6]` and `(phase - 94)[:6]`, stay within the 64-element
Table even when unsigned subtraction wraps. Guards use the complete u64 phase,
so higher phase values cannot alias the active windows. This preserves the full
known-u64 behavior; arbitrary X/Z phase equivalence is not claimed.

All eight fields across the original 160 settings scenarios remain identical.
The button, hour, mode and blink expressions keep their original form, as do
the DUT calls, five assertions, five unconditional logs and check-before-advance
order. The long root and shared advance rule retain their exact source bytes.
Regular low/high system sampling remains separate from the physical drivers'
held-level, reset, failure/discard and four-state scenarios.

## Verification

Both the expanded and Table versions pass all three existing CTests with the
same checkout-built tools and Release configuration:

| Test | Preserved coverage |
| --- | --- |
| `digital_clock` | Native workers 1/2 and Verilator each check 200,000,333 epochs and 100,000,000 real rising edges; two ticks, midnight rollover, a tick held while setting, and no pilot mode. The 546 printed WORK rows are sparse checkpoints plus a summary. |
| `digital_clock_system` | Native workers 1/2 and Verilator each finish 200,000,002 epochs with all five original source checks. This root has no log observations; successful termination and bound executable inputs establish the full duration. |
| `digital_clock_encoder` | Native and Icarus check 64 known values, 4,032 X/Z patterns and 4,032 known recoveries; Verilator checks the 64 known values. The same gate runs the original main-clock Icarus test with 332 known epochs and ten X/Z cases. |

The settings root is linked separately and produces 1,600 observations plus a
successful terminal result on native workers 1/2 and RTL. All 24 complete
first/warm setting traces agree across the two source versions after removing
only changed source-position metadata. Independent source proof checks every
field, the full known-u64 tail, protected source bytes and mutation rejection.
Complete source-import and transformed artifacts reproduce the published units;
both roots retain final IR and complete C++/Verilog outputs. These results do
not establish full-catalog, nightly or cross-platform acceptance.

Set the existing CMake environment variable before configuring **and running**
the tests. The public runner creates nested build trees, which do not inherit
the outer CMake cache. Fresh nested caches must therefore receive the same explicit Release setting.

```sh
export PYC_TOOLCHAIN_ROOT=/absolute/pycircuit/install
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
export CMAKE_BUILD_TYPE=Release
cmake -S examples/digital_clock -B .pycircuit_out/digital-clock -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PYC_TOOLCHAIN_ROOT"
cmake --build .pycircuit_out/digital-clock --parallel 4
ctest --test-dir .pycircuit_out/digital-clock --output-on-failure --no-tests=error
```

Use a fresh build directory when setting the environment variable: CMake uses
it only to initialize a new cache. The module command budget remains 626 seconds,
the long-system budget 1,800 seconds and the encoder command budget 240 seconds.
DUT behavior, independent oracle computations, clock divisor, scenario values,
cycle counts and timeouts remain unchanged. The RTL harness now drives the
standard managed transaction phases required by the nested Rule.
See [generated excerpts and verification inputs](GENERATED.md) for the module.

## Run either system

The catalog selects the long root. With the environment above, run it with:

```sh
pycircuit run examples/digital_clock --target cpp --cycles 100000001
pycircuit run examples/digital_clock --target verilog --cycles 100000001
```

Changing that command to `--cycles 160` still selects the long root. To run the
settings root, use the existing public source-unit commands and generated build:

```sh
out="$PWD/.pycircuit_out/clock-settings"
pycircuit compile -c examples/digital_clock/digital_clock.py \
  --source-root examples/digital_clock --package-prefix example_digital_clock \
  -o "$out/units/digital_clock"
pycircuit compile -c examples/digital_clock/bench.py \
  --source-root examples/digital_clock --package-prefix example_digital_clock \
  -I "$out/units/digital_clock" -o "$out/units/bench"
pycircuit link "$out/units/digital_clock" "$out/units/bench" \
  --top example_digital_clock.bench.ExerciseDigitalClockSettings -o "$out/settings.ac"
for target in cpp verilog; do
  pycircuit emit "$out/settings.ac" --target "$target" -o "$out/$target"
  cmake -S "$out/$target" -B "$out/build-$target" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PYC_TOOLCHAIN_ROOT"
  cmake --build "$out/build-$target" --target pycircuit_sim --parallel 4
done
"$out/build-cpp/bin/pycircuit_sim" --cycles 160 --workers 1
"$out/build-cpp/bin/pycircuit_sim" --cycles 160 --workers 2
"$out/build-verilog/bin/pycircuit_sim" +cycles=160
```
