"""Independent resource identity, multiple-output and transaction oracle."""

import shutil
import sys
from pathlib import Path

from transactional_test_support import SourceTest, log_history

fixture = SourceTest(sys.argv[1:], None, "example_resource_composition")
repo, scratch, sources = fixture.repo, fixture.scratch, fixture.sources
opt = fixture.opt
target, mode = sys.argv[-2:]
run, cli, compile_source = fixture.run, fixture.cli, fixture.compile_source
artifact, interfaces_only = fixture.artifact, fixture.interfaces_only

if mode == "composition":
    shutil.copytree(repo / "examples/resource_composition", sources, dirs_exist_ok=True)
else:
    shutil.copyfile(
        Path(__file__).with_name("transactional-resource-composition-cases.py"),
        sources / "cases.py",
    )


if mode == "composition":
    units = ("counter", "split_stage", "relay", "observer", "Top")
    compile_source("counter")
    compile_source("split_stage")
    compile_source("observer")
    with interfaces_only("split_stage"):
        compile_source("relay", "split_stage")
    with interfaces_only(*units[:-1]):
        compile_source("Top", *units[:-1])
    top = "Top.Top"
else:
    units = ("cases",)
    compile_source("cases")
    top = {
        "mixed": "cases.Mixed",
        "separate": "cases.Separate",
    }[mode]
shutil.rmtree(sources)
final = scratch / "composition.ac"
result = cli(
    "link",
    *(scratch / unit for unit in units),
    "--top",
    f"example_resource_composition.{top}",
    "-o",
    final,
)
run([opt, final, "--ac-verify-hardware"])
executable, generated = fixture.build_system(final, target)
sys.path.insert(0, str(repo / "tests/compiler/oracles"))
from resource_composition import expected_trace  # noqa: E402

expected = expected_trace(mode, 20)
for workers in (1, 2) if target == "cpp" else (1,):
    arguments = (
        ["--cycles", "20", "--workers", workers] if target == "cpp" else ["+cycles=20"]
    )
    actual = log_history(run([executable, *arguments]))
    assert actual == expected, {
        "unexpected": actual - expected,
        "missing": expected - actual,
    }
print(f"transactional resource composition {mode} {target}: PASS")
