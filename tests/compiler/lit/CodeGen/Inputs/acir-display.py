"""Reading views preserve source-generated resource and transaction structure."""

import hashlib
import json
import shutil
import sys
from pathlib import Path

from transactional_test_support import SourceTest

fixture = SourceTest(sys.argv[1:], "structured_packet", "example_structured_packet")
fixture.env["PYCIRCUIT_PRINTER"] = str(Path(fixture.opt).with_name("pycircuit-print"))
fixture.compile_source("types")
fixture.compile_source("processor", "types")
fixture.compile_source("Top", "types", "processor")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def display(path, *options):
    before = digest(path)
    text = fixture.cli("print", path, *options).stdout
    assert digest(path) == before, "printing changed its input"
    return text


unlinked = fixture.artifact("processor", "body")
readable = display(unlinked)
assert "reading view" in readable.lower(), readable[:500]
for keyword in ("ac.module", "ac.rule", "ac.method", "process", "!ac.struct"):
    assert keyword in readable, keyword
assert "ast_path" not in readable
assert "source_owner" not in readable
assert "!ac.bits<8>" in readable
for requirement in ("ac.methods", "result_requirement", "!ac.infer", "!ac.primitive"):
    assert requirement in readable, requirement
assert "ast_path" in display(unlinked, "--profile", "debug")

linked = fixture.scratch / "pipeline.linked.ac"
fixture.cli(
    "link",
    *(fixture.scratch / unit for unit in ("types", "processor", "Top")),
    "--top",
    "example_structured_packet.Top.Top",
    "--target",
    "linked",
    "-o",
    linked,
)
linked_view = display(linked)
for keyword in ("ac.method.call", "guard", "!ac.state", "!ac.queue", "!ac.enum"):
    assert keyword in linked_view, keyword
assert "ast_path" not in linked_view

config = fixture.scratch / "display.json"
config.write_text(json.dumps({"ops": {"ac.module": {"fields": ["sym_name"]}}}))
custom = display(unlinked, "--config", config)


def module_lines(text):
    return [
        line for line in text.splitlines() if line.lstrip().startswith("ac.module ")
    ]


assert module_lines(readable)
assert any("output_kinds" in line for line in module_lines(readable))
assert all("output_kinds" not in line for line in module_lines(custom))
assert "process" in custom

full = fixture.scratch / "full.ac"
before_full = digest(linked)
fixture.cli("print", linked, "--profile", "full", "-o", full)
assert digest(linked) == before_full
fixture.run([fixture.opt, full, "-o", fixture.scratch / "reparsed.ac"])

# Reuse the existing branch-bearing source, keeping this a printer contract test.
shutil.copyfile(
    Path(__file__).with_name("transactional-control-flow-design.py"),
    fixture.sources / "conditional.py",
)
fixture.compile_source("conditional")
branch_linked = fixture.scratch / "conditional.linked.ac"
fixture.cli(
    "link",
    fixture.scratch / "conditional",
    "--top",
    "example_structured_packet.conditional.Top",
    "--target",
    "linked",
    "-o",
    branch_linked,
)
branches = display(branch_linked)
assert "scf.if" in branches
assert "else" in branches

# An unconfigured operation must keep every ordinary structural component.
generic = fixture.scratch / "generic.ac"
generic.write_text(
    "module { func.func @identity(%arg: i8) -> i8 { "
    "%sum = arith.addi %arg, %arg : i8 "
    '%result = "reader.extension"(%sum) <{mode = "required"}> ({ ^bb0(%inner: i8): '
    '"reader.yield"(%inner) : (i8) -> () '
    '}) {note = "kept"} : (i8) -> i8 '
    "func.return %result : i8 } }\n"
)
generic_view = display(generic)
for keyword in (
    "func.func",
    "func.return",
    "identity",
    "i8",
    "arith.addi",
    "reader.extension",
    "reader.yield",
    "region",
    "note",
    "kept",
    "mode",
    "required",
):
    assert keyword in generic_view, keyword
extension = next(
    line for line in generic_view.splitlines() if "reader.extension" in line
)
assert " = " in extension and "%" in extension and "->" in extension

# The display never binds symbolic static expressions to instance arguments.
symbolic = display(fixture.repo / "tests/compiler/lit/IR/collection-generic.mlir")
assert "!ac.table<" in symbolic
assert "N" in symbolic
assert "@generic" in symbolic
assert "ast_path" not in symbolic
