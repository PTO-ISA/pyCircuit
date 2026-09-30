"""Independent C2-DECL source inventory, final-only, and C++ header oracles."""

from __future__ import annotations

import json
import re
import shutil
import subprocess
from pathlib import Path

import pytest
import test_cpp_source_parts as cpp

pytestmark = pytest.mark.system

PROVIDER = """\
from typing import Annotated
Word = Annotated[int, range(256)]
Unused = Annotated[int, range(16)]
_Hidden = Annotated[int, range(8)]
Flag = bool
UnsignedOne = Annotated[int, range(2)]
SignedOne = Annotated[int, range(2)]
NarrowSigned = Annotated[int, range(256)]
Std = Annotated[int, range(16)]
Gfsim = Annotated[int, range(16)]
FalseValue = False
TrueValue = True
Zero = 0
One = 1
MinusOne = 1
Minimum = 0
Maximum = 18446744073709551615
"""

ROOT_SOURCE = """\
from typing import Annotated
from pycircuit import module
from .facade import PublicWord
Own = Annotated[int, range(32)]
Std = Annotated[int, range(16)]
Gfsim = Annotated[int, range(16)]
Limit = 9
@module
def Root():
    state: PublicWord = 7
"""

PROVIDER_NAMES = {
    "Word",
    "Unused",
    "_Hidden",
    "Flag",
    "UnsignedOne",
    "SignedOne",
    "NarrowSigned",
    "Std",
    "Gfsim",
    "FalseValue",
    "TrueValue",
    "Zero",
    "One",
    "MinusOne",
    "Minimum",
    "Maximum",
}
ROOT_NAMES = {"Own", "Std", "Gfsim", "Limit"}


def _balanced(text: str, start: int) -> int:
    """Match braces while ignoring quoted MLIR strings."""
    assert text[start] == "{"
    depth = 0
    quoted = escaped = False
    for index in range(start, len(text)):
        char = text[index]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if not depth:
                return index
    raise AssertionError("unclosed MLIR region or attribute")


def _units(text: str) -> dict[tuple[str, str], str]:
    units = {}
    for match in re.finditer(r"(?m)^  module attributes \{", text):
        attributes = text.index("{", match.start())
        attributes_end = _balanced(text, attributes)
        owner = re.search(
            r'ac.source_owner = \{package = "([^"]*)", path = "([^"]+)"\}',
            text[attributes : attributes_end + 1],
        )
        assert owner, text[match.start() : attributes_end + 1]
        body = text.index("{", attributes_end + 1)
        end = _balanced(text, body) + 1
        key = owner.group(1), owner.group(2)
        assert key not in units, f"duplicate unit owner: {key}"
        units[key] = text[match.start() : end]
    assert units, "final package contains no source-owned units"
    return units


def _declarations(text: str) -> dict[str, str]:
    declarations = {}
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped.startswith(
            ('"ac.type_alias"', "ac.type_alias ", '"ac.constant"')
        ):
            continue
        symbol = re.search(r'\bsym_name = "([^"]+)"', line)
        if symbol:
            name = symbol.group(1)
        else:
            custom = re.match(
                r'ac\.type_alias\s+(?:"([^"]+)"|@"([^"]+)"|@([^\s]+))',
                stripped,
            )
            assert custom, line
            name = next(value for value in custom.groups() if value is not None)
        assert name not in declarations, f"duplicate declaration: {name}"
        declarations[name] = line
    return declarations


def _build(
    directory: Path,
    *,
    provider: str = PROVIDER,
    extra: dict[str, str] | None = None,
    reverse: bool = False,
) -> tuple[Path, dict[str, cpp.SourceUnit]]:
    source_root = directory / "source"
    source_root.mkdir(parents=True)
    sources = {
        "provider.py": provider,
        "facade.py": "from .provider import Word as PublicWord\n",
        "empty.py": '"""Empty declaration source."""\n',
        **(extra or {}),
        "root.py": ROOT_SOURCE,
    }
    units = {}
    for index, (relative, contents) in enumerate(sources.items()):
        source = source_root / relative
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_text(contents, encoding="utf-8")
        headers = tuple(unit.header for unit in units.values())
        units[relative] = cpp._compile_source(
            source,
            source_root=source_root,
            output_dir=directory / "units" / str(index),
            headers=headers,
        )
    final = directory / "root.final.ac"
    ordered = list(units.values())
    linked = cpp._link(
        list(reversed(ordered)) if reverse else ordered,
        final,
        top="demo.root.Root",
        role="design",
    )
    assert linked.returncode == 0, linked.stderr
    assert final.is_file()
    return final, units


