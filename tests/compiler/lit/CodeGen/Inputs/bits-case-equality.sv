module tb;
  logic lhs1, rhs1;
  logic [64:0] lhs65, rhs65;
  wire case1, eq1, case65, eq65;
  integer row;
  pyc_root dut(.*);

  task drive_symbol(input integer code, output logic value);
    begin
      case (code)
        0: value = 1'b0;
        1: value = 1'b1;
        2: value = 1'bx;
        default: value = 1'bz;
      endcase
    end
  endtask

  task check_wide(input integer ordinal, input logic expected_case,
                  input logic expected_eq);
    begin
      #1;
      if (case65 !== expected_case || eq65 !== expected_eq)
        $fatal(1, "wide row %0d mismatch", ordinal);
      if ((case65 !== 1'b0 && case65 !== 1'b1) || case65 === 1'bz)
        $fatal(1, "wide case equality must be known");
      $display("WIDE %0d %b %b", ordinal, case65, eq65);
    end
  endtask

  initial begin
    lhs65 = '0;
    rhs65 = '0;
    for (row = 0; row < 16; row = row + 1) begin
      drive_symbol(row / 4, lhs1);
      drive_symbol(row % 4, rhs1);
      #1;
      if (case1 !== ((row / 4) == (row % 4)))
        $fatal(1, "one-bit case row %0d mismatch", row);
      if ((row / 4) < 2 && (row % 4) < 2) begin
        if (eq1 !== ((row / 4) == (row % 4)))
          $fatal(1, "one-bit known equality row %0d mismatch", row);
      end else if (eq1 !== 1'bx) begin
        $fatal(1, "one-bit ordinary equality row %0d lost X", row);
      end
      if (case1 !== 1'b0 && case1 !== 1'b1)
        $fatal(1, "one-bit case equality must be known");
      $display("ONE %0d %b %b", row, case1, eq1);
    end

    lhs1 = 0;
    rhs1 = 0;
    lhs65 = {1'b1, 64'h0}; rhs65 = {1'b1, 64'h0};
    check_wide(0, 1'b1, 1'b1);
    lhs65 = {1'b1, 64'h0}; rhs65 = {1'b0, 64'h0};
    check_wide(1, 1'b0, 1'b0);
    lhs65 = {1'bx, 63'b0, 1'bz}; rhs65 = {1'bx, 63'b0, 1'bz};
    check_wide(2, 1'b1, 1'bx);
    lhs65 = {1'bx, 63'b0, 1'bz}; rhs65 = {1'bz, 63'b0, 1'bz};
    check_wide(3, 1'b0, 1'bx);
    lhs65 = {1'bx, 63'b0, 1'b0}; rhs65 = {1'bx, 63'b0, 1'b1};
    check_wide(4, 1'b0, 1'b0);
    $finish(0);
  end
endmodule
