module tb;
  `include "bounded-for-vectors.svh"
  pyc_root dut(.*);
  initial begin
    for (integer row = 0; row < row_count; row = row + 1) begin
`ifndef BOUNDED_FOR_FOUR_STATE
      if (known_row(row)) begin
`endif
      drive(row);
      #1;
      if (result !== golden(row))
        $fatal(1, "bounded-for row %0d: got %b expected %b", row, result,
               golden(row));
      $display("WORK %0d %b", row, result);
`ifndef BOUNDED_FOR_FOUR_STATE
      end
`endif
    end
    $finish;
  end
endmodule
