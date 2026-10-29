"""The reading driver resolves configuration without interpreting ACIR text."""

from __future__ import annotations

import json
import sys
from pathlib import Path

from pycircuit.cli import main


def test_display_config_and_output(tmp_path: Path, monkeypatch, capfd):
    log = tmp_path / "arguments.json"
    helper = tmp_path / "pycircuit-print"
    helper.write_text(
        f"#!{sys.executable}\n"
        "import json, pathlib, sys\n"
        "args = sys.argv[1:]\n"
        "config = json.loads(pathlib.Path(args[args.index('--config') + 1]).read_text())\n"
        f"pathlib.Path({str(log)!r}).write_text(json.dumps([args, config]))\n"
        "output = next((args[i + 1] for i, arg in enumerate(args) "
        "if arg in ('-o', '--output')), None)\n"
        "if output:\n"
        "    pathlib.Path(output).write_text('reading view\\n')\n"
        "else:\n"
        "    sys.stdout.write('reading view\\n')\n"
    )
    helper.chmod(0o755)
    monkeypatch.setenv("PYCIRCUIT_PRINTER", str(helper))
    source = tmp_path / "unit.ac"
    source.write_text("an opaque input passed directly to the native parser\n")
    original = source.read_bytes()
    assert main(["print", str(source)]) == 0
    assert "reading view" in capfd.readouterr().out
    arguments, defaults = json.loads(log.read_text())
    assert str(source) in arguments
    assert "readable" in arguments
    assert defaults["globals"]["compact_types"] is True
    assert defaults["globals"]["source_info"] is False

    custom = tmp_path / "display.json"
    custom.write_text(
        json.dumps(
            {
                "globals": {"empty_attributes": True},
                "ops": {"ac.module": {"fields": ["sym_name"]}},
            }
        )
    )
    output = tmp_path / "readable.txt"
    assert (
        main(
            [
                "print",
                str(source),
                "--profile",
                "debug",
                "--config",
                str(custom),
                "-o",
                str(output),
            ]
        )
        == 0
    )
    arguments, merged = json.loads(log.read_text())
    assert "debug" in arguments
    assert merged["globals"]["empty_attributes"] is True
    assert merged["globals"]["compact_types"] is True
    assert merged["ops"]["ac.module"]["fields"] == ["sym_name"]
    assert output.read_text() == "reading view\n"
    assert source.read_bytes() == original
