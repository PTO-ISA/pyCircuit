/**
 * regfile_capi.cpp — C API wrapper for the RegisterFile RTL model.
 *
 * Build (from pyCircuit root):
 *   c++ -std=c++17 -O2 -shared -fPIC -I library \
 *       -o designs/blocks/RegisterFile/libregfile_sim.dylib \
 *       designs/blocks/RegisterFile/regfile_capi.cpp
 */
#include <cstdint>
#include <cstring>
#include <cpp/pyc_sim.hpp>
#include <cpp/pyc_tb.hpp>
#include <cpp/pyc_change_detect.hpp>

#include "generated/regfile_gen.hpp"

using pyc::cpp::Wire;
using pyc::cpp::InputFingerprint;

static constexpr unsigned NR = 10;
static constexpr unsigned NW = 5;
static constexpr unsigned PTAG_W = 8;

struct SimContext {
    pyc::gen::RegFile__p6da24dd3 dut{};
    pyc::cpp::Testbench<pyc::gen::RegFile__p6da24dd3> tb;
    uint64_t cycle = 0;

    InputFingerprint<80, 5, 40, 320> input_fp;
    bool eval_dirty = true;

    SimContext()
        : tb(dut),
          input_fp(dut.raddr_bus, dut.wen_bus, dut.waddr_bus, dut.wdata_bus) {
        tb.addClock(dut.clk, 1);
    }

    void mark_inputs_dirty() { eval_dirty = true; }

    void eval_if_dirty() {
        if (eval_dirty || input_fp.check_and_capture()) {
            dut.eval();
            eval_dirty = false;
        }
    }

    void force_eval() {
        dut.eval();
        input_fp.capture();
        eval_dirty = false;
    }
};

static void pack_raddr(SimContext *c, const uint8_t addrs[NR]) {
    uint64_t w0 = 0;
    for (unsigned i = 0; i < 8; i++)
        w0 |= (uint64_t)addrs[i] << (i * PTAG_W);
    uint64_t w1 = 0;
    for (unsigned i = 8; i < NR; i++)
        w1 |= (uint64_t)addrs[i] << ((i - 8) * PTAG_W);
    c->dut.raddr_bus.setWord(0, w0);
    c->dut.raddr_bus.setWord(1, w1);
}

static void pack_write(SimContext *c, const uint8_t wen[NW],
                       const uint8_t waddr[NW], const uint64_t wdata[NW]) {
    uint64_t wen_val = 0;
    for (unsigned i = 0; i < NW; i++)
        if (wen[i]) wen_val |= (1u << i);
    c->dut.wen_bus = Wire<5>((uint64_t)wen_val);

    uint64_t wa = 0;
    for (unsigned i = 0; i < NW; i++)
        wa |= (uint64_t)waddr[i] << (i * PTAG_W);
    c->dut.waddr_bus = Wire<40>(wa);

    for (unsigned i = 0; i < NW; i++)
        c->dut.wdata_bus.setWord(i, wdata[i]);
}

static uint64_t extract_rdata(SimContext *c, unsigned lane) {
    return c->dut.rdata_bus.word(lane);
}

