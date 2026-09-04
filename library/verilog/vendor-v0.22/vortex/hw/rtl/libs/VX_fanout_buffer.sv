// Vortex combinational fanout buffer, vendored from hw/rtl/libs/VX_fanout_buffer.sv.
// Copyright (c) 2019-2023 Vortex contributors.
// SPDX-License-Identifier: Apache-2.0
`include "VX_platform.vh"

`TRACING_OFF

// Replicate a single-bit control net into N outputs.  For larger vectors the
// net is split into preserved intermediate copies, limiting each copy's load.
module VX_fanout_buffer #(
    parameter N          = 1,
    parameter MAX_FANOUT = `MAX_FANOUT
) (
    input wire          data_in,
    output wire [N-1:0] data_out
);
    if (MAX_FANOUT != 0 && N > (MAX_FANOUT + MAX_FANOUT/2)) begin : g_split
        localparam F = `UP(MAX_FANOUT);
        localparam R = (N + F - 1) / F;
        `PRESERVE_NET wire [R-1:0] buf_r;
        for (genvar i = 0; i < R; ++i) begin : g_buf
            assign buf_r[i] = data_in;
        end
        for (genvar i = 0; i < N; ++i) begin : g_out
            assign data_out[i] = buf_r[i / F];
        end
    end else begin : g_passthru
        assign data_out = {N{data_in}};
    end
endmodule

`TRACING_ON
