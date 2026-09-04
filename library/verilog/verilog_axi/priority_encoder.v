/*
 * Copyright (c) 2014-2021 Alex Forencich
 * SPDX-License-Identifier: MIT
 *
 * Vendored from alexforencich/verilog-axi, commit
 * 516bd5dadc3365b7f9e225d2af8fe0b8d804fe53.
 */

`resetall
`timescale 1ns / 1ps
`default_nettype none

module priority_encoder #
(
    parameter WIDTH = 4,
    parameter LSB_HIGH_PRIORITY = 0
)
(
    input  wire [WIDTH-1:0]         input_unencoded,
    output wire                     output_valid,
    output wire [$clog2(WIDTH)-1:0] output_encoded,
    output wire [WIDTH-1:0]         output_unencoded
);

parameter LEVELS = WIDTH > 2 ? $clog2(WIDTH) : 1;
parameter W = 2**LEVELS;

wire [W-1:0] input_padded = {{W-WIDTH{1'b0}}, input_unencoded};
wire [W/2-1:0] stage_valid[LEVELS-1:0];
wire [W/2-1:0] stage_enc[LEVELS-1:0];

generate
    genvar l, n;
    for (n = 0; n < W/2; n = n + 1) begin : loop_in
        assign stage_valid[0][n] = |input_padded[n*2+1:n*2];
        if (LSB_HIGH_PRIORITY) begin
            assign stage_enc[0][n] = !input_padded[n*2+0];
        end else begin
            assign stage_enc[0][n] = input_padded[n*2+1];
        end
    end
    for (l = 1; l < LEVELS; l = l + 1) begin : loop_levels
        for (n = 0; n < W/(2*2**l); n = n + 1) begin : loop_compress
            assign stage_valid[l][n] = |stage_valid[l-1][n*2+1:n*2];
            if (LSB_HIGH_PRIORITY) begin
                assign stage_enc[l][(n+1)*(l+1)-1:n*(l+1)] = stage_valid[l-1][n*2+0] ? {1'b0, stage_enc[l-1][(n*2+1)*l-1:(n*2+0)*l]} : {1'b1, stage_enc[l-1][(n*2+2)*l-1:(n*2+1)*l]};
            end else begin
                assign stage_enc[l][(n+1)*(l+1)-1:n*(l+1)] = stage_valid[l-1][n*2+1] ? {1'b1, stage_enc[l-1][(n*2+2)*l-1:(n*2+1)*l]} : {1'b0, stage_enc[l-1][(n*2+1)*l-1:(n*2+0)*l]};
            end
        end
    end
endgenerate

// Only entry zero of the final reduction stage is populated.  Selecting it
// explicitly avoids width-truncation warnings in Verilator while preserving
// the upstream priority semantics.
assign output_valid = stage_valid[LEVELS-1][0];
assign output_encoded = stage_enc[LEVELS-1][LEVELS-1:0];
assign output_unencoded = 1 << output_encoded;

endmodule

`resetall
