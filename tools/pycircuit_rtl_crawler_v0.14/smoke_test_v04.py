#!/usr/bin/env python3
from tb_generators import generate
from run_correctness import parse_sim_result, stable_case_id

def main():
    lzc = generate("lzc", {"width": 8, "mode": "leading", "random_tests": 10}, 0x1234)
    assert "cc_lzc" in lzc
    assert "ref_count" in lzc
    assert "PYC_CORRECTNESS_RESULT PASS" in lzc

    pop = generate("popcount", {"width": 8, "random_tests": 10}, 0x1234)
    assert "cc_popcount" in pop
    assert "ref_popcount" in pop

    arb = generate("rr_arb_extprio", {"num_in": 4, "data_width": 16, "random_tests": 10}, 0x1234)
    assert "cc_rr_arb_tree" in arb
    assert "expected_winner" in arb
    assert ".ExtPrio    ( 1'b1 )" in arb

    parsed = parse_sim_result(
        "hello\nPYC_CORRECTNESS_RESULT PASS module=cc_lzc tests=123 errors=0\n"
    )
    assert parsed["status"] == "PASS"
    assert parsed["tests"] == 123
    assert parsed["errors"] == 0

    cid1 = stable_case_id("cc_lzc", "lzc", {"width": 8}, 1)
    cid2 = stable_case_id("cc_lzc", "lzc", {"width": 8}, 1)
    assert cid1 == cid2

    print("smoke_test_v0.4: PASS")

if __name__ == "__main__":
    main()
