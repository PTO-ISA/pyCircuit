module tb;
localparam bit IS_JOIN=1;
localparam integer KNOWN_COUNT=4325,FOUR_COUNT=536;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=1,left_valid=0,right_valid=0;
logic[63:0] left_data='0,right_data='0;
logic take=0;
wire[66:0] result;
pyc_root dut(.*);
wire actual_lr=result[66],actual_rr=result[65],actual_lv=result[64],actual_rv=0;wire[63:0]actual_ld=result[63:0],actual_rd=0;
logic[63:0] li[0:1],ri[0:1],lo[0:0],ro[0:0],history_l[0:9999],history_r[0:9999];
integer history_l_id[0:9999],history_r_id[0:9999];
integer nli=0,nri=0,nlo=0,nro=0,row_index=0,hl=0,tl=0,hr=0,tr=0;
integer al=0,ar=0,retl=0,retr=0,dl=0,dr=0,pl=0,pr=0,peakl=0,peakr=0,physical_peak=0,contributor_peak=0,full_resets=0,repl=0,repr=0,stalll=0,stallr=0,last_id_l=-1,last_id_r=-1;
bit last_clock=0;
function automatic logic[63:0] sum(input logic[63:0]a,b);
logic[63:0] value;bit known,carry;
known=1;carry=0;value='0;
for(integer i=0;i<64;i=i+1)if((a[i]!==1'b0&&a[i]!==1'b1)||(b[i]!==1'b0&&b[i]!==1'b1))known=0;
for(integer i=0;i<64;i=i+1)begin
 value[i]=known?(a[i]^b[i]^carry):1'bx;
 carry=(a[i]&&b[i])||(carry&&(a[i]^b[i]));
