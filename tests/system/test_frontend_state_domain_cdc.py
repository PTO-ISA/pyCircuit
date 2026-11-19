"""Independent source-level state, domain and CDC execution contracts."""

from __future__ import annotations

import hashlib
import json
import re
from collections import deque
from pathlib import Path

import pytest
from test_anchor_state_reset import (
    _build_native_probe,
    _fifo_rows,
    _simulate_rtl,
    _system_target_records,
)
from test_source_system_execution import _cli, _compile, _ok

pytestmark = pytest.mark.system
FIXTURES = Path(__file__).parent / "fixtures/frontend_state_domain_cdc"


def _saved_anchor(tmp_path, filename, top, providers=(), source=FIXTURES):
    unit = tmp_path / "unit"
    _ok(_compile(source, filename, unit, providers))
    anchor = tmp_path / "anchor"
    _ok(
        _cli(
            "link",
            *map(str, providers),
            str(unit),
            "--top",
            f"checks.{Path(filename).stem}.{top}",
            "--target",
            "anchor",
            "-o",
            str(anchor),
        )
    )
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    return anchor


def _roundtrip(tmp_path, filename, top, providers=(), source=FIXTURES):
    anchor = _saved_anchor(tmp_path, filename, top, providers, source)
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    return final


# Each row supplies controls, old Work value, and value after successful commit.
# Values come from source behavior and edge/reset semantics, never DUT traces.
STATE_CASES = {
    "Sync": [
        ((0, 0, 0), 5, 5),
        ((0, 1, 0), 5, 5),
        ((1, 1, 0), 5, 0),
        ((0, 0, 1), 0, 0),
        ((1, 0, 1), 0, 1),
        ((0, 0, 0), 1, 1),
        ((1, 0, 0), 1, 1),
    ],
    "Async": [
        ((0, 1, 0), 5, 5),
        ((1, 1, 1), 5, 5),
        ((0, 1, 1), 5, 6),
        ((0, 0, 0), 6, 0),
        ((0, 1, 0), 0, 0),
        ((1, 1, 1), 0, 0),
        ((0, 1, 1), 0, 1),
    ],
    "Uninitialized": [
        ((0, 0, 0), None, None),
        ((1, 1, 0), None, None),
        ((0, 0, 1), None, None),
        ((1, 0, 1), None, 9),
        ((0, 1, 0), 9, 9),
        ((1, 1, 0), 9, 9),
    ],
}


def _state_inputs(ports, controls):
    clock, reset, enable = controls
    values = {"pyc_clk": clock, "pyc_rst": reset, "rst_n": reset, "enable": enable}
    if any(port["name"] == "rst_n" for port in ports["inputs"]):
        values["pyc_rst"] = 0
    return [values[port["name"]] for port in ports["inputs"]]


def _state_cpp(ports, rows):
    output = ports["outputs"][0]["wire"]
    steps = []
    for controls, old, _ in rows:
        values = ",".join(f"bit({v})" for v in _state_inputs(ports, controls))
        check = (
            "q.knownMask().value()==0 && q.zMask().value()==0"
            if old is None
            else f"q.isFullyKnown() && q.value().value()=={old}"
        )
        steps.append(
            f"dut.drive({{{values}}});"
            "require(dut.system().Step()==gfsim::SimStepResult::Running);"
            f"{{auto q=dut.sample().{output}.packed();require({check});}}"
        )
    initial = ",".join(f"bit({v})" for v in _state_inputs(ports, rows[0][0]))
    return (
        '#include "pycircuit_system.hpp"\n#include <stdexcept>\n'
        "using Bit=gfsim::wire<gfsim::Bits<1>>;\n"
        "static Bit bit(unsigned v){return Bit::known(gfsim::Bits<1>{v});}\n"
        'static void require(bool v){if(!v)throw std::runtime_error("state oracle");}\n'
        "int main(){for(unsigned workers:{1u,2u}){pyc_dut dut(workers);"
        f"dut.drive({{{initial}}});dut.system().Build();dut.system().Reset();"
        + "\n".join(steps)
        + "}}\n"
    )


