// Vendored from BaseJump STL bsg_misc/bsg_imul_iterative.sv.
// SPDX-License-Identifier: Solderpad-Hardware-License-0.51
`include "bsg_defines.sv"

module bsg_imul_iterative  #( width_p = 32)
    (input                  clk_i
    ,input                  reset_i
    ,input                  v_i
    ,output                 ready_and_o
    ,input [width_p-1: 0]   opA_i
    ,input                  signed_opA_i
    ,input [width_p-1: 0]   opB_i
    ,input                  signed_opB_i
    ,input                  gets_high_part_i
    ,output                 v_o
    ,output [width_p-1: 0]   result_o
    ,input                  yumi_i
    );

    localparam lg_width_lp = `BSG_SAFE_CLOG2( width_p + 1);
    logic[lg_width_lp-1:0] shift_counter_r;
    logic gets_high_part_r;
    wire shift_counter_full = gets_high_part_r
            ? ( shift_counter_r == (width_p-1) )
            : ( shift_counter_r ==  width_p    );

    typedef enum logic[2:0] {IDLE, NEG_A, NEG_B, CALC, NEG_R, DONE } imul_ctrl_stat;
    imul_ctrl_stat curr_state_r, next_state;

    always_ff@( posedge clk_i ) begin
        if( reset_i ) curr_state_r <= IDLE;
        else curr_state_r <= next_state;
    end

    always_comb begin
        unique case(curr_state_r )
            IDLE: if( v_i ) next_state = NEG_A; else next_state = IDLE;
            NEG_A: next_state = NEG_B;
            NEG_B: next_state = CALC;
            CALC: if( !shift_counter_full ) next_state = CALC; else next_state = NEG_R;
            NEG_R: next_state = DONE;
            DONE: if( yumi_i ) next_state = IDLE; else next_state = DONE;
            default: next_state = IDLE;
        endcase
    end

    always_ff@( posedge clk_i )  begin
        if ( reset_i ) shift_counter_r <= 'b0;
        else if( curr_state_r != CALC  && next_state == CALC ) shift_counter_r <= 'b0;
        else if( curr_state_r == CALC) shift_counter_r <= shift_counter_r + 1;
    end

    logic [width_p-1:0] opA_r, opB_r, result_r;
    logic [width_p-1:0] adder_a, adder_b;
    logic [width_p  :0] adder_result,shifted_adder_result;
    assign adder_a = (curr_state_r == NEG_A) ? ~opA_r :
                     (curr_state_r == NEG_B) ? ~opB_r :
                     (curr_state_r == NEG_R) ? ~result_r : result_r;
    wire adder_neg_op = (curr_state_r == NEG_A || curr_state_r == NEG_B || curr_state_r == NEG_R);
    assign adder_b = adder_neg_op ? { {(width_p-1){1'b0}}, 1'b1} : opA_r;
    assign adder_result = {1'b0, adder_a} + {1'b0, adder_b};
    assign shifted_adder_result = adder_result >> 1;

    wire latch_input = v_i & ready_and_o;
    logic signed_opA_r, signed_opB_r, need_neg_result_r;
    wire signed_opA = signed_opA_i & opA_i[width_p-1];
    wire signed_opB = signed_opB_i & opB_i[width_p-1];

    always_ff@(posedge clk_i ) begin
      if( reset_i ) signed_opA_r <= 1'b0; else if( latch_input ) signed_opA_r <= signed_opA;
    end
    always_ff@(posedge clk_i ) begin
      if( reset_i ) signed_opB_r <= 1'b0; else if( latch_input ) signed_opB_r <= signed_opB;
    end
    always_ff@(posedge clk_i ) begin
      if( reset_i ) need_neg_result_r <= 1'b0; else if( latch_input ) need_neg_result_r <= signed_opA ^ signed_opB;
    end
    always_ff@(posedge clk_i ) begin
      if( reset_i ) gets_high_part_r <= 1'b0; else if( latch_input ) gets_high_part_r <= gets_high_part_i;
    end

    always_ff@(posedge clk_i) begin
      if( reset_i ) opA_r <= 'b0;
      else if( latch_input ) opA_r <= opA_i;
      else if(curr_state_r == CALC  && (!gets_high_part_r ) ) opA_r <= opA_r << 1 ;
      else if(curr_state_r == NEG_A && signed_opA_r) opA_r <= adder_result[width_p-1:0];
    end
    always_ff@(posedge clk_i) begin
      if( reset_i ) opB_r <= 'b0;
      else if( latch_input ) opB_r <= opB_i;
      else if(curr_state_r == CALC) opB_r <= opB_r >> 1 ;
      else if(curr_state_r == NEG_B && signed_opB_r) opB_r <= adder_result[width_p-1:0];
    end

    wire shifted_lsb = opB_r[0] ? adder_result[0] : result_r[0];
    logic all_sh_lsb_zero_r;
    always_ff@(posedge clk_i ) begin
      if( reset_i ) all_sh_lsb_zero_r <= 1'b0;
      else if( latch_input ) all_sh_lsb_zero_r <= 1'b1;
      else if( curr_state_r == CALC ) all_sh_lsb_zero_r <= all_sh_lsb_zero_r & (~shifted_lsb);
    end

    always_ff@(posedge clk_i) begin
      if( reset_i ) result_r <= 'b0;
      else if( latch_input ) result_r <= 'b0;
      else if(curr_state_r == NEG_R && need_neg_result_r)
        if( gets_high_part_r && !all_sh_lsb_zero_r ) result_r <= ~result_r;
        else result_r <= adder_result[width_p-1:0];
      else if(curr_state_r == CALC && opB_r[0]) begin
        if( gets_high_part_r ) result_r <= shifted_adder_result[width_p-1:0];
        else result_r <= adder_result[width_p-1:0];
      end else if(curr_state_r == CALC && !opB_r[0]) begin
        if( gets_high_part_r ) result_r <= result_r >>1 ;
      end
    end

    assign ready_and_o = ( curr_state_r == IDLE );
    assign result_o = result_r;
    assign v_o = ( curr_state_r == DONE );
endmodule
