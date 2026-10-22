"""Independent execution oracles for sibling references and callable order.

Expected values follow whole-transaction fire and each state's write intent;
they are never recovered from compiler output or generated signal expressions.
"""

from __future__ import annotations

import hashlib
import json
import re
import shutil
from pathlib import Path

import pytest
from test_saved_anchor import _tree
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _install,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system
FIXTURE = Path(__file__).parent / "fixtures/sibling_calls"
TOP = "checks.bench.ExerciseSiblings"


def _units(
    tmp_path, *, scenario="ordinary", filenames=("counter.py", "clients.py", "bench.py")
):
    source = tmp_path / "source"
    shutil.copytree(FIXTURE, source)
    if scenario == "total-return":
        (source / "counter.py").write_text(
            "import pycircuit as ac\n@ac.module\ndef Counter():\n"
            "    count: ac.u8 = 5\n"
            "    @ac.method\n    def old() -> ac.u8:\n        return count\n"
        )
        (source / "clients.py").write_text(
            "import pycircuit as ac\nfrom checks.counter import Counter\n"
            "@ac.module\ndef Reader(counter: Counter) -> ac.u8:\n"
            "    @ac.rule\n    def read() -> ac.u8:\n"
            "        return counter.old()\n"
            "    return read()\n"
        )
        (source / "bench.py").write_text(
            "import pycircuit as ac\nfrom checks.counter import Counter\n"
            "from checks.clients import Reader\n"
            "@ac.system\ndef ExerciseSiblings():\n"
            "    counter = Counter()\n    value = Reader(counter)\n"
            "    @ac.rule\n    def observe() -> None:\n"
            '        ac.log("info", "total returned", value)\n'
            "    observe()\n"
        )
    elif scenario == "methods":
        path = source / "counter.py"
        text = path.read_text().replace("@ac.rule(", "@ac.method(")
        assert "@ac.rule(" not in text
        path.write_text(text)
    elif scenario in ("transitive", "pass-through"):
        path = source / "counter.py"
        text = path.read_text()
        text = text.replace(
            "    @ac.rule(after=(increment,))\n    def read()",
            "    @ac.rule(after=(increment,))\n"
            + (
                "    def bridge() -> ac.u8:\n        return count\n\n"
                if scenario == "pass-through"
                else "    def bridge() -> None:\n        pass\n\n"
            )
            + "    @ac.rule(after=(bridge,))\n    def read()",
        )
        assert "def bridge()" in text and "after=(bridge,)" in text
        path.write_text(text)
        if scenario == "pass-through":
            path = source / "clients.py"
            path.write_text(
                path.read_text() + "\n@ac.module\ndef Middle(counter: Counter):\n"
                "    @ac.rule\n    def observe() -> None:\n"
                "        value = counter.bridge()\n"
                '        ac.log("info", "middle visible", value)\n'
                "    observe()\n"
            )
            path = source / "bench.py"
            path.write_text(
                path.read_text()
                .replace("import Relay, Writer", "import Middle, Relay, Writer")
                .replace(
                    "    Relay(counter, ledger)\n",
                    "    Relay(counter, ledger)\n    Middle(counter)\n",
                )
            )
    elif scenario == "returned":
        path = source / "clients.py"
        text = path.read_text()
        signature = (
            "    counter: Counter, ledger: Ledger, allow: ac.u1, change_marker: ac.u1\n):"
        )
        assert text.count(signature) == 1
        text = text.replace(signature, signature.removesuffix(":") + " -> ac.u8:")
        text = text.replace("    def write() -> None:", "    def write() -> ac.u8:")
        text = text.replace(
            '        ac.log("info", "sibling written", alias.old())',
            '        ac.log("info", "sibling written", alias.old())\n        return alias.old()',
        )
        text = text.replace("    write()\n", "    return write()\n")
        text = text.replace(
            "def Reader(counter: Counter, ledger: Ledger):",
            "def Reader(counter: Counter, ledger: Ledger) -> ac.u8:",
        )
        text = text.replace("    def observe() -> None:", "    def observe() -> ac.u8:")
        text = text.replace(
            '        ac.log("info", "sibling visible", value, balance, marker)',
            '        ac.log("info", "sibling visible", value, balance, marker)\n        return value',
        )
        text = text.replace(
            "    observe()\n    observe_six()\n    observe_balance()",
            "    observe_six()\n    observe_balance()\n    return observe()",
        )
        text = text.replace(
            "def Relay(counter: Counter, ledger: Ledger):",
            "def Relay(counter: Counter, ledger: Ledger) -> ac.u8:",
        )
        text = text.replace(
            "    Reader(alias, ledger)", "    return Reader(alias, ledger)"
        )
        path.write_text(text)
        path = source / "bench.py"
        text = path.read_text().replace(
            "    Writer(counter, ledger,", "    written = Writer(counter, ledger,"
        )
        text = text.replace(
            "    Relay(counter, ledger)", "    visible = Relay(counter, ledger)"
        )
        text += "\n    @ac.rule\n    def publish_written() -> None:\n"
        text += '        ac.log("info", "sibling returned writer", written)\n'
        text += "\n    @ac.rule\n    def publish_visible() -> None:\n"
        text += '        ac.log("info", "sibling returned reader", visible)\n'
        text += "\n    publish_written()\n    publish_visible()\n"
        path.write_text(text)
    elif scenario == "repeated":
        path = source / "clients.py"
        path.write_text(
            path.read_text().replace(
                "        alias.increment()\n",
                "        alias.increment()\n        first = alias.old()\n"
                "        alias.increment()\n        second = alias.old()\n"
                '        ac.log("info", "repeated snapshots", first, second)\n',
            )
        )
    elif scenario == "conditional":
        path = source / "clients.py"
        path.write_text(
            path.read_text().replace(
                "        alias.increment()",
                "        if change_marker:\n            alias.increment()",
            )
        )
        path = source / "bench.py"
        path.write_text(path.read_text().replace("phase != 1", "1"))
    elif scenario == "two-instances":
        path = source / "bench.py"
        path.write_text(
            path.read_text().replace(
                "    Relay(counter, ledger)\n",
                "    Relay(counter, ledger)\n"
                "    held_ledger = Ledger()\n"
                "    held_writer = Writer(held, held_ledger, 0, 0)\n"
                "    held_reader = Relay(held, held_ledger)\n",
            )
        )
    units = []
    for filename in filenames:
        unit = tmp_path / (Path(filename).stem + "-unit")
        # The current source can import interfaces only. Prior provider Python
        # has been removed, and all prior provider bodies are temporarily absent.
        hidden = []
        for provider in units:
            receipt = json.loads((provider / "unit.json").read_text())
            body = provider / receipt["files"]["body"]
            unavailable = tmp_path / (provider.name + "-hidden.ac")
            body.rename(unavailable)
            hidden.append((body, unavailable))
        try:
            _ok(_compile(source, filename, unit, units))
        finally:
            for body, unavailable in hidden:
                unavailable.rename(body)
        (source / filename).unlink()
        units.append(unit)
    return source, units