def _state_rtl(ports, rows):
    bindings = [f".{p['wire']}(i{n})" for n, p in enumerate(ports["inputs"])]
    bindings += [f".{ports['outputs'][0]['wire']}(q)"]
    bindings += [
        ".pyc_phase(phase)",
        ".pyc_root_commit_ok(permit)",
        ".pyc_check_complete(complete)",
        ".pyc_sampling_error(sampling_error)",
        ".pyc_local_error(error)",
    ]
    initial = _state_inputs(ports, rows[0][0])
    declarations = "\n".join(f"logic i{n}={v};" for n, v in enumerate(initial))
    steps = []
    for controls, old, new in rows:
        drive = "".join(
            f"i{n}={v};" for n, v in enumerate(_state_inputs(ports, controls))
        )
        before = "9'bxxxxxxxxx" if old is None else f"9'd{old}"
        after = "9'bxxxxxxxxx" if new is None else f"9'd{new}"
        steps.append(
            drive + "#1;phase=1;#1;"
            f"if(q!=={before} || error!==0 || sampling_error!==0 || complete!==1)"
            '$fatal(1,"old state or admission");'
            "permit=1;phase=2;#1;phase=0;permit=0;#1;"
            f'if(q!=={after})$fatal(1,"committed state");'
        )
    return (
        "module state_probe;\n"
        + declarations
        + "\nlogic[2:0]phase=0;logic permit=0;wire[8:0]q;"
        "wire complete,sampling_error,error;\n"
        "pyc_root dut(" + ",".join(bindings) + ");\n"
        "initial begin permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;\n"
        + "\n".join(steps)
        + '\n$display("FRONTEND_STATE_OK");$finish;end\nendmodule\n'
    )


@pytest.mark.parametrize("top", STATE_CASES)
def test_state_policy_roundtrip(tmp_path, top):
    final = _roundtrip(tmp_path, "state.py", top)
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    rows = STATE_CASES[top]
    _build_native_probe(final, tmp_path, lambda ports: _state_cpp(ports, rows))
    generated = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(generated)))
    ports = json.loads((generated / "ports.json").read_text())
    bench = tmp_path / "state.sv"
    bench.write_text(_state_rtl(ports, rows))
    assert "FRONTEND_STATE_OK" in _simulate_rtl(
        generated, bench, tmp_path / "state.vvp", "state_probe"
    )
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_named_domains_independent_hierarchy(tmp_path):
    counter, hierarchy = tmp_path / "counter", tmp_path / "hierarchy"
    _ok(_compile(FIXTURES, "counters.py", counter))
    _ok(_compile(FIXTURES, "hierarchy.py", hierarchy, [counter]))
    final = _roundtrip(tmp_path, "domains.py", "Bench", [counter, hierarchy])
    # Work observes old state; reset only changes either counter on its edge.
    expected = sorted(
        [
            (1, 1, 2, 2, 3, 3, 1, 1, 2),
            (10, 10, 10, 11, 11, 11, 10, 10, 11),
        ]
    )
    # domains.py asserts rst_n low in rows 4 and 5. Counter's cohesive Rule
    # writes that reset-controlled state, so its log is suppressed in both
    # rows; synchronous storage itself resets only on the row-5 clock edge.
    expected_epochs = (0, 1, 2, 3, 6, 7, 8)
    expected_observations = sorted(
        tuple(values[epoch] for epoch in expected_epochs) for values in expected
    )
    for records in _system_target_records(final, tmp_path):
        instances = {}
        for row in records:
            if row.get("kind") == "log" and row["spec"]["event"] == "domain count":
                epochs = instances.setdefault(row["instance"], {})
                epoch = int(row["evaluation_epoch"])
                assert epoch not in epochs
                epochs[epoch] = int(row["values"][0]["value"])
        assert len(instances) == 2
        assert all(set(epochs) == set(expected_epochs) for epochs in instances.values())
        assert (
            sorted(
                tuple(epochs[n] for n in expected_epochs)
                for epochs in instances.values()
            )
            == expected_observations
        )


