`include "availability_ports.svh"
module tb;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0,allow=0;
  logic [7:0] data=0;
  wire [15:0] packet;
  wire [7:0] projected,independent,accepted;
  wire packet_valid,projected_valid;
  logic [2:0] pyc_phase=0;
  logic pyc_root_commit_ok=0;
  wire pyc_check_complete,pyc_sampling_error,pyc_local_error;
  pyc_root dut(.`PYC_PACKET_VALID(packet_valid),
               .`PYC_PROJECTED_VALID(projected_valid), .*);
  string path;
  integer fd,n,count=0;
  bit previous_clock=0,rising,checks,available,observed_checks;
  logic [7:0] ticks=0,accepted_count=0;
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"missing rows");
    fd=$fopen(path,"r");if(!fd)$fatal(1,"cannot open rows");
    pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
    pyc_phase=0;pyc_root_commit_ok=0;#1;
    while(!$feof(fd))begin
      n=$fscanf(fd,"%h %h %h %h\n",pyc_7079635f636c6b,pyc_7079635f727374,allow,data);
      if(n!=4)$fatal(1,"invalid row");
      rising=pyc_7079635f636c6b && !previous_clock;
      checks=!rising || pyc_7079635f727374 || (allow===0 || allow===1);
      available=(allow===1);
      #1;pyc_phase=1;#1;
      if(packet_valid!==available || projected_valid!==available)
        $fatal(1,"availability mismatch row %0d",count);
      if(independent!==ticks || accepted!==accepted_count || packet[7:0]!==ticks)
        $fatal(1,"independent value mismatch row %0d",count);
      if(available && (packet[15:8]!==data || projected!==data))
        $fatal(1,"available payload mismatch row %0d",count);
      observed_checks=(pyc_check_complete===1 && pyc_sampling_error===0 && pyc_local_error===0);
      if(observed_checks!==checks)$fatal(1,"check mismatch row %0d",count);
      if(checks)begin pyc_root_commit_ok=1;pyc_phase=2;#1;end
      else begin pyc_phase=3;#1;pyc_phase=2;#1;end
      pyc_phase=0;pyc_root_commit_ok=0;#1;
      if(checks)begin
        if(rising)begin
          if(pyc_7079635f727374)begin ticks=0;accepted_count=0;end
          else begin ticks=ticks+1;if(available)accepted_count=accepted_count+1;end
        end
        previous_clock=pyc_7079635f636c6b;
      end
      $display("ROW %0d %0d %0d %0d %0d",count,available,checks,ticks,accepted_count);
      count=count+1;
    end
    if(count!=20)$fatal(1,"incomplete rows");
    $fclose(fd);$finish;
  end
endmodule
