module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1;
  logic seed_valid=0,copy_valid=0,check_valid=0;
  logic [31:0] seed='0,copy='0,check='0;
  logic take_seed=1,take_copy=1,take_check=1;
  wire [109:0] result;
  wire pyc_5f5f7079635f76616c69645f6f75745f30;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.*);
  task automatic checked_prepare;
    pyc_root_commit_ok=0;pyc_phase=1;#1;
    pyc_root_commit_ok=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
    if(pyc_root_commit_ok!==1'b1)begin pyc_phase=3;#1;$fatal(1,"prepared DMA row failed completion/sampling/check validation");end
  endtask
  task automatic checked_xfer;
    pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  task automatic host_reset;
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
  endtask
  task automatic physical_reset_edges;
    pyc_7079635f636c6b=1;checked_prepare();checked_xfer();
    pyc_7079635f636c6b=0;pyc_7079635f727374=0;checked_prepare();checked_xfer();
  endtask
  integer file,status,row=0;
  integer clock_level,reset_level,sv,cv,hv,ts,tc,th;
  logic [31:0] sp,cp,hp;
  logic [109:0] expected,mask,normalized;
  initial begin
    // Match native host Reset before sampling. RAM is cold-zero initially;
    // subsequent physical reset edges cancel tokens but retain committed RAM.
    #1;host_reset();physical_reset_edges();
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
      pyc_7079635f636c6b=clock_level[0];
      // Settled old-state Work observation before the physical edge.
      #1;checked_prepare();
      if((result&mask)!==(expected&mask))
        $fatal(1,"DMA precommit row %0d expected %b mask %b actual %b",row,expected,mask,result);
      normalized=result&mask;
      $display("WORK %0d %b",row,normalized);
      checked_xfer();
      row=row+1;
    end
    $fclose(file);
    if(row<300)$fatal(1,"DMA finite stress sequence missing");
    $finish;
  end
endmodule
