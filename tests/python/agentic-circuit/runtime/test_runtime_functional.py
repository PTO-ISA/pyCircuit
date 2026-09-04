from __future__ import annotations

import json
from pathlib import Path

from acir_runtime_functional import functional_catalog, generate_tb


def test_reduction_oracles_cover_empty_sparse_and_tie_cases() -> None:
    sum_tb = generate_tb("sum", num_src=8, width=8, saturate=True)
    max_tb = generate_tb("max", num_src=8, width=8)
    assert "PYC_RUNTIME_FUNCTIONAL_PASS" in sum_tb
    assert "PYC_RUNTIME_FUNCTIONAL_PASS" in max_tb
    assert "valid = '0" in sum_tb
    assert "left-most winner" in max_tb
    assert "OUT_WIDTH'(255)" in sum_tb


def test_functional_catalog_can_check_packaging_without_tools() -> None:
    root = Path(__file__).resolve().parents[4]
    report = functional_catalog(
        (root / "library/verilog/catalog.json").resolve(),
        verilator=None,
        yosys=None,
        timeout=1,
        no_tools=True,
    )
    count = len(json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))["entries"])
    assert report["summary"] == {"entries": count, "status": "skipped", "counts": {"skipped": count}}


def test_catalog_has_reduction_tree_functional_metadata() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    for name in ("opentitan-sum-tree", "opentitan-max-tree", "verilog-axi-priority-encoder"):
        assert entries[name]["validation"]["semantic_status"] == "functional_oracle_v1"
        assert entries[name]["validation"]["functional"]["status"] == "passed"


def test_parameterized_oracles_are_available_for_new_runtime_components() -> None:
    pop = generate_tb("popcount", width=13, dut_name="pyc_runtime_pulp_popcount")
    lzc = generate_tb("lzc", width=8, saturate=False, dut_name="pyc_runtime_pulp_lzc")
    arb = generate_tb("arbiter", num_src=4, dut_name="pyc_runtime_basejump_rr_arbiter")
    counter = generate_tb("counter", num_src=3, width=1, dut_name="pyc_runtime_basejump_counter")
    assert "POPCOUNT_MISMATCH" in pop and "WIDTH = 13" in pop
    assert "LZC_MISMATCH" in lzc and ".MODE(0)" in lzc
    assert "ARB_MISMATCH" in arb and "PYC_RUNTIME_FUNCTIONAL_PASS" in arb
    assert "COUNTER_MISMATCH" in counter and "MAX_VALUE = 3" in counter
    vortex_pop = generate_tb("vortex-popcount", width=13, saturate=False,
                             dut_name="pyc_runtime_vortex_popcount")
    vortex_rr = generate_tb("vortex-rr-arbiter", num_src=4,
                            dut_name="pyc_runtime_vortex_rr_arbiter")
    assert ".MODEL(2)" in vortex_pop and "POPCOUNT_MISMATCH" in vortex_pop
    assert "VORTEX_RR_MISMATCH" in vortex_rr and "grant_ready" in vortex_rr


def test_functional_oracles_cover_existing_arithmetic_and_encoding_entries() -> None:
    adder = generate_tb("adder", width=13, dut_name="pyc_runtime_basejump_adder")
    and_tb = generate_tb("bitwise-and", width=8, dut_name="pyc_runtime_basejump_and")
    xor_tb = generate_tb("bitwise-xor", width=8, dut_name="pyc_runtime_basejump_xor")
    to_gray = generate_tb("binary-to-gray", width=13, dut_name="pyc_runtime_pulp_binary_to_gray")
    from_gray = generate_tb("gray-to-binary", width=13, dut_name="pyc_runtime_pulp_gray_to_binary")
    assert "ADDER_MISMATCH" in adder and "WIDTH = 13" in adder
    assert "AND_MISMATCH" in and_tb and "PYC_RUNTIME_FUNCTIONAL_PASS" in and_tb
    assert "XOR_MISMATCH" in xor_tb and "PYC_RUNTIME_FUNCTIONAL_PASS" in xor_tb
    assert "BINARY_TO_GRAY_MISMATCH" in to_gray
    assert "GRAY_TO_BINARY_MISMATCH" in from_gray
    onehot = generate_tb("onehot", width=13, dut_name="pyc_runtime_opentitan_onehot_encode")
    assert "ONEHOT_MISMATCH" in onehot and "PYC_RUNTIME_FUNCTIONAL_PASS" in onehot