def test_ordinary_cross_domain_method_rejected(tmp_path):
    counter, consumer = tmp_path / "counter", tmp_path / "consumer"
    _ok(_compile(FIXTURES, "counters.py", counter))
    result = _compile(FIXTURES, "cross_domain_method.py", consumer, [counter])
    if result.returncode == 0:
        result = _cli(
            "link",
            str(counter),
            str(consumer),
            "--top",
            "checks.cross_domain_method.Bench",
            "-o",
            str(tmp_path / "bad.ac"),
        )
    assert result.returncode != 0
    assert "domain" in (result.stdout + result.stderr).lower()


def _scheduled_source(tmp_path, filename, columns, rows):
    source = tmp_path / "source"
    source.mkdir()
    schedule = [dict(zip(columns, row, strict=True)) for row in rows]
    template = (FIXTURES / filename).read_text()
    assert template.count("samples=[]") == 1
    (source / filename).write_text(
        template.replace("samples=[]", f"samples={schedule!r}")
    )
    return source


def _explicit_endpoint_roundtrip(
    tmp_path, source, filename, inferred, explicit, providers=()
):
    # Explicit endpoint syntax also must survive publication and saved reload;
    # only the inferred variant needs the full backend execution matrix.
    root = tmp_path / "explicit-endpoints"
    configured_source = root / "source"
    configured_source.mkdir(parents=True)
    text = (source / filename).read_text()
    assert text.count(inferred) == 1
    (configured_source / filename).write_text(text.replace(inferred, explicit))
    _roundtrip(root, filename, "Bench", providers, configured_source)


def test_async_reg_shared_level_roundtrip(tmp_path):
    rows = [(0, 0, 1), (0, 0, 0)]
    rows += [(clock, clock, 0) for _ in range(40) for clock in (1, 0)]
    # Assert whole-resource reset while both clocks remain low.
    rows += [(0, 0, 1), (0, 0, 0)]
    rows += [(clock, clock, 0) for _ in range(12) for clock in (1, 0)]
    source = _scheduled_source(
        tmp_path,
        "async_reg.py",
        ("clk_fast", "clk_slow", "reset", "pyc_rst"),
        [(*row, row[2]) for row in rows],
    )
    final = _roundtrip(tmp_path, "async_reg.py", "Bench", source=source)
    _explicit_endpoint_roundtrip(
        tmp_path,
        source,
        "async_reg.py",
        "ac.AsyncReg[ac.u1]()",
        "ac.AsyncReg[ac.u1](source=fast, target=slow, reset=reset)",
    )
    # The primary fixture drives inherited pyc_rst from each row's reset.
    # Receiver's observation Rule cannot commit while that reset is asserted.
    expected_epochs = {epoch for epoch, row in enumerate(rows) if not row[2]}
    traces = []
    for records in _system_target_records(final, tmp_path):
        instances = {}
        for row in records:
            if row.get("kind") != "log" or row["spec"]["event"] != "synchronized level":
                continue
            instances.setdefault(row["instance"], {})[int(row["evaluation_epoch"])] = (
                int(row["values"][0]["value"])
            )
        assert len(instances) == 2
        left, right = instances.values()
        # Instance enumeration is not a source contract. The non-inverting
        # consumer observes one at this independently derived stable checkpoint.
        if left[20] == 0:
            left, right = right, left
        assert set(left) == expected_epochs
        assert set(right) == set(left)
        assert all(
            left[n] == 1 - right[n] for n in left
        ), "a transparent expression must preserve the shared resource's target domain"
        # Stable source levels have had many receiving edges to propagate.
        assert all(left[n] == 1 for n in range(20, 30))
        assert all(left[n] == 0 for n in range(65, 82))
        assert left[83] == 0, "between-edge whole reset must clear synchronized output"
        assert all(left[n] == 1 for n in range(100, len(rows)))
        traces.append((left, right))
    assert traces[0] == traces[1] == traces[2]


