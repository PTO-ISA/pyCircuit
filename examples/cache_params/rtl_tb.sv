module tb;
  logic [39:0] addr;
  wire [27:0] tag;
  wire [8:0] line_words, tag_bits;
  wire [45:0] result;
  assign {tag, line_words, tag_bits} = result;
  pyc_root dut(.*);
  task row(input logic [39:0] value, input logic [27:0] expected);
    addr=value; #1;
    if(!(tag===expected && line_words===9'd8 && tag_bits===9'd28))
      $fatal(1,"cache_params fixed tag/constants/no-delay oracle failed");
    $display("WORK %0d 8 28",tag);
  endtask
`ifdef PYC_CACHE_FOUR_STATE
  task masks(input logic [39:0] value, input logic [27:0] expected);
    addr=value; #1;
    if(!(tag===expected && line_words===9'd8 && tag_bits===9'd28))
      $fatal(1,"cache_params exact value/known/Z plane retention failed");
    $display("MASK %b 8 28",tag);
    addr=40'd4096; #1;
    if(!(tag===28'd1 && line_words===9'd8 && tag_bits===9'd28))
      $fatal(1,"cache_params immediate known recovery failed");
  endtask
`endif
  initial begin
    row(40'd0,28'd0);
    row(40'd2048,28'd0);
    row(40'd4095,28'd0);
    row(40'd4096,28'd1);
    row(40'd4097,28'd1);
    row(40'd8191,28'd1);
    row(40'd8192,28'd2);
    row(40'd549755813888,28'd134217728);
    row(40'd1099511627775,28'd268435455);
    row(40'd78187493530,28'd19088743);
    row(40'd737894400291,28'd180150000);
    row(40'd178954240,28'd43690);
    row(40'd89477120,28'd21845);
    row(40'd0,28'd0);
`ifdef PYC_CACHE_FOUR_STATE
    masks(40'b0000000000000000000000000101xxxxxxxxxxxx,28'b0000000000000000000000000101);
    masks(40'b0000000000000000000000001001zzzzzzzzzzzz,28'b0000000000000000000000001001);
    masks(40'bx000000000000000000000000000111111111111,28'bx000000000000000000000000000);
    masks(40'bz000000000000000000000000000000000000000,28'bz000000000000000000000000000);
    masks(40'b01xz000000000000000000000000xzxzxzxzxzxz,28'b01xz000000000000000000000000);
    masks(40'bzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz,28'bzzzzzzzzzzzzzzzzzzzzzzzzzzzz);
    masks(40'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,28'bxxxxxxxxxxxxxxxxxxxxxxxxxxxx);
`endif
    $finish;
  end
endmodule
