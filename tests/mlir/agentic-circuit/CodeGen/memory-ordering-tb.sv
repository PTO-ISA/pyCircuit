module tb_memory_ordering;
  logic clk = 0;
  logic rst = 1;
  logic in_valid = 0;
  logic [35:0] in_data = '0;
  logic out_ready = 0;
  wire out_valid;
  wire [23:0] out_data;
  wire in_ready;

  memory_ordering dut(.*);
  always #5 clk = ~clk;

  // Deterministic splitmix64. The draw schedule differs from the C++
  // differential harness, so the two engines do not share trial inputs.
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

  function automatic integer popcount4(input logic [3:0] mask);
    integer index;
    begin
      popcount4 = 0;
      for (index = 0; index < 4; index = index + 1)
        popcount4 = popcount4 + mask[index];
    end
  endfunction

  function automatic [35:0] pack_input(
      input logic [3:0] pending, input logic [3:0] alias_mask,
      input logic [3:0] disjoint, input logic [3:0] ready_mask,
      input logic [3:0] executed, input logic [3:0] identity,
      input logic [3:0] producer, input logic [3:0] consumer,
      input logic [3:0] killed);
    pack_input = {pending, alias_mask, disjoint, ready_mask, executed,
                  identity, producer, consumer, killed};
  endfunction

  function automatic [23:0] oracle(
      input logic [3:0] pending, input logic [3:0] alias_mask,
      input logic [3:0] disjoint, input logic [3:0] ready_mask,
      input logic [3:0] executed, input logic [3:0] identity,
      input logic [3:0] producer, input logic [3:0] consumer,
      input logic [3:0] killed);
    logic [3:0] qualified, stale, forward, replay, bypass, wait_mask;
    begin
      // A killed (flush-invalidated) outstanding load is stale regardless of
      // its response identity.
      stale = pending & (~identity | killed);
      qualified = pending & ~stale;
      forward = qualified & alias_mask & ready_mask & ~executed;
      replay = qualified & alias_mask & executed;
      bypass = qualified & disjoint & ~alias_mask;
      wait_mask = qualified & ~(forward | replay | bypass);
      oracle = {producer & consumer, wait_mask, bypass, forward, replay, stale};
    end
  endfunction

  // Check the DUT's own output: every qualified lane must have exactly one live
  // disposition, and an unqualified lane must be reported as stale only.
  task automatic check_conformance(input logic [35:0] value,
                                   input logic [23:0] actual);
    logic [3:0] qualified, live;
    integer index;
    begin
      qualified = value[35:32] & value[15:12] & ~value[3:0];
      live = actual[19:16] | actual[15:12] | actual[11:8] | actual[7:4];
      for (index = 0; index < 4; index = index + 1) begin
        if (qualified[index] && (!live[index] || actual[index]))
          $fatal(1, "lane %0d must have exactly one live disposition", index);
        if (!qualified[index] &&
            (live[index] || (actual[index] !== value[32 + index])))
          $fatal(1, "lane %0d must be consumed as stale only", index);
      end
    end
  endtask

  task automatic check_one(input logic [35:0] value);
    logic [23:0] expected;
    integer cycles;
    begin
      expected = oracle(value[35:32], value[31:28], value[27:24],
                        value[23:20], value[19:16], value[15:12],
                        value[11:8], value[7:4], value[3:0]);
      @(negedge clk);
      in_data = value;
      in_valid = 1;
      do @(posedge clk); while (!in_ready);
      @(negedge clk);
      in_valid = 0;
      out_ready = 0;
      cycles = 0;
      while (cycles < 128) begin
        @(negedge clk);
        if (out_valid) begin
          if (out_data !== expected)
            $fatal(1, "memory ordering mismatch got=%h expected=%h",
                   out_data, expected);
          check_conformance(value, out_data);
          out_ready = 1;
          @(posedge clk);
          @(negedge clk);
          out_ready = 0;
          cycles = 128;
        end
        cycles = cycles + 1;
      end
    end
  endtask

  integer trial, lane, store;
  integer wait_count, bypass_count, forward_count, replay_count, stale_count;
  logic [3:0] store_valid, store_resolved, store_ready;
  logic [2:0] store_address [0:1];
  logic [2:0] lane_address [0:3];
  logic [3:0] alias_mask, disjoint, ready_mask, executed, identity, pending;
  logic [3:0] producer, consumer, killed;
  logic [3:0] wait_mask, bypass_mask, forward_mask, replay_mask, stale_mask;
  logic [23:0] observed;
  logic aliasing, data_ready, all_resolved;
  logic [63:0] rnd;
  logic [35:0] value;

  initial begin
    repeat (3) @(posedge clk);
    @(negedge clk);
    rst = 0;
    wait_count = 0;
    bypass_count = 0;
    forward_count = 0;
    replay_count = 0;
    stale_count = 0;
    for (trial = 0; trial < 200; trial = trial + 1) begin
      for (store = 0; store < 2; store = store + 1) begin
        rnd = next_random();
        store_valid[store] = rnd[0];
        store_resolved[store] = rnd[1];
        store_ready[store] = rnd[2];
        store_address[store] = rnd[5:3];
      end
      pending = 0;
      executed = 0;
      identity = 0;
      for (lane = 0; lane < 4; lane = lane + 1) begin
        rnd = next_random();
        lane_address[lane] = rnd[2:0];
        pending[lane] = rnd[3];
        executed[lane] = rnd[4];
        identity[lane] = rnd[5];
      end
      // Real address comparison: alias needs a resolved older store at the
      // same address, and a disjoint proof needs every older store resolved
      // and at a different address.
      alias_mask = 0;
      disjoint = 0;
      ready_mask = 0;
      for (lane = 0; lane < 4; lane = lane + 1) begin
        aliasing = 1'b0;
        data_ready = 1'b0;
        all_resolved = 1'b1;
        for (store = 0; store < 2; store = store + 1) begin
          if (store_valid[store]) begin
            if (!store_resolved[store])
              all_resolved = 1'b0;
            else if (store_address[store] == lane_address[lane]) begin
              aliasing = 1'b1;
              data_ready = data_ready || store_ready[store];
            end
          end
        end
        if (aliasing)
          alias_mask[lane] = 1'b1;
        if (data_ready)
          ready_mask[lane] = 1'b1;
        if (all_resolved && !aliasing)
          disjoint[lane] = 1'b1;
      end
      rnd = next_random();
      producer = rnd[3:0];
      consumer = rnd[7:4];
      // Flush model: an event boundary invalidates outstanding loads whose
      // recovery epoch changed or whose slot is younger than the boundary.
      rnd = next_random();
      killed = rnd[3:0] & pending & rnd[7:4];
      value = pack_input(pending, alias_mask, disjoint, ready_mask, executed,
                         identity, producer, consumer, killed);
      observed = oracle(value[35:32], value[31:28], value[27:24],
                        value[23:20], value[19:16], value[15:12],
                        value[11:8], value[7:4], value[3:0]);
      wait_mask = observed[19:16];
      bypass_mask = observed[15:12];
      forward_mask = observed[11:8];
      replay_mask = observed[7:4];
      stale_mask = observed[3:0];
      wait_count = wait_count + popcount4(wait_mask);
      bypass_count = bypass_count + popcount4(bypass_mask);
      forward_count = forward_count + popcount4(forward_mask);
      replay_count = replay_count + popcount4(replay_mask);
      stale_count = stale_count + popcount4(stale_mask);
      check_one(value);
    end
    // Directed boundaries, matching the C++ differential harness.
    // Unknown address: neither alias nor disjoint is provable, so lane 0 waits.
    value = pack_input(4'b0001, 4'b0000, 4'b0000, 4'b0000, 4'b0000, 4'b0001,
                       4'b1111, 4'b1111, 4'b0000);
    check_one(value);
    // Resolved and disjoint: lane 0 bypasses.
    value = pack_input(4'b0001, 4'b0000, 4'b0001, 4'b0000, 4'b0000, 4'b0001,
                       4'b1111, 4'b1111, 4'b0000);
    check_one(value);
    // Alias with ready data on a not-yet-executed load: forward.
    value = pack_input(4'b0001, 4'b0001, 4'b0000, 4'b0001, 4'b0000, 4'b0001,
                       4'b1111, 4'b1111, 4'b0000);
    check_one(value);
    // Alias on an already executed load: late violation, replay.
    value = pack_input(4'b0001, 4'b0001, 4'b0000, 4'b0000, 4'b0001, 4'b0001,
                       4'b1111, 4'b1111, 4'b0000);
    check_one(value);
    // Identity mismatch: the response can only be dropped.
    value = pack_input(4'b0001, 4'b0001, 4'b0000, 4'b0001, 4'b0000, 4'b0000,
                       4'b1111, 4'b1111, 4'b0000);
    check_one(value);
    // Directed flush case: the outstanding lane is invalidated, so it must be
    // consumed as stale even though its response identity still matches.
    value = pack_input(4'b0001, 4'b0000, 4'b0000, 4'b0000, 4'b0000, 4'b0001,
                       4'b1111, 4'b1111, 4'b0001);
    check_one(value);
    stale_count = stale_count + 1;
    if (wait_count == 0 || bypass_count == 0 || forward_count == 0 ||
        replay_count == 0 || stale_count == 0)
      $fatal(1, "a disposition never occurred");
    $display("memory ordering RTL PASS 200 wait=%0d bypass=%0d forward=%0d replay=%0d stale=%0d",
             wait_count, bypass_count, forward_count, replay_count, stale_count);
    $finish;
  end
endmodule