def _final(tmp_path, *, saved=False, scenario="ordinary"):
    source, units = _units(tmp_path, scenario=scenario)
    final = tmp_path / "final.ac"
    unit_bytes = [_tree(unit) for unit in units]
    arguments = ["link", *(str(unit) for unit in units), "--top", TOP]
    if saved:
        anchor = tmp_path / "published-anchor"
        duplicate = tmp_path / "duplicate-anchor"
        _ok(_cli(*arguments, "--target", "anchor", "-o", str(anchor)))
        _ok(
            _cli(
                "link",
                *(str(unit) for unit in reversed(units)),
                "--top",
                TOP,
                "--target",
                "anchor",
                "-o",
                str(duplicate),
            )
        )
        assert _tree(anchor) == _tree(duplicate)
        assert [_tree(unit) for unit in units] == unit_bytes
        for directory in (source, *units):
            shutil.rmtree(directory)
        relocated = tmp_path / "relocated" / "anchor"
        shutil.copytree(anchor, relocated)
        shutil.rmtree(anchor)
        before = _tree(relocated)
        _ok(_cli("link", "--from-anchor", str(relocated), "--verify-only"))
        _ok(
            _cli(
                "link",
                "--from-anchor",
                str(relocated),
                "--target",
                "final",
                "-o",
                str(final),
            )
        )
        assert _tree(relocated) == before
    else:
        _ok(_cli(*arguments, "-o", str(final)))
        assert [_tree(unit) for unit in units] == unit_bytes
        for directory in (source, *units):
            shutil.rmtree(directory)
    assert all(not directory.exists() for directory in (source, *units))
    return final


def _events(rows, name):
    return [
        row for row in rows if row["kind"] == "log" and row["spec"]["event"] == name
    ]


def _values(rows, name):
    return [
        tuple(int(value["value"]) for value in row["values"])
        for row in _events(rows, name)
    ]


