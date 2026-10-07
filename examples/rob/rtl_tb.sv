// Physical IR pyc_clk/pyc_rst use the existing injective generated identifier
// spelling: pyc_7079635f636c6b / pyc_7079635f727374.
module tb;
  logic pyc_7079635f636c6b=0, pyc_7079635f727374=1;
  logic allocate=0, complete=0, retire=0, flush=0;
  logic [7:0] allocate_tag=0, complete_tag=0;
  logic [1:0] complete_index=0;
  logic [15:0] complete_value=0;
  wire allocate_accepted, complete_accepted, retire_accepted;
  wire [1:0] allocated_index;
  wire [7:0] retired_tag;
  wire [15:0] retired_value;
  wire [2:0] count;
  wire [31:0] result;
  // The native struct layout uses declaration order, with the first field
  // occupying the most significant bits. Unpack only for the existing oracle.
  assign {allocate_accepted, allocated_index, complete_accepted, retire_accepted,
          retired_tag, retired_value, count} = result;
  pyc_root dut(.*);

  // Independent scoreboard: expected values only, never DUT state or outputs.
  bit valid[4], done[4];
  int tags[4], values[4], head=0, tail=0, occupancy=0;
  int ea, ec, er, ei, et, ev;
  logic [31:0] random_state=32'h6d2b79f5;

  task sample_row(input bit clock_level, a, input int tag,
                  input bit c, input int idx, ctag, val,
                  input bit r, f);
    allocate=a; allocate_tag=8'(tag); complete=c; complete_index=2'(idx);
    complete_tag=8'(ctag); complete_value=16'(val); retire=r; flush=f;
    ea=0;ec=0;er=0;ei=0;et=0;ev=0;
    if(f)begin
      for(int i=0;i<4;i++)begin valid[i]=0;done[i]=0;tags[i]=0;values[i]=0;end
      head=0;tail=0;occupancy=0;
    end else begin
      if(c && valid[idx] && tags[idx]==ctag)begin
        done[idx]=1;values[idx]=val;ec=1;
      end
      if(r && occupancy>0 && done[head])begin
        er=1;et=tags[head];ev=values[head];valid[head]=0;
        head=(head+1)%4;occupancy=occupancy-1;
      end
      if(a && occupancy<4)begin
        ea=1;ei=tail;valid[tail]=1;done[tail]=0;tags[tail]=tag;values[tail]=0;
        tail=(tail+1)%4;occupancy=occupancy+1;
      end
    end
    #1;
    if(allocate_accepted!==1'(ea) || complete_accepted!==1'(ec) ||
       retire_accepted!==1'(er) || allocated_index!==2'(ei) ||
       retired_tag!==8'(et) || retired_value!==16'(ev) || count!==3'(occupancy))
      $fatal(1,"ROB independent transaction golden failed: expected %0d %0d %0d %0d %0d %0d %0d got %0d %0d %0d %0d %0d %0d %0d",
             ea,ec,er,ei,et,ev,occupancy,allocate_accepted,complete_accepted,
             retire_accepted,allocated_index,retired_tag,retired_value,count);
    $display("WORK %0d %0d %0d %0d %0d %0d %0d",allocate_accepted,
             complete_accepted,retire_accepted,allocated_index,retired_tag,retired_value,count);
    pyc_7079635f636c6b=clock_level;#1;
  endtask
  task event_row(input bit a,input int tag,input bit c,input int idx,ctag,val,
                 input bit r,f);
    sample_row(0,0,0,0,0,0,0,0,0);
    sample_row(1,a,tag,c,idx,ctag,val,r,f);
  endtask
  initial begin
    for(int i=0;i<4;i++)begin valid[i]=0;done[i]=0;tags[i]=0;values[i]=0;end
    // Physical reset establishes the same starting state as native host Reset.
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    event_row(0,0,1,0,0,7,1,0);
    event_row(1,11,0,0,0,0,0,0);event_row(1,12,0,0,0,0,0,0);
    event_row(1,13,0,0,0,0,0,0);event_row(1,14,0,0,0,0,0,0);
    event_row(1,15,0,0,0,0,0,0);
    event_row(0,0,1,0,99,999,0,0);event_row(0,0,1,2,13,'h1333,0,0);
    event_row(0,0,0,0,0,0,1,0);event_row(0,0,1,0,11,'h1111,1,0);
    event_row(1,15,0,0,0,0,0,0);event_row(1,16,1,1,12,'h1222,1,0);
    event_row(0,0,1,1,12,'hffff,0,0);event_row(0,0,1,0,15,'h1555,1,0);
    event_row(0,0,0,0,0,0,1,0);event_row(1,17,1,3,14,'h1444,1,0);
    event_row(1,18,1,0,15,'h1555,1,0);event_row(0,0,1,1,16,'h1666,1,0);
    event_row(0,0,1,2,17,'h1777,1,0);event_row(0,0,1,3,18,'h1888,1,0);
    event_row(0,0,1,3,18,9,1,0);event_row(1,21,0,0,0,0,0,0);
    event_row(1,22,1,0,21,21,1,1);event_row(1,31,0,0,0,0,0,0);
    event_row(0,0,1,0,21,21,1,0);event_row(0,0,1,0,31,'h3131,1,0);
    event_row(0,0,0,0,0,0,0,1);
    for(int n=0;n<256;n++)begin
      random_state=random_state^(random_state<<13);
      random_state=random_state^(random_state>>17);
      random_state=random_state^(random_state<<5);
      event_row(random_state[0],int'(random_state[15:8]),random_state[1],
                int'(random_state[3:2]),int'(random_state[23:16]),
                int'(random_state[23:8]),random_state[4],random_state[6:0]==0);
    end
    event_row(0,0,0,0,0,0,0,1);
    sample_row(0,0,0,0,0,0,0,0,0);
    $finish;
  end
endmodule
