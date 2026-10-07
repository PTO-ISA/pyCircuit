module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1;
  logic seed_valid=0,copy_valid=0,check_valid=0;
  logic [31:0] seed='0,copy='0,check='0;
  logic take_seed=1,take_copy=1,take_check=1;
  wire [109:0] result;
  pyc_root dut(.*);
  integer file,status,row=0;
  integer clock_level,reset_level,sv,cv,hv,ts,tc,th;
  logic [31:0] sp,cp,hp;
  logic [109:0] expected,mask,normalized;
  initial begin
    // Match native host Reset before sampling. RAM is cold-zero initially;
    // subsequent physical reset edges cancel tokens but retain committed RAM.
    #1;pyc_7079635f636c6b=1;#1;
    pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    // Created in the verifier build cwd by the independent C++ oracle. These
    // are expected rows, never native DUT results; RTL checks actual outputs.
    file=$fopen("dma-oracle.rows","r");
    if(file==0)$fatal(1,"missing independently generated DMA oracle rows");
    while(!$feof(file)) begin
      status=$fscanf(file,"%d %d %d %d %d %h %h %h %d %d %d %b %b\n",
        clock_level,reset_level,sv,cv,hv,sp,cp,hp,ts,tc,th,expected,mask);
      if(status!=13)$fatal(1,"DMA row %0d malformed expected row (%0d)",row,status);
      pyc_7079635f727374=reset_level[0];
      seed_valid=sv[0];copy_valid=cv[0];check_valid=hv[0];
      seed=sp;copy=cp;check=hp;take_seed=ts[0];take_copy=tc[0];take_check=th[0];
      // Settled old-state Work observation before the physical edge.
      #1;
      if((result&mask)!==(expected&mask))
        $fatal(1,"DMA precommit row %0d expected %b mask %b actual %b",row,expected,mask,result);
      normalized=result&mask;
      $display("WORK %0d %b",row,normalized);
      pyc_7079635f636c6b=clock_level[0];#1;
      row=row+1;
    end
    $fclose(file);
    if(row<300)$fatal(1,"DMA finite stress sequence missing");
    $finish;
  end
endmodule
