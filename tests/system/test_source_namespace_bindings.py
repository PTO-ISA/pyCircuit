"""N1 source namespace tests for records, aliases, constants, and helpers."""

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


@dataclass(frozen=True)
class Unit:
    completed: subprocess.CompletedProcess[str]
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
    raise AssertionError("set ACIR_SOURCE_UNIT_HARNESS for N1 system tests")


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
        body,
        interface,
    )


def _write(root: Path, name: str, source: str) -> Path:
    path = root / name
    path.write_text(source, encoding="utf-8")
    return path


def _module_attribute(text: str, name: str, following: str) -> str:
    return text.split(f"{name} = ", 1)[1].split(f", {following} = ", 1)[0]


def _binding_table(array_text: str) -> list[tuple[str, str]]:
    return re.findall(
        r'\{name = "([^"]+)", site = .*?target = @([^},]+)\}',
        array_text,
    )


PROVIDER = """\
from typing import Annotated

Word = Annotated[int, range(256)]
LIMIT = 7
ALSO_LIMIT = 7
ENABLED = True

class Payload:
    value: Word
    def __init__(self, value: Word = 7):
        self.value = value

class Unused:
    value: bool
    def __init__(self, value: bool = False):
        self.value = value

class _Private:
    value: bool
    def __init__(self, value: bool = False):
        self.value = value

def Make() -> Payload:
    return Payload()
"""


def _provider(tmp_path: Path) -> tuple[Path, Unit]:
    root = tmp_path / "source"
    root.mkdir()
    source = _write(root, "provider.py", PROVIDER)
    unit = _compile(source, root=root, output=tmp_path / "provider-out")
    assert unit.completed.returncode == 0, unit.completed.stderr
    text = unit.interface.read_text(encoding="utf-8")
    assert re.search(
        r'sym_name = "demo\.provider\.LIMIT", type = \{kind = "integer"\}, '
        r'value = \{kind = "integer", value = #ac\.math_int<7>\}',
        text,
    )
    assert re.search(
        r'sym_name = "demo\.provider\.ALSO_LIMIT", '
        r'type = \{kind = "integer"\}, '
        r'value = \{kind = "integer", value = #ac\.math_int<7>\}',
        text,
    )
    assert re.search(
        r'sym_name = "demo\.provider\.ENABLED", type = \{kind = "bool"\}, '
        r'value = \{kind = "bool", value = true\}',
        text,
    )
    return root, unit


def test_aliases_private_and_unused_imports_have_exact_namespace_mapping(
    tmp_path: Path,
) -> None:
    root, provider = _provider(tmp_path)
    facade_source = _write(
        root,
        "facade.py",
        "from .provider import (Payload as Item, Word as Count, LIMIT,\n"
        "                       ALSO_LIMIT as Same,\n"
        "                       ENABLED as Switch, Make,\n"
        "                       Unused as NeverUsed, _Private as _Hidden)\n"
        "Local = Count\n\n"
        "def Read() -> Count:\n"
        "    return Make().value\n",
    )
    facade = _compile(
        facade_source,
        root=root,
        output=tmp_path / "facade-out",
        headers=(provider.interface,),
    )
    assert facade.completed.returncode == 0, facade.completed.stderr

    text = facade.interface.read_text(encoding="utf-8")
    exports = _binding_table(
        _module_attribute(text, "ac.exports", "ac.import_bindings")
    )
    imports = _binding_table(
        _module_attribute(text, "ac.import_bindings", "ac.interfaces")
    )
    assert exports == [
        ("Count", "demo.provider.Word"),
        ("Item", "demo.provider.Payload"),
        ("LIMIT", "demo.provider.LIMIT"),
        ("Local", "demo.facade.Local"),
        ("Make", "demo.provider.Make"),
        ("NeverUsed", "demo.provider.Unused"),
        ("Read", "demo.facade.Read"),
        ("Same", "demo.provider.ALSO_LIMIT"),
        ("Switch", "demo.provider.ENABLED"),
        ("_Hidden", "demo.provider._Private"),
    ]
    assert imports == [
        ("ALSO_LIMIT", "demo.provider.ALSO_LIMIT"),
        ("ENABLED", "demo.provider.ENABLED"),
        ("LIMIT", "demo.provider.LIMIT"),
        ("Make", "demo.provider.Make"),
        ("Payload", "demo.provider.Payload"),
        ("Unused", "demo.provider.Unused"),
        ("Word", "demo.provider.Word"),
        ("_Private", "demo.provider._Private"),
    ]
    body_text = facade.body.read_text(encoding="utf-8")
    assert _module_attribute(text, "ac.exports", "ac.import_bindings") == (
        _module_attribute(body_text, "ac.exports", "ac.import_bindings")
    )
    assert _module_attribute(text, "ac.import_bindings", "ac.interfaces") == (
        _module_attribute(body_text, "ac.import_bindings", "ac.interfaces")
    )
    assert '"ac.constant"() <{sym_name = "demo.provider.LIMIT"' in text
    assert '"ac.constant"() <{sym_name = "demo.provider.ALSO_LIMIT"' in text
    assert '"ac.constant"() <{sym_name = "demo.provider.ENABLED"' in text


