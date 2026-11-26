module tb;
  logic [15:0] rows=0;
  logic index=0;
  wire [7:0] joined,selected;
  wire in_range;
  pyc_root dut(.*);
  string path;
  integer fd,n,count=0;
  logic [7:0] expected_joined,expected_selected;
  logic expected_range;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    while(!$feof(fd))begin
      n=$fscanf(fd,"%b %b %b %b %b\n",rows,index,expected_joined,expected_selected,expected_range);
      if(n!=5)$fatal(1,"invalid row");
      #1;
      if(joined!==expected_joined || selected!==expected_selected || in_range!==expected_range)
        $fatal(1,"native table lookup mismatch row %0d",count);
      count=count+1;
    end
    if(count!=6)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
