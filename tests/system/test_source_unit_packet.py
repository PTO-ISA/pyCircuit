"""Real Python Packet sources compile through the isolated source native harness."""

from __future__ import annotations

import os
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import pytest
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "tests/system/fixtures/record_source_units"
CANONICAL_PACKET = ROOT / "tests/integration/fixtures/source_language/packet.py"


@dataclass(frozen=True)
class UnitResult:
    completed: subprocess.CompletedProcess[str]
    transport: Path
    body: Path
    interface: Path


def _configured_harness() -> Path:
    candidates: list[str | None] = [os.environ.get("PYCIRCUIT_SOURCE_COMPILER")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/pycircuit-source-unit"))
    candidates.append(shutil.which("pycircuit-source-unit"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError(
        "set PYCIRCUIT_SOURCE_COMPILER or PYC_TOOLCHAIN_ROOT for source system tests"
    )


def _compile_unit(
    source: Path,
    *,
    source_root: Path,
    output_root: Path,
    headers: tuple[Path, ...] = (),
) -> UnitResult:
    captured = _capture_source_file(source, source_root=source_root)
    transport = output_root / f"{source.stem}.transport.mlir"
    body = output_root / f"{source.stem}.body.mlir"
    interface = output_root / f"{source.stem}.interface.mlir"
    transport.write_text(_emit_source_transport(captured), encoding="utf-8")
    command = [
        str(_configured_harness()),
        "--capture",
        str(transport),
        "--package",
        "demo",
        "--path",
        source.relative_to(source_root).as_posix(),
    ]
    for header in headers:
        command.extend(("--header", str(header)))
    command.extend(("--body-out", str(body), "--interface-out", str(interface)))
    completed = subprocess.run(
        command,
        text=True,
        capture_output=True,
        check=False,
    )
    return UnitResult(completed, transport, body, interface)


def _prepare_sources(tmp_path: Path) -> Path:
    source_root = tmp_path / "source"
    shutil.copytree(FIXTURES, source_root)
    return source_root


def _run_transport_text(
    tmp_path: Path,
    transport_text: str,
    *,
    source_path: str,
    headers: tuple[Path, ...] = (),
) -> subprocess.CompletedProcess[str]:
    transport = tmp_path / "mutated.transport.mlir"
    body = tmp_path / "mutated.body.mlir"
    interface = tmp_path / "mutated.interface.mlir"
    transport.write_text(transport_text, encoding="utf-8")
    command = [
        str(_configured_harness()),
        "--capture",
        str(transport),
        "--package",
        "demo",
        "--path",
        source_path,
    ]
    for header in headers:
        command.extend(("--header", str(header)))
    command.extend(("--body-out", str(body), "--interface-out", str(interface)))
    return subprocess.run(command, text=True, capture_output=True, check=False)


def _assert_diagnostic_failure(completed: subprocess.CompletedProcess[str]) -> None:
    assert completed.returncode > 0, completed.stderr
    assert completed.stderr.strip(), "native rejection must emit a diagnostic"


def _compile_packet(tmp_path: Path, source_root: Path) -> UnitResult:
    packet = _compile_unit(
        source_root / "packet.py", source_root=source_root, output_root=tmp_path
    )
    assert packet.completed.returncode == 0, packet.completed.stderr
    assert packet.body.is_file()
    assert packet.interface.is_file()
    return packet


def test_canonical_source_record_produces_body_and_interface(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "canonical"
    source_root.mkdir()
    shutil.copy2(CANONICAL_PACKET, source_root / "packet.py")

    packet = _compile_unit(
        source_root / "packet.py", source_root=source_root, output_root=tmp_path
    )

    assert packet.completed.returncode == 0, packet.completed.stderr
    assert packet.body.is_file()
    assert packet.interface.is_file()
    interface = packet.interface.read_text(encoding="utf-8")
    assert "demo.packet.Request" in interface
    assert "demo.packet.Request.__init__" in interface


def test_header_only_consumer_uses_defaults_kwargs_constructor_body_and_field_read(
    tmp_path: Path,
) -> None:
    source_root = _prepare_sources(tmp_path)
    packet = _compile_packet(tmp_path, source_root)
    consumer_transport = _capture_source_file(
        source_root / "consumer.py", source_root=source_root
    )
    saved_transport = tmp_path / "consumer.saved.transport.mlir"
    saved_transport.write_text(
        _emit_source_transport(consumer_transport), encoding="utf-8"
    )
    (source_root / "packet.py").unlink()
    packet.body.unlink()
    consumer = _compile_unit(
        source_root / "consumer.py",
        source_root=source_root,
        output_root=tmp_path,
        headers=(packet.interface,),
    )

    assert consumer.completed.returncode == 0, consumer.completed.stderr
    interface = consumer.interface.read_text(encoding="utf-8")
    assert "demo.packet.Request" in interface
    assert "ac.struct.create" in interface
    assert "ac.struct.get" in interface
    assert "demo.consumer.default_request" in interface
    assert "demo.consumer.keyword_request" in interface
    assert "demo.consumer.read_value" in interface
    assert saved_transport.read_text(encoding="utf-8") == consumer.transport.read_text(
        encoding="utf-8"
    )


@pytest.mark.parametrize(
    "expression, expected_terms",
    [
        ("Request(unknown=1)", ("unknown", "keyword")),
        ("Request(True, valid=False)", ("duplicate", "valid")),
        ("Request(False, 1, 2)", ("too many", "positional")),
        ("Required()", ("missing", "value")),
        ("Modes(positional=1, keyword=2)", ("positional", "keyword")),
        ("Modes(1, 2)", ("keyword", "positional")),
    ],
)
def test_constructor_binding_errors_fail_at_native_import(
    tmp_path: Path, expression: str, expected_terms: tuple[str, ...]
) -> None:
    source_root = _prepare_sources(tmp_path)
    packet = _compile_packet(tmp_path, source_root)
    failing = source_root / "failing.py"
    failing.write_text(
        "from .packet import Modes, Request, Required\n\n"
        "def construct() -> Request:\n"
        f"    return {expression}\n",
        encoding="utf-8",
    )

    result = _compile_unit(
        failing,
        source_root=source_root,
        output_root=tmp_path,
        headers=(packet.interface,),
    )

    assert result.completed.returncode != 0
    diagnostic = result.completed.stderr.lower()
    assert all(term in diagnostic for term in expected_terms), diagnostic


@pytest.mark.parametrize(
    "mutation",
    [
        "module_fields_wrong_kind",
        "module_body_missing",
        "module_body_wrong_kind",
        "module_kind_wrong_kind",
        "span_missing",
        "span_wrong_kind",
        "span_negative",
        "span_oversize",
        "span_reversed_bytes",
        "module_nonempty",
    ],
)
def test_malformed_capture_records_fail_with_diagnostics_not_crashes(
    tmp_path: Path, mutation: str
) -> None:
    source = tmp_path / "empty.py"
    source.write_text("", encoding="utf-8")
    captured = _capture_source_file(source, source_root=tmp_path)
    transport = _emit_source_transport(captured)
    fields = "fields = {body = [], type_ignores = []}"
    if mutation == "module_fields_wrong_kind":
        transport = transport.replace(fields, 'fields = "bad"', 1)
    elif mutation == "module_body_missing":
        transport = transport.replace(fields, "fields = {type_ignores = []}", 1)
    elif mutation == "module_body_wrong_kind":
        transport = transport.replace(
            fields, 'fields = {body = "bad", type_ignores = []}', 1
        )
    elif mutation == "module_kind_wrong_kind":
        transport = transport.replace('kind = "Module"', "kind = true", 1)
    elif mutation == "span_missing":
        transport = transport.replace("start_line = 1 : i64, ", "", 1)
    elif mutation == "span_wrong_kind":
        transport = re.sub(r"span = \{[^{}]+\}", 'span = "bad"', transport, count=1)
    elif mutation == "span_negative":
        transport = transport.replace(
            "start_line = 1 : i64", "start_line = -1 : i64", 1
        )
    elif mutation == "span_oversize":
        transport = transport.replace(
            "start_line = 1 : i64",
            "start_line = 18446744073709551616 : i128",
            1,
        )
    elif mutation == "span_reversed_bytes":
        transport = transport.replace(
            "start_byte_column = 1 : i64",
            "start_byte_column = 2 : i64",
            1,
        )
    elif mutation == "module_nonempty":
        transport = transport.replace(
            "{\n}\n",
            '{\n  "builtin.unrealized_conversion_cast"() : () -> ()\n}\n',
            1,
        )

    completed = _run_transport_text(tmp_path, transport, source_path="empty.py")

    _assert_diagnostic_failure(completed)


def test_malformed_keyword_node_fails_with_diagnostic_not_crash(
    tmp_path: Path,
) -> None:
    source_root = _prepare_sources(tmp_path)
    packet = _compile_packet(tmp_path, source_root)
    consumer = source_root / "malformed_keyword.py"
    consumer.write_text(
        "from .packet import Request\n\n"
        "def construct() -> Request:\n"
        "    return Request(valid=True)\n",
        encoding="utf-8",
    )
    captured = _capture_source_file(consumer, source_root=source_root)
    transport = _emit_source_transport(captured).replace(
        'arg = "valid"', 'arg = {integer = "0"}', 1
    )

    completed = _run_transport_text(
        tmp_path,
        transport,
        source_path="malformed_keyword.py",
        headers=(packet.interface,),
    )

    _assert_diagnostic_failure(completed)


@pytest.mark.parametrize(
    "source",
    [
        "import os\n",
        "value = dangerous_call()\n",
        "class DuplicateField:\n"
        "    value: bool\n"
        "    value: bool\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n",
        "class DuplicateConstructor:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n",
        "class VarArgs:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool, *args):\n"
        "        self.value = value\n",
        "class VarKeywords:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool, **kwargs):\n"
        "        self.value = value\n",
        "class ReceiverAnnotation:\n"
        "    value: bool\n"
        "    def __init__(self: ReceiverAnnotation, value: bool):\n"
        "        self.value = value\n",
        "class ReceiverDefault:\n"
        "    value: bool\n"
        "    def __init__(self=False, value: bool = True):\n"
        "        self.value = value\n",
        "from typing import Annotated\n"
        "Bad = Annotated[int, range(256, unexpected=True)]\n",
        "from typing import Annotated\n"
        "Bad = Annotated[int, range(256, **{'unexpected': True})]\n",
        "@decorate\n"
        "class Decorated:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n",
        "class Derived(Base):\n"
        "    value: bool\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n",
        "class MetaRecord(metaclass=Meta):\n"
        "    value: bool\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n",
        "class Generic[T]:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n",
        "class InitializedField:\n"
        "    value: bool = False\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n",
        "@decorate\ndef helper() -> bool:\n    return True\n",
        "def helper[T]() -> bool:\n    return True\n",
        "class ExtraField:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool):\n"
        "        self.value = value\n"
        "        self.extra = value\n",
    ],
)
def test_unsupported_source_forms_are_rejected_instead_of_silently_ignored(
    tmp_path: Path, source: str
) -> None:
    source_root = tmp_path / "source"
    source_root.mkdir()
    path = source_root / "unsupported.py"
    path.write_text(source, encoding="utf-8")

    result = _compile_unit(path, source_root=source_root, output_root=tmp_path)

    _assert_diagnostic_failure(result.completed)


def test_level_two_relative_import_is_rejected_instead_of_misresolved(
    tmp_path: Path,
) -> None:
    source_root = _prepare_sources(tmp_path)
    packet = _compile_packet(tmp_path, source_root)
    nested = source_root / "nested"
    nested.mkdir()
    consumer = nested / "level_two.py"
    consumer.write_text(
        "from ..packet import Request\n\n"
        "def make() -> Request:\n"
        "    return Request()\n",
        encoding="utf-8",
    )

    result = _compile_unit(
        consumer,
        source_root=source_root,
        output_root=tmp_path,
        headers=(packet.interface,),
    )

    _assert_diagnostic_failure(result.completed)


def test_dependencies_and_snapshots_are_canonical_under_reversed_import_order(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "source"
    source_root.mkdir()
    for stem in ("a", "z"):
        (source_root / f"{stem}.py").write_text(
            f"class {stem.upper()}:\n"
            "    value: bool\n"
            "    def __init__(self, value: bool):\n"
            "        self.value = value\n",
            encoding="utf-8",
        )
    a = _compile_unit(
        source_root / "a.py", source_root=source_root, output_root=tmp_path
    )
    z = _compile_unit(
        source_root / "z.py", source_root=source_root, output_root=tmp_path
    )
    assert a.completed.returncode == 0, a.completed.stderr
    assert z.completed.returncode == 0, z.completed.stderr
    ordered = source_root / "ordered.py"
    ordered.write_text(
        "from .z import Z\n"
        "from .a import A\n\n"
        "def make() -> A:\n"
        "    return A(value=True)\n",
        encoding="utf-8",
    )

    result = _compile_unit(
        ordered,
        source_root=source_root,
        output_root=tmp_path,
        headers=(z.interface, a.interface),
    )

    assert result.completed.returncode == 0, result.completed.stderr
    interface = result.interface.read_text(encoding="utf-8")
    assert interface.index('path = "a.py"') < interface.index('path = "z.py"')
    assert interface.index("demo.a.A") < interface.index("demo.z.Z")


@pytest.mark.parametrize(
    "mutation",
    [
        "class_bases_missing",
        "class_bases_wrong_kind",
        "class_keywords_missing",
        "class_keywords_wrong_kind",
        "annassign_value_missing",
        "annassign_value_wrong_kind",
        "annassign_simple_missing",
        "annassign_simple_wrong_kind",
        "arguments_args_wrong_kind",
        "arguments_defaults_wrong_kind",
        "arguments_vararg_wrong_kind",
        "arguments_kwarg_wrong_kind",
    ],
)
def test_malformed_class_field_and_argument_ast_shapes_are_rejected(
    tmp_path: Path, mutation: str
) -> None:
    source_root = tmp_path / "source"
    source_root.mkdir()
    source = source_root / "shape.py"
    source.write_text(
        "class Shape:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool = False):\n"
        "        self.value = value\n",
        encoding="utf-8",
    )
    captured = _capture_source_file(source, source_root=source_root)
    transport = _emit_source_transport(captured)
    replacements = {
        "class_bases_missing": ("bases = [], ", ""),
        "class_bases_wrong_kind": ("bases = []", 'bases = "bad"'),
        "class_keywords_missing": ("keywords = [], ", ""),
        "class_keywords_wrong_kind": ("keywords = []", 'keywords = "bad"'),
        "annassign_value_missing": ("value = unit, ", ""),
        "annassign_value_wrong_kind": ("value = unit", 'value = "bad"'),
        "annassign_simple_missing": (', simple = {integer = "1"}', ""),
        "annassign_simple_wrong_kind": (
            'simple = {integer = "1"}',
            "simple = true",
        ),
        "arguments_args_wrong_kind": ("args = [", 'args = "bad", ignored = ['),
        "arguments_defaults_wrong_kind": (
            "defaults = [",
            'defaults = "bad", ignored = [',
        ),
        "arguments_vararg_wrong_kind": ("vararg = unit", 'vararg = "bad"'),
        "arguments_kwarg_wrong_kind": ("kwarg = unit", 'kwarg = "bad"'),
    }
    old, new = replacements[mutation]
    assert old in transport
    transport = transport.replace(old, new, 1)

    completed = _run_transport_text(tmp_path, transport, source_path="shape.py")

    _assert_diagnostic_failure(completed)
