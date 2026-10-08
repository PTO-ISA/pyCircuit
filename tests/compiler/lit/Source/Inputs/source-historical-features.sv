module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1;
`ifdef FEATURE_COUNTER
  logic en=0;
  wire [7:0] y;
`elsif FEATURE_TRACE
  logic [7:0] in_x=0;
  wire [7:0] y0,y1;
`else
  logic [7:0] in_a=0;
  wire [7:0] y;
`endif
  pyc_root dut(.*);
  integer file,fields,stimulus,pre0,post0,pre1,post1,n=0;
  string rows;
  task check_values(input integer first,second);
`ifdef FEATURE_TRACE
    if(y0 !== 8'(first) || y1 !== 8'(second))
      $fatal(1,"trace cycle%0d expected%0d,%0d actual%0d,%0d",n,first,second,y0,y1);
`else
    if(y !== 8'(first))$fatal(1,"cycle%0d expected%0d actual%0d",n,first,y);
`endif
  endtask
  task drive_data(input integer value);
`ifdef FEATURE_COUNTER
    en=1'(value);
`elsif FEATURE_TRACE
    in_x=8'(value);
`else
    in_a=8'(value);
`endif
  endtask
  initial begin
    if(!$value$plusargs("rows=%s",rows))$fatal(1,"missing historical rows");
    file=$fopen(rows,"r");if(!file)$fatal(1,"cannot open historical rows");
    repeat(2)begin #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;check_values(0,0);end
    pyc_7079635f727374=0;
    while(!$feof(file))begin
      fields=$fscanf(file,"%d %d %d %d %d\n",stimulus,pre0,post0,pre1,post1);
      if(fields==5)begin
        drive_data(stimulus);#1;check_values(pre0,pre1);
        pyc_7079635f636c6b=1;#1;check_values(post0,post1);
        pyc_7079635f636c6b=0;#1;
        $display("CYCLE %0d %0d %0d %0d %0d",n,pre0,post0,pre1,post1);n++;
      end else if(fields!=-1)$fatal(1,"malformed historical row");
    end
    if(n!=`FEATURE_CYCLES)$fatal(1,"wrong historical cycle count");
    drive_data(1);pyc_7079635f727374=1;#1;pyc_7079635f636c6b=1;#1;check_values(0,0);
    $display("PASS");$finish;
  end
endmodule
