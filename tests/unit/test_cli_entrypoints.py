from __future__ import annotations

import json

import pytest
from pycircuit.cli import (
    _base_name_of,
    _collect_jit_params,
    _is_timed_domain_build,
    _merge_verilog_primitive_bundles,
)

pytestmark = pytest.mark.unit


def timed_build(m, domain, *, width: int = 8, signed: bool = False) -> None:
    _ = (m, domain, width, signed)


timed_build.__pycircuit_name__ = "timed_smoke"


def structural_build(m, *, depth: int = 4) -> None:
    _ = (m, depth)


def test_collect_jit_params_skips_cycle_aware_domain_argument() -> None:
    assert _is_timed_domain_build(timed_build) is True
    assert _collect_jit_params(timed_build, overrides=[]) == {
        "signed": False,
        "width": 8,
    }


def test_collect_jit_params_keeps_structural_defaults() -> None:
    assert _is_timed_domain_build(structural_build) is False
    assert _collect_jit_params(structural_build, overrides=[]) == {"depth": 4}


def test_base_name_prefers_public_cycle_aware_symbol_override() -> None:
    assert _base_name_of(timed_build) == "timed_smoke"
    assert _base_name_of(structural_build) == "structural_build"


def test_verilog_primitive_merge_keeps_later_module_closure(tmp_path) -> None:
    base = "// base\nmodule fixed; endmodule\n"
    lint_on = "/* verilator lint_on DECLFILENAME */\n"
    plain = tmp_path / "a" / "pyc_primitives.v"
    selected = tmp_path / "b" / "pyc_primitives.v"
    selected_wide = tmp_path / "c" / "pyc_primitives.v"
    plain.parent.mkdir()
    selected.parent.mkdir()
    selected_wide.parent.mkdir()
    plain.write_text(base + lint_on, encoding="utf-8")
    selected.write_text(
        base
        + "// --- selected RTL: selected.v (BSD-3-Clause)\n"
        + "module selected; endmodule\n"
        + lint_on,
        encoding="utf-8",
    )
    selected_wide.write_text(selected.read_text(encoding="utf-8"), encoding="utf-8")
    (plain.parent / "manifest.json").write_text(
        '{"rtl_implementations": []}', encoding="utf-8"
    )
    implementation = {
        "implementation_id": "pyc.selected.v1",
        "semantic_id": "pyc.selected.v1",
        "module": "selected",
        "sources": [
            {
                "path": "selected.v",
                "sha256": "sha256:" + "1" * 64,
                "license": "BSD-3-Clause",
            }
        ],
    }
    (selected.parent / "manifest.json").write_text(
        json.dumps(
            {
                "rtl_implementations": [implementation],
                "rtl_bindings": [
                    {
                        "implementation_id": "pyc.selected.v1",
                        "semantic_id": "pyc.selected.v1",
                        "parameters": {"WIDTH": 4},
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    (selected_wide.parent / "manifest.json").write_text(
        json.dumps(
            {
                "rtl_implementations": [implementation],
                "rtl_bindings": [
                    {
                        "implementation_id": "pyc.selected.v1",
                        "semantic_id": "pyc.selected.v1",
                        "parameters": {"WIDTH": 13},
                    }
                ],
            }
        ),
        encoding="utf-8",
    )

    merged, implementations, bindings = _merge_verilog_primitive_bundles(
        [plain, selected, selected_wide], tmp_path / "pyc_primitives.v"
    )

    text = merged.read_text(encoding="utf-8")
    assert text.count("module fixed") == 1
    assert text.count("module selected") == 1
    assert implementations == [implementation]
    assert {binding["parameters"]["WIDTH"] for binding in bindings} == {
        4,
        13,
    }
