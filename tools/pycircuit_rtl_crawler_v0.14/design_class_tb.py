from __future__ import annotations

def generate_rr_property_tb(n: int, accepted_rounds: int = 2) -> str:
    return f"""`timescale 1ns/1ps
module tb;
  localparam int N = {n};
  logic clk_i = 1'b0;
  logic rst_ni = 1'b0;
  logic [N-1:0] req_i = '0;
  logic accept_i = 1'b0;
  logic valid_o;
  logic [N-1:0] sel_o;

  int errors = 0;
  logic [N-1:0] seen;
  logic [N-1:0] held_sel;

  pyc_synth_top #(.N(N)) dut (
    .clk_i, .rst_ni, .req_i, .accept_i, .valid_o, .sel_o
  );

  always #5 clk_i = ~clk_i;

  task automatic drive_and_sample(
    input logic [N-1:0] req,
    input logic accept,
    output logic [N-1:0] sel
  );
    @(negedge clk_i);
    req_i = req;
    accept_i = accept;
    #1;
    sel = sel_o;

    if ((|req) !== valid_o) begin
      $display("ERROR valid mismatch req=%b valid=%b", req, valid_o);
      errors++;
    end

    if (!valid_o && sel_o !== '0) begin
      $display("ERROR nonzero selection without valid: %b", sel_o);
      errors++;
    end

    if (valid_o) begin
      if (!$onehot(sel_o)) begin
        $display("ERROR selection is not onehot: %b", sel_o);
        errors++;
      end
      if ((sel_o & req) !== sel_o) begin
        $display("ERROR selected requester is not active: req=%b sel=%b", req, sel_o);
        errors++;
      end
    end

    @(posedge clk_i);
    #1;
  endtask

  task automatic reset_dut();
    req_i = '0;
    accept_i = 1'b0;
    rst_ni = 1'b0;
    repeat (3) @(posedge clk_i);
    @(negedge clk_i);
    rst_ni = 1'b1;
    repeat (1) @(posedge clk_i);
  endtask

  logic [N-1:0] tmp_sel;

  initial begin
    reset_dut();

    // 1) No request.
    drive_and_sample('0, 1'b0, tmp_sel);

    // 2) Every single-request case must select that requester.
    for (int i = 0; i < N; i++) begin
      logic [N-1:0] one;
      one = '0;
      one[i] = 1'b1;
      drive_and_sample(one, 1'b1, tmp_sel);
      if (tmp_sel !== one) begin
        $display("ERROR singleton req=%b sel=%b", one, tmp_sel);
        errors++;
      end
    end

    // 3) Backpressure: stable requests must produce a stable selection.
    reset_dut();
    drive_and_sample('1, 1'b0, held_sel);
    repeat (3) begin
      drive_and_sample('1, 1'b0, tmp_sel);
      if (tmp_sel !== held_sel) begin
        $display("ERROR selection changed under stall: held=%b now=%b", held_sel, tmp_sel);
        errors++;
      end
    end
    drive_and_sample('1, 1'b1, tmp_sel);

    // 4) All-requester fairness.
    // Exact phase/direction is intentionally NOT required. In each block of N
    // accepted transfers, every requester must win exactly once.
    reset_dut();
    for (int round = 0; round < {accepted_rounds}; round++) begin
      seen = '0;
      for (int k = 0; k < N; k++) begin
        drive_and_sample('1, 1'b1, tmp_sel);
        if ((seen & tmp_sel) != '0) begin
          $display("ERROR duplicate winner before complete RR round: seen=%b sel=%b", seen, tmp_sel);
          errors++;
        end
        seen |= tmp_sel;
      end
      if (seen !== '1) begin
        $display("ERROR incomplete RR round: seen=%b", seen);
        errors++;
      end
    end

    // 5) Sparse pair fairness.
    if (N >= 2) begin
      logic [N-1:0] pair;
      pair = '0;
      pair[0] = 1'b1;
      pair[N-1] = 1'b1;
      reset_dut();
      seen = '0;
      repeat (2) begin
        drive_and_sample(pair, 1'b1, tmp_sel);
        seen |= tmp_sel;
      end
      if ((seen & pair) !== pair) begin
        $display("ERROR sparse-pair fairness failed pair=%b seen=%b", pair, seen);
        errors++;
      end
    end

    if (errors == 0) begin
      $display("PYC_DC_PASS DF-09 N=%0d", N);
      $finish;
    end else begin
      $display("PYC_DC_FAIL errors=%0d", errors);
      $fatal(1);
    end
  end
endmodule
"""