def test_async_fifo_independent_endpoints_roundtrip(tmp_path):
    endpoints = tmp_path / "endpoints"
    _ok(_compile(FIXTURES, "fifo_endpoints.py", endpoints))
    rows, stopped_release = _fifo_rows()
    source = _scheduled_source(
        tmp_path,
        "async_fifo.py",
        ("clk_write", "clk_read", "reset", "permit", "enable"),
        [(*row, 1, 1) for row in rows],
    )
    final = _roundtrip(tmp_path, "async_fifo.py", "Bench", [endpoints], source)
    _explicit_endpoint_roundtrip(
        tmp_path,
        source,
        "async_fifo.py",
        "ac.AsyncFIFO[ac.u9, 4](reset=reset)",
        "ac.AsyncFIFO[ac.u9, 4](write=writer, read=reader, reset=reset)",
        [endpoints],
    )
    _logical_fifo_depth_three(tmp_path, source, final)
    traces = []
    for records in _system_target_records(final, tmp_path):
        events = {}
        for row in records:
            if row.get("kind") != "log" or row["spec"]["event"] not in (
                "sent",
                "received",
            ):
                continue
            key = (int(row["evaluation_epoch"]), row["spec"]["event"])
            assert key not in events
            events[key] = int(row["values"][0]["value"])
        pending = deque()
        previous = (0, 0, 0)
        token = pushes = pops = capacity = 0
        for epoch, (write, read, reset) in enumerate(rows):
            if reset:
                pending.clear()
                token = 0
            sent, received = events.get((epoch, "sent")), events.get(
                (epoch, "received")
            )
            if epoch in stopped_release:
                assert sent is None and received is None
            if not reset and read and not previous[1] and received is not None:
                assert pending, "pop must not bypass a same-epoch push"
                assert received == pending.popleft()
                pops += 1
            if not reset and write and not previous[0] and sent is not None:
                assert sent == token, "blocked push must not advance producer state"
                pending.append(sent)
                token = (token + 1) & 511
                pushes += 1
                capacity = max(capacity, len(pending))
                assert len(pending) <= 4
            previous = (write, read, reset)
        assert pushes > 20 and pops > 16
        assert capacity == 4
        assert not pending
        traces.append(events)
    assert traces[0] == traces[1] == traces[2]
    _fifo_rollback(final, tmp_path)


def _logical_fifo_depth_three(tmp_path, source, protected_final):
    root = tmp_path / "logical-depth-three"
    depth_source = root / "source"
    depth_source.mkdir(parents=True)
    for filename, original in (
        ("async_fifo.py", source),
        ("fifo_endpoints.py", FIXTURES),
    ):
        text = (original / filename).read_text()
        assert "ac.AsyncFIFO[ac.u9, 4]" in text
        (depth_source / filename).write_text(
            text.replace("ac.AsyncFIFO[ac.u9, 4]", "ac.AsyncFIFO[ac.u9, 3]")
        )
    endpoints = root / "endpoints"
    _ok(_compile(depth_source, "fifo_endpoints.py", endpoints))
    anchor = _saved_anchor(root, "async_fifo.py", "Bench", [endpoints], depth_source)
    before = protected_final.read_bytes()
    result = _cli(
        "link",
        "--from-anchor",
        str(anchor),
        "--target",
        "final",
        "-o",
        str(protected_final),
        "--replace",
    )
    assert (
        result.returncode != 0
    ), "logical depth three has no supported physical recipe"
    assert (
        "async_fifo.py" in result.stdout + result.stderr
    ), "report the resource's source location"
    assert protected_final.read_bytes() == before


def _fifo_fault_rows():
    rows = [(0, 0, 1, 0), (0, 0, 0, 0)]
    rows += [(clock, clock, 0, 1) for _ in range(20) for clock in (1, 0)]
    # Stop algorithmic production while both clocks continue: reads drain the
    # queue and the writer can learn that capacity has returned. Stopping its
    # clock instead would leave a stale full view and mask the intended fault.
    rows += [(clock, clock, 0, 0) for _ in range(20) for clock in (1, 0)]
    failed_epoch = len(rows)
    rows += [(1, 0, 0, 1), (0, 0, 0, 1)]
    rows += [(clock, clock, 0, 1) for _ in range(20) for clock in (1, 0)]
    rows += [(clock, clock, 0, 0) for _ in range(20) for clock in (1, 0)]
    return rows, failed_epoch


