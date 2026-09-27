"""U02-A source-module tests using the approved migration C1 fixtures."""

from __future__ import annotations

import os
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import pytest
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "tests/integration/pycircuit/fixtures/migration_c1"

ACCUMULATOR_PROBE = """\
from pycircuit import module, rule
from .packet import Request, Word

@module
class AccumulatorProbe:
    def __init__(self, request: Request, result: Word):
        self.request = request
        self.result = result
        self.total: Word = 0
        self.result = self.forward(self.request)

    @rule
    def forward(self, item: Request) -> Word:
        return item.value
"""

PROBE_ROOT = """\
from pycircuit import module
from .packet import Request, Word
from .accumulator_probe import AccumulatorProbe

@module
class ProbeRoot:
    def __init__(self):
        self.request: Request = Request(3, True)
        self.result: Word = 0
        self.child = AccumulatorProbe(self.request, self.result)
"""


@dataclass(frozen=True)
class Unit:
    completed: subprocess.CompletedProcess[str]
    transport: Path
    body: Path
    interface: Path


def _harness() -> Path:
    candidates: list[str | None] = [os.environ.get("ACIR_SOURCE_UNIT_HARNESS")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/acir-source-unit-harness"))
    candidates.append(shutil.which("acir-source-unit-harness"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError("set ACIR_SOURCE_UNIT_HARNESS for U02-A system tests")


def _compile(
    source: Path,
    *,
    root: Path,
    output: Path,
    headers: tuple[Path, ...] = (),
) -> Unit:
    output.mkdir(parents=True, exist_ok=True)
    capture = _capture_source_file(source, source_root=root)
    transport = output / f"{source.stem}.transport.mlir"
    body = output / f"{source.stem}.body.mlir"
    interface = output / f"{source.stem}.interface.mlir"
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    command = [
        str(_harness()),
        "--capture",
        str(transport),
        "--package",
        "demo",
        "--path",
        source.relative_to(root).as_posix(),
    ]
    for header in headers:
        command.extend(("--header", str(header)))
    command.extend(("--body-out", str(body), "--interface-out", str(interface)))
    return Unit(
        subprocess.run(command, text=True, capture_output=True, check=False),
        transport,
        body,
        interface,
    )


def _prepare(tmp_path: Path) -> Path:
    source = tmp_path / "source"
    source.mkdir()
    shutil.copy2(FIXTURES / "packet.py", source / "packet.py")
    (source / "accumulator_probe.py").write_text(ACCUMULATOR_PROBE, encoding="utf-8")
    (source / "probe_root.py").write_text(PROBE_ROOT, encoding="utf-8")
    return source


def _compile_probe(tmp_path: Path) -> tuple[Path, Unit, Unit]:
    source = _prepare(tmp_path)
    packet = _compile(source / "packet.py", root=source, output=tmp_path / "packet-out")
    assert packet.completed.returncode == 0, packet.completed.stderr
    probe = _compile(
        source / "accumulator_probe.py",
        root=source,
        output=tmp_path / "probe-out",
        headers=(packet.interface,),
    )
    assert probe.completed.returncode == 0, probe.completed.stderr
    return source, packet, probe


def _operation_line(text: str, operation: str) -> str:
    matches = [line.strip() for line in text.splitlines() if operation in line]
    assert len(matches) == 1, matches
    return matches[0]


def _module_attribute(text: str, name: str, following: str) -> str:
    return text.split(f"{name} = ", 1)[1].split(f", {following} = ", 1)[0]


def _balanced(text: str, start: int, opening: str, closing: str) -> tuple[int, int]:
    assert text[start] == opening
    depth = 0
    for index in range(start, len(text)):
        if text[index] == opening:
            depth += 1
        elif text[index] == closing:
            depth -= 1
            if depth == 0:
                return start, index + 1
    raise AssertionError(f"unclosed {opening}{closing} record")


def _top_level_records(array_text: str) -> list[str]:
    records: list[str] = []
    index = 1
    while index < len(array_text) - 1:
        if array_text[index] in " ,":
            index += 1
            continue
        start, end = _balanced(array_text, index, "{", "}")
        records.append(array_text[start:end])
        index = end
    return records


def _replace_balanced_field(record: str, field: str, replacement: str) -> str:
    marker = f"{field} = "
    start = record.index(marker) + len(marker)
    _, end = _balanced(record, start, "{", "}")
    return record[:start] + replacement + record[end:]


def _mutate_module_header(
    interface: Path,
    output: Path,
    *,
    static_default: bool | None = None,
    connection_default: bool = False,
) -> Path:
    text = interface.read_text(encoding="utf-8")
    lines = text.splitlines(keepends=True)
    line_index = next(i for i, line in enumerate(lines) if "ac.module.import" in line)
    line = lines[line_index]
    marker = "parameters = "
    array_start = line.index(marker) + len(marker)
    _, array_end = _balanced(line, array_start, "[", "]")
    array_text = line[array_start:array_end]
    parameters = _top_level_records(array_text)
    assert len(parameters) == 2
    if static_default is not None:
        static_parameter = parameters[1].replace('name = "result"', 'name = "scale"')
        static_parameter = static_parameter.replace(
            'category = "connection"', 'category = "static"'
        )
        static_parameter = _replace_balanced_field(
            static_parameter, "type", '{kind = "integer"}'
        )
        default = (
            '{present = true, value = {kind = "integer", value = #ac.math_int<4>}}'
            if static_default
            else "{present = false}"
        )
        static_parameter = _replace_balanced_field(static_parameter, "default", default)
        replacement = array_text[:-1] + ", " + static_parameter + "]"
        line = line[:array_start] + replacement + line[array_end:]
    if connection_default:
        old = "default = {present = false}"
        first = line.index(old)
        second = line.index(old, first + len(old))
        replacement = (
            'default = {present = true, value = {kind = "integer", '
            "value = #ac.math_int<0>}}"
        )
        line = line[:second] + replacement + line[second + len(old) :]
    lines[line_index] = line
    output.write_text("".join(lines), encoding="utf-8")
    return output


def _compile_root_source(
    tmp_path: Path,
    source: Path,
    packet: Unit,
    probe_header: Path,
    text: str,
    stem: str,
) -> Unit:
    path = source / f"{stem}.py"
    path.write_text(text, encoding="utf-8")
    return _compile(
        path,
        root=source,
        output=tmp_path / f"{stem}-out",
        headers=(packet.interface, probe_header),
    )


def test_real_packet_then_probe_emit_body_and_interface(tmp_path: Path) -> None:
    source, packet, probe = _compile_probe(tmp_path)

    assert packet.interface.is_file()
    assert probe.body.is_file()
    assert probe.interface.is_file()
    assert probe.transport.read_text(encoding="utf-8") == _emit_source_transport(
        _capture_source_file(source / "accumulator_probe.py", source_root=source)
    )


def test_probe_header_matches_the_approved_c2_module_contract(
    tmp_path: Path,
) -> None:
    _, _, probe = _compile_probe(tmp_path)
    interface = probe.interface.read_text(encoding="utf-8")
    declaration = _operation_line(interface, "ac.module.import")

    assert "demo.accumulator_probe.AccumulatorProbe" in declaration
    assert 'path = "accumulator_probe.py"' in declaration
    assert declaration.index('name = "request"') < declaration.index('name = "result"')
    assert declaration.count('category = "connection"') == 2
    assert declaration.count('binding = "positional_or_keyword"') == 2
    assert declaration.count("default = {present = false}") == 2
    assert declaration.count('precision = "exact"') == 2
    assert declaration.count("{ordinal, origins") == 2
    assert declaration.count("read = true") == 1
    assert declaration.count("read = false") == 1
    assert declaration.count("write = true") == 1
    assert declaration.count("write = false") == 1
    assert "demo.packet.Request" in declaration
    assert "demo.packet.Word" not in declaration
    assert 'kind = "integer"' in declaration
    assert "storage = i8" in declaration
    assert "lower = #ac.math_int<0>" in declaration
    assert "upper = #ac.math_int<256>" in declaration
    imports = _module_attribute(interface, "ac.import_bindings", "ac.interfaces")
    assert 'name = "Word"' in imports
    assert "target = @demo.packet.Word" in imports


def test_probe_body_and_header_mirror_unit_metadata(tmp_path: Path) -> None:
    _, _, probe = _compile_probe(tmp_path)
    body = probe.body.read_text(encoding="utf-8")
    interface = probe.interface.read_text(encoding="utf-8")

    for name, following in (
        ("ac.exports", "ac.import_bindings"),
        ("ac.import_bindings", "ac.interfaces"),
        ("ac.interfaces", "ac.source_owner"),
    ):
        assert _module_attribute(body, name, following) == _module_attribute(
            interface, name, following
        )
    assert "ac.module.import" not in body
    assert "ac.module.import" in interface
    assert 'ac.type_alias "demo.packet.Word"' in body
    assert 'ac.struct "demo.packet.Request"' in body
    assert 'ac.declaration_role = "import_snapshot"' in body
    assert 'path = "packet.py"' in _module_attribute(
        interface, "ac.interfaces", "ac.source_owner"
    )


def test_probe_root_uses_headers_without_provider_source_or_body(
    tmp_path: Path,
) -> None:
    source, packet, probe = _compile_probe(tmp_path)
    (source / "packet.py").unlink()
    (source / "accumulator_probe.py").unlink()
    packet.body.unlink()
    probe.body.unlink()

    root = _compile(
        source / "probe_root.py",
        root=source,
        output=tmp_path / "root-out",
        headers=(packet.interface, probe.interface),
    )

    assert root.completed.returncode == 0, root.completed.stderr
    body = root.body.read_text(encoding="utf-8")
    assert "ac.instance" in body
    assert "demo.accumulator_probe.AccumulatorProbe" in body
    assert 'ac.module.import "demo.accumulator_probe.AccumulatorProbe"' in body
    assert 'ac.declaration_role = "import_snapshot"' in body
    assert 'parameter = "request"' in body
    assert 'parameter = "result"' in body
    assert "@demo.packet.Request.__init__" in body
    assert "#ac.math_int<3>" in body
    assert "value = true" in body
    assert "#ac.math_int<0>" in body


@pytest.mark.parametrize(
    "inactive_body",
    [
        "        return\n",
        "        print(item.value)\n        return item.value\n",
        "        return self.missing\n",
        "        return missing\n",
        "        return False\n",
    ],
)
def test_malformed_unregistered_rules_are_rejected(
    tmp_path: Path, inactive_body: str
) -> None:
    source = _prepare(tmp_path)
    packet = _compile(source / "packet.py", root=source, output=tmp_path / "packet-out")
    assert packet.completed.returncode == 0, packet.completed.stderr
    malformed = ACCUMULATOR_PROBE + (
        "\n    @rule\n    def inactive(self, item: Request) -> Word:\n" + inactive_body
    )
    path = source / "inactive_probe.py"
    path.write_text(malformed, encoding="utf-8")

    result = _compile(
        path,
        root=source,
        output=tmp_path / "inactive-out",
        headers=(packet.interface,),
    )

    assert result.completed.returncode != 0
    assert result.completed.stderr.strip()


def test_valid_unregistered_rule_is_inactive(tmp_path: Path) -> None:
    source = _prepare(tmp_path)
    packet = _compile(source / "packet.py", root=source, output=tmp_path / "packet-out")
    assert packet.completed.returncode == 0, packet.completed.stderr
    valid = ACCUMULATOR_PROBE + (
        "\n    @rule\n"
        "    def inactive(self, item: Request) -> Word:\n"
        "        return self.result\n"
    )
    path = source / "inactive_probe.py"
    path.write_text(valid, encoding="utf-8")
    result = _compile(
        path,
        root=source,
        output=tmp_path / "inactive-out",
        headers=(packet.interface,),
    )

    assert result.completed.returncode == 0, result.completed.stderr
    body = result.body.read_text(encoding="utf-8")
    assert body.count('name = "forward"') == 1
    assert 'name = "inactive"' not in body
    interface = result.interface.read_text(encoding="utf-8")
    declaration = _operation_line(interface, "ac.module.import")
    assert 'parameter = "result"' in declaration
    assert declaration.count("read = false") == 1
    assert declaration.count("write = true") == 1


@pytest.mark.parametrize(
    "expression",
    [
        "False",
        "Missing(3, True)",
        "Word(3)",
        "Request(3, value=4)",
        "Request(unknown=3)",
    ],
)
def test_invalid_source_reset_expressions_are_rejected(
    tmp_path: Path, expression: str
) -> None:
    source, packet, probe = _compile_probe(tmp_path)
    root_text = PROBE_ROOT.replace("Request(3, True)", expression)
    result = _compile_root_source(
        tmp_path, source, packet, probe.interface, root_text, "invalid_reset"
    )

    assert result.completed.returncode != 0
    assert result.completed.stderr.strip()


def test_wrong_nominal_record_reset_is_rejected(tmp_path: Path) -> None:
    source, packet, probe = _compile_probe(tmp_path)
    other = source / "other.py"
    other.write_text(
        "class Other:\n"
        "    value: Word\n"
        "    valid: bool\n"
        "    def __init__(self, value: Word = 0, valid: bool = False):\n"
        "        self.value = value\n"
        "        self.valid = valid\n",
        encoding="utf-8",
    )
    other_source = other.read_text(encoding="utf-8").replace(
        "class Other:", "from .packet import Word\n\nclass Other:"
    )
    other.write_text(other_source, encoding="utf-8")
    other_unit = _compile(
        other,
        root=source,
        output=tmp_path / "other-out",
        headers=(packet.interface,),
    )
    assert other_unit.completed.returncode == 0, other_unit.completed.stderr
    root_text = PROBE_ROOT.replace(
        "from .accumulator_probe import AccumulatorProbe",
        "from .accumulator_probe import AccumulatorProbe\nfrom .other import Other",
    ).replace("Request(3, True)", "Other(3, True)")
    path = source / "wrong_nominal.py"
    path.write_text(root_text, encoding="utf-8")
    result = _compile(
        path,
        root=source,
        output=tmp_path / "wrong-nominal-out",
        headers=(packet.interface, probe.interface, other_unit.interface),
    )

    assert result.completed.returncode != 0
    assert result.completed.stderr.strip()


@pytest.mark.parametrize(
    ("static_default", "call"),
    [
        (False, "AccumulatorProbe(self.request, self.result)"),
        (True, "AccumulatorProbe(self.request, self.result)"),
        (False, "AccumulatorProbe(self.request, self.result, 4)"),
    ],
)
def test_child_static_parameters_never_lower_to_empty_static_args(
    tmp_path: Path, static_default: bool, call: str
) -> None:
    source, packet, probe = _compile_probe(tmp_path)
    mutated = _mutate_module_header(
        probe.interface,
        tmp_path / "probe-static.interface.mlir",
        static_default=static_default,
    )
    root_text = PROBE_ROOT.replace("AccumulatorProbe(self.request, self.result)", call)
    result = _compile_root_source(
        tmp_path, source, packet, mutated, root_text, "static_parent"
    )

    assert result.completed.returncode != 0
    assert result.completed.stderr.strip()


def test_connection_default_metadata_requires_an_explicit_handle(
    tmp_path: Path,
) -> None:
    source, packet, probe = _compile_probe(tmp_path)
    mutated = _mutate_module_header(
        probe.interface,
        tmp_path / "probe-default.interface.mlir",
        connection_default=True,
    )
    explicit = _compile_root_source(
        tmp_path, source, packet, mutated, PROBE_ROOT, "explicit_parent"
    )
    assert explicit.completed.returncode == 0, explicit.completed.stderr

    for stem, call in (
        ("omitted_parent", "AccumulatorProbe(self.request)"),
        ("literal_parent", "AccumulatorProbe(self.request, 0)"),
    ):
        result = _compile_root_source(
            tmp_path,
            source,
            packet,
            mutated,
            PROBE_ROOT.replace("AccumulatorProbe(self.request, self.result)", call),
            stem,
        )
        assert result.completed.returncode != 0
        assert result.completed.stderr.strip()
