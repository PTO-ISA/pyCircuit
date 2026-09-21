module tb_transaction_algebra;
  logic clk = 0;
  logic rst = 1;
  logic in_valid = 0;
  logic [67:0] in_data = '0;
  logic out_ready = 0;
  wire out_valid;
  wire [56:0] out_data;
  wire in_ready;

  transaction_algebra dut(.*);
  always #5 clk = ~clk;

  // splitmix64 keeps every sampled field independent. A plain odd-multiplier
  // LCG alternates bit zero on every draw, which pins an AND of consecutive
  // fields to zero and silently drops most of the algebra out of the fixture.
  logic [63:0] rnd_state = 64'h9e3779b97f4a7c15;
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
    integer lane;
    begin
      popcount4 = 0;
      for (lane = 0; lane < 4; lane = lane + 1)
        popcount4 = popcount4 + mask[lane];
    end
  endfunction

  // Reference model of `ac.multi_allocator` acceptance: a contiguous request
  // prefix limited by the candidate free capacity.
  function automatic logic [3:0] capacity_prefix(
      input logic [3:0] request, input logic [3:0] candidate_free);
    integer lane, taken, capacity;
    logic live, take;
    logic [3:0] accepted;
    begin
      accepted = 0;
      live = 1;
      taken = 0;
      capacity = popcount4(candidate_free);
      for (lane = 0; lane < 4; lane = lane + 1) begin
        take = live && request[lane] && (taken < capacity);
        accepted[lane] = take;
        if (take)
          taken = taken + 1;
        live = take;
      end
      capacity_prefix = accepted;
    end
  endfunction

  function automatic logic [3:0] lowest_free(
      input logic [3:0] candidate_free, input integer needed);
    integer slot, taken;
    logic [3:0] mask;
    begin
      mask = 0;
      taken = 0;
      for (slot = 0; slot < 4; slot = slot + 1)
        if (candidate_free[slot] && (taken < needed)) begin
          mask[slot] = 1;
          taken = taken + 1;
        end
      lowest_free = mask;
    end
  endfunction

  function automatic [67:0] pack_input(
      input logic [3:0] valid, input logic [3:0] r0,
      input logic [3:0] r1, input logic [3:0] r2,
      input logic [3:0] candidates, input logic [2:0] age0,
      input logic [2:0] age1, input logic [2:0] age2,
      input logic [2:0] age3, input logic [3:0] current,
      input logic [3:0] resolve, input logic [3:0] kill,
      input logic [3:0] add, input logic [3:0] identity,
      input logic [3:0] effects, input logic [3:0] terminal,
      input logic [3:0] free_mask, input logic [3:0] release_mask);
    pack_input = {valid, r0, r1, r2, candidates, age0, age1, age2, age3,
                  current, resolve, kill, add, identity, effects, terminal,
                  free_mask, release_mask};
  endfunction

  function automatic [56:0] oracle(input logic [67:0] input_value);
    logic [3:0] valid, r0, r1, r2, candidates;
    logic [2:0] ages [0:3];
    logic [3:0] current, resolve, kill, add, identity, effects, terminal;
    logic [3:0] free_mask, release_mask;
    logic [3:0] reserved, accepted, accepted_all, accepted_independent;
    logic [3:0] candidate_free_committed, candidate_free_reused;
    logic [3:0] allocation, allocator_accepted, commit_mask, next_free;
    logic [3:0] allocation_reuse, allocator_accepted_reuse, next_free_reuse;
    logic [3:0] winners, deps_next, completed;
    integer lane, pick, winner, accepted_count_committed, accepted_count_reused;
    logic prefix, take;
    begin
      {valid, r0, r1, r2, candidates, ages[0], ages[1], ages[2], ages[3],
       current, resolve, kill, add, identity, effects, terminal, free_mask,
       release_mask} = input_value;
      reserved = r0 & r1 & r2;
      accepted = 0;
      prefix = 1;
      for (lane = 0; lane < 4; lane = lane + 1) begin
        take = prefix && valid[lane] && reserved[lane];
        accepted[lane] = take;
        prefix = take;
      end
      accepted_all = ((valid & reserved) == valid) ? valid : 0;
      accepted_independent = valid & reserved;

      candidate_free_committed = free_mask;
      candidate_free_reused = free_mask | release_mask;
      allocator_accepted =
          capacity_prefix(accepted, candidate_free_committed);
      accepted_count_committed = popcount4(allocator_accepted);
      allocation = lowest_free(candidate_free_committed,
                               accepted_count_committed);
      next_free = (candidate_free_committed & ~allocation) | release_mask;
      commit_mask = allocator_accepted;

      allocator_accepted_reuse =
          capacity_prefix(accepted, candidate_free_reused);
      accepted_count_reused = popcount4(allocator_accepted_reuse);
      allocation_reuse = lowest_free(candidate_free_reused,
                                     accepted_count_reused);
      next_free_reuse = candidate_free_reused & ~allocation_reuse;

      winners = 0;
      for (pick = 0; pick < 2; pick = pick + 1) begin
        winner = 4;
        for (lane = 0; lane < 4; lane = lane + 1)
          if (candidates[lane] && !winners[lane] &&
              (winner == 4 || ages[lane] < ages[winner]))
            winner = lane;
        if (winner != 4)
          winners[winner] = 1;
      end
      deps_next = (current | add) & ~(resolve & identity) & ~(kill & identity);
      completed = commit_mask & effects & terminal;
      oracle = {reserved, accepted, accepted_all, accepted_independent,
                allocation, allocator_accepted, commit_mask, next_free,
                allocation_reuse, allocator_accepted_reuse, next_free_reuse,
                winners, deps_next, deps_next == 0, completed};
    end
  endfunction

  task automatic transact(input logic [67:0] value,
                           input logic [56:0] expected,
                           input integer stall_cycles);
    integer cycles;
    begin
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
        if (out_valid && stall_cycles > 0)
          stall_cycles = stall_cycles - 1;
        else if (out_valid) begin
          if (out_data !== expected)
            $fatal(1, "transaction mismatch got=%h expected=%h",
                   out_data, expected);
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

  integer trial, draw;
  logic [67:0] input_value;
  logic [63:0] draws [0:17];
  initial begin
    repeat (3) @(posedge clk);
    @(negedge clk);
    rst = 0;
    for (trial = 0; trial < 50; trial = trial + 1) begin
      for (draw = 0; draw < 18; draw = draw + 1)
        draws[draw] = next_random();
      input_value = pack_input(
          draws[0][3:0], draws[1][3:0], draws[2][3:0], draws[3][3:0],
          draws[4][3:0], draws[5][2:0], draws[6][2:0], draws[7][2:0],
          draws[8][2:0], draws[9][3:0], draws[10][3:0], draws[11][3:0],
          draws[12][3:0], draws[13][3:0], draws[14][3:0], draws[15][3:0],
          draws[16][3:0], draws[17][3:0]);
      transact(input_value, oracle(input_value), next_random() % 5);
    end
    $display("transaction algebra RTL PASS 50");
    $finish;
  end
endmodule