def _expected(scenario):
    if scenario == "returned":
        return {
            **_expected("ordinary"),
            "sibling returned writer": [(6,), (7,)],
            "sibling returned reader": [(6,), (6,), (7,), (7,)],
        }
    if scenario == "repeated":
        return {
            "sibling visible": [(7, 50, 91)] * 4,
            "sibling six": [],
            "sibling old": [(5, 40, 90, 5)] + [(7, 50, 91, 5)] * 3,
            "sibling written": [(7,)],
            "sibling fifty": [(7, 91)] * 4,
            "repeated snapshots": [(6, 7)],
        }
    if scenario == "pass-through":
        return {**_expected("ordinary"), "middle visible": [(6,), (6,), (7,), (7,)]}
    if scenario == "conditional":
        return {
            "sibling visible": [(6, 50, 91), (6, 60, 91), (6, 70, 91), (6, 80, 91)],
            "sibling six": [(6, 50, 91), (6, 60, 91), (6, 70, 91), (6, 80, 91)],
            "sibling old": [
                (5, 40, 90, 5),
                (6, 50, 91, 5),
                (6, 60, 91, 5),
                (6, 70, 91, 5),
            ],
            "sibling written": [(6,), (6,), (6,), (6,)],
            "sibling fifty": [(6, 91)],
        }
    return {
        "sibling visible": [(6, 50, 91), (6, 50, 91), (7, 60, 91), (7, 60, 91)],
        "sibling six": [(6, 50, 91), (6, 50, 91)],
        "sibling old": [(5, 40, 90, 5), (6, 50, 91, 5), (6, 50, 91, 5), (7, 60, 91, 5)],
        "sibling written": [(6,), (7,)],
        "sibling fifty": [(6, 91), (6, 91)],
    }


