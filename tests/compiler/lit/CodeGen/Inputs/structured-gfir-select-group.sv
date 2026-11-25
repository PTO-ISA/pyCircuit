module tb;
  logic pick;
  wire [23:0] chosen;
  pyc_root dut(.pick(pick), .yes(24'ha501zz), .no(24'ha581zz), .chosen(chosen));

  task automatic check(input logic condition, input logic [23:0] expected);
    pick = condition;
    #1;
    if (chosen !== expected)
      $fatal(1, "selection result");
  endtask

  initial begin
    check(1'b1, 24'ha501zz);
    check(1'b0, 24'ha581zz);
    check(1'bx, 24'b10100101_x0000001_zzzzzzzz);
    check(1'bz, 24'b10100101_x0000001_zzzzzzzz);
    $finish;
  end
endmodule
