from __future__ import annotations

from pathlib import Path

from acir_runtime_full_validation import (
    PARSER_TOKEN_PRUNES,
    choose_sweeps,
    generate_tb,
    generate_typed_closed_wrapper,
    generate_wrapper,
    load_candidates,
    normalize_port,
    parameter_values,
    recover_legacy_port_metadata,
    recover_known_top_metadata,
    stat_metrics,
    unsupported_port_shape,
)


ROOT = Path(__file__).resolve().parents[4]


def test_parser_token_prunes_are_explicit_and_project_scoped() -> None:
    assert "unique" in PARSER_TOKEN_PRUNES["basejump_stl"]
    assert "chandle" in PARSER_TOKEN_PRUNES["basejump_stl"]
    assert "void" in PARSER_TOKEN_PRUNES["basejump_stl"]
    assert "unique" in PARSER_TOKEN_PRUNES["pulp_common_cells"]
    assert "VX_config.vh" not in PARSER_TOKEN_PRUNES.get("vortex", ())


def test_type_parameters_keep_the_native_default() -> None:
    candidate = {
        "module": "typed_reg",
        "parameters": [
            {"name": "WIDTH", "type": "int", "default": "8", "raw": "parameter int WIDTH=8"},
            {"name": "data_t", "type": "type", "default": "logic [WIDTH-1:0]", "raw": "parameter type data_t=logic [WIDTH-1:0]"},
        ],
        "ports": [
            {"name": "i", "direction": "input", "type": "logic", "width": "[WIDTH-1:0]", "raw": "input logic [WIDTH-1:0] i"},
            {"name": "o", "direction": "output", "type": "logic", "width": "[WIDTH-1:0]", "raw": "output logic [WIDTH-1:0] o"},
        ],
    }
    values = parameter_values(candidate)
    assert values == {"WIDTH": 8}
    wrapper, _ = generate_wrapper(candidate, values)
    assert ".data_t(" not in wrapper
    assert ".WIDTH(8)" in wrapper


def test_unpacked_ports_are_folded_into_structural_wrapper() -> None:
    candidate = {
        "ports": [{
            "name": "data_i", "direction": "input", "type": "logic",
            "width": "[WIDTH-1:0]",
            "raw": "input logic [WIDTH-1:0] data_i [N-1:0]",
        }]
    }
    assert unsupported_port_shape(candidate) == ""


def test_unpacked_native_temporary_keeps_array_shape() -> None:
    candidate = {
        "module": "array_port",
        "parameters": [],
        "ports": [{
            "name": "data_i", "direction": "input", "type": "logic",
            "width": "[7:0]", "unpacked": "[3:0]",
            "raw": "input logic [7:0] data_i [3:0]",
        }],
    }
    wrapper, _ = generate_wrapper(candidate, {})
    assert "logic [7:0] __native_data_i [3:0];" in wrapper
    assert "assign __native_data_i = {>>{data_i}};" in wrapper


def test_ansi_typed_unpacked_output_keeps_output_direction(tmp_path: Path) -> None:
    # The legacy recovery pass must not reinterpret an ANSI port list as one
    # semicolon-terminated ``input`` declaration.  This is the shape used by
    # PULP memory-bank outputs such as ``output addr_t [N-1:0] bank_addr_o``.
    source = tmp_path / "repos" / "pulp_common_cells" / "src" / "cc_mem_to_banks.sv"
    source.parent.mkdir(parents=True)
    source.write_text(
        "module cc_mem_to_banks #(parameter int N=1) (\n"
        "  input logic clk_i,\n"
        "  output addr_t [N-1:0] bank_addr_o,\n"
        "  input logic [N-1:0] bank_rvalid_i\n"
        ");\nendmodule\n",
        encoding="utf-8",
    )
    candidate = {
        "source_project": "pulp_common_cells",
        "module": "cc_mem_to_banks",
        "file": "src/cc_mem_to_banks.sv",
        "ports": [
            {"name": "clk_i", "direction": "input", "type": "logic", "width": "", "raw": "input logic clk_i"},
            {"name": "bank_addr_o", "direction": "output", "type": "addr_t", "width": "", "unpacked": "[N-1:0]", "raw": "output addr_t [N-1:0] bank_addr_o"},
            {"name": "bank_rvalid_i", "direction": "input", "type": "logic", "width": "[N-1:0]", "raw": "input logic [N-1:0] bank_rvalid_i"},
        ],
    }
    recover_legacy_port_metadata(candidate, tmp_path)
    bank_addr = next(port for port in candidate["ports"] if port["name"] == "bank_addr_o")
    assert bank_addr["direction"] == "output"
    assert bank_addr["unpacked"] == "[N-1:0]"


