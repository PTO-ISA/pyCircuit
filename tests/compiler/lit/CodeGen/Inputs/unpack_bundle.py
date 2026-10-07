"""Unpack emitter artifacts; expected DUT values stay in independent harnesses."""
import json
import pathlib
import shlex
import sys

bundle = json.loads(pathlib.Path(sys.argv[1]).read_text())
root = pathlib.Path(sys.argv[2])
root.mkdir(parents=True, exist_ok=True)

def write(name, text):
    if text is None:
        return
    path = pathlib.PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError("artifact path escapes test directory")
    dest = root / path
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(text)

for key, name in (("support_header", "pycircuit_support.hpp"),
                  ("system_header", "pycircuit_system.hpp")):
    if key in bundle:
        write(name, bundle[key])
if "rtl_core" in bundle:
    write("design_top.sv", bundle["rtl_core"])
sources = []
for group in bundle.get("source_groups", []):
    write(group["header_path"], group["header"])
    if group.get("source_path"):
        write(group["source_path"], group["implementation"])
        sources.append(str(root / group["source_path"]))
for group in bundle.get("rtl_source_groups", []):
    write(group["path"], group["text"])

(root / "sources.rsp").write_text("\n".join(shlex.quote(path) for path in sources) + "\n")
