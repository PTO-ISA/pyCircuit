module tb;
  logic [7:0] stimulus;
  logic permit;
  wire [7:0] forwarded, accepted;
  wire did_send;

  pyc_root dut(.*);

  task automatic check(input logic [7:0] value,
                       input logic allow,
                       input logic [7:0] expected_acceptance);
    stimulus = value;
    permit = allow;
    #1;
    if (forwarded !== value || did_send !== allow ||
        accepted !== expected_acceptance)
      $fatal(1, "feedback result mismatch");
  endtask

  initial begin
    check(8'h93, 1'b0, 8'h00);
    check(8'h2e, 1'b1, 8'h2e);
    check(8'he5, 1'b0, 8'h00);
    $finish;
  end
endmodule
