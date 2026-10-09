// Independent endpoint scan implements the reviewed three-case X/Z oracle.
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
  wire [277:0] result;
  pyc_root dut(.*);
  integer frame = 0, known_frames = 0, masked_frames = 0;
  localparam integer SCALAR = 0, ZERO = 1, ONES = 2, ONEHOT = 3,
      PREFIX = 4, SUFFIX = 5, MIXED = 6, ZERO_MASKED = 7, ONES_MASKED = 8,
      LEADING_ENDPOINT = 9, TRAILING_ENDPOINT = 10, DENSE = 11, DOUBLE = 12,
      LITERAL_CONTROL = 13;

`ifdef COUNT_ZEROS_FOUR_STATE
  function automatic logic [3:0] literal_input(input integer row);
    logic [3:0] value;
    case (row % 7)
      0: value = 4'b1xxx;
      1: value = 4'b01xz;
      2: value = 4'bx111;
      3: value = 4'bx011;
      4: value = 4'b00x1;
      5: value = 4'bxx11;
      6: value = 4'b0000;
    endcase
    if (row >= 7) return {value[0], value[1], value[2], value[3]};
    return value;
  endfunction

  function automatic logic [2:0] literal_count(input integer row);
    case (row % 7)
      0: return 3'b000;
      1: return 3'b001;
      2: return 3'b00x;
      3: return 3'b0xx;
      4: return 3'bxxx;
      5: return 3'bxxx;
      6: return 3'b100;
    endcase
  endfunction
`endif

  function automatic logic [128:0] stimulus(
      input integer width, input integer pattern, input integer row,
      input integer high_z);
    logic [128:0] value;
    logic [3:0] control_bits;
    value = '0;
`ifdef COUNT_ZEROS_FOUR_STATE
    if (pattern == LITERAL_CONTROL) control_bits = literal_input(row);
`endif
    for (integer index = 0; index < width; index = index + 1) begin
      case (pattern)
        SCALAR: value[index] = index < 9 && ((row >> index) & 1);
        ZERO: value[index] = 1'b0;
        ONES: value[index] = 1'b1;
        ONEHOT: value[index] = index == row;
        PREFIX: value[index] = row < width && index < width - row;
        SUFFIX: value[index] = index >= row;
        MIXED: value[index] = ((index * 7 + row * 3) % 11) < 5;
