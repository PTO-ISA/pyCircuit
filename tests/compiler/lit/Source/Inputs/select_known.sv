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
    a=8'b00000000;
    b=8'b00000000;
    narrow=5'b00000;
    wide=65'b00000000000000000000000000000000000000000000000000000000000000000;
    flag=1'b0;
    left=1'b0;
    right=1'b1;
    #1;
    if(!(constant_right===10'b1111111111 &&
         constant_left===10'b0000000001 &&
         masked_right===8'b11111111 &&
         masked_left===8'b00000001 &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000000000000000 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000000000000 &&
         after_select===8'b00000001 &&
         narrow_selected===8'b00000000 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'b00000000 &&
         after_constant_left===8'b00000001 &&
         literal_true===9'b000000001 &&
         literal_false===9'b000000000 &&
         boolean_true===1'b0 &&
         boolean_false===1'b1 &&
         boolean_dynamic===1'b0 &&
         nested_right===8'b11111111 && nested_left===8'b11111111))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 1.
    a=8'b11111111;
    b=8'b00000000;
    narrow=5'b11111;
    wide=65'b11111111111111111111111111111111111111111111111111111111111111111;
    flag=1'b1;
    left=1'b1;
    right=1'b0;
    #1;
    if(!(constant_right===10'b0100000000 &&
         constant_left===10'b1111111111 &&
         masked_right===8'b00000000 &&
         masked_left===8'b11111111 &&
         raw_right===65'b11111111111111111111111111111111111111111111111111111111111111111 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000011111111 &&
         after_select===8'b00000000 &&
         narrow_selected===8'b11111111 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'b00000000 &&
         after_constant_left===8'b00000000 &&
         literal_true===9'b100000000 &&
         literal_false===9'b000000000 &&
         boolean_true===1'b1 &&
         boolean_false===1'b0 &&
         boolean_dynamic===1'b1 &&
         nested_right===8'b00000000 && nested_left===8'b00000000))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 2.
    a=8'b11111110;
    b=8'b11111111;
    narrow=5'b10001;
    wide=65'b10000000000000000000000000000000000000000000000000000000000000000;
    flag=1'b0;
    left=1'b0;
    right=1'b0;
    #1;
    if(!(constant_right===10'b1111111111 &&
         constant_left===10'b0011111111 &&
         masked_right===8'b11111111 &&
         masked_left===8'b11111111 &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000000011111110 &&
         raw_left===65'b10000000000000000000000000000000000000000000000000000000000000000 &&
         after_select===8'b11111111 &&
         narrow_selected===8'b00010001 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'b00000000 &&
         after_constant_left===8'b11111111 &&
         literal_true===9'b011111111 &&
         literal_false===9'b011111111 &&
         boolean_true===1'b0 &&
         boolean_false===1'b0 &&
         boolean_dynamic===1'b0 &&
         nested_right===8'b11111111 && nested_left===8'b11111111))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 3.
    a=8'b00000000;
    b=8'b11111111;
    narrow=5'b11111;
    wide=65'b10000000000000000000000000000000000000000000000000000000000000001;
    flag=1'b1;
    left=1'b1;
    right=1'b1;
    #1;
    if(!(constant_right===10'b0000000001 &&
         constant_left===10'b1111111111 &&
         masked_right===8'b00000001 &&
         masked_left===8'b11111111 &&
         raw_right===65'b10000000000000000000000000000000000000000000000000000000000000001 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000000000000 &&
         after_select===8'b00000010 &&
         narrow_selected===8'b00000000 &&
         selected_signed===8'b00000001 &&
         after_constant_right===8'b00000001 &&
         after_constant_left===8'b00000000 &&
         literal_true===9'b000000001 &&
         literal_false===9'b011111111 &&
         boolean_true===1'b1 &&
         boolean_false===1'b1 &&
         boolean_dynamic===1'b1 &&
         nested_right===8'b00000001 && nested_left===8'b11111111))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 4.
    a=8'b01111111;
    b=8'b10000000;
    narrow=5'b00001;
    wide=65'b11111111111111111111111111111111111111111111111111111111111111110;
    flag=1'b1;
    left=1'b0;
    right=1'b1;
    #1;
    if(!(constant_right===10'b0010000000 &&
         constant_left===10'b1111111111 &&
         masked_right===8'b10000000 &&
         masked_left===8'b11111111 &&
         raw_right===65'b11111111111111111111111111111111111111111111111111111111111111110 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000001111111 &&
         after_select===8'b11111111 &&
         narrow_selected===8'b01111111 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'b10000000 &&
         after_constant_left===8'b00000000 &&
         literal_true===9'b010000000 &&
         literal_false===9'b010000000 &&
         boolean_true===1'b0 &&
         boolean_false===1'b1 &&
         boolean_dynamic===1'b1 &&
         nested_right===8'b11111111 && nested_left===8'b10000000))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 5.
    a=8'b00011111;
    b=8'b00010001;
    narrow=5'b11110;
    wide=65'b00000000000000000000000000000000000000000000000000000000000010001;
    flag=1'b0;
    left=1'b1;
    right=1'b0;
    #1;
    if(!(constant_right===10'b1111111111 &&
         constant_left===10'b0000100000 &&
         masked_right===8'b11111111 &&
         masked_left===8'b00100000 &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000000000011111 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000000010001 &&
         after_select===8'b00100000 &&
         narrow_selected===8'b00011110 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'b00000000 &&
         after_constant_left===8'b00100000 &&
         literal_true===9'b000100000 &&
         literal_false===9'b000010001 &&
         boolean_true===1'b1 &&
         boolean_false===1'b0 &&
         boolean_dynamic===1'b0 &&
         nested_right===8'b11111111 && nested_left===8'b00010001))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 6.
    a=8'b00000011;
    b=8'b00000100;
    narrow=5'b00111;
    wide=65'b00000000000000000000000000000000000000000000000000000001010111100;
    flag=1'b1;
    left=1'b1;
    right=1'b0;
    #1;
    if(!(constant_right===10'b0000000100 &&
         constant_left===10'b1111111111 &&
         masked_right===8'b00000100 &&
         masked_left===8'b11111111 &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000001010111100 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000000000000011 &&
         after_select===8'b10111101 &&
         narrow_selected===8'b00000011 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'b00000100 &&
         after_constant_left===8'b00000000 &&
         literal_true===9'b000000100 &&
         literal_false===9'b000000100 &&
         boolean_true===1'b1 &&
         boolean_false===1'b0 &&
         boolean_dynamic===1'b1 &&
         nested_right===8'b00000100 && nested_left===8'b00000100))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    // Fixed independent frame 7.
    a=8'b11001000;
    b=8'b01100100;
    narrow=5'b01111;
    wide=65'b00000000000000000000000000000000000000000000000000000111111111111;
    flag=1'b0;
    left=1'b0;
    right=1'b1;
    #1;
    if(!(constant_right===10'b1111111111 &&
         constant_left===10'b0011001001 &&
         masked_right===8'b11111111 &&
         masked_left===8'b11001001 &&
         raw_right===65'b00000000000000000000000000000000000000000000000000000000011001000 &&
         raw_left===65'b00000000000000000000000000000000000000000000000000000111111111111 &&
         after_select===8'b11001001 &&
         narrow_selected===8'b00001111 &&
         selected_signed===8'b11111111 &&
         after_constant_right===8'b00000000 &&
         after_constant_left===8'b11001001 &&
         literal_true===9'b011001001 &&
         literal_false===9'b001100100 &&
         boolean_true===1'b0 &&
         boolean_false===1'b1 &&
         boolean_dynamic===1'b0 &&
         nested_right===8'b11111111 && nested_left===8'b11111111))$fatal(1,"exact select golden failed");
    $display("WORK %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b",constant_right,constant_left,masked_right,masked_left,raw_right,raw_left,after_select,narrow_selected,selected_signed,after_constant_right,after_constant_left,literal_true,literal_false,boolean_true,boolean_false,boolean_dynamic,nested_right,nested_left);
    $finish;
  end
endmodule
