module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic [4:0] data = 0;
  logic take = 0;
  wire [6:0] result;
  wire ready = result[6];
  wire out_valid = result[5];
  wire [4:0] out_data = result[4:0];

  logic first_valid = 0;
  logic [4:0] first_data = 0;
  logic second_valid = 0;
  logic [4:0] second_data = 0;
  bit last_clock = 0;
  integer row_index = 0, stalled = 0;
  integer accepted = 0, retired = 0, dropped = 0, outstanding = 0, peak = 0;

  pyc_root dut(.*);

  function automatic logic [4:0] decoded(input logic [4:0] instruction);
    return {instruction[4:1], ((instruction[4:1] & 4'b1001) == 4'b1000)};
  endfunction

  task row(input bit clock_level, reset_level, valid_level,
           input logic [4:0] token, input bit take_level);
    bit pop_second, ready_second, pop_first, expected_ready;
    bit push_first, push_second;
    logic [4:0] old_first;
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
                    second_valid ? second_data : 5'b0})
      $fatal(1, "masked_decode_pipeline two-queue oracle failed");
    $display("WORK %0d %0d %0d %05b", row_index, expected_ready,
             second_valid, second_valid ? second_data : 5'b0);
    stalled = stalled + (valid_level && !expected_ready);
    if (clock_level && !last_clock) begin
      if (reset_level) begin
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
          if (outstanding > peak) peak = outstanding;
        end
        old_first = first_data;
        if (push_second) begin
          second_valid = 1;
          second_data = decoded(old_first);
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
        $fatal(1, "masked_decode_pipeline outstanding token bound failed");
    end
    last_clock = clock_level;
    pyc_7079635f636c6b = clock_level;
    #1;
    row_index = row_index + 1;
  endtask

  task capture_edge(input logic [4:0] token, input bit valid_level = 1,
                    input bit take_level = 1, input bit reset_level = 0);
    row(1, reset_level, valid_level, token, take_level);
    row(0, reset_level, valid_level, token, take_level);
  endtask

`ifdef PYC_MASKED_DECODE_FOUR_STATE
  task raw_edge;
    pyc_7079635f636c6b = 1;
    #1;
    pyc_7079635f636c6b = 0;
    #1;
  endtask
  task four_case(input integer ordinal, input logic [4:0] token,
                 input logic [4:0] expected);
    pyc_7079635f727374 = 1;
    valid = 0;
    take = 0;
    raw_edge();
    pyc_7079635f727374 = 0;
    valid = 1;
    data = token;
    take = 1;
    raw_edge();
    valid = 0;
    data = 0;
    raw_edge();
    take = 0;
    #1;
    if (out_valid !== 1'b1 || out_data !== expected)
      $fatal(1, "masked_decode_pipeline full-DUT X/Z case %0d failed", ordinal);
    #1;
    if (out_valid !== 1'b1 || out_data !== expected)
      $fatal(1, "masked_decode_pipeline X/Z output did not hold");
    $display("FOUR %0d %05b", ordinal, out_data);
  endtask
`endif

  logic [4:0] values [0:4];
  initial begin
    values[0] = 5'd0;
    values[1] = 5'd1;
    values[2] = 5'd2;
    values[3] = 5'd3;
    values[4] = 5'd4;
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
    row(1, 0, 1, values[0], 1);
    row(0, 0, 1, values[1], 1);
    row(0, 0, 1, values[2], 1);
    capture_edge(values[4]);
    capture_edge(0, 0, 1);
    capture_edge(0, 0, 1);
    capture_edge(0, 1, 1, 1);
    for (integer instruction = 0; instruction < 32; instruction = instruction + 1)
      capture_edge(5'(instruction));
    capture_edge(0, 0, 1);
    capture_edge(0, 0, 1);
    capture_edge(values[0]);
    capture_edge(values[1]);
    capture_edge(values[2], 1, 1, 1);
    if (row_index != 97 || stalled < 3 || peak != 2 || outstanding != 0 ||
        accepted != retired + dropped + outstanding)
      $fatal(1, "masked_decode_pipeline finite history coverage failed");
    $display("HISTORY_RTL %0d %0d %0d %0d %0d", accepted, retired, dropped,
             outstanding, peak);
`ifdef PYC_MASKED_DECODE_FOUR_STATE
    four_case(0, 5'b1xz0z, 5'b1xz01);
    four_case(1, 5'bx0000, 5'bx000x);
    four_case(2, 5'b100z1, 5'b100zx);
    four_case(3, 5'b0xzzx, 5'b0xzz0);
`endif
    $finish;
  end
endmodule
