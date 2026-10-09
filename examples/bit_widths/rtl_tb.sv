module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic [75:0] data = 0;
  logic take = 0;
  wire [77:0] result;
  wire ready = result[77];
  wire out_valid = result[76];
  wire [75:0] out_data = result[75:0];

  logic first_valid = 0;
  logic [75:0] first_data = 0;
  logic second_valid = 0;
  logic [75:0] second_data = 0;
  bit last_clock = 0;
  integer row_index = 0;
  integer stalled = 0;
  integer valid_outputs = 0;

  pyc_root dut(.*);

  function automatic logic [75:0] tag(
      input logic [12:0] value,
      input logic [12:0] mask,
      input logic [12:0] rotated,
      input logic [36:0] sequence_value);
    return {value, mask, rotated, sequence_value};
  endfunction

  function automatic logic [75:0] transformed(input logic [75:0] input_data);
    logic [12:0] value, mask, rotated;
    logic [36:0] sequence_value;
    value = input_data[75:63];
    mask = input_data[62:50];
    sequence_value = input_data[36:0];
    rotated = (value << 1) | (value >> 12);
    return {(value & mask) ^ 13'd1, mask, rotated, sequence_value};
  endfunction

  task row(input bit clock_level, reset_level, valid_level,
           input logic [75:0] token, input bit take_level);
    bit pop_second, ready_second, pop_first, expected_ready;
    bit push_first, push_second;
    logic [75:0] old_first;
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
                    second_valid ? second_data : 76'b0})
      $fatal(1, "bit_widths two-queue old-Q/full-replacement oracle failed");
    $display("WORK %0d %0d %0d %076b", row_index, expected_ready,
             second_valid, second_valid ? second_data : 76'b0);
    stalled = stalled + (valid_level && !expected_ready);
    valid_outputs = valid_outputs + second_valid;
    if (clock_level && !last_clock) begin
      if (reset_level) begin
        first_valid = 0;
        first_data = 0;
        second_valid = 0;
        second_data = 0;
      end else begin
        old_first = first_data;
        if (push_second) begin
          second_valid = 1;
          second_data = transformed(old_first);
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
    end
    last_clock = clock_level;
    pyc_7079635f636c6b = clock_level;
    #1;
    row_index = row_index + 1;
  endtask

  task capture_edge(input logic [75:0] token, input bit valid_level = 1,
                    input bit take_level = 1, input bit reset_level = 0);
    row(1, reset_level, valid_level, token, take_level);
    row(0, reset_level, valid_level, token, take_level);
  endtask

`ifdef PYC_BIT_WIDTHS_FOUR_STATE
  task raw_edge;
    pyc_7079635f636c6b = 1;
    #1;
    pyc_7079635f636c6b = 0;
    #1;
  endtask

  task four_case(input integer ordinal, input logic [75:0] token,
                 input logic [75:0] expected);
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
      $fatal(1,
             "bit_widths full-DUT Icarus X/Z oracle failed case %0d actual=%076b expected=%076b valid=%b",
             ordinal, out_data, expected, out_valid);
    #1;
    if (out_valid !== 1'b1 || out_data !== expected)
      $fatal(1, "bit_widths X/Z output did not hold under backpressure");
    $display("FOUR %0d %076b", ordinal, out_data);
  endtask
`endif

  logic [75:0] values [0:4];
  initial begin
    values[0] = 0;
    values[1] = {13'h1fff, 13'h1fff, 13'h1fff, 37'h1fffffffff};
    values[2] = tag(13'h1000, 13'h1fff, 13'h1555,
                    37'h1180000001);
    values[3] = tag(13'h1555, 13'h0f0f, 13'h1fff,
                    37'h1555555555);
    values[4] = tag(13'h1fff, 13'h0000, 13'h0001,
                    37'h1000000000);

    // Match SystemRunner's host Reset before the compared Work rows.
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
    for (integer bit_index = 0; bit_index < 81; bit_index = bit_index + 1) begin
      if (bit_index < 5)
        capture_edge(values[bit_index]);
      else
        capture_edge(76'b1 << (bit_index - 5));
    end
    capture_edge(0, 0, 1);
    capture_edge(0, 0, 1);
    capture_edge(values[0]);
    capture_edge(values[1]);
    capture_edge(values[2], 1, 1, 1);
    if (row_index != 195 || stalled < 3 || valid_outputs < 81)
      $fatal(1, "bit_widths finite coverage counters failed");

`ifdef PYC_BIT_WIDTHS_FOUR_STATE
    four_case(
        0,
        76'bxz10xz10xz10x0000000000000zzzzzzzzzzzzzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzx,
        76'b00000000000010000000000000x10xx10xx10xxxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzx);
    four_case(
        1,
        76'b10xz10xz10xz11111111111111xxxxxxxxxxxxxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxz,
        76'b10xx10xx10xx011111111111110xx10xx10xx11zxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxzxz);
    four_case(
        2,
        76'b0000000000000xzxzxzxzxzxzxzxzxzxzxzxzxzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz,
        76'b0000000000001xzxzxzxzxzxzx0000000000000zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz);
    four_case(
        3,
        76'bz00000000000x11111111111111010101010101x000000000000000000000000000000z00000,
        76'bx00000000000x111111111111100000000000xxx000000000000000000000000000000z00000);
`endif
    $finish;
  end
endmodule
