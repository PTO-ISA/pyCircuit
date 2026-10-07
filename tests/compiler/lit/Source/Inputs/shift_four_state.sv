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
    a = 8'b11111111;
    mid = 13'b100000000000z;
    addr = 40'b1111111111111111111111111111xxxxxxxxxxxx;
    wide = 65'bz000000000000000000000000000000000000000000000000000000000000000z;
    one = 1'bz;
    #1;
    if (!(
      identity === 8'b11111111 &&
      original_xor === 8'b11111110 &&
      original_or === 8'b11111111 &&
      original_equal === 1'b0 &&
      mid_zero === 13'b100000000000z &&
      mid_one === 12'b100000000000 &&
      mid_last === 1'b1 &&
      mid_computed === 1'b1 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b1111111111111111111111111111 &&
      wide_zero === 65'bz000000000000000000000000000000000000000000000000000000000000000z &&
      wide_one === 64'bz000000000000000000000000000000000000000000000000000000000000000 &&
      wide_last === 1'bz &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'bz &&
      one_at === 1'b0 &&
      add_before === 1'b1 &&
      masked === 8'b00000000 &&
      singleton === 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      static_signed === 8'b11111100 &&
      bound === 8'b10000000)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 1.
    a = 8'b0000000z;
    mid = 13'b100000000000x;
    addr = 40'b0000000000000000000000000001zzzzzzzzzzzz;
    wide = 65'bx000000000000000000000000000000000000000000000000000000000000000x;
    one = 1'bx;
    #1;
    if (!(
      identity === 8'b0000000z &&
      original_xor === 8'b0000000x &&
      original_or === 8'b00000001 &&
      original_equal === 1'bx &&
      mid_zero === 13'b100000000000x &&
      mid_one === 12'b100000000000 &&
      mid_last === 1'b1 &&
      mid_computed === 1'b1 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b0000000000000000000000000001 &&
      wide_zero === 65'bx000000000000000000000000000000000000000000000000000000000000000x &&
      wide_one === 64'bx000000000000000000000000000000000000000000000000000000000000000 &&
      wide_last === 1'bx &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'bx &&
      one_at === 1'b0 &&
      add_before === 1'bx &&
      masked === 8'b00000000 &&
      singleton === 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      static_signed === 8'b11111100 &&
      bound === 8'bxxxxxxxx)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 2.
    a = 8'bz0000001;
    mid = 13'bz000000000000;
    addr = 40'bz000000000000000000000000000111111111111;
    wide = 65'b1zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz1;
    one = 1'b1;
    #1;
    if (!(
      identity === 8'bz0000001 &&
      original_xor === 8'bx0000000 &&
      original_or === 8'bx0000001 &&
      original_equal === 1'b0 &&
      mid_zero === 13'bz000000000000 &&
      mid_one === 12'bz00000000000 &&
      mid_last === 1'bz &&
      mid_computed === 1'bz &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'bz000000000000000000000000000 &&
      wide_zero === 65'b1zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz1 &&
      wide_one === 64'b1zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz &&
      wide_last === 1'b1 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b1 &&
      one_at === 1'b0 &&
      add_before === 1'bx &&
      masked === 8'bxxxxxxxx &&
      singleton === 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      static_signed === 8'b11111100 &&
      bound === 8'bxxxxxxxx)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 3.
    a = 8'bx0000000;
    mid = 13'bx111111111111;
    addr = 40'bx111111111111111111111111111000000000000;
    wide = 65'b0xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0;
    one = 1'b0;
    #1;
    if (!(
      identity === 8'bx0000000 &&
      original_xor === 8'bx0000001 &&
      original_or === 8'bx0000001 &&
      original_equal === 1'bx &&
      mid_zero === 13'bx111111111111 &&
      mid_one === 12'bx11111111111 &&
      mid_last === 1'bx &&
      mid_computed === 1'bx &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'bx111111111111111111111111111 &&
      wide_zero === 65'b0xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0 &&
      wide_one === 64'b0xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      wide_last === 1'b0 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b0 &&
      one_at === 1'b0 &&
      add_before === 1'bx &&
      masked === 8'bxxxxxxxx &&
      singleton === 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      static_signed === 8'b11111100 &&
      bound === 8'bxxxxxxxx)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 4.
    a = 8'bxxxxxxxx;
    mid = 13'bzzzzzzzzzzzzz;
    addr = 40'bzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz;
    wide = 65'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;
    one = 1'bz;
    #1;
    if (!(
      identity === 8'bxxxxxxxx &&
      original_xor === 8'bxxxxxxxx &&
      original_or === 8'bxxxxxxx1 &&
      original_equal === 1'bx &&
      mid_zero === 13'bzzzzzzzzzzzzz &&
      mid_one === 12'bzzzzzzzzzzzz &&
      mid_last === 1'bz &&
      mid_computed === 1'bz &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'bzzzzzzzzzzzzzzzzzzzzzzzzzzzz &&
      wide_zero === 65'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      wide_one === 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      wide_last === 1'bx &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'bz &&
      one_at === 1'b0 &&
      add_before === 1'bx &&
      masked === 8'bxxxxxxxx &&
      singleton === 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      static_signed === 8'b11111100 &&
      bound === 8'bxxxxxxxx)) $fatal(1, "right shift golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",identity,original_xor,original_or,original_equal,mid_zero,mid_one,mid_last,mid_computed,mid_at,mid_above,mid_huge,tag,wide_zero,wide_one,wide_last,wide_at,wide_above,wide_huge,one_zero,one_at,add_before,masked,singleton,static_signed,bound);
    // Independent fixed frame 5.
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
    // Independent fixed frame 6.
    a = 8'b00000000;
    mid = 13'b000000000000z;
    addr = 40'b0000000000000000000000000000zzzzzzzzzzzz;
    wide = 65'b0000000000000000000000000000000000000000000000000000000000000000z;
    one = 1'b0;
    #1;
    if (!(
      identity === 8'b00000000 &&
      original_xor === 8'b00000001 &&
      original_or === 8'b00000001 &&
      original_equal === 1'b1 &&
      mid_zero === 13'b000000000000z &&
      mid_one === 12'b000000000000 &&
      mid_last === 1'b0 &&
      mid_computed === 1'b0 &&
      mid_at === 1'b0 &&
      mid_above === 1'b0 &&
      mid_huge === 1'b0 &&
      tag === 28'b0000000000000000000000000000 &&
      wide_zero === 65'b0000000000000000000000000000000000000000000000000000000000000000z &&
      wide_one === 64'b0000000000000000000000000000000000000000000000000000000000000000 &&
      wide_last === 1'b0 &&
      wide_at === 1'b0 &&
      wide_above === 1'b0 &&
      wide_huge === 1'b0 &&
      one_zero === 1'b0 &&
      one_at === 1'b0 &&
      add_before === 1'b0 &&
      masked === 8'b00000000 &&
      singleton === 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx &&
      static_signed === 8'b11111100 &&
      bound === 8'b00000000)) $fatal(1, "right shift golden failed");
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
