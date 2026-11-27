# Superseded integer/control prototype

These sources preserve the earlier 16-entry index/map experiment. They are not
part of the active P0S compile/link closure and must not be used as a complete
accelerator or copied as current pyCircuit authoring style. The `test_*` files
only inspect text; they never tested generated hardware behavior.

Independent review found stale/unwritten duplicate comparisons, unpaired map
responses, incomplete transfer drain and terminal-state reuse, among other
gaps. The active linear cache needs no token-to-address memory. The replacement
is `src/fa2_engine.py` plus `src/fa2_scheduler.py`, with explicit external index
loading, direct addresses, actual arithmetic and one-outstanding drain.

Historical compile/link/emit logs describe those old bytes only. Keeping this
directory is preservation, not continued support or acceptance evidence.
