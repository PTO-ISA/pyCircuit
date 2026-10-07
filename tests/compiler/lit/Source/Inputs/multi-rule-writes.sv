module tb;

`ifdef FIELD_MODE
  logic en_a=0,en_b=0;
  logic[7:0] data=0,other=0;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  wire[55:0] result;
  pyc_root dut(.*);
  integer file,status,row=0;
  integer clock_level,reset,av,ak,az,bv,bk,bz,dv,dk,dz,ov,ok,oz,action,four;
  logic[55:0] value_bits,known_bits,z_bits,value_mask,expected;
  string path;
  bit enabled;
  function automatic logic[7:0] symbols(input integer v,k,z);
    logic[7:0] bits_value;
    for(integer b=0;b<8;b=b+1)bits_value[b]=z[b]?1'bz:!k[b]?1'bx:v[b];
    return bits_value;
  endfunction
  task initialize;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
  endtask
  initial begin
    if(!$value$plusargs("rows=%s",path))$fatal(1,"field oracle rows missing");
    file=$fopen(path,"r");if(file==0)$fatal(1,"field oracle file unavailable");initialize();
    while(!$feof(file))begin
      status=$fscanf(file,"%h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h\n",
        clock_level,reset,av,ak,az,bv,bk,bz,dv,dk,dz,ov,ok,oz,action,four,value_bits,known_bits,z_bits,value_mask);
      if(status!=20)$fatal(1,"field expected row malformed %0d",status);
      enabled=action!=2&&action!=4&&action!=5;
`ifndef FOUR_STATE
      enabled=enabled&&!four;
`endif
      if(enabled)begin
      en_a=az[0]?1'bz:!ak[0]?1'bx:av[0];en_b=bz[0]?1'bz:!bk[0]?1'bx:bv[0];
      data=symbols(dv,dk,dz);other=symbols(ov,ok,oz);pyc_7079635f727374=reset[0];
      if(action==3)initialize();else begin
`ifdef FOUR_STATE
      for(integer b=0;b<56;b=b+1)expected[b]=z_bits[b]?1'bz:!known_bits[b]?1'bx:value_bits[b];