end
return value;
endfunction
function automatic logic[63:0] transformed(input logic[63:0]value,input bit right);
logic[63:0] computed;bit known;
if(!right)return sum(value,64'd1);
known=1;computed='0;
for(integer i=0;i<64;i=i+1)if(value[i]!==1'b0&&value[i]!==1'b1)known=0;
// Arithmetic multiplication, never partial-X shift transport, even for bit63.
for(integer i=0;i<64;i=i+1)computed[i]=known?(i==0?1'b0:value[i-1]):1'bx;
return computed;
endfunction
function automatic logic[63:0] carry_value(input integer length);
return length==64?64'hffffffffffffffff:(64'd1<<length)-64'd1;
endfunction
function automatic logic[63:0] onehot(input integer position);
return position==64?64'd0:64'd1<<position;
endfunction
function automatic logic[63:0] boundary(input integer index);
case(index)0:return 64'd0;1:return 64'hffffffffffffffff;2:return 64'hfffffffffffffffe;
3:return 64'h7fffffffffffffff;4:return 64'h8000000000000000;5:return 64'h8000000000000001;default:return 64'd0;endcase
endfunction
function automatic logic[63:0] random_value(input integer index);
logic[63:0] state;state=64'h41479;
for(integer i=0;i<=index;i=i+1)state=state*64'd6364136223846793005+64'd1442695040888963407;
return state;
endfunction
function automatic logic[63:0] known_scalar(input integer index);
if(index<65)return carry_value(index);
if(index<129)return onehot(index-65);
if(index<135)return boundary(index-129);
if(index==135)return 64'h5555555555555555;
if(index==136)return 64'haaaaaaaaaaaaaaaa;
return random_value(index-137);
endfunction
function automatic logic[127:0] known_pair(input integer index);
integer ordinal;logic[63:0] a,b;
if(!IS_JOIN)return {known_scalar(index),known_scalar((index*47)%201)};
if(index<4225)begin a=carry_value(index/65);b=onehot(index%65);end
else if(index<4261)begin ordinal=index-4225;a=boundary(ordinal/6);b=boundary(ordinal%6);end
else begin ordinal=index-4261;a=random_value(ordinal);b=random_value(63-ordinal);end
return {a,b};
endfunction
`ifdef PYC_MULTI_INPUT_RULE_PIPELINE_FOUR_STATE
function automatic logic[63:0] dense(input integer pattern);
logic[63:0] value;
for(integer i=0;i<64;i=i+1)case(pattern)
0:value[63-i]=1'bx;1:value[63-i]=1'bz;
2:value[63-i]=i%2?1'bz:1'bx;
3:case(i%4)0:value[63-i]=0;1:value[63-i]=1;2:value[63-i]=1'bx;3:value[63-i]=1'bz;endcase
endcase
return value;
endfunction
function automatic logic[127:0] four_pair(input integer index);
integer owner,b,symbol,ordinal,pattern,low;logic[127:0] p;
if(index<512)begin owner=index/256;b=(index%256)/4;symbol=(index%4)/2;
p={64'ha5a5a5a55a5a5a5a,64'h0123456789abcdef};low=64-owner*64;p[low+b]=symbol==0?1'bx:1'bz;
end else if(index<528)begin ordinal=index-512;owner=ordinal/8;pattern=(ordinal%8)/2;
p={64'hffffffffffffffff,64'h8000000000000000};low=64-owner*64;p[low+:64]=dense(pattern);
end else begin ordinal=(index-528)/2;case(ordinal)
0:p={dense(0),dense(1)};1:p={dense(1),dense(0)};2:p={dense(2),dense(3)};3:p={dense(3),dense(2)};endcase
end
return p;
endfunction
`endif
function automatic logic[127:0] vector_value(input integer index,input bit four);
if(four)begin
`ifdef PYC_MULTI_INPUT_RULE_PIPELINE_FOUR_STATE
return four_pair(index);
`else
$fatal(1,"four-state sequence requires genuine four-state build");return '0;
`endif
end
return known_pair(index);
endfunction
task row(input bit clock_level,reset_level,lv,input logic[63:0]ld,input bit rv,input logic[63:0]rd,input bit lt,rt,four);
bit lp,rp,lr,rr,la,ra,fire,ready_l,ready_r,full_l,full_r;
integer physical;logic[63:0] expected_l,expected_r,old_l,old_r;
lp=nlo>0&&lt;lr=nlo==0||lp;rp=nro>0&&rt;rr=nro==0||rp;
fire=nli>0&&nri>0&&lr;la=nli>0&&lr;ra=nri>0&&rr;
ready_l=IS_JOIN?(nli<2||fire):(nli==0||la);
ready_r=IS_JOIN?(nri<2||fire):(nri==0||ra);
expected_l=nlo>0?lo[0]:64'd0;expected_r=nro>0?ro[0]:64'd0;
pyc_7079635f727374=reset_level;left_valid=lv;right_valid=rv;left_data=ld;right_data=rd;
take=lt;
#1;
if(result!=={ready_l,ready_r,(nlo>0),expected_l})$fatal(1,"multi_input_rule_pipeline row %0d old-state oracle",row_index);
if(four)$display("FOUR %0d %0d %0d %0d %b",row_index,actual_lr,actual_rr,actual_lv,expected_l);
else $display("WORK %0d %0d %0d %0d %b",row_index,actual_lr,actual_rr,actual_lv,expected_l);
stalll=stalll+(lv&&!actual_lr);stallr=stallr+(rv&&!actual_rr);
physical=pl+pr-(IS_JOIN&&actual_lv?1:0);if(physical>physical_peak)physical_peak=physical;
if(physical>(IS_JOIN?5:4))$fatal(1,"physical capacity");
if(clock_level&&!last_clock)begin
 if(reset_level)begin
  full_resets=full_resets+(physical==(IS_JOIN?5:4));dl=dl+pl;dr=dr+pr;pl=0;pr=0;hl=0;tl=0;hr=0;tr=0;nli=0;nri=0;nlo=0;nro=0;
 end else begin
  full_l=pl==(IS_JOIN?3:2);full_r=pr==(IS_JOIN?3:2);
  if(actual_lv&&lt)begin
   if(pl==0||history_l_id[hl]<=last_id_l)$fatal(1,"left identity order");last_id_l=history_l_id[hl];
   if(IS_JOIN)begin
    if(pr==0||history_r_id[hr]<=last_id_r||actual_ld!==sum(history_l[hl],history_r[hr]))$fatal(1,"paired identities/sum");
    last_id_r=history_r_id[hr];hr=hr+1;pr=pr-1;retr=retr+1;
   end else if(actual_ld!==transformed(history_l[hl],0))$fatal(1,"left identity payload");
   hl=hl+1;pl=pl-1;retl=retl+1;
  end
  if(!IS_JOIN&&actual_rv&&rt)begin
   if(pr==0||history_r_id[hr]<=last_id_r||actual_rd!==transformed(history_r[hr],1))$fatal(1,"right identity payload/order");
   last_id_r=history_r_id[hr];hr=hr+1;pr=pr-1;retr=retr+1;
  end
  if(actual_lr&&lv)begin history_l[tl]=ld;history_l_id[tl]=al;tl=tl+1;al=al+1;pl=pl+1;repl=repl+full_l;end
  if(actual_rr&&rv)begin history_r[tr]=rd;history_r_id[tr]=ar;tr=tr+1;ar=ar+1;pr=pr+1;repr=repr+full_r;end
  old_l=li[0];old_r=ri[0];
  if(lp)nlo=0;
  if(IS_JOIN)begin
   if(fire)begin lo[0]=sum(old_l,old_r);nlo=1;li[0]=li[1];ri[0]=ri[1];nli=nli-1;nri=nri-1;end
  end else begin
   if(rp)nro=0;
   if(la)begin lo[0]=transformed(old_l,0);nlo=1;nli=0;end
   if(ra)begin ro[0]=transformed(old_r,1);nro=1;nri=0;end
  end
  if(ready_l&&lv)begin li[nli]=ld;nli=nli+1;end
  if(ready_r&&rv)begin ri[nri]=rd;nri=nri+1;end
 end
end
if(pl!=nli+nlo||pr!=nri+(IS_JOIN?nlo:nro)||al!=retl+dl+pl||ar!=retr+dr+pr)$fatal(1,"per-stream identity conservation");
if(pl>peakl)peakl=pl;if(pr>peakr)peakr=pr;if(pl+pr>contributor_peak)contributor_peak=pl+pr;
last_clock=clock_level;pyc_7079635f636c6b=clock_level;#1;row_index=row_index+1;
endtask
 task edge_row(input logic[127:0]p,input bit lv=1,rv=1,lt=1,rt=1,r=0,four=0);
row(1,r,lv,p[127:64],rv,p[63:0],lt,rt,four);row(0,r,lv,p[127:64],rv,p[63:0],lt,rt,four);endtask
 task sequence_rows(input bit four);
integer vector_count,fill;logic[127:0] p,startup;
row_index=0;hl=0;tl=0;hr=0;tr=0;al=0;ar=0;retl=0;retr=0;dl=0;dr=0;pl=0;pr=0;peakl=0;peakr=0;physical_peak=0;contributor_peak=0;full_resets=0;repl=0;repr=0;stalll=0;stallr=0;last_id_l=-1;last_id_r=-1;nli=0;nri=0;nlo=0;nro=0;last_clock=0;
vector_count=four?FOUR_COUNT:KNOWN_COUNT;fill=IS_JOIN?3:2;
startup={64'hffffffffffffffff,IS_JOIN?64'd1:64'h8000000000000000};
row(0,1,0,0,0,0,0,0,four);edge_row(0,0,0,0,0,1,four);edge_row(startup,1,1,1,1,0,four);edge_row(0,0,0,1,1,0,four);edge_row(0,0,0,1,1,0,four);
if(IS_JOIN)begin
for(integer i=0;i<3;i=i+1)edge_row(vector_value(i,four),1,0,1,1,0,four);
for(integer i=0;i<2;i=i+1)edge_row(vector_value(i+3,four),0,1,1,1,0,four);
for(integer i=0;i<2;i=i+1)edge_row(0,0,0,1,1,0,four);
for(integer i=0;i<3;i=i+1)edge_row(vector_value(i+5,four),0,1,1,1,0,four);
for(integer i=0;i<2;i=i+1)edge_row(vector_value(i+8,four),1,0,1,1,0,four);
for(integer i=0;i<2;i=i+1)edge_row(0,0,0,1,1,0,four);
end else begin
for(integer i=0;i<8;i=i+1)edge_row(vector_value(i,four),1,0,1,1,0,four);
for(integer i=0;i<8;i=i+1)edge_row(vector_value(i+8,four),0,1,1,1,0,four);
for(integer i=0;i<6;i=i+1)edge_row(vector_value(i,four),1,1,0,1,0,four);
for(integer i=0;i<6;i=i+1)edge_row(vector_value(i+6,four),1,1,1,0,0,four);
for(integer i=0;i<3;i=i+1)edge_row(0,0,0,1,1,0,four);
end
for(integer i=0;i<fill+1;i=i+1)edge_row(vector_value(i,four),1,1,0,0,0,four);
p=vector_value(4,four);row(1,0,1,p[127:64],1,p[63:0],0,0,four);
p=vector_value(5,four);row(1,1,1,p[127:64],1,p[63:0],1,1,four);
p=vector_value(6,four);row(0,0,1,p[127:64],1,p[63:0],1,0,four);
p=vector_value(7,four);row(0,0,1,p[127:64],1,p[63:0],0,1,four);
edge_row(0,1,1,1,1,1,four);
for(integer i=0;i<fill;i=i+1)edge_row(vector_value(i,four),1,1,0,0,0,four);
for(integer i=0;i<6;i=i+1)edge_row(vector_value(i+3,four),1,1,1,1,0,four);
for(integer i=0;i<vector_count;i=i+1)edge_row(vector_value(i,four),1,1,1,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(0,0,0,1,1,0,four);
for(integer i=0;i<fill;i=i+1)edge_row(vector_value(i,four),1,1,0,0,0,four);
edge_row(0,1,1,1,1,1,four);edge_row(startup,1,1,1,1,0,four);edge_row(0,1,1,1,1,0,four);
for(integer i=0;i<5;i=i+1)edge_row(0,0,0,1,1,0,four);
if(row_index!=2*vector_count+(IS_JOIN?101:129))$fatal(1,"finite recipe rows");
if(pl!=0||pr!=0||physical_peak!=(IS_JOIN?5:4)||contributor_peak!=(IS_JOIN?6:4)||full_resets!=2||stalll<3||stallr<3||repl<6||repr<6||peakl!=(IS_JOIN?3:2)||peakr!=(IS_JOIN?3:2)||dl!=(IS_JOIN?6:4)||dr!=(IS_JOIN?6:4))$fatal(1,"complete finite coverage");
if(retl!=retr)$fatal(1,"paired retirement");
if(four)$display("HISTORY_FOUR_RTL %0d %0d %0d %0d %0d %0d %0d %0d %0d",al,ar,retl,dl,dr,pl,pr,physical_peak,contributor_peak);
else $display("HISTORY_RTL %0d %0d %0d %0d %0d %0d %0d %0d %0d",al,ar,retl,dl,dr,pl,pr,physical_peak,contributor_peak);
endtask
initial begin
#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(0);
`ifdef PYC_MULTI_INPUT_RULE_PIPELINE_FOUR_STATE
pyc_7079635f727374=1;left_valid=0;right_valid=0;#1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
sequence_rows(1);
`endif
$finish;end
endmodule