def test_chained_facade_uses_only_headers_and_clones_transitive_make_value(
    tmp_path: Path,
) -> None:
    root, provider = _provider(tmp_path)
    facade_source = _write(
        root,
        "facade.py",
        "from .provider import (Payload as Item, Word as Count, LIMIT as Bound,\n"
        "                       ALSO_LIMIT as Mirror, ENABLED, Make)\n",
    )
    facade = _compile(
        facade_source,
        root=root,
        output=tmp_path / "facade-out",
        headers=(provider.interface,),
    )
    assert facade.completed.returncode == 0, facade.completed.stderr
    consumer_source = _write(
        root,
        "consumer.py",
        "from .facade import (Item, Count, Bound as Limit, Mirror as Same,\n"
        "                     ENABLED as Active, Make)\n\n"
        "def Read() -> Count:\n"
        "    return Make().value\n",
    )

    (root / "provider.py").unlink()
    (root / "facade.py").unlink()
    provider.body.unlink()
    facade.body.unlink()
    first = _compile(
        consumer_source,
        root=root,
        output=tmp_path / "consumer-forward",
        headers=(provider.interface, facade.interface),
    )
    second = _compile(
        consumer_source,
        root=root,
        output=tmp_path / "consumer-reverse",
        headers=(facade.interface, provider.interface),
    )
    assert first.completed.returncode == 0, first.completed.stderr
    assert second.completed.returncode == 0, second.completed.stderr
    assert first.body.read_bytes() == second.body.read_bytes()
    assert first.interface.read_bytes() == second.interface.read_bytes()
    interface = first.interface.read_text(encoding="utf-8")
    assert "@demo.provider.Make" in interface
    assert "@demo.provider.Payload" in interface
    assert "@demo.provider.Payload.__init__" in interface
    assert '"ac.constant"() <{sym_name = "demo.provider.LIMIT"' in interface
    assert '"ac.constant"() <{sym_name = "demo.provider.ALSO_LIMIT"' in interface
    assert '"ac.constant"() <{sym_name = "demo.provider.ENABLED"' in interface
    assert "ac.struct.get" in interface
    exports = _binding_table(
        _module_attribute(interface, "ac.exports", "ac.import_bindings")
    )
    imports = _binding_table(
        _module_attribute(interface, "ac.import_bindings", "ac.interfaces")
    )
    assert ("Active", "demo.provider.ENABLED") in exports
    assert ("Limit", "demo.provider.LIMIT") in exports
    assert ("Same", "demo.provider.ALSO_LIMIT") in exports
    assert ("Bound", "demo.provider.LIMIT") in imports
    assert ("Mirror", "demo.provider.ALSO_LIMIT") in imports
    assert ("ENABLED", "demo.provider.ENABLED") in imports
    assert dict(exports)["Limit"] != dict(exports)["Same"]


def test_multiple_aliases_and_later_local_shadow_preserve_import_uses(
    tmp_path: Path,
) -> None:
    root, provider = _provider(tmp_path)
    source = _write(
        root,
        "aliases.py",
        "from .provider import Payload as A, Payload as B\n"
        "from .provider import Word as Thing\n\n"
        "class Thing:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool = False):\n"
        "        self.value = value\n",
    )
    unit = _compile(
        source,
        root=root,
        output=tmp_path / "aliases-out",
        headers=(provider.interface,),
    )
    assert unit.completed.returncode == 0, unit.completed.stderr
    text = unit.interface.read_text(encoding="utf-8")
    exports = _binding_table(
        _module_attribute(text, "ac.exports", "ac.import_bindings")
    )
    imports = _binding_table(
        _module_attribute(text, "ac.import_bindings", "ac.interfaces")
    )
    assert exports == [
        ("A", "demo.provider.Payload"),
        ("B", "demo.provider.Payload"),
        ("Thing", "demo.aliases.Thing"),
    ]
    assert imports == [
        ("Payload", "demo.provider.Payload"),
        ("Payload", "demo.provider.Payload"),
        ("Word", "demo.provider.Word"),
    ]


def test_later_shadow_does_not_rescue_an_invalid_import(tmp_path: Path) -> None:
    root, provider = _provider(tmp_path)
    source = _write(
        root,
        "invalid.py",
        "from .provider import Missing\n\n"
        "class Missing:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool = False):\n"
        "        self.value = value\n",
    )
    unit = _compile(
        source,
        root=root,
        output=tmp_path / "invalid-out",
        headers=(provider.interface,),
    )
    assert unit.completed.returncode != 0
    assert "does not export 'Missing'" in unit.completed.stderr


