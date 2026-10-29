"""Configure the native, read-only ACIR display tool."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

from ._native_verify import native_helper


def display_config(profile: str, config: str | None) -> dict:
    defaults = Path(__file__).with_name("acir_display.json")
    table = json.loads(defaults.read_text(encoding="utf-8"))
    if profile == "debug":
        table["globals"].update(locations=True, source_info=True)
    if config is not None:
        override = json.loads(Path(config).read_text(encoding="utf-8"))
        if not isinstance(override, dict) or set(override) - {"globals", "ops"}:
            raise ValueError("display config requires globals and/or ops objects")
        for section in ("globals", "ops"):
            values = override.get(section, {})
            if not isinstance(values, dict):
                raise ValueError(f"display config {section} must be an object")
            if section == "globals":
                if set(values) - set(table[section]):
                    raise ValueError("unknown display global option")
                table[section].update(values)
            else:
                for name, rule in values.items():
                    if not isinstance(rule, dict) or set(rule) - {
                        "fields",
                        "omit_empty",
                    }:
                        raise ValueError(f"invalid display rule for {name}")
                    table[section].setdefault(name, {}).update(rule)
    for name, value in table["globals"].items():
        if name == "metadata_fields":
            if not isinstance(value, list) or any(
                not isinstance(field, str) for field in value
            ):
                raise ValueError("metadata_fields must be an array of strings")
        elif type(value) is not bool:
            raise ValueError(f"display option {name} must be boolean")
    for name, rule in table["ops"].items():
        for value in rule.values():
            if not isinstance(value, list) or any(
                not isinstance(field, str) for field in value
            ):
                raise ValueError(f"display rule {name} requires arrays of field names")
    return table


def print_command(
    source: str, *, profile: str, config: str | None, output: str | None
) -> None:
    path = Path(source).resolve(strict=True)
    if output is not None and (
        Path(output).resolve() == path
        or (Path(output).exists() and Path(output).samefile(path))
    ):
        raise ValueError("display output must differ from the input file")
    table = display_config(profile, config)
    with tempfile.TemporaryDirectory(prefix="pycircuit-display-") as scratch:
        resolved = Path(scratch) / "display.json"
        resolved.write_text(json.dumps(table), encoding="utf-8")
        command = [
            str(native_helper("print")),
            str(path),
            "--profile",
            profile,
            "--config",
            str(resolved),
        ]
        # Capture before opening output: a parse failure cannot truncate it.
        result = subprocess.run(command, capture_output=True, check=False)
    if result.returncode:
        raise ValueError(result.stderr.decode("utf-8", errors="replace").strip())
    if output is None:
        sys.stdout.write(result.stdout.decode("utf-8"))
    else:
        Path(output).write_bytes(result.stdout)
