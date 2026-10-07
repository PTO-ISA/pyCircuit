module tb;
  logic [7:0] a;
  logic [12:0] mid;
  logic [39:0] addr;
  logic [64:0] wide;
  logic [0:0] one;
  wire [7:0] identity;
  wire [7:0] original_xor;
  wire [7:0] original_or;
  wire [0:0] original_equal;
  wire [12:0] mid_zero;
  wire [11:0] mid_one;
  wire [0:0] mid_last;
  wire [0:0] mid_computed;
  wire [0:0] mid_at;
  wire [0:0] mid_above;
  wire [0:0] mid_huge;
  wire [27:0] tag;
  wire [64:0] wide_zero;
  wire [63:0] wide_one;
  wire [0:0] wide_last;
  wire [0:0] wide_at;
  wire [0:0] wide_above;
  wire [0:0] wide_huge;
  wire [0:0] one_zero;
  wire [0:0] one_at;
  wire [0:0] add_before;
  wire [7:0] masked;
  wire [63:0] singleton;
  wire [7:0] static_signed;
  wire [7:0] bound;
  pyc_root dut(.*);
  initial begin
    // Independent fixed frame 0.
    a = 8'b00000000;
    mid = 13'b0000000000000;
    addr = 40'b0000000000000000000000000000000000000000;
    wide = 65'b00000000000000000000000000000000000000000000000000000000000000000;
    one = 1'b0;
    #1;
    if (!(
      identity === 8'b00000000 &&
      original_xor === 8'b00000001 &&
      original_or === 8'b00000001 &&
      original_equal === 1'b1 &&
      mid_zero === 13'b0000000000000 &&
      mid_one === 12'b000000000000 &&
      mid_last === 1'b0 &&
      mid_computed === 1'b0 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b0000000000000000000000000000 &&
      wide_zero === 65'b00000000000000000000000000000000000000000000000000000000000000000 &&
      wide_one === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      wide_last === 1'b0 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b0 &&
      one_at === 1'b0 &&
      add_before === 1'b0 &&
      masked === 8'b00000000 &&
      singleton === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      static_signed === 8'b11111100 &&
      bound === 8'b00000000)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 1.
    a = 8'b11111111;
    mid = 13'b1111111111111;
    addr = 40'b1111111111111111111111111111111111111111;
    wide = 65'b11111111111111111111111111111111111111111111111111111111111111111;
    one = 1'b1;
    #1;
    if (!(
      identity === 8'b11111111 &&
      original_xor === 8'b11111110 &&
      original_or === 8'b11111111 &&
      original_equal === 1'b0 &&
      mid_zero === 13'b1111111111111 &&
      mid_one === 12'b111111111111 &&
      mid_last === 1'b1 &&
      mid_computed === 1'b1 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b1111111111111111111111111111 &&
      wide_zero === 65'b11111111111111111111111111111111111111111111111111111111111111111 &&
      wide_one === 64'b1111111111111111111111111111111111111111111111111111111111111111 &&
      wide_last === 1'b1 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b1 &&
      one_at === 1'b0 &&
      add_before === 1'b1 &&
      masked === 8'b11111111 &&
      singleton === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      static_signed === 8'b11111100 &&
      bound === 8'b10000000)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 2.
    a = 8'b11111110;
    mid = 13'b1000000000000;
    addr = 40'b0000000000000000000000000001000000000000;
    wide = 65'b10000000000000000000000000000000000000000000000000000000000000000;
    one = 1'b1;
    #1;
    if (!(
      identity === 8'b11111110 &&
      original_xor === 8'b11111111 &&
      original_or === 8'b11111111 &&
      original_equal === 1'b0 &&
      mid_zero === 13'b1000000000000 &&
      mid_one === 12'b100000000000 &&
      mid_last === 1'b1 &&
      mid_computed === 1'b1 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b0000000000000000000000000001 &&
      wide_zero === 65'b10000000000000000000000000000000000000000000000000000000000000000 &&
      wide_one === 64'b1000000000000000000000000000000000000000000000000000000000000000 &&
      wide_last === 1'b1 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b1 &&
      one_at === 1'b0 &&
      add_before === 1'b0 &&
      masked === 8'b00000000 &&
      singleton === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      static_signed === 8'b11111100 &&
      bound === 8'b01111111)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 3.
    a = 8'b01111111;
    mid = 13'b0000000010001;
    addr = 40'b1000000000000000000000000000000000000000;
    wide = 65'b10000000000000000000000000000000000000000000000000000000000000011;
    one = 1'b0;
    #1;
    if (!(
      identity === 8'b01111111 &&
      original_xor === 8'b01111110 &&
      original_or === 8'b01111111 &&
      original_equal === 1'b0 &&
      mid_zero === 13'b0000000010001 &&
      mid_one === 12'b000000001000 &&
      mid_last === 1'b0 &&
      mid_computed === 1'b0 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b1000000000000000000000000000 &&
      wide_zero === 65'b10000000000000000000000000000000000000000000000000000000000000011 &&
      wide_one === 64'b1000000000000000000000000000000000000000000000000000000000000001 &&
      wide_last === 1'b1 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b0 &&
      one_at === 1'b0 &&
      add_before === 1'b0 &&
      masked === 8'b00000001 &&
      singleton === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      static_signed === 8'b11111100 &&
      bound === 8'b01000000)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 4.
    a = 8'b00000001;
    mid = 13'b0010000000000;
    addr = 40'b0000000000000000000000000000111111111111;
    wide = 65'b00000000000000000000000000000000000000000000000000000000000000010;
    one = 1'b1;
    #1;
    if (!(
      identity === 8'b00000001 &&
      original_xor === 8'b00000000 &&
      original_or === 8'b00000001 &&
      original_equal === 1'b0 &&
      mid_zero === 13'b0010000000000 &&
      mid_one === 12'b001000000000 &&
      mid_last === 1'b0 &&
      mid_computed === 1'b0 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b0000000000000000000000000000 &&
      wide_zero === 65'b00000000000000000000000000000000000000000000000000000000000000010 &&
      wide_one === 64'b0000000000000000000000000000000000000000000000000000000000000001 &&
      wide_last === 1'b0 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b1 &&
      one_at === 1'b0 &&
      add_before === 1'b0 &&
      masked === 8'b00000001 &&
      singleton === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      static_signed === 8'b11111100 &&
      bound === 8'b00000001)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 5.
    a = 8'b00000011;
    mid = 13'b0000000100000;
    addr = 40'b0001001000110100010101100111100010011010;
    wide = 65'b00001001000110100010101100111100010011010101111001101111011110000;
    one = 1'b0;
    #1;
    if (!(
      identity === 8'b00000011 &&
      original_xor === 8'b00000010 &&
      original_or === 8'b00000011 &&
      original_equal === 1'b0 &&
      mid_zero === 13'b0000000100000 &&
      mid_one === 12'b000000010000 &&
      mid_last === 1'b0 &&
      mid_computed === 1'b0 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b0001001000110100010101100111 &&
      wide_zero === 65'b00001001000110100010101100111100010011010101111001101111011110000 &&
      wide_one === 64'b0000100100011010001010110011110001001101010111100110111101111000 &&
      wide_last === 1'b0 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b0 &&
      one_at === 1'b0 &&
      add_before === 1'b0 &&
      masked === 8'b01111000 &&
      singleton === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      static_signed === 8'b11111100 &&
      bound === 8'b00000010)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 6.
    a = 8'b10000000;
    mid = 13'b0111111111111;
    addr = 40'b1000000000000000000000000001000000000000;
    wide = 65'b00000000000000000000000000000000000000000000000000000000000000001;
    one = 1'b1;
    #1;
    if (!(
      identity === 8'b10000000 &&
      original_xor === 8'b10000001 &&
      original_or === 8'b10000001 &&
      original_equal === 1'b0 &&
      mid_zero === 13'b0111111111111 &&
      mid_one === 12'b011111111111 &&
      mid_last === 1'b0 &&
      mid_computed === 1'b0 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b1000000000000000000000000001 &&
      wide_zero === 65'b00000000000000000000000000000000000000000000000000000000000000001 &&
      wide_one === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      wide_last === 1'b0 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b1 &&
      one_at === 1'b0 &&
      add_before === 1'b0 &&
      masked === 8'b00000000 &&
      singleton === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      static_signed === 8'b11111100 &&
      bound === 8'b01000000)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 7.
    a = 8'b00000000;
    mid = 13'b0000000000001;
    addr = 40'b0000000000000000000000000000000000000001;
    wide = 65'b00000000000000000000000000000000000000000000000000000000000000000;
    one = 1'b0;
    #1;
    if (!(
      identity === 8'b00000000 &&
      original_xor === 8'b00000001 &&
      original_or === 8'b00000001 &&
      original_equal === 1'b1 &&
      mid_zero === 13'b0000000000001 &&
      mid_one === 12'b000000000000 &&
      mid_last === 1'b0 &&
      mid_computed === 1'b0 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b0000000000000000000000000000 &&
      wide_zero === 65'b00000000000000000000000000000000000000000000000000000000000000000 &&
      wide_one === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      wide_last === 1'b0 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b0 &&
      one_at === 1'b0 &&
      add_before === 1'b0 &&
      masked === 8'b00000000 &&
      singleton === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      static_signed === 8'b11111100 &&
      bound === 8'b00000000)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    $finish;
  end
endmodule