@pytest.mark.parametrize(
    "saved,scenario",
    [
        (False, "ordinary"),
        (True, "ordinary"),
        (False, "transitive"),
        (False, "conditional"),
        (False, "two-instances"),
        (False, "repeated"),
        (False, "pass-through"),
        (False, "returned"),
        (True, "returned"),
        (True, "methods"),
    ],
    ids=[
        "direct",
        "portable-anchor",
        "absent-transitive-call",
        "unreached-static-edge",
        "distinct-same-type-actuals",
        "repeated-call-one-transaction",
        "nonwriting-middle-transaction",
        "public-return-availability",
        "saved-return-availability",
        "method-declaration-after",
    ],
)
def test_sibling_calls_forward_atomic_candidates_and_keep_instance_identity(
    tmp_path,
    saved,
    scenario,
):
    final = _final(tmp_path, saved=saved, scenario=scenario)
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        root = tmp_path / target
        root.mkdir()
        binary = _bundle(final, root, target)
        if target == "verilog":
            # The generated closed-System runner is the four-state Icarus lane.
            cache = (root / "build-verilog/CMakeCache.txt").read_text()
            assert "iverilog" in cache.lower()
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            observed = rows
            if scenario == "two-instances":
                held_rows = [
                    row for row in rows if "held_reader" in row.get("instance", "")
                ]
                assert _values(held_rows, "sibling visible") == [(5, 40, 90)] * 8
                assert not _events(held_rows, "sibling six")
                assert not _events(held_rows, "sibling fifty")
                observed = [
                    row for row in rows if "held_reader" not in row.get("instance", "")
                ]
            for name, expected in _expected(scenario).items():
                assert _values(observed, name) == [
                    value for value in expected for _ in range(2)
                ]
            assert [
                int(row["evaluation_epoch"])
                for row in _events(observed, "sibling visible")
            ] == list(range(8))
            transcript = [row for row in rows if row["kind"] != "result"]
            transcripts.append(transcript)
            arguments = (
                ["--cycles", "5", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=5"]
            )
            failed = _run([str(binary), *arguments], timeout=30)
            assert failed.returncode != 0, failed.stdout + failed.stderr
            failed_rows = _records(failed.stdout)
            assert [row for row in failed_rows if row["kind"] != "result"] == transcript
            terminal = [row for row in failed_rows if row["kind"] == "result"]
            if terminal:
                assert len(terminal) == 1 and terminal[0]["status"] == "FAILED"
                assert int(terminal[0]["epoch_time"]) == 8
            else:
                assert "check failed at epoch 8" in failed.stdout + failed.stderr
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_total_lifted_return_survives_saved_reload_without_dynamic_availability(
    tmp_path,
):
    # No guarded public result or writer exists in this design. Total returns
    # still require authenticated transport of their internal transaction fire.
    final = _final(tmp_path, saved=True, scenario="total-return")
    before = final.read_bytes()
    transcripts = []
    for target in ("cpp", "verilog"):
        root = tmp_path / target
        root.mkdir()
        binary = _bundle(final, root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "3", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=3"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            assert _values(rows, "total returned") == [(5,)] * 6
            assert [
                int(row["evaluation_epoch"]) for row in _events(rows, "total returned")
            ] == list(range(6))
            transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert final.read_bytes() == before


@pytest.mark.parametrize("case", ["reversed", "same-method", "cycle"])
def test_illegal_callable_order_rejects_without_replacing_program(tmp_path, case):
    source = tmp_path / "source"
    source.mkdir()
    counter = (FIXTURE / "counter.py").read_text()
    body = {
        "reversed": "        value = counter.read()\n        counter.increment()\n",
        "same-method": "        counter.increment()\n",
        "cycle": "        counter.increment()\n",
    }[case]
    if case == "cycle":
        counter = counter.replace(
            "@ac.rule(guard=lambda: count < 7)",
            "@ac.rule(after=(read,), guard=lambda: count < 7)",
        )
    top = (
        "\n@ac.system\ndef Bad():\n    counter = Counter()\n    @ac.rule\n    def first() -> None:\n"
        + body
        + "    first()\n"
    )
    if case == "same-method":
        top += "    @ac.rule\n    def second() -> None:\n        counter.increment()\n    second()\n"
    path = source / "bad.py"
    # Publish a valid program first to bind the protection oracle.
    path.write_text(
        "import pycircuit as ac\n@ac.system\ndef Good():\n    'Closed test System.'\n"
    )
    unit = tmp_path / "unit"
    _ok(_compile(source, path.name, unit))
    final = tmp_path / "preserved.ac"
    _ok(_cli("link", str(unit), "--top", "checks.bad.Good", "-o", str(final)))
    before = final.read_bytes()
    path.write_text(counter + top)
    candidate = tmp_path / "candidate"
    compiled = _compile(source, path.name, candidate)
    if compiled.returncode == 0:
        rejected = _cli(
            "link",
            str(candidate),
            "--top",
            "checks.bad.Bad",
            "-o",
            str(final),
            "--replace",
        )
    else:
        rejected = compiled
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert final.read_bytes() == before
    reasons = {
        "reversed": ("order", "reversed", "predecessor"),
        "same-method": ("conflict", "writer"),
        "cycle": ("cycl",),
    }
    assert any(
        word in rejected.stderr.lower() for word in reasons[case]
    ), rejected.stderr


def test_four_state_sibling_candidates_survive_held_and_falling_samples(tmp_path):
    source, units = _units(
        tmp_path, filenames=("bits.py", "bits_clients.py", "bits_bench.py")
    )
    final = tmp_path / "bits.ac"
    _ok(
        _cli(
            "link",
            *(str(unit) for unit in units),
            "--top",
            "checks.bits_bench.ExerciseBits",
            "-o",
            str(final),
        )
    )
    for directory in (source, *units):
        shutil.rmtree(directory)
    transcripts = []
    for target in ("cpp", "verilog"):
        root = tmp_path / target
        root.mkdir()
        binary = _bundle(final, root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = ["--workers", str(workers)] if target == "cpp" else []
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            forwarded = _events(rows, "bits forwarded")
            old = _events(rows, "bits old")
            assert len(forwarded) == len(old) == 5
            # Compare the complete encoded four-state value, including X/Z.
            expected = {
                "kind": "bits4",
                "width": 4,
                "value": "8",
                "known": "c",
                "z": "1",
            }
            assert all(row["values"][0] == expected for row in forwarded)
            assert all(row["values"][0] == expected for row in old[2:])
            assert (
                old[0]["values"][0]
                == old[1]["values"][0]
                == {"kind": "integer", "value": "0"}
            )
            assert [int(row["evaluation_epoch"]) for row in forwarded] == list(range(5))
            transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


def test_sibling_system_reset_discard_and_failed_check_leave_no_partial_commit(
    tmp_path,
):
    final = _final(tmp_path)
    generated = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(generated)))
    # Access instrumentation is test-owned and never supplies any DUT result.
    for header in generated.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    (generated / "sibling_probe.cpp").write_text(
        r"""
#include "pycircuit_system.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit bit(bool v) { return Bit::known(gfsim::Bits<1>{v ? 1u : 0u}); }
static void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
int main() {
  for (unsigned workers : {1u,2u}) {
    pyc_dut dut(workers);
    auto drive = [&](bool clock, bool reset=false) { dut.drive({bit(clock),bit(reset)}); };
    drive(false); dut.system().Build();
    auto root = dut.root_->pyc_implementation;
    auto counter = root->pyc_instance_counter;
    auto held = root->pyc_instance_held;
    auto ledger = root->pyc_instance_ledger;
    auto snapshot = [&]() {
      std::vector<std::uint64_t> values;
      auto add = [&](const auto &storage) {
        const auto &q = storage->current(0);
        values.insert(values.end(), {q.q.packed().value().value(),
          q.q.packed().knownMask().value(), q.q.packed().zMask().value(), q.clock});
      };
      add(counter->pyc_instance_count_state); add(held->pyc_instance_count_state);
      add(ledger->pyc_instance_balance_state); add(ledger->pyc_instance_marker_state);
      add(root->pyc_instance_phase_state);
      return values;
    };
    auto current = [&](unsigned count, unsigned balance, unsigned marker,
                       unsigned phase, bool history) {
      const auto s = snapshot();
      require(s[0]==count && s[4]==5 && s[8]==balance && s[12]==marker && s[16]==phase,
              "committed state mismatch");
      for (unsigned i=3; i<s.size(); i+=4) require(s[i]==history,"clock history mismatch");
    };
    auto reset = [&]() {
      drive(false); dut.system().Reset(); current(5,40,90,0,false);
      require(dut.system().cycle()==0 && dut.observations().Events().empty(),"Reset publications");
    };
    auto step = [&](bool clock) {
      drive(clock);
      require(dut.system().Step()==gfsim::SimStepResult::Running,"successful epoch");
    };
    reset();
    drive(true);
    const auto initial = snapshot();
    dut.root_->Work(); dut.root_->DiscardNext(); dut.observations().DiscardNext();
    require(snapshot()==initial && dut.system().cycle()==0 && dut.observations().Events().empty(),
            "discard leaked state, clock or observations");
    step(true); current(6,50,91,1,true);
    step(true); current(6,50,91,1,true); // held high is not another edge
    step(false); current(6,50,91,1,false);
    step(true); current(6,50,91,2,true); // late require(0) rejects the whole writer
    step(false); current(6,50,91,2,false);
    step(true); current(7,60,91,3,true); // marker has no write intent
    step(false); current(7,60,91,3,false);
    step(true); current(7,60,91,4,true);
    const auto before = snapshot();
    const auto epoch = dut.system().cycle();
    const auto events = std::vector(dut.observations().Events().begin(),dut.observations().Events().end());
    drive(false);
    require(dut.system().Step()==gfsim::SimStepResult::Failed,"System assertion must fail");
    require(dut.system().failureInfo().phase==gfsim::SimFailurePhase::Check,"check phase");
    require(snapshot()==before && dut.system().cycle()==epoch,"failed epoch committed");
    require(std::vector(dut.observations().Events().begin(),dut.observations().Events().end())==events,
            "failed epoch published observations");
    require(dut.system().Step()==gfsim::SimStepResult::Failed && snapshot()==before,"sticky failure");
    reset();
    drive(true,true);
    require(dut.system().Step()==gfsim::SimStepResult::Running,"synchronous reset");
    current(5,40,90,0,true);
    // Force only the test scenario's phase to its deliberate failure point.
    // All shared DUT owners remain at initial Q and propose real new values.
    reset();
    using Current = std::remove_cvref_t<decltype(root->pyc_instance_phase_state->current(0))>;
    auto &phase = const_cast<Current &>(root->pyc_instance_phase_state->current(0));
    phase.q = decltype(phase.q)::known(gfsim::Bits<4>{4});
    root->pyc_instance_phase_q.element(0) = phase.q;
    const auto failing = snapshot();
    drive(true);
    require(dut.system().Step()==gfsim::SimStepResult::Failed,"initial rising assertion");
    require(snapshot()==failing && dut.system().cycle()==0 && dut.observations().Events().empty(),
            "assertion leaked sibling proposals or clock history");
    reset(); step(true); current(6,50,91,1,true);
  }
  std::cout << "SIBLING_ZERO_COMMIT_OK\n";
}
"""
    )
    with (generated / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(sibling_probe sibling_probe.cpp)\n"
            "target_compile_features(sibling_probe PRIVATE cxx_std_20)\n"
            "target_link_libraries(sibling_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "cpp-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(generated),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(
        _run(
            [
                "cmake",
                "--build",
                str(build),
                "--target",
                "sibling_probe",
                "--parallel",
                "2",
            ]
        )
    )
    assert (
        _ok(_run([str(build / "sibling_probe")], timeout=30)).strip()
        == "SIBLING_ZERO_COMMIT_OK"
    )


def test_sibling_icarus_reset_discard_and_failure_preserve_all_state_and_clocks(
    tmp_path,
):
    final = _final(tmp_path)
    generated = tmp_path / "verilog"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(generated)))
    probe = tmp_path / "sibling_probe.sv"
    probe.write_text(
        r"""
module sibling_probe;
  logic clk=0, rst=0, permit=0;
  logic [2:0] phase=0;
  wire error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permit), .pyc_local_error(error));
  task automatic current(input integer count, balance, marker, step, input logic clock);
    if (dut.dut.pyc_instance_counter.pyc_instance_count.q_current !== count ||
        dut.dut.pyc_instance_held.pyc_instance_count.q_current !== 5 ||
        dut.dut.pyc_instance_ledger.pyc_instance_balance.q_current !== balance ||
        dut.dut.pyc_instance_ledger.pyc_instance_marker.q_current !== marker ||
        dut.dut.pyc_instance_phase.q_current !== step)
      $fatal(1,"committed sibling state mismatch");
    if (dut.dut.pyc_instance_counter.pyc_instance_count.managed.clock_current !== clock ||
        dut.dut.pyc_instance_held.pyc_instance_count.managed.clock_current !== clock ||
        dut.dut.pyc_instance_ledger.pyc_instance_balance.managed.clock_current !== clock ||
        dut.dut.pyc_instance_ledger.pyc_instance_marker.managed.clock_current !== clock ||
        dut.dut.pyc_instance_phase.managed.clock_current !== clock)
      $fatal(1,"sibling clock history mismatch");
  endtask
  task automatic reset_model;
    clk=0; rst=0; permit=1; phase=4; #1;
    phase=2; #1; phase=0; permit=0; #1;
    current(5,40,90,0,0);
  endtask
  task automatic step(input logic clock);
    clk=clock; phase=1; #1;
    if (error) $fatal(1,"unexpected sibling check failure");
    permit=1; phase=2; #1; phase=0; permit=0; #1;
  endtask
  initial begin
    #1; reset_model();
    clk=1; phase=1; #1;
    if (error) $fatal(1,"discard setup failed");
    if (dut.dut.pyc_instance_counter.pyc_instance_count.managed.q_pending !== 6 ||
        dut.dut.pyc_instance_ledger.pyc_instance_balance.managed.q_pending !== 50 ||
        dut.dut.pyc_instance_ledger.pyc_instance_marker.managed.q_pending !== 91)
      $fatal(1,"missing real sibling pending proposals");
    phase=3; #1; phase=0; #1; current(5,40,90,0,0);
    step(1); current(6,50,91,1,1);
    step(1); current(6,50,91,1,1);
    step(0); current(6,50,91,1,0);
    step(1); current(6,50,91,2,1);
    step(0); current(6,50,91,2,0);
    step(1); current(7,60,91,3,1);
    step(0); current(7,60,91,3,0);
    step(1); current(7,60,91,4,1);
    clk=0; phase=1; #1;
    if (!error) $fatal(1,"missing source System assertion");
    phase=2; #1; current(7,60,91,4,1);
    phase=3; #1; phase=0; #1; current(7,60,91,4,1);
    reset_model(); rst=1; step(1); current(5,40,90,0,1);
    reset_model();
    // Test-only scenario stimulus makes the assertion fail while both sibling
    // owners propose writes. It never supplies counter or ledger results.
    dut.dut.pyc_instance_phase.q_current=4; #1;
    clk=1; phase=1; #1;
    if (!error) $fatal(1,"missing initial rising assertion");
    if (dut.dut.pyc_instance_counter.pyc_instance_count.managed.q_pending !== 6 ||
        dut.dut.pyc_instance_ledger.pyc_instance_balance.managed.q_pending !== 50)
      $fatal(1,"failure case did not have sibling write candidates");
    phase=2; #1; current(5,40,90,4,0);
    phase=3; #1; phase=0; #1; current(5,40,90,4,0);
    reset_model(); step(1); current(6,50,91,1,1);
    $display("SIBLING_RTL_ZERO_COMMIT_OK"); $finish;
  end
endmodule
"""
    )
    binary = tmp_path / "sibling-rtl"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "sibling_probe",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dff.v"),
                str(_install() / "include/verilog/dffe.v"),
                str(generated / "design_top.sv"),
                *(str(path) for path in sorted((generated / "sources").rglob("*.v"))),
                str(probe),
            ]
        )
    )
    stdout = _ok(_run(["vvp", str(binary)], timeout=30))
    assert "SIBLING_RTL_ZERO_COMMIT_OK" in stdout
    assert _records(stdout) == []