def _fifo_clock_inputs(ports, receipt):
    # Match published control transport to independently authored clock stimuli;
    # neither compiler name mangling nor DUT observations identify these clocks.
    rows, _ = _fifo_rows()
    samples = receipt["system_samples"]
    sample_inputs = receipt["system_sample_inputs"]
    assert len(samples) == len(rows)
    clock_inputs = {
        row["ordinal"] for row in ports["inputs"] if "clock" in row["roles"]
    }
    result = {}
    for name, index in (("write", 0), ("read", 1)):
        expected = tuple(str(row[index]) for row in rows)
        matches = [
            ordinal
            for column, ordinal in enumerate(sample_inputs)
            if ordinal in clock_inputs
            and tuple(row[column] for row in samples) == expected
        ]
        assert len(matches) == 1, f"published {name} clock column must be unique"
        result[name] = matches[0]
    assert result["write"] != result["read"]
    return result


def _fifo_inputs(ports, controls, permit, clocks):
    write, read, reset, enable = controls
    values = {
        "reset": reset,
        "permit": permit,
        "enable": enable,
        "pyc_rst": 0,
        "pyc_clk": 0,
    }
    result = []
    for row in ports["inputs"]:
        if row["ordinal"] == clocks["write"]:
            result.append(write)
        elif row["ordinal"] == clocks["read"]:
            result.append(read)
        elif "clock" in row["roles"]:
            result.append(0)
        else:
            result.append(values[row["name"]])
    return result


def _fifo_rollback_cpp(ports, clocks):
    rows, failed_epoch = _fifo_fault_rows()
    steps = []
    for epoch, row in enumerate(rows):
        steps.append(f"epoch={epoch};")
        good = ",".join(f"bit({v})" for v in _fifo_inputs(ports, row, 1, clocks))
        if epoch == failed_epoch:
            bad = ",".join(f"bit({v})" for v in _fifo_inputs(ports, row, 0, clocks))
            steps.append(
                "{gfsim::ObservationSlots before=replay.observations();"
                f"replay.drive({{{bad}}});replay.root_->Work();"
                "gfsim::SimFailureInfo failure{};"
                'require(!replay.root_->pyc_validate_checks(failure),"expected fault");'
                "replay.root_->DiscardNext();replay.observations().DiscardNext();"
                'require(std::ranges::equal(before.Events(),replay.observations().Events()),"discard observations");}'
            )
        steps.append(
            f"reference.drive({{{good}}});replay.drive({{{good}}});"
            "commit(reference);commit(replay);"
            'require(std::ranges::equal(reference.observations().Events(),replay.observations().Events()),"replay observations");'
        )
    initial = ",".join(f"bit({v})" for v in _fifo_inputs(ports, rows[0], 1, clocks))
    return (
        '#include "pycircuit_system.hpp"\n#include <algorithm>\n#include <stdexcept>\n'
        "using Bit=gfsim::wire<gfsim::Bits<1>>;"
        "static Bit bit(unsigned v){return Bit::known(gfsim::Bits<1>{v});}"
        'static unsigned epoch=0,worker=0; static void require(bool v,const char* label){if(!v)throw std::runtime_error(std::string(label)+" epoch="+std::to_string(epoch)+" worker="+std::to_string(worker));}'
        "static void commit(pyc_dut &dut){dut.root_->Work();gfsim::SimFailureInfo failure{};"
        'require(dut.root_->pyc_validate_checks(failure),"successful precheck");'
        'require(dut.observations().Check() && dut.observations().Precommit(0),"observation precommit");'
        "dut.root_->Xfer();dut.observations().Xfer(1);}"
        "int main(){for(unsigned workers:{1u,2u}){worker=workers;pyc_dut reference(workers),replay(workers);"
        f"reference.drive({{{initial}}});replay.drive({{{initial}}});"
        "reference.system().Build();replay.system().Build();"
        "reference.system().Reset();replay.system().Reset();"
        + "\n".join(steps)
        + "}}\n"
    )