extern "C" {

SimContext *rf_create()               { return new SimContext(); }
void        rf_destroy(SimContext *c) { delete c; }

void rf_reset(SimContext *c, uint64_t n) {
    c->dut.wen_bus = Wire<5>(0u);
    c->dut.raddr_bus = Wire<80>(0u);
    c->dut.waddr_bus = Wire<40>(0u);
    for (unsigned i = 0; i < NW; i++)
        c->dut.wdata_bus.setWord(i, 0);
    c->tb.reset(c->dut.rst, n, 1);
    c->force_eval();
    c->cycle = 0;
}

void rf_drive_read(SimContext *c, uint32_t lane, uint8_t addr) {
    uint64_t w = c->dut.raddr_bus.word(lane / 8);
    unsigned shift = (lane % 8) * PTAG_W;
    w &= ~((uint64_t)0xFF << shift);
    w |= (uint64_t)addr << shift;
    c->dut.raddr_bus.setWord(lane / 8, w);
    c->mark_inputs_dirty();
}

void rf_drive_write(SimContext *c, uint32_t lane, uint8_t en,
                    uint8_t addr, uint64_t data) {
    uint64_t wen_val = c->dut.wen_bus.value();
    if (en) wen_val |= (1u << lane); else wen_val &= ~(1u << lane);
    c->dut.wen_bus = Wire<5>((uint64_t)wen_val);

    uint64_t wa = c->dut.waddr_bus.value();
    unsigned shift = lane * PTAG_W;
    wa &= ~((uint64_t)0xFF << shift);
    wa |= (uint64_t)addr << shift;
    c->dut.waddr_bus = Wire<40>(wa);

    c->dut.wdata_bus.setWord(lane, data);
    c->mark_inputs_dirty();
}

void rf_tick(SimContext *c, uint64_t n) {
    c->tb.runCycles(n);
    c->cycle += n;
    c->eval_dirty = true;
}

uint64_t rf_get_rdata(SimContext *c, uint32_t lane) {
    return extract_rdata(c, lane);
}

uint64_t rf_get_cycle(SimContext *c) { return c->cycle; }

// High-performance benchmark loop with change-detection fast path.
// Inlines the clock toggling and eval to avoid Testbench dispatch overhead.
void rf_run_bench(SimContext *c, uint64_t n_cycles) {
    uint8_t raddrs[NR];
    uint8_t wen[NW] = {};
    uint8_t waddr[NW] = {};
    uint64_t wdata[NW] = {};

    auto &dut = c->dut;

    uint64_t rng = 0xDEADBEEF12345678ULL;
    auto xorshift = [&]() -> uint64_t {
        rng ^= rng << 13;
        rng ^= rng >> 7;
        rng ^= rng << 17;
        return rng;
    };

    for (uint64_t i = 0; i < n_cycles; i++) {
        // Drive random inputs
        uint64_t r = xorshift();
        for (unsigned j = 0; j < NR; j++)
            raddrs[j] = (uint8_t)((r >> (j * 2)) & 0xFF);
        pack_raddr(c, raddrs);

        r = xorshift();
        for (unsigned j = 0; j < NW; j++) {
            wen[j] = (r >> j) & 1;
            waddr[j] = (uint8_t)((r >> (8 + j * 8)) & 0xFF);
            wdata[j] = xorshift();
        }
        pack_write(c, wen, waddr, wdata);

        // Pre-posedge combinational settle
        dut.eval();

        // Posedge
        dut.clk = Wire<1>(1u);
        dut.tick();

        // Post-posedge combinational settle
        dut.eval();

        // Negedge — lightweight: just update clkPrev on all registers
        dut.clk = Wire<1>(0u);
        dut.tick();

        c->cycle++;
    }
}

// Benchmark loop with idle cycles to demonstrate change-detection benefit.
// Alternates between 'active_pct' % active cycles (random traffic) and
// idle cycles (no input changes, eval skippable).
void rf_run_bench_cd(SimContext *c, uint64_t n_cycles, uint32_t active_pct) {
    auto &dut = c->dut;
    auto &fp = c->input_fp;

    uint64_t rng = 0xDEADBEEF12345678ULL;
    auto xorshift = [&]() -> uint64_t {
        rng ^= rng << 13;
        rng ^= rng >> 7;
        rng ^= rng << 17;
        return rng;
    };

    uint64_t evals_skipped = 0;

    for (uint64_t i = 0; i < n_cycles; i++) {
        bool active = (xorshift() % 100) < active_pct;

        if (active) {
            // Drive new random inputs
            uint64_t r = xorshift();
            uint64_t w0 = 0;
            for (unsigned j = 0; j < 8; j++)
                w0 |= (uint64_t)((uint8_t)((r >> (j * 2)) & 0xFF)) << (j * PTAG_W);
            uint64_t w1 = 0;
            for (unsigned j = 8; j < NR; j++)
                w1 |= (uint64_t)((uint8_t)((r >> (j * 2)) & 0xFF)) << ((j - 8) * PTAG_W);
            dut.raddr_bus.setWord(0, w0);
            dut.raddr_bus.setWord(1, w1);

            r = xorshift();
            uint64_t wen_val = r & 0x1F;
            dut.wen_bus = Wire<5>((uint64_t)wen_val);

            uint64_t wa = 0;
            for (unsigned j = 0; j < NW; j++)
                wa |= (uint64_t)((uint8_t)((r >> (8 + j * 8)) & 0xFF)) << (j * PTAG_W);
            dut.waddr_bus = Wire<40>(wa);

            for (unsigned j = 0; j < NW; j++)
                dut.wdata_bus.setWord(j, xorshift());
        }

        // Change-detection eval: skip if inputs are identical to last capture
        if (fp.check_and_capture()) {
            dut.eval();
        } else {
            evals_skipped++;
        }

        // Posedge
        dut.clk = Wire<1>(1u);
        dut.tick();

        // Post-posedge settle (registers may have changed, must re-eval)
        dut.eval();
        fp.capture();

        // Negedge
        dut.clk = Wire<1>(0u);
        dut.tick();

        c->cycle++;
    }
}

} // extern "C"