def generate_fifo_property_tb(width: int, capacity: int, random_steps: int = 160) -> str:
    return f"""`timescale 1ns/1ps
module tb;
  localparam int DATA_W = {width};
  localparam int DEPTH  = {capacity};

  logic clk_i = 1'b0;
  logic rst_ni = 1'b0;
  logic clr_i = 1'b0;

  logic in_valid_i = 1'b0;
  logic in_ready_o;
  logic [DATA_W-1:0] in_data_i = '0;

  logic out_valid_o;
  logic out_ready_i = 1'b0;
  logic [DATA_W-1:0] out_data_o;

  logic [DATA_W-1:0] ref_mem [0:DEPTH-1];
  int ref_head = 0;
  int ref_tail = 0;
  int ref_count = 0;
  int errors = 0;
  int vectors = 0;

  pyc_synth_top #(.DATA_W(DATA_W), .DEPTH(DEPTH)) dut (
    .clk_i, .rst_ni, .clr_i,
    .in_valid_i, .in_ready_o, .in_data_i,
    .out_valid_o, .out_ready_i, .out_data_o
  );

  always #5 clk_i = ~clk_i;

  task automatic model_clear();
    ref_head = 0;
    ref_tail = 0;
    ref_count = 0;
  endtask

  task automatic cycle(
    input logic iv,
    input logic [DATA_W-1:0] idata,
    input logic ordy
  );
    logic do_in, do_out;
    logic [DATA_W-1:0] expected_head;

    @(negedge clk_i);
    in_valid_i = iv;
    in_data_i = idata;
    out_ready_i = ordy;
    #1;

    vectors++;

    if (in_ready_o !== (ref_count < DEPTH)) begin
      $display("ERROR ready mismatch count=%0d ready=%b", ref_count, in_ready_o);
      errors++;
    end

    if (out_valid_o !== (ref_count > 0)) begin
      $display("ERROR valid mismatch count=%0d valid=%b", ref_count, out_valid_o);
      errors++;
    end

    if (ref_count > 0) begin
      expected_head = ref_mem[ref_head];
      if (out_data_o !== expected_head) begin
        $display("ERROR data mismatch count=%0d exp=%h got=%h",
                 ref_count, expected_head, out_data_o);
        errors++;
      end
    end

    do_in  = in_valid_i & in_ready_o;
    do_out = out_valid_o & out_ready_i;

    @(posedge clk_i);

    // Update model using transfers observed before the active edge.
    if (do_out) begin
      ref_head = (ref_head == DEPTH-1) ? 0 : ref_head + 1;
    end
    if (do_in) begin
      ref_mem[ref_tail] = idata;
      ref_tail = (ref_tail == DEPTH-1) ? 0 : ref_tail + 1;
    end

    case ({{do_in, do_out}})
      2'b10: ref_count = ref_count + 1;
      2'b01: ref_count = ref_count - 1;
      default: ref_count = ref_count;
    endcase

    #1;
  endtask

  task automatic reset_dut();
    clr_i = 1'b0;
    in_valid_i = 1'b0;
    out_ready_i = 1'b0;
    rst_ni = 1'b0;
    model_clear();
    repeat (3) @(posedge clk_i);
    @(negedge clk_i);
    rst_ni = 1'b1;
    // OpenTitan deliberately suppresses wready briefly after reset.
    repeat (3) @(posedge clk_i);
  endtask

  task automatic logical_clear();
    @(negedge clk_i);
    in_valid_i = 1'b0;
    out_ready_i = 1'b0;
    clr_i = 1'b1;
    @(posedge clk_i);
    model_clear();
    @(negedge clk_i);
    clr_i = 1'b0;
    repeat (1) @(posedge clk_i);
  endtask

  initial begin
    reset_dut();

    // Empty state.
    cycle(1'b0, '0, 1'b0);

    // Fill to capacity with deterministic values.
    for (int i = 0; i < DEPTH; i++) begin
      cycle(1'b1, DATA_W'(32'h1000 + i), 1'b0);
    end

    // Backpressure: head must remain stable.
    repeat (4) begin
      cycle(1'b0, '0, 1'b0);
    end

    // Drain and verify FIFO ordering.
    for (int i = 0; i < DEPTH; i++) begin
      cycle(1'b0, '0, 1'b1);
    end

    // Refill partially, then clear.
    for (int i = 0; i < ((DEPTH > 3) ? 3 : DEPTH); i++) begin
      cycle(1'b1, DATA_W'(32'h2000 + i), 1'b0);
    end
    logical_clear();
    cycle(1'b0, '0, 1'b0);

    // Mixed deterministic pseudo-random traffic.
    for (int step = 0; step < {random_steps}; step++) begin
      logic iv;
      logic ordy;
      logic [DATA_W-1:0] d;
      iv   = (((step * 7 + 3) % 5) != 0);
      ordy = (((step * 11 + 1) % 4) != 0);
      d = DATA_W'((step * 32'h9e3779b1) ^ 32'h5a5aa5a5);
      cycle(iv, d, ordy);
    end

    // Drain remaining model contents.
    while (ref_count > 0) begin
      cycle(1'b0, '0, 1'b1);
    end

    cycle(1'b0, '0, 1'b0);

    if (errors == 0) begin
      $display("PYC_DC_PASS FIFO-SYNC W=%0d D=%0d vectors=%0d",
               DATA_W, DEPTH, vectors);
      $finish;
    end else begin
      $display("PYC_DC_FAIL FIFO-SYNC W=%0d D=%0d errors=%0d vectors=%0d",
               DATA_W, DEPTH, errors, vectors);
      $fatal(1);
    end
  end
endmodule
"""