`ifdef COUNT_ZEROS_FOUR_STATE
        // Isolate actual procedural tristates from the Verilator known build.
        ZERO_MASKED: value[index] = index == row ? (high_z ? 1'bz : 1'bx) : 1'b0;
        ONES_MASKED: value[index] = index == row ? (high_z ? 1'bz : 1'bx) : 1'b1;
        LEADING_ENDPOINT:
          if (index == width - 1) value[index] = high_z ? 1'bz : 1'bx;
          else value[index] = row < width && index == width - 1 - row;
        TRAILING_ENDPOINT:
          if (index == 0) value[index] = high_z ? 1'bz : 1'bx;
          else value[index] = row < width && index == row;
        DENSE: case ((index + row) % 4)
          0: value[index] = 1'b0;
          1: value[index] = 1'b1;
          2: value[index] = 1'bx;
          3: value[index] = 1'bz;
        endcase
        DOUBLE:
          value[index] = index == 0 || index == width - 1
                         ? (high_z ? 1'bz : 1'bx) : 1'b0;
        LITERAL_CONTROL:
          if (width == 4) value[index] = control_bits[index];
          else value[index] = 1'b0;
`endif
        default: $fatal(1, "invalid stimulus pattern");
      endcase
    end
    return value;
  endfunction

  task automatic drive(input integer pattern, input integer row,
                       input integer high_z);
    w1 = stimulus(1, pattern, row, high_z);
    w2 = stimulus(2, pattern, row, high_z);
    w3 = stimulus(3, pattern, row, high_z);
    w4 = stimulus(4, pattern, row, high_z);
    w5 = stimulus(5, pattern, row, high_z);
    w7 = stimulus(7, pattern, row, high_z);
    w8 = stimulus(8, pattern, row, high_z);
    w9 = stimulus(9, pattern, row, high_z);
    w15 = stimulus(15, pattern, row, high_z);
    w16 = stimulus(16, pattern, row, high_z);
    w17 = stimulus(17, pattern, row, high_z);
    w31 = stimulus(31, pattern, row, high_z);
    w32 = stimulus(32, pattern, row, high_z);
    w33 = stimulus(33, pattern, row, high_z);
    w63 = stimulus(63, pattern, row, high_z);
    w64 = stimulus(64, pattern, row, high_z);
    w65 = stimulus(65, pattern, row, high_z);
    w73 = stimulus(73, pattern, row, high_z);
    w127 = stimulus(127, pattern, row, high_z);
    w128 = stimulus(128, pattern, row, high_z);
    w129 = stimulus(129, pattern, row, high_z);
    state = stimulus(3, pattern, row, high_z);
  endtask

  function automatic integer bit_width(input integer value);
    integer width;
    width = 0;
    do begin
      width = width + 1;
      value = value >> 1;
    end while (value != 0);
    return width;
  endfunction

  // LSB-based start/length selects source bits; direction selects the endpoint.
  // Upper bits are known zero for destination widening. Input Z becomes X.
  function automatic logic [12:0] count_zeros(
      input logic [128:0] value, input integer length, input integer start,
      input bit leading);
    logic [12:0] expected;
    logic symbol;
    integer distance, unknowns, natural_width, poison, index;
    bit endpoint_unknown;
    expected = '0;
    distance = length;
    unknowns = 0;
    endpoint_unknown = 0;
    natural_width = bit_width(length);
    for (integer position = 0; position < length && distance == length;
         position = position + 1) begin
      index = start + (leading ? length - 1 - position : position);
      symbol = value[index];
      if (symbol === 1'b1) begin
        distance = position;
      end
      else if (symbol !== 1'b0) begin
        unknowns = unknowns + 1;
        if (position == 0) endpoint_unknown = 1;
      end
    end
    if (unknowns == 0) begin
      for (integer index = 0; index < natural_width; index = index + 1)
        expected[index] = (distance >> index) & 1;
    end else begin
      poison = endpoint_unknown && unknowns == 1
               ? bit_width(distance) : natural_width;
      for (integer index = 0; index < poison; index = index + 1)
        expected[index] = 1'bx;
    end
    return expected;
  endfunction

  function automatic logic [128:0] increment_five(input logic [4:0] value);
    logic [128:0] expected;
    integer known_value;
    bit known;
    expected = '0;
    known = 1;
    known_value = 0;
    for (integer index = 0; index < 5; index = index + 1) begin
      if (value[index] !== 1'b0 && value[index] !== 1'b1) known = 0;
      if (value[index] === 1'b1) known_value = known_value + (1 << index);
    end
    known_value = (known_value + 1) % 32;
    for (integer index = 0; index < 5; index = index + 1)
      expected[index] = known ? ((known_value >> index) & 1) : 1'bx;
    return expected;
  endfunction

  function automatic logic [277:0] golden();
    logic [277:0] expected;
    logic [12:0] counted;
    logic [128:0] nested;
    expected = '0;
    counted = count_zeros(w1, 1, 0, 1);
    expected[277 +: 1] = counted[0:0]; // leading1
    counted = count_zeros(w1, 1, 0, 0);
    expected[276 +: 1] = counted[0:0]; // trailing1
    counted = count_zeros(w2, 2, 0, 1);
    expected[274 +: 2] = counted[1:0]; // leading2
    counted = count_zeros(w2, 2, 0, 0);
    expected[272 +: 2] = counted[1:0]; // trailing2
    counted = count_zeros(w3, 3, 0, 1);
    expected[270 +: 2] = counted[1:0]; // leading3
    counted = count_zeros(w3, 3, 0, 0);
    expected[268 +: 2] = counted[1:0]; // trailing3
    counted = count_zeros(w4, 4, 0, 1);
    expected[265 +: 3] = counted[2:0]; // leading4
    counted = count_zeros(w4, 4, 0, 0);
    expected[262 +: 3] = counted[2:0]; // trailing4
    counted = count_zeros(w5, 5, 0, 1);
    expected[259 +: 3] = counted[2:0]; // leading5
    counted = count_zeros(w5, 5, 0, 0);
    expected[256 +: 3] = counted[2:0]; // trailing5
    counted = count_zeros(w7, 7, 0, 1);
    expected[253 +: 3] = counted[2:0]; // leading7
    counted = count_zeros(w7, 7, 0, 0);
    expected[250 +: 3] = counted[2:0]; // trailing7
    counted = count_zeros(w8, 8, 0, 1);
    expected[246 +: 4] = counted[3:0]; // leading8
    counted = count_zeros(w8, 8, 0, 0);
    expected[242 +: 4] = counted[3:0]; // trailing8
    counted = count_zeros(w9, 9, 0, 1);
    expected[238 +: 4] = counted[3:0]; // leading9
    counted = count_zeros(w9, 9, 0, 0);
    expected[234 +: 4] = counted[3:0]; // trailing9
    counted = count_zeros(w15, 15, 0, 1);
    expected[230 +: 4] = counted[3:0]; // leading15
    counted = count_zeros(w15, 15, 0, 0);
    expected[226 +: 4] = counted[3:0]; // trailing15
    counted = count_zeros(w16, 16, 0, 1);
    expected[221 +: 5] = counted[4:0]; // leading16
    counted = count_zeros(w16, 16, 0, 0);
    expected[216 +: 5] = counted[4:0]; // trailing16
    counted = count_zeros(w17, 17, 0, 1);
    expected[211 +: 5] = counted[4:0]; // leading17
    counted = count_zeros(w17, 17, 0, 0);
    expected[206 +: 5] = counted[4:0]; // trailing17
    counted = count_zeros(w31, 31, 0, 1);
    expected[201 +: 5] = counted[4:0]; // leading31
    counted = count_zeros(w31, 31, 0, 0);
    expected[196 +: 5] = counted[4:0]; // trailing31
    counted = count_zeros(w32, 32, 0, 1);
    expected[190 +: 6] = counted[5:0]; // leading32
    counted = count_zeros(w32, 32, 0, 0);
    expected[184 +: 6] = counted[5:0]; // trailing32
    counted = count_zeros(w33, 33, 0, 1);
    expected[178 +: 6] = counted[5:0]; // leading33
    counted = count_zeros(w33, 33, 0, 0);
    expected[172 +: 6] = counted[5:0]; // trailing33
    counted = count_zeros(w63, 63, 0, 1);
    expected[166 +: 6] = counted[5:0]; // leading63
    counted = count_zeros(w63, 63, 0, 0);
    expected[160 +: 6] = counted[5:0]; // trailing63
    counted = count_zeros(w64, 64, 0, 1);
    expected[153 +: 7] = counted[6:0]; // leading64
    counted = count_zeros(w64, 64, 0, 0);
    expected[146 +: 7] = counted[6:0]; // trailing64
    counted = count_zeros(w65, 65, 0, 1);
    expected[139 +: 7] = counted[6:0]; // leading65
    counted = count_zeros(w65, 65, 0, 0);
    expected[132 +: 7] = counted[6:0]; // trailing65
    counted = count_zeros(w73, 73, 0, 1);
    expected[125 +: 7] = counted[6:0]; // leading73
    counted = count_zeros(w73, 73, 0, 0);
    expected[118 +: 7] = counted[6:0]; // trailing73
    counted = count_zeros(w127, 127, 0, 1);
    expected[111 +: 7] = counted[6:0]; // leading127
    counted = count_zeros(w127, 127, 0, 0);
    expected[104 +: 7] = counted[6:0]; // trailing127
    counted = count_zeros(w128, 128, 0, 1);
    expected[96 +: 8] = counted[7:0]; // leading128
    counted = count_zeros(w128, 128, 0, 0);
    expected[88 +: 8] = counted[7:0]; // trailing128
    counted = count_zeros(w129, 129, 0, 1);
    expected[80 +: 8] = counted[7:0]; // leading129
    counted = count_zeros(w129, 129, 0, 0);
    expected[72 +: 8] = counted[7:0]; // trailing129
    counted = count_zeros(w1, 1, 0, 1);
    expected[64 +: 8] = counted[7:0]; // leadingWiden1
    counted = count_zeros(w1, 1, 0, 0);
    expected[56 +: 8] = counted[7:0]; // trailingWiden1
    counted = count_zeros(w65, 65, 0, 1);
    expected[43 +: 13] = counted[12:0]; // leadingWiden65
    counted = count_zeros(w65, 65, 0, 0);
    expected[30 +: 13] = counted[12:0]; // trailingWiden65
    counted = count_zeros(w73, 9, 61, 1);
    expected[26 +: 4] = counted[3:0]; // leadingSlice73
    counted = count_zeros(w73, 9, 61, 0);
    expected[22 +: 4] = counted[3:0]; // trailingSlice73
    counted = count_zeros(increment_five(w5), 5, 0, 1);
    expected[19 +: 3] = counted[2:0]; // leadingArithmetic5
    counted = count_zeros(increment_five(w5), 5, 0, 0);
    expected[16 +: 3] = counted[2:0]; // trailingArithmetic5
    counted = count_zeros(state, 3, 0, 1);
    expected[14 +: 2] = counted[1:0]; // leadingEnum3
    counted = count_zeros(state, 3, 0, 0);
    expected[12 +: 2] = counted[1:0]; // trailingEnum3
    counted = count_zeros(w5, 5, 0, 1);
    expected[9 +: 3] = counted[2:0]; // leadingChild5
    counted = count_zeros(w5, 5, 0, 0);
    expected[6 +: 3] = counted[2:0]; // trailingChild5
    nested = count_zeros(w73, 73, 0, 0);
    counted = count_zeros(nested, 7, 0, 1);
    expected[3 +: 3] = counted[2:0]; // leadingNested73
    nested = count_zeros(w73, 73, 0, 1);
    counted = count_zeros(nested, 7, 0, 0);
    expected[0 +: 3] = counted[2:0]; // trailingNested73
    return expected;
  endfunction

  task automatic sample(input bit masked);
    logic [277:0] expected;
    #1;
    expected = golden();
    if (result !== expected)
      $fatal(1, "count-zero frame %0d got %b expected %b", frame, result, expected);
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
      drive(SCALAR, row, 0); sample(0);
    end
    drive(ZERO, 0, 0); sample(0);
    drive(ONES, 0, 0); sample(0);
    for (integer row = 0; row < 129; row = row + 1) begin
      drive(ONEHOT, row, 0); sample(0);
    end
    for (integer row = 0; row < 129; row = row + 1) begin
      drive(PREFIX, row, 0); sample(0);
      drive(SUFFIX, row, 0); sample(0);
    end
    for (integer row = 0; row < 16; row = row + 1) begin
      drive(MIXED, row, 0); sample(0);
    end
    if (known_frames != 917) $fatal(1, "wrong known prefix frame count");
`ifdef COUNT_ZEROS_FOUR_STATE
    for (integer pattern = ZERO_MASKED; pattern <= ONES_MASKED; pattern = pattern + 1)
      for (integer row = 0; row < 129; row = row + 1)
        for (integer high_z = 0; high_z < 2; high_z = high_z + 1)
          // Replay both native latent alternatives with their same RTL symbols.
          for (integer latent = 0; latent < 2; latent = latent + 1) begin
            drive(pattern, row, high_z); sample(1);
          end
    for (integer pattern = LEADING_ENDPOINT; pattern <= TRAILING_ENDPOINT; pattern = pattern + 1)
      for (integer row = 1; row <= 129; row = row + 1)
        for (integer high_z = 0; high_z < 2; high_z = high_z + 1)
          for (integer latent = 0; latent < 2; latent = latent + 1) begin
            drive(pattern, row, high_z); sample(1);
          end
    for (integer phase = 0; phase < 4; phase = phase + 1)
      for (integer latent = 0; latent < 2; latent = latent + 1) begin
        drive(DENSE, phase, 0); sample(1);
      end
    for (integer high_z = 0; high_z < 2; high_z = high_z + 1)
      for (integer latent = 0; latent < 2; latent = latent + 1) begin
        drive(DOUBLE, 0, high_z); sample(1);
      end
    if (masked_frames != 2076) $fatal(1, "wrong masked frame count");
    // Preserve all existing controls; append seven literal leading cases and
    // their reversed trailing counterparts, replaying both native latents.
    for (integer row = 0; row < 14; row = row + 1)
      for (integer latent = 0; latent < 2; latent = latent + 1) begin
        drive(LITERAL_CONTROL, row, 0); sample(1);
        if (row < 7) begin
          if (result[265 +: 3] !== literal_count(row))
            $fatal(1, "literal four-bit leading control row %0d", row);
        end else begin
          if (result[262 +: 3] !== literal_count(row))
            $fatal(1, "reversed four-bit trailing control row %0d", row);
        end
      end
    if (masked_frames != 2104) $fatal(1, "wrong repaired masked frame count");
`endif
    drive(ONES, 0, 0); sample(0);
    if (known_frames != 918) $fatal(1, "wrong known total frame count");
`ifdef COUNT_ZEROS_FOUR_STATE
    if (frame != 3022) $fatal(1, "wrong total frame count");
`else
    if (frame != 918) $fatal(1, "wrong total frame count");
`endif
    $finish;
  end
endmodule
