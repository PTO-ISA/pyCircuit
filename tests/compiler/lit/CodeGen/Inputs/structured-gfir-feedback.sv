module tb;
  logic [7:0] stimulus;
  logic permit;
  wire [7:0] forwarded, mirror, accepted;
  wire did_send;

  pyc_root dut(.*);

  task automatic check(input logic [7:0] value,
                       input logic allow,
                       input logic [7:0] expected_forwarded,
                       input logic [7:0] expected_mirror,
                       input logic expected_sent,
                       input logic [7:0] expected_acceptance);
    stimulus = value;
    permit = allow;
    #1;
    if (forwarded !== expected_forwarded || mirror !== expected_mirror ||
        did_send !== expected_sent || accepted !== expected_acceptance)
      $fatal(1, "feedback result mismatch");
  endtask

  initial begin
    check(8'h93, 1'b0, 8'h39, 8'h6c, 1'b0, 8'h00);
    check(8'h2e, 1'b1, 8'h84, 8'hd1, 1'b1, 8'h55);
    check(8'he5, 1'b0, 8'h4f, 8'h1a, 1'b0, 8'h00);
    check(8'he5, 1'b1, 8'h4f, 8'h1a, 1'b0, 8'h00);
    $finish;
  end
endmodule