@pytest.mark.parametrize("case", ["wrong-nominal", "dynamic", "missing", "rebound"])
def test_module_reference_admission_rejects_and_preserves_unit(tmp_path, case):
    source = tmp_path / "source"
    source.mkdir()
    path = source / "reference.py"
    path.write_text(
        "import pycircuit as ac\n@ac.module\ndef Good() -> ac.u8:\n    return 1\n"
    )
    unit = tmp_path / "unit"
    _ok(_compile(source, path.name, unit))
    before = _tree(unit)
    declarations = """import pycircuit as ac
@ac.module
def Counter():
    count: ac.u8 = 5
    @ac.method
    def increment() -> None:
        count = count + 1
@ac.module
def Other():
    count: ac.u8 = 5
    @ac.method
    def increment() -> None:
        count = count + 1
@ac.module
def Writer(counter: Counter):
    @ac.rule
    def write() -> None:
        counter.increment()
    write()
@ac.module
def Bad(flag: ac.u1):
    left = Counter()
    right = Counter()
    other = Other()
"""
    bad = {
        "wrong-nominal": "    Writer(other)\n",
        "dynamic": "    selected = left if flag else right\n    Writer(selected)\n",
        "missing": "    Writer()\n",
        "rebound": "    selected = left\n    selected = right\n    Writer(selected)\n",
    }[case]
    path.write_text(declarations + bad)
    rejected = _cli(
        "compile",
        "-c",
        str(path),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    )
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert _tree(unit) == before
    assert any(
        word in rejected.stderr.lower()
        for word in (
            "reference",
            "instance",
            "module",
            "argument",
            "binding",
            "alias",
            "immutable",
        )
    ), rejected.stderr


