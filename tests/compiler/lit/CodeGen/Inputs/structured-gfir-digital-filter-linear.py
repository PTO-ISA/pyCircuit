"""Derive the source's linear mux form without duplicating its FIR arithmetic."""

import sys
from pathlib import Path

source = Path(sys.argv[1]).read_text()
start = source.index("      %next:4 = gfir.if ")
body_start = source.index("\n", start) + 1
otherwise = source.index("      } else {\n", body_start)
end = source.index("      }\n", otherwise) + len("      }\n")
body = source[body_start:otherwise]
body = body[: body.index("        gfir.yield ")]
# Both forms retain identical computations and old-state snapshots. Only the
# four proposal joins change from gfir.if results to ordinary scalar selects.
linear = "".join(line[2:] for line in body.splitlines(keepends=True))
for ordinal, (take, hold, width) in enumerate(
    (
        ("delay2", "delay3", 16),
        ("delay1", "delay2", 16),
        ("input", "delay1", 16),
        ("total", "old_output", 34),
    )
):
    linear += (
        f'      %next{ordinal} = "ac.bits.select"(%valid, %{take}, %{hold}) '
        f": (!b1, !b{width}, !b{width}) -> !b{width}\n"
    )
result = source[:start] + linear + source[end:]
for ordinal in range(4):
    result = result.replace(f"%next#{ordinal}", f"%next{ordinal}")
Path(sys.argv[2]).write_text(result)
