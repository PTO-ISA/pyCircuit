module tb;
  logic [7:0] a=0,b=0,count=0;
  logic selector=0;
  wire [7:0] picked,left,right,echo,mirror;
  pyc_root dut(.*);
  string path;
  integer fd,n,rows=0;
  logic [7:0] expected_picked,expected_left,expected_right,expected_echo,expected_mirror;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    while(!$feof(fd))begin
      n=$fscanf(fd,"%b %b %b %b %b %b %b %b %b\n",a,b,count,selector,
                expected_picked,expected_left,expected_right,expected_echo,expected_mirror);
      if(n!=9)$fatal(1,"invalid row");
      #1;
      if(picked!==expected_picked || left!==expected_left || right!==expected_right ||
         echo!==expected_echo || mirror!==expected_mirror)
        $fatal(1,"native output group mismatch row %0d",rows);
      rows=rows+1;
    end
    if(rows!=13)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
