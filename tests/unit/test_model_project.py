"""Project UX and supplied-golden comparison without invoking a compiler."""

import json
from pathlib import Path

import pytest
from pycircuit._driver import _DriverError
from pycircuit._project import _record_bits, init_project, load_project


def test_init_is_portable_and_preserves_existing_files(tmp_path):
    project = init_project(tmp_path / "model", "backpressure")
    _, config, expected = load_project(project)
    assert config["consumer_start"] == 6
    assert expected["final_state"] == [4, 27912, 4, 1]
    assert [item["value"] for item in expected["publications"]] == [4811, 28168, 27912]
    assert "FunctionalPipeline" not in (project / "model.py").read_text()
    assert str(Path.cwd()) not in (project / "CMakeLists.txt").read_text()
    before = (project / "model.py").read_bytes()
    with pytest.raises(_DriverError):
        init_project(project)
    assert (project / "model.py").read_bytes() == before


def test_unsupported_schema_and_input_escape_are_rejected(tmp_path):
    project = init_project(tmp_path / "model")
    file = project / "project.json"
    data = json.loads(file.read_text())
    data["schema"] = "future"
    file.write_text(json.dumps(data))
    with pytest.raises(_DriverError):
        load_project(project)
    data["schema"] = "pycircuit-model-project-v1"
    data["inputs"] = "../outside.json"
    file.write_text(json.dumps(data))
    with pytest.raises(_DriverError):
        load_project(project)


def test_expected_raw_pack_uses_declared_offsets():
    port = {
        "width": 9,
        "fields": [
            {"path": ["ok"], "width": 1, "low": 8},
            {"path": ["data"], "width": 8, "low": 0},
        ],
    }
    assert _record_bits({"data": 0xA5, "ok": 1}, port) == "110100101"
    with pytest.raises(_DriverError):
        _record_bits({"data": "xxxxxxxx", "ok": 1}, port)


def test_failed_trace_counts_only_complete_successful_records(tmp_path):
    from pycircuit._model_run import _successful_records

    trace = tmp_path / "trace.jsonl"
    trace.write_text(
        '{"cycle":0,"outputs":{}}\n{"cycle":1,"outputs":{}}\n{"cycle":2,"outputs":'
    )
    assert _successful_records(trace) == 2
