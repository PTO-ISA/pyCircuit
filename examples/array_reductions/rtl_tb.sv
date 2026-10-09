module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic [23:0] data = 0;
  logic take = 0;
  wire [48:0] result;
  wire ready = result[48];
  wire out_valid = result[47];
  wire [46:0] out_data = result[46:0];

  logic first_valid = 0;
  logic [23:0] first_data = 0;
  logic second_valid = 0;
  logic [46:0] second_data = 0;
  bit last_clock = 0;
  integer row_index = 0, stalled = 0;
  integer accepted = 0, retired = 0, dropped = 0, outstanding = 0;
  integer peak = 0, replacements = 0, reset_full = 0;

  pyc_root dut(.*);

  function automatic logic [46:0] reduced(input logic [23:0] token);
    logic [7:0] first, second, third, minimum, maximum;
    logic first_flag, second_flag, third_flag, selected;
    logic [1:0] count, first_index, best_index, range_best_index;
    begin
      first = token[23:16];
      second = token[15:8];
      third = token[7:0];
      first_flag = first != 0;
      second_flag = second != 0;
      third_flag = third != 0;
      count = first_flag + second_flag + third_flag;
      selected = 0;
      first_index = 0;
      best_index = 0;
      if (first_flag) begin
        selected = 1;
        first_index = 0;
        best_index = 0;
      end
      if (second_flag) begin
        if (!selected) begin
          selected = 1;
          first_index = 1;
          best_index = 1;
        end else if (second < first) begin
          best_index = 1;
        end
      end
      if (third_flag) begin
        if (!selected) begin
          selected = 1;
          first_index = 2;
          best_index = 2;
        end else if ((best_index == 0 && third < first) ||
                     (best_index == 1 && third < second)) begin
          best_index = 2;
        end
      end
      range_best_index = 0;
      if ((second % 5) < (first % 5)) range_best_index = 1;
      if ((third % 5) <
          (range_best_index == 0 ? (first % 5) : (second % 5)))
        range_best_index = 2;
      minimum = first;
      if (second < minimum) minimum = second;
      if (third < minimum) minimum = third;
      maximum = first;
      if (second > maximum) maximum = second;
      if (third > maximum) maximum = third;
      reduced = {count == 3, count != 0, count, count,
                 first + second + third, first * second * third,
                 minimum, maximum, first_flag ^ second_flag ^ third_flag,
                 first_index, selected, best_index, selected, range_best_index};
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
                    second_valid ? second_data : 47'b0})
      $fatal(1, "array_reductions two-queue oracle failed at row %0d", row_index);
    $display("WORK %0d %0d %0d %047b", row_index, expected_ready,
             second_valid, second_valid ? second_data : 47'b0);
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
          second_data = reduced(old_first);
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
        $fatal(1, "array_reductions outstanding token bound failed");
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

  logic [23:0] values [0:73];
  logic [31:0] random_state;
  integer vector_index;
  initial begin
    values[0] = {8'd0, 8'd0, 8'd0};
    values[1] = {8'd0, 8'd0, 8'd5};
    values[2] = {8'd7, 8'd7, 8'd9};
    values[3] = {8'd3, 8'd1, 8'd2};
    values[4] = {8'd255, 8'd2, 8'd3};
    values[5] = {8'd0, 8'd5, 8'd0};
    values[6] = {8'd9, 8'd7, 8'd7};
    values[7] = {8'd6, 8'd1, 8'd11};
    values[8] = {8'd255, 8'd255, 8'd2};
    values[9] = {8'd4, 8'd5, 8'd6};
    random_state = 32'h83a7d529;
    for (vector_index = 10; vector_index < 74; vector_index = vector_index + 1) begin
      random_state = random_state * 32'd1664525 + 32'd1013904223;
      values[vector_index][23:16] = random_state[31:24];
      random_state = random_state * 32'd1664525 + 32'd1013904223;
      values[vector_index][15:8] = random_state[31:24];
      random_state = random_state * 32'd1664525 + 32'd1013904223;
      values[vector_index][7:0] = random_state[31:24];
    end
    if (reduced(values[0]) !== 47'd0 ||
        reduced(values[1]) !== 47'd46222438042548 ||
        reduced(values[2]) !== 47'd138742242087716 ||
        reduced(values[3]) !== 47'd138590206166829 ||
        reduced(values[4]) !== 47'd138581213839148)
      $fatal(1, "array_reductions historical golden self-check failed");

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
    for (vector_index = 0; vector_index < 74; vector_index = vector_index + 1)
      capture_edge(values[vector_index]);
    capture_edge(0, 0, 1);
    capture_edge(0, 0, 1);
    capture_edge(values[0]);
    capture_edge(values[1]);
    capture_edge(values[2], 1, 1, 1);
    if (row_index != 187 || stalled < 3 || peak != 2 || reset_full != 2 ||
        replacements <= 20 || outstanding != 0 ||
        accepted != retired + dropped)
      $fatal(1, "array_reductions finite history coverage failed");
    $display("HISTORY_RTL %0d %0d %0d %0d %0d %0d", accepted, retired,
             dropped, outstanding, peak, replacements);
    $finish;
  end
endmodule
