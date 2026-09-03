/**
 * fmac_capi.cpp — C API for the BF16 FMAC RTL model.
 *
 * Build (from pyCircuit root):
 *   c++ -std=c++17 -O2 -shared -fPIC -I library -I . \
 *       -o examples/fmac/libfmac_sim.dylib examples/fmac/fmac_capi.cpp
 */
#include <cstdint>
#include <cpp/pyc_sim.hpp>
#include <cpp/pyc_tb.hpp>

#include "examples/generated/fmac/bf16_fmac_gen.hpp"

using pyc::cpp::Wire;

struct SimContext {
    pyc::gen::bf16_fmac dut{};
    pyc::cpp::Testbench<pyc::gen::bf16_fmac> tb;
    uint64_t cycle = 0;
    SimContext() : tb(dut) { tb.addClock(dut.clk, 1); }
};

extern "C" {

SimContext* fmac_create()                     { return new SimContext(); }
void        fmac_destroy(SimContext* c)       { delete c; }

void fmac_reset(SimContext* c, uint64_t n) {
    c->tb.reset(c->dut.rst, n, 1);
    c->dut.eval();
    c->cycle = 0;
}

void fmac_push(SimContext* c, uint16_t a_bf16, uint16_t b_bf16, uint32_t acc_fp32) {
    c->dut.a_in     = Wire<16>(a_bf16);
    c->dut.b_in     = Wire<16>(b_bf16);
    c->dut.acc_in   = Wire<32>(acc_fp32);
    c->dut.valid_in = Wire<1>(1u);
    c->tb.runCycles(1);
    c->cycle++;
    c->dut.valid_in = Wire<1>(0u);
}

void fmac_idle(SimContext* c, uint64_t n) {
    c->dut.valid_in = Wire<1>(0u);
    c->tb.runCycles(n);
    c->cycle += n;
}

uint32_t fmac_get_result(SimContext* c)      { return c->dut.result.value(); }
uint32_t fmac_get_result_valid(SimContext* c) { return c->dut.result_valid.value(); }
uint64_t fmac_get_cycle(SimContext* c)        { return c->cycle; }

} // extern "C"
