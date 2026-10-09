module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic [23:0] data = 0;
  logic take = 0;
  wire [63:0] result;
  wire ready = result[63];
  wire out_valid = result[62];
  wire [61:0] out_data = result[61:0];

  logic first_valid = 0;
  logic [23:0] first_data = 0;
  logic second_valid = 0;
  logic [61:0] second_data = 0;
  bit last_clock = 0;
  integer row_index = 0, stalled = 0;
  integer accepted = 0, retired = 0, dropped = 0, outstanding = 0;
  integer peak = 0, replacements = 0, reset_full = 0;

  pyc_root dut(.*);

  function automatic logic [2:0] checked_index5(input logic [7:0] value);
    checked_index5 = value < 8'd5 ? value[2:0] : 3'd0;
  endfunction

  // Independent reconstruction of the original Tables. In particular, the
  // reversed nested row is transformed before selection, while replacement
  // copies each row and changes only lane zero.
  function automatic logic [61:0] combined(input logic [23:0] token);
    logic [7:0] values [0:2];
    logic [3:0] narrow [0:2];
    logic [7:0] nested [0:1][0:2];
    logic [7:0] mapped [0:2], helper_mapped [0:2];
    logic [7:0] tuple_next [0:2];
    logic record_valid [0:2];
    logic [7:0] nested_mapped [0:1][0:2];
    logic [7:0] updated_nested [0:1][0:2];
    logic [2:0] checked [0:2];
    integer lane, row;
    begin
      values[0] = token[23:16];
      values[1] = token[15:8];
      values[2] = token[7:0];
      nested[0][0] = values[0];
      nested[0][1] = values[1];
      nested[0][2] = values[2];
      nested[1][0] = values[2];
      nested[1][1] = values[1];
      nested[1][2] = values[0];
      for (lane = 0; lane < 3; lane = lane + 1) begin
        narrow[lane] = values[lane][3:0];
        mapped[lane] = values[lane] + 8'd1;
        helper_mapped[lane] = values[lane] + 8'd1;
        // The callback argument shadows the outer historical constant.
        tuple_next[lane] = values[lane] + 8'd1;
        record_valid[lane] = values[lane] != 0;
        checked[lane] = checked_index5(values[lane]);
      end
      for (row = 0; row < 2; row = row + 1) begin
        for (lane = 0; lane < 3; lane = lane + 1) begin
          nested_mapped[row][lane] = nested[row][lane] + 8'd1;
          updated_nested[row][lane] = nested[row][lane];
        end
        updated_nested[row][0] = 8'd1;
      end
      combined = {helper_mapped[0], mapped[2], values[1], narrow[1],
                  tuple_next[2], record_valid[1], nested_mapped[1][2],
                  updated_nested[1][0], checked[0], checked[1], checked[2]};
    end
  endfunction

  task row(input bit clock_level, reset_level, valid_level,
           input logic [23:0] token, input bit take_level);
    bit pop_second, ready_second, pop_first, expected_ready;
    bit push_first, push_second;
    logic [23:0] old_first;
    pop_second = second_valid && take_level;
    ready_second = !second_valid || pop_second;
    pop_first = first_valid && ready_second;
    expected_ready = !first_valid || pop_first;
    push_first = valid_level && expected_ready;
    push_second = first_valid && ready_second;
    pyc_7079635f727374 = reset_level;
    valid = valid_level;
    data = token;
    take = take_level;
    #1;
    if (result !== {expected_ready, second_valid,
                    second_valid ? second_data : 62'b0})
      $fatal(1, "array_combinators two-queue oracle failed at row %0d", row_index);
    $display("WORK %0d %0d %0d %062b", row_index, expected_ready,
             second_valid, second_valid ? second_data : 62'b0);
    stalled = stalled + (valid_level && !expected_ready);
    if (clock_level && !last_clock) begin
      if (reset_level) begin
        reset_full = reset_full + (outstanding == 2);
        dropped = dropped + outstanding;
        outstanding = 0;
        first_valid = 0;
        first_data = 0;
        second_valid = 0;
        second_data = 0;
      end else begin
        if (out_valid && take_level) begin
          retired = retired + 1;
          outstanding = outstanding - 1;
        end
        if (ready && valid_level) begin
          accepted = accepted + 1;
          outstanding = outstanding + 1;
        end
        replacements = replacements +
                       (out_valid && take_level && ready && valid_level &&
                        outstanding == 2);
        if (outstanding > peak) peak = outstanding;
        old_first = first_data;
        if (push_second) begin
          second_valid = 1;
          second_data = combined(old_first);
        end else if (pop_second) begin
          second_valid = 0;
          second_data = 0;
        end
        if (push_first) begin
          first_valid = 1;
          first_data = token;
        end else if (pop_first) begin
          first_valid = 0;
          first_data = 0;
        end
      end
      if (outstanding < 0 || outstanding > 2)
        $fatal(1, "array_combinators outstanding token bound failed");
    end
    last_clock = clock_level;
    pyc_7079635f636c6b = clock_level;
    #1;
    row_index = row_index + 1;
  endtask

  task capture_edge(input logic [23:0] token, input bit valid_level = 1,
                    input bit take_level = 1, input bit reset_level = 0);
    row(1, reset_level, valid_level, token, take_level);
    row(0, reset_level, valid_level, token, take_level);
  endtask

  logic [23:0] values [0:75];
  logic [7:0] boundaries [0:3];
  integer vector_index, first_index, second_index, third_index;
  initial begin
    values[0] = {8'd0, 8'd1, 8'd4};
    values[1] = {8'd3, 8'd5, 8'd7};
    values[2] = {8'd255, 8'd2, 8'd254};
    boundaries[0] = 0;
    boundaries[1] = 4;
    boundaries[2] = 5;
    boundaries[3] = 255;
    vector_index = 3;
    for (first_index = 0; first_index < 4; first_index = first_index + 1)
      for (second_index = 0; second_index < 4; second_index = second_index + 1)
        for (third_index = 0; third_index < 4; third_index = third_index + 1) begin
          values[vector_index] = {boundaries[first_index],
                                  boundaries[second_index],
                                  boundaries[third_index]};
          vector_index = vector_index + 1;
        end
    values[67] = {8'd7, 8'd8, 8'd15};
    values[68] = {8'd8, 8'd15, 8'd16};
    values[69] = {8'd15, 8'd16, 8'd254};
    values[70] = {8'd16, 8'd254, 8'd7};
    values[71] = {8'd254, 8'd7, 8'd8};
    values[72] = {8'd9, 8'd12, 8'd17};
    values[73] = {8'd12, 8'd17, 8'd252};
    values[74] = {8'd17, 8'd252, 8'd9};
    values[75] = {8'd252, 8'd9, 8'd12};
    if (vector_index != 67 ||
        combined(values[0]) !== 62'd18366534657376780 ||
        combined(values[1]) !== 62'd72622004851180224 ||
        combined(values[2]) !== 62'd17944631027171856)
      $fatal(1, "array_combinators historical golden self-check failed");

    #1;
    pyc_7079635f636c6b = 1;
    #1;
    pyc_7079635f636c6b = 0;
    pyc_7079635f727374 = 0;
    #1;
    row(0, 1, 0, 0, 0);
    capture_edge(0, 0, 0, 1);
    capture_edge(values[0]);
    capture_edge(values[1]);
    capture_edge(values[2]);
    capture_edge(values[3], 1, 0);
    row(1, 0, 1, values[4], 0);
    row(1, 0, 1, values[5], 1);
    row(0, 0, 1, values[6], 1);
    row(0, 0, 1, values[7], 1);
    capture_edge(values[4]);
    capture_edge(0, 0, 1);
    capture_edge(0, 0, 1);
    capture_edge(values[5]);
    capture_edge(values[6], 1, 0);
    capture_edge(values[7], 1, 0);
    capture_edge(values[8], 1, 1, 1);
    for (vector_index = 0; vector_index < 76; vector_index = vector_index + 1)
      capture_edge(values[vector_index]);
    capture_edge(0, 0, 1);
    capture_edge(0, 0, 1);
    capture_edge(values[0]);
    capture_edge(values[1]);
    capture_edge(values[2], 1, 1, 1);
    if (row_index != 191 || stalled < 3 || peak != 2 || reset_full != 2 ||
        replacements <= 20 || outstanding != 0 ||
        accepted != retired + dropped)
      $fatal(1, "array_combinators finite history coverage failed");
    $display("HISTORY_RTL %0d %0d %0d %0d %0d %0d", accepted, retired,
             dropped, outstanding, peak, replacements);
    $finish;
  end
endmodule
