module tb;
  reg clk = 0;
  reg rst = 1;
  reg in_valid_0 = 0;
  reg [7:0] in_data_0 = 0;
  reg in_valid_1 = 0;
  reg [7:0] in_data_1 = 0;
  reg in_valid_2 = 0;
  reg [7:0] in_data_2 = 0;
  reg out_ready = 0;
  wire out_valid_0;
  wire [7:0] out_data_0;
  wire out_valid_1;
  wire [7:0] out_data_1;
  wire out_valid_2;
  wire [7:0] out_data_2;
  wire in_ready;

  lane_bundle dut(.*);
  always #5 clk = ~clk;

  task tick;
    begin
      @(posedge clk);
      #1;
    end
  endtask

  task expect_pair(input [7:0] a, input [7:0] b);
    begin
      if (!out_valid_0 || !out_valid_1 || out_valid_2 ||
          out_data_0 !== a || out_data_1 !== b) begin
        $display("FAIL pair valid=%b%b%b data=%0d,%0d", out_valid_2,
                 out_valid_1, out_valid_0, out_data_0, out_data_1);
        $finish(1);
      end
    end
  endtask

  initial begin
    tick();
    tick();
    rst = 0;
    if (!in_ready) $finish(1);

    in_valid_0 = 1; in_data_0 = 10;
    in_valid_1 = 1; in_data_1 = 11;
    tick();
    expect_pair(10, 11);

    in_data_0 = 20; in_data_1 = 21;
    if (!in_ready) $finish(1);
    tick();
    expect_pair(10, 11);

    in_data_0 = 30; in_data_1 = 31;
    if (in_ready) $finish(1);
    tick();
    expect_pair(10, 11);

    out_ready = 1;
    #1;
    if (!in_ready) $finish(1);
    tick();
    expect_pair(20, 21);
    in_valid_1 = 0; in_data_0 = 40;
    tick();
    expect_pair(30, 31);
    in_valid_0 = 0;
    tick();
    if (!out_valid_0 || out_valid_1 || out_valid_2 || out_data_0 !== 40)
      $finish(1);

    rst = 1;
    tick();
    if (out_valid_0 || out_valid_1 || out_valid_2) $finish(1);
    $display("PASS lane_bundle behavior");
    $finish(0);
  end
endmodule
