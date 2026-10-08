module tb;
  logic path=0,condition=0;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire value,pyc_local_error;
  pyc_root dut(.*);
  initial begin
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    for(integer p=0;p<2;p++)for(integer c=0;c<2;c++)begin
      path=1'(p);condition=1'(c);pyc_phase=1;#1;
      if(value !== 1'(c))$fatal(1,"checked module value mismatch");
      if(pyc_local_error !== 1'(p && !c))$fatal(1,"checked module error mismatch");
      if(p && !c)begin
        // A caller discards a failed Work and supplies no commit permission.
        pyc_phase=3;#1;pyc_phase=0;#1;
      end else begin
        pyc_root_commit_ok=1;pyc_phase=2;#1;
        pyc_phase=0;pyc_root_commit_ok=0;#1;
      end
      $display("MATRIX %0d%0d %0d",p,c,p && !c);
    end
    $display("MANAGED_CHECKED_MODULE_OK");$finish;
  end
endmodule