def _verify(final: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [cpp._design_harness(), "--design", str(final), "--verify-only"],
        text=True,
        capture_output=True,
        check=False,
    )


def _emit(final: Path, target: str, output: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            cpp._design_harness(),
            "--design",
            str(final),
            "--target",
            target,
            "--role",
            "design",
            "--output",
            str(output),
        ],
        text=True,
        capture_output=True,
        check=False,
    )


def _parts(final: Path) -> dict:
    emitted = cpp._emit_parts(final)
    assert emitted.returncode == 0, emitted.stderr
    return json.loads(emitted.stdout)


def test_complete_owning_inventory_survives_final_and_unit_reordering(tmp_path: Path):
    extra = {
        "std.py": "from typing import Annotated\nValue = Annotated[int, range(4)]\n",
        "pkg/__init__.py": "from typing import Annotated\nTag = Annotated[int, range(8)]\n",
    }
    final, sources = _build(tmp_path, extra=extra)
    text = final.read_text(encoding="utf-8")
    units = _units(text)
    assert set(units) == {("demo", path) for path in sources}
    expected = {
        "provider.py": {f"demo.provider.{name}" for name in PROVIDER_NAMES},
        "root.py": {f"demo.root.{name}" for name in ROOT_NAMES},
        "facade.py": set(),
        "empty.py": set(),
        "std.py": {"demo.std.Value"},
        "pkg/__init__.py": {"demo.pkg.Tag"},
    }
    for path, names in expected.items():
        actual = _declarations(units["demo", path])
        assert set(actual) == names, path
        assert list(actual) == sorted(actual), f"{path}: canonical declaration order"
        owning = {
            name
            for name, line in _declarations(sources[path].header.read_text()).items()
            if 'ac.declaration_role = "definition"' in line
        }
        assert owning == names, f"{path}: independent source inventory"
        kind = "implementation" if path == "root.py" else "declarations"
        assert f'ac.unit_kind = "{kind}"' in units["demo", path]
        assert units["demo", path].count('"ac.module"') == (path == "root.py")
    for residue in (
        "ac.interfaces",
        "ac.exports",
        "ac.import_bindings",
        "ac.module.import",
        "import_snapshot",
    ):
        assert residue not in text
    assert text.count('"ac.reg"') == 1
    reversed_final, _ = _build(tmp_path / "reversed", extra=extra, reverse=True)
    assert _parts(final) == _parts(reversed_final)


