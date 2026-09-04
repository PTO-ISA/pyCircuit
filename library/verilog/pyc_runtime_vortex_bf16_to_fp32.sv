// Canonical PYC runtime wrapper for the Vortex BF16-to-FP32 leaf conversion.
// The source module is vendored separately from the surrounding TCU datapath.
module pyc_runtime_vortex_bf16_to_fp32 (
    input  wire [15:0] bf16_in,
    output wire [31:0] fp32_out
);
    bf16_to_fp32 impl (.bf16_in(bf16_in), .fp32_out(fp32_out));
endmodule
