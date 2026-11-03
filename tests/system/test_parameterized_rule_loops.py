"""Static Module parameters bind Rule/Method loop bounds per instance.

Providers are published independently and their Python source is removed before
consumer compilation. Generated C++ and Verilog each execute three cycles and
are checked against test-owned integer expectations, including both Work samples
per cycle. No expected result is derived from the other backend.
"""

from __future__ import annotations

import json
import os
from pathlib import Path

import pytest
from test_scalar_generic_modules import _link, _parameters, _write
from test_source_system_execution import _bundle, _compile, _ok, _records, _run

pytestmark = pytest.mark.system


RULE_PROVIDER = """import pycircuit as ac

@ac.module
def Scan(value: ac.u8, *, SLOTS: int = 3) -> ac.u8:
    @ac.rule
    def accumulate() -> ac.u8:
        total = value
        for index in range(SLOTS):
            total = total + 2
        return total
    return accumulate

@ac.module
def Retained(value: ac.u8) -> ac.u8:
    return Scan(value, SLOTS=1)

@ac.module
def Required(value: ac.u8, *, SLOTS: int) -> ac.u8:
    @ac.rule
    def accumulate() -> ac.u8:
        total = value
        for index in range(SLOTS):
            total = total + 3
        return total
    return accumulate

@ac.module
def Empty(*, SLOTS: int = 0) -> ac.u8:
    @ac.rule
    def preserve() -> ac.u8:
        index: ac.u8 = 17
        for index in range(SLOTS):
            index = unresolved_dead_body
        else:
            index = index + 1
        return index
    return preserve
"""


CHECK_PROVIDER = """import pycircuit as ac

@ac.module
def Checks():
    @ac.method
    def require(available: bool) -> None:
        assert available
"""


METHOD_PROVIDER = """import pycircuit as ac
from checks.support import Checks

@ac.module
def Bank(*, SLOTS: int = 3, ROWS: int = SLOTS + 1):
    checks = Checks()
    entries: ac.table[ROWS, ac.u8] = 0

    @ac.method
    def update(delta: ac.u8) -> ac.u8:
        checks.require(delta > 0)
        for index in range(SLOTS):
            entries[index] = entries[index] + delta
        total: ac.u8 = 0
        for index in range(SLOTS):
            total = total + entries[index]
        return total

    @ac.method
    def read() -> ac.u8:
        total: ac.u8 = 0
        for index in range(SLOTS):
            total = total + entries[index]
        return total

@ac.module
def Forward(*, SLOTS: int):
    bank = Bank(SLOTS=SLOTS)

    @ac.method
    def update(delta: ac.u8) -> ac.u8:
        return bank.update(delta)

    @ac.method
    def read() -> ac.u8:
        return bank.read()
"""


def _imported_system(
    tmp_path: Path, provider_text: str, consumer_text: str, support_text: str = ""
) -> Path:
    source = tmp_path / "source"
    source.mkdir()
    dependencies = []
    if support_text:
        support_source = _write(source, "support.py", support_text)
        support = tmp_path / "support"
        _ok(_compile(source, "support.py", support))
        support_source.unlink()
        dependencies.append(support)
    provider_source = _write(source, "provider.py", provider_text)
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider, dependencies))
    dependencies.append(provider)
    provider_source.unlink()
    _write(source, "bench.py", consumer_text)
    consumer = tmp_path / "consumer"
    _ok(_compile(source, "bench.py", consumer, dependencies))
    final = tmp_path / "system.ac"
    _ok(_link([*dependencies, consumer], "checks.bench.Exercise", final))
    return final


def _check_execution(final: Path, tmp_path: Path, expected: list[tuple]) -> None:
    for target in ("cpp", "verilog"):
        directory = tmp_path / target
        directory.mkdir()
        binary = _bundle(final, directory, target)
        arguments = (
            ["--cycles", "3", "--workers", "1"] if target == "cpp" else ["+cycles=3"]
        )
        rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
        logs = [row for row in rows if row["kind"] == "log"]
        assert [row["spec"]["event"] for row in logs] == ["values"] * 6
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in logs
        ] == expected
        assert [int(row["evaluation_epoch"]) for row in logs] == list(range(6))
        terminal = [row for row in rows if row["kind"] == "result"]
        assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"


