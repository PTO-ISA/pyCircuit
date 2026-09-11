# Dodgeball Game (pyCircuit)

A cycle-aware rewrite of the dodgeball VGA demo. The design keeps the original
FSM and object motion timing while adding `left/right` movement for the player.
The terminal emulator renders a downsampled VGA view to keep runtime low.

## Key files

- `dodgeball_game.py`: pyCircuit top-level (game FSM, objects, player, VGA colors).
- `tb_dodgeball_game.py`: canonical compiler/simulation entrypoint.
- `dodgeball_capi.cpp`: C API wrapper for ctypes simulation.
- `emulate_dodgeball.py`: terminal visualization + optional auto-build.
- `stimuli/basic.py`: external stimulus for `START/left/right/RST_BTN`.

## Ports

| Port | Dir | Width | Description |
|------|-----|-------|-------------|
| `clk` | in | 1 | System clock |
| `rst` | in | 1 | Synchronous reset (for deterministic init) |
| `RST_BTN` | in | 1 | Game reset input (matches reference behavior) |
| `START` | in | 1 | Start game |
| `left` | in | 1 | Move player left (game tick) |
| `right` | in | 1 | Move player right (game tick) |
| `VGA_HS_O` | out | 1 | VGA HSync |
| `VGA_VS_O` | out | 1 | VGA VSync |
| `VGA_R` | out | 4 | VGA red (MSB used) |
| `VGA_G` | out | 4 | VGA green (MSB used) |
| `VGA_B` | out | 4 | VGA blue (MSB used) |
| `dbg_state` | out | 3 | FSM state (0 init, 1 play, 2 over) |
| `dbg_j` | out | 5 | Object step counter |
| `dbg_player_x` | out | 4 | Player column (0-15) |
| `dbg_ob*_x/y` | out | 4 | Object positions |

## Run (Auto-Build)

The emulator will build the C++ simulation library if it is missing. Use
`--rebuild` to force regeneration.

```bash
python3 examples/pycircuit/applications/dodgeball_game/emulate_dodgeball.py
python3 examples/pycircuit/applications/dodgeball_game/emulate_dodgeball.py --rebuild
```

## Manual Build and Run

```bash
PYTHONPATH=python:. python3 -m pycircuit.cli emit \
  examples/pycircuit/applications/dodgeball_game/dodgeball_game.py \
  -o .pycircuit_out/applications/dodgeball_game/generated/dodgeball_game.pyc

$PYC_TOOLCHAIN_ROOT/bin/pycc .pycircuit_out/applications/dodgeball_game/generated/dodgeball_game.pyc \
  --emit=cpp --out-dir=.pycircuit_out/applications/dodgeball_game/generated

c++ -std=c++20 -O2 -shared -fPIC \
  -I "$PYC_TOOLCHAIN_ROOT/include" \
  -I .pycircuit_out/applications/dodgeball_game/generated \
  .pycircuit_out/applications/dodgeball_game/generated/dodgeball_game.cpp \
  "$PYC_TOOLCHAIN_ROOT/lib/libpyc6_runtime.a" \
  -o .pycircuit_out/applications/dodgeball_game/libdodgeball_sim.dylib \
  examples/pycircuit/applications/dodgeball_game/dodgeball_capi.cpp

python3 examples/pycircuit/applications/dodgeball_game/emulate_dodgeball.py --stim basic
```

## Stimuli

Stimulus is separated from the DUT and loaded as a module.
Available modules live under `examples/pycircuit/applications/dodgeball_game/stimuli/`.

- `basic`: start, move left, then move right, plus a reset/restart sequence.
