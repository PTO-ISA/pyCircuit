"""Generate an offline HTML document from a committed gfsim journal."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .reader import ReplayError, read_replay


def render(trace: Path, output: Path) -> Path:
    replay = read_replay(trace)
    # Ignore dangling events from an incomplete final barrier.
    from .reader import integer

    events = [e for e in replay.events if integer(e["batch"]) < len(replay.commits)]
    document = {
        "objects": replay.manifest["objects"],
        "initial": replay.initial,
        "commits": replay.commits,
        "events": events,
        "complete": replay.complete,
        "diagnostic": replay.diagnostic,
    }
    payload = json.dumps(document, ensure_ascii=False, separators=(",", ":"))
    payload = (
        payload.replace("<", "\\u003c")
        .replace("\u2028", "\\u2028")
        .replace("\u2029", "\\u2029")
    )
    template = Path(__file__).with_name("viewer.html").read_text(encoding="utf-8")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(template.replace("/*RECORDING*/null", payload), encoding="utf-8")
    return output


def main() -> int:
    parser = argparse.ArgumentParser(prog="circuit-flow-viewer")
    commands = parser.add_subparsers(dest="command", required=True)
    command = commands.add_parser(
        "render", help="Generate a self-contained offline HTML"
    )
    command.add_argument("trace", type=Path)
    command.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        sys.stdout.write(f"{render(args.trace, args.output)}\n")
        return 0
    except (OSError, ReplayError, ValueError) as error:
        parser.exit(2, f"circuit-flow-viewer: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
