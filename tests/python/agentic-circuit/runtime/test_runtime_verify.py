from __future__ import annotations

import json
from pathlib import Path

from acir_runtime import main
from acir_runtime_verify import verify_catalog


def test_packaged_runtime_metadata_is_uniform() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    accepted = [entry for entry in catalog["entries"] if entry.get("status") == "accepted"]
    # The catalog grows with each promoted release batch; all current entries
    # must still satisfy the same uniform metadata contract.
    assert len(accepted) >= 83
    for entry in accepted:
        interface = entry["interface"]
        assert interface["wrapper_module"]
        assert interface["parameters"] is not None
        assert interface["ports"]
        assert entry["wrapper"] in entry["files"]
        assert entry["oracle"]["id"]
        assert entry["dependency_closure"]["status"] == "complete"
        assert entry["validation"]["semantic_status"] == "functional_oracle_v1"


def test_verify_runtime_can_check_packaging_without_tools(tmp_path: Path) -> None:
    root = Path(__file__).resolve().parents[4]
    report = verify_catalog(
        (root / "library/verilog/catalog.json").resolve(),
        verilator=None,
        yosys=None,
        timeout=1,
        no_tools=True,
    )
    count = len(json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))["entries"])
    assert report["summary"] == {"entries": count, "status": "skipped", "counts": {"skipped": count}}
    assert main(["verify-runtime", "--no-tools", "--catalog", str(root / "library/verilog/catalog.json"), "--report", str(tmp_path / "report.json")]) == 0


def test_v08_vortex_entries_are_hashed_and_accepted() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    manifest = json.loads((root / "library/verilog/manifests/parameterized-components-v0.8.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    assert entries["vortex-popcount"]["validation"]["status"] == "passed"
    assert entries["vortex-rr-arbiter"]["validation"]["status"] == "passed"
    assert set(manifest["sha256"]) >= {
        "pyc_runtime_vortex_popcount.sv",
        "pyc_runtime_vortex_rr_arbiter.sv",
        "vendor-v0.4/vortex/hw/rtl/libs/VX_popcount.sv",
        "vendor-v0.4/vortex/hw/rtl/libs/VX_rr_arbiter.sv",
    }


def test_v08_pulp_and_basejump_round_robin_batch_is_accepted() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    manifest = json.loads((root / "library/verilog/manifests/parameterized-components-v0.8.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    names = ("pulp-stream-arbiter", "pulp-rr-arb-tree", "pulp-stream-xbar",
             "basejump-rr-1-to-n", "basejump-rr-n-to-1", "basejump-rr-2-to-2")
    for name in names:
        assert entries[name]["validation"]["status"] == "passed"
        assert entries[name]["validation"]["runtime_gate"] == "passed"
        assert entries[name]["dependency_closure"]["status"] == "complete"
    assert set(manifest["sha256"]) >= {
        "pyc_runtime_pulp_stream_arbiter.sv",
        "pyc_runtime_pulp_rr_arb_tree.sv",
        "pyc_runtime_pulp_stream_xbar.sv",
        "pyc_runtime_basejump_rr_1_to_n.sv",
        "pyc_runtime_basejump_rr_n_to_1.sv",
        "pyc_runtime_basejump_rr_2_to_2.sv",
    }


def test_v09_basejump_one_hot_batch_is_accepted_and_hashed() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    manifest = json.loads((root / "library/verilog/manifests/parameterized-components-v0.9.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    names = ("basejump-adder-one-hot", "basejump-mux-one-hot", "basejump-mux-butterfly")
    for name in names:
        assert entries[name]["validation"]["status"] == "passed"
        assert entries[name]["validation"]["runtime_gate"] == "passed"
        assert entries[name]["dependency_closure"]["status"] == "complete"
    assert set(manifest["sha256"]) >= {
        "pyc_runtime_basejump_adder_one_hot.sv",
        "pyc_runtime_basejump_mux_one_hot.sv",
        "pyc_runtime_basejump_mux_butterfly.sv",
        "vendor-v0.5/basejump/bsg_misc/bsg_adder_one_hot.sv",
        "vendor-v0.5/basejump/bsg_misc/bsg_swap.sv",
    }


def test_v10_array_concentrate_and_credit_counter_are_accepted_and_hashed() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    manifest = json.loads((root / "library/verilog/manifests/parameterized-components-v0.10.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    for name in ("basejump-array-concentrate-static", "pulp-credit-counter"):
        assert entries[name]["validation"]["status"] == "passed"
        assert entries[name]["validation"]["runtime_gate"] == "passed"
        assert entries[name]["dependency_closure"]["status"] == "complete"
    assert set(manifest["sha256"]) >= {
        "pyc_runtime_basejump_array_concentrate_static.sv",
        "pyc_runtime_pulp_credit_counter.sv",
        "vendor-v0.5/basejump/bsg_misc/bsg_array_concentrate_static.sv",
        "vendor-v0.10/pulp_common_cells/src/cc_credit_counter.sv",
        "vendor-v0.10/pulp_common_cells/include/common_cells/registers.svh",
        "licenses/pulp-common-cells-v0.10-LICENSE",
    }


def test_v11_ripple_concentrate_and_plru_batch_is_accepted_and_hashed() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    manifest = json.loads((root / "library/verilog/manifests/parameterized-components-v0.11.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    names = ("basejump-adder-ripple-carry", "basejump-concentrate-static", "pulp-plru-tree")
    for name in names:
        assert entries[name]["validation"]["status"] == "passed"
        assert entries[name]["validation"]["runtime_gate"] == "passed"
        assert entries[name]["dependency_closure"]["status"] == "complete"
    assert set(manifest["sha256"]) >= {
        "pyc_runtime_basejump_adder_ripple_carry.sv",
        "pyc_runtime_basejump_concentrate_static.sv",
        "pyc_runtime_pulp_plru_tree.sv",
        "vendor-v0.5/basejump/bsg_misc/bsg_adder_ripple_carry.sv",
        "vendor-v0.5/basejump/bsg_misc/bsg_concentrate_static.sv",
        "vendor-v0.10/pulp_common_cells/src/cc_plru_tree.sv",
        "licenses/basejump-stl-v0.5/LICENSE",
        "licenses/pulp-common-cells-v0.10-LICENSE",
    }


def test_v12_mux_unconcentrate_and_counter_batch_is_accepted_and_hashed() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    manifest = json.loads((root / "library/verilog/manifests/parameterized-components-v0.12.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    names = ("basejump-mux", "basejump-unconcentrate-static", "basejump-counter-clear-up-saturating")
    for name in names:
        assert entries[name]["validation"]["status"] == "passed"
        assert entries[name]["validation"]["runtime_gate"] == "passed"
        assert entries[name]["dependency_closure"]["status"] == "complete"
    assert set(manifest["sha256"]) >= {
        "pyc_runtime_basejump_mux.sv",
        "pyc_runtime_basejump_unconcentrate_static.sv",
        "pyc_runtime_basejump_counter_clear_up_saturating.sv",
        "vendor-v0.11/basejump/bsg_misc/bsg_mux.sv",
        "vendor-v0.11/basejump/bsg_misc/bsg_unconcentrate_static.sv",
        "vendor-v0.11/basejump/bsg_misc/bsg_counter_clear_up_saturating.sv",
        "vendor-v0.5/basejump/bsg_misc/bsg_defines.sv",
        "licenses/basejump-stl-v0.5/LICENSE",
    }
