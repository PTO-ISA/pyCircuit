#!/usr/bin/env python3
from stateful_tb_generator import generate_stateful_tb

def main():
    modes = [
        "fair_internal", "unfair_internal", "backpressure_hold",
        "reset_clear", "lockin", "axi_vld_rdy"
    ]
    for mode in modes:
        tb = generate_stateful_tb(
            {"name": mode, "mode": mode, "num_in": 4, "data_width": 16, "cycles": 16},
            0x12345678
        )
        assert "cc_rr_arb_tree" in tb
        assert ".ExtPrio   ( 1'b0 )" in tb
        assert "PYC_STATEFUL_RESULT PASS" in tb
    print("smoke_test_v0.4.1: PASS")

if __name__ == "__main__":
    main()
