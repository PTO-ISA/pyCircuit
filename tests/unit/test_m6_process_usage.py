"""Focused tests for M6 owned-process-group resource measurements."""

from __future__ import annotations

import importlib.util
import json
import math
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
USAGE_TOOL = ROOT / "flows/tools/m6_process_usage.py"
RESOURCE_TOOL = ROOT / "flows/tools/measure_m6_resources.py"
RESOURCE_TOOLS = ROOT / "flows/tools"
STEMS = ["types", "leaf", "design_top"]


def _usage_tool():
    spec = importlib.util.spec_from_file_location("m6_process_usage", USAGE_TOOL)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load process-usage helper: {USAGE_TOOL}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _resource_tool():
    inserted = str(RESOURCE_TOOLS) not in sys.path
    if inserted:
        sys.path.insert(0, str(RESOURCE_TOOLS))
    try:
        spec = importlib.util.spec_from_file_location(
            "m6_resource_measurement_tests", RESOURCE_TOOL
        )
        if spec is None or spec.loader is None:
            raise RuntimeError(
                f"cannot load resource measurement tool: {RESOURCE_TOOL}"
            )
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    finally:
        if inserted:
            sys.path.remove(str(RESOURCE_TOOLS))


def _backend_files(target: str, stems: list[str]) -> tuple[list[dict], list[dict]]:
    if target == "cpp":
        common = [
            ("CMakeLists.txt", "cmake"),
            ("dut.h", "header"),
            ("model_api.cpp", "runtime-glue"),
            ("pycircuit_support.hpp", "runtime-glue"),
            ("pycircuit_system.hpp", "runtime-glue"),
            ("runner_main.cpp", "runtime-glue"),
            ("runner_metadata.hpp", "runtime-glue"),
        ]
        groups = []
        for stem in stems:
            paths = [f"sources/m6/{stem}.source-map.json"]
            if stem != "types":
                paths += [f"sources/m6/{stem}.cpp", f"sources/m6/{stem}.hpp"]
            else:
                paths += [f"sources/m6/{stem}.hpp"]
            groups.append(
                {"source": {"package": "m6", "path": f"{stem}.py"}, "files": paths}
            )
    else:
        common = [
            ("CMakeLists.txt", "cmake"),
            ("design_top.sv", "rtl"),
            ("rtl_system.hpp", "runtime-glue"),
            ("runner_bridge.sv", "runtime-glue"),
            ("runner_main.cpp", "runtime-glue"),
            ("runner_metadata.hpp", "runtime-glue"),
        ]
        groups = []
        for stem in stems:
            paths = [f"sources/m6/{stem}.source-map.json"]
            if stem != "types":
                paths += [f"sources/m6/{stem}.sv"]
            groups.append(
                {"source": {"package": "m6", "path": f"{stem}.py"}, "files": paths}
            )
    files = [{"path": path, "role": role} for path, role in common]
    for group in groups:
        for path in group["files"]:
            if path.endswith(".cpp") or path.endswith(".sv"):
                role = "source" if path.endswith(".cpp") else "rtl"
            elif path.endswith(".source-map.json"):
                role = "source-map"
            else:
                role = "header"
            files.append({"path": path, "role": role})
    return files, groups


def _write_backend_fixture(root: Path, target: str) -> Path:
    files, groups = _backend_files(target, STEMS)
    for row in files:
        path = root / row["path"]
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("fixture\n", encoding="utf-8")
    receipt = {
        "kind": "pycircuit-generated",
        "target": target,
        "files": files,
        "source_groups": groups,
    }
    root.mkdir(parents=True, exist_ok=True)
    (root / "generated.json").write_text(json.dumps(receipt), encoding="utf-8")
    return root / "generated.json"


def _assert_finite_nonnegative(value: object) -> None:
    assert type(value) in {int, float}
    assert math.isfinite(value)
    assert value >= 0


