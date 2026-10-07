// Independent per-bit counts and four-state symbols for the frozen fixture.
module tb;
  logic [0:0] w1;
  logic [1:0] w2;
  logic [2:0] w3;
  logic [3:0] w4;
  logic [4:0] w5;
  logic [6:0] w7;
  logic [7:0] w8;
  logic [8:0] w9;
  logic [14:0] w15;
  logic [15:0] w16;
  logic [16:0] w17;
  logic [30:0] w31;
  logic [31:0] w32;
  logic [32:0] w33;
  logic [62:0] w63;
  logic [63:0] w64;
  logic [64:0] w65;
  logic [72:0] w73;
  logic [126:0] w127;
  logic [127:0] w128;
  logic [128:0] w129;
  logic [2:0] state;
  wire [138:0] result;
  pyc_root dut(.*);

  integer frame = 0;
  integer known_frames = 0;
  integer masked_frames = 0;
  localparam integer SCALAR = 0, ZERO = 1, ONES = 2, ONEHOT = 3, MIXED = 4;

  function automatic logic [128:0] known_input(
      input integer width, input integer pattern, input integer row);
    logic [128:0] value;
    value = '0;
    for (integer bit_index = 0; bit_index < width; bit_index = bit_index + 1) begin
      case (pattern)
        SCALAR: value[bit_index] = bit_index < 9 && ((row >> bit_index) & 1);
        ZERO: value[bit_index] = 1'b0;
        ONES: value[bit_index] = 1'b1;
        ONEHOT: value[bit_index] = bit_index == row;
        MIXED: value[bit_index] = ((bit_index * 7 + row * 3) % 11) < 5;
        default: $fatal(1, "invalid known pattern");
      endcase
    end
    return value;
  endfunction

  task automatic drive_known(input integer pattern, input integer row);
    w1 = known_input(1, pattern, row);
    w2 = known_input(2, pattern, row);
    w3 = known_input(3, pattern, row);
    w4 = known_input(4, pattern, row);
    w5 = known_input(5, pattern, row);
    w7 = known_input(7, pattern, row);
    w8 = known_input(8, pattern, row);
    w9 = known_input(9, pattern, row);
    w15 = known_input(15, pattern, row);
    w16 = known_input(16, pattern, row);
    w17 = known_input(17, pattern, row);
    w31 = known_input(31, pattern, row);
    w32 = known_input(32, pattern, row);
    w33 = known_input(33, pattern, row);
    w63 = known_input(63, pattern, row);
    w64 = known_input(64, pattern, row);
    w65 = known_input(65, pattern, row);
    w73 = known_input(73, pattern, row);
    w127 = known_input(127, pattern, row);
    w128 = known_input(128, pattern, row);
    w129 = known_input(129, pattern, row);
    state = known_input(3, pattern, row);
  endtask

