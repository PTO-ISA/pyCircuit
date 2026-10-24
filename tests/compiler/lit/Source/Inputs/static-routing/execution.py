"""Independent System observation oracle and real storage failure probes."""

import hashlib
import json
import re
import runpy
import shutil
from pathlib import Path


SAMPLES = ((0, 0), (1, 0), (1, 0), (0, 0), (1, 1), (0, 0), (1, 0))
RESOURCE_SAMPLES = (*SAMPLES, (0, 0), (1, 0), (0, 0), (1, 0))
HELD = {"kind": "bits4", "width": 8, "value": "83", "known": "cf", "z": "10"}


def records(text):
    return [json.loads(line) for line in text.splitlines() if line.startswith("{")]


def integer(value):
    return {"kind": "integer", "value": str(value)}


def memory_expectations(name):
    return runpy.run_path(str(Path(__file__).with_name("memory_oracles.py")))["expected"](name)


def expected_logs(name):
    if name == "memory-late-failure":
        return [("memory-accepted", [integer(0), integer(1), HELD])] * 2
    if name.startswith("memory-"):
        return [(event, values) for _, event, values in memory_expectations(name)]
    if name in ("indexed-demand", "indexed-demand-conjunction"):
        return [("indexed-demand", [integer(value) for value in values])
                for values in ([0] * 8, [0] * 8,
                               [7, 0, 0, 0, 7, 0, 7, 9],
                               [7, 0, 0, 0, 7, 0, 7, 9])]
    if name.startswith("partial-"):
        left, right, first, second = 5, 9, 5, 9
        previous_clock = 0
        expected = []
        samples = SAMPLES[:2] if name == "partial-late-failure" else SAMPLES
        for clock, reset in samples:
            new_left, new_right = (left + 3) % 256, (right + 5) % 256
            new_first, new_second = (first + 7) % 256, (second + 11) % 256
            # Each independent outer transaction sees committed foreign state,
            # while its own subsequent read sees its candidate and saved locals
            # remain snapshots. Disabled poison writes never appear here.
            expected.extend([
                ("partial-left", [integer(v) for v in (left, right, new_left, right,
                    first, second, new_first, second)] + [HELD, HELD, integer(23)]),
                ("partial-right", [integer(v) for v in (left, right, left, new_right,
                    first, second, first, new_second)] + [HELD, HELD, integer(23)]),
                ("partial-state", [integer(left), integer(right), HELD,
                    integer(first), integer(second), HELD, integer(23)]),
            ])
            if clock and not previous_clock:
                left, right, first, second = ((5, 9, 5, 9) if reset else
                                              (new_left, new_right, new_first, new_second))
            previous_clock = clock
        return expected
    if name == "reference-resources":
        # A push commits on the first normal rising edge. Holds/falls repeatedly
        # observe that same committed head; reset discards the pending pop/write.
        phases = (0, 0, 1, 1, 1, 0, 0, 1, 1, 2, 2)
        return [("resource-fill", [integer(13)]) if phase == 0 else
                ("resource-consume", [integer(13), integer(0x34)]) if phase == 1 else
                ("resource-read", [integer(0xAB34)]) for phase in phases]
    if name.startswith("ancestor-"):
        return [("ancestor", [integer(7)]) for _ in SAMPLES]
    if name == "late-failure":
        return [("accepted", [integer(13), integer(1)])] * 2
    if name not in ("sibling", "crossbranch"):
        return []
    peer = distinct = 13
    previous_clock = 0
    expected = []
    for clock, reset in SAMPLES:
        forwarded, other_forwarded = (peer + 3) % 256, (distinct + 5) % 256
        values = [integer(v) for v in (peer, forwarded, distinct, distinct, other_forwarded)]
        # Literal 10xz0011, derived directly from symbols: value=10000011,
        # known=11001111, Z=00010000. X and Z never become availability.
        values.append(HELD)
        expected.append(("routing", values))
        if clock and not previous_clock:
            peer, distinct = (13, 13) if reset else (forwarded, other_forwarded)
        previous_clock = clock
    return expected


