module tb;
  logic [2:0] count;
  wire [7:0] result;
  pyc_root dut(.*);
  function automatic logic digit(input integer value);
    case(value)0:return 1'b0;1:return 1'b1;2:return 1'bx;3:return 1'bz;endcase
  endfunction
  function automatic logic [2:0] pattern(input integer code);
    return {digit((code/16)%4),digit((code/4)%4),digit(code%4)};
  endfunction
  function automatic logic [7:0] known_value(input integer value);
    case(value)
      0:return 8'h48;1:return 8'h49;2:return 8'h58;3:return 8'h59;
      4:return 8'h5a;5:return 8'h5b;6:return 8'h5c;7:return 8'h5d;
    endcase
  endfunction
  initial begin
    for(integer code=0;code<64;code=code+1)begin
      bit known_case;
      known_case=code%4<2 && (code/4)%4<2 && (code/16)%4<2;
`ifndef PYC_ENCODER_FOUR_STATE
      if(known_case)begin
`endif
        count=pattern(code);#1;
        if(known_case)begin
          if(result !== known_value(int'(count)))$fatal(1,"original coerced encoder known truth mapping failed");
          $display("WORK %b %b",count,result);
        end else begin
          // Every original relational comparison is unknown, including >=0.
          // Merging all six tens literals leaves 0xxx; unknown arithmetic then
          // makes units xxxx. Packing therefore leaves exactly 0xxxxxxx.
          if(result !== 8'b0xxxxxxx)$fatal(1,"original full comparison/select chain X/Z oracle failed");
          $display("MASK %b %b",count,result);
          count=3'd7;#1;if(result !== 8'h5d)$fatal(1,"pure encoder immediate known recovery failed");
        end
`ifndef PYC_ENCODER_FOUR_STATE
      end
`endif
    end
    $finish;
  end
endmodule
