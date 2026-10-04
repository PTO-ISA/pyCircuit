#pragma once
#include "stages.hpp"
#include <array>
#include <memory>
#include <vector>

namespace ripes5 {
// Each entry is a separate registered Queue with a stable address. Allocation is
// confined to construction; the generated Module sees only a fixed reference array.
struct QueueArray {
    const std::vector<std::unique_ptr<Queue<Word>>> storage;
    const std::vector<Queue<Word> *> refs;
    static auto allocate(std::span<const Word> values) {
        std::vector<std::unique_ptr<Queue<Word>>> result;
        for (auto value : values)
            result.push_back(std::make_unique<Queue<Word>>(1, std::vector<Word>{value}, true));
        return result;
    }
    explicit QueueArray(std::span<const Word> values)
        : storage(allocate(values)), refs([&] {
              std::vector<Queue<Word> *> result;
              for (auto &q : storage)
                  result.push_back(q.get());
              return result;
          }()) {}
};
struct CPU {
    const std::vector<Word> words;
    const Word data_base;
    Queue<Word> pc{1, {0}, true};
    Queue<Slot> if_id{1, {Slot{}}, true}, id_ex{1, {Slot{}}, true}, ex_mem{1, {Slot{}}, true},
        mem_wb{1, {Slot{}}, true};
    QueueArray registers, data;
    Queue<Event> retirement{1, {Event{}}, true};
    Queue<Store> store{1, {Store{}}, true};
    Signal<ExResult> ex_result{this,
                               [](void *p) { return static_cast<CPU *>(p)->evaluate_ex_result(); }};
    Signal<bool> load_use_stall{
        this, [](void *p) { return static_cast<CPU *>(p)->evaluate_load_use_stall(); }};
    gfsim::Simulator sim;
    Fetch fetch;
    Decode decode_stage;
    Execute execute_stage;
    Memory memory;
    Writeback writeback;

    CPU(std::vector<Word> code, Word base, std::span<const Word> regs,
        std::span<const Word> initial_data, bool cache = true, bool reverse = false)
        : words(std::move(code)), data_base(base), registers(regs), data(initial_data), sim(cache),
          fetch{{sim}, pc, if_id, ex_result, load_use_stall, words},
          decode_stage{{sim}, if_id, id_ex, mem_wb, registers.refs, ex_result, load_use_stall},
          execute_stage{{sim}, ex_mem, ex_result},
          memory{{sim}, ex_mem, mem_wb, data.refs, data_base, store},
          writeback{{sim}, mem_wb, registers.refs, retirement, 4 * words.size()} {
        if (regs.size() != 32)
            throw std::invalid_argument("expected 32 registers");
        construct(reverse);
    }
    ExResult evaluate_ex_result() { return execute(id_ex.peek(), ex_mem.peek(), mem_wb.peek()); }
    bool evaluate_load_use_stall() { return loadUseStall(if_id.peek(), id_ex.peek()); }

    // The following table-shaped construction is the compiler's output boundary.
    // Work methods neither register resources nor maintain scheduling metadata.
    void construct(bool reverse) {
        struct ModuleBinding {
            Stage *record;
            void *object;
            gfsim::Simulator::Work work;
            gfsim::Simulator::Arbitrate arbitrate;
        };
        std::array<ModuleBinding, 5> modules{
            {{&fetch, &fetch, [](void *p) { static_cast<Fetch *>(p)->Work(); },
              [](void *p, auto &, auto) { return static_cast<Fetch *>(p)->arbitrate_fetch(); }},
             {&decode_stage, &decode_stage, [](void *p) { static_cast<Decode *>(p)->Work(); },
              [](void *p, auto &, auto) { return static_cast<Decode *>(p)->arbitrate_decode(); }},
             {&execute_stage, &execute_stage, [](void *p) { static_cast<Execute *>(p)->Work(); },
              [](void *p, auto &, auto) { return static_cast<Execute *>(p)->arbitrate_execute(); }},
             {&memory, &memory, [](void *p) { static_cast<Memory *>(p)->Work(); },
              [](void *p, auto &, auto) { return static_cast<Memory *>(p)->arbitrate_memory(); }},
             {&writeback, &writeback, [](void *p) { static_cast<Writeback *>(p)->Work(); },
              [](void *p, auto &, auto) {
                  return static_cast<Writeback *>(p)->arbitrate_writeback();
              }}}};
        for (std::size_t i = 0; i < modules.size(); ++i) {
            auto &m = modules[reverse ? modules.size() - 1 - i : i];
            m.record->mid = sim.addModule(m.object, m.work);
        }
        for (auto &m : modules)
            m.record->rid = sim.addRule(m.record->mid, m.arbitrate);
        for (gfsim::QueueBase *q :
             std::initializer_list<gfsim::QueueBase *>{&pc, &if_id, &id_ex, &ex_mem, &mem_wb})
            sim.addQueue(*q);
        for (auto *q : registers.refs)
            sim.addQueue(*q);
        for (auto *q : data.refs)
            sim.addQueue(*q);
        sim.addQueue(retirement);
        sim.addQueue(store);
        sim.addSignal(ex_result);
        sim.addSignal(load_use_stall);
        auto declare = [&](const Stage &m, std::initializer_list<gfsim::ResourceBase *> resources) {
            for (auto *resource : resources)
                sim.declareResource(m.mid, *resource);
        };
        declare(fetch, {&pc, &if_id, &ex_result, &load_use_stall});
        declare(decode_stage, {&if_id, &id_ex, &mem_wb, &ex_result, &load_use_stall});
        for (auto *q : registers.refs)
            sim.declareResource(decode_stage.mid, *q);
        declare(execute_stage, {&ex_mem, &ex_result});
        declare(memory, {&ex_mem, &mem_wb, &store});
        for (auto *q : data.refs)
            sim.declareResource(memory.mid, *q);
        declare(writeback, {&mem_wb, &retirement});
        for (std::size_t i = 1; i < registers.refs.size(); ++i)
            sim.declareResource(writeback.mid, *registers.refs[i]);
        for (auto *q : {&id_ex, &ex_mem, &mem_wb})
            sim.declareInput(ex_result, *q);
        for (auto *q : {&if_id, &id_ex})
            sim.declareInput(load_use_stall, *q);
        for (auto rid : {fetch.rid, decode_stage.rid}) {
            sim.declareInput(rid, ex_result);
            sim.declareInput(rid, load_use_stall);
        }
        sim.declareInput(execute_stage.rid, ex_result);
        sim.bind(fetch.rid, pc, gfsim::Revise);
        sim.bind(fetch.rid, if_id, gfsim::Revise);
        sim.bind(decode_stage.rid, id_ex, gfsim::Revise);
        sim.bind(execute_stage.rid, ex_mem, gfsim::Revise);
        sim.bind(memory.rid, mem_wb, gfsim::Revise);
        sim.bind(memory.rid, store, gfsim::Revise);
        for (auto *q : data.refs)
            sim.bind(memory.rid, *q, gfsim::Revise);
        sim.bind(writeback.rid, retirement, gfsim::Revise);
        for (std::size_t i = 1; i < registers.refs.size(); ++i)
            sim.bind(writeback.rid, *registers.refs[i], gfsim::Revise);
        sim.freeze();
    }
};
} // namespace ripes5
