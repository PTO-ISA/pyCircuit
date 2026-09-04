#!/usr/bin/env python3
from pathlib import Path
import tempfile
import yaml

from run_timing_benchmark import write_sta_script, config_key

def main():
    specs = yaml.safe_load(
        Path("design_class_specs.yaml").read_text(encoding="utf-8")
    )

    rr = specs["design_classes"]["DF-09"]["timing_contract"]
    fifo = specs["design_classes"]["FIFO-SYNC"]["timing_contract"]

    assert rr["clock_port"] == "clk_i"
    assert "req_i*" in rr["timed_inputs"]
    assert "accept_i" in rr["timed_inputs"]

    assert fifo["clock_port"] == "clk_i"
    assert "in_data_i*" in fifo["timed_inputs"]
    assert "out_data_o*" in fifo["timed_outputs"]
    assert "clr_i" in fifo["timed_inputs"]
    assert "rst_ni" in fifo["false_path_inputs"]

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        tcl = td / "timing.tcl"
        write_sta_script(
            tcl,
            td / "x.lib",
            td / "top.v",
            100.0,
            fifo,
        )
        text = tcl.read_text(encoding="utf-8")
        assert "get_ports {in_data_i*}" in text
        assert "get_ports {out_data_o*}" in text
        assert "set_false_path -from [get_ports {rst_ni}]" in text
        assert "set_input_delay -clock clk 0.0 [get_ports {clr_i}]" in text
        assert "report_checks -path_delay max -group_count 1" in text

    assert config_key({"config": "n16"}) == "n16"
    assert config_key({"config": "w32_d16"}) == "w32_d16"

    print("smoke_test_v0.12: PASS")

if __name__ == "__main__":
    main()
