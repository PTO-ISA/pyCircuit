#!/usr/bin/env python3
from pathlib import Path
import tempfile
import yaml

from design_class_adapters import generate_adapter
from design_class_tb import generate_popcount_property_tb
from run_timing_benchmark import write_sta_script

def main():
    specs = yaml.safe_load(
        Path("design_class_specs.yaml").read_text(encoding="utf-8")
    )
    dc = specs["design_classes"]["INT-11"]
    assert dc["benchmark_kind"] == "popcount"
    assert len(dc["candidates"]) == 3
    assert {c["project"] for c in dc["candidates"]} == {
        "pulp_common_cells", "basejump_stl", "vortex"
    }

    cfg = dc["profiles"]["smoke"][0]
    for c in dc["candidates"]:
        sv = generate_adapter(c["adapter"], cfg)
        assert "module pyc_synth_top" in sv
        assert c["module"] in sv
        assert "count_o" in sv

    tb = generate_popcount_property_tb(8, 16)
    assert "golden_popcount" in tb
    assert "PYC_DC_PASS INT-11" in tb

    vortex = next(c for c in dc["candidates"] if c["project"] == "vortex")
    assert vortex["defines"] == ["SYNTHESIS"]

    contract = dc["timing_contract"]
    assert contract["clock_port"] is None
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        tcl = td / "timing.tcl"
        write_sta_script(
            tcl,
            td / "x.lib",
            td / "top.v",
            100.0,
            contract,
        )
        text = tcl.read_text(encoding="utf-8")
        assert "create_clock -name vclk -period 100.000000" in text
        assert "get_ports {data_i*}" in text
        assert "get_ports {count_o*}" in text
        assert "set_input_delay -clock vclk" in text
        assert "set_output_delay -clock vclk" in text

    sources = yaml.safe_load(Path("sources.yaml").read_text(encoding="utf-8"))
    vx = next(x for x in sources["sources"] if x["project"] == "vortex")
    assert vx["enabled"] is True
    assert vx["path_hints"] == ["hw/rtl"]

    road = yaml.safe_load(Path("design_family_roadmap.yaml").read_text(encoding="utf-8"))
    assert len(road["families"]) == 12

    print("smoke_test_v0.14: PASS")

if __name__ == "__main__":
    main()
