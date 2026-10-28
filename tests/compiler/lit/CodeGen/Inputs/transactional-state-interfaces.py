"""Independent parent State binding, Rule interface and export oracles."""

import json
import re
import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest

fixture = SourceTest(
    sys.argv[1:], "parent_state_interfaces", "example_parent_state_interfaces"
)
repo, scratch, sources = fixture.repo, fixture.scratch, fixture.sources
emitter, cxx, opt = fixture.emitter, fixture.cxx, fixture.opt
target = sys.argv[8]
reset = sys.argv[9:] == ["reset"]
run, cli, compile_source = fixture.run, fixture.cli, fixture.compile_source
artifact, interfaces_only = fixture.artifact, fixture.interfaces_only
if reset:
    # Module adapters observe exported values directly. System log publication
    # is checked above in the ordinary mode; bare Module RTL has no log runner.
    for name in ("top.py", "stage.py"):
        source = sources / name
        source.write_text(
            re.sub(
                r"    @ac.autorule\n    def observe\(\) -> None:\n        ac.log[^\n]*\n",
                "",
                source.read_text(),
            )
        )
    top = sources / "top.py"
    text = (
        top.read_text()
        .replace("@ac.system", "@ac.module")
        .replace("def Top():", "def Top() -> tuple[ac.u8, ac.u8, ac.u8, ac.u8]:")
    )
    top.write_text(text + "\n    return incremented, first, second, alias\n")
compile_source("rules")
compile_source("stage")
with interfaces_only("rules", "stage"):
    compile_source("top", "rules", "stage")

# A mismatched existing output State must reject without replacing the valid
# published body, interface or receipt. Check the complete unit bytes as well.
published = scratch / "top"
before = {
    p.relative_to(published): p.read_bytes()
    for p in published.rglob("*")
    if p.is_file()
}
top_source = sources / "top.py"
original = top_source.read_text()
assert "first: ac.u8 = 20" in original
top_source.write_text(original.replace("first: ac.u8 = 20", "first: ac.u16 = 20"))
compile_source("top", "rules", "stage", replace=True, fails=True)
after = {
    p.relative_to(published): p.read_bytes()
    for p in published.rglob("*")
    if p.is_file()
}
assert after == before, "failed mismatched binding changed the published unit"
top_source.write_text(original)

# No original Python or provider body reads are available to substitute for link.
shutil.rmtree(sources)
final = scratch / "state-interfaces.ac"
cli(
    "link",
    scratch / "rules",
    scratch / "stage",
    scratch / "top",
    "--top",
    "example_parent_state_interfaces.top.Top",
    "-o",
    final,
)
run([opt, final, "--ac-verify-hardware"])


def check_storage_owners():
    # Inspect actual final Module/instance edges, excluding the serialized source
    # provenance attribute. Symbols may specialize; their spelling is immaterial.
    modules = {}
    storage = set()
    current = None
    for line in final.read_text().splitlines():
        if line.startswith('  "ac.module.import"'):
            current = None
            if re.search(r'primitive_kind = "dffe?"', line):
                storage.add(re.search(r'sym_name = "([^"]+)"', line)[1])
        elif line.startswith('  "ac.module"'):
            symbol = re.search(r'sym_name = "([^"]+)"', line)[1]
            owner = re.search(r'source_owner = \{[^}]*path = "([^"]+)"', line)[1]
            current = {"owner": owner, "children": {}}
            modules[symbol] = current
        elif current is not None and "ac.instance(" in line:
            name = re.search(r'instance_name = "([^"]+)"', line)[1]
            callee = re.search(r'callee = @(?:(?:"([^"]+)")|([\w.$-]+))', line)
            current["children"][name] = callee[1] or callee[2]
    # The parent owns its five declared States; each Stage owns only accepted.
    # Export and all resource bindings allocate no replacement storage.
    counts = Counter()

    def visit(symbol):
        module = modules[symbol]
        for child in module["children"].values():
            if child in storage:
                counts[module["owner"]] += 1
            else:
                visit(child)

    visit("example_parent_state_interfaces.top.Top")
    assert counts == {"top.py": 5, "stage.py": 2}, counts


check_storage_owners()
bundle = scratch / target
cli("emit", final, "--target", target, "-o", bundle)
generated = json.loads((bundle / "generated.json").read_text())
# Source ownership must survive resource binding and specialization.
assert {row["source"]["path"] for row in generated["source_groups"]} >= {
    "stage.py",
    "top.py",
}
executable = scratch / f"state-interfaces-{target}.exe"
if target == "cpp":
    implementations = [
        bundle / row["path"]
        for row in generated["files"]
        if row["path"].endswith(".cpp")
    ]
    runtime = Path(emitter).parent.parent / "runtime/libpyc6_runtime.a"
    if not runtime.is_file():
        runtime = Path(emitter).parent.parent / "lib/libpyc6_runtime.a"
    assert runtime.is_file(), runtime
    run(
        [
            cxx,
            "-std=c++20",
            "-pthread",
            f"-I{repo / 'include'}",
            f"-I{bundle}",
            *implementations,
            *([Path(__file__).with_suffix(".cpp")] if reset else []),
            runtime,
            "-o",
            executable,
        ]
    )
elif reset:
    rtl = scratch / "reset.sv"
    rtl.write_text(
        run([emitter, final, "--target", "verilog", "--format", "source"]).stdout
    )
    run(
        [
            "iverilog",
            "-g2012",
            "-s",
            "tb",
            "-o",
            executable,
            rtl,
            repo / "include/verilog/dff.v",
            repo / "include/verilog/dffe.v",
            Path(__file__).with_suffix(".sv"),
        ]
    )
else:
    run(
        [
            sys.executable,
            repo / "cmake/verify_example.py",
            "--compile-system",
            bundle,
            "--output",
            executable,
            "--include",
            repo / "include",
        ]
    )


if reset:
    if target == "cpp":
        for workers in (1, 2):
            run([executable, workers])
    else:
        run(["vvp", executable])
    print(f"transactional parent State interfaces reset {target}: PASS")
    sys.exit(0)


def observations(result):
    rows = [
        json.loads(line) for line in result.stdout.splitlines() if line.startswith("{")
    ]
    assert any(row.get("kind") == "result" for row in rows), rows
    logs = Counter()
    for row in rows:
        if row.get("kind") != "log":
            continue
        values = []
        for value in row["values"]:
            if value["kind"] == "bits4":
                assert int(value["known"], 16) == (1 << value["width"]) - 1
                assert int(value["z"], 16) == 0
                values.append(int(value["value"], 16))
            else:
                values.append(int(value["value"]))
        epoch = int(row["evaluation_epoch"])
        event = row["spec"]["event"]
        logs[(epoch, event, tuple(values))] += 1
    return logs


# Derive each row from old Q: the first Stage initially sees incremented=0,
# then 4. Its own accepted update cannot forward into the returned sum.
# Once accepted reaches 2, output and internal state both hold.
expected = Counter()
for cycle, (incremented, first, second, accepted) in enumerate(
    ((0, 20, 30, 0), (4, 0, 10, 1), (4, 5, 11, 2), (4, 5, 11, 2))
):
    for epoch in (cycle * 2, cycle * 2 + 1):
        expected[(epoch, "parent", (3, 10, incremented, first, second, first))] += 1
        expected[(epoch, "stage", (accepted, first))] += 1
        expected[(epoch, "stage", (accepted, second))] += 1
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "4", "--workers", workers] if target == "cpp" else ["+cycles=4"]
    )
    actual = observations(run([executable, *arguments]))
    assert actual == expected, (actual, expected)
print(f"transactional parent State interfaces {target}: PASS")
