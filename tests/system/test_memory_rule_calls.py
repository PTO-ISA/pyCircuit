"""Rule-owned Memory calls preserve existing kernel timing and port limits."""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from test_four_state_frontend_system import _generated_system, _link, _published
from test_source_system_execution import _cli, _compile, _ok, _records, _run

pytestmark = pytest.mark.system


def _final(tmp_path: Path, name: str, source: str, providers=()) -> Path:
    unit = _published(tmp_path, name, source, providers)
    final = tmp_path / (name + ".ac")
    linked = _link((*(providers or ()), unit), f"checks.{name}.Exercise", final)
    assert linked.returncode == 0, linked.stdout + linked.stderr
    return final


def _run_both(final: Path, tmp_path: Path, *, success: bool = True):
    transcripts = []
    for target in ("cpp", "verilog"):
        runner = _generated_system(final, tmp_path / target, target)
        workers = (1, 2) if target == "cpp" else (1,)
        for worker in workers:
            arguments = [str(runner)] + (
                ["--workers", str(worker)] if target == "cpp" else []
            )
            result = _run(arguments)
            assert (result.returncode == 0) == success, result.stdout + result.stderr
            transcripts.append(_records(result.stdout))
    return transcripts


SYNC_TIMING = """from typing import Annotated
import pycircuit as ac
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0),(1,0),
                    (0,0),(1,0),(0,0),(1,0),(0,0),(1,0)))
def Exercise():
    phase: ac.u4 = 0
    memory = ac.sync_mem[ac.u16](depth=4)
    @ac.rule
    def drive():
        if phase == 1:
            memory.request(1)
            memory.write(1, 0x1234, mask=1)
        elif phase == 2:
            first = memory.result()
            second = memory.result()
            assert first == 0
            assert second == 0
            ac.log('info', 'response', first, second)
        elif phase == 3:
            memory.request(1)
            previous = memory.result()
            assert previous == 0
        elif phase >= 4:
            current = memory.result()
            assert current == 0x34
            ac.log('info', 'response', current)
        if phase == 15:
            unused = memory.result()
        phase = phase + 1
"""


def test_sync_request_result_readfirst_mask_repeat_and_first_idle_retention(
    tmp_path: Path,
) -> None:
    traces = _run_both(_final(tmp_path, "sync_timing", SYNC_TIMING), tmp_path)
    observations = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert observations[0] == observations[1] == observations[2]
    values = [[entry["value"] for entry in row["values"]] for row in observations[0]]
    assert values[:2] == [["0", "0"], ["0", "0"]]
    assert values[2:] == [["52"], ["52"], ["52"], ["52"]]


