// Executable acceptance testbench for the reduced MiniOOO. It replays the same
// bounded schedule as the C++ stress harness -- one killing recovery on the
// first cycle, then a dispatch and a versioned completion every cycle -- but
// draws its payloads from a different generator and drives the ready/valid
// handshake directly, so the RTL lowering has to accept the same stimulus
// stream on its own.
module tb_miniooo;
  logic clk = 0;
  logic rst = 1;
  logic in0_valid = 0;
  logic [83:0] in0_data = '0;
  logic in1_valid = 0;
  logic [16:0] in1_data = '0;
  logic in2_valid = 0;
  logic [16:0] in2_data = '0;
  wire in0_ready;
  wire in1_ready;
  wire in2_ready;

  miniooo dut(.*);
  always #5 clk = ~clk;

  localparam integer CYCLES = 48;
  localparam integer STALE_PERIOD = 8;
  localparam integer LIMIT = 64;

  // Deterministic splitmix64. The draw schedule differs from the C++ harness,
  // so the two engines do not share payload inputs.
  logic [63:0] rnd_state = 64'h243f6a8885a308d3;
  function automatic logic [63:0] next_random();
    logic [63:0] z;
    begin
      rnd_state = rnd_state + 64'h9e3779b97f4a7c15;
      z = rnd_state;
      z = (z ^ (z >> 30)) * 64'hbf58476d1ce4e5b9;
      z = (z ^ (z >> 27)) * 64'h94d049bb133111eb;
      next_random = z ^ (z >> 31);
    end
  endfunction

  function automatic [83:0] pack_dispatch(
      input logic [3:0] valid, input logic [3:0] free_entries,
      input logic [3:0] identity, input logic [3:0] alias_mask,
      input logic [3:0] disjoint, input logic [3:0] data_ready,
      input logic [3:0] executed, input logic [3:0] killed);
    begin
      pack_dispatch = {valid, 4'd15, 4'd15, 4'd15, free_entries, 4'd0, 4'd15,
                       3'd0, 3'd1, 3'd2, 3'd3,
                       4'd15, 4'd0, 4'd0, identity, 4'd15, 4'd15,
                       alias_mask, disjoint, data_ready, executed, killed};
    end
  endfunction

  function automatic [16:0] pack_completion(
      input logic [1:0] slot, input logic [1:0] generation,
      input logic [2:0] recovery_epoch, input logic [1:0] attempt,
      input logic [7:0] payload);
    begin
      pack_completion = {slot, generation, recovery_epoch, attempt, payload};
    end
  endfunction

  function automatic [16:0] pack_recovery(
      input logic [2:0] next_epoch, input logic [1:0] slot,
      input logic [1:0] generation, input logic [2:0] transaction_epoch);
    begin
      pack_recovery = {1'b1, next_epoch, 2'd0, 2'd0, slot, generation,
                       transaction_epoch, 2'd0};
    end
  endfunction

  integer cycle;
  integer attempt_index;
  integer dispatched;
  integer completed;
  integer recovered;
  integer guard;
  logic [63:0] payload;
  logic take0, take1, take2;
  logic all_taken;

  initial begin
    repeat (2) @(posedge clk);
    @(negedge clk);
    rst = 0;

    dispatched = 0;
    completed = 0;
    recovered = 0;

    for (cycle = 0; cycle < CYCLES; cycle = cycle + 1) begin
      attempt_index = cycle % 4;
      payload = next_random();
      in0_valid = 1;
      in0_data = pack_dispatch(4'd3, 4'd15,
                               (cycle % STALE_PERIOD) == STALE_PERIOD - 1 ? 4'd0 : 4'd3,
                               (cycle % 2) == 0 ? 4'd3 : 4'd0,
                               (cycle % 2) == 0 ? 4'd0 : 4'd3,
                               4'd3,
                               (cycle % 4) == 3 ? 4'd3 : 4'd0,
                               (cycle % 16) == 15 ? 4'd3 : 4'd0);
      if (cycle == 0) begin
        // First cycle: the killing recovery invalidates the version the very
        // first commit publishes.
        in1_valid = 0;
        in1_data = '0;
        in2_valid = 1;
        in2_data = pack_recovery(3'd1, 2'd0, 2'd0, 3'd0);
      end else begin
        in1_valid = 1;
        in1_data = pack_completion(2'd0, 2'd0, 3'd0, attempt_index[1:0],
                                   payload[7:0]);
        in2_valid = 0;
        in2_data = '0;
      end

      // `take*` marks a source as already settled when this cycle does not
      // drive it, so the handshake loop only waits on the driven sources and
      // the counters only credit a driven source.
      take0 = 0;
      if (cycle == 0) begin
        take1 = 1;
        take2 = 0;
      end else begin
        take1 = 0;
        take2 = 1;
      end
      guard = 0;
      all_taken = 0;
      while (guard < LIMIT && !all_taken) begin
        @(negedge clk);
        if (!take0 && in0_ready) take0 = 1;
        if (!take1 && in1_ready) take1 = 1;
        if (!take2 && in2_ready) take2 = 1;
        @(posedge clk);
        if (take0) in0_valid = 0;
        if (take1) in1_valid = 0;
        if (take2) in2_valid = 0;
        guard = guard + 1;
        if (take0 && take1 && take2) all_taken = 1;
      end

      if (!all_taken) begin
        $display("miniOOO rtl stress FAIL cycle=%0d stalled", cycle);
        $fatal(1, "miniOOO source did not accept within the limit");
      end
      dispatched = dispatched + 1;
      if (cycle == 0)
        recovered = recovered + 1;
      else
        completed = completed + 1;
    end

    if (dispatched != CYCLES || completed != CYCLES - 1 || recovered != 1) begin
      $display("miniOOO rtl stress FAIL dispatched=%0d completed=%0d recovered=%0d",
               dispatched, completed, recovered);
      $fatal(1, "miniOOO handshake counts diverged");
    end
    $display("miniOOO rtl stress PASS cycles=%0d dispatched=%0d completed=%0d recovered=%0d",
             CYCLES, dispatched, completed, recovered);
    $finish;
  end

  initial begin
    #100000;
    $display("miniOOO rtl stress FAIL timeout");
    $fatal(1, "miniOOO testbench timed out");
  end
endmodule