def test_catalog_new_components_have_closed_sources_and_oracles() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    for name in ("pulp-cc-popcount", "basejump-popcount", "basejump-rr-arbiter", "basejump-counter", "opentitan-onehot-encode", "basejump-encode-one-hot", "basejump-priority-onehot", "basejump-scan-or", "vortex-popcount", "vortex-rr-arbiter", "pulp-stream-arbiter", "pulp-rr-arb-tree", "pulp-stream-xbar", "basejump-rr-1-to-n", "basejump-rr-n-to-1", "basejump-rr-2-to-2"):
        entry = entries[name]
        assert entry["status"] == "accepted"
        assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
        assert entry["dependency_closure"]["status"] == "complete"
        runtime_root = root / "library/verilog"
        assert all((runtime_root / item).is_file() for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_basejump_scan_oracles_are_available_in_both_directions() -> None:
    encode = generate_tb("encode-onehot", width=13, saturate=False,
                         dut_name="pyc_runtime_basejump_encode_one_hot")
    priority = generate_tb("priority-onehot", width=8, saturate=True,
                           dut_name="pyc_runtime_basejump_priority_onehot")
    scan = generate_tb("scan-or", width=8, saturate=False,
                       dut_name="pyc_runtime_basejump_scan_or")
    assert "BASEJUMP_ENCODE_MISMATCH" in encode
    assert "BASEJUMP_PRIORITY_ONEHOT_MISMATCH" in priority
    assert "BASEJUMP_SCAN_OR_MISMATCH" in scan


def test_basejump_cam_fifo_and_mux_oracles_are_parameterized() -> None:
    cam = generate_tb("cam", num_src=4, width=8, dut_name="pyc_runtime_basejump_cam_1r1w_unmanaged")
    sync_cam = generate_tb("cam-sync", num_src=2, width=4,
                           dut_name="pyc_runtime_basejump_cam_1r1w_sync_unmanaged")
    tag_cam = generate_tb("cam-tag-array", num_src=4, width=8,
                          dut_name="pyc_runtime_basejump_cam_1r1w_tag_array")
    fifo = generate_tb("fifo-narrowed", num_src=2, width=8,
                       saturate=False, dut_name="pyc_runtime_basejump_fifo_narrowed")
    mux = generate_tb("bitwise-mux", width=13,
                      dut_name="pyc_runtime_basejump_mux_bitwise")
    muxi = generate_tb("muxi2", width=8,
                       dut_name="pyc_runtime_basejump_muxi2_gatestack")
    assert "CAM_READ_MISMATCH" in cam and "CAM_FALSE_HIT" in cam
    assert "CAM_READ_MISMATCH" in sync_cam and "@(posedge clk)" in sync_cam
    assert "CAM_TAG_MATCH_MISMATCH" in tag_cam
    assert "NARROW_FIFO_CHUNK_" in fifo and ".LSB_TO_MSB(0)" in fifo
    assert "BITWISE_MUX_MISMATCH" in mux and "MUXI2_MISMATCH" in muxi


def test_v06_cdc_and_v07_ecc_oracles_are_explicit() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    cdc = generate_tb("pulp-cdc-fifo-gray", num_src=2, width=3,
                      dut_name="pyc_runtime_pulp_cdc_fifo_gray")
    assert "src_clk" in cdc and "dst_clk" in cdc and "word did not cross" in cdc
    assert entries["pulp-cdc-fifo-gray"]["family"] == "cdc"
    assert entries["pulp-cdc-fifo-gray"]["validation"]["status"] == "passed"
    for name in ("opentitan-secded-64-57-enc", "opentitan-secded-64-57-dec",
                 "opentitan-secded-72-64-enc", "opentitan-secded-72-64-dec"):
        assert entries[name]["validation"]["status"] == "passed"
        assert entries[name]["dependency_closure"]["status"] == "complete"


def test_v09_basejump_oracles_cover_new_one_hot_and_butterfly_wrappers() -> None:
    adder = generate_tb("basejump-adder-one-hot", num_src=4, width=7,
                        dut_name="pyc_runtime_basejump_adder_one_hot")
    mux = generate_tb("basejump-mux-one-hot", num_src=4, width=8,
                      dut_name="pyc_runtime_basejump_mux_one_hot")
    butterfly = generate_tb("basejump-mux-butterfly", num_src=4, width=8,
                            dut_name="pyc_runtime_basejump_mux_butterfly")
    assert "ADDER_ONE_HOT_MISMATCH" in adder and ".OUTPUT_WIDTH(OUTPUT_WIDTH)" in adder
    assert "MUX_ONE_HOT_OR_MISMATCH" in mux
    assert "MUX_BUTTERFLY_MISMATCH" in butterfly and "j ^ s" in butterfly


def test_v10_array_concentrate_and_credit_oracles_are_parameterized() -> None:
    array = generate_tb("basejump-array-concentrate-static", num_src=4, width=8,
                        saturate=False,
                        dut_name="pyc_runtime_basejump_array_concentrate_static")
    credit = generate_tb("pulp-credit-counter", num_src=4, width=1,
                         saturate=True, dut_name="pyc_runtime_pulp_credit_counter")
    assert "ARRAY_CONCENTRATE_MISMATCH" in array and "PATTERN" in array
    assert "CREDIT_FLAGS_MISMATCH" in credit and "credit_critical" in credit


def test_v11_ripple_concentrate_and_plru_oracles_are_parameterized() -> None:
    ripple = generate_tb("basejump-adder-ripple-carry", width=13,
                         dut_name="pyc_runtime_basejump_adder_ripple_carry")
    concentrate = generate_tb("basejump-concentrate-static", num_src=8,
                              saturate=False,
                              dut_name="pyc_runtime_basejump_concentrate_static")
    plru = generate_tb("pulp-plru-tree", num_src=8,
                       dut_name="pyc_runtime_pulp_plru_tree")
    assert "ADDER_RIPPLE_MISMATCH" in ripple and "WIDTH = 13" in ripple
    assert "CONCENTRATE_MISMATCH" in concentrate and "PATTERN" in concentrate
    assert "PLRU_MISMATCH" in plru and "used_7" in plru


def test_v12_mux_unconcentrate_and_counter_oracles_are_parameterized() -> None:
    mux = generate_tb("basejump-mux", num_src=5, width=13,
                      dut_name="pyc_runtime_basejump_mux")
    unconcentrate = generate_tb("basejump-unconcentrate-static", num_src=8,
                                saturate=False,
                                dut_name="pyc_runtime_basejump_unconcentrate_static")
    counter = generate_tb("basejump-counter-clear-up-saturating", num_src=7,
                          width=4,
                          dut_name="pyc_runtime_basejump_counter_clear_up_saturating")
    assert "MUX_MISMATCH" in mux and "ELS = 5" in mux
    assert "UNCONCENTRATE_MISMATCH" in unconcentrate and "PATTERN" in unconcentrate
    assert "COUNTER_MISMATCH" in counter and "MAX_VALUE = 7" in counter


def test_v13_priority_and_channel_narrow_oracles_are_parameterized() -> None:
    priority = generate_tb("vortex-priority-encoder", width=13, saturate=False,
                           dut_name="pyc_runtime_vortex_priority_encoder")
    channel = generate_tb("basejump-channel-narrow", num_src=8, width=4,
                          saturate=False,
                          dut_name="pyc_runtime_basejump_channel_narrow")
    assert "priority encoder mismatch" in priority
    assert "onehot_out" in priority and ".MODEL(MODEL)" in priority
    assert "channel narrow mismatch" in channel
    assert "WIDTH_IN = 8" in channel and "WIDTH_OUT = 4" in channel


def test_v13_catalog_entries_are_closed_and_accepted() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    # The catalog is append-only across release batches; v0.14 adds six
    # OpenTitan inverse-SECDED entries on top of the v0.13 baseline.
    assert len(catalog["entries"]) >= 85
    for name in ("vortex-priority-encoder", "basejump-channel-narrow"):
        entry = entries[name]
        assert entry["status"] == "accepted"
        assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
        assert entry["dependency_closure"]["status"] == "complete"
        runtime_root = root / "library/verilog"
        assert all((runtime_root / item).is_file() for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_v14_inverse_secded_oracles_are_registered() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    for name in (
        "opentitan-secded-inv-22-16-enc", "opentitan-secded-inv-22-16-dec",
        "opentitan-secded-inv-28-22-enc", "opentitan-secded-inv-28-22-dec",
        "opentitan-secded-inv-39-32-enc", "opentitan-secded-inv-39-32-dec",
    ):
        entry = entries[name]
        assert entry["status"] == "accepted"
        assert entry["validation"]["semantic_status"] == "functional_oracle_v1"


def test_v26_hamming_variants_are_registered_with_packed_interfaces() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {item["name"]: item for item in catalog["entries"]}
    expected = {
        "opentitan-secded-hamming-22-16-enc": (16, 22),
        "opentitan-secded-hamming-22-16-dec": (22, 16),
        "opentitan-secded-inv-hamming-22-16-enc": (16, 22),
        "opentitan-secded-inv-hamming-22-16-dec": (22, 16),
        "opentitan-secded-hamming-39-32-enc": (32, 39),
        "opentitan-secded-hamming-39-32-dec": (39, 32),
        "opentitan-secded-inv-hamming-39-32-enc": (32, 39),
        "opentitan-secded-inv-hamming-39-32-dec": (39, 32),
        "opentitan-secded-hamming-72-64-enc": (64, 72),
        "opentitan-secded-hamming-72-64-dec": (72, 64),
        "opentitan-secded-inv-hamming-72-64-enc": (64, 72),
        "opentitan-secded-inv-hamming-72-64-dec": (72, 64),
    }
    for name, (in_width, out_width) in expected.items():
        entry = entries[name]
        assert entry["status"] == "accepted"
        assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
        assert entry["dependency_closure"]["status"] == "complete"
        ports = entry["interface"]["ports"]
        assert ports[0]["width"] == str(in_width)
        assert ports[1]["width"] == str(out_width)
        assert all((root / "library/verilog" / item).is_file()
                   for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_v27_onehot_mux_oracle_and_catalog_entry() -> None:
    root = Path(__file__).resolve().parents[4]
    tb = generate_tb("opentitan-onehot-mux", num_src=5, width=3,
                     dut_name="pyc_runtime_opentitan_onehot_mux")
    assert "OpenTitan onehot mux" in tb
    assert "data_in[0 +: WIDTH]" in tb and "zero-select" in tb
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entry = {item["name"]: item for item in catalog["entries"]}["opentitan-onehot-mux"]
    assert entry["status"] == "accepted"
    assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
    assert entry["dependency_closure"]["status"] == "complete"
    runtime_root = root / "library/verilog"
    assert all((runtime_root / item).is_file()
               for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_v28_iterative_multiplier_oracle_and_catalog_entry() -> None:
    root = Path(__file__).resolve().parents[4]
    tb = generate_tb("basejump-imul-iterative", width=8,
                     dut_name="pyc_runtime_basejump_imul_iterative")
    assert "iterative multiplier mismatch" in tb
    assert "out_ready" in tb and "high_part" in tb
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entry = {item["name"]: item for item in catalog["entries"]}["basejump-imul-iterative"]
    assert entry["status"] == "accepted"
    assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
    assert entry["dependency_closure"]["status"] == "complete"
    runtime_root = root / "library/verilog"
    assert all((runtime_root / item).is_file()
               for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_v29_vortex_ks_adder_oracle_and_catalog_entry() -> None:
    root = Path(__file__).resolve().parents[4]
    tb = generate_tb("vortex-ks-adder", width=13,
                     dut_name="pyc_runtime_vortex_ks_adder")
    assert "Vortex KS adder mismatch" in tb
    assert "wanted = av + bv + cv" in tb
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entry = {item["name"]: item for item in catalog["entries"]}["vortex-ks-adder"]
    assert entry["status"] == "accepted"
    assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
    assert entry["validation"]["configs"][-1] == {"WIDTH": 8, "BYPASS": 1}
    assert entry["dependency_closure"]["status"] == "complete"
    runtime_root = root / "library/verilog"
    assert all((runtime_root / item).is_file()
               for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_v30_vortex_fanout_buffer_oracle_and_catalog_entry() -> None:
    root = Path(__file__).resolve().parents[4]
    tb = generate_tb("vortex-fanout-buffer", num_src=13, width=8,
                     dut_name="pyc_runtime_vortex_fanout_buffer")
    assert "Vortex fanout buffer mismatch" in tb
    assert "OUTPUTS = 13" in tb and "MAX_FANOUT = 8" in tb
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entry = {item["name"]: item for item in catalog["entries"]}["vortex-fanout-buffer"]
    assert entry["status"] == "accepted"
    assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
    assert entry["dependency_closure"]["status"] == "complete"
    runtime_root = root / "library/verilog"
    assert all((runtime_root / item).is_file()
               for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_v16_mux_and_two_phase_cdc_oracles_are_parameterized() -> None:
    mux = generate_tb("vortex-mux", num_src=5, width=8,
                      dut_name="pyc_runtime_vortex_mux")
    demux = generate_tb("vortex-demux", num_src=5, width=8, saturate=False,
                        dut_name="pyc_runtime_vortex_demux")
    onehot = generate_tb("vortex-onehot-mux", num_src=5, width=8,
                         saturate=False,
                         dut_name="pyc_runtime_vortex_onehot_mux")
    cdc = generate_tb("pulp-cdc-fifo-2phase", num_src=2, width=2,
                      dut_name="pyc_runtime_pulp_cdc_fifo_2phase")
    fifo = generate_tb("basejump-fifo-small", num_src=2, width=4,
                       dut_name="pyc_runtime_basejump_fifo_small")
    assert "Vortex mux mismatch" in mux and "INPUTS = 5" in mux
    assert "Vortex demux mismatch" in demux and "MODEL(1)" in demux
    assert "Vortex onehot mux mismatch" in onehot and "select_onehot" in onehot
    assert "2phase FIFO" in cdc and "PYC_RUNTIME_FUNCTIONAL_PASS" in cdc
    assert "BaseJump FIFO" in fifo and "PYC_RUNTIME_FUNCTIONAL_PASS" in fifo


def test_v16_catalog_entries_are_vendored_and_accepted() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    for name in ("basejump-fifo-small", "vortex-mux", "vortex-demux", "vortex-onehot-mux", "pulp-cdc-fifo-2phase"):
        entry = entries[name]
        assert entry["status"] == "accepted"
        assert entry["validation"]["status"] == "passed"
        assert entry["dependency_closure"]["status"] == "complete"
        runtime_root = root / "library/verilog"
        assert all((runtime_root / item).is_file() for item in entry["files"] + entry["dependency_closure"]["license_files"])
        assert entry["dependency_closure"]["status"] == "complete"


def test_v18_isochronous_spill_oracle_and_catalog_entry() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entry = {item["name"]: item for item in catalog["entries"]}["pulp-isochronous-spill-register"]
    assert entry["status"] == "accepted"
    assert entry["validation"]["status"] == "passed"
    assert entry["dependency_closure"]["status"] == "complete"


def test_v19_clk_or_tree_oracle_covers_recursive_fan_in() -> None:
    tree = generate_tb("pulp-clk-or-tree", num_src=5,
                       dut_name="pyc_runtime_pulp_clk_or_tree")
    assert "CLK_OR_TREE_MISMATCH" in tree
    assert "NUM_INPUTS = 5" in tree
    assert "(|clks_in)" in tree


def test_v19_clk_or_tree_catalog_entry_is_closed_and_accepted() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entry = {item["name"]: item for item in catalog["entries"]}["pulp-clk-or-tree"]
    assert entry["status"] == "accepted"
    assert entry["validation"]["status"] == "passed"
    assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
    assert entry["dependency_closure"]["status"] == "complete"
    runtime_root = root / "library/verilog"
    assert all((runtime_root / item).is_file()
               for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_v20_fall_through_register_oracle_is_parameterized() -> None:
    fall_through = generate_tb(
        "pulp-fall-through-register", width=13,
        dut_name="pyc_runtime_pulp_fall_through_register")
    assert "fall-through bypass mismatch" in fall_through
    assert "WIDTH = 13" in fall_through
    assert "PYC_RUNTIME_FUNCTIONAL_PASS" in fall_through


def test_v20_fall_through_register_catalog_entry_is_closed_and_accepted() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entry = {item["name"]: item for item in catalog["entries"]}["pulp-fall-through-register"]
    assert entry["status"] == "accepted"
    assert entry["validation"]["status"] == "passed"
    assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
    assert entry["dependency_closure"]["status"] == "complete"
    runtime_root = root / "library/verilog"
    assert all((runtime_root / item).is_file()
               for item in entry["files"] + entry["dependency_closure"]["license_files"])
    runtime_root = root / "library/verilog"
    assert all((runtime_root / item).is_file() for item in entry["files"] + entry["dependency_closure"]["license_files"])
    buffered = generate_tb("pulp-isochronous-spill-register", width=4, saturate=False,
                           dut_name="pyc_runtime_pulp_isochronous_spill_register")
    bypass = generate_tb("pulp-isochronous-spill-register", width=8, saturate=True,
                         dut_name="pyc_runtime_pulp_isochronous_spill_register")
    assert "related" in buffered and "BYPASS(BYPASS)" in bypass


def test_v21_basejump_round_robin_variants_have_explicit_oracles() -> None:
    composable = generate_tb(
        "basejump-rr-composable", num_src=8,
        dut_name="pyc_runtime_basejump_rr_composable")
    two_level = generate_tb(
        "basejump-rr-two-level", num_src=4,
        dut_name="pyc_runtime_basejump_rr_two_level")
    assert "composable arbiter sparse priority mismatch" in composable
    assert "THERMO_WIDTH = 7" in composable
    assert "two-level high-priority mismatch" in two_level
    assert "two-level round-robin advance mismatch" in two_level


def test_v21_basejump_round_robin_entries_are_closed_and_accepted() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {item["name"]: item for item in catalog["entries"]}
    runtime_root = root / "library/verilog"
    for name in ("basejump-rr-composable", "basejump-rr-two-level"):
        entry = entries[name]
        assert entry["status"] == "accepted"
        assert entry["validation"]["status"] == "passed"
        assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
        assert entry["dependency_closure"]["status"] == "complete"
        assert all((runtime_root / item).is_file()
                   for item in entry["files"] + entry["dependency_closure"]["license_files"])


def test_v15_hamming_76_68_oracles_are_registered() -> None:
    root = Path(__file__).resolve().parents[4]
    catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
    entries = {entry["name"]: entry for entry in catalog["entries"]}
    for name, kind, dut in (
        ("opentitan-secded-hamming-76-68-enc", "opentitan-secded-hamming-76-68-enc", "pyc_runtime_opentitan_secded_hamming_76_68_enc"),
        ("opentitan-secded-hamming-76-68-dec", "opentitan-secded-hamming-76-68-dec", "pyc_runtime_opentitan_secded_hamming_76_68_dec"),
        ("opentitan-secded-inv-hamming-76-68-enc", "opentitan-secded-inv-hamming-76-68-enc", "pyc_runtime_opentitan_secded_inv_hamming_76_68_enc"),
        ("opentitan-secded-inv-hamming-76-68-dec", "opentitan-secded-inv-hamming-76-68-dec", "pyc_runtime_opentitan_secded_inv_hamming_76_68_dec"),
    ):
        entry = entries[name]
        assert entry["status"] == "accepted"
        assert entry["validation"]["semantic_status"] == "functional_oracle_v1"
        assert entry["dependency_closure"]["status"] == "complete"
        assert all((root / "library/verilog" / item).is_file() for item in entry["files"] + entry["dependency_closure"]["license_files"])
        tb = generate_tb(kind, dut_name=dut)
        assert "SECDED" in tb and "PYC_RUNTIME_FUNCTIONAL_PASS" in tb