def _fifo_rollback_rtl(ports, observation_slots, clocks):
    rows, failed_epoch = _fifo_fault_rows()
    declarations = []
    bindings = {"reference": [], "replay": []}
    for n, (port, value) in enumerate(
        zip(ports["inputs"], _fifo_inputs(ports, rows[0], 1, clocks), strict=True)
    ):
        declarations.append(f"logic r{n}={value},t{n}={value};")
        bindings["reference"].append(f".{port['wire']}(r{n})")
        bindings["replay"].append(f".{port['wire']}(t{n})")
    for name, prefix in (("reference", "r"), ("replay", "t")):
        bindings[name] += [
            f".pyc_phase({prefix}phase)",
            f".pyc_root_commit_ok({prefix}permit)",
            f".pyc_check_complete({prefix}complete)",
            f".pyc_sampling_error({prefix}sampling)",
            f".pyc_local_error({prefix}error)",
        ]
        declarations.append(f"pyc_root {name}({','.join(bindings[name])});")
    checks = []
    for slot, prefix in observation_slots.items():
        for field in ("pending", "path", "valid"):
            checks.append(
                f'if(reference.{prefix}pyc_observation_{field}_{slot} !== replay.{prefix}pyc_observation_{field}_{slot})$fatal(1,"FIFO observation admission");'
            )
        checks.append(
            f'if(reference.{prefix}pyc_observation_pending_{slot} && reference.{prefix}pyc_observation_path_{slot} && reference.{prefix}pyc_observation_valid_{slot} && reference.{prefix}pyc_observation_value_{slot} !== replay.{prefix}pyc_observation_value_{slot})$fatal(1,"FIFO rollback changed token sequence");'
        )
    steps = []
    for epoch, row in enumerate(rows):
        values = _fifo_inputs(ports, row, 1, clocks)
        if epoch == failed_epoch:
            steps.append(
                "".join(
                    f"t{n}={v};"
                    for n, v in enumerate(_fifo_inputs(ports, row, 0, clocks))
                )
                + '#1;tphase=1;#1;if(terror!==1)$fatal(1,"missing FIFO fault");'
                "tphase=3;#1;tphase=0;#1;"
            )
        steps.append(
            "".join(f"r{n}={v};t{n}={v};" for n, v in enumerate(values))
            + '#1;rphase=1;tphase=1;#1;if(rerror!==0 || terror!==0 || rsampling!==0 || tsampling!==0 || rcomplete!==1 || tcomplete!==1)$fatal(1,"FIFO retry admission");'
            + "".join(checks)
            + "rpermit=1;tpermit=1;rphase=2;tphase=2;#1;rphase=0;tphase=0;rpermit=0;tpermit=0;#1;"
        )
    return (
        "module fifo_rollback;logic[2:0]rphase=0,tphase=0;logic rpermit=0,tpermit=0;"
        "wire rcomplete,tcomplete,rsampling,tsampling,rerror,terror;\n"
        + "\n".join(declarations)
        + "\ninitial begin rphase=4;tphase=4;#1;rpermit=1;tpermit=1;rphase=2;tphase=2;#1;rphase=0;tphase=0;rpermit=0;tpermit=0;#1;\n"
        + "\n".join(steps)
        + '\n$display("FIFO_ROLLBACK_OK");$finish;end\nendmodule\n'
    )


def _fifo_rollback(final, tmp_path):
    root = tmp_path / "rollback"
    root.mkdir()

    def native(ports):
        # Existing fault adapters expose the wrapper solely to drive its normal
        # lifecycle. No storage graph or generated register names form the oracle.
        header = root / "cpp/pycircuit_system.hpp"
        header.write_text(header.read_text().replace("private:", "public:"))
        receipt = json.loads((root / "cpp/generated.json").read_text())
        return _fifo_rollback_cpp(ports, _fifo_clock_inputs(ports, receipt))

    _build_native_probe(final, root, native)
    generated = tmp_path / "rtl"
    ports = json.loads((generated / "ports.json").read_text())
    support = (generated / "simulation_top.sv").read_text()
    # Follow the generated driver's observation transport through any wrapper.
    slots = {}
    for prefix, slot in re.findall(
        r"\bdut\.((?:[A-Za-z_]\w*\.)*)pyc_observation_value_(\d+)", support
    ):
        assert slot not in slots or slots[slot] == prefix
        slots[slot] = prefix
    assert slots, "the generated observation transport must expose source logs"
    bench = root / "rollback.sv"
    receipt = json.loads((generated / "generated.json").read_text())
    bench.write_text(
        _fifo_rollback_rtl(ports, slots, _fifo_clock_inputs(ports, receipt))
    )
    assert "FIFO_ROLLBACK_OK" in _simulate_rtl(
        generated, bench, root / "rollback.vvp", "fifo_rollback"
    )
