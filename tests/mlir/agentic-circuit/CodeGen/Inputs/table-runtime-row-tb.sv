module tb;
  reg clk = 0;
  reg rst = 1;
  reg in_valid = 0;
  reg [9:0] in_data = 0;
  reg out_ready = 1;
  wire out_valid;
  wire [4:0] out_data;
  wire in_ready;

  table_runtime_row dut(.*);
  always #5 clk = ~clk;
  task tick; begin @(posedge clk); #1; end endtask
  task request;
    input [1:0] row;
    input [7:0] tag;
    input [3:0] expected_index;
    input expected_valid;
    begin
      while (!in_ready) tick();
      in_valid = 1;
      in_data = {row, tag};
      tick();
      in_valid = 0;
      while (!out_valid) tick();
      if (out_data[4:1] !== expected_index ||
          out_data[0] !== expected_valid) $finish(1);
      tick();
    end
  endtask

  initial begin
    tick(); tick(); rst = 0;
    request(2, 32, 8, 1);
    request(1, 21, 5, 1);
    request(2, 32, 9, 1);
    request(3, 99, 0, 0);
    $display("PASS verilator table runtime row capture and update");
    $finish(0);
  end
endmodule
