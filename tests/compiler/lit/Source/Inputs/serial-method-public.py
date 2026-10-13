"""Public serial Method unit closure, real lowering stages and execution oracle."""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "emitter", "optimizer", "cxx",
             "iverilog", "vvp", "scratch"):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
retained = Path(args.scratch).resolve()
retained.mkdir(parents=True, exist_ok=True)
# Keep every invocation's complete artifacts, including failures. Fresh output
# paths make reruns independent without weakening in-run no-clobber checks.
scratch = Path(tempfile.mkdtemp(prefix="run-", dir=retained)).resolve()
(retained / "latest-run.txt").write_text(str(scratch) + "\n")
env = dict(os.environ, PYTHONPATH=str(repo / "python"), PYTHONDONTWRITEBYTECODE="1",
           PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(command, accepted=True, expected_exit=None):
    command = list(map(str, command))
    result = subprocess.run(command, cwd=repo, env=env, capture_output=True,
                            text=True, timeout=240)
    expected_status = expected_exit if expected_exit is not None else (0 if accepted else 1)
    commands.append({"command": command, "exit_status": result.returncode,
                     "expected_exit": expected_status,
                     "stdout": result.stdout, "stderr": result.stderr})
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == expected_status, commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, commands[-1]
    if expected_status != 0:
        assert result.stderr.strip(), commands[-1]
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(path):
    return {p.relative_to(path).as_posix(): p.read_bytes()
            for p in path.rglob("*") if p.is_file()}


def payload(unit, kind):
    return unit / json.loads((unit / "unit.json").read_text())["files"][kind]


def emitted_source_text(bundle, source_owner, role):
    manifest = json.loads((bundle / "generated.json").read_text())
    group = next(group for group in manifest["source_groups"]
                 if group["source"] == source_owner)
    paths = [row["path"] for row in manifest["files"]
             if row["path"] in group["files"] and row["role"] == role]
    assert paths, (source_owner, role)
    return "\n".join((bundle / path).read_text() for path in paths)


def emitted_instance_name(actual, source_name):
    # FinalCppNames owns a common injective native/RTL spelling: simple names
    # remain literal; escaped identifiers are pyc_ plus UTF-8 hex. Select from
    # actual declarations rather than duplicating its reserved-keyword list.
    candidates = {"pyc_instance_" + source_name,
                  "pyc_instance_pyc_" + source_name.encode().hex()}
    found = actual & candidates
    assert len(found) == 1, (source_name, actual, found)
    return found.pop()


def compile_unit(source, path, output, imports=()):
    arguments = ["compile", "-c", path, "--source-root", source,
                 "--package-prefix", "serial_methods", "-o", output]
    for unit in imports:
        arguments.extend(("-I", unit))
    cli(*arguments)


def replay_arguments(units, top):
    # Match public _driver.py's SourceOwner ordering using the exact published
    # body/header bytes. No test-side merging or location/summary rewriting.
    ordered = sorted(units, key=lambda unit: tuple(
        json.loads((unit / "unit.json").read_text())["source"][key]
        for key in ("package", "path")))
    command = [args.linker]
    for unit in ordered:
        command.extend(("--body", payload(unit, "body"),
                        "--header", payload(unit, "interface")))
    return [*command, "--top", top, "--target", "final"]


def protection_checks(base, directory, units):
    directory.mkdir()
    sentinel = directory / "sentinel"
    sentinel.write_bytes(b"existing stage bytes\n")
    protected = {p: p.read_bytes() for unit in units
                 for p in unit.rglob("*") if p.is_file()}
    protected[sentinel] = sentinel.read_bytes()
    body_alias = directory / "body-alias.ac"
    body_alias.symlink_to(payload(units[0], "body"))
    cases = [
        ("missing-pair", ["--transaction-input-out", directory / "before.ac"]),
        ("verify-mode", ["--verify-only", "--transaction-input-out", directory / "before.ac",
                         "--transaction-output-out", directory / "after.ac"]),
        ("duplicate-stage-flag", ["--transaction-input-out", directory / "before.ac",
                                  "--transaction-input-out", directory / "extra-before.ac",
                                  "--transaction-output-out", directory / "after.ac"]),
        ("same-stage", ["--transaction-input-out", directory / "shared.ac",
                        "--transaction-output-out", directory / "shared.ac"]),
        ("input-alias", ["--transaction-input-out", body_alias,
                         "--transaction-output-out", directory / "after.ac"]),
        ("existing-stage", ["--transaction-input-out", sentinel,
                            "--transaction-output-out", directory / "after.ac"]),
        ("missing-parent", ["--transaction-input-out", directory / "before.ac",
                            "--transaction-output-out", directory / "absent" / "after.ac"]),
        ("final-alias", ["--transaction-input-out", directory / "final.ac",
                         "--transaction-output-out", directory / "after.ac"]),
        ("report-alias", ["--entry-owner-out", directory / "before.ac",
                          "--transaction-input-out", directory / "before.ac",
                          "--transaction-output-out", directory / "after.ac"]),
    ]
    for name, options in cases:
        # Native option-shape errors use the established usage status 2;
        # admitted link/output-protection failures use semantic status 1.
        expected_exit = 2 if name in ("missing-pair", "verify-mode", "duplicate-stage-flag") else 1
        run([*base, "--output", directory / "final.ac", *options],
            accepted=False, expected_exit=expected_exit)
        assert all(p.read_bytes() == content for p, content in protected.items()), name
        assert not any((directory / filename).exists()
                       for filename in ("final.ac", "before.ac", "after.ac", "shared.ac",
                                        "extra-before.ac")), name
    # A hard-link input alias must also be protected by actual file identity.
    hard_alias = directory / "hard-alias.ac"
    os.link(payload(units[0], "body"), hard_alias)
    run([*base, "--output", directory / "final.ac", "--transaction-input-out", hard_alias,
         "--transaction-output-out", directory / "after.ac"], accepted=False)
    assert all(p.read_bytes() == content for p, content in protected.items())
    assert not (directory / "final.ac").exists() and not (directory / "after.ac").exists()
    run([*base, "--output", sentinel, "--transaction-input-out", directory / "before.ac",
         "--transaction-output-out", directory / "after.ac"], accepted=False)
    assert all(p.read_bytes() == content for p, content in protected.items())
    assert not (directory / "before.ac").exists() and not (directory / "after.ac").exists()
    return [name for name, _ in cases] + ["hard-link-input-alias", "existing-final"]


toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next((p for p in (toolroot / "runtime/libpyc6_runtime.a",
                           toolroot / "lib/libpyc6_runtime.a") if p.is_file()), None)
assert runtime is not None, "Runtime archive missing from this checkout's compiler build/install"
owners = ("count", "before", "middle", "saved", "after", "old_seen", "payload",
          "copied", "held", "identity")
summaries = []
cases = [(mode, *values) for mode in ("local", "cross", "root")
         for values in ((7, 5, 1, 2, 3, 4), (13, 4091, 17, 29, 7, 31))]
for mode, width, initial, first, second, bias, last in cases:
    case = scratch / f"{mode}-width-{width}"
    case.mkdir()
    source = case / "source"
    source.mkdir()
    provider_name = "local_owner.py" if mode == "local" else "cross_provider.py"
    owner_text = (fixtures / "serial-method" / provider_name).read_text()
    provider_path = source / provider_name
    text = owner_text.replace("u7", f"u{width}").replace("= 5\n", f"= {initial}\n")
    text = text.replace("bias = 3\n", f"bias = {bias}\n")
    text = text.replace("add(1)", f"add({first})").replace("chain(2)", f"chain({second})")
    text = text.replace("add(4)", f"add({last})")
    if width == 13:
        text = text.replace("    step()\n    inspect()", "    inspect()\n    step()")
    provider_path.write_text(text)
    (case / provider_name).write_text(text)
    provider = case / "provider"
    compile_unit(source, provider_path, provider)
    # Parent compilation sees only the published header and receipt. Restore
    # exact body/depfile bytes for the explicit link closure afterward.
    hidden = case / "unavailable-provider"
    hidden.mkdir()
    moved = (provider_path, payload(provider, "body"), payload(provider, "depfile"))
    for path in moved:
        path.rename(hidden / path.name)
    imports = [provider]
    if mode != "local":
        relay_path = source / "cross_relay.py"
        relay_text = (fixtures / "serial-method/cross_relay.py").read_text()
        relay_text = relay_text.replace("add(1)", f"add({first})")
        relay_text = relay_text.replace("chain(2)", f"chain({second})")
        relay_text = relay_text.replace("add(4)", f"add({last})")
        relay_path.write_text(relay_text)
        (case / relay_path.name).write_text(relay_text)
        relay = case / "relay"
        compile_unit(source, relay_path, relay, imports)
        relay_hidden = hidden / "relay"
        relay_hidden.mkdir()
        relay_moved = (relay_path, payload(relay, "body"), payload(relay, "depfile"))
        for path in relay_moved:
            path.rename(relay_hidden / path.name)
        imports.append(relay)
        parent_path = source / "cross_consumer.py"
        parent_text = (fixtures / "serial-method/cross_consumer.py").read_text()
        if width == 13:
            parent_text = parent_text.replace("    advance_left()\n    advance_right()",
                                              "    advance_right()\n    advance_left()")
        parent_path.write_text(parent_text)
    else:
        parent_path = source / "composition.py"
        parent_path.write_text("from pycircuit import module\n"
                              "from serial_methods.local_owner import LocalOwner\n"
                              "@module\ndef Top():\n"
                              "    left = LocalOwner()\n    right = LocalOwner()\n")
    (case / parent_path.name).write_text(parent_path.read_text())
    parent = case / "parent"
    compile_unit(source, parent_path, parent, imports)
    depfile = payload(parent, "depfile").read_text()
    assert str(payload(provider, "interface")) in depfile
    assert str(provider_path) not in depfile and str(payload(provider, "body")) not in depfile
    assert not provider_path.exists()
    if mode != "local":
        assert str(payload(relay, "interface")) in depfile
        assert str(relay_path) not in depfile and str(payload(relay, "body")) not in depfile
        assert not relay_path.exists()
        for path in relay_moved[1:]:
            (relay_hidden / path.name).rename(path)
    for path in moved[1:]:
        (hidden / path.name).rename(path)
    units = (parent, *imports)
    top = ("serial_methods.composition.Top" if mode == "local" else
           "serial_methods.cross_provider.Counter" if mode == "root" else
           "serial_methods.cross_consumer.Top")
    final = case / "public-final.ac"
    cli("link", *units, "--top", top, "-o", final)
    base = replay_arguments(units, top)
    replay, before, after = (case / filename for filename in
                            ("replay-final.ac", "before-transactions.ac", "after-transactions.ac"))
    run([*base, "--output", replay, "--transaction-input-out", before,
         "--transaction-output-out", after])
    assert replay.read_bytes() == final.read_bytes() == after.read_bytes()
    before_text, final_text = before.read_text(), final.read_text()
    assert '"ac.transaction"' in before_text and '"ac.method.call"' in before_text
    assert '"ac.state"' in before_text and '"ac.rule"' in final_text
    assert before_text.count('"ac.module"') == final_text.count('"ac.module"')
    assert final_text.count('"ac.instance"') == (before_text.count('"ac.instance"')
                                                + before_text.count('"ac.state"'))
    assert all(f'"{operation}"' not in final_text for operation in
               ("ac.state", "ac.state.read", "ac.state.write", "ac.method",
                "ac.method.call", "ac.transaction"))
    run([args.optimizer, final, "--ac-verify-hardware", "-o", case / "verified-final.ac"])
    protected_cases = protection_checks(base, case / "protection", units)
    # Diagnostic retention belongs only to native final linking. Ordinary emit
    # and verify entrances reject the paired flags before creating any output.
    for entrance, options in (("emit", ["--target", "cpp", "--output", case / "bad-emitted.cpp"]),
                              ("verify", ["--verify-only"])):
        run([args.linker, "--design", final, *options,
             "--transaction-input-out", case / "bad-before.ac",
             "--transaction-output-out", case / "bad-after.ac"],
            accepted=False, expected_exit=2)
        assert not any((case / filename).exists() for filename in
                       ("bad-before.ac", "bad-after.ac", "bad-emitted.cpp")), entrance
        protected_cases.append(entrance + "-stage-flags")
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", case / target)
        groups = json.loads((case / target / "generated.json").read_text())["source_groups"]
        assert sorted(group["source"]["path"] for group in groups) == sorted(
            json.loads((unit / "unit.json").read_text())["source"]["path"] for unit in units)
    provider_owner = json.loads((provider / "unit.json").read_text())["source"]
    parent_owner = json.loads((parent / "unit.json").read_text())["source"]
    owner_header = emitted_source_text(case / "cpp", provider_owner, "header")
    owner_rtl = emitted_source_text(case / "verilog", provider_owner, "rtl")
    native_states = set(re.findall(
        r"(?m)^\s*std::shared_ptr<gfsim::collection_storage<[^;\n]+>>\s+(pyc_instance_\w+)_state;",
        owner_header))
    rtl_states = set(re.findall(r"\bdffe\b[^\n]*\s+(pyc_instance_\w+)\s*\(", owner_rtl))
    assert native_states == rtl_states and len(native_states) == len(owners)
    owner_names = {name: emitted_instance_name(native_states, name) for name in owners}
    parent_header = emitted_source_text(case / "cpp", parent_owner, "header")
    parent_rtl = emitted_source_text(case / "verilog", parent_owner, "rtl")
    native_instances = set(re.findall(
        r"(?m)^\s*std::shared_ptr<[^;\n]*::pyc_family_\w+<pyc_count>>\s+(pyc_instance_\w+);",
        parent_header))
    rtl_instances = set(re.findall(r"\b(pyc_instance_\w+)\s*\(", parent_rtl))
    assert native_instances == rtl_instances and len(native_instances) == (2 if mode == "local" else 3)
    instance_names = {name: emitted_instance_name(native_instances, name)
                      for name in (("left", "right") if mode == "local" else ("left", "right", "idle"))}
    cpp_owners = [f"family->{instance_names[side]}" for side in ("left", "right")]
    rtl_owners = [f"dut.dut.{instance_names[side]}" for side in ("left", "right")]
    if mode != "local":
        relay_owner = json.loads((relay / "unit.json").read_text())["source"]
        relay_header = emitted_source_text(case / "cpp", relay_owner, "header")
        relay_rtl = emitted_source_text(case / "verilog", relay_owner, "rtl")
        relay_native_instances = set(re.findall(
            r"(?m)^\s*std::shared_ptr<[^;\n]*::pyc_family_\w+<pyc_count>>\s+(pyc_instance_\w+);",
            relay_header))
        relay_rtl_instances = set(re.findall(r"\b(pyc_instance_\w+)\s*\(", relay_rtl))
        assert relay_native_instances == relay_rtl_instances and len(relay_native_instances) == 1
        counter_name = emitted_instance_name(relay_native_instances, "counter")
        cpp_owners = [owner + "->" + counter_name for owner in cpp_owners]
        rtl_owners = [owner + "." + counter_name for owner in rtl_owners]
        cpp_owners.append(f"family->{instance_names['idle']}")
        rtl_owners.append(f"dut.dut.{instance_names['idle']}")
        if mode == "root":
            cpp_owners, rtl_owners = ["family"], ["dut.dut"]
    # Reuse the existing queue-source storage probe pattern. Every native
    # translation unit and the driver use the same copied headers, with only
    # standalone access specifiers changed; no mixed class definitions or
    # modified transition/Work calculations enter this execution.
    cpp_originals = snapshot(case / "cpp")
    probe = case / "native-probe"
    shutil.copytree(case / "cpp", probe)
    visibility = {}
    for header in probe.rglob("*.hpp"):
        original = header.read_text()
        changed = "".join(line.replace("private:", "public:")
                          if line.strip() == "private:" else line
                          for line in original.splitlines(keepends=True))
        before_lines, after_lines = original.splitlines(), changed.splitlines()
        assert len(before_lines) == len(after_lines), header
        changed_lines = []
        for number, (left_line, right_line) in enumerate(zip(before_lines, after_lines, strict=True), 1):
            assert left_line == right_line or (left_line.strip() == "private:"
                                               and right_line.strip() == "public:"), (header, number)
            if left_line != right_line:
                changed_lines.append(number)
        if changed_lines:
            header.write_text(changed)
            visibility[header.relative_to(probe).as_posix()] = {
                "original_sha256": hashlib.sha256(original.encode()).hexdigest(),
                "probe_sha256": digest(header), "access_specifier_lines": changed_lines}
    cpp_probes, rtl_probes = [], []
    for native_owner, rtl_owner in zip(cpp_owners, rtl_owners, strict=True):
        for owner in owners:
            stem = owner_names[owner]
            cpp_probes.append(f"probe(*{native_owner}->{stem}_state, {native_owner}->{stem}_d, "
                              f"{native_owner}->{stem}_en);")
            rtl_probes.append(f'$write(" %b %b %b %b", {rtl_owner}.{stem}.q, '
                              f'{rtl_owner}.{stem}.d, {rtl_owner}.{stem}.en, pyc_7079635f636c6b);')
    left = cpp_owners[0]
    injection_cpp = "\n".join(
        f'if ({token} != "-") inject(*{left}->{owner_names[owner]}_state, '
        f'{left}->{owner_names[owner]}_q, {token});'
        for token, owner in (("count", "count"), ("payload", "payload")))
    injection_rtl = "\n".join(
        f'if ({token}_text != "-") begin\n'
        f'  if ($sscanf({token}_text, "%b", {token}_bits) != 1) $fatal(1, "invalid injection");\n'
        f'  {rtl_owners[0]}.{owner_names[owner]}.q_current = {token}_bits;\nend'
        for token, owner in (("count", "count"), ("payload", "payload")))
    driver = case / "driver.cpp"
    default_checks = []
    if mode == "root":
        physical_inputs = re.findall(r"(?m)^  input .* (\w+),?$", owner_rtl)
        assert physical_inputs[:2] == ["pyc_7079635f636c6b", "pyc_7079635f727374"]
        assert len(physical_inputs) > 2
        for port in physical_inputs[2:]:
            default_checks.append(f"static_assert(!requires(Root value) {{ value.{port}; }});")
            default_checks.append(f"require_zero(root.pyc_implementation->{port}.element(0));")
    driver.write_text((fixtures / "serial-method-public.cpp").read_text()
                      .replace("@WIDTH@", str(width)).replace("@INJECT@", injection_cpp)
                      .replace("@DEFAULT_CHECKS@", "\n".join(default_checks))
                      .replace("@PROBES@", "\n".join(cpp_probes)))
    bench = case / "tb.sv"
    bench.write_text((fixtures / "serial-method-public.sv").read_text()
                     .replace("@WIDTH_MINUS_ONE@", str(width - 1)).replace("@INJECT@", injection_rtl)
                     .replace("@PROBES@", "\n".join(rtl_probes)))
    def bits(value, active_width=width):
        return format(value % (1 << active_width), f"0{active_width}b")

    mixed = "z" + "0" * (width - 3) + "x1"
    rows = [(0, 0, 0, bits(initial + 19), mixed), (1, 0, 0, "-", "-"),
            (1, 0, 0, "-", "-"), (0, 0, 0, "-", "-"),
            (1, 0, 1, "-", "-"), (1, 0, 1, "-", "-"),
            (1, 0, 0, "-", "-"), (0, 0, 0, "-", "-"),
            (1, 1, 1, "-", "-"), (1, 0, 0, "-", "-"),
            (0, 0, 0, "-", "-"), (1, 1, 0, "-", "-"),
            (1, 0, 0, "-", "-"), (0, 0, 0, bits((1 << width) - 2), "x" * width),
            (1, 0, 0, "-", "-"), (0, 0, 0, "-", "z" * width),
            (1, 0, 0, "-", "-"), (0, 0, 0, "-", "-"),
            (1, 1, 0, "-", "-"), (0, 0, 0, "-", "-"),
            (1, 0, 0, "-", "-"), (0, 0, 0, "-", "-")]
    stimuli = case / "stimuli.txt"
    stimuli.write_text("".join(" ".join(map(str, row)) + "\n" for row in rows))
    receipt = json.loads((probe / "generated.json").read_text())
    cpp = [probe / row["path"] for row in receipt["files"] if row["path"].endswith(".cpp")]
    executable = case / "native-runner"
    run([args.cxx, "-std=c++20", "-pthread", "-I" + str(repo / "include"),
         "-I" + str(probe), driver, *cpp, runtime, "-o", executable])
    traces = []
    for workers in (1, 2):
        trace = case / f"native-workers-{workers}.txt"
        trace.write_text(run([executable, workers, stimuli]).stdout)
        traces.append(trace)
    assert traces[0].read_bytes() == traces[1].read_bytes()
    if mode == "root":
        for workers in (1, 2):
            trace = case / f"native-direct-root-workers-{workers}.txt"
            trace.write_text(run([executable, workers, stimuli, "direct"]).stdout)
            traces.append(trace)
    rtl_receipt = json.loads((case / "verilog/generated.json").read_text())
    rtl = [case / "verilog" / row["path"] for row in rtl_receipt["files"] if row["role"] == "rtl"]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    rtl_executable = case / "rtl-runner"
    run([args.iverilog, "-g2012", "-s", "tb", "-o", rtl_executable,
         repo / "include/verilog/dffe.v", *rtl, bench])
    trace = case / "rtl-four-state.txt"
    trace.write_text(run([args.vvp, rtl_executable, "+stimuli=" + str(stimuli)]).stdout)
    traces.append(trace)
    oracle = [sys.executable, repo / "tests/compiler/oracles/serial_method_oracle.py",
              "--stimuli", stimuli, "--mode", mode]
    for name, value in (("width", width), ("initial", initial), ("first", first),
                        ("second", second), ("bias", bias), ("last", last)):
        oracle.extend(("--" + name, value))
    for trace in traces:
        oracle.extend(("--trace", trace))
    run(oracle)
    assert snapshot(case / "cpp") == cpp_originals
    summaries.append({"mode": mode, "width": width, "frames": len(rows), "provider_python_unavailable": True,
                      "provider_body_unavailable_during_consumer_compile": True,
                      "before_sha256": digest(before), "after_sha256": digest(after),
                      "public_final_sha256": digest(final), "replay_final_sha256": digest(replay),
                      "native_probe": "access-only instrumented generated headers; no behavior patch",
                      "visibility": visibility, "generated_cpp_originals_preserved": True,
                      "probe_owners": owner_names, "probe_instances": instance_names,
                      "protected_failures": protected_cases})

tracked = subprocess.run(["git", "ls-files", "-co", "--exclude-standard", "-z"],
                          cwd=repo, capture_output=True, check=True).stdout
candidate_files = {name.decode(): digest(repo / name.decode())
                   for name in sorted(set(tracked.split(b"\0"))) if name and (repo / name.decode()).is_file()}
candidate_head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=repo,
                                capture_output=True, text=True, check=True).stdout.strip()
(scratch / "receipt.json").write_text(json.dumps({
    "scope": "same-owner and cross-instance true-guard whole-Bits Method execution with selected-root ABI",
    "cases": summaries, "candidate_head": candidate_head, "candidate_content_sha256": hashlib.sha256(
        json.dumps(candidate_files, sort_keys=True).encode()).hexdigest(),
    "candidate_files": candidate_files,
    "tools": {str(Path(tool).resolve()): digest(Path(tool).resolve()) for tool in
           (args.source_compiler, args.linker, args.emitter, args.optimizer)},
    "artifact_sha256": {path.relative_to(scratch).as_posix(): digest(path)
                     for path in sorted(scratch.rglob("*")) if path.is_file()}
}, indent=2) + "\n")
print("serial Method public closure passed: native workers1/2 and actual four-state RTL")