def test_implicit_multi_packed_port_keeps_all_dimensions_and_direction() -> None:
    normalized = normalize_port({"raw": "output [O-1:0][I-1:0] grants_o"})
    assert normalized["direction"] == "output"
    assert normalized["width"] == "[O-1:0][I-1:0]"
    candidate = {
        "module": "crossbar_control",
        "ports": [{
            "name": "grants_o", "direction": "output", "type": "",
            "width": "[O-1:0][I-1:0]",
            "raw": "output [O-1:0][I-1:0] grants_o",
        }]
    }
    wrapper, _ = generate_wrapper(candidate, {"O": 4, "I": 2})
    assert "output logic [3:0][1:0] grants_o" in wrapper


def test_sweep_is_two_bounded_points() -> None:
    candidate = {
        "parameters": [{"name": "WIDTH", "type": "int", "default": "32", "raw": "parameter int WIDTH=32"}]
    }
    sweeps = choose_sweeps(candidate)
    assert [item["WIDTH"] for item in sweeps] == [4, 32]


def test_basejump_round_robin_arbiter_sweep_uses_legal_input_counts() -> None:
    candidate = {
        "source_project": "basejump_stl",
        "module": "bsg_round_robin_arb",
        "parameters": [
            {"name": "inputs_p", "type": "`BSG_INV_PARAM(", "default": "", "raw": "parameter `BSG_INV_PARAM(inputs_p)"},
            {"name": "lg_inputs_p", "type": "", "default": "`BSG_SAFE_CLOG2(inputs_p)", "raw": "lg_inputs_p=`BSG_SAFE_CLOG2(inputs_p)"},
        ],
    }
    sweeps = choose_sweeps(candidate)
    assert [item["inputs_p"] for item in sweeps] == [2, 4]
    assert [item["lg_inputs_p"] for item in sweeps] == [1, 2]


def test_derived_widths_and_string_parameters_are_preserved() -> None:
    candidate = {
        "parameters": [
            {"name": "NUM", "type": "int", "default": "4", "raw": "parameter int NUM=4"},
            {"name": "ADDR_W", "type": "", "default": "`LOG2UP(NUM)", "raw": "parameter ADDR_W=`LOG2UP(NUM)"},
            {"name": "ARBITER", "type": "`STRING", "default": '"R"', "raw": 'parameter `STRING ARBITER="R"'},
        ],
        "ports": [],
        "module": "string_param",
    }
    assert parameter_values(candidate) == {"NUM": 4, "ADDR_W": 2}
    wrapper, _ = generate_wrapper(candidate, parameter_values(candidate))
    assert '.ARBITER("R")' in wrapper


def test_dpi_memory_sweep_uses_byte_granular_widths() -> None:
    candidate = {
        "module": "bsg_nonsynth_mem_1r1w_sync_dma",
        "parameters": [{"name": "width_p", "type": "int", "default": "8", "raw": "parameter int width_p=8"}],
    }
    assert [item["width_p"] for item in choose_sweeps(candidate)] == [8, 16]


