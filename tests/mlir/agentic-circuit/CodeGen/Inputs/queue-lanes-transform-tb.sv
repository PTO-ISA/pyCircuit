module tb;
  reg clk = 0;
  reg rst = 1;
  reg in_valid_0 = 0;
  reg [7:0] in_data_0 = 0;
  reg in_valid_1 = 0;
  reg [7:0] in_data_1 = 0;
  reg in_valid_2 = 0;
  reg [7:0] in_data_2 = 0;
  reg out_ready = 1;
  wire out_valid_0;
  wire [7:0] out_data_0;
  wire out_valid_1;
  wire [7:0] out_data_1;
  wire out_valid_2;
  wire [7:0] out_data_2;
  wire in_ready;
  lane_transform dut(.*);
  always #5 clk = ~clk;
  task tick; begin @(posedge clk); #1; end endtask
  initial begin
    tick(); tick(); rst = 0;
    in_valid_0 = 1; in_data_0 = 10;
    in_valid_1 = 1; in_data_1 = 11;
    tick();
    in_valid_1 = 0; in_data_0 = 20;
    tick();
    if (!out_valid_0 || !out_valid_1 || out_valid_2 ||
        out_data_0 !== 11 || out_data_1 !== 12) $finish(1);
    in_valid_0 = 0;
    tick();
    if (!out_valid_0 || out_valid_1 || out_data_0 !== 21) $finish(1);
    rst = 1; tick();
    if (out_valid_0 || out_valid_1 || out_valid_2) $finish(1);
    $display("PASS lane_transform behavior");
    $finish(0);
  end
endmodule
