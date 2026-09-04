from __future__ import annotations

def _header(n: int) -> str:
    return f"""
module pyc_synth_top #(
  parameter int N = {n}
) (
  input  logic         clk_i,
  input  logic         rst_ni,
  input  logic [N-1:0] req_i,
  input  logic         accept_i,
  output logic         valid_o,
  output logic [N-1:0] sel_o
);
  localparam int IDX_W = (N > 1) ? $clog2(N) : 1;
"""

def _footer() -> str:
    return "\nendmodule\n"

def gen_pulp_cc_rr(n: int) -> str:
    return _header(n) + r"""
  logic [N-1:0] native_gnt;
  logic [IDX_W-1:0] native_idx;
  logic native_req_o;
  logic [N-1:0][0:0] dummy_data_i;
  logic [0:0] dummy_data_o;

  assign dummy_data_i = '0;
  assign valid_o = native_req_o;

  always_comb begin
    sel_o = '0;
    if (native_req_o)
      sel_o[native_idx] = 1'b1;
  end

  cc_rr_arb_tree #(
    .NumIn      ( N ),
    .DataWidth  ( 1 ),
    .ExtPrio    ( 1'b0 ),
    .AxiVldRdy  ( 1'b1 ),
    .LockIn     ( 1'b1 ),
    .FairArb    ( 1'b1 )
  ) dut (
    .clk_i  ( clk_i ),
    .rst_ni ( rst_ni ),
    .clr_i  ( 1'b0 ),
    .rr_i   ( '0 ),
    .req_i  ( req_i ),
    .gnt_o  ( native_gnt ),
    .data_i ( dummy_data_i ),
    .req_o  ( native_req_o ),
    .gnt_i  ( accept_i ),
    .data_o ( dummy_data_o ),
    .idx_o  ( native_idx )
  );
""" + _footer()

def gen_basejump_bsg_rr(n: int) -> str:
    return _header(n) + r"""
  logic yumi;

  assign valid_o = |req_i;
  assign yumi = accept_i & valid_o;

  bsg_arb_round_robin #(
    .width_p ( N )
  ) dut (
    .clk_i    ( clk_i ),
    .reset_i  ( ~rst_ni ),
    .reqs_i   ( req_i ),
    .grants_o ( sel_o ),
    .yumi_i   ( yumi )
  );
""" + _footer()

def gen_opentitan_prim_tree(n: int) -> str:
    return _header(n) + r"""
  logic [N-1:0] native_gnt;
  logic [IDX_W-1:0] native_idx;
  logic [0:0] dummy_data_i [N];
  logic [0:0] dummy_data_o;

  for (genvar i = 0; i < N; i++) begin : gen_dummy
    assign dummy_data_i[i] = '0;
  end

  always_comb begin
    sel_o = '0;
    if (valid_o)
      sel_o[native_idx] = 1'b1;
  end

  prim_arbiter_tree #(
    .N          ( N ),
    .DW         ( 1 ),
    .EnDataPort ( 1'b0 )
  ) dut (
    .clk_i     ( clk_i ),
    .rst_ni    ( rst_ni ),
    .req_chk_i ( 1'b1 ),
    .req_i     ( req_i ),
    .data_i    ( dummy_data_i ),
    .gnt_o     ( native_gnt ),
    .idx_o     ( native_idx ),
    .valid_o   ( valid_o ),
    .data_o    ( dummy_data_o ),
    .ready_i   ( accept_i )
  );
""" + _footer()


def _fifo_header(width: int, capacity: int) -> str:
    return f"""
module pyc_synth_top #(
  parameter int DATA_W = {width},
  parameter int DEPTH  = {capacity}
) (
  input  logic              clk_i,
  input  logic              rst_ni,
  input  logic              clr_i,
  input  logic              in_valid_i,
  output logic              in_ready_o,
  input  logic [DATA_W-1:0] in_data_i,
  output logic              out_valid_o,
  input  logic              out_ready_i,
  output logic [DATA_W-1:0] out_data_o
);
"""

def gen_pulp_cc_fifo(config: dict) -> str:
    w = int(config["data_width"])
    d = int(config["capacity"])
    return _fifo_header(w, d) + r"""
  logic full, empty;
  logic [$clog2(DEPTH+1)-1:0] usage_unused;

  assign in_ready_o  = ~full;
  assign out_valid_o = ~empty;

  cc_fifo #(
    .FallThrough ( 1'b0 ),
    .DataWidth   ( DATA_W ),
    .Depth       ( DEPTH ),
    .data_t      ( logic [DATA_W-1:0] )
  ) dut (
    .clk_i,
    .rst_ni,
    .clr_i,
    .flush_i ( 1'b0 ),
    .full_o  ( full ),
    .empty_o ( empty ),
    .usage_o ( usage_unused ),
    .data_i  ( in_data_i ),
    .push_i  ( in_valid_i & in_ready_o ),
    .data_o  ( out_data_o ),
    .pop_i   ( out_valid_o & out_ready_i )
  );
""" + _footer()

