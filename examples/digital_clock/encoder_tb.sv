module tb;
  logic [5:0] value;
  wire [7:0] result;
  pyc_root dut(.*);
  function automatic logic digit(input integer code);
    case(code)0:return 1'b0;1:return 1'b1;2:return 1'bx;3:return 1'bz;endcase
  endfunction
  function automatic logic [5:0] pattern(input integer code);
    logic [5:0] raw;
    for(integer bit_index=0;bit_index<6;bit_index=bit_index+1)begin
      raw[bit_index]=digit(code%4);code=code/4;
    end
    return raw;
  endfunction
  initial begin
    integer known_cases;
    known_cases=0;
    for(integer code=0;code<4096;code=code+1)begin
      bit known_case;
      integer remaining;
      known_case=1;remaining=code;
      repeat(6)begin if(remaining%4>=2)known_case=0;remaining=remaining/4;end
`ifndef PYC_ENCODER_FOUR_STATE
      if(known_case)begin
`endif
        value=pattern(code);#1;
        if(known_case)begin
          if(result !== 8'((int'(value)/10)*16+int'(value)%10))$fatal(1,"DigitalClock BCD known divmod failed");
          $display("WORK %b %b",value,result);known_cases=known_cases+1;
        end else begin
          if(result !== 8'bxxxxxxxx)$fatal(1,"DigitalClock BCD arithmetic whole-X failed");
          $display("MASK %b %b",value,result);
          value=63;#1;if(result !== 8'h63)$fatal(1,"DigitalClock BCD known recovery failed");
        end
`ifndef PYC_ENCODER_FOUR_STATE
      end
`endif
    end
    if(known_cases!=64)$fatal(1,"DigitalClock BCD exhaustive known inventory incomplete");
    $finish;
  end
endmodule