def _empty_array(text, key):
    start = text.index(key + " = [") + len(key + " = ")
    depth = 0
    for end in range(start, len(text)):
        if text[end] == "[":
            depth += 1
        elif text[end] == "]":
            depth -= 1
            if depth == 0:
                return text[:start] + "[]" + text[end + 1 :]
    raise AssertionError("unterminated array in test source-unit fixture")


@pytest.mark.parametrize(
    "metadata",
    ["callable-after", "reference-formals", "schema-missing", "schema-invalid"],
)
def test_native_pair_verifier_rejects_forged_reference_and_order_interface(
    tmp_path, metadata
):
    source = tmp_path / "source"
    shutil.copytree(FIXTURE, source)
    provider = tmp_path / "counter-unit"
    _ok(_compile(source, "counter.py", provider))
    if metadata == "reference-formals":
        unit = tmp_path / "clients-unit"
        _ok(_compile(source, "clients.py", unit, [provider]))
        key = "ac.reference_parameters"
    else:
        unit = provider
        key = "after"
    receipt = json.loads((unit / "unit.json").read_text())
    body = unit / receipt["files"]["body"]
    header = unit / receipt["files"]["interface"]
    linker = _install() / "bin/pycircuit-link"
    report = tmp_path / "owner.json"
    _ok(
        _run(
            [
                str(linker),
                "--body",
                str(body),
                "--header",
                str(header),
                "--verify-only",
                "--unit-owner-out",
                str(report),
            ]
        )
    )
    report_before = report.read_bytes()
    original = header.read_text()
    tampered = tmp_path / "tampered.interface.ac"
    if metadata == "schema-missing":
        changed, count = re.subn(r", schema = 2 : i64", "", original, count=1)
        assert count == 1
    elif metadata == "schema-invalid":
        changed = original.replace("schema = 2 : i64", "schema = 1 : i64", 1)
    else:
        changed = _empty_array(original, key)
    tampered.write_text(changed)
    assert tampered.read_text() != original
    rejected = _run(
        [
            str(linker),
            "--body",
            str(body),
            "--header",
            str(tampered),
            "--verify-only",
            "--unit-owner-out",
            str(report),
        ]
    )
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert report.read_bytes() == report_before
    assert any(
        word in rejected.stderr.lower()
        for word in (
            "summary",
            "reference",
            "order",
            "snapshot",
            "interface",
            "declaration",
        )
    ), rejected.stderr


