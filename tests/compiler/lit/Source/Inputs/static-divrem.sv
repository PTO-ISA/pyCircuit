// All expected values are generated with host integer divmod, including wide values.
module tb;
  `include "static-divrem-vectors.svh"
  pyc_root dut(.*);
  logic [$bits(result)-1:0] expected_value;
  initial begin
    for(integer row=0;row<KNOWN_SAMPLE_COUNT;row=row+1)begin
      drive(row); #1;
      if(result !== golden(row))begin
        expected_value = golden(row);
        for(integer bit_index=0;bit_index<$bits(result);bit_index=bit_index+1)
          if(result[bit_index] !== expected_value[bit_index])
            $fatal(1,"divrem mismatch row %0d bit %0d: got %b expected %b",
                   row,bit_index,result[bit_index],expected_value[bit_index]);
        $fatal(1,"divrem known oracle row %0d",row);
      end
      $display("WORK %0d",row);
    end
`ifdef STATIC_DIVREM_FOUR_STATE
    for(integer row=KNOWN_SAMPLE_COUNT;row<SAMPLE_COUNT;row=row+1)begin
      drive(row); #1;
      if(result !== golden(row))$fatal(1,"static divrem X/Z/recovery oracle row %0d",row);
      if((row-KNOWN_SAMPLE_COUNT)%2==0)$display("MASK %0d",row);else $display("WORK %0d",row);
    end
`endif
    $finish;
  end
endmodule