def instance_names(cpp, partial=False, source_names=None, state_names=None):
    text = "\n".join(path.read_text() for path in cpp.rglob("*.hpp"))
    declared = set(re.findall(r"\b(pyc_instance_\w+)\b", text))
    # Native storage has a declared collection_storage pointer named <base>_state;
    # RTL instantiates <base> directly. Do not infer a base from clk/d/q tokens.
    storage = set(re.findall(
        r"^\s*std::shared_ptr<gfsim::collection_storage<[^;\n]+>\s+"
        r"(pyc_instance_\w+)_state;\s*$", text, re.MULTILINE))
    if state_names is None:
        state_names = ({"packet", "entries", "progress", "phase", "disabled_gate"}
                       if partial else {"count", "progress", "sibling"})
    names = {}
    if source_names is None:
        source_names = (("remote", "bank", "packet", "entries", "left", "right", "disabled", "worker", "progress", "phase", "disabled_gate")
                        if partial else ("peer", "worker", "count", "progress", "sibling"))
    for name in source_names:
        choices = {"pyc_instance_" + name, "pyc_instance_pyc_" + name.encode().hex()}
        found = choices & (storage if name in state_names else declared)
        assert len(found) == 1, (name, found)
        names[name.upper()] = found.pop()
    return names


def failure_check_endpoint(cpp, rtl, final, message):
    # Both emitters consumed this final artifact. Bind the monitor to the actual
    # generated native failure descriptor, then authenticate its RTL endpoint.
    assertions = [line for line in final.read_text().splitlines()
                  if '"ac.expect"' in line and 'kind = "assert"' in line
                  and 'ac.message = ' + json.dumps(message) in line]
    assert len(assertions) == 1, (message, assertions)
    branches = []
    descriptor = f'std::string_view{{{json.dumps(message)}, {len(message)}}}'
    for header in cpp.rglob("*.hpp"):
        for expression, failure in re.findall(
                r"const auto pyc_error = ([^\n]+);\n\s*if \([^\n]+\) \{\n\s*failure = ([^\n]+);",
                header.read_text()):
            if descriptor in failure and 'std::string_view{"source_check_failed", 19}' in failure:
                branches.append((header, expression))
    assert len(branches) == 1, (message, branches)
    header, expression = branches[0]
    endpoints = re.findall(
        r"pyc_implementation((?:->pyc_instance_\w+)*)->pyc_get_check_(path|condition)_(\d+)\(0\)",
        expression)
    assert len(endpoints) == 2 and {item[1] for item in endpoints} == {"path", "condition"}, endpoints
    owner, _, ordinal = endpoints[0]
    assert all(item[0] == owner and item[2] == ordinal for item in endpoints), endpoints
    assert "pyc_implementation" + owner + "->pyc_get_check_reset(0)" in expression
    instances = owner.split("->")[1:]
    modules = {}
    for source in rtl.rglob("*.v"):
        for definition, body in re.findall(r"^module\s+(\w+)\b(.*?)^endmodule", source.read_text(),
                                          re.MULTILINE | re.DOTALL):
            assert definition not in modules, definition
            modules[definition] = body
    roots = re.findall(r"^\s*(\w+)\s+dut\s*\(", (rtl / "design_top.sv").read_text(), re.MULTILINE)
    assert len(roots) == 1, roots
    definition = roots[0]
    for instance in instances:
        children = re.findall(r"^\s*(\w+)\s+" + re.escape(instance) + r"\s*\(",
                              modules[definition], re.MULTILINE)
        assert len(children) == 1, (definition, instance, children)
        definition = children[0]
    check = "pyc_check_error_" + ordinal
    formula = (f"wire {check} = (({check}_path & ~{check}_reset & "
               f"~{check}_condition) !== 1'b0);")
    assert formula in modules[definition], (definition, check)
    for suffix in ("path", "reset", "condition", "complete"):
        assert f"logic {check}_{suffix} = 1'b0;" in modules[definition], (definition, check, suffix)
    endpoint = ".".join((*instances, check))
    return endpoint, {"message": message, "native_header": str(header.relative_to(cpp)),
                      "rtl_module": definition, "rtl_endpoint": endpoint}


def substitute(text, names):
    for name, value in names.items():
        text = text.replace("@" + name + "@", value)
    assert not re.search(r"@[A-Z_]+@", text)
    return text