def test_final_only_headers_are_standalone_and_constants_have_one_odr_address(
    tmp_path: Path,
):
    final, _ = _build(
        tmp_path,
        extra={
            "std.py": "from typing import Annotated\nValue = Annotated[int, range(4)]\n",
            "pkg/__init__.py": "from typing import Annotated\nTag = Annotated[int, range(8)]\n",
        },
    )
    shutil.rmtree(tmp_path / "source")
    shutil.rmtree(tmp_path / "units")
    checked = _verify(final)
    assert checked.returncode == 0, checked.stderr
    for target in ("cpp", "verilog"):
        emitted = _emit(final, target, tmp_path / f"standalone.{target}")
        assert emitted.returncode == 0, emitted.stderr
    payload = _parts(final)
    output, groups = cpp._write_parts(tmp_path, payload)
    by_path = {group["source"]["path"]: group for group in groups}
    assert set(by_path) == {
        "provider.py",
        "facade.py",
        "empty.py",
        "std.py",
        "pkg/__init__.py",
        "root.py",
    }
    for path, group in by_path.items():
        if path == "root.py":
            assert group["source_path"].endswith(".cpp") and group["implementation"]
        else:
            assert group["source_path"] is None and group["implementation"] is None
    assert by_path["pkg/__init__.py"]["header_path"] == "sources/demo/pkg/init.hpp"
    for path in ("facade.py", "empty.py"):
        assert not re.search(
            r"\b(using|class|struct|constexpr)\b", by_path[path]["header"]
        )
    for index, group in enumerate(groups):
        if group["source_path"] is not None:
            continue
        source = tmp_path / f"declaration-only-{index}.cpp"
        source.write_text(f'#include "{group["header_path"]}"\n')
        result = subprocess.run(
            [cpp._cxx(), "-std=c++20", "-I", str(output), "-fsyntax-only", str(source)],
            text=True,
            capture_output=True,
            check=False,
        )
        assert result.returncode == 0, result.stderr
    objects = cpp._compile_groups(output, groups, tmp_path)
    assert len(objects) == 1, "declaration sources must not create object files"
    includes = "".join(f'#include "{group["header_path"]}"\n' for group in groups)
    checks = r"""
#include <cstdint>
#include <type_traits>
namespace p = demo::provider;
static_assert(::std::is_same_v<p::flag, bool>);
static_assert(::std::is_same_v<p::unsigned_one, ::std::uint64_t>);
static_assert(!::std::is_same_v<p::unsigned_one, bool>);
static_assert(::std::is_same_v<p::word, ::std::uint64_t>);
static_assert(::std::is_same_v<p::hidden, ::std::uint64_t>);
static_assert(::std::is_same_v<demo::root::own, ::std::uint64_t>);
static_assert(::std::is_same_v<demo::std::value, ::std::uint64_t>);
static_assert(::std::is_same_v<demo::pkg::tag, ::std::uint64_t>);
static_assert(!p::false_value && p::true_value);
static_assert(p::zero == 0 && p::one == 1);
static_assert(p::maximum == UINT64_C(18446744073709551615));
static_assert(::std::is_same_v<::std::remove_cv_t<decltype(p::true_value)>, bool>);
static_assert(::std::is_same_v<::std::remove_cv_t<decltype(p::maximum)>, ::std::uint64_t>);
"""
    first = tmp_path / "first.cpp"
    second = tmp_path / "second.cpp"
    first.write_text(
        includes
        + checks
        + """
const ::std::uint64_t *constant_address();
int main() { return constant_address() == &p::maximum ? 0 : 1; }
""",
        encoding="utf-8",
    )
    second.write_text(
        includes
        + checks
        + """
const ::std::uint64_t *constant_address() { return &p::maximum; }
""",
        encoding="utf-8",
    )
    for index, source in enumerate((first, second)):
        obj = tmp_path / f"consumer-{index}.o"
        result = subprocess.run(
            [
                cpp._cxx(),
                "-std=c++20",
                *cpp._include_args(output),
                "-c",
                str(source),
                "-o",
                str(obj),
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        assert result.returncode == 0, result.stderr
        objects.append(obj)
    executable = tmp_path / "headers-odr"
    linked = subprocess.run(
        [cpp._cxx(), *(str(obj) for obj in objects), "-o", str(executable)],
        text=True,
        capture_output=True,
        check=False,
    )
    assert linked.returncode == 0, linked.stderr
    ran = subprocess.run([str(executable)], text=True, capture_output=True, check=False)
    assert ran.returncode == 0, ran.stderr


def _edit_declaration(text: str, symbol: str, change) -> str:
    original = _declarations(text)[symbol]
    replacement = change(original)
    assert replacement != original, f"mutation did not apply: {symbol}"
    return text.replace(original, replacement, 1)


def _set_dictionary(line: str, field: str, value: str) -> str:
    match = re.search(rf"\b{re.escape(field)}(?:\s*=\s*|\s+)\{{", line)
    assert match, (field, line)
    start = line.index("{", match.start())
    return line[:start] + value + line[_balanced(line, start) + 1 :]


def _edit_unit(text: str, path: str, change) -> str:
    original = _units(text)["demo", path]
    replacement = change(original)
    assert replacement != original, path
    return text.replace(original, replacement, 1)


def _insert_unit(text: str, path: str, body: str = "", package: str = "demo") -> str:
    key = package, path
    at = next(
        (text.index(block) for owner, block in _units(text).items() if owner > key),
        re.search(r"(?m)^}", text).start(),
    )
    block = (
        '  module attributes {ac.source_owner = {package = "'
        + package
        + '", path = "'
        + path
        + '"}, ac.stage = "final", '
        'ac.unit_kind = "declarations"} {\n' + body + "  }\n"
    )
    return text[:at] + block + text[at:]


def _error_text(result: subprocess.CompletedProcess[str]) -> str:
    # Do not let a pytest temp directory's name satisfy diagnostic assertions.
    lines = [
        line.split("error:", 1)[1]
        for line in result.stderr.splitlines()
        if "error:" in line
    ]
    return "\n".join(lines)


def _reject_final(path: Path, pattern: str) -> None:
    checked = _verify(path)
    assert checked.returncode == 1, checked.stderr
    assert re.search(pattern, _error_text(checked), re.IGNORECASE), checked.stderr
    for target in ("cpp", "verilog"):
        output = path.parent / f"rejected.{target}"
        assert not output.exists()
        emitted = _emit(path, target, output)
        assert emitted.returncode == 1, emitted.stderr
        assert re.search(pattern, _error_text(emitted), re.IGNORECASE), emitted.stderr
        assert not output.exists()
    parts = cpp._emit_parts(path)
    assert parts.returncode == 1 and parts.stdout == "", parts.stderr


def _reject_cpp_only(final: Path, pattern: str) -> None:
    checked = _verify(final)
    assert checked.returncode == 0, checked.stderr
    rtl = _emit(final, "verilog", final.parent / "accepted.sv")
    assert rtl.returncode == 0, rtl.stderr
    sentinel = final.parent / "existing.cpp"
    sentinel.write_bytes(b"previous user artifact\n")
    cpp_result = _emit(final, "cpp", sentinel)
    assert cpp_result.returncode == 1, cpp_result.stderr
    assert re.search(pattern, _error_text(cpp_result), re.IGNORECASE), cpp_result.stderr
    assert sentinel.read_bytes() == b"previous user artifact\n"
    parts = cpp._emit_parts(final)
    assert parts.returncode == 1 and parts.stdout == "", parts.stderr
    assert re.search(pattern, _error_text(parts), re.IGNORECASE), parts.stderr


def test_signed_and_integer_one_bit_final_declarations_have_exact_native_types(
    tmp_path: Path,
):
    # Focused compiler-input transformations: this test does not claim new
    # signed-range or negative-literal Python frontend support.
    final, _ = _build(tmp_path)
    text = final.read_text()
    for symbol, target in (
        (
            "SignedOne",
            '{kind = "integer", storage = i1, lower = #ac.math_int<-1>, '
            'upper = #ac.math_int<1>, interpretation = "signed"}',
        ),
        (
            "NarrowSigned",
            '{kind = "integer", storage = i4, lower = #ac.math_int<-8>, '
            'upper = #ac.math_int<8>, interpretation = "signed"}',
        ),
    ):
        text = _edit_declaration(
            text,
            "demo.provider." + symbol,
            lambda line, target=target: _set_dictionary(line, "target", target),
        )
    for symbol, value in (("MinusOne", -1), ("Minimum", -(1 << 63))):
        text = _edit_declaration(
            text,
            "demo.provider." + symbol,
            lambda line, value=value: _set_dictionary(
                line, "value", f'{{kind = "integer", value = #ac.math_int<{value}>}}'
            ),
        )
    variant = tmp_path / "signed.ac"
    variant.write_text(text)
    verified = _verify(variant)
    assert verified.returncode == 0, verified.stderr
    output, groups = cpp._write_parts(tmp_path, _parts(variant))
    cpp._compile_groups(output, groups, tmp_path)
    source = tmp_path / "signed.cpp"
    source.write_text(
        r"""
#include "sources/demo/provider.hpp"
#include <cstdint>
#include <type_traits>
namespace p = demo::provider;
static_assert(::std::is_same_v<p::flag, bool>);
static_assert(::std::is_same_v<p::unsigned_one, ::std::uint64_t>);
static_assert(::std::is_same_v<p::signed_one, ::std::int64_t>);
static_assert(::std::is_same_v<p::narrow_signed, ::std::int64_t>);
static_assert(!::std::is_same_v<p::signed_one, bool>);
static_assert(p::minus_one == -INT64_C(1));
static_assert(p::minimum == (-INT64_C(9223372036854775807) - INT64_C(1)));
static_assert(::std::is_same_v<::std::remove_cv_t<decltype(p::minimum)>, ::std::int64_t>);
"""
    )
    result = subprocess.run(
        [
            cpp._cxx(),
            "-std=c++20",
            *cpp._include_args(output),
            "-fsyntax-only",
            str(source),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("value", [-(1 << 63) - 1, 1 << 64, 1 << 180])
def test_wide_constants_link_and_pass_rtl_but_both_cpp_renderers_reject(
    tmp_path: Path, value: int
):
    final, units = _build(
        tmp_path, provider=PROVIDER + f"\nOversized = {max(0, value)}\n"
    )
    if value < 0:
        # Feed a legal negative MathInt source declaration to the native linker.
        # The unsigned cases above originate directly in real Python literals.
        header = units["provider.py"].header
        header.write_text(
            _edit_declaration(
                header.read_text(),
                "demo.provider.Oversized",
                lambda line: _set_dictionary(
                    line,
                    "value",
                    f'{{kind = "integer", value = #ac.math_int<{value}>}}',
                ),
            )
        )
        final = tmp_path / "negative.final.ac"
        linked = cpp._link(
            list(units.values()), final, top="demo.root.Root", role="design"
        )
        assert linked.returncode == 0, linked.stderr
    assert f"#ac.math_int<{value}>" in final.read_text()
    _reject_cpp_only(final, r"demo\.provider\.Oversized")
    for result in (
        cpp._emit_parts(final),
        _emit(final, "cpp", tmp_path / "must-not-exist.cpp"),
    ):
        assert result.returncode == 1
        assert str(value) in _error_text(result), result.stderr
    assert not (tmp_path / "must-not-exist.cpp").exists()


def _malformed(text: str, case: str) -> tuple[str, str]:
    symbol = "demo.provider.Word"
    if case == "symbol":
        return (
            _edit_declaration(
                text,
                symbol,
                lambda line: line.replace("demo.provider.Word", "demo.foreign.Word", 1),
            ),
            r"owner|symbol|origin|definition",
        )
    if case == "owner":
        return (
            _edit_declaration(
                text,
                symbol,
                lambda line: line.replace(
                    'path = "provider.py"', 'path = "foreign.py"', 1
                ),
            ),
            r"owner|source",
        )
    if case == "origin":
        return (
            _edit_declaration(
                text,
                symbol,
                lambda line: re.sub(
                    r'definition = @(?:"demo\.provider\.Word"|demo\.provider\.Word)',
                    'definition = @"demo.provider.Other"',
                    line,
                    count=1,
                ),
            ),
            r"origin|definition",
        )
    if case in {"snapshot", "unknown-role", "unknown-field"}:
        replacement = {
            "snapshot": 'ac.declaration_role = "import_snapshot"',
            "unknown-role": 'ac.declaration_role = "bogus"',
            "unknown-field": 'ac.declaration_role = "definition", ac.unapproved = true',
        }[case]
        return (
            _edit_declaration(
                text,
                symbol,
                lambda line: line.replace(
                    'ac.declaration_role = "definition"', replacement, 1
                ),
            ),
            r"role|snapshot|field|attribute|declaration",
        )
    if case == "width":
        return (
            _edit_declaration(
                text,
                symbol,
                lambda line: line.replace("storage = i8", "storage = i7", 1),
            ),
            r"width|storage|range",
        )
    if case == "bool-value":
        return (
            _edit_declaration(
                text,
                "demo.provider.TrueValue",
                lambda line: line.replace("value = true", "value = #ac.math_int<1>", 1),
            ),
            r"value|bool|type",
        )
    if case in {"exports", "imports", "interfaces"}:
        attr = {
            "exports": "ac.exports",
            "imports": "ac.import_bindings",
            "interfaces": "ac.interfaces",
        }[case]
        return (
            _edit_unit(
                text,
                "provider.py",
                lambda block: block.replace(
                    'ac.stage = "final"', f'{attr} = [], ac.stage = "final"', 1
                ),
            ),
            r"attribute|envelope|metadata|unit",
        )
    if case == "hardware":
        return (
            _edit_unit(
                text,
                "empty.py",
                lambda block: block[:-1] + "  %illegal = arith.constant false\n  }",
            ),
            r"declaration|operation|hardware|unit",
        )
    if case == "helper":
        return (
            _edit_unit(
                text,
                "empty.py",
                lambda block: block[:-1] + "  func.func @forbidden() { return }\n  }",
            ),
            r"declaration|helper|operation|unit",
        )
    if case == "nested":
        line = _declarations(text)["demo.root.Own"]
        without = text.replace(line + "\n", "", 1)
        return (
            _edit_unit(
                without,
                "root.py",
                lambda block: re.sub(
                    r"(?m)^([ \t]*\^bb0[^\n]*\n)",
                    lambda match: match.group(1) + "      " + line.strip() + "\n",
                    block,
                    count=1,
                ),
            ),
            r"parent|placement|declaration|operation",
        )
    if case == "duplicate":
        line = _declarations(text)[symbol]
        return (
            text.replace(line, line + "\n" + line, 1),
            r"duplicate|redefin|symbol|order",
        )
    if case == "order":
        lines = list(_declarations(_units(text)["demo", "provider.py"]).values())
        first, second = lines[:2]
        return (
            text.replace(first, "__SWAP__", 1)
            .replace(second, first, 1)
            .replace("__SWAP__", second, 1),
            r"order|sort",
        )
    raise AssertionError(case)


@pytest.mark.parametrize(
    "case",
    [
        "owner",
        "symbol",
        "origin",
        "snapshot",
        "unknown-role",
        "unknown-field",
        "width",
        "bool-value",
        "exports",
        "imports",
        "interfaces",
        "hardware",
        "helper",
        "nested",
        "duplicate",
        "order",
    ],
)
def test_common_declaration_rejections_cover_verify_and_both_backends(
    tmp_path: Path, case: str
):
    final, _ = _build(tmp_path)
    original = final.read_text()
    text, pattern = _malformed(original, case)
    assert text != original, case
    invalid = tmp_path / "invalid.ac"
    invalid.write_text(text)
    _reject_final(invalid, pattern)


@pytest.mark.parametrize(
    "paths",
    [
        ("foo.py", "foo/__init__.py"),
        ("case.py", "CASE.py"),
        ("σ.py", "Σ.py"),
        ("ß.py", "ss.py"),
        ("foo/__init__.py", "FOO/__INIT__.py"),
    ],
)
def test_empty_unit_owners_cannot_alias_import_module_identity(tmp_path: Path, paths):
    final, _ = _build(tmp_path)
    text = final.read_text()
    for path in paths:
        text = _insert_unit(text, path)
    invalid = tmp_path / "invalid.ac"
    invalid.write_text(text)
    _reject_final(invalid, r"owner|module|case|ambig|collision")


@pytest.mark.parametrize("kind", ["list-alias", "list-constant"])
def test_unused_nonscalar_definitions_are_not_silently_erased(
    tmp_path: Path, kind: str
):
    _, units = _build(tmp_path)
    header = units["provider.py"].header
    if kind == "list-alias":

        def transform(line):
            return _set_dictionary(
                line,
                "target",
                '{kind = "list", element = {kind = "bool", storage = i1}, length = 2 : i64}',
            )

        symbol = "demo.provider.Unused"
    else:

        def transform(line):
            line = _set_dictionary(
                line,
                "type",
                '{kind = "list", element = {kind = "integer"}, length = 2 : i64}',
            )
            return _set_dictionary(
                line,
                "value",
                '{kind = "list", values = [{kind = "integer", value = #ac.math_int<0>}, '
                '{kind = "integer", value = #ac.math_int<1>}]}',
            )

        symbol = "demo.provider.One"
    header.write_text(_edit_declaration(header.read_text(), symbol, transform))
    output = tmp_path / "unsupported.ac"
    result = cpp._link(
        list(units.values()), output, top="demo.root.Root", role="design"
    )
    assert result.returncode == 1, result.stderr
    assert re.search(
        r"scalar|unsupported|declaration|projection", _error_text(result), re.I
    ), result.stderr
    assert not output.exists()


@pytest.mark.parametrize(
    "source",
    [
        "\nunused = 3\n",
        "\né = Annotated[int, range(4)]\nue9 = 3\n",
    ],
)
def test_declaration_cpp_name_collisions_are_target_rejections(
    tmp_path: Path, source: str
):
    final, _ = _build(tmp_path, provider=PROVIDER + source)
    _reject_cpp_only(final, r"collision|name|scope")


@pytest.mark.parametrize("name", ["Std", "Gfsim"])
def test_global_runtime_names_are_rejected_without_a_source_name_ban(
    tmp_path: Path, name: str
):
    final, _ = _build(tmp_path)
    body = (
        f'    "ac.type_alias"() <{{sym_name = "{name}", '
        'target = {kind = "bool", storage = i1}}> {'
        'ac.source_owner = {package = "", path = "__init__.py"}, '
        f'ac.origin = {{site = {{definition = @"{name}", ast_path = []}}, expansion = []}}, '
        'ac.declaration_role = "definition"} : () -> () loc("__init__.py":1:1)\n'
    )
    variant = tmp_path / "global-name.ac"
    variant.write_text(_insert_unit(final.read_text(), "__init__.py", body, package=""))
    _reject_cpp_only(variant, r"collision|reserved|name|scope")


@pytest.mark.parametrize("case", ["module", "namespace", "path"])
def test_all_declaration_group_names_share_the_cpp_collision_domain(
    tmp_path: Path, case: str
):
    extra = {
        "namespace": {"provider/unused.py": "Value = 7\n"},
        "path": {"fooBar.py": "Value = 7\n", "foo_bar.py": "Value = 8\n"},
    }.get(case)
    final, _ = _build(tmp_path, extra=extra)
    if case == "module":
        # ROOT and Root are distinct canonical source symbols but both legalize
        # to the C++ family name root. ROOT keeps the existing declaration order.
        text = _edit_declaration(
            final.read_text(),
            "demo.root.Own",
            lambda line: line.replace("demo.root.Own", "demo.root.ROOT"),
        )
        final = tmp_path / "same-cpp-name.ac"
        final.write_text(text)
    _reject_cpp_only(final, r"collision|name|scope|namespace")


@pytest.mark.parametrize("name", ["Std", "Gfsim"])
def test_global_module_names_reject_both_cpp_renderers_from_real_source(
    tmp_path: Path,
    name: str,
):
    source_root = tmp_path / "source"
    source_root.mkdir()
    source = source_root / "__init__.py"
    source.write_text(
        f"from pycircuit import module\n\n@module\ndef {name}():\n    state: bool = False\n",
        encoding="utf-8",
    )
    unit = cpp._compile_source(
        source,
        source_root=source_root,
        output_dir=tmp_path / "unit",
        package="",
    )
    final = tmp_path / "module.ac"
    linked = cpp._link([unit], final, top=name, role="design")
    assert linked.returncode == 0, linked.stderr

    # Common verification and RTL must succeed. Both C++ renderers must reject;
    # the shared oracle checks empty JSON stdout and preservation of old bytes.
    pattern = r"collision|reserved|global|scope|name"
    _reject_cpp_only(final, pattern)

    # Also prove the monolithic renderer creates no new output on rejection.
    absent = tmp_path / "must-not-exist.cpp"
    rejected = _emit(final, "cpp", absent)
    assert rejected.returncode == 1, rejected.stderr
    assert re.search(pattern, _error_text(rejected), re.IGNORECASE), rejected.stderr
    assert not absent.exists()


def test_projection_rejects_casefolded_raw_init_owners(tmp_path: Path):
    source_root = tmp_path / "source"
    units = []
    for index, (path, contents) in enumerate(
        (
            ("foo/__init__.py", '"""lower package"""\n'),
            ("FOO/__INIT__.py", '"""upper package"""\n'),
            (
                "root.py",
                "from pycircuit import module\n@module\ndef Root():\n    state: bool = False\n",
            ),
        )
    ):
        source = source_root / path
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_text(contents)
        units.append(
            cpp._compile_source(
                source,
                source_root=source_root,
                output_dir=tmp_path / "units" / str(index),
                package="demo",
            )
        )
    output = tmp_path / "rejected.ac"
    result = cpp._link(units, output, top="demo.root.Root", role="design")
    assert result.returncode == 1, result.stderr
    assert re.search(r"owner|module|case|ambig|collision", _error_text(result), re.I)
    assert not output.exists()
