// Vortex Kogge-Stone adder, vendored from hw/rtl/libs/VX_ks_adder.sv.
// Copyright (c) 2019-2023 Vortex contributors.
// SPDX-License-Identifier: Apache-2.0
`include "VX_platform.vh"
`TRACING_OFF
module VX_ks_adder #(
    parameter N = 16,
    parameter BYPASS = 0
) (
    input wire [N-1:0] dataa,
    input wire [N-1:0] datab,
    input wire         cin,
    output wire [N-1:0] sum,
    output wire         cout
);
    if (BYPASS) begin : g_bypass
        assign {cout, sum} = dataa + datab + N'(cin);
    end else begin : g_KS
        localparam LEVELS = $clog2(N);
        wire [N-1:0] G [LEVELS+1];
        wire [N-1:0] P [LEVELS+1];
        for (genvar i = 0; i < N; i++) begin : g_initial_gp
            assign G[0][i] = dataa[i] & datab[i];
            assign P[0][i] = dataa[i] ^ datab[i];
        end
        for (genvar k = 1; k <= LEVELS; k++) begin : g_ks_levels
            localparam STEP = 1 << (k - 1);
            for (genvar i = 0; i < N; i++) begin : g_ks_nodes
                if (i >= STEP) begin : g_compute_gp
                    assign G[k][i] = G[k-1][i] | (P[k-1][i] & G[k-1][i-STEP]);
                    assign P[k][i] = P[k-1][i] & P[k-1][i-STEP];
                end else begin : g_passthrough_gp
                    assign G[k][i] = G[k-1][i];
                    assign P[k][i] = P[k-1][i];
                end
            end
        end
        assign sum[0] = P[0][0] ^ cin;
        for (genvar i = 1; i < N; i++) begin : g_sum
            wire carry_in_i = G[LEVELS][i-1] | (P[LEVELS][i-1] & cin);
            assign sum[i] = P[0][i] ^ carry_in_i;
        end
        assign cout = G[LEVELS][N-1] | (P[LEVELS][N-1] & cin);
    end
endmodule
`TRACING_ON
