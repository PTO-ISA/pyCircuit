module tb;
  localparam integer CHANNELS = 23;
  localparam logic [22:0] ALL_TAKES = 23'h7fffff;

  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic [47:0] data = 0;
  logic [22:0] take = 0;
  wire result_0;
  wire [3:0] result_1,result_3,result_5,result_8,result_16;
  wire [4:0] result_2,result_12;
  wire [1:0] result_4,result_7,result_13,result_14,result_15;
  wire [8:0] result_6,result_9,result_10,result_11,result_17,result_18,
             result_19,result_20,result_21,result_22,result_23;
  wire [22:0] output_valid={
    result_1[3],result_2[4],result_3[3],result_4[1],result_5[3],result_6[8],
    result_7[1],result_8[3],result_9[8],result_10[8],result_11[8],result_12[4],
    result_13[1],result_14[1],result_15[1],result_16[3],result_17[8],
    result_18[8],result_19[8],result_20[8],result_21[8],result_22[8],result_23[8]
  };
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;

  bit last_clock = 0, source_valid = 0;
  logic [47:0] source_data = 0;
  bit sink_valid [0:22];
  logic [7:0] sink_data [0:22];
  logic [7:0] history [0:22][0:2047];
  integer head [0:22], tail [0:22], retired [0:22];
  integer dropped [0:22], isolated [0:22];
  integer sampled = 0, accepted = 0, stalled = 0, transfers = 0;
  integer replacements = 0, no_flow_through = 0, reset_full = 0;
  integer reset_partial = 0, held_high = 0, peak = 0;

  pyc_root dut(.*);

  task automatic checked_prepare;
    pyc_root_commit_ok=0;pyc_phase=1;#1;
    pyc_root_commit_ok=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
    if(pyc_root_commit_ok!==1'b1)begin pyc_phase=3;#1;$fatal(1,"prepared bounded integer row failed completion/sampling/check validation");end
  endtask
  task automatic checked_xfer;
    pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  task automatic host_reset;
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  task automatic physical_reset_edges;
    pyc_7079635f636c6b=1;checked_prepare();checked_xfer();
    pyc_7079635f636c6b=0;pyc_7079635f727374=0;checked_prepare();checked_xfer();
  endtask

  function automatic bit take_at(input logic [22:0] takes,
                                 input integer channel);
    take_at = takes[22-channel];
  endfunction

  function automatic logic [22:0] without_channel(input integer channel);
    without_channel = ALL_TAKES ^ (23'b1 << (22-channel));
  endfunction

  function automatic logic [47:0] fixed_request(input logic [7:0] raw);
    fixed_request = {8'd11, 8'd22, 8'd33, 8'd44, 8'd55, raw};
  endfunction

  function automatic logic [47:0] varied_request(input logic [7:0] raw);
    if (raw == 0)
      varied_request = {8'd0, 8'd255, 8'd0, 8'd255, 8'd0, raw};
    else if (raw == 4)
      varied_request = {8'd9, 8'd9, 8'd9, 8'd9, 8'd250, raw};
    else if (raw == 255)
      varied_request = {8'd255, 8'd0, 8'd128, 8'd0, 8'd255, raw};
    else
      varied_request = {raw ^ 8'ha5, raw * 8'd3 + 8'd17, raw,
                        8'd255 - raw, raw * 8'd5 + 8'd1, raw};
  endfunction

  // Independent array-copy oracle for the original algorithm.
  function automatic logic [7:0] expected_value(input logic [47:0] token,
                                                 input integer channel);
    logic [7:0] values [0:4], updated [0:4], chained [0:4];
    logic [7:0] raw;
    integer checked, wrapped, saturated, lane;
    bit checked_valid;
    begin
      values[0] = token[47:40];
      values[1] = token[39:32];
      values[2] = token[31:24];
      values[3] = token[23:16];
      values[4] = token[15:8];
      raw = token[7:0];
      wrapped = raw % 5;
      checked_valid = raw < 5;
      checked = checked_valid ? raw : 0;
      saturated = raw < 4 ? 4 : (raw > 8 ? 8 : raw);
      for (lane = 0; lane < 5; lane = lane + 1)
        updated[lane] = values[lane];
      updated[checked] = raw;
      for (lane = 0; lane < 5; lane = lane + 1)
        chained[lane] = updated[lane];
      chained[0] = 99;
      case (channel)
        0: expected_value = wrapped;
        1: expected_value = saturated;
        2: expected_value = checked;
        3: expected_value = checked_valid;
        4: expected_value = wrapped + 1;
        5: expected_value = values[checked];
        6: expected_value = raw % 2;
        7: expected_value = raw % 8;
        8: expected_value = raw;
        9: expected_value = values[raw[1:0]];
        10: expected_value = values[0];
        11: expected_value = raw >= 4 && raw <= 8 ? raw : 4;
        12: expected_value = raw >= 4 && raw <= 8;
        13: expected_value = 1;
        14: expected_value = saturated > wrapped;
        15: expected_value = (wrapped + 1) - 1;
        16: expected_value = updated[checked];
        17: expected_value = updated[0];
        18: expected_value = updated[4];
        19: expected_value = values[checked];
        20: expected_value = chained[0];
        21: expected_value = chained[checked];
        22: expected_value = updated[checked];
        default: expected_value = 0;
      endcase
    end
  endfunction

  function automatic logic [7:0] actual_value(input integer channel);
    case (channel)
      0: actual_value = {5'b0, result_1[2:0]};
      1: actual_value = {4'b0, result_2[3:0]};
      2: actual_value = {5'b0, result_3[2:0]};
      3: actual_value = {7'b0, result_4[0]};
      4: actual_value = {5'b0, result_5[2:0]};
      5: actual_value = result_6[7:0];
      6: actual_value = {7'b0, result_7[0]};
      7: actual_value = {5'b0, result_8[2:0]};
      8: actual_value = result_9[7:0];
      9: actual_value = result_10[7:0];
      10: actual_value = result_11[7:0];
      11: actual_value = {4'b0, result_12[3:0]};
      12: actual_value = {7'b0, result_13[0]};
      13: actual_value = {7'b0, result_14[0]};
      14: actual_value = {7'b0, result_15[0]};
      15: actual_value = {5'b0, result_16[2:0]};
      16: actual_value = result_17[7:0];
      17: actual_value = result_18[7:0];
      18: actual_value = result_19[7:0];
      19: actual_value = result_20[7:0];
      20: actual_value = result_21[7:0];
      21: actual_value = result_22[7:0];
      22: actual_value = result_23[7:0];
      default: actual_value = 0;
    endcase
  endfunction

  task row(input bit clock_level, reset_level, valid_level,
           input logic [47:0] token, input logic [22:0] takes);
    bit all_room, ready, pop, old_source_valid;
    logic [47:0] old_source;
    integer channel, occupied;
    all_room = 1;
    occupied = 0;
    for (channel = 0; channel < CHANNELS; channel = channel + 1) begin
      all_room = all_room && (!sink_valid[channel] || take_at(takes, channel));
      occupied = occupied + sink_valid[channel];
    end
    ready = !source_valid || all_room;
    pyc_7079635f636c6b = clock_level;
    pyc_7079635f727374 = reset_level;
    valid = valid_level;
    data = token;
    take = takes;
    #1;checked_prepare();
    if (result_0 !== ready)
      $fatal(1, "bounded input ready mismatch at row %0d", sampled);
    $write("WORK %0d %0d", sampled, ready);
    for (channel = 0; channel < CHANNELS; channel = channel + 1) begin
      if (output_valid[22-channel] !== sink_valid[channel] ||
          actual_value(channel) !==
              (sink_valid[channel] ? sink_data[channel] : 8'd0))
        $fatal(1, "bounded channel %0d old-Q mismatch at row %0d", channel,
               sampled);
      $write(" %0d %0d", output_valid[22-channel], actual_value(channel));
    end
    $display;

    held_high = held_high + (clock_level && last_clock);
    if (clock_level && !last_clock) begin
      if (reset_level) begin
        reset_full = reset_full + (source_valid && occupied == CHANNELS);
        reset_partial = reset_partial +
                        ((source_valid || occupied != 0) && occupied != CHANNELS);
        source_valid = 0;
        source_data = 0;
        for (channel = 0; channel < CHANNELS; channel = channel + 1) begin
          dropped[channel] = dropped[channel] + tail[channel] - head[channel];
          head[channel] = 0;
          tail[channel] = 0;
          sink_valid[channel] = 0;
          sink_data[channel] = 0;
        end
      end else begin
        stalled = stalled + (valid_level && !ready);
        no_flow_through = no_flow_through +
                          (valid_level && ready && !source_valid && occupied == 0);
        replacements = replacements +
                       (source_valid && all_room && valid_level && ready);
        old_source = source_data;
        old_source_valid = source_valid;
        for (channel = 0; channel < CHANNELS; channel = channel + 1) begin
          pop = sink_valid[channel] && take_at(takes, channel);
          if (pop) begin
            if (head[channel] == tail[channel] ||
                sink_data[channel] !== history[channel][head[channel]])
              $fatal(1, "bounded channel %0d lost/reordered", channel);
            head[channel] = head[channel] + 1;
            retired[channel] = retired[channel] + 1;
            isolated[channel] = isolated[channel] + (!all_room);
          end
          if (old_source_valid && all_room) begin
            sink_valid[channel] = 1;
            sink_data[channel] = expected_value(old_source, channel);
          end else if (pop) begin
            sink_valid[channel] = 0;
            sink_data[channel] = 0;
          end
          if (valid_level && ready) begin
            history[channel][tail[channel]] = expected_value(token, channel);
            tail[channel] = tail[channel] + 1;
          end
          if (tail[channel] - head[channel] > peak)
            peak = tail[channel] - head[channel];
          if (tail[channel] - head[channel] > 2)
            $fatal(1, "bounded channel %0d logical capacity", channel);
        end
        transfers = transfers + (old_source_valid && all_room);
        if (valid_level && ready) begin
          source_valid = 1;
          source_data = token;
          accepted = accepted + 1;
        end else if (old_source_valid && all_room) begin
          source_valid = 0;
          source_data = 0;
        end
      end
    end
    last_clock = clock_level;
    sampled = sampled + 1;
    checked_xfer();
  endtask

  task edge_row(input logic [47:0] token, input bit valid_level = 1,
                input logic [22:0] takes = ALL_TAKES,
                input bit reset_level = 0);
    row(1, reset_level, valid_level, token, takes);
    row(0, reset_level, valid_level, token, takes);
  endtask

  logic [7:0] historical [0:6];
  integer cycle, offered, before_accepted, channel, raw;
  initial begin
    historical[0] = 0;
    historical[1] = 3;
    historical[2] = 4;
    historical[3] = 5;
    historical[4] = 8;
    historical[5] = 9;
    historical[6] = 255;
    for (channel = 0; channel < CHANNELS; channel = channel + 1) begin
      sink_valid[channel] = 0;
      sink_data[channel] = 0;
      head[channel] = 0;
      tail[channel] = 0;
      retired[channel] = 0;
      dropped[channel] = 0;
      isolated[channel] = 0;
    end
    #1;host_reset();physical_reset_edges();
    row(0, 1, 0, 0, 0);
    edge_row(0, 0, 0, 1);

    offered = 0;
    for (cycle = 0; cycle < 96; cycle = cycle + 1) begin
      before_accepted = accepted;
      edge_row(fixed_request(offered < 7 ? historical[offered] : 8'd255),
               offered < 7,
               cycle % 7 != 2 && cycle % 7 != 3 ? ALL_TAKES : 0);
      if (accepted != before_accepted)
        offered = offered + 1;
    end
    if (offered != 7)
      $fatal(1, "historical bounded vectors were not all accepted");
    for (cycle = 0; cycle < 4; cycle = cycle + 1)
      edge_row(0, 0);

    for (channel = 0; channel < CHANNELS; channel = channel + 1) begin
      edge_row(varied_request(32 + channel * 3));
      edge_row(varied_request(33 + channel * 3));
      edge_row(varied_request(34 + channel * 3), 1,
               without_channel(channel));
      edge_row(varied_request(35 + channel * 3), 1,
               without_channel(channel));
      edge_row(varied_request(36 + channel * 3));
      edge_row(0, 0);
      edge_row(0, 0);
    end

    for (raw = 0; raw < 256; raw = raw + 1)
      edge_row(varied_request(raw));
    edge_row(0, 0);
    edge_row(0, 0);

    row(1, 0, 1, varied_request(201), 0);
    row(1, 0, 1, varied_request(202), ALL_TAKES);
    row(1, 1, 0, 0, ALL_TAKES);
    row(0, 0, 0, 0, ALL_TAKES);
    edge_row(0, 0);
    edge_row(0, 0);

    edge_row(varied_request(210));
    edge_row(varied_request(211), 1, 0);
    edge_row(0, 0, ALL_TAKES, 1);
    edge_row(varied_request(212));
    edge_row(varied_request(213));
    edge_row(varied_request(214), 1, without_channel(7));
    edge_row(0, 0, ALL_TAKES, 1);
    edge_row(varied_request(255));
    edge_row(varied_request(0));
    edge_row(0, 0);
    edge_row(0, 0);

    if (sampled >= 1600 || source_valid || peak != 2 || accepted < 300 ||
        stalled < CHANNELS || transfers < accepted - 8 ||
        replacements < 250 || no_flow_through < 3 || reset_full < 1 ||
        reset_partial < 1 || held_high < 2)
      $fatal(1, "bounded finite coverage counters failed");
    for (channel = 0; channel < CHANNELS; channel = channel + 1) begin
      if (sink_valid[channel] || head[channel] != tail[channel] ||
          isolated[channel] == 0 || dropped[channel] < 2 ||
          accepted != retired[channel] + dropped[channel])
        $fatal(1, "bounded channel %0d conservation failed", channel);
      $display("HISTORY %0d %0d %0d %0d 0 %0d", channel, accepted,
               retired[channel], dropped[channel], peak);
    end
    $finish;
  end
endmodule