`else
      if(known_bits!==56'hffffffffffffff||z_bits!==0)$fatal(1,"unknown expected row in two-state sequence");
      expected=value_bits;
`endif
      #1;
      if(result!==expected)$fatal(1,"field mode%0d row%0d expected%b actual%b",`FIELD_MODE,row,expected,result);
      if(four)$display("MASK %b",result);else $display("WORK %b",result);
      // A native prepared discard has no committed physical edge in RTL.
      if(action!=1)pyc_7079635f636c6b=clock_level[0];
      #1;row=row+1;
      end
      end
    end
    $fclose(file);$finish;
  end
`elsif GRANT_MODE

  localparam MODE=`GRANT_MODE;
  logic en_a=0,en_b=0;
  logic[7:0] data=0,other=0;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  wire[31:0] result;
  pyc_root dut(.*);
  logic[7:0] a,b,c,side;
  bit last_clock=0;
  task initialize;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    a=MODE==2?0:7;b=MODE==2?0:17;c=MODE==2?0:29;side=9;last_clock=0;
  endtask
  task row(input bit clock_level,reset,p,q,input logic[7:0] d,o);
    bit ga,gb;int index;logic[7:0] value;
    en_a=p;en_b=q;data=d;other=o;pyc_7079635f727374=reset;#1;
    if(result!=={a,b,c,side})$fatal(1,"grant old-Q/order/effective-enable mismatch mode%0d expected%h actual%h",MODE,{a,b,c,side},result);
    $display("WORK %b",result);
    ga=p;
`ifdef GRANT_COMPLEMENT
    gb=!p;
`else
    gb=!p&&q;
`endif
    if(clock_level&&!last_clock)begin
      if(reset)begin a=MODE==2?0:7;b=MODE==2?0:17;c=MODE==2?0:29;side=9;end
      else begin
        if(MODE==0)a=a^(ga?d:gb?o:d^o);
        else if(MODE==1)begin
          if(ga)begin a=b^d;b=d;end
          else if(gb)a=b^o;
          else begin a=b^(d^o);c=d^o;end
        end else if(MODE==2)begin
          index=ga?int'(d%2):gb?int'(o%2):0;value=ga?d:gb?o:d^o;
          if(index==0)a=value;else b=value;
        end else if(MODE==5)begin if(p)a=a^o;end
        else a=a^(p?d:o);
        side=side^d;
      end
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
`ifdef FOUR_STATE
  task unknown_data(input bit z);
    logic[31:0] expected;
    initialize();en_a=1;en_b=0;data=z?8'hzz:8'hxx;other=8'h5a;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
    expected={8'hxx,8'd17,8'd29,8'hxx};
    if(MODE==1)expected={8'hxx,(z?8'hzz:8'hxx),8'd29,8'hxx};
    if(MODE==2)expected={8'hxx,8'hxx,8'd0,8'hxx};
    if(MODE==5)expected={8'h5d,8'd17,8'd29,8'hxx};
    if(result!==expected)$fatal(1,"grant unknown-data mismatch mode%0d expected%b actual%b",MODE,expected,result);
    $display("MASK %b",result);
  endtask
`endif
  initial begin
    logic[19:0] clocks;
    clocks=20'b01010101010110101010;initialize();
    for(int n=0;n<20;n++)row(clocks[n],n==12,n%4<2,n%3!=0,8'(n*37+17),8'(n*61+33));
`ifdef FOUR_STATE
    for(int z=0;z<2;z++)unknown_data(z!=0);
`endif
    $finish;
  end
`elsif TABLE_MODE
  localparam MODE=`TABLE_MODE, P=`PACKED_WIDTH;
  logic en_a=0,en_b=0;
  logic [7:0] data=0,other=0;
  logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
  wire [P-1:0] result;
  pyc_root dut(.*);
  logic [8:0] a[5],b[5];
  bit last_clock=0;
  function automatic logic [P-1:0] packed_model();
    logic [P-1:0] value;
    value=0;
    if(MODE==4) return (P'(a[0])<<24)|(P'(b[1])<<16);
    for(int i=0;i<(MODE==5?5:MODE==6?1:3);i++)
      if(MODE==5||MODE==6)value=(value<<8)|P'(a[i]);
      else if(MODE==1||MODE==7)value=(value<<5)|P'(a[i]);
      else value=(value<<14)|(P'(a[i])<<9)|P'(b[i]);
    return value;
  endfunction
  task initialize;
    pyc_7079635f636c6b=0;pyc_7079635f727374=1;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    last_clock=0;
    for(int i=0;i<5;i++)begin a[i]=0;b[i]=0;end
  endtask
  task row(input bit clock_level,reset,ea,eb,input logic[7:0] d,o);
    logic[8:0] old_a[5],old_b[5];
    en_a=ea;en_b=eb;data=d;other=o;pyc_7079635f727374=reset;#1;
    if(result!==packed_model())$fatal(1,"table mismatch mode%0d data%0d other%0d expected%b actual%b",MODE,d,o,packed_model(),result);
    $display("WORK %b",result);
    for(int i=0;i<5;i++)begin old_a[i]=a[i];old_b[i]=b[i];end
    if(clock_level&&!last_clock)begin
      if(reset)for(int i=0;i<5;i++)begin a[i]=0;b[i]=0;end
      else if(MODE==5||MODE==6)begin
        if(ea)a[d%(MODE==5?5:1)]=9'(d);
      end else if(MODE==0)begin
        if(ea)a[d%3]=(old_b[d%3]&9'd31)^9'(d&8'd31);
        if(eb)b[o%3]=old_a[o%3]^{1'b1,o};
      end else if(MODE==1)begin
        if(ea)a[0]=old_a[2]^9'(d&8'd31);
        if(eb)a[2]=old_a[0]^9'(o&8'd31);
      end else if(MODE==7)begin
        if(ea)a[0]=9'(d&8'd3);
        if(eb)a[2]=9'(o&8'd3);
      end else if(MODE==2)begin
        if(ea)begin a[0]=9'(d&8'd31);b[0]={1'b1,o};end
        if(eb)a[2]=9'(o&8'd31);
      end else if(MODE==3)begin
        if(ea)a[0]=9'(d&8'd31);
        b[0]={1'b1,o};
        if(eb)a[2]=9'(o&8'd31);
      end else begin a[0]=9'(d);b[1]=9'(d);end
    end
    last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;
  endtask
`ifdef FOUR_STATE
  task unknown_data(input bit z,condition);
    logic [P-1:0] expected;
    initialize();en_a=condition?(z?1'bz:1'bx):1'b1;en_b=0;
    data=condition?8'd31:(z?8'bzzzzzzzz:8'bxxxxxxxx);other=8'h5a;#1;
    pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;#1;
    expected=0;
    if(MODE==5||MODE==6)expected='x;
    else if(MODE==0)begin
      for(int i=0;i<3;i++)expected[14*(2-i)+9+:5]=5'bxxxxx;
    end else if(MODE==1)expected[10+:5]=5'bxxxxx;
    else if(MODE==7)expected[10+:2]=z?2'bzz:2'bxx;
    else if(MODE==4)expected[16+:16]=z?16'hzzzz:16'hxxxx;
    else begin
      expected[28+:9]=9'h15a;
      expected[37+:5]=(z&&!condition)?5'bzzzzz:5'bxxxxx;
    end
    if(result!==expected)$fatal(1,"table X/Z data/owner-enable mismatch mode%0d",MODE);
    $display("MASK %b",result);
  endtask
`endif
  initial begin
    logic [13:0] clocks;
    clocks=14'b10101101010110;initialize();
    for(int n=0;n<14;n++)row(clocks[n],n==8||n==9,n%3!=1,n%4!=2,8'(n*37+17),8'(n*61+33));
    row(0,0,1,1,8'd3,8'd5);row(1,0,1,1,8'd3,8'd5);row(0,0,1,1,8'd3,8'd5);
`ifdef FOUR_STATE
    for(int z=0;z<2;z++)begin
      unknown_data(z!=0,0);
      if(MODE==3)unknown_data(z!=0,1);
    end
`endif
    $finish;
  end
`else
  logic en_a = 0, en_b = 0;
  logic [7:0] data = 0, other = 0;
  logic pyc_7079635f636c6b = 0, pyc_7079635f727374 = 0;
  wire [31:0] result;
  pyc_root dut(.*);
  bit last_clock = 0;
  logic [7:0] a, b, c, side;

  task initialize;
    pyc_7079635f636c6b = 0; pyc_7079635f727374 = 1; #1;
    pyc_7079635f636c6b = 1; #1;
    pyc_7079635f636c6b = 0; pyc_7079635f727374 = 0; #1;
    last_clock = 0;
`ifdef TABLES
    a = 0; b = 0; c = 0; side = 0;
`else
    a = 0; b = 0; c = 92; side = 9;
`endif
  endtask
  task row(input bit clock_level, reset, ea, eb, input logic [7:0] d, o);
    logic [7:0] old_a, old_b;
    en_a = ea; en_b = eb; data = d; other = o;
    pyc_7079635f727374 = reset; #1;
    if (result !== {a,b,c,side}) $fatal(1, "old-Q/disjoint-field/order mismatch");
    $display("WORK %b", result);
    if (clock_level && !last_clock) begin
      if (reset) begin
`ifdef TABLES
        a=0; b=0; c=0; side=0;
`else
        a=0; b=0; c=92; side=9;
`endif
      end else begin
`ifdef TABLES
        a=d; b=o;
`elsif SOLO
        if (ea) a=d;
        b=o; side=side^d;
`else
        old_a=a; old_b=b;
        if (ea) a=old_b^d;
        if (eb) b=old_a^o;
        side=side^d;
`endif
      end
    end
    last_clock=clock_level; pyc_7079635f636c6b=clock_level; #1;
  endtask
`ifdef FOUR_STATE
`ifndef TABLES
  task unknown_data(input bit z);
    initialize();
    en_a=1; en_b=0; data=z ? 8'bzzzzzzzz : 8'bxxxxxxxx; other=8'h5a; #1;
    pyc_7079635f636c6b=1; #1; pyc_7079635f636c6b=0; #1;
`ifdef SOLO
    if (result !== {(z ? 8'bzzzzzzzz : 8'bxxxxxxxx),8'h5a,8'h5c,8'bxxxxxxxx})
      $fatal(1,"known rule enable rejected/corrupted unknown data");
`else
    if (result !== {8'bxxxxxxxx,8'h00,8'h5c,8'bxxxxxxxx})
      $fatal(1,"known rule enable corrupted XOR data");
`endif
    $display("MASK %b",result);
  endtask
`ifdef SOLO
  task unknown_condition(input bit z);
    initialize(); en_a=z ? 1'bz : 1'bx; en_b=0; data=8'hff; other=8'h5a; #1;
    pyc_7079635f636c6b=1; #1; pyc_7079635f636c6b=0; #1;
    if (result !== {8'bxxxxxxxx,8'h5a,8'h5c,8'hf6})
      $fatal(1,"single-rule unconditional sibling must preserve known enable");
    $display("MASK %b",result);
  endtask
`endif
`endif
`endif
  initial begin
    logic [13:0] clocks;
    clocks=14'b10101101010110;
    initialize();
    for (integer n=0; n<14; n=n+1)
      row(clocks[n],n==8||n==9,n%3!=1,n%4!=2,8'(n*37+17),8'(n*61+33));
`ifdef FOUR_STATE
`ifndef TABLES
    for (integer z=0; z<2; z=z+1) begin
      unknown_data(z!=0);
`ifdef SOLO
      unknown_condition(z!=0);
`endif
    end
`endif
`endif
    $finish;
  end
`endif
endmodule
