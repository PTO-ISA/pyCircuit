module tb;
  logic [7:0] a;
  logic [7:0] b;
  logic [4:0] narrow;
  logic [64:0] wide;
  logic [0:0] flag;
  logic [0:0] left;
  logic [0:0] right;
  wire [9:0] constant_right;
  wire [9:0] constant_left;
  wire [7:0] masked_right;
  wire [7:0] masked_left;
  wire [64:0] raw_right;
  wire [64:0] raw_left;
  wire [7:0] after_select;
  wire [7:0] narrow_selected;
  wire [7:0] selected_signed;
  wire [7:0] after_constant_right;
  wire [7:0] after_constant_left;
  wire [8:0] literal_true;
  wire [8:0] literal_false;
  wire [0:0] boolean_true;
  wire [0:0] boolean_false;
  wire [0:0] boolean_dynamic;
  wire [7:0] nested_right,nested_left;
  pyc_root dut(.*);
  initial begin
    // Fixed independent frame 0.
    a=8'b11111111;
    b=8'bz0000001;
    narrow=5'b11111;
    wide=65'bz0000000000000000000000000000000000000000000000000000000000000101;
    flag=1'b1;
    left=1'bz;
    right=1'bx;
    #1;
    if(!(constant_right===10'b0100000000 &&
         constant_left===10'b1111111111 &&
         masked_right===8'b00000000 &&
         masked_left===8'b11111111 &&
         raw_right===65'bz0000000000000000000000000000000000000000000000000000000000000101 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000011111111 &&
         after_select===8'bxxxxxxxx &&
         narrow_selected===8'b11111111 &&
         selected_signed===8'bxxxxxxxx &&
         after_constant_right===8'b00000000 &&
         after_constant_left===8'b00000000 &&
         literal_true===9'b100000000 &&
         literal_false===9'b0z0000001 &&
         boolean_true===1'bz &&
         boolean_false===1'bx &&
         boolean_dynamic===1'b1 &&
         nested_right===8'bxxxxxxxx && nested_left===8'bxxxxxxxx))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 1.
    a=8'b00000011;
    b=8'bx0000001;
    narrow=5'b00111;
    wide=65'bz0000000000000000000000000000000000000000000000000000000000000101;
    flag=1'b0;
    left=1'bx;
    right=1'bz;
    #1;
    if(!(constant_right===10'b1111111111 &&
         constant_left===10'b0000000100 &&
         masked_right===8'b11111111 &&
         masked_left===8'b00000100 &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000000000000011 &&
         raw_left===65'bz0000000000000000000000000000000000000000000000000000000000000101 &&
         after_select===8'b00000100 &&
         narrow_selected===8'b00000111 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'b00000000 &&
         after_constant_left===8'b00000100 &&
         literal_true===9'b000000100 &&
         literal_false===9'b0x0000001 &&
         boolean_true===1'bx &&
         boolean_false===1'bz &&
         boolean_dynamic===1'b0 &&
         nested_right===8'bxxxxxxx1 && nested_left===8'bxxxxxxx1))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 2.
    a=8'b11111111;
    b=8'b00000000;
    narrow=5'b11111;
    wide=65'b00000000000000000000000000000000000000000000000000000000011111111;
    flag=1'bx;
    left=1'b1;
    right=1'b0;
    #1;
    if(!(constant_right===10'bx1xxxxxxxx &&
         constant_left===10'bx1xxxxxxxx &&
         masked_right===8'bxxxxxxxx &&
         masked_left===8'bxxxxxxxx &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000000011111111 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000011111111 &&
         after_select===8'b00000000 &&
         narrow_selected===8'bxxx11111 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'bxxxxxxxx &&
         after_constant_left===8'bxxxxxxxx &&
         literal_true===9'b100000000 &&
         literal_false===9'b000000000 &&
         boolean_true===1'b1 &&
         boolean_false===1'b0 &&
         boolean_dynamic===1'bx &&
         nested_right===8'bxxxxxxxx && nested_left===8'b00000000))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 3.
    a=8'b00000011;
    b=8'b00000001;
    narrow=5'b00011;
    wide=65'b00000000000000000000000000000000000000000000000000000000000000011;
    flag=1'bz;
    left=1'bz;
    right=1'bz;
    #1;
    if(!(constant_right===10'bxxxxxxx1xx &&
         constant_left===10'bxxxxxxx1xx &&
         masked_right===8'bxxxxx1xx &&
         masked_left===8'bxxxxx1xx &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000000000000011 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000000000011 &&
         after_select===8'b00000100 &&
         narrow_selected===8'b00000011 &&
         selected_signed===8'bxxxxxx1x &&
         after_constant_right===8'bxxxxxxxx &&
         after_constant_left===8'bxxxxxxxx &&
         literal_true===9'b000000100 &&
         literal_false===9'b000000001 &&
         boolean_true===1'bz &&
         boolean_false===1'bz &&
         boolean_dynamic===1'bx &&
         nested_right===8'bxxxxxxxx && nested_left===8'bxxxxxxxx))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 4.
    a=8'b0000000z;
    b=8'b00000000;
    narrow=5'b00001;
    wide=65'b00000000000000000000000000000000000000000000000000000000000000000;
    flag=1'b1;
    left=1'b0;
    right=1'b1;
    #1;
    if(!(constant_right===10'b0xxxxxxxxx &&
         constant_left===10'b1111111111 &&
         masked_right===8'bxxxxxxxx &&
         masked_left===8'b11111111 &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000000000000000 &&
         raw_left===65'b0000000000000000000000000000000000000000000000000000000000000000z &&
         after_select===8'b00000001 &&
         narrow_selected===8'b0000000z &&
         selected_signed===8'bxxxxxxxx &&
         after_constant_right===8'bxxxxxxxx &&
         after_constant_left===8'b00000000 &&
         literal_true===9'bxxxxxxxxx &&
         literal_false===9'b000000000 &&
         boolean_true===1'b0 &&
         boolean_false===1'b1 &&
         boolean_dynamic===1'b1 &&
         nested_right===8'b11111111 && nested_left===8'bxxxxxxxx))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 5.
    a=8'b0000000z;
    b=8'b00000000;
    narrow=5'b0000z;
    wide=65'b0000000000000000000000000000000000000000000000000000000000000000z;
    flag=1'bx;
    left=1'bz;
    right=1'bz;
    #1;
    if(!(constant_right===10'bxxxxxxxxxx &&
         constant_left===10'bxxxxxxxxxx &&
         masked_right===8'bxxxxxxxx &&
         masked_left===8'bxxxxxxxx &&
         raw_right===65'b0000000000000000000000000000000000000000000000000000000000000000z &&
         raw_left===65'b0000000000000000000000000000000000000000000000000000000000000000z &&
         after_select===8'bxxxxxxxx &&
         narrow_selected===8'b0000000z &&
         selected_signed===8'bxxxxxxxx &&
         after_constant_right===8'bxxxxxxxx &&
         after_constant_left===8'bxxxxxxxx &&
         literal_true===9'bxxxxxxxxx &&
         literal_false===9'b000000000 &&
         boolean_true===1'bz &&
         boolean_false===1'bz &&
         boolean_dynamic===1'bx &&
         nested_right===8'bxxxxxxxx && nested_left===8'bxxxxxxxx))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    $finish;
  end
endmodule
