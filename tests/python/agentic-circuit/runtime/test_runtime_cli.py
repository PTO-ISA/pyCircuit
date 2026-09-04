from __future__ import annotations

import json
from pathlib import Path

from acir_runtime import main


def test_checked_in_catalog_verifies_and_exposes_external_runtime_set() -> None:
    root = Path(__file__).resolve().parents[4]
    assert main(["verify-catalog", "--catalog", str(root / "library/verilog/catalog.json")]) == 0
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    # The runtime catalog contains the prior releases plus v0.6 CDC FIFO,
    # v0.7 larger SECDED primitives, the v0.8 arbitration/dataflow batch, and
    # the v0.9 BaseJump one-hot/butterfly batch, the v0.10 compaction/
    # credit-counter batch, the v0.11 ripple/concentrate/PLRU batch, and the
    # v0.12 mux/unconcentrate/counter batch, v0.13 channel/priority batch,
    # v0.14 inverse SECDED, v0.15 Hamming(76,68), and v0.16 Vortex/PULP
    # selection and CDC batches are present in the append-only catalog.
    assert len(catalog["entries"]) >= 103
    external = [entry for entry in catalog["entries"] if entry["provider"] == "github"]
    assert len(external) >= 99
    assert all(entry["provenance"]["commit"] for entry in external)
    assert all(entry["validation"]["status"] == "passed" for entry in external)


def test_catalog_list_can_filter_by_family(capsys) -> None:
    root = Path(__file__).resolve().parents[4]
    assert main(["list", "--catalog", str(root / "library/verilog/catalog.json"), "--family", "encoding"]) == 0
    output = capsys.readouterr().out
    assert "pulp-binary-to-gray" in output
    assert "fifo" not in output
