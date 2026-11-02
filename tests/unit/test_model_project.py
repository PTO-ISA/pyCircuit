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


@pytest.mark.parametrize("failure", ["schema", "expectations", "toolchain"])
def test_verification_setup_failure_invalidates_previous_pass(tmp_path, failure):
    from pycircuit._project import verify_project

    project = init_project(tmp_path / "model")
    output = tmp_path / "check"
    output.mkdir()
    receipt = output / "verification.json"
    receipt.write_text('{"status":"passed","views":[{"stale":true}]}\n')
    if failure == "schema":
        manifest = project / "project.json"
        data = json.loads(manifest.read_text())
        data["schema"] = "unsupported"
        manifest.write_text(json.dumps(data))
    elif failure == "expectations":
        (project / "expected.json").unlink()
    with pytest.raises((_DriverError, OSError)):
        verify_project(project, output, selected=tmp_path / "missing-toolchain")
    result = json.loads(receipt.read_text())
    assert result["status"] == "failed"
    assert result["views"] == []
    assert result["error"]


@pytest.mark.parametrize("timer_kind", ["gnu", "bsd", "missing", "gtime"])
def test_benchmark_uses_optional_gnu_time_only(tmp_path, monkeypatch, timer_kind):
    import subprocess

    from pycircuit import _project

    project = init_project(tmp_path / "model")
    prefix = tmp_path / "toolchain"
    (prefix / "bin").mkdir(parents=True)
    (prefix / "bin/pycircuit").touch()
    timer = "/usr/bin/gtime" if timer_kind == "gtime" else "/usr/bin/time"
    monkeypatch.setattr(
        _project.shutil,
        "which",
        lambda name: (
            timer if name == Path(timer).name and timer_kind != "missing" else None
        ),
    )
    monkeypatch.setattr(_project.platform, "platform", lambda: "test platform")
    monkeypatch.setattr(_project.platform, "processor", lambda: "test processor")

    def version(command, **kwargs):
        if command[0] == timer:
            gnu = timer_kind in ("gnu", "gtime")
            return subprocess.CompletedProcess(
                command,
                0 if gnu else 1,
                "time (GNU Time) 1.9\n" if gnu else "",
                "" if gnu else "time: illegal option -- -\n",
            )
        return subprocess.CompletedProcess(command, 0, "test version\n", "")

    commands = []

    def stop_at_verification(command, **kwargs):
        commands.append(command)
        raise _DriverError("stop before invoking fixture toolchain")

    monkeypatch.setattr(_project.subprocess, "run", version)
    monkeypatch.setattr(_project, "_invoke", stop_at_verification)
    with pytest.raises(_DriverError, match="stop before"):
        _project.benchmark_project(project, tmp_path / "benchmark", selected=prefix)
    assert len(commands) == 1
    command = commands[0]
    if timer_kind in ("gnu", "gtime"):
        assert command[:3] == [timer, "-f", "%e %M %U %S"]
    else:
        assert command[:2] == [str(prefix / "bin/pycircuit"), "verify"]
