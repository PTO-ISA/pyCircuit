module tb;
  logic clk=0,rst=1,sel=0;
  logic [7:0] a=0,b=0;
  wire [7:0] y;
  wire [7:0] result;
  assign y = result;
logic[2:0] pyc_phase=0;
logic pyc_root_commit_ok=0;
wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
// Each stimulus row is one prepared Work epoch. Permission is frozen from
// the complete error channel before any physical queue/state Xfer.
task automatic checked_prepare;
 pyc_root_commit_ok=0;pyc_phase=1;#1;
 pyc_root_commit_ok=(pyc_check_complete===1'b1 && pyc_sampling_error===1'b0 && pyc_local_error===1'b0);
 if(pyc_root_commit_ok!==1'b1)begin
  pyc_phase=3;#1;
  $fatal(1,"prepared pipeline row failed completion/sampling/check validation");
 end
endtask
task automatic checked_xfer;
 pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
endtask
task automatic host_reset;
 pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
 pyc_phase=0;pyc_root_commit_ok=0;#1;
endtask
task automatic physical_reset_edges;
 clk=1;checked_prepare();checked_xfer();
 clk=0;rst=0;checked_prepare();checked_xfer();
endtask
  pyc_root dut( .pyc_phase(pyc_phase),.pyc_root_commit_ok(pyc_root_commit_ok),.pyc_check_complete(pyc_check_complete),.pyc_sampling_error(pyc_sampling_error),.pyc_local_error(pyc_local_error),.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
               .a(a),.b(b),.sel(sel),.result(result));
  task row(input logic c,r,input logic [7:0] av,bv,input logic s,
           input logic [7:0] expected);
    clk=c;rst=r;a=av;b=bv;sel=s;#1;checked_prepare();
    if(y!==expected)$fatal(1,"wire_ops old-Q/selector/clock/reset oracle failed");
    $display("WORK %0d",y);
    checked_xfer();
  endtask
  initial begin
    // Initialize the register to the same state as the C++ host Reset.
    #1;host_reset();clk=1;checked_prepare();checked_xfer();clk=0;checked_prepare();checked_xfer();rst=0;#1;
    row(0,0,3,1,1,0);row(1,0,3,1,1,0);row(0,0,3,1,1,1);
    row(1,0,170,15,0,1);row(0,0,170,15,0,165);
    row(0,0,255,0,1,165);row(1,0,255,0,1,165);row(0,0,255,0,1,0);
    row(1,0,255,255,1,0);row(0,0,255,255,1,255);
    row(1,1,255,255,1,255);row(0,1,255,255,1,0);
    row(1,0,128,1,0,0);row(0,0,128,1,0,129);
    $finish;
  end
endmodule