def test_imported_nested_rule_parameter_bounds(tmp_path: Path) -> None:
    final = _imported_system(
        tmp_path,
        RULE_PROVIDER,
        """import pycircuit as ac
from checks.provider import Scan, Required, Empty

@ac.system
def Exercise():
    default = Scan(10)
    nondefault = Scan(10, SLOTS=2)
    zero = Scan(10, SLOTS=0)
    required = Required(10, SLOTS=4)
    empty = Empty()

    @ac.rule
    def observe() -> None:
        ac.log("info", "values", default, nondefault, zero, required, empty)
""",
    )
    source = tmp_path / "source"
    assert not (source / "provider.py").exists()
    _write(source, "reexport.py", "from checks.provider import Scan\n")
    _ok(_compile(source, "reexport.py", tmp_path / "reexport", [tmp_path / "provider"]))
    # Inputs are constant: 10 + 2*N, 10 + 3*4, and the preserved 17 + else 1.
    _check_execution(final, tmp_path, [(16, 14, 10, 22, 18)] * 6)


def test_imported_parameterized_method_table_candidates(tmp_path: Path) -> None:
    final = _imported_system(
        tmp_path,
        METHOD_PROVIDER,
        """import pycircuit as ac
from checks.provider import Bank, Forward

@ac.system
def Exercise():
    left = Bank()
    peer = Bank(SLOTS=3)
    right = Forward(SLOTS=2)
    empty = Forward(SLOTS=0)

    @ac.rule
    def update() -> None:
        before = left.read()
        first = left.update(1)
        second = left.update(2)
        same_size = peer.update(4)
        other = right.update(5)
        no_change = empty.update(7)
        ac.log("info", "values", before, first, second, same_size, other, no_change,
               left.read(), peer.read(), right.read(), empty.read())
""",
        support_text=CHECK_PROVIDER,
    )
    # Left has three active entries, increased by 1 then 2 in one transaction.
    # Peer has the same bound but separate state, increased by 4 per entry.
    # Right has two active entries increased by 5. Empty's one stored row stays 0.
    expected = [
        (
            9 * cycle,
            9 * cycle + 3,
            9 * cycle + 9,
            12 * (cycle + 1),
            10 * (cycle + 1),
            0,
            9 * cycle + 9,
            12 * (cycle + 1),
            10 * (cycle + 1),
            0,
        )
        for cycle in range(3)
        for _ in range(2)
    ]
    _check_execution(final, tmp_path, expected)


def test_negative_parameterized_loop_preserves_final_output(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    _write(source, "provider.py", RULE_PROVIDER)
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider))
    final = tmp_path / "scan.ac"
    _ok(
        _link(
            [provider],
            "checks.provider.Scan",
            final,
            _parameters(tmp_path / "positive.json", SLOTS=2),
        )
    )
    payload = json.loads(
        _ok(_run([os.environ["PYCIRCUIT_EMITTER"], str(final), "--target", "cpp"]))
    )
    assert payload["entry"]["definition"] == '@"checks.provider.Scan"'
    before = final.read_bytes()
    result = _link(
        [provider],
        "checks.provider.Scan",
        final,
        _parameters(tmp_path / "parameters.json", SLOTS=-1),
        replace=True,
    )
    assert result.returncode != 0, "a negative static loop bound must be rejected"
    assert final.read_bytes() == before


def test_parameterized_provider_pair_mismatch_preserves_final_output(tmp_path: Path):
    source = tmp_path / "source"
    source.mkdir()
    provider_source = _write(source, "provider.py", RULE_PROVIDER)
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider))
    final = tmp_path / "scan.ac"
    _ok(_link([provider], "checks.provider.Scan", final))
    before = final.read_bytes()

    # Compile both declarations normally under the same public source owner.
    # Mix only their published bodies/headers; no IR spelling is assumed here.
    provider_source.write_text(
        RULE_PROVIDER.replace("SLOTS: int = 3", "SLOTS: int = 2"), encoding="utf-8"
    )
    changed = tmp_path / "changed-provider"
    _ok(_compile(source, "provider.py", changed))
    original_files = json.loads((provider / "unit.json").read_text())["files"]
    changed_files = json.loads((changed / "unit.json").read_text())["files"]
    body = provider / original_files["body"]
    header = provider / original_files["interface"]
    different_body = (changed / changed_files["body"]).read_bytes()
    assert body.read_bytes() != different_body
    body.write_bytes(different_body)
    provider_source.unlink()

    # Native pair verification bypasses publication hashes and must reject the
    # conflicting declaration, before public replacement is attempted.
    owner_report = tmp_path / "owner.json"
    pair = _run(
        [
            os.environ["PYCIRCUIT_LINKER"],
            "--body",
            str(body),
            "--header",
            str(header),
            "--verify-only",
            "--unit-owner-out",
            str(owner_report),
        ]
    )
    assert pair.returncode != 0
    assert not owner_report.exists()
    result = _link([provider], "checks.provider.Scan", final, replace=True)
    assert result.returncode != 0
    assert final.read_bytes() == before
