
from __future__ import annotations
from typing import Dict


def generate_stateful_tb(config: Dict, seed: int) -> str:
    num_in = int(config["num_in"])
    data_width = int(config.get("data_width", 16))
    mode = str(config["mode"])
    cycles = int(config.get("cycles", 64))
    if num_in < 2:
        raise ValueError("stateful arbiter verification requires NumIn >= 2")
    idx_w = max(1, (num_in - 1).bit_length())

    fair = "1'b0" if mode == "unfair_internal" else "1'b1"
    lockin = "1'b1" if mode == "lockin" else "1'b0"
    axi = "1'b1" if mode == "axi_vld_rdy" else "1'b0"

    return f"""// Auto-generated pyCircuit Stateful Correctness Harness v0.4.1
`timescale 1ns/1ps

module tb;
  localparam int unsigned NUM_IN = {num_in};
  localparam int unsigned DATA_W = {data_width};
  localparam int unsigned IDX_W = {idx_w};
  localparam int unsigned TEST_CYCLES = {cycles};

  logic clk_i = 1'b0;
  logic rst_ni = 1'b0;
  logic clr_i = 1'b0;
  logic [IDX_W-1:0] rr_i = '0; // ignored because ExtPrio=0
  logic [NUM_IN-1:0] req_i = '0;
  logic [NUM_IN-1:0] gnt_o;
  logic [NUM_IN-1:0][DATA_W-1:0] data_i;
  logic req_o;
  logic gnt_i = 1'b0;
  logic [DATA_W-1:0] data_o;
  logic [IDX_W-1:0] idx_o;

  int unsigned rng_state = 32'h{seed & 0xffffffff:08x};
  int unsigned test_count = 0;
  int unsigned error_count = 0;
  int unsigned model_rr = 0;
  bit model_lock = 0;
  logic [NUM_IN-1:0] model_req_q = '0;

  always #5 clk_i = ~clk_i;

  cc_rr_arb_tree #(
    .NumIn     ( NUM_IN ),
    .DataWidth ( DATA_W ),
    .ExtPrio   ( 1'b0 ),
    .AxiVldRdy ( {axi} ),
    .LockIn    ( {lockin} ),
    .FairArb   ( {fair} )
  ) dut (
    .clk_i, .rst_ni, .clr_i, .rr_i,
    .req_i, .gnt_o, .data_i, .req_o, .gnt_i, .data_o, .idx_o
  );

  function automatic int unsigned next_rand();
    rng_state = rng_state ^ (rng_state << 13);
    rng_state = rng_state ^ (rng_state >> 17);
    rng_state = rng_state ^ (rng_state << 5);
    return rng_state;
  endfunction

  // External-priority v0.4 regression established this selection abstraction:
  // among active indices, choose the minimum (index XOR priority).
  function automatic int winner_for(
    input logic [NUM_IN-1:0] req,
    input int unsigned prio
  );
    int win;
    int unsigned best;
    bit found;
    win = 0; best = '1; found = 0;
    for (int i = 0; i < NUM_IN; i++) begin
      if (req[i]) begin
        int unsigned metric;
        metric = i ^ prio;
        if (!found || metric < best) begin
          found = 1;
          best = metric;
          win = i;
        end
      end
    end
    return win;
  endfunction

  function automatic int unsigned fair_next(
    input logic [NUM_IN-1:0] req,
    input int unsigned rr
  );
    // Mirrors the documented state semantics, not the arbitration tree:
    // smallest active index > rr, otherwise smallest active index <= rr.
    for (int i = rr + 1; i < NUM_IN; i++)
      if (req[i]) return i;
    for (int i = 0; i <= rr && i < NUM_IN; i++)
      if (req[i]) return i;
    return rr;
  endfunction

  task automatic init_data();
    for (int i = 0; i < NUM_IN; i++)
      data_i[i] = DATA_W'((i + 1) * 16'h31);
  endtask

  task automatic check_outputs(
    input logic [NUM_IN-1:0] effective_req,
    input int unsigned expected_rr,
    input bit expected_gnt_i
  );
    int win;
    logic [NUM_IN-1:0] expected_gnt;
    expected_gnt = '0;
    #1;
    test_count++;

    if (effective_req == '0) begin
      if (req_o !== 1'b0) begin
        error_count++;
        $display("FAIL req_o expected 0 got %0b", req_o);
      end
      // In AXI mode gnt_o is intentionally not asserted as a functional
      // requirement when req_o=0; only req_o validity is checked here.
    end else begin
      win = winner_for(effective_req, expected_rr);
      if (req_o !== 1'b1) begin
        error_count++;
        $display("FAIL req_o expected 1");
      end
      if (idx_o !== IDX_W'(win)) begin
        error_count++;
        $display("FAIL idx got=%0d exp=%0d rr=%0d req=%b",
                 idx_o, win, expected_rr, effective_req);
      end
      if (data_o !== data_i[win]) begin
        error_count++;
        $display("FAIL data got=%h exp=%h", data_o, data_i[win]);
      end

      if (!{axi}) begin
        if (expected_gnt_i) expected_gnt[win] = 1'b1;
        if (gnt_o !== expected_gnt) begin
          error_count++;
          $display("FAIL gnt got=%b exp=%b", gnt_o, expected_gnt);
        end
      end else begin
        // AXI valid/ready mode: selected input sees ready independent of req.
        // When req_o=1, the selected winner must receive gnt_i.
        if (expected_gnt_i && !gnt_o[win]) begin
          error_count++;
          $display("FAIL AXI selected ready not propagated");
        end
      end
    end
  endtask

  task automatic apply_async_reset();
    rst_ni = 1'b0;
    clr_i = 1'b0;
    req_i = '0;
    gnt_i = 1'b0;
    #2;
    @(negedge clk_i);
    rst_ni = 1'b1;
    model_rr = 0;
    model_lock = 0;
    model_req_q = '0;
  endtask

  task automatic step_model_and_dut(
    input logic [NUM_IN-1:0] next_req,
    input bit next_gnt
  );
    logic [NUM_IN-1:0] effective_req;
    int unsigned rr_before;
    bit transfer;
    bit next_lock;
    logic [NUM_IN-1:0] next_req_q;

    req_i = next_req;
    gnt_i = next_gnt;

    // LockIn uses registered request snapshot only after a blocked cycle.
    effective_req = model_lock ? model_req_q : next_req;
    rr_before = model_rr;

    check_outputs(effective_req, rr_before, next_gnt);

    transfer = next_gnt && (effective_req != '0);

    // Compute registered next state as sampled at the next positive edge.
    if ("{mode}" == "unfair_internal") begin
      if (transfer)
        model_rr = (model_rr == NUM_IN-1) ? 0 : model_rr + 1;
    end else begin
      if (transfer)
        model_rr = fair_next(effective_req, model_rr);
    end

    if ("{mode}" == "lockin") begin
      next_lock = (effective_req != '0) && !next_gnt;
      next_req_q = effective_req;
      model_lock = next_lock;
      model_req_q = next_req_q;
    end

    @(posedge clk_i);
    #1;
  endtask

  task automatic test_reset_clear();
    logic [NUM_IN-1:0] all_req;
    all_req = '1;
    apply_async_reset();

    // Priority reset to zero: with all requests active winner must be zero.
    req_i = all_req; gnt_i = 1'b1;
    check_outputs(all_req, 0, 1'b1);
    @(posedge clk_i); #1;
    model_rr = fair_next(all_req, 0);

    // Advance once more, then assert synchronous clear.
    req_i = all_req; gnt_i = 1'b1;
    check_outputs(all_req, model_rr, 1'b1);
    @(negedge clk_i);
    clr_i = 1'b1;
    @(posedge clk_i); #1;
    clr_i = 1'b0;
    model_rr = 0;

    req_i = all_req; gnt_i = 1'b0;
    check_outputs(all_req, 0, 1'b0);
  endtask

  task automatic test_backpressure_hold();
    logic [NUM_IN-1:0] req;
    apply_async_reset();
    req = '1;

    // With gnt_i=0 there is no transfer, therefore internal rr state must hold.
    for (int c = 0; c < TEST_CYCLES/2; c++)
      step_model_and_dut(req, 1'b0);

    // Once transfers begin, state may advance.
    for (int c = 0; c < TEST_CYCLES/2; c++)
      step_model_and_dut(req, 1'b1);
  endtask

  task automatic test_lockin();
    logic [NUM_IN-1:0] req_a, req_b;
    apply_async_reset();

    req_a = '0;
    req_a[0] = 1'b1;
    if (NUM_IN > 2) req_a[2] = 1'b1;

    // First blocked request chooses and locks a winner.
    step_model_and_dut(req_a, 1'b0);

    // Add more requests without withdrawing old ones; decision must remain based
    // on the captured request vector while blocked.
    req_b = '1;
    for (int c = 0; c < TEST_CYCLES/2; c++)
      step_model_and_dut(req_b, 1'b0);

    // Grant releases the lock and permits a new decision afterward.
    step_model_and_dut(req_b, 1'b1);
    for (int c = 0; c < TEST_CYCLES/2; c++)
      step_model_and_dut(req_b, 1'b1);
  endtask

  task automatic test_random_internal();
    logic [NUM_IN-1:0] req;
    apply_async_reset();

    // Directed full-request rotation first.
    req = '1;
    for (int c = 0; c < NUM_IN * 2; c++)
      step_model_and_dut(req, 1'b1);

    // Stateful randomized sequence.
    for (int c = 0; c < TEST_CYCLES; c++) begin
      for (int i = 0; i < NUM_IN; i++)
        req[i] = next_rand()[0];
      step_model_and_dut(req, next_rand()[0]);
    end
  endtask

  task automatic test_axi();
    logic [NUM_IN-1:0] req;
    apply_async_reset();

    // AXI mode still uses request validity for req_o and state update.
    for (int c = 0; c < TEST_CYCLES; c++) begin
      for (int i = 0; i < NUM_IN; i++)
        req[i] = next_rand()[0];
      step_model_and_dut(req, next_rand()[0]);
    end
  endtask

  initial begin
    init_data();

    if ("{mode}" == "reset_clear")
      test_reset_clear();
    else if ("{mode}" == "backpressure_hold")
      test_backpressure_hold();
    else if ("{mode}" == "lockin")
      test_lockin();
    else if ("{mode}" == "axi_vld_rdy")
      test_axi();
    else
      test_random_internal();

    if (error_count != 0) begin
      $display("PYC_STATEFUL_RESULT FAIL module=cc_rr_arb_tree mode={mode} tests=%0d errors=%0d",
               test_count, error_count);
      $fatal(1);
    end
    $display("PYC_STATEFUL_RESULT PASS module=cc_rr_arb_tree mode={mode} tests=%0d errors=0",
             test_count);
    $finish;
  end
endmodule
"""
