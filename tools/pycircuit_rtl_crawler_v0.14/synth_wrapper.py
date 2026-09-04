
from __future__ import annotations
from typing import Dict


def gen_lzc(cfg: Dict) -> str:
    width = int(cfg["width"])
    mode = str(cfg.get("mode", "leading"))
    mode_expr = (
        "cc_pkg::LZC_LEADING_ZERO_CNT"
        if mode == "leading"
        else "cc_pkg::LZC_TRAILING_ZERO_CNT"
    )
    return f"""module pyc_synth_top (
  input  logic [{width-1}:0] in_i,
  output logic [cc_pkg::idx_width({width})-1:0] cnt_o,
  output logic empty_o
);
  cc_lzc #(
    .Width ({width}),
    .Mode  ({mode_expr})
  ) dut (
    .in_i,
    .cnt_o,
    .empty_o
  );
endmodule
"""


def gen_popcount(cfg: Dict) -> str:
    width = int(cfg["width"])
    out_w = max(1, (width + 1).bit_length() - 1)
    # ceil(log2(width+1))
    import math
    out_w = max(1, math.ceil(math.log2(width + 1)))
    return f"""module pyc_synth_top (
  input  logic [{width-1}:0] data_i,
  output logic [{out_w-1}:0] popcount_o
);
  cc_popcount #(
    .InputWidth ({width})
  ) dut (
    .data_i,
    .popcount_o
  );
endmodule
"""


def gen_rr(cfg: Dict) -> str:
    n = int(cfg["num_in"])
    w = int(cfg.get("data_width", 16))
    ext = int(cfg.get("ext_prio", 1))
    axi = int(cfg.get("axi_vld_rdy", 0))
    lock = int(cfg.get("lock_in", 0))
    fair = int(cfg.get("fair_arb", 1))
    import math
    idx_w = max(1, math.ceil(math.log2(n)))
    return f"""module pyc_synth_top (
  input  logic clk_i,
  input  logic rst_ni,
  input  logic clr_i,
  input  logic [{idx_w-1}:0] rr_i,
  input  logic [{n-1}:0] req_i,
  output logic [{n-1}:0] gnt_o,
  input  logic [{n-1}:0][{w-1}:0] data_i,
  output logic req_o,
  input  logic gnt_i,
  output logic [{w-1}:0] data_o,
  output logic [{idx_w-1}:0] idx_o
);
  cc_rr_arb_tree #(
    .NumIn      ({n}),
    .DataWidth  ({w}),
    .ExtPrio    (1'b{ext}),
    .AxiVldRdy  (1'b{axi}),
    .LockIn     (1'b{lock}),
    .FairArb    (1'b{fair})
  ) dut (
    .clk_i,
    .rst_ni,
    .clr_i,
    .rr_i,
    .req_i,
    .gnt_o,
    .data_i,
    .req_o,
    .gnt_i,
    .data_o,
    .idx_o
  );
endmodule
"""


GENERATORS = {
    "cc_lzc": gen_lzc,
    "cc_popcount": gen_popcount,
    "cc_rr_arb_tree": gen_rr,
}


def generate_wrapper(module: str, cfg: Dict) -> str:
    if module not in GENERATORS:
        raise KeyError(f"Unsupported synthesis module: {module}")
    return GENERATORS[module](cfg)
