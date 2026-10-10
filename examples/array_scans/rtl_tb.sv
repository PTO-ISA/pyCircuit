module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic [23:0] data = 0;
  logic take = 0;
  wire [65:0] result;
  wire ready = result[65];
  wire out_valid = result[64];
  wire [63:0] out_data = result[63:0];

  bit first_valid = 0, second_valid = 0, last_clock = 0;
  logic [23:0] first_data = 0;
  logic [63:0] second_data = 0;
  logic [63:0] history [0:2047];
  integer head = 0, tail = 0, sampled = 0, accepted = 0, retired = 0;
  integer dropped = 0, outstanding = 0, stalled = 0, replacements = 0;
  integer reset_full = 0, reset_partial = 0, held_high = 0, peak = 0;

  pyc_root dut(.*);

  // Full ordered-prefix oracle. Every accumulator snapshot is retained,
  // including the unchanged second field of all three Pair snapshots.
  function automatic logic [63:0] scanned(input logic [23:0] request);
    logic [7:0] values [0:2];
    logic [7:0] subtract_prefix [0:2], add_prefix [0:2];
    logic [7:0] reset_prefix [0:2];
    logic [7:0] pair_first [0:2], pair_second [0:2];
    logic [7:0] subtract_accumulator, add_accumulator, reset_accumulator;
    logic [7:0] pair_first_accumulator, pair_second_accumulator;
    integer index;
    begin
      values[0] = request[23:16];
      values[1] = request[15:8];
      values[2] = request[7:0];
      subtract_accumulator = 0;
      add_accumulator = 10;
      reset_accumulator = 5;
      pair_first_accumulator = values[0];
      pair_second_accumulator = values[0];
      for (index = 0; index < 3; index = index + 1) begin
        subtract_accumulator = subtract_accumulator - values[index];
        add_accumulator = add_accumulator + values[index];
        reset_accumulator = values[index] == 0
                                ? 0
                                : reset_accumulator + values[index];
        pair_first_accumulator = values[index] == 0
                                     ? 0
                                     : pair_first_accumulator + values[index];
        subtract_prefix[index] = subtract_accumulator;
        add_prefix[index] = add_accumulator;
        reset_prefix[index] = reset_accumulator;
        pair_first[index] = pair_first_accumulator;
        pair_second[index] = pair_second_accumulator;
      end
      scanned = {subtract_prefix[0], subtract_prefix[1], subtract_prefix[2],
                 add_prefix[2], reset_prefix[1], reset_prefix[2],
                 pair_first[0], pair_first[2]};
    end
  endfunction

  function automatic logic [7:0] boundary(input integer index);
    case (index)
      0: boundary = 0;
      1: boundary = 1;
      2: boundary = 2;
      3: boundary = 127;
      4: boundary = 128;
      5: boundary = 254;
      6: boundary = 255;
      default: boundary = 0;
    endcase
  endfunction

  task row(input bit clock_level, reset_level, valid_level,
           input logic [23:0] request, input bit take_level);
    bit pop_second, room, pop_first, input_room, push_first, push_second;
    bit was_full;
    logic [23:0] old_first;
    pop_second = second_valid && take_level;
    room = !second_valid || pop_second;
    pop_first = first_valid && room;
    input_room = !first_valid || pop_first;
    push_first = valid_level && input_room;
    push_second = first_valid && room;
    pyc_7079635f727374 = reset_level;
    valid = valid_level;
    data = request;
    take = take_level;
    #1;
    if (result !== {input_room, second_valid,
                    second_valid ? second_data : 64'b0})
      $fatal(1, "array_scans old-Q mismatch at row %0d", sampled);
    if (out_data[63:56] !== (second_valid ? second_data[63:56] : 8'b0) ||
        out_data[55:48] !== (second_valid ? second_data[55:48] : 8'b0) ||
        out_data[47:40] !== (second_valid ? second_data[47:40] : 8'b0) ||
        out_data[39:32] !== (second_valid ? second_data[39:32] : 8'b0) ||
        out_data[31:24] !== (second_valid ? second_data[31:24] : 8'b0) ||
        out_data[23:16] !== (second_valid ? second_data[23:16] : 8'b0) ||
        out_data[15:8] !== (second_valid ? second_data[15:8] : 8'b0) ||
        out_data[7:0] !== (second_valid ? second_data[7:0] : 8'b0))
      $fatal(1, "array_scans field layout mismatch at row %0d", sampled);
    $display("WORK %0d %0d %0d %0d", sampled, input_room, second_valid,
             second_valid ? second_data : 64'b0);

    held_high = held_high + (clock_level && last_clock);
    if (clock_level && !last_clock) begin
      if (reset_level) begin
        reset_full = reset_full + (outstanding == 2);
        reset_partial = reset_partial + (outstanding == 1);
        dropped = dropped + outstanding;
        outstanding = 0;
        head = 0;
        tail = 0;
        first_valid = 0;
        second_valid = 0;
        first_data = 0;
        second_data = 0;
      end else begin
        stalled = stalled + (valid_level && !input_room);
        was_full = outstanding == 2;
        if (out_valid && take_level) begin
          if (outstanding == 0 || out_data !== history[head])
            $fatal(1, "array_scans lost/reordered output");
          head = head + 1;
          retired = retired + 1;
          outstanding = outstanding - 1;
        end
        if (ready && valid_level) begin
          history[tail] = scanned(request);
          tail = tail + 1;
          accepted = accepted + 1;
          outstanding = outstanding + 1;
          replacements = replacements + was_full;
        end
        old_first = first_data;
        if (push_second) begin
          second_valid = 1;
          second_data = scanned(old_first);
        end else if (pop_second) begin
          second_valid = 0;
          second_data = 0;
        end
        if (push_first) begin
          first_valid = 1;
          first_data = request;
        end else if (pop_first) begin
          first_valid = 0;
          first_data = 0;
        end
      end
      if (outstanding > peak)
        peak = outstanding;
      if (outstanding < 0 || outstanding > 2)
        $fatal(1, "array_scans logical capacity");
    end
    last_clock = clock_level;
    sampled = sampled + 1;
    pyc_7079635f636c6b = clock_level;
    #1;
  endtask

  task edge_row(input logic [23:0] request, input bit valid_level = 1,
                input bit take_level = 1, input bit reset_level = 0);
    row(1, reset_level, valid_level, request, take_level);
    row(0, reset_level, valid_level, request, take_level);
  endtask

  logic [23:0] historical [0:3];
  logic [63:0] historical_expected [0:3];
  integer index, first_index, second_index, third_index, cycle;
  initial begin
    historical[0] = {8'd0, 8'd0, 8'd0};
    historical[1] = {8'd1, 8'd2, 8'd3};
    historical[2] = {8'd255, 8'd1, 8'd2};
    historical[3] = {8'd7, 8'd9, 8'd11};
    historical_expected[0] = 64'd42949672960;
    historical_expected[1] = 64'd18446174595540779527;
    historical_expected[2] = 64'd72336921615400449;
    historical_expected[3] = 64'd18010146857285586466;
    for (index = 0; index < 4; index = index + 1)
      if (scanned(historical[index]) !== historical_expected[index])
        $fatal(1, "array_scans historical golden %0d mismatch", index);

    #1;
    pyc_7079635f636c6b = 1;
    #1;
    pyc_7079635f636c6b = 0;
    pyc_7079635f727374 = 0;
    #1;
    row(0, 1, 0, 0, 0);
    edge_row(0, 0, 0, 1);
    for (index = 0; index < 4; index = index + 1) begin
      edge_row(historical[index]);
      edge_row(0, 0);
      edge_row(0, 0);
    end

    edge_row({8'd31, 8'd47, 8'd63}, 1, 0);
    edge_row({8'd32, 8'd48, 8'd64}, 1, 0);
    edge_row({8'd33, 8'd49, 8'd65}, 1, 0);
    row(1, 0, 1, {8'd34, 8'd50, 8'd66}, 0);
    row(1, 0, 1, {8'd35, 8'd51, 8'd67}, 1);
    row(1, 1, 0, 0, 1);
    row(0, 0, 0, 0, 1);
    edge_row(0, 0, 1, 1);

    edge_row({8'd36, 8'd52, 8'd68});
    edge_row(0, 0, 1, 1);

    edge_row({8'd37, 8'd53, 8'd69});
    edge_row({8'd38, 8'd54, 8'd70}, 1, 0);
    edge_row({8'd39, 8'd55, 8'd71});
    edge_row(0, 0);
    edge_row(0, 0);

    for (first_index = 0; first_index < 7; first_index = first_index + 1)
      for (second_index = 0; second_index < 7; second_index = second_index + 1)
        for (third_index = 0; third_index < 7; third_index = third_index + 1)
          edge_row({boundary(first_index), boundary(second_index),
                    boundary(third_index)});
    edge_row({8'd0, 8'd17, 8'd33});
    edge_row({8'd17, 8'd0, 8'd33});
    edge_row({8'd17, 8'd33, 8'd0});
    edge_row({8'd0, 8'd255, 8'd1});
    edge_row({8'd255, 8'd0, 8'd1});
    edge_row({8'd255, 8'd1, 8'd0});
    edge_row({8'd3, 8'd17, 8'd241});
    edge_row({8'd241, 8'd17, 8'd3});
    edge_row({8'd1, 8'd128, 8'd255});
    edge_row({8'd255, 8'd128, 8'd1});
    edge_row({8'd254, 8'd255, 8'd1});
    edge_row({8'd255, 8'd255, 8'd255});
    edge_row({8'd127, 8'd128, 8'd129});
    edge_row({8'd128, 8'd127, 8'd126});
    for (index = 0; index < 128; index = index + 1)
      edge_row({8'(index * 73 + 19), 8'(index * 151 + 7),
                8'(index * 199 + 251)});
    for (cycle = 0; cycle < 4; cycle = cycle + 1)
      edge_row(0, 0);

    if (sampled >= 1100 || outstanding != 0 || accepted < 495 ||
        accepted != retired + dropped || peak != 2 || stalled < 2 ||
        replacements < 480 || reset_full < 1 || reset_partial < 1 ||
        held_high < 2)
      $fatal(1, "array_scans finite coverage failed");
    $display("HISTORY %0d %0d %0d %0d %0d %0d", accepted, retired, dropped,
             outstanding, peak, replacements);
    $finish;
  end
endmodule
