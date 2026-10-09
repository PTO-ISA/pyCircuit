"""Check compact uniform families without freezing generated source spelling."""

import json
import re
import subprocess
import sys
from pathlib import Path

emitter, input_dir, output_dir = sys.argv[1:]
root = Path(output_dir)
root.mkdir(parents=True, exist_ok=True)
records = []
for count in (1, 64, 4096):
    fixture = Path(input_dir) / f"collection_composite_{count}.mlir"
    commands = {}
    for target in ("cpp", "verilog"):
        cmd = [emitter, str(fixture), "--target", target]
        run = subprocess.run(cmd, text=True, capture_output=True)
        (root / f"{count}-{target}.json").write_text(run.stdout)
        (root / f"{count}-{target}.stderr").write_text(run.stderr)
        assert run.returncode == 0, run.stderr
        bundle = json.loads(run.stdout)
        if target == "cpp":
            groups = bundle["source_groups"]
            source = "\n".join(
                g["header"] + g.get("implementation", "") for g in groups
            )
            definitions = source.count("public gfsim::SimModule")
        else:
            groups = bundle.get("rtl_source_groups", [])
            source = bundle["rtl_core"] + "\n" + "\n".join(g["text"] for g in groups)
            definitions = len(re.findall(r"^module\s+\w+", source, re.MULTILINE))
        commands[target] = {
            "command": cmd,
            "exit_status": run.returncode,
            "bytes": len(source.encode()),
            "groups": len(groups),
            "definitions": definitions,
        }
    records.append({"lanes": count, "targets": commands})
for target in ("cpp", "verilog"):
    smallest = records[0]["targets"][target]
    for record in records[1:]:
        current = record["targets"][target]
        assert (
            current["groups"] == smallest["groups"]
        ), "uniform collection emitted per-lane source groups"
        assert (
            current["definitions"] == smallest["definitions"]
        ), "uniform collection emitted per-lane definitions"
        assert (
            current["bytes"] <= smallest["bytes"] + 16384
        ), "uniform collection expanded into lane-sized source"
(root / "structure.json").write_text(json.dumps(records, indent=2) + "\n")
