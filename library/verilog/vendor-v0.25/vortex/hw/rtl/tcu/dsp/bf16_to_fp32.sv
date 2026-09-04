// Extracted verbatim from Vortex hw/rtl/tcu/dsp/VX_tcu_fedp_dsp.sv.
// The surrounding TCU datapath is not required by this leaf conversion.
// Copyright © 2019-2023 Vortex contributors.
// Licensed under the Apache License, Version 2.0.
module bf16_to_fp32 (
    input  wire [15:0] bf16_in,
    output wire [31:0] fp32_out
);
    wire        sign     = bf16_in[15];
    wire [7:0]  exponent = bf16_in[14:7];
    wire [6:0]  fraction = bf16_in[6:0];
    wire [7:0]  fp32_exponent = exponent;
    wire [22:0] fp32_fraction = {fraction, 16'b0};
    assign fp32_out = {sign, fp32_exponent, fp32_fraction};
endmodule
