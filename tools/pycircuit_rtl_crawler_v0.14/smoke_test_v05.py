#!/usr/bin/env python3
from synth_wrapper import generate_wrapper

def main():
    a = generate_wrapper("cc_lzc", {"width": 8, "mode": "leading"})
    assert "cc_lzc" in a and "pyc_synth_top" in a

    b = generate_wrapper("cc_popcount", {"width": 16})
    assert "cc_popcount" in b and "InputWidth (16)" in b

    c = generate_wrapper(
        "cc_rr_arb_tree",
        {"num_in": 4, "data_width": 16, "ext_prio": 0,
         "axi_vld_rdy": 0, "lock_in": 1, "fair_arb": 1}
    )
    assert "cc_rr_arb_tree" in c
    assert ".ExtPrio    (1'b0)" in c
    assert ".LockIn     (1'b1)" in c
    assert "logic [3:0][15:0] data_i" in c

    print("smoke_test_v0.5: PASS")

if __name__ == "__main__":
    main()