def gen_basejump_bsg_fifo(config: dict) -> str:
    w = int(config["data_width"])
    d = int(config["capacity"])
    return _fifo_header(w, d) + r"""
  logic native_ready;
  logic native_valid;

  assign in_ready_o  = native_ready;
  assign out_valid_o = native_valid;

  bsg_fifo_1r1w_small #(
    .width_p            ( DATA_W ),
    .els_p              ( DEPTH ),
    .harden_p           ( 1'b0 ),
    .ready_THEN_valid_p ( 1'b0 )
  ) dut (
    .clk_i,
    .reset_i       ( ~rst_ni | clr_i ),
    .v_i           ( in_valid_i ),
    .ready_param_o ( native_ready ),
    .data_i        ( in_data_i ),
    .v_o           ( native_valid ),
    .data_o        ( out_data_o ),
    .yumi_i        ( native_valid & out_ready_i )
  );
""" + _footer()

def gen_opentitan_prim_fifo(config: dict) -> str:
    w = int(config["data_width"])
    d = int(config["capacity"])
    return _fifo_header(w, d) + r"""
  logic full_unused;
  logic err_unused;
  logic [$clog2(DEPTH+1)-1:0] depth_unused;

  prim_fifo_sync #(
    .Width              ( DATA_W ),
    .Pass               ( 1'b0 ),
    .Depth              ( DEPTH ),
    .OutputZeroIfEmpty  ( 1'b0 ),
    .NeverClears        ( 1'b0 ),
    .Secure             ( 1'b0 )
  ) dut (
    .clk_i,
    .rst_ni,
    .clr_i,
    .wvalid_i ( in_valid_i ),
    .wready_o ( in_ready_o ),
    .wdata_i  ( in_data_i ),
    .rvalid_o ( out_valid_o ),
    .rready_i ( out_ready_i ),
    .rdata_o  ( out_data_o ),
    .full_o   ( full_unused ),
    .depth_o  ( depth_unused ),
    .err_o    ( err_unused )
  );
""" + _footer()


def _popcount_header(width: int) -> str:
    import math
    count_w = max(1, math.ceil(math.log2(width + 1)))
    return f"""
module pyc_synth_top #(
  parameter int WIDTH = {width},
  parameter int COUNT_W = {count_w}
) (
  input  logic [WIDTH-1:0]   data_i,
  output logic [COUNT_W-1:0] count_o
);
"""


def gen_pulp_cc_popcount(config: dict) -> str:
    w = int(config["width"])
    return _popcount_header(w) + r"""
  cc_popcount #(
    .InputWidth ( WIDTH )
  ) dut (
    .data_i      ( data_i ),
    .popcount_o  ( count_o )
  );
""" + _footer()


def gen_basejump_bsg_popcount(config: dict) -> str:
    w = int(config["width"])
    return _popcount_header(w) + r"""
  bsg_popcount #(
    .width_p ( WIDTH )
  ) dut (
    .i ( data_i ),
    .o ( count_o )
  );
""" + _footer()


def gen_vortex_vx_popcount(config: dict) -> str:
    w = int(config["width"])
    return _popcount_header(w) + r"""
  VX_popcount #(
    .MODEL ( 1 ),
    .N     ( WIDTH ),
    .M     ( COUNT_W )
  ) dut (
    .data_in  ( data_i ),
    .data_out ( count_o )
  );
""" + _footer()


GENERATORS = {
    "pulp_cc_rr": gen_pulp_cc_rr,
    "basejump_bsg_rr": gen_basejump_bsg_rr,
    "opentitan_prim_tree": gen_opentitan_prim_tree,
    "pulp_cc_fifo": gen_pulp_cc_fifo,
    "basejump_bsg_fifo": gen_basejump_bsg_fifo,
    "opentitan_prim_fifo": gen_opentitan_prim_fifo,
    "pulp_cc_popcount": gen_pulp_cc_popcount,
    "basejump_bsg_popcount": gen_basejump_bsg_popcount,
    "vortex_vx_popcount": gen_vortex_vx_popcount,
}

def generate_adapter(adapter: str, config) -> str:
    if adapter not in GENERATORS:
        raise KeyError(f"Unknown design-class adapter: {adapter}")
    # Backward compatibility: DF-09 historically passed N as an int.
    if isinstance(config, dict):
        if adapter in {"pulp_cc_rr", "basejump_bsg_rr", "opentitan_prim_tree"}:
            return GENERATORS[adapter](int(config["n"]))
        return GENERATORS[adapter](config)
    return GENERATORS[adapter](int(config))
