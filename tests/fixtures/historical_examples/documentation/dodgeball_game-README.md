# Dodgeball game

`DodgeballGame(RST_BTN, START, left, right)` uses four `bits[1]` inputs and returns
one 50-bit `GameResult`. Field order and types match the original output order:

| Field | Type |
| --- | --- |
| VGA_HS_O | u1 |
| VGA_VS_O | u1 |
| VGA_R | u4 |
| VGA_G | u4 |
| VGA_B | u4 |
| dbg_state | u3 |
| dbg_j | u5 |
| dbg_player_x | u4 |
| dbg_ob1_x | u4 |
| dbg_ob1_y | u4 |
| dbg_ob2_x | u4 |
| dbg_ob2_y | u4 |
| dbg_ob3_x | u4 |
| dbg_ob3_y | u4 |

The fourteen persistent variables total 98 bits: pixel accumulator 16, strobe 1,
main divider 25, player column 4, step counter 5, six object coordinates at 4
bits each, FSM 3, and horizontal/vertical counters at 10 bits each. Reset values
are player column 8 and object columns 1/4/7; every other variable starts at zero.
Object columns remain read-only. Python declares no clock/reset or register
primitive; physical clock/reset inputs and storage are compiler-owned.

All state belongs to one physical clock. The game tick is a combinational test
for the rising transition of bit 4 of the old main divider to its incremented
value, so it is an enable rather than another clock domain. The pixel accumulator
adds 0x4000 at 17 bits, saves its low 16 bits and registers its carry as the next
strobe. VGA consumes the old strobe. Horizontal count wraps when its old value
equals 800, so the line includes counts 0 through 800. Vertical count increments
on that horizontal wrap, but resets whenever its old value equals 524 on an old
pixel-strobe sample. Sync bounds remain horizontal 16..111 and vertical 491..492.
Display x clamps below horizontal 160; display y clamps at 479 and uses nine
low bits. Drawing uses strict rectangle boundaries and four-bit RGB values with
only bit 3 active. Coordinate upper bounds retain the original four-bit
`position + 1` wrap before widening and multiplying by 40.

The single nested Rule owns all fourteen lexical state variables and saves
explicit old-Q snapshots for every RHS and output before proposing any updates.
Guarded writes retain their original last-assignment
priority. In play state, collision overrides the FSM write for `RST_BTN`;
the later `old_j + 1` overrides both the button reset and the `j == 20` reset.
The later object motion writes likewise override those resets, even when their
increment is zero. The game-over state's button reset clears FSM, j and object
Y positions, while retaining the player column. Left and right together do not
move the player; one button moves within columns 0..15 on a game tick.

All outputs observe old Q during Work. Whole-system checking precedes Xfer;
successful rising-edge commit publishes proposed state, while failed/discarded
Work preserves the committed state. Generated physical reset restores the
declared initial values and is separate from the game `RST_BTN` input.

Compile this source independently with `pycircuit compile`, link its explicit
unit closure with `DodgeballGame` selected, then emit C++ or Verilog from the same
verified final artifact. Host testbenches drive physical clock/reset levels
through the typed DUT and shared SystemRunner with a finite runner limit.
This fixed source unrolls three declared objects; it establishes no general
object collection, recursive elaboration or VGA display adapter.

## Verified implementation

The full public flow passed 8005 Work frames with native workers 1 and 2 and Verilator. The test checks all 50 output bits over 4000 active edges, including button/FSM priority, player movement limits, object and divider timing, the inclusive h-counter wrap and next-line output. Collision/game-over and the full vertical raster are retained in source but are not exhaustively exercised.

See [actual generated excerpts](GENERATED.md) and [verification inputs](GENERATED.json).

## Scenario bench

`bench.py` exports `example_dodgeball_game.bench.ExerciseDodgeballGame` and still runs
4,001 complete cycles. Interval data reduces source repetition; it does not skip
simulation epochs, add an interval counter or change the u64 phase update.

Only five expanded debug expectations change: `j`, player x and the three
object y fields. They share 123 maximal constant intervals with inclusive
`end: bits[64]` endpoints, selected by the existing `Table.first` operation.
The first interval covers phases 0–47; the final interval covers 3984 through
UINT64_MAX. All other thirteen input/expected expressions remain unchanged.

All 20,005 changed field values are preserved; the independent reader also
compares the complete 72,018-field original scenario. Long intervals ending at
911, 975 and 1039 retain their actual data instead of assuming a uniform period.
The reset-button sequences still advance j from 27 to 28 and 28 to 29 while
retaining object y, exactly as the original write ordering requires. Player
boundaries, both-held buttons, five-/four-bit wrapping, HS transitions and the
blue pixel at phase 3849 retain their original checks.

The source retains its original DUT calls, guards, assertions, messages, logs
and check-before-advance registration. Independent source proof verifies exact
interval endpoints, typed payloads, complete known-u64 coverage and the remaining
AST. Arbitrary X/Z phase equivalence is not claimed; the closed phase retains
its known initialization and increment. `found` uses the existing query binding
and adds no control or logging.

Twenty complete native/RTL system traces preserve all 112,028 observations and the
terminal result, including native workers 1/2, after excluding only changed
source-position metadata. Regular-clock system tests remain distinct from the
original physical clock/reset histories.

Both versions pass the two existing module/system CTests. The unchanged physical
driver and RTL retain 8,005 known Work samples and exact HISTORY 125 122 15 2.
Collision/game-over and the full vertical raster remain outside exhaustive
coverage, and this example has no existing RTL four-state macro. No new
four-state result is claimed.

See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 1,268 lines / 45,943 bytes to
271 lines / 21,706 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Interval Table |
| --- | ---: | ---: |
| Compile bench | 0.973 s | 0.588 s |
| Link system | 1.618 s | 1.136 s |
| Emit C++ | 1.495 s | 1.045 s |
| Emit Verilog | 1.242 s | 0.882 s |
| Build C++ | 1.065 s | 0.969 s |
| Build Verilog | 1.451 s | 1.604 s |
| First measured full C++ run | 4.823 s | 3.256 s |
| First measured full Verilog run | 1.341 s | 1.393 s |
| Warm C++ median, one worker | 4.644236 s | 2.936177 s |
| Warm Verilog median | 1.069004 s | 1.087912 s |
| Warm native peak RSS median | 3.36 MiB | 2.81 MiB |

| Artifact size | Expanded expressions | Interval Table |
| --- | ---: | ---: |
| Final IR | 6,826,447 bytes | 4,612,128 bytes |
| Generated C++ total | 1,619,153 bytes | 1,394,184 bytes |
| Generated Verilog total | 316,383 bytes | 265,103 bytes |

Recorded first-run regressions: RTL 1.341 → 1.393 s.
They remain separate from warm medians; one observation does not identify a cause.

Warm RTL median increases from 1.069004 to 1.087912 s (1.77%, about 18.91 ms).
Two paired runs are slower and one faster. RTL build also increases from 1.451
to 1.604 s; the first RTL run increases from 1.341 to 1.393 s. These observed costs
remain explicit alongside smaller source/artifacts and improved native runs.
They do not establish a cause or a universal regression.

Three warm pairs describe these local runs; they do not establish causes or
performance across platforms.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUT behavior, independent golden values and stimuli, configuration, cycle limits
and timeouts remain unchanged. The raw RTL harness uses the standard managed
phase protocol for the nested transaction.

```sh
pycircuit run examples/dodgeball_game --target cpp --cycles 4001
pycircuit run examples/dodgeball_game --target verilog --cycles 4001
```
