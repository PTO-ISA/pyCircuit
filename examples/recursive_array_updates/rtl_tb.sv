module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic [15:0] data = 0;
  logic take = 0;
  wire [43:0] result;
  wire ready = result[43];
  wire out_valid = result[42];
  wire [41:0] out_data = result[41:0];

  bit first_valid = 0, second_valid = 0, last_clock = 0;
  logic [15:0] first_data = 0;
  logic [41:0] second_data = 0;
  logic [41:0] history [0:4095];
  integer head = 0, tail = 0, sampled = 0, accepted = 0, retired = 0;
  integer dropped = 0, outstanding = 0, stalled = 0, replacements = 0;
  integer reset_full = 0, reset_partial = 0, held_high = 0, peak = 0;

  pyc_root dut(.*);

  // Full independent model of the five immutable Table copies. Ordinal is
  // retained in the host structs even though the historical result omits it.
  function automatic logic [41:0] expected(input logic [15:0] request);
    logic [7:0] struct_tag [0:2], updated_struct_tag [0:2];
    logic struct_mode [0:2], updated_struct_mode [0:2];
    logic [1:0] struct_ordinal [0:2], updated_struct_ordinal [0:2];
    logic enums [0:4], updated_enums [0:4];
    logic [1:0] ranges [0:2], updated_ranges [0:2];
    logic [2:0] wide [0:64], updated_wide [0:64], chained_wide [0:64];
    logic [7:0] raw, replacement;
    integer struct_index, enum_index, wide_index, lane;
    begin
      raw = request[15:8];
      replacement = request[7:0];
      struct_index = raw % 3;
      enum_index = raw % 5;
      wide_index = raw % 65;
      for (lane = 0; lane < 3; lane = lane + 1) begin
        struct_tag[lane] = raw;
        struct_mode[lane] = 0;
        struct_ordinal[lane] = raw % 3;
        ranges[lane] = raw % 3;
      end
      for (lane = 0; lane < 5; lane = lane + 1)
        enums[lane] = 0;
      for (lane = 0; lane < 65; lane = lane + 1)
        wide[lane] = raw[2:0];
      for (lane = 0; lane < 3; lane = lane + 1) begin
        updated_struct_tag[lane] = struct_tag[lane];
        updated_struct_mode[lane] = struct_mode[lane];
        updated_struct_ordinal[lane] = struct_ordinal[lane];
        updated_ranges[lane] = ranges[lane];
      end
      for (lane = 0; lane < 5; lane = lane + 1)
        updated_enums[lane] = enums[lane];
      for (lane = 0; lane < 65; lane = lane + 1)
        updated_wide[lane] = wide[lane];
      updated_struct_tag[struct_index] = replacement;
      updated_struct_mode[struct_index] = 1;
      updated_struct_ordinal[struct_index] = replacement % 3;
      updated_enums[enum_index] = 1;
      updated_ranges[struct_index] = replacement % 3;
      updated_wide[wide_index] = replacement[2:0];
      for (lane = 0; lane < 65; lane = lane + 1)
        chained_wide[lane] = updated_wide[lane];
      chained_wide[0] = 7;
      expected = {
          struct_tag[struct_index], updated_struct_tag[struct_index],
          struct_mode[struct_index], updated_struct_mode[struct_index],
          enums[enum_index], updated_enums[enum_index], ranges[struct_index],
          updated_ranges[struct_index], wide[wide_index],
          updated_wide[wide_index], updated_wide[0], updated_wide[64],
          chained_wide[wide_index], updated_wide[wide_index]};
    end
  endfunction

  function automatic logic [7:0] replacement_at(input integer index);
    case (index)
      0: replacement_at = 0;
      1: replacement_at = 1;
      2: replacement_at = 2;
      3: replacement_at = 7;
      4: replacement_at = 8;
      5: replacement_at = 127;
      6: replacement_at = 128;
      7: replacement_at = 255;
      default: replacement_at = 0;
    endcase
  endfunction

  task row(input bit clock_level, reset_level, valid_level,
           input logic [15:0] request, input bit take_level);
    bit pop_second, room, pop_first, input_room, push_first, push_second;
    bit was_full;
    logic [15:0] old_first;
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
                    second_valid ? second_data : 42'b0})
      $fatal(1, "recursive_array_updates old-Q mismatch at row %0d", sampled);
    // Each named payload field is covered by the exact 42-bit comparison;
    // these slices also lock the frozen declaration order and physical widths.
    if (out_data[41:34] !== (second_valid ? second_data[41:34] : 8'b0) ||
        out_data[33:26] !== (second_valid ? second_data[33:26] : 8'b0) ||
        out_data[25:22] !== (second_valid ? second_data[25:22] : 4'b0) ||
        out_data[21:18] !== (second_valid ? second_data[21:18] : 4'b0) ||
        out_data[17:0] !== (second_valid ? second_data[17:0] : 18'b0))
      $fatal(1, "recursive_array_updates field layout mismatch at row %0d",
             sampled);
    $display("WORK %0d %0d %0d %0d", sampled, input_room, second_valid,
             second_valid ? second_data : 42'b0);

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
            $fatal(1, "recursive_array_updates lost/reordered output");
          head = head + 1;
          retired = retired + 1;
          outstanding = outstanding - 1;
        end
        if (ready && valid_level) begin
          history[tail] = expected(request);
          tail = tail + 1;
          accepted = accepted + 1;
          outstanding = outstanding + 1;
          replacements = replacements + was_full;
        end
        old_first = first_data;
        if (push_second) begin
          second_valid = 1;
          second_data = expected(old_first);
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
        $fatal(1, "recursive_array_updates logical capacity");
    end
    last_clock = clock_level;
    sampled = sampled + 1;
    pyc_7079635f636c6b = clock_level;
    #1;
  endtask

  task edge_row(input logic [15:0] request, input bit valid_level = 1,
                input bit take_level = 1, input bit reset_level = 0);
    row(1, reset_level, valid_level, request, take_level);
    row(0, reset_level, valid_level, request, take_level);
  endtask

  logic [15:0] historical [0:3];
  logic [41:0] historical_expected [0:3];
  integer index, raw, replacement_index, cycle;
  initial begin
    historical[0] = {8'd0, 8'd9};
    historical[1] = {8'd2, 8'd14};
    historical[2] = {8'd64, 8'd5};
    historical[3] = {8'd255, 8'd131};
    historical_expected[0] = 42'd624955961;
    historical_expected[1] = 42'd35322946742;
    historical_expected[2] = 42'd1099869737325;
    historical_expected[3] = 42'd4389679644635;
    for (index = 0; index < 4; index = index + 1)
      if (expected(historical[index]) !== historical_expected[index])
        $fatal(1, "recursive historical golden %0d mismatch", index);

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

    edge_row({8'd17, 8'd33}, 1, 0);
    edge_row({8'd18, 8'd34}, 1, 0);
    edge_row({8'd19, 8'd35}, 1, 0);
    row(1, 0, 1, {8'd20, 8'd36}, 0);
    row(1, 0, 1, {8'd21, 8'd37}, 1);
    row(1, 1, 0, 0, 1);
    row(0, 0, 0, 0, 1);
    edge_row(0, 0, 1, 1);

    edge_row({8'd22, 8'd38});
    edge_row(0, 0, 1, 1);

    edge_row({8'd23, 8'd39});
    edge_row({8'd24, 8'd40}, 1, 0);
    edge_row({8'd25, 8'd41});
    edge_row(0, 0);
    edge_row(0, 0);

    for (raw = 0; raw < 256; raw = raw + 1)
      for (replacement_index = 0; replacement_index < 8;
           replacement_index = replacement_index + 1)
        edge_row({raw[7:0], replacement_at(replacement_index)});
    for (raw = 0; raw < 256; raw = raw + 1)
      edge_row({raw[7:0], raw[7:0]});
    for (cycle = 0; cycle < 4; cycle = cycle + 1)
      edge_row(0, 0);

    if (sampled >= 5000 || outstanding != 0 || accepted < 2310 ||
        accepted != retired + dropped || peak != 2 || stalled < 2 ||
        replacements < 2000 || reset_full < 1 || reset_partial < 1 ||
        held_high < 2)
      $fatal(1, "recursive_array_updates finite coverage failed");
    $display("HISTORY %0d %0d %0d %0d %0d %0d", accepted, retired, dropped,
             outstanding, peak, replacements);
    $finish;
  end
endmodule
