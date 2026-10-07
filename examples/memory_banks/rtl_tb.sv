module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  logic valid=0,take=1;
  logic [30:0] request='0;
  wire [52:0] result;
  pyc_root dut(.*);
  integer file,status,row=0,clock_level,reset_level,valid_level,take_level;
  logic [30:0] packet;
  logic [52:0] expected,mask,normalized;
  initial begin
    #1;pyc_7079635f636c6b=1;#1;
    pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    // The existing verifier runs in its build cwd. These expected rows come
    // from the independent RAM/queue oracle, never from native DUT output.
    file=$fopen("memory-banks-oracle.rows","r");
    if(file==0)$fatal(1,"missing independent memory-banks expected rows");
    while(!$feof(file)) begin
      status=$fscanf(file,"%d %d %d %h %d %b %b\n",clock_level,reset_level,valid_level,packet,take_level,expected,mask);
      if(status!=7)$fatal(1,"memory_banks row %0d malformed expected row (%0d)",row,status);
      pyc_7079635f727374=reset_level[0];valid=valid_level[0];request=packet;take=take_level[0];
      // Compare settled Work before committing the physical edge.
      #1;
      if((result&mask)!==(expected&mask))
        $fatal(1,"memory_banks precommit row %0d expected %b mask %b actual %b",row,expected,mask,result);
      normalized=result&mask;$display("WORK %0d %b",row,normalized);
      pyc_7079635f636c6b=clock_level[0];#1;row=row+1;
    end
    $fclose(file);if(row<400)$fatal(1,"memory_banks finite stress rows missing");$finish;
  end
endmodule
