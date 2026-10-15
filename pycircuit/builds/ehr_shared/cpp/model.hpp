#pragma once
#include "runtime.hpp"
#include <set>
namespace acir_generated {
  using namespace acir_sim;
  using acir_sim::json;
  class Model {
  public:
    struct EHRShared {
      struct Storage {
        EHR<uint32_t,3> state{};
        // Rule top.storage.early#3
        template<class Context> void try_early(Context &ctx) {
          if (!(true)) return;
          // Prepare every effect before publishing state operations.
          const uint32_t ehr_write_value = 7u;
          ctx.top.storage.state.write(0,ehr_write_value);
          ctx.fired_top_storage_early=true;
        }
        // Rule top.storage.late#0
        template<class Context> void try_late(Context &ctx) {
          if (!(true)) return;
          // Prepare every effect before publishing state operations.
          const uint32_t ehr_write_value = 9u;
          ctx.top.storage.state.write(2,ehr_write_value);
          ctx.fired_top_storage_late=true;
        }
      } storage;
      Cell<uint32_t> first_seen{};
      Cell<uint32_t> second_seen{};
      // Rule top.first#1
      template<class Context> void try_first(Context &ctx) {
        if (!(std::get<1>(ctx.compute_state_view()))) return;
        // Prepare every effect before publishing state operations.
        const uint32_t write_value = std::get<0>(ctx.compute_shared());
        ctx.top.first_seen.write(write_value);
        ctx.fired_top_first=true;
      }
      // Rule top.second#2
      template<class Context> void try_second(Context &ctx) {
        if (!(std::get<1>(ctx.compute_state_view()))) return;
        // Prepare every effect before publishing state operations.
        const uint32_t shared = std::get<0>(ctx.compute_shared());
        const uint32_t write_value = (shared + 1u);
        ctx.top.second_seen.write(write_value);
        ctx.fired_top_second=true;
      }
    } top;
    std::optional<std::tuple<uint32_t,bool>> compute_state_view_cache;
    std::optional<std::tuple<uint32_t>> compute_shared_cache;
    bool fired_top_storage_early=false;
    bool fired_top_first=false;
    bool fired_top_second=false;
    bool fired_top_storage_late=false;
    Model() { reset(); }
    void reset() {
      top.first_seen={};
      top.first_seen.value=0u;
      top.second_seen={};
      top.second_seen.value=0u;
      top.storage.state={};
      top.storage.state.value=1u;
      begin_tick();
    }
    void begin_tick() {
      top.first_seen.begin();
      top.second_seen.begin();
      top.storage.state.begin();
      compute_state_view_cache.reset();
      compute_shared_cache.reset();
      fired_top_storage_early=false;
      fired_top_first=false;
      fired_top_second=false;
      fired_top_storage_late=false;
    }
    void commit() {
      top.first_seen.commit();
      top.second_seen.commit();
      top.storage.state.commit();
    }
    void tick() {
      begin_tick();
      top.storage.try_early(*this);
      top.try_first(*this);
      top.try_second(*this);
      top.storage.try_late(*this);
      commit();
    }
    void trace(std::ostream &o,uint64_t tick) const { o<<"{\"tick\":"<<tick<<",\"fire\":{";
      o<<"\"top.storage.early#3\":"; json(o,fired_top_storage_early);
      o<<',';
      o<<"\"top.first#1\":"; json(o,fired_top_first);
      o<<',';
      o<<"\"top.second#2\":"; json(o,fired_top_second);
      o<<',';
      o<<"\"top.storage.late#0\":"; json(o,fired_top_storage_late);
      o<<"},\"state\":{";
      o<<"\"top.first_seen\":"; json(o,top.first_seen.value);
      o<<',';
      o<<"\"top.second_seen\":"; json(o,top.second_seen.value);
      o<<',';
      o<<"\"top.storage.state\":"; json(o,top.storage.state.value);
      o<<"}}"; }
    const std::tuple<uint32_t,bool> &compute_state_view() {
      if (!compute_state_view_cache) {
        const uint32_t state_value = top.storage.state.read(1);
        compute_state_view_cache.emplace(std::tuple<uint32_t,bool>{state_value,true});
      }
      return *compute_state_view_cache;
    }
    const std::tuple<uint32_t> &compute_shared() {
      if (!compute_shared_cache) {
        const uint32_t state_value = std::get<0>(compute_state_view());
        const uint32_t shared = state_value;
        compute_shared_cache.emplace(std::tuple<uint32_t>{shared});
      }
      return *compute_shared_cache;
    }
  };
} // namespace acir_generated