def _attribute_range(text, key, opening, closing, start=0):
    begin = text.index(key + " = " + opening, start) + len(key + " = ")
    depth = 0
    for end in range(begin, len(text)):
        if text[end] == opening:
            depth += 1
        elif text[end] == closing:
            depth -= 1
            if depth == 0:
                return begin, end + 1
    raise AssertionError("unterminated metadata in test artifact")


def _mutate_routed_identity(text, mutation):
    if mutation == "order":
        return _empty_array(text, "ac.after")
    if mutation in ("target-instance", "registration"):
        key, opening, closing = (
            ("ac.registration_path", "[", "]")
            if mutation == "target-instance"
            else ("ac.lexical_registration", "{", "}")
        )
        begin, end = _attribute_range(text, key, opening, closing)
        changed, count = re.subn(
            r"value = [0-9]+ : i64", "value = 99999 : i64", text[begin:end], count=1
        )
        assert count == 1
        return text[:begin] + changed + text[end:]
    start = 0
    while True:
        begin, end = _attribute_range(text, "ac.transaction_returns", "{", "}", start)
        metadata = text[begin:end]
        if "results = [" in metadata:
            results = metadata.index("results = [")
            changed, count = re.subn(
                r"enable = [0-9]+ : i64",
                "enable = 0 : i64",
                metadata[results:],
                count=1,
            )
            assert count == 1 and changed != metadata[results:]
            return text[:begin] + metadata[:results] + changed + text[end:]
        start = end


@pytest.mark.parametrize(
    "mutation", ["target-instance", "registration", "order", "return-enable"]
)
def test_native_final_rederives_lifted_identity_order_and_return_fire(
    tmp_path, mutation
):
    final = _final(tmp_path, saved=True, scenario="returned")
    report = tmp_path / "entry-owner.json"
    linker = _install() / "bin/pycircuit-link"
    _ok(
        _run(
            [
                str(linker),
                "--design",
                str(final),
                "--verify-only",
                "--entry-owner-out",
                str(report),
            ]
        )
    )
    original_report = report.read_bytes()
    original = final.read_bytes()
    published = tmp_path / "published-cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(published)))
    published_before = _tree(published)
    tampered = tmp_path / "tampered.ac"
    tampered.write_text(_mutate_routed_identity(original.decode(), mutation))
    assert tampered.read_bytes() != original
    rejected = _run(
        [
            str(linker),
            "--design",
            str(tampered),
            "--verify-only",
            "--entry-owner-out",
            str(report),
        ]
    )
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert report.read_bytes() == original_report
    assert any(
        word in rejected.stderr.lower()
        for word in (
            "registration",
            "after",
            "predecessor",
            "forward",
            "candidate",
            "return",
            "allocation",
        )
    ), rejected.stderr
    rejected = _cli(
        "emit", str(tampered), "--target", "cpp", "-o", str(published), "--replace"
    )
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert _tree(published) == published_before
    assert final.read_bytes() == original