def test_ps_parser_keeps_only_nonnegative_rss_for_owned_process_group(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    tool = _usage_tool()

    def fake_run(argv, **kwargs):
        assert argv == ["ps-fixture", "-axo", "pid=,pgid=,rss="]
        return subprocess.CompletedProcess(
            argv,
            0,
            stdout="101 77 4096\n102 78 9000\n103 77 -1\ninvalid row\n",
            stderr="",
        )

    monkeypatch.setattr(tool.subprocess, "run", fake_run)
    members, error = tool._sample_process_group("ps-fixture", 77)

    assert error is None
    assert members == [{"pid": 101, "rss_kib": 4096}]


def test_sampling_error_and_unavailable_platform_are_explicit(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    tool = _usage_tool()

    def failed_run(*_args, **_kwargs):
        raise OSError("ps denied")

    monkeypatch.setattr(tool.subprocess, "run", failed_run)
    members, error = tool._sample_process_group("ps-fixture", 1)
    assert members == []
    assert error and "ps sample failed" in error and "ps denied" in error

    monkeypatch.setattr(tool.shutil, "which", lambda _name: None)
    result = tool.run_sampled(
        [sys.executable, "-c", "print('ok')"],
        cwd=tmp_path,
        env=os.environ.copy(),
        log_dir=tmp_path / "logs",
        label="no-ps",
    )
    assert result["exit_status"] == 0
    assert result["timed_out"] is False
    assert result["rss"]["available"] is False
    assert result["rss"]["unit"] == "KiB"
    assert result["rss"]["sample_count"] == 0
    assert result["rss"]["peak_sampled_sum_kib"] is None
    assert result["rss"]["unavailable_reason"] == "ps executable unavailable"
    _assert_finite_nonnegative(result["wall_seconds"])
    assert Path(result["stdout_log"]).read_text(encoding="utf-8") == "ok\n"


@pytest.mark.parametrize("invalid", [math.nan, math.inf, -math.inf])
@pytest.mark.parametrize("field", ["sample_interval_seconds", "timeout_seconds"])
def test_sampler_rejects_nonfinite_interval_and_timeout_before_creating_logs(
    tmp_path: Path, field: str, invalid: float
) -> None:
    tool = _usage_tool()
    log_dir = tmp_path / "must-not-be-created"
    with pytest.raises(ValueError, match="finite and positive"):
        tool.run_sampled(
            [sys.executable, "-c", "raise SystemExit('must not run')"],
            cwd=tmp_path,
            env=os.environ.copy(),
            log_dir=log_dir,
            label="invalid-number",
            **{field: invalid},
        )
    assert not log_dir.exists()


@pytest.mark.parametrize(
    ("option", "value", "message"),
    [
        ("--rss-interval", "nan", "finite and positive"),
        ("--rss-interval", "inf", "finite and positive"),
        ("--rss-interval", "-inf", "finite and positive"),
        ("--timeout", "nan", "finite and positive"),
        ("--timeout", "inf", "finite and positive"),
        ("--timeout", "-inf", "finite and positive"),
        ("--epochs", "0", "1..UINT64_MAX"),
        ("--epochs", "-1", "1..UINT64_MAX"),
        ("--epochs", str(1 << 64), "1..UINT64_MAX"),
    ],
)
def test_measurement_cli_rejects_invalid_limits_before_creating_output(
    tmp_path: Path, option: str, value: str, message: str
) -> None:
    output = tmp_path / "must-not-be-created"
    invalid_option = f"{option}={value}" if value == "-inf" else option
    invalid_value = [] if value == "-inf" else [value]
    result = subprocess.run(
        [
            sys.executable,
            str(RESOURCE_TOOL),
            "--prefix",
            str(tmp_path / "missing-prefix"),
            "--output-dir",
            str(output),
            invalid_option,
            *invalid_value,
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
        timeout=10,
    )
    assert result.returncode == 2
    assert message in result.stderr
    assert not output.exists()


def test_backend_inventory_accepts_only_materialized_owned_outputs(
    tmp_path: Path,
) -> None:
    tool = _resource_tool()
    build = tmp_path / "generated"
    for target in ("cpp", "verilog"):
        _write_backend_fixture(build / target, target)

    inventory = tool._validate_backend_inventory(build, STEMS, None)
    assert inventory["cpp"]["implementation_file_count"] == 2
    assert inventory["verilog"]["implementation_file_count"] == 2
    for target in ("cpp", "verilog"):
        root = build / target
        for group in inventory[target]["groups"]:
            for row in group["resolved_files"]:
                resolved = Path(row["resolved_path"])
                assert resolved.is_relative_to(root.resolve())
                assert resolved.is_file()
                assert not resolved.is_symlink()


def test_backend_inventory_rejects_implementation_paths_owned_by_another_source(
    tmp_path: Path,
) -> None:
    tool = _resource_tool()
    build = tmp_path / "generated"
    receipt_path = _write_backend_fixture(build / "cpp", "cpp")
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    groups = {row["source"]["path"]: row for row in receipt["source_groups"]}
    leaf_files = groups["leaf.py"]["files"]
    design_files = groups["design_top.py"]["files"]
    leaf_index = next(i for i, path in enumerate(leaf_files) if path.endswith(".cpp"))
    design_index = next(
        i for i, path in enumerate(design_files) if path.endswith(".cpp")
    )
    leaf_files[leaf_index], design_files[design_index] = (
        design_files[design_index],
        leaf_files[leaf_index],
    )
    receipt_path.write_text(json.dumps(receipt), encoding="utf-8")

    with pytest.raises(RuntimeError) as failure:
        tool._validate_backend_inventory(build, STEMS, None)
    assert "leaf.py" in str(failure.value)
    assert "design_top.cpp" in str(failure.value)


def test_backend_inventory_rejects_duplicate_implementation_owner_paths(
    tmp_path: Path,
) -> None:
    tool = _resource_tool()
    build = tmp_path / "generated"
    receipt_path = _write_backend_fixture(build / "cpp", "cpp")
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    groups = {row["source"]["path"]: row for row in receipt["source_groups"]}
    groups["leaf.py"]["files"].append("sources/m6/leaf.hpp")
    receipt_path.write_text(json.dumps(receipt), encoding="utf-8")
    with pytest.raises(RuntimeError, match="leaf.hpp"):
        tool._validate_backend_inventory(build, STEMS, None)


def test_backend_inventory_rejects_duplicate_or_unlisted_group_paths(
    tmp_path: Path,
) -> None:
    tool = _resource_tool()
    build = tmp_path / "generated"
    receipt_path = _write_backend_fixture(build / "cpp", "cpp")
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    groups = {row["source"]["path"]: row for row in receipt["source_groups"]}
    groups["design_top.py"]["files"].append("sources/m6/leaf.cpp")
    receipt_path.write_text(json.dumps(receipt), encoding="utf-8")
    with pytest.raises(RuntimeError, match="leaf.cpp"):
        tool._validate_backend_inventory(build, STEMS, None)

    receipt_path = _write_backend_fixture(build / "cpp", "cpp")
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    groups = {row["source"]["path"]: row for row in receipt["source_groups"]}
    unlisted = "sources/m6/extra.hpp"
    (build / "cpp" / unlisted).write_text("fixture\n", encoding="utf-8")
    groups["leaf.py"]["files"].append(unlisted)
    receipt_path.write_text(json.dumps(receipt), encoding="utf-8")
    with pytest.raises(RuntimeError, match="extra.hpp"):
        tool._validate_backend_inventory(build, STEMS, None)


@pytest.mark.parametrize(
    ("target", "missing_path"),
    [
        ("cpp", "CMakeLists.txt"),
        ("verilog", "runner_bridge.sv"),
        ("verilog", "sources/m6/design_top.sv"),
    ],
)
def test_backend_inventory_rejects_missing_top_level_and_rtl_outputs(
    tmp_path: Path, target: str, missing_path: str
) -> None:
    tool = _resource_tool()
    build = tmp_path / "generated"
    for backend in ("cpp", "verilog"):
        _write_backend_fixture(build / backend, backend)
    (build / target / missing_path).unlink()

    with pytest.raises(RuntimeError, match=Path(missing_path).name):
        tool._validate_backend_inventory(build, STEMS, None)


def test_backend_inventory_rejects_escaping_group_path(tmp_path: Path) -> None:
    tool = _resource_tool()
    build = tmp_path / "generated"
    receipt_path = _write_backend_fixture(build / "cpp", "cpp")
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    receipt["files"].append({"path": "../outside.txt", "role": "source"})
    receipt_path.write_text(json.dumps(receipt), encoding="utf-8")

    with pytest.raises(RuntimeError, match="outside.txt"):
        tool._validate_backend_inventory(build, STEMS, None)


@pytest.mark.skipif(os.name != "posix", reason="symlink validation requires POSIX")
def test_backend_inventory_rejects_symlinked_receipt_output(tmp_path: Path) -> None:
    tool = _resource_tool()
    build = tmp_path / "generated"
    _write_backend_fixture(build / "cpp", "cpp")
    target = build / "cpp" / "sources/m6/leaf.source-map.json"
    target.unlink()
    target.symlink_to(build / "cpp" / "dut.h")

    with pytest.raises(RuntimeError, match="leaf.source-map.json"):
        tool._validate_backend_inventory(build, STEMS, None)


def test_backend_inventory_rejects_directory_for_declared_regular_file(
    tmp_path: Path,
) -> None:
    tool = _resource_tool()
    build = tmp_path / "generated"
    _write_backend_fixture(build / "cpp", "cpp")
    declared_directory = build / "cpp" / "CMakeLists.txt"
    declared_directory.unlink()
    declared_directory.mkdir()

    with pytest.raises(RuntimeError, match="CMakeLists.txt"):
        tool._validate_backend_inventory(build, STEMS, None)


@pytest.mark.skipif(
    os.name != "posix", reason="owned process-group sampling is POSIX-only"
)
def test_sampling_observes_owned_parent_and_child_without_leaving_child(
    tmp_path: Path,
) -> None:
    tool = _usage_tool()
    child_code = (
        "import os, sys, time; "
        "open(sys.argv[1], 'w').write(str(os.getpid())); time.sleep(0.35)"
    )
    parent_code = (
        "import subprocess, sys; "
        "child = subprocess.Popen([sys.executable, '-c', sys.argv[1], sys.argv[2]]); "
        "child.wait()"
    )
    child_pid_file = tmp_path / "child.pid"
    result = tool.run_sampled(
        [sys.executable, "-c", parent_code, child_code, str(child_pid_file)],
        cwd=tmp_path,
        env=os.environ.copy(),
        log_dir=tmp_path / "logs",
        label="owned-child",
        sample_interval_seconds=0.02,
        timeout_seconds=5,
    )

    assert result["exit_status"] == 0
    assert result["timed_out"] is False
    assert result["process_group"]["isolated_session"] is True
    assert result["process_group"]["pgid"] is not None
    rss = result["rss"]
    assert rss["available"] is True
    assert rss["unit"] == "KiB"
    assert rss["sample_count"] > 0
    _assert_finite_nonnegative(rss["peak_sampled_sum_kib"])
    _assert_finite_nonnegative(rss["peak_sampled_member_count"])
    assert rss["peak_sampled_member_count"] >= 2
    child_pid = int(child_pid_file.read_text(encoding="utf-8"))
    with pytest.raises(ProcessLookupError):
        os.kill(child_pid, 0)
    assert all(
        "shared pages" in item or "short-lived" in item
        for item in rss["limitations"][:2]
    )


@pytest.mark.skipif(
    os.name != "posix", reason="owned process-group cleanup is POSIX-only"
)
def test_timeout_kills_term_ignoring_child_after_parent_exits(tmp_path: Path) -> None:
    tool = _usage_tool()
    child_pid_file = tmp_path / "ignoring-child.pid"
    owned_pids_file = tmp_path / "owned-pids.txt"
    child_code = (
        "import os, signal, sys, time\n"
        "signal.signal(signal.SIGTERM, signal.SIG_IGN)\n"
        "open(sys.argv[1], 'w').write(str(os.getpid()))\n"
        "time.sleep(30)\n"
    )
    parent_code = (
        "import os, subprocess, sys, time\n"
        "from pathlib import Path\n"
        "child = subprocess.Popen([sys.executable, '-c', sys.argv[1], sys.argv[2]])\n"
        "child_pid_file = Path(sys.argv[2])\n"
        "deadline = time.monotonic() + 5\n"
        "while not child_pid_file.exists() and time.monotonic() < deadline: "
        "time.sleep(0.01)\n"
        "if not child_pid_file.exists(): raise SystemExit(4)\n"
        "Path(sys.argv[3]).write_text(f'{os.getpid()} {child_pid_file.read_text()}')\n"
        "time.sleep(30)\n"
    )
    result = None
    owned_pids: list[int] = []
    try:
        result = tool.run_sampled(
            [
                sys.executable,
                "-c",
                parent_code,
                child_code,
                str(child_pid_file),
                str(owned_pids_file),
            ],
            cwd=tmp_path,
            env=os.environ.copy(),
            log_dir=tmp_path / "logs",
            label="term-ignoring-child",
            sample_interval_seconds=0.02,
            timeout_seconds=2,
        )
        assert result["timed_out"] is True
        assert result["wall_seconds"] < 10
        assert result["timeout_cleanup"]["sigterm_sent_to_group"] is True
        assert result["timeout_cleanup"]["sigkill_sent_to_group"] is True
        _assert_finite_nonnegative(result["timeout_cleanup"]["cleanup_wait_seconds"])
        owned_pids = [int(value) for value in owned_pids_file.read_text().split()]
        assert len(owned_pids) == 2

        def exists(pid: int) -> bool:
            try:
                os.kill(pid, 0)
            except ProcessLookupError:
                return False
            return True

        deadline = time.monotonic() + 2
        while any(exists(pid) for pid in owned_pids) and time.monotonic() < deadline:
            time.sleep(0.05)
        assert not any(exists(pid) for pid in owned_pids), (
            f"timed-out process group left owned PIDs alive: {owned_pids}; "
            f"pgid={result['process_group']['pgid']}"
        )
    finally:
        # Contain this regression test's intentionally resistant child even
        # when the assertion detects the sampler bug.
        if not owned_pids and owned_pids_file.is_file():
            owned_pids = [int(value) for value in owned_pids_file.read_text().split()]
        if result and result["process_group"]["pgid"] is not None:
            try:
                os.killpg(result["process_group"]["pgid"], signal.SIGKILL)
            except ProcessLookupError:
                pass
        else:
            for pid in owned_pids:
                try:
                    os.kill(pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