def test_rebinding_cannot_silently_retarget_definition_time_record_default(
    tmp_path: Path,
) -> None:
    root = tmp_path / "source"
    root.mkdir()
    provider_source = _write(
        root,
        "provider.py",
        "class A:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool = False):\n"
        "        self.value = value\n\n"
        "class B:\n"
        "    value: bool\n"
        "    def __init__(self, value: bool = True):\n"
        "        self.value = value\n",
    )
    provider = _compile(provider_source, root=root, output=tmp_path / "provider-out")
    assert provider.completed.returncode == 0, provider.completed.stderr
    source = _write(
        root,
        "defaults.py",
        "from .provider import A as T\n\n"
        "def Choose(value: T = T()) -> T:\n"
        "    return value\n\n"
        "from .provider import B as T\n",
    )
    unit = _compile(
        source,
        root=root,
        output=tmp_path / "defaults-out",
        headers=(provider.interface,),
    )

    assert unit.completed.returncode != 0, (
        "record-construction defaults require definition-time binding; "
        "accepting this source with the final T=B binding is a semantic bug"
    )
    assert unit.completed.stderr.strip()


def test_module_facade_preserves_canonical_child_definition(tmp_path: Path) -> None:
    root = tmp_path / "source"
    root.mkdir()
    provider_source = _write(
        root,
        "provider.py",
        "from typing import Annotated\n"
        "from pycircuit import module, rule\n"
        "Word = Annotated[int, range(256)]\n\n"
        "@module\n"
        "class Pass:\n"
        "    def __init__(self, source: Word, sink: Word):\n"
        "        self.source = source\n"
        "        self.sink = sink\n"
        "        self.sink = self.forward(self.source)\n\n"
        "    @rule\n"
        "    def forward(self, value: Word) -> Word:\n"
        "        return value\n",
    )
    provider = _compile(provider_source, root=root, output=tmp_path / "provider-out")
    assert provider.completed.returncode == 0, provider.completed.stderr
    facade_source = _write(
        root,
        "facade.py",
        "from .provider import Pass as Channel, Word\n",
    )
    facade = _compile(
        facade_source,
        root=root,
        output=tmp_path / "facade-out",
        headers=(provider.interface,),
    )
    assert facade.completed.returncode == 0, facade.completed.stderr

    consumer_source = _write(
        root,
        "consumer.py",
        "from pycircuit import module\n"
        "from .facade import Channel, Word\n\n"
        "@module\n"
        "class Root:\n"
        "    def __init__(self):\n"
        "        self.a: Word = 1\n"
        "        self.b: Word = 2\n"
        "        self.x: Word = 0\n"
        "        self.y: Word = 0\n"
        "        self.left = Channel(self.a, self.x)\n"
        "        self.right = Channel(self.b, self.y)\n",
    )
    provider_source.unlink()
    facade_source.unlink()
    provider.body.unlink()
    facade.body.unlink()
    consumer = _compile(
        consumer_source,
        root=root,
        output=tmp_path / "consumer-out",
        headers=(provider.interface, facade.interface),
    )
    assert consumer.completed.returncode == 0, consumer.completed.stderr
    facade_text = facade.interface.read_text(encoding="utf-8")
    consumer_text = consumer.interface.read_text(encoding="utf-8")
    facade_exports = dict(
        _binding_table(
            _module_attribute(facade_text, "ac.exports", "ac.import_bindings")
        )
    )
    consumer_imports = dict(
        _binding_table(
            _module_attribute(consumer_text, "ac.import_bindings", "ac.interfaces")
        )
    )
    assert facade_exports["Channel"] == "demo.provider.Pass"
    assert consumer_imports["Channel"] == "demo.provider.Pass"
    body_text = consumer.body.read_text(encoding="utf-8")
    instances = [line for line in body_text.splitlines() if '"ac.instance"' in line]
    assert len(instances) == 2
    assert all("callee = @demo.provider.Pass" in line for line in instances)
    assert 'name = "left"' in instances[0]
    assert 'name = "right"' in instances[1]
    handles = [re.search(r'"ac.instance"\(([^)]*)\)', line) for line in instances]
    assert all(handle is not None for handle in handles)
    assert handles[0].group(1) != handles[1].group(1)

    imports = _module_attribute(facade_text, "ac.import_bindings", "ac.interfaces")
    stale_imports = imports.replace(
        "target = @demo.provider.Pass", "target = @demo.provider.Word", 1
    )
    assert stale_imports != imports
    stale_header = tmp_path / "stale-facade.interface.mlir"
    stale_header.write_text(
        facade_text.replace(
            f"ac.import_bindings = {imports}", f"ac.import_bindings = {stale_imports}"
        ),
        encoding="utf-8",
    )
    stale = _compile(
        consumer_source,
        root=root,
        output=tmp_path / "stale-consumer-out",
        headers=(provider.interface, stale_header),
    )
    assert stale.completed.returncode != 0
    assert "stale import binding" in stale.completed.stderr