def test_sync_second_idle_edge_invalidates_response(tmp_path: Path) -> None:
    source = SYNC_TIMING.replace("(1,0)))", "(1,0),(0,0)))")
    # The thirteenth Work reads after the second idle rising edge invalidated Q.
    traces = _run_both(_final(tmp_path, "sync_expiry", source), tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert len(logs[0]) == 6


def test_initial_request_and_result_same_rule_cannot_bootstrap(tmp_path: Path) -> None:
    final = _final(
        tmp_path,
        "bootstrap",
        """import pycircuit as ac
@ac.system(samples=((0,0),(1,0)))
def Exercise():
    memory = ac.sync_mem[ac.u8](depth=4)
    @ac.rule
    def access():
        memory.request(0)
        value = memory.result()
        ac.log('info', 'unexpected', value)
""",
    )
    traces = _run_both(final, tmp_path)
    assert all(not any(row["kind"] == "log" for row in trace) for trace in traces)


def test_unavailable_result_discards_outer_rule_but_other_rules_progress(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "result_atomicity",
        """from typing import Annotated
import pycircuit as ac
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0),
                    (1,0),(0,0),(1,0),(0,0)))
def Exercise():
    phase: ac.u4 = 0
    discarded: ac.u8 = 0
    memory = ac.sync_mem[ac.u8](depth=4)
    @ac.rule
    def inhibited():
        if phase == 0:
            discarded = discarded + 1
            memory.request(1)
            memory.write(1, 99)
            unavailable = memory.result()
            ac.log('info', 'must_not_publish', unavailable)
    @ac.rule
    def producer():
        if phase == 1:
            memory.request(0)
            memory.write(0, 7)
        elif phase == 3:
            memory.request(1)
    @ac.rule
    def consumer():
        if phase == 2:
            first = memory.result()
            assert first == 0
            ac.log('info', 'first', first)
        elif phase == 4:
            untouched = memory.result()
            assert untouched == 0
            assert discarded == 0
            ac.log('info', 'atomic', untouched, discarded)
    @ac.rule
    def advance():
        if phase == 15:
            unused = memory.result()
        ac.log('info', 'heartbeat', phase)
        phase = phase + 1
""",
    )
    traces = _run_both(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    events = [row["spec"]["event"] for row in logs[0]]
    assert events == [
        "heartbeat",
        "heartbeat",
        "heartbeat",
        "heartbeat",
        "first",
        "heartbeat",
        "first",
        "heartbeat",
        "heartbeat",
        "heartbeat",
        "atomic",
        "heartbeat",
    ]
    assert "must_not_publish" not in events


def _selector_before_unavailable_result(selector: str) -> str:
    return f"""import pycircuit as ac
@ac.system(samples=((1,0),))
def Exercise():
    memory = ac.sync_mem[ac.u8](depth=4)
    marker: ac.u8 = 0
    @ac.rule
    def heartbeat():
        marker = marker + 1
        ac.log('info', 'heartbeat', 1)
    @ac.rule
    def access():
        if {selector}:
            memory.write(0, 11)
        else:
            memory.write(1, 22)
        unavailable = memory.result()
        ac.log('info', 'must_not_publish', unavailable)
"""


def test_known_memory_effect_selector_stalls_only_on_unavailable_result(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "known_selector_stall",
        _selector_before_unavailable_result("ac.four_state('1')"),
    )
    traces = _run_both(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [row["spec"]["event"] for row in logs[0]] == ["heartbeat"]
    assert logs[0][0]["values"][0] == {"kind": "bool", "value": True}


@pytest.mark.parametrize("symbol", ["x", "z"])
def test_reached_memory_effect_selector_fault_precedes_unavailable_result(
    tmp_path: Path, symbol: str
) -> None:
    final = _final(
        tmp_path,
        "unknown_selector_fault",
        _selector_before_unavailable_result(f"ac.four_state({symbol!r})"),
    )
    traces = _run_both(final, tmp_path, success=False)
    assert all(not any(row["kind"] == "log" for row in trace) for trace in traces)


def test_dual_port_requests_share_one_write_port_and_independent_results(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "dual_port",
        """from typing import Annotated
import pycircuit as ac
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0),
                    (1,0),(0,0),(1,0),(0,0)))
def Exercise():
    phase: ac.u3 = 0
    memory = ac.sync_mem_dp[ac.u8](depth=4)
    @ac.rule
    def drive():
        if phase == 1:
            memory.write(0, 11)
            memory.request(0, port=0 + 0)
            memory.request(1, port=2 - 1)
        elif phase == 3:
            memory.request(0, port=0 + 0)
            memory.request(1, port=2 - 1)
        elif phase == 4:
            assert memory.result(port=0 + 0) == 11
            assert memory.result(port=2 - 1) == 0
        phase = phase + 1
""",
    )
    _run_both(final, tmp_path)


def test_struct_memory_masked_write_and_readfirst_preserve_packed_fields(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "struct_payload",
        """from typing import Annotated
import pycircuit as ac
@ac.struct
class Packet:
    tag: ac.u5
    byte: ac.u8
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0),(1,0),
                    (0,0),(1,0),(0,0),(1,0),(0,0),(1,0),(0,0)))
def Exercise():
    phase: ac.u3 = 0
    memory = ac.sync_mem[Packet](depth=4)
    @ac.rule
    def drive():
        if phase == 1:
            memory.request(1)
            memory.write(1, Packet(tag=26, byte=0xbc), mask=1)
        elif phase == 2:
            old = memory.result()
            assert old.tag == 0
            assert old.byte == 0
        elif phase == 3:
            memory.request(1)
            memory.write(1, Packet(tag=21, byte=0x44), mask=2)
        elif phase == 4:
            low_lane = memory.result()
            assert low_lane.tag == 0
            assert low_lane.byte == 0xbc
            ac.log('info', 'struct_low', low_lane.tag, low_lane.byte)
        elif phase == 5:
            memory.request(1)
        elif phase == 6:
            merged = memory.result()
            assert merged.tag == 21
            assert merged.byte == 0xbc
            ac.log('info', 'struct_merged', merged.tag, merged.byte)
        phase = phase + 1
""",
    )
    traces = _run_both(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [row["spec"]["event"] for row in logs[0]] == [
        "struct_low",
        "struct_low",
        "struct_merged",
    ]
    assert [[value["value"] for value in row["values"]] for row in logs[0]] == [
        ["0", "188"],
        ["0", "188"],
        ["21", "188"],
    ]


def test_enum_memory_without_zero_member_uses_valid_idle_carrier(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "enum_payload",
        """from enum import Enum
from typing import Annotated
import pycircuit as ac
@ac.encoding(width=8)
class Mode(Enum):
    IDLE = 3
    RUN = 17
    DONE = 255
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0),(1,0),
                    (0,0),(1,0),(0,0),(1,0),(0,0),(1,0),(0,0)))
def Exercise():
    phase: ac.u3 = 0
    memory = ac.sync_mem[Mode](depth=4)
    @ac.rule
    def drive():
        if phase == 1:
            memory.write(0, Mode.RUN)
        elif phase == 3:
            memory.request(0)
            memory.write(0, Mode.DONE)
        elif phase == 4:
            assert memory.result() == Mode.RUN
            ac.log('info', 'enum_run', 17)
        elif phase == 5:
            memory.request(0)
        elif phase == 6:
            assert memory.result() == Mode.DONE
            ac.log('info', 'enum_done', 255)
        phase = phase + 1
""",
    )
    traces = _run_both(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [row["spec"]["event"] for row in logs[0]] == [
        "enum_run",
        "enum_run",
        "enum_done",
    ]
    assert [row["values"][0]["value"] for row in logs[0]] == ["17", "17", "255"]


def test_byte_memory_async_read_observes_only_committed_masked_write(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "byte_read",
        """from typing import Annotated
import pycircuit as ac
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0)))
def Exercise():
    phase: ac.u4 = 0
    memory = ac.byte_mem[ac.u16](depth=8)
    alias = memory
    @ac.rule
    def drive():
        if phase <= 2:
            before = memory.read(2)
            if phase == 1 and before == 0:
                memory.write(2, 0x1234, mask=1)
            elif phase == 2:
                repeated = alias.read(2)
                if before == 0x34 and repeated == before:
                    ac.log('info', 'byte_read', before, repeated)
        elif phase == 15:
            inactive = memory.read(ac.four_state('xxx'))
            ac.log('info', 'inactive', inactive)
        phase = phase + 1
""",
    )
    traces = _run_both(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [row["spec"]["event"] for row in logs[0]] == ["byte_read"]


def test_byte_read_dependent_write_direct_and_cross_module(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "byte_feedback",
        """import pycircuit as ac
@ac.module
def Bank():
    memory = ac.byte_mem[ac.u16](depth=8)
    @ac.method
    def update(address: ac.u3, value: ac.u16) -> ac.u16:
        current = memory.read(address)
        if current == 0:
            memory.write(address, value)
        return current
    @ac.method
    def read(address: ac.u3) -> ac.u16:
        return memory.read(address)
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0)))
def Exercise():
    phase: ac.u2 = 0
    direct = ac.byte_mem[ac.u16](depth=8)
    bank = Bank()
    @ac.rule
    def drive():
        if phase == 1:
            current = direct.read(0)
            if current == 0:
                direct.write(0, 0x1234)
            assert bank.update(1, 0x5678) == 0
        elif phase == 2:
            direct_value = direct.read(0)
            child_value = bank.read(1)
            assert direct_value == 0x1234
            assert child_value == 0x5678
            ac.log('info', 'byte_feedback', direct_value, child_value)
        phase = phase + 1
""",
    )
    traces = _run_both(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [row["spec"]["event"] for row in logs[0]] == ["byte_feedback"]
    assert [value["value"] for value in logs[0][0]["values"]] == ["4660", "22136"]


def test_unused_byte_read_with_unknown_address_faults_and_discards_root(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "unused_byte_read",
        """import pycircuit as ac
@ac.system(samples=((1,0),))
def Exercise():
    memory = ac.byte_mem[ac.u16](depth=8)
    marker: ac.u1 = 0
    @ac.rule
    def heartbeat():
        marker = 1
        ac.log('info', 'must_not_publish', marker)
    @ac.rule
    def fault():
        unused = memory.read(ac.four_state('xxx'))
""",
    )
    traces = _run_both(final, tmp_path, success=False)
    assert all(not any(row["kind"] == "log" for row in trace) for trace in traces)


def test_known_oob_sync_read_is_zero_and_write_is_noop(tmp_path: Path) -> None:
    final = _final(
        tmp_path,
        "oob",
        """from typing import Annotated
import pycircuit as ac
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0)))
def Exercise():
    phase: ac.u2 = 0
    memory = ac.sync_mem[ac.u8](depth=3)
    @ac.rule
    def drive():
        if phase == 1:
            memory.request(3)
            memory.write(3, 99)
        elif phase == 2:
            assert memory.result() == 0
        phase = phase + 1
""",
    )
    _run_both(final, tmp_path)


def test_reset_invalidates_response_but_retains_ram(tmp_path: Path) -> None:
    final = _final(
        tmp_path,
        "reset_retains",
        """from typing import Annotated
import pycircuit as ac
@ac.system(samples=((0,0),(1,0),(0,0),(1,0),(0,0),(1,1),
                    (0,0),(1,0),(0,0),(1,0),(0,0)))
def Exercise():
    phase: ac.u2 = 0
    memory = ac.sync_mem[ac.u8](depth=4)
    @ac.rule
    def drive():
        if phase == 1:
            memory.request(1)
            memory.write(1, 60)
        elif phase == 2:
            ac.log('info', 'after_edge', memory.result())
        phase = phase + 1
""",
    )
    traces = _run_both(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [row["values"][0]["value"] for row in logs[0]] == ["0", "0", "60"]


def test_imported_memory_provider_header_preserves_call_contract(
    tmp_path: Path,
) -> None:
    provider = _published(
        tmp_path,
        "provider",
        """import pycircuit as ac
@ac.module
def Cell(address: ac.u2) -> ac.u8:
    memory = ac.sync_mem[ac.u8](depth=4)
    @ac.rule
    def access() -> ac.u8:
        memory.request(address)
        return memory.result()
    return access
""",
    )
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden_body = tmp_path / "hidden provider body.ac"
    (tmp_path / "provider-source" / "provider.py").unlink()
    body.rename(hidden_body)
    root = tmp_path / "consumer-source"
    root.mkdir()
    path = root / "consumer.py"
    path.write_text(
        """import pycircuit as ac
from checks.provider import Cell
@ac.system(samples=((0,0),(1,0)))
def Exercise():
    value = Cell(0)
    @ac.rule
    def observe():
        if ac.is_known(value): ac.log('info', 'value', value)
""",
        encoding="utf-8",
    )
    consumer = tmp_path / "consumer-unit"
    try:
        _ok(_compile(root, path.name, consumer, [provider]))
        assert not body.exists()
    finally:
        hidden_body.rename(body)
    final = tmp_path / "imported.ac"
    linked = _link((provider, consumer), "checks.consumer.Exercise", final)
    assert linked.returncode == 0, linked.stdout + linked.stderr


CONFLICTS = {
    "alias-read-port": (
        "sync_mem",
        "memory.request(0)\n        alias.request(1)",
    ),
    "alias-write-port": (
        "sync_mem",
        "memory.write(0, 1)\n        alias.write(0, 2)",
    ),
    "byte-read-port": (
        "byte_mem",
        "first = memory.read(0)\n        second = alias.read(1)\n        assert first == second",
    ),
    "same-read-port": (
        "sync_mem",
        "memory.request(0)\n        memory.request(1)",
    ),
    "same-dp-port": (
        "sync_mem_dp",
        "memory.request(0, port=1)\n        memory.request(1, port=1)",
    ),
    "write-port": (
        "sync_mem",
        "memory.write(0, 1)\n        memory.write(1, 2)",
    ),
}


@pytest.mark.parametrize("case", sorted(CONFLICTS))
def test_memory_port_alias_conflicts_reject(tmp_path: Path, case: str) -> None:
    allocation, calls = CONFLICTS[case]
    source = f"""import pycircuit as ac
@ac.system(samples=((1,0),))
def Exercise():
    memory = ac.{allocation}[ac.u8](depth=4)
    alias = memory
    @ac.rule
    def conflict():
        {calls}
"""
    root = tmp_path / "source"
    root.mkdir()
    path = root / "conflict.py"
    path.write_text(source, encoding="utf-8")
    unit = tmp_path / "unit"
    path.write_text(
        """import pycircuit as ac
@ac.system(samples=((1,0),))
def Exercise():
    memory = ac.sync_mem[ac.u8](depth=4)
    @ac.rule
    def valid(): memory.request(0)
""",
        encoding="utf-8",
    )
    _ok(_compile(root, path.name, unit))
    final = tmp_path / "protected.ac"
    _ok(_link((unit,), "checks.conflict.Exercise", final))
    before = final.read_bytes()
    path.write_text(source, encoding="utf-8")
    compiled = _cli(
        "compile",
        "-c",
        str(path),
        "--source-root",
        str(root),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    )
    _ok(compiled)
    linked = _link((unit,), "checks.conflict.Exercise", final, replace=True)
    assert linked.returncode != 0, linked.stdout + linked.stderr
    assert (
        "Memory port conflict: potentially simultaneous same-port accesses"
        in linked.stderr
    ), linked.stderr
    assert final.read_bytes() == before


PORT_REJECTIONS = {
    "boolean": """import pycircuit as ac
@ac.module
def Top() -> ac.u8:
    memory = ac.sync_mem_dp[ac.u8](depth=4)
    @ac.rule
    def access() -> ac.u8:
        memory.request(0, port=True)
        return 0
    return access
""",
    "dual-out-of-range": """import pycircuit as ac
@ac.module
def Top() -> ac.u8:
    memory = ac.sync_mem_dp[ac.u8](depth=4)
    @ac.rule
    def access() -> ac.u8:
        memory.request(0, port=2)
        return 0
    return access
""",
    "runtime": """from typing import Annotated
import pycircuit as ac
@ac.module
def Top(port: Annotated[int, range(2)]) -> ac.u8:
    memory = ac.sync_mem_dp[ac.u8](depth=4)
    @ac.rule
    def access() -> ac.u8:
        memory.request(0, port=port)
        return 0
    return access
""",
    "single-out-of-range": """import pycircuit as ac
@ac.module
def Top() -> ac.u8:
    memory = ac.sync_mem[ac.u8](depth=4)
    @ac.rule
    def access() -> ac.u8:
        memory.request(0, port=1)
        return 0
    return access
""",
}


@pytest.mark.parametrize("case", sorted(PORT_REJECTIONS))
def test_memory_port_requires_in_range_closed_static_integer(
    tmp_path: Path, case: str
) -> None:
    root = tmp_path / "source"
    root.mkdir()
    path = root / "port.py"
    path.write_text(
        """import pycircuit as ac
@ac.module
def Top() -> ac.u8:
    memory = ac.sync_mem[ac.u8](depth=4)
    @ac.rule
    def access() -> ac.u8:
        memory.request(0, port=0)
        return 0
    return access
""",
        encoding="utf-8",
    )
    unit = tmp_path / "unit"
    _ok(_compile(root, path.name, unit))
    before = {item.name: item.read_bytes() for item in unit.iterdir() if item.is_file()}
    path.write_text(PORT_REJECTIONS[case], encoding="utf-8")
    result = _cli(
        "compile",
        "-c",
        str(path),
        "--source-root",
        str(root),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    )
    assert result.returncode != 0, result.stdout + result.stderr
    assert any(
        marker in result.stderr
        for marker in ("port", "static", "Integer", "range", "Boolean")
    ), result.stderr
    assert before == {
        item.name: item.read_bytes() for item in unit.iterdir() if item.is_file()
    }


def test_memory_write_sibling_fault_publishes_no_observations(tmp_path: Path) -> None:
    final = _final(
        tmp_path,
        "rollback",
        """import pycircuit as ac
@ac.system(samples=((1,0),))
def Exercise():
    memory = ac.sync_mem[ac.u8](depth=4)
    marker: ac.u1 = 0
    @ac.rule
    def write():
        memory.write(1, 85)
        ac.log('info', 'pending_write', 85)
    @ac.rule(guard=lambda: ac.four_state('x'))
    def fail(): marker = 1
""",
    )
    traces = _run_both(final, tmp_path, success=False)
    assert all(not any(row["kind"] == "log" for row in trace) for trace in traces)