def test_vortex_stream_xbar_sweep_avoids_identical_recursive_fanout() -> None:
    candidate = {
        "module": "VX_stream_xbar",
        "parameters": [
            {"name": "NUM_INPUTS", "type": "int", "default": "4", "raw": "parameter NUM_INPUTS=4"},
            {"name": "NUM_OUTPUTS", "type": "int", "default": "4", "raw": "parameter NUM_OUTPUTS=4"},
            {"name": "MAX_FANOUT", "type": "int", "default": "1", "raw": "parameter MAX_FANOUT=1"},
        ],
    }
    sweeps = choose_sweeps(candidate)
    assert [item["NUM_INPUTS"] for item in sweeps] == [2, 4]
    assert {item["MAX_FANOUT"] for item in sweeps} == {2}


def test_common_cells_typed_sweeps_skip_type_parameters_and_resolve_idx_width() -> None:
    candidate = {
        "source_project": "pulp_common_cells",
        "module": "cc_stream_mux",
        "parameters": [
            {"name": "data_t", "type": "type", "default": "logic", "raw": "parameter type data_t = logic"},
            {"name": "NumInp", "type": "int unsigned", "default": "1", "raw": "parameter int unsigned NumInp = 1"},
            {"name": "SelWidth", "type": "int unsigned", "default": "cc_pkg::idx_width(NumInp)", "raw": "parameter int unsigned SelWidth = cc_pkg::idx_width(NumInp)"},
        ],
        "ports": [],
    }
    sweeps = choose_sweeps(candidate)
    assert [item["NumInp"] for item in sweeps] == [2, 4]
    assert [item["SelWidth"] for item in sweeps] == [1, 2]
    assert all("data_t" not in item for item in sweeps)


def test_common_cells_typed_wrapper_flattens_payload_arrays() -> None:
    candidate = {
        "source_project": "pulp_common_cells",
        "module": "cc_stream_arbiter",
        "parameters": [
            {"name": "data_t", "type": "type", "default": "logic", "raw": "parameter type data_t = logic"},
            {"name": "NumInp", "type": "int unsigned", "default": "1", "raw": "parameter int unsigned NumInp = 1"},
        ],
        "ports": [
            {"name": "inp_data_i", "direction": "input", "type": "data_t", "width": "", "unpacked": "[NumInp-1:0]", "raw": "input data_t [NumInp-1:0] inp_data_i"},
        ],
    }
    wrapper = generate_typed_closed_wrapper(candidate, {"NumInp": 2})
    assert wrapper is not None
    assert "pyc_data_t [1:0] inp_data_i" in wrapper
    assert ".data_t(pyc_data_t)" in wrapper


def test_interface_only_xbar_gets_closed_minimal_smoke() -> None:
    tb = generate_tb({"module": "VX_mem_axi_xbar", "ports": []}, {})
    assert "pyc_synth_top dut ();" in tb
    assert ".clk(" not in tb


def test_stat_metrics_reads_mapped_liberty_area(tmp_path: Path) -> None:
    stats = tmp_path / "stats.json"
    stats.write_text('{"modules":{"\\\\pyc_synth_top":{"num_cells":7}}}', encoding="utf-8")
    mapped = tmp_path / "mapped_stats.json"
    mapped.write_text('{"modules":{"\\\\pyc_synth_top":{"num_cells":9,"area":12.5}}}', encoding="utf-8")
    assert stat_metrics(stats) == (7, None, None)
    assert stat_metrics(mapped) == (9, None, 12.5)


def test_known_package_import_tops_recover_ports_and_parameters() -> None:
    candidate = {"source_project": "pulp_common_cells", "module": "cc_lzc", "ports": [], "parameters": []}
    recover_known_top_metadata(candidate)
    assert [port["name"] for port in candidate["ports"]] == ["in_i", "cnt_o", "empty_o"]
    assert [item["name"] for item in candidate["parameters"]] == ["Width", "Mode"]


def test_real_inventory_deduplicates_unique_tops() -> None:
    inventory = ROOT / "tools" / "pycircuit_rtl_crawler_v0.14" / "inventory" / "candidates_frozen.csv"
    if inventory.exists():
        candidates = load_candidates(inventory)
        assert len(candidates) == len({(x["source_project"], x["module"], x["file"], x["commit_sha"]) for x in candidates})
        assert len(candidates) >= 50
