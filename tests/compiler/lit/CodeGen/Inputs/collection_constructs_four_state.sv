module tb;
  logic [0:2][7:0] values;
  wire [0:1][0:2][7:0] nested;
  wire [0:64][6:0] ordinals;
  wire [6:0] sum;
  wire [0:2][7:0] wide_rotated,negative;
  logic [0:2][13:0] query_data;
  logic [0:4][69:0] query_indices;
  wire [0:4][13:0] query_gathered,query_reference;
  ac_top dut(.pyc_696e707574(values),.nested(nested),.ordinals(ordinals),.sum(sum),.pyc_6c61726765(wide_rotated),.negative(negative),.query_data(query_data),.query_indices(query_indices),.query_gathered(query_gathered),.query_reference(query_reference));
  initial begin
    query_data={14'h1234,14'h2bad,14'h3123};
    query_indices={70'd0,70'd1,70'd2,70'd3,70'h10000000000000000};
    values={8'd7,8'd11,8'd13};#1;
    if(nested!=={8'd7,8'd11,8'd13,8'd7,8'd11,8'd13} ||
       wide_rotated!=={8'd13,8'd7,8'd11} || negative!=={8'd11,8'd13,8'd7})
      $fatal(1,"nested splat or +/- (2^200+1) exact mathematical rotate failed");
    if(ordinals[0]!==0 || ordinals[1]!==1 || ordinals[31]!==31 || ordinals[63]!==63 || ordinals[64]!==64 || sum!==32)
      $fatal(1,"zero-input map must use known logical ordinal per lane");
    values[2]='z;#1;
    if(nested[0][2]!==8'hzz || nested[1][2]!==8'hzz || wide_rotated[0]!==8'hzz || negative[1]!==8'hzz)
      $fatal(1,"nested/rotate value conversion lost Z mask");
    if(query_gathered!==query_reference || query_gathered[0]!==query_data[0] || query_gathered[1]!==query_data[1] || query_gathered[2]!==query_data[2])
      $fatal(1,"compact gather/rawget known-index mismatch");
    query_data[1]='z;query_data[2]=14'b11x1z100101011;#1;
    if(query_gathered!==query_reference || query_gathered[1]!==14'bzzzzzzzzzzzzzz || query_gathered[2]!==query_data[2])$fatal(1,"gather nestedStruct/Enum XZ transport");
    query_data={14'h2777,14'h2777,14'h2777};
    query_indices[0]=70'd1;query_indices[0][64]=1'bx;
    query_indices[1]='x;query_indices[2]=70'd2;query_indices[2][0]=1'bz;
    query_indices[3]=70'd3;query_indices[4]=70'd1;#1;
    if(query_gathered!==query_reference || query_gathered[0]!==14'bxxxxxxxxxxxxxx || query_gathered[1]!==14'bxxxxxxxxxxxxxx || query_gathered[2]!==14'bxxxxxxxxxxxxxx || query_gathered[3]!==14'bxxxxxxxxxxxxxx || query_gathered[4]!==14'h2777)$fatal(1,"gather must poison unknown/high/OOB indices despite equal payloads");
    query_indices[0][64]=1'bz;query_indices[2][0]=1'bx;#1;
    if(query_gathered!==query_reference)$fatal(1,"gather highZ/partialX rawget mismatch");
    $finish;
  end
endmodule