@pytest.mark.parametrize(
    "case", ["combinational-result-cycle", "cross-resource-static-cycle"]
)
def test_lifted_cycles_reject_without_replacing_final(tmp_path, case):
    source = tmp_path / "source"
    source.mkdir()
    path = source / "cycle.py"
    path.write_text(
        'import pycircuit as ac\n@ac.system\ndef Good():\n    "Closed root."\n'
    )
    good = tmp_path / "good-unit"
    _ok(_compile(source, path.name, good))
    final = tmp_path / "final.ac"
    _ok(_cli("link", str(good), "--top", "checks.cycle.Good", "-o", str(final)))
    before = final.read_bytes()
    if case == "combinational-result-cycle":
        text = """import pycircuit as ac
@ac.module
def Counter():
    count: ac.u8 = 5
    @ac.method(guard=lambda allow: allow)
    def increment(allow: ac.u1) -> None:
        count = count + 1
    @ac.method(after=(increment,))
    def read() -> ac.u8:
        return count
@ac.module
def Writer(counter: Counter, allow: ac.u1):
    @ac.rule
    def write() -> None:
        counter.increment(allow)
    write()
@ac.module
def Reader(counter: Counter) -> ac.u8:
    @ac.rule
    def read() -> ac.u8:
        return counter.read()
    return read()
@ac.system
def Bad():
    counter = Counter()
    value = Reader(counter)
    Writer(counter, value == 6)
"""
    else:
        text = (
            (FIXTURE / "counter.py").read_text()
            + """
@ac.module
def First(left: Counter, right: Counter, choose: ac.u1):
    @ac.rule
    def act() -> None:
        if choose:
            left.increment()
        else:
            value = right.read()
            ac.log("info", "right", value)
    act()
@ac.module
def Second(left: Counter, right: Counter, choose: ac.u1):
    @ac.rule
    def act() -> None:
        if choose:
            right.increment()
        else:
            value = left.read()
            ac.log("info", "left", value)
    act()
@ac.system
def Bad():
    choose: ac.u1 = 0
    left = Counter()
    right = Counter()
    First(left, right, choose)
    Second(left, right, choose)
"""
        )
    path.write_text(text)
    unit = tmp_path / "bad-unit"
    compiled = _compile(source, path.name, unit)
    rejected = compiled
    if compiled.returncode == 0:
        rejected = _cli(
            "link",
            str(unit),
            "--top",
            "checks.cycle.Bad",
            "-o",
            str(final),
            "--replace",
        )
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    # The source filename is cycle.py; it must not satisfy the diagnostic oracle.
    diagnostic = rejected.stderr.rsplit("error:", 1)[-1].lower()
    assert any(
        word in diagnostic for word in ("cycle", "cyclic", "dependency")
    ), rejected.stderr
    assert final.read_bytes() == before


def test_unselected_reference_definition_does_not_bind_or_invalidate_closed_top(
    tmp_path,
):
    source = tmp_path / "source"
    shutil.copytree(FIXTURE, source)
    (source / "bench.py").write_text(
        """import pycircuit as ac
from checks.counter import Counter
@ac.system
def Closed():
    counter = Counter()
    @ac.rule
    def observe() -> None:
        ac.log("info", "closed old", counter.old())
    observe()
"""
    )
    units = []
    for filename in ("counter.py", "clients.py", "bench.py"):
        unit = tmp_path / (Path(filename).stem + "-unit")
        _ok(_compile(source, filename, unit, units))
        (source / filename).unlink()
        units.append(unit)
    before = [_tree(unit) for unit in units]
    final = tmp_path / "closed.ac"
    _ok(
        _cli(
            "link",
            *(str(unit) for unit in units),
            "--top",
            "checks.bench.Closed",
            "-o",
            str(final),
        )
    )
    valid = final.read_bytes()
    rejected = _cli(
        "link",
        *(str(unit) for unit in units),
        "--top",
        "checks.clients.Writer",
        "-o",
        str(final),
        "--replace",
    )
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert any(
        word in rejected.stderr.lower() for word in ("reference", "binding", "bound")
    ), rejected.stderr
    assert final.read_bytes() == valid
    assert [_tree(unit) for unit in units] == before
    for directory in (source, *units):
        shutil.rmtree(directory)
    for target in ("cpp", "verilog"):
        root = tmp_path / target
        root.mkdir()
        binary = _bundle(final, root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "1", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=1"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            assert _values(rows, "closed old") == [(5,), (5,)]
