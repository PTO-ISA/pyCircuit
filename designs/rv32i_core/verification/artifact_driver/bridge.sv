module bridge;
logic [67:0] bus;
logic [0:0] pyc_7079635f636c6b;
logic [0:0] pyc_7079635f727374;
logic [417:0] result_0;
logic [2101:0] result_1;
pyc_root dut(.bus(bus),.pyc_7079635f636c6b(pyc_7079635f636c6b),.pyc_7079635f727374(pyc_7079635f727374),.result_0(result_0),.result_1(result_1));
reg [8*8-1:0] command;
reg [2101:0] before_state;
reg [417:0] saved_packet;
integer got;
initial begin
 pyc_7079635f636c6b=0;pyc_7079635f727374=1;bus=0;#10;
 while(!$feof(32'h80000000))begin
  got=$fscanf(32'h80000000,"%s",command);
  if(got==1)begin
   if(command=="S")begin $display("%b",result_1);$fflush(32'h80000001);end
   else begin
    got=$fscanf(32'h80000000,"%b",bus);
    if(got!=1)$fatal(1,"missing clocked input");
    pyc_7079635f636c6b=0;pyc_7079635f727374=(command=="R");#10;
    if(command=="Q")begin $display("%b",result_0);$fflush(32'h80000001);end
    else begin
     before_state=result_1;saved_packet=result_0;
     pyc_7079635f636c6b=1;#10;pyc_7079635f636c6b=0;#10;
     $display("%b %0d",saved_packet,((before_state===result_1) && (^before_state!==1'bx)));
     $fflush(32'h80000001);
    end
   end
  end
 end
 $finish;
end
endmodule
