module tb;
  logic [31:0] seed;
  wire [31:0] acc;
  wire [31:0] result;
  assign acc = result;
  pyc_root dut(.*);
  task row(input logic [31:0] value,expected);
    seed=value;#1;
    if(acc!==expected)$fatal(1,"boundary_value_ports fixed modular/no-delay golden failed");
    $display("WORK %0d",acc);
  endtask
`ifdef PYC_BOUNDARY_FOUR_STATE
  task unknown_and_recover(input logic [31:0] value,recovery,expected);
    seed=value;#1;
    if(!(acc===32'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx))
      $fatal(1,"boundary_value_ports arithmetic X/Z propagation failed");
    seed=recovery;#1;
    if(acc!==expected)$fatal(1,"boundary_value_ports immediate recovery without reset failed");
  endtask
`endif
  initial begin
    row(32'd10,32'd48);row(32'd0,32'd18);row(32'd1,32'd21);
    row(32'd4294967283,32'd4294967275);row(32'd4294967284,32'd4294967278);
    row(32'd4294967289,32'd4294967293);row(32'd4294967290,32'd0);
    row(32'd4294967291,32'd3);row(32'd4294967294,32'd12);row(32'd4294967295,32'd15);
    row(32'd2147483648,32'd2147483666);row(32'd2147483647,32'd2147483663);
    row(32'd1073741824,32'd3221225490);row(32'd1431655765,32'd17);
    row(32'd2863311530,32'd16);row(32'd17,32'd69);
`ifdef PYC_BOUNDARY_FOUR_STATE
    unknown_and_recover(32'b0000000000000000000000000000000x,32'd10,32'd48);
    unknown_and_recover(32'bz0000000000000000000000000000000,32'd4294967295,32'd15);
    unknown_and_recover(32'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,32'd10,32'd48);
    unknown_and_recover(32'bzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz,32'd4294967295,32'd15);
`endif
    $finish;
  end
endmodule