`ifdef POPCOUNT_FOUR_STATE
  // Keep procedural tristate assignments out of the Verilator known build.
  task automatic drive_masked(input integer position, input integer high_z);
    drive_known(MIXED, 0);
    if (position < 1) w1[position] = high_z ? 1'bz : 1'bx;
    if (position < 2) w2[position] = high_z ? 1'bz : 1'bx;
    if (position < 3) w3[position] = high_z ? 1'bz : 1'bx;
    if (position < 4) w4[position] = high_z ? 1'bz : 1'bx;
    if (position < 5) w5[position] = high_z ? 1'bz : 1'bx;
    if (position < 7) w7[position] = high_z ? 1'bz : 1'bx;
    if (position < 8) w8[position] = high_z ? 1'bz : 1'bx;
    if (position < 9) w9[position] = high_z ? 1'bz : 1'bx;
    if (position < 15) w15[position] = high_z ? 1'bz : 1'bx;
    if (position < 16) w16[position] = high_z ? 1'bz : 1'bx;
    if (position < 17) w17[position] = high_z ? 1'bz : 1'bx;
    if (position < 31) w31[position] = high_z ? 1'bz : 1'bx;
    if (position < 32) w32[position] = high_z ? 1'bz : 1'bx;
    if (position < 33) w33[position] = high_z ? 1'bz : 1'bx;
    if (position < 63) w63[position] = high_z ? 1'bz : 1'bx;
    if (position < 64) w64[position] = high_z ? 1'bz : 1'bx;
    if (position < 65) w65[position] = high_z ? 1'bz : 1'bx;
    if (position < 73) w73[position] = high_z ? 1'bz : 1'bx;
    if (position < 127) w127[position] = high_z ? 1'bz : 1'bx;
    if (position < 128) w128[position] = high_z ? 1'bz : 1'bx;
    if (position < 129) w129[position] = high_z ? 1'bz : 1'bx;
    if (position < 3) state[position] = high_z ? 1'bz : 1'bx;
  endtask

  function automatic logic [128:0] dense_input(
      input integer width, input integer phase);
    logic [128:0] value;
    value = '0;
    for (integer bit_index = 0; bit_index < width; bit_index = bit_index + 1)
      case ((bit_index + phase) % 4)
        0: value[bit_index] = 1'b0;
        1: value[bit_index] = 1'b1;
        2: value[bit_index] = 1'bx;
        3: value[bit_index] = 1'bz;
      endcase
    return value;
  endfunction

  task automatic drive_dense(input integer phase);
    w1 = dense_input(1, phase);
    w2 = dense_input(2, phase);
    w3 = dense_input(3, phase);
    w4 = dense_input(4, phase);
    w5 = dense_input(5, phase);
    w7 = dense_input(7, phase);
    w8 = dense_input(8, phase);
    w9 = dense_input(9, phase);
    w15 = dense_input(15, phase);
    w16 = dense_input(16, phase);
    w17 = dense_input(17, phase);
    w31 = dense_input(31, phase);
    w32 = dense_input(32, phase);
    w33 = dense_input(33, phase);
    w63 = dense_input(63, phase);
    w64 = dense_input(64, phase);
    w65 = dense_input(65, phase);
    w73 = dense_input(73, phase);
    w127 = dense_input(127, phase);
    w128 = dense_input(128, phase);
    w129 = dense_input(129, phase);
    state = dense_input(3, phase);
  endtask
`endif

  // The width-one case transports its symbol. Wider unknown counts are all X;
  // widened high bits remain zero. Only selected bits participate in slices.
  function automatic logic [12:0] count_bits(
      input logic [128:0] value, input integer length, input integer start,
      input integer natural_width);
    logic [12:0] expected;
    integer ones;
    bit known;
    expected = '0;
    ones = 0;
    known = 1;
    if (length == 1) begin
      expected[0] = value[start];
    end else begin
      for (integer bit_index = start; bit_index < start + length;
           bit_index = bit_index + 1) begin
        if (value[bit_index] !== 1'b0 && value[bit_index] !== 1'b1) known = 0;
        if (value[bit_index] === 1'b1) ones = ones + 1;
      end
      for (integer bit_index = 0; bit_index < natural_width;
           bit_index = bit_index + 1)
        expected[bit_index] = known ? ((ones >> bit_index) & 1) : 1'bx;
    end
    return expected;
  endfunction

  function automatic logic [138:0] golden();
    logic [138:0] expected;
    logic [12:0] counted;
    logic [128:0] arithmetic;
    integer source_value, incremented, wide_count, nested_count;
    bit five_known, wide_known;
    expected = '0;
    counted = count_bits(w1, 1, 0, 1);
    expected[138 +: 1] = counted[0:0];
    counted = count_bits(w2, 2, 0, 2);
    expected[136 +: 2] = counted[1:0];
    counted = count_bits(w3, 3, 0, 2);
    expected[134 +: 2] = counted[1:0];
    counted = count_bits(w4, 4, 0, 3);
    expected[131 +: 3] = counted[2:0];
    counted = count_bits(w5, 5, 0, 3);
    expected[128 +: 3] = counted[2:0];
    counted = count_bits(w7, 7, 0, 3);
    expected[125 +: 3] = counted[2:0];
    counted = count_bits(w8, 8, 0, 4);
    expected[121 +: 4] = counted[3:0];
    counted = count_bits(w9, 9, 0, 4);
    expected[117 +: 4] = counted[3:0];
    counted = count_bits(w15, 15, 0, 4);
    expected[113 +: 4] = counted[3:0];
    counted = count_bits(w16, 16, 0, 5);
    expected[108 +: 5] = counted[4:0];
    counted = count_bits(w17, 17, 0, 5);
    expected[103 +: 5] = counted[4:0];
    counted = count_bits(w31, 31, 0, 5);
    expected[98 +: 5] = counted[4:0];
    counted = count_bits(w32, 32, 0, 6);
    expected[92 +: 6] = counted[5:0];
    counted = count_bits(w33, 33, 0, 6);
    expected[86 +: 6] = counted[5:0];
    counted = count_bits(w63, 63, 0, 6);
    expected[80 +: 6] = counted[5:0];
    counted = count_bits(w64, 64, 0, 7);
    expected[73 +: 7] = counted[6:0];
    counted = count_bits(w65, 65, 0, 7);
    expected[66 +: 7] = counted[6:0];
    counted = count_bits(w73, 73, 0, 7);
    expected[59 +: 7] = counted[6:0];
    counted = count_bits(w127, 127, 0, 7);
    expected[52 +: 7] = counted[6:0];
    counted = count_bits(w128, 128, 0, 8);
    expected[44 +: 8] = counted[7:0];
    counted = count_bits(w129, 129, 0, 8);
    expected[36 +: 8] = counted[7:0];
    counted = count_bits(w1, 1, 0, 1);
    expected[28 +: 8] = counted[7:0];
    counted = count_bits(w65, 65, 0, 7);
    expected[15 +: 13] = counted;
    counted = count_bits(w73, 9, 61, 4);
    expected[11 +: 4] = counted[3:0];
    counted = count_bits(state, 3, 0, 2);
    expected[6 +: 2] = counted[1:0];
    counted = count_bits(w5, 5, 0, 3);
    expected[3 +: 3] = counted[2:0];

    five_known = 1;
    source_value = 0;
    for (integer bit_index = 0; bit_index < 5; bit_index = bit_index + 1) begin
      if (w5[bit_index] !== 1'b0 && w5[bit_index] !== 1'b1) five_known = 0;
      if (w5[bit_index] === 1'b1) source_value = source_value + (1 << bit_index);
    end
    arithmetic = '0;
    incremented = (source_value + 1) % 32;
    for (integer bit_index = 0; bit_index < 5; bit_index = bit_index + 1)
      arithmetic[bit_index] = five_known ? ((incremented >> bit_index) & 1) : 1'bx;
    counted = count_bits(arithmetic, 5, 0, 3);
    expected[8 +: 3] = counted[2:0];

    wide_known = 1;
    wide_count = 0;
    nested_count = 0;
    for (integer bit_index = 0; bit_index < 73; bit_index = bit_index + 1) begin
      if (w73[bit_index] !== 1'b0 && w73[bit_index] !== 1'b1) wide_known = 0;
      if (w73[bit_index] === 1'b1) wide_count = wide_count + 1;
    end
    for (integer bit_index = 0; bit_index < 7; bit_index = bit_index + 1)
      if ((wide_count >> bit_index) & 1) nested_count = nested_count + 1;
    for (integer bit_index = 0; bit_index < 3; bit_index = bit_index + 1)
      expected[bit_index] = wide_known ? ((nested_count >> bit_index) & 1) : 1'bx;
    return expected;
  endfunction

  task automatic sample(input bit masked);
    logic [138:0] expected;
    #1;
    expected = golden();
    if (result !== expected)
      $fatal(1, "popcount frame %0d got %b expected %b", frame, result, expected);
    if (masked) begin
      $display("MASK %b", result);
      masked_frames = masked_frames + 1;
    end else begin
      $display("WORK %b", result);
      known_frames = known_frames + 1;
    end
    frame = frame + 1;
  endtask

  initial begin
    for (integer row = 0; row < 512; row = row + 1) begin
      drive_known(SCALAR, row);
      sample(0);
    end
    drive_known(ZERO, 0); sample(0);
    drive_known(ONES, 0); sample(0);
    for (integer position = 0; position < 129; position = position + 1) begin
      drive_known(ONEHOT, position);
      sample(0);
    end
    for (integer row = 0; row < 16; row = row + 1) begin
      drive_known(MIXED, row);
      sample(0);
    end
    if (known_frames != 659) $fatal(1, "wrong known prefix frame count");
`ifdef POPCOUNT_FOUR_STATE
    for (integer position = 0; position < 129; position = position + 1)
      for (integer high_z = 0; high_z < 2; high_z = high_z + 1)
        // Native latent alternatives have identical RTL symbols; replay both.
        for (integer latent = 0; latent < 2; latent = latent + 1) begin
          drive_masked(position, high_z);
          sample(1);
        end
    for (integer phase = 0; phase < 4; phase = phase + 1)
      for (integer latent = 0; latent < 2; latent = latent + 1) begin
        drive_dense(phase);
        sample(1);
      end
    if (masked_frames != 524) $fatal(1, "wrong masked frame count");
`endif
    drive_known(ONES, 0); sample(0);
    if (known_frames != 660) $fatal(1, "wrong known total frame count");
    $finish;
  end
endmodule
