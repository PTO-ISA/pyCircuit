#pragma once
#include "logic.hpp"
#include <gfsim/queue.hpp>
#include <gfsim/signal.hpp>
#include <span>

namespace ripes5 {
using gfsim::Queue;
using gfsim::Signal;
// These members are fixed connections and runtime records, never circuit state.
struct Stage {
    gfsim::Simulator &sim;
    gfsim::ModuleId mid{};
    gfsim::RuleId rid{};
};
struct Fetch : Stage {
    Queue<Word> &pc;
    Queue<Slot> &if_id;
    Signal<ExResult> &ex_result;
    Signal<bool> &load_use_stall;
    const std::span<const Word> words;
    void Work() { work_fetch(); }
    void work_fetch() {
        if (!sim.beginRule(rid))
            return;
        try {
            const auto ex = ex_result.value();
            const bool stall = load_use_stall.value();
            const Word current = pc.peek();
            if (!stall) {
                pc.proposeRevise(rid, ex.redirect ? ex.next_slot.result : current + 4);
                Slot next{};
                if (!ex.redirect) {
                    if (current % 4)
                        throw std::invalid_argument("unaligned instruction address");
                    const Word word = current / 4 < words.size() ? words[current / 4] : 0;
                    next = Slot{true, current, word};
                }
                if_id.proposeRevise(rid, next);
            }
            sim.completeRule(rid);
        } catch (const gfsim::NeedInput &) {
            sim.abortRule(rid);
        }
    }
};
struct Decode : Stage {
    Queue<Slot> &if_id, &id_ex, &mem_wb;
    const std::span<Queue<Word> *const> registers;
    Signal<ExResult> &ex_result;
    Signal<bool> &load_use_stall;
    void Work() { work_decode(); }
    void work_decode() {
        if (!sim.beginRule(rid))
            return;
        try {
            const auto ex = ex_result.value();
            const bool stall = load_use_stall.value();
            Slot next{};
            if (ex.redirect || stall)
                next.stalled = stall;
            else {
                const auto old = if_id.peek(), wb = mem_wb.peek();
                const auto ins = decode(old.word);
                const Word a = registers[ins.rs1]->peek(), b = registers[ins.rs2]->peek();
                next = Slot{old.valid, old.pc, old.word,
                            ins.rs1 && ins.rs1 == writer(wb) ? wb.value : a,
                            ins.rs2 && ins.rs2 == writer(wb) ? wb.value : b};
            }
            id_ex.proposeRevise(rid, next);
            sim.completeRule(rid);
        } catch (const gfsim::NeedInput &) {
            sim.abortRule(rid);
        }
    }
};
struct Execute : Stage {
    Queue<Slot> &ex_mem;
    Signal<ExResult> &ex_result;
    void Work() { work_execute(); }
    void work_execute() {
        if (!sim.beginRule(rid))
            return;
        try {
            ex_mem.proposeRevise(rid, ex_result.value().next_slot);
            sim.completeRule(rid);
        } catch (const gfsim::NeedInput &) {
            sim.abortRule(rid);
        }
    }
};
struct Memory : Stage {
    Queue<Slot> &ex_mem, &mem_wb;
    const std::span<Queue<Word> *const> data;
    const Word data_base;
    Queue<Store> &store;
    void Work() { work_memory(); }
    void work_memory() {
        if (!sim.beginRule(rid))
            return;
        try {
            auto mem = ex_mem.peek();
            const auto ins = decode(mem.word);
            Word value = ins.op == JAL || ins.op == JALR ? mem.pc + 4 : mem.result;
            if (ins.op == LW || ins.op == SW) {
                if (mem.result < data_base || (mem.result - data_base) % 4 ||
                    (mem.result - data_base) / 4 >= data.size())
                    throw std::invalid_argument("data access outside aligned region");
                auto &queue = *data[(mem.result - data_base) / 4];
                if (ins.op == LW)
                    value = queue.peek();
                else {
                    queue.proposeRevise(rid, mem.right);
                    store.proposeRevise(rid, Store{gfsim::checkedAdd(store.peek().sequence, 1),
                                                   mem.result, mem.right});
                }
            }
            mem.value = value;
            mem_wb.proposeRevise(rid, mem);
            sim.completeRule(rid);
        } catch (const gfsim::NeedInput &) {
            sim.abortRule(rid);
        }
    }
};
struct Writeback : Stage {
    Queue<Slot> &mem_wb;
    const std::span<Queue<Word> *const> registers;
    Queue<Event> &retirement;
    const std::size_t code_size;
    void Work() { work_writeback(); }
    void work_writeback() {
        if (!sim.beginRule(rid))
            return;
        try {
            const auto wb = mem_wb.peek();
            const Word rd = writer(wb);
            if (rd)
                registers[rd]->proposeRevise(rid, wb.value);
            if (wb.valid && wb.pc % 4 == 0 && wb.pc < code_size)
                retirement.proposeRevise(rid,
                                         Event{gfsim::checkedAdd(retirement.peek().sequence, 1),
                                               wb.pc, wb.word, rd, rd ? wb.value : 0});
            sim.completeRule(rid);
        } catch (const gfsim::NeedInput &) {
            sim.abortRule(rid);
        }
    }
};
} // namespace ripes5
