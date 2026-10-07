# Dodgeball game

`DodgeballGame(RST_BTN, START, left, right)` uses four `ac.u1` inputs and returns
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

The single rule saves explicit old-Q snapshots for every RHS and output before
proposing any updates. Guarded writes retain their original last-assignment
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
object collection, source system, recursive elaboration or VGA display adapter.

## Verified implementation

The full public flow passed 8005 Work frames with native workers 1 and 2 and Verilator. The test checks all 50 output bits over 4000 active edges, including button/FSM priority, player movement limits, object and divider timing, the inclusive h-counter wrap and next-line output. Collision/game-over and the full vertical raster are retained in source but are not exhaustively exercised.

See [actual generated excerpts](GENERATED.md) and [verification inputs](GENERATED.json).
