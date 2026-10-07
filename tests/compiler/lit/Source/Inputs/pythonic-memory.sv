module tb;
`ifdef MEM_PROTOCOL
  localparam P=`RESULT_BITS, R=`REQUEST_BITS;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
`ifdef SHARED_PROTOCOL
  logic valid_writer=0,take_writer=0,valid_reader=0,take_reader=0;
  logic [R-1:0] writer=0,reader=0;
`else
  logic valid=0,take=0;
  logic [R-1:0] request=0;
`endif
  wire [P-1:0] result;
  pyc_root dut(.*);
  integer file,count,n=0;
  integer clock_level,reset_level,v0,t0,v1,t1;
  longint unsigned p0,p1,wanted,mask;
  string rows;
  initial begin
    if(!$value$plusargs("rows=%s",rows))$fatal(1,"missing rows");
    file=$fopen(rows,"r");if(!file)$fatal(1,"cannot open rows");
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
    while(!$feof(file))begin
      count=$fscanf(file,"%d %d %d %d %d %d %d %d %d %d\n",
                    clock_level,reset_level,v0,p0,t0,v1,p1,t1,wanted,mask);
      if(count==10)begin
        pyc_7079635f727374=1'(reset_level);
`ifdef SHARED_PROTOCOL
        valid_writer=1'(v0);writer=R'(p0);take_writer=1'(t0);
        valid_reader=1'(v1);reader=R'(p1);take_reader=1'(t1);
`else
        valid=1'(v0);request=R'(p0);take=1'(t0);
`endif
        #1;
        for(int bit_index=0;bit_index<P;bit_index++)
          if(mask[bit_index]&&result[bit_index]!==wanted[bit_index])
            $fatal(1,"protocol frame%0d bit%0d expected%0b actual%0b",n,bit_index,wanted[bit_index],result[bit_index]);
        $write("WORK %0d ",n);
        for(int bit_index=P-1;bit_index>=0;bit_index--)
          if(mask[bit_index])$write("%b",result[bit_index]);else $write("-");
        $write("\n");
        pyc_7079635f636c6b=1'(clock_level);#1;n++;
      end else if(count!=-1)$fatal(1,"malformed row");
    end
    if(n!=`MEM_FRAMES)$fatal(1,"wrong protocol row count");
    $fclose(file);$finish;
  end
`else
  localparam W=`MEM_WIDTH, A=`MEM_ADDR_WIDTH, S=(W+7)/8;
  logic ren0=0,ren1=0,wvalid=0;
  logic [A-1:0] raddr0=0,raddr1=0,waddr=0;
  logic [W-1:0] wdata=0;
  logic [S-1:0] wstrb=0;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=1;
  wire [2*W-1:0] result;
  pyc_root dut(.*);
  integer file,count,n=0;
  integer clock_level,reset_level,r0,a0,r1,a1,write_en,wa,strobes,v0,v1;
  longint unsigned word,q0,q1;
  string rows;
  initial begin
    if(!$value$plusargs("rows=%s",rows))$fatal(1,"missing rows");
    file=$fopen(rows,"r"); if(!file)$fatal(1,"cannot open rows");
    // Native host Reset invalidates Q; a physical RTL reset establishes the
    // same state for valid-window comparisons and preserves fresh-zero RAM.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
    while(!$feof(file))begin
      count=$fscanf(file,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
        clock_level,reset_level,r0,a0,r1,a1,write_en,wa,word,strobes,q0,q1,v0,v1);
      if(count==14)begin
        ren0=r0; raddr0=a0; ren1=r1; raddr1=a1; wvalid=write_en;
        waddr=wa; wdata=word; wstrb=strobes; pyc_7079635f727374=reset_level; #1;
        if(v0 && result[2*W-1:W] !== W'(q0))$fatal(1,"memory port0 row %0d",n);
        if(v1 && result[W-1:0] !== W'(q1))$fatal(1,"memory port1 row %0d",n);
        $write("WORK %0d ",n);if(v0)$write("%0d",q0);else $write("-");
        $write(" ");if(v1)$display("%0d",q1);else $display("-");
        pyc_7079635f636c6b=clock_level;#1;n=n+1;
      end else if(count!=-1)$fatal(1,"malformed row");
    end
    if(n!=`MEM_FRAMES)$fatal(1,"wrong row count");
    $fclose(file);$finish;
  end
`endif
endmodule