def generate_popcount_property_tb(width: int, random_tests: int = 256) -> str:
    import math
    count_w = max(1, math.ceil(math.log2(width + 1)))
    return f"""`timescale 1ns/1ps
module tb;
  localparam int WIDTH = {width};
  localparam int COUNT_W = {count_w};

  logic [WIDTH-1:0] data_i = '0;
  logic [COUNT_W-1:0] count_o;

  int errors = 0;
  int vectors = 0;
  longint unsigned state;

  pyc_synth_top #(
    .WIDTH(WIDTH),
    .COUNT_W(COUNT_W)
  ) dut (
    .data_i,
    .count_o
  );

  function automatic int unsigned golden_popcount(
    input logic [WIDTH-1:0] v
  );
    int unsigned c;
    c = 0;
    for (int i = 0; i < WIDTH; i++)
      c += v[i];
    return c;
  endfunction

  task automatic check(input logic [WIDTH-1:0] v);
    int unsigned expected;
    data_i = v;
    #1;
    expected = golden_popcount(v);
    vectors++;
    if (count_o !== COUNT_W'(expected)) begin
      $display(
        "ERROR popcount W=%0d data=%h expected=%0d got=%0d",
        WIDTH, v, expected, count_o
      );
      errors++;
    end
  endtask

  initial begin
    // Directed boundary cases.
    check('0);
    check('1);

    // Every bit position independently.
    for (int i = 0; i < WIDTH; i++) begin
      logic [WIDTH-1:0] one;
      one = '0;
      one[i] = 1'b1;
      check(one);
      check(~one);
    end

    // Alternating patterns.
    begin
      logic [WIDTH-1:0] a;
      logic [WIDTH-1:0] b;
      for (int i = 0; i < WIDTH; i++) begin
        a[i] = (i % 2) == 0;
        b[i] = (i % 2) != 0;
      end
      check(a);
      check(b);
    end

    // Deterministic xorshift64 sequence. Scaling profile is <= 64 bits,
    // so every candidate receives identical repeatable pseudo-random vectors.
    state = 64'h9e3779b97f4a7c15;
    for (int t = 0; t < {random_tests}; t++) begin
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      check(WIDTH'(state));
    end

    if (errors == 0) begin
      $display(
        "PYC_DC_PASS INT-11 WIDTH=%0d vectors=%0d",
        WIDTH, vectors
      );
      $finish;
    end else begin
      $display(
        "PYC_DC_FAIL INT-11 WIDTH=%0d errors=%0d vectors=%0d",
        WIDTH, errors, vectors
      );
      $fatal(1);
    end
  end
endmodule
"""