def execute(args, repo, fixture_dir, case, final, name, cli, run, tree):
    assert args.cxx and args.iverilog and args.vvp, "execution requires C++, iverilog and vvp tools"
    toolroot = Path(args.source_compiler).resolve().parent.parent
    runtime = next((p for p in (toolroot / "runtime/libpyc6_runtime.a",
                               toolroot / "lib/libpyc6_runtime.a") if p.is_file()), None)
    assert runtime, "same-checkout runtime archive is required"
    final_bytes = final.read_bytes()
    emitted = {}
    for target in ("cpp", "verilog"):
        generated = case / target
        cli("emit", final, "--target", target, "-o", generated)
        emitted[target] = generated
    assert final.read_bytes() == final_bytes
    cpp = emitted["cpp"]
    rtl = emitted["verilog"]
    native_receipt = json.loads((cpp / "generated.json").read_text())
    rtl_receipt = json.loads((rtl / "generated.json").read_text())
    native_sources = [cpp / item["path"] for item in native_receipt["files"]
                      if item["path"].endswith(".cpp")]
    native_runner = case / "native-system"
    run([args.cxx, "-std=c++20", "-pthread", "-I" + str(repo / "include"),
         "-I" + str(cpp), *native_sources, runtime, "-o", native_runner])
    rtl_sources = sorted((rtl / item["path"] for item in rtl_receipt["files"]
                          if item["role"] == "rtl"),
                         key=lambda path: (path.name != "design_top.sv", str(path)))
    assert (rtl / "simulation_top.sv").is_file()
    if rtl / "simulation_top.sv" not in rtl_sources:
        rtl_sources.append(rtl / "simulation_top.sv")
    monitor_arguments = []
    attribution = None
    memory_failure = name == "memory-late-failure"
    failure = name in ("late-failure", "partial-late-failure", "memory-late-failure")
    partial = name.startswith("partial-")
    source_failure_message = ("memory_routed_late_failure" if memory_failure else
                              "partial_late_failure" if partial else "routed_late_failure")
    if failure:
        names = (instance_names(cpp, source_names=(
                    "remote", "store", "branch", "worker", "memory", "progress", "held", "other_memory"),
                    state_names={"memory", "progress", "held", "other_memory"})
                 if memory_failure else instance_names(cpp, partial))
        if not partial:
            names["FAILED_CHECK"], attribution = failure_check_endpoint(
                cpp, rtl, final, source_failure_message)
            names["FAILED_MESSAGE"] = attribution["message"]
        if partial:
            bank = "`R." + names["REMOTE"] + "." + names["BANK"]
            paths = [bank + "." + names["PACKET"]]
            collection = "pyc_instances_" + names["ENTRIES"].removeprefix("pyc_instance_")
            paths.extend(f"{bank}.{collection}[{lane}].{names['ENTRIES']}" for lane in range(4))
            paths.extend("`R." + names[branch] + "." + names["WORKER"] + "." + names["PROGRESS"]
                         for branch in ("LEFT", "RIGHT"))
            paths.append("`R." + names["RIGHT"] + "." + names["DISABLED"] + "." + names["PROGRESS"])
            paths.append("`R." + names["PHASE"])
            paths.append("`R." + names["DISABLED_GATE"])
            names["STATE_VALUES"] = ", ".join(path + ".q_current" for path in paths)
            names["STATE_HISTORIES"] = ", ".join(path + ".managed.clock_current" for path in paths)
        monitor = case / "zero-commit-monitor.sv"
        monitor.write_text(substitute((fixture_dir / (name + ".sv")).read_text(), names))
        monitor_arguments = ["-s", "routed_zero_commit", monitor]
    rtl_runner = case / "rtl-system"
    run([args.iverilog, "-g2012", "-s", "pycircuit_sim", "-o", rtl_runner,
         *monitor_arguments, *sorted((repo / "include/verilog").glob("*.v")), *rtl_sources])
    transcripts = []
    finite = name.startswith(("ancestor-", "memory-")) or partial or name in ("sibling", "crossbranch", "reference-resources", "indexed-demand", "indexed-demand-conjunction")
    for backend, workers in (("cpp", 1), ("cpp", 2), ("verilog", 1)):
        command = [native_runner, "--workers", workers] if backend == "cpp" else [args.vvp, rtl_runner]
        if not finite:
            command += ["--cycles", 4] if backend == "cpp" else ["+cycles=4"]
        outcome = run(command, expected=1 if failure else 0)
        (case / f"{backend}-workers-{workers}.stdout").write_text(outcome.stdout)
        decoded = records(outcome.stdout)
        logs = [row for row in decoded if row["kind"] == "log"]
        if name == "memory-byte-repeated":
            logs.sort(key=lambda row: (int(row["evaluation_epoch"]), row["instance"]))
        if partial:
            # Unordered contributors have no semantic inter-Rule event order.
            # Compare each complete epoch by event name, preserving every event.
            logs.sort(key=lambda row: (int(row["evaluation_epoch"]), row["spec"]["event"]))
        expected = expected_logs(name)
        assert [(row["spec"]["event"], row["values"]) for row in logs] == expected, (name, backend, logs, expected)
        epochs = ([epoch for epoch, _, _ in memory_expectations(name)] if name.startswith("memory-") and not memory_failure else
                  [epoch for epoch in range(len(expected) // 3) for _ in range(3)]
                  if partial else list(range(len(expected))))
        assert [int(row["evaluation_epoch"]) for row in logs] == epochs
        assert [int(row["commit_epoch"]) for row in logs] == [epoch + 1 for epoch in epochs]
        terminal = [row for row in decoded if row["kind"] == "result"]
        if backend == "cpp" or not failure:
            assert len(terminal) == 1, (backend, terminal)
            assert terminal[0]["status"] == ("FAILED" if failure else "TERMINATED")
            assert int(terminal[0]["epoch_time"]) == (2 if failure else len(RESOURCE_SAMPLES)
                if name == "reference-resources" else 16 if name.startswith("memory-") else 4 if name in ("indexed-demand", "indexed-demand-conjunction") else len(SAMPLES) if finite else 8)
        if failure:
            if backend == "cpp":
                assert terminal[0]["error"]["code"] == "source_check_failed"
                assert terminal[0]["error"]["message"] == source_failure_message
            else:
                assert "ROUTED_ZERO_COMMIT_OK" in outcome.stdout
                assert "ROUTED_ZERO_COMMIT_FAIL" not in outcome.stdout
                if not partial:
                    assert "ROUTED_SOURCE_CHECK_OK " + source_failure_message in outcome.stdout
                assert source_failure_message in outcome.stdout + outcome.stderr
        transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]
    evidence = {"native_workers": [1, 2], "rtl": "iverilog-four-state",
                "published_observations": len(transcripts[0]),
                "same_final_sha256": hashlib.sha256(final_bytes).hexdigest()}
    if failure:
        original = tree(cpp)
        probe = case / "native-probe"
        shutil.copytree(cpp, probe)
        visibility = {}
        for header in probe.rglob("*.hpp"):
            before = header.read_text()
            after = "".join(line.replace("private:", "public:")
                            if line.strip() == "private:" else line
                            for line in before.splitlines(keepends=True))
            if before != after:
                header.write_text(after)
                visibility[str(header.relative_to(probe))] = {
                    "before_sha256": hashlib.sha256(before.encode()).hexdigest(),
                    "after_sha256": hashlib.sha256(after.encode()).hexdigest()}
        driver = case / "zero-commit.cpp"
        driver.write_text(substitute((fixture_dir / (name + ".cpp")).read_text(), names))
        sources = [probe / item["path"] for item in native_receipt["files"]
                   if item["path"].endswith(".cpp") and item["path"] != "simulation_main.cpp"]
        executable = case / "native-zero-commit"
        run([args.cxx, "-std=c++20", "-pthread", "-I" + str(repo / "include"),
             "-I" + str(probe), driver, *sources, runtime, "-o", executable])
        for workers in (1, 2):
            assert run([executable, workers]).stdout.strip() == "ROUTED_ZERO_COMMIT_OK"
        assert tree(cpp) == original
        evidence["zero_commit"] = {"storage": True, "clock_history": True,
                                    "epoch": True, "observations": True,
                                    "discard_retry": True, "visibility_only_probe": visibility}
        if attribution is not None:
            evidence["zero_commit"]["rtl_failure_attribution"] = attribution
        if memory_failure:
            evidence["zero_commit"]["ram"] = True
            evidence["zero_commit"]["transported_xz"] = True
    return evidence
