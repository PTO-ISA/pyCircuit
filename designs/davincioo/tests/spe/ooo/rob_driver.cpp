// Expected-result driver for generated single or dual per-flow ROBs.
#include "gfsim/replay_session.h"
#include "model.cpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace ac_generated;
#ifdef DAVINCIOO_SINGLE_ROB
using Model = RobSystem;
constexpr unsigned Flows = 1;
#else
using Model = DualRobSystem;
constexpr unsigned Flows = 2;
#endif
constexpr unsigned Inputs = 4 * Flows;
constexpr unsigned Queues = 6 * Flows;
constexpr unsigned StateBase = Queues + 5;
constexpr size_t N =
    std::tuple_size_v<decltype(std::declval<Model>().dispatch_rows())>;
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x))                                                                  \
      throw std::runtime_error("line " + std::to_string(__LINE__) + ": " #x);  \
  } while (false)

class Clock final : public gfsim::SimObject {
  gfsim::SimSystem &system;
  bool scan;

public:
  Clock(gfsim::SimSystem &s, bool full)
      : SimObject(gfsim::ObjectKind::Process, "clock", N), system(s),
        scan(full) {}
  void doWork(gfsim::Epoch e) override {
    CHECK(e.time < 10000);
    if (scan)
      for (unsigned id = 0; id < N; ++id)
        CHECK(system.scheduleWork(id, {e.time + 1, 0}));
    CHECK(system.scheduleWork(N, {e.time + 1, 0}));
  }
};
struct Lane {
  Model model;
  gfsim::SimSystem system{"rob"};
  Clock clock;
  std::array<gfsim::DispatchRow, N + 1> rows;
  std::unique_ptr<gfsim::ReplaySession> replay;
  std::vector<uint32_t> ao, co;
  const decltype(Model::activation_targets()) at = Model::activation_targets();
  const decltype(Model::work_closure_targets()) ct =
      Model::work_closure_targets();
  Lane(bool scan) : clock(system, scan) {
    auto base = model.dispatch_rows();
    std::copy(base.begin(), base.end(), rows.begin());
    rows.back() = gfsim::makeDispatchRow(&clock);
    // Reset is complete before observation registration and recording.
    for (auto &row : base)
      row.reset(row.object);
    CHECK(system.setDispatchTable(rows));
    system.setBuildProfile(gfsim::BuildProfile::Validated);
    if (scan) {
      for (unsigned id = 0; id < N; ++id)
        CHECK(system.scheduleWork(id, {0, 0}));
    } else {
      auto a = Model::activation_offsets();
      auto c = Model::work_closure_offsets();
      ao.assign(a.begin(), a.end());
      co.assign(c.begin(), c.end());
      ao.push_back(ao.back());
      co.push_back(co.back());
      CHECK(system.setActivationPlan(ao, at));
      CHECK(system.setWorkClosurePlan(co, ct));
      CHECK(Model::schedule_initial_work(system));
    }
    CHECK(system.scheduleWork(N, {0, 0}));
  }
  void record(const std::string &path) {
    replay =
        std::make_unique<gfsim::ReplaySession>(path, std::span(rows).first(N));
    model.registerObservations(*replay);
    replay->start();
    replay->attach(system);
  }
  template <class T> T &object(unsigned id) {
    return *static_cast<T *>(
        static_cast<gfsim::SimObject *>(rows.at(id).object));
  }
  template <class T> bool offer(unsigned id, const T &value) {
    auto &queue = object<gfsim::SimQueue<T>>(id);
    return queue.canProposePush() && system.scheduleExternalXfer(id) &&
           queue.proposePush(value);
  }
  template <unsigned W> uint64_t scalar(unsigned id) {
    return object<gfsim::SimTable<gfsim::UInt<W>>>(id).at(0).value();
  }
  unsigned count(unsigned side = 0) {
    return scalar<5>(StateBase + 2 + 12 * side);
  }
  unsigned epoch(unsigned side = 0) {
    return scalar<16>(StateBase + 3 + 12 * side);
  }
  bool pending(unsigned side = 0) {
    return scalar<1>(StateBase + 4 + 12 * side);
  }
  bool recovering(unsigned side = 0) {
    return scalar<1>(StateBase + 5 + 12 * side);
  }
  const RobEvent &entry(unsigned slot, unsigned side = 0) {
    return object<gfsim::SimTable<RobEvent>>(StateBase + 6 + 12 * side)
        .at(slot);
  }
  template <class T> gfsim::ReplayValue queue(unsigned id) {
    auto &q = object<gfsim::SimQueue<T>>(id);
    return gfsim::ReplayValue::Array{gfsim::replayValue(q.committedValues()),
                                     gfsim::replayValue(q.delayedValues()),
                                     gfsim::replayValue(q.totalPushes()),
                                     gfsim::replayValue(q.totalPops())};
  }
  // Full registered Queue/Table projection, sampled independently of
  // ReplaySession.
  gfsim::ReplayValue projection() {
    gfsim::ReplayValue::Array result;
    for (unsigned id = 0; id < Queues; ++id) {
      if (id < Inputs && id % 4 == 2)
        result.push_back(queue<RobCompletion>(id));
      else if (id < Inputs && id % 4 == 3)
        result.push_back(queue<RobHandoff>(id));
      else
        result.push_back(queue<RobEvent>(id));
    }
    for (unsigned side = 0; side < Flows; ++side) {
      auto base = StateBase + 12 * side;
      result.push_back(gfsim::replayValue(scalar<4>(base)));
      result.push_back(gfsim::replayValue(scalar<4>(base + 1)));
      result.push_back(gfsim::replayValue(count(side)));
      result.push_back(gfsim::replayValue(epoch(side)));
      result.push_back(gfsim::replayValue(pending(side)));
      result.push_back(gfsim::replayValue(recovering(side)));
      for (unsigned slot = 0; slot < 16; ++slot)
        result.push_back(gfsim::replayValue(entry(slot, side)));
    }
    return result;
  }
};
// Exact typed encoding for the independent per-boundary parity artifact.
void writeProjection(std::ostream &out, const gfsim::ReplayValue &value) {
  out << value.data.index() << ':';
  std::visit(
      [&](const auto &v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, gfsim::ReplayValue::Array>) {
          out << v.size() << '[';
          for (const auto &e : v)
            writeProjection(out, e);
          out << ']';
        } else if constexpr (std::is_same_v<T, gfsim::ReplayValue::Object>) {
          out << v.size() << '{';
          for (const auto &[k, e] : v) {
            out << k.size() << ':' << k;
            writeProjection(out, e);
          }
          out << '}';
        } else if constexpr (std::is_same_v<T, gfsim::ReplayValue::Integer>)
          out << v.width << ',' << v.isSigned << ',' << v.bits;
        else if constexpr (std::is_same_v<T, gfsim::ReplayValue::FloatBits>)
          out << v.bits;
        else if constexpr (std::is_same_v<T, std::string>)
          out << v.size() << ':' << v;
        else if constexpr (std::is_same_v<T, bool>)
          out << v;
      },
      value.data);
  out << ';';
}
struct Pair {
  Lane scan{true}, active{false};
  std::ofstream projection_file{"projection.bin", std::ios::binary};
  Pair(bool record) {
    // Exercise reset from published state before either recording begins.
    {
      Lane probe(true);
      const auto reset_image = probe.projection();
      RobEvent request{};
      request.valid = 1;
      CHECK(probe.offer(1, request));
      for (unsigned i = 0; i < 4; ++i)
        CHECK(probe.system.step());
      CHECK(probe.count() == 1);
      const auto allocated = probe.entry(0);
      RobCompletion completion{};
      completion.valid = 1;
      completion.attempt.identity.epoch = allocated.epoch;
      completion.attempt.identity.inst = allocated.inst;
      completion.attempt.identity.block = allocated.block;
      completion.attempt.identity.rob = allocated.rob;
      CHECK(probe.offer(2, completion));
      for (unsigned i = 0; i < 4; ++i)
        CHECK(probe.system.step());
      CHECK(probe.pending());
      for (auto &row : probe.model.dispatch_rows())
        row.reset(row.object);
      CHECK(probe.projection() == reset_image);
    }
    CHECK(scan.projection() == active.projection());
    for (unsigned side = 0; side < Flows; ++side) {
      CHECK(scan.count(side) == 0 && scan.epoch(side) == 0 &&
            !scan.pending(side) && !scan.recovering(side));
      CHECK(scan.scalar<4>(StateBase + 12 * side) == 0 &&
            scan.scalar<4>(StateBase + 1 + 12 * side) == 0);
      for (unsigned i = 0; i < 16; ++i)
        CHECK(scan.entry(i, side) == RobEvent{});
    }
    if (record) {
      scan.record("execution.pyctrace");
      active.record("activation.pyctrace");
    }
  }
  void step(unsigned n = 1) {
    for (unsigned i = 0; i < n; ++i) {
      CHECK(scan.system.step());
      CHECK(active.system.step());
      CHECK(scan.system.currentEpoch() == active.system.currentEpoch());
      auto projection = scan.projection();
      CHECK(projection == active.projection());
      CHECK(scan.system.commitTimeline() == active.system.commitTimeline());
      writeProjection(projection_file, projection);
      projection_file << '\n';
    }
  }
  template <class T> void offer(unsigned id, const T &value) {
    for (auto *lane : {&scan, &active}) {
      auto &q = lane->object<gfsim::SimQueue<T>>(id);
      CHECK(q.canProposePush());
      CHECK(lane->system.scheduleExternalXfer(id));
      CHECK(q.proposePush(value));
    }
  }
  const gfsim::SimQueue<RobEvent> &output(unsigned index) {
    return scan.object<gfsim::SimQueue<RobEvent>>(Inputs + index);
  }
  RobEvent take(unsigned index) {
    for (unsigned i = 0; output(index).isEmpty() && i < 24; ++i)
      step();
    CHECK(!output(index).isEmpty());
    auto expected = output(index).committedValues().front();
    for (auto *lane : {&scan, &active}) {
      auto &q = lane->object<gfsim::SimQueue<RobEvent>>(Inputs + index);
      CHECK(q.committedValues().front() == expected);
      CHECK(lane->system.scheduleExternalXfer(Inputs + index));
      CHECK(q.proposePop().has_value());
    }
    step();
    return expected;
  }
  void finish() {
    if (scan.replay) {
      scan.replay->finish();
      active.replay->finish();
    }
    CHECK(active.system.activationTraversalCount() > 0);
    CHECK(active.system.workInvocationCount() <
          scan.system.workInvocationCount());
    std::cout << "PASS scan_work=" << scan.system.workInvocationCount()
              << " activation_work=" << active.system.workInvocationCount()
              << '\n';
  }
};
FlowKey flow(unsigned side = 0) {
  FlowKey f{};
  f.stid = side;
  return f;
}
RobEvent request(unsigned sequence, unsigned epoch = 0, unsigned side = 0) {
  RobEvent r{};
  r.valid = 1;
  r.kind = RobEventKind::ALLOCATE;
  r.epoch.flow = flow(side);
  r.epoch.recovery_epoch = epoch;
  r.inst.flow = flow(side);
  r.inst.instruction_sequence = sequence;
  r.inst.original_pc = 0x1000000000000000ULL + sequence;
  r.block.flow = flow(side);
  r.block.block_sequence = sequence / 4;
  r.block.generation = 5;
  r.rob.flow = flow(side);
  r.destination_tag = sequence % 127;
  r.destination_generation = 7;
  r.handoff_required_mask = 3;
  r.mpq_history_record_count = 2;
  return r;
}
RobEvent flush(unsigned epoch, unsigned side = 0) {
  auto r = request(0, epoch, side);
  r.kind = RobEventKind::FLUSH;
  return r;
}
RobCompletion completion(const RobEvent &a) {
  RobCompletion c{};
  c.valid = 1;
  c.attempt.identity.epoch = a.epoch;
  c.attempt.identity.inst = a.inst;
  c.attempt.identity.block = a.block;
  c.attempt.identity.rob = a.rob;
  c.result = 0xfedcba9876540000ULL + a.inst.instruction_sequence.value();
  c.result_valid = 1;
  c.status = TerminalStatus::FAULT;
  c.fault_valid = 1;
  c.fault_code = 0xdeadbeef;
  c.fault_arg0 = 0xffffeeeeaaaabbbbULL;
  c.fault_bi = 1;
  return c;
}
RobHandoff ack(const RobEvent &a) {
  RobHandoff h{};
  h.valid = 1;
  h.durable = 1;
  h.epoch = a.epoch;
  h.inst = a.inst;
  h.block = a.block;
  h.rob = a.rob;
  h.required_owner_mask = a.handoff_required_mask;
  h.received_owner_mask = a.handoff_required_mask;
  h.mpq_histories_required = a.mpq_history_record_count;
  h.mpq_histories_acked = a.mpq_history_record_count;
  return h;
}
RobEvent allocated(Pair &p, unsigned seq, unsigned slot, unsigned gen = 1,
                   unsigned epoch = 0, unsigned side = 0) {
  auto r = request(seq, epoch, side);
  p.offer(1 + 4 * side, r);
  auto a = p.take(2 * side);
  auto expected = r;
  expected.kind = RobEventKind::ALLOCATED;
  expected.rob.slot = slot;
  expected.rob.generation = gen;
  CHECK(a == expected);
  return a;
}
RobEvent committed(Pair &p, const RobEvent &a, unsigned side = 0) {
  auto c = completion(a);
  auto expected = a;
  expected.kind = RobEventKind::MICROCOMMIT;
  expected.done = 1;
  expected.handoff_pending = 1;
  expected.result = c.result;
  expected.result_valid = c.result_valid;
  expected.status = c.status;
  expected.fault_code = c.fault_code;
  expected.fault_arg0 = c.fault_arg0;
  expected.fault_bi = c.fault_bi;
  expected.fault_valid = c.fault_valid;
  auto actual = p.take(1 + 2 * side);
  CHECK(actual == expected);
  return actual;
}
void capacity(Pair &p) {
  std::vector<RobEvent> entries;
  for (unsigned i = 0; i < 16; ++i)
    entries.push_back(allocated(p, i, i));
  CHECK(p.scan.count() == 16);
  p.offer(1, request(16));
  p.step(6);
  CHECK(!p.scan.object<gfsim::SimQueue<RobEvent>>(1).isEmpty());
  CHECK(p.scan.count() == 16);
  for (unsigned i = 16; i-- > 1;) {
    p.offer(2, completion(entries[i]));
    p.step(3);
    CHECK(p.output(1).isEmpty());
  }
  p.offer(2, completion(entries[0]));
  auto c = committed(p, entries[0]);
  p.step(5);
  CHECK(p.scan.count() == 16 && p.scan.pending());
  CHECK(!p.scan.object<gfsim::SimQueue<RobEvent>>(1).isEmpty());
  p.offer(3, ack(c));
  auto reused = p.take(0);
  CHECK(reused.rob.slot == 0 && reused.rob.generation == 2);
  CHECK(p.scan.count() == 16);
  p.offer(3, ack(c));
  p.offer(2, completion(entries[0]));
  p.step(4);
  CHECK(p.scan.count() == 16 && !p.scan.entry(0).done);
  for (unsigned i = 1; i < 16; ++i) {
    auto out = committed(p, entries[i]);
    CHECK(p.scan.pending());
    p.offer(3, ack(out));
    p.step(3);
    CHECK(p.scan.count() == 16 - i);
  }
  p.offer(2, completion(reused));
  auto last = committed(p, reused);
  p.offer(3, ack(last));
  p.step(4);
  CHECK(p.scan.count() == 0);
}
void backpressure(Pair &p) {
  p.offer(1, request(1));
  p.step(5);
  CHECK(p.scan.count() == 1);
  CHECK(!p.output(0).isEmpty());
  auto first = p.output(0).committedValues().front();
  p.offer(1, request(2));
  p.step(5);
  CHECK(p.scan.count() == 1);
  CHECK(!p.scan.object<gfsim::SimQueue<RobEvent>>(1).isEmpty());
  CHECK(p.take(0) == first);
  auto second = p.take(0);
  CHECK(second.rob.slot == 1);
  CHECK(p.scan.count() == 2);
  p.offer(2, completion(first));
  p.step(5);
  CHECK(p.scan.pending());
  CHECK(!p.output(1).isEmpty());
  auto first_commit = p.output(1).committedValues().front();
  p.offer(2, completion(second));
  p.offer(3, ack(first_commit));
  p.step(5);
  CHECK(p.scan.count() == 1 && !p.scan.pending());
  CHECK(p.scan.entry(1).done && !p.scan.entry(1).handoff_pending);
  CHECK(p.take(1) == first_commit);
  auto c = committed(p, second);
  p.offer(3, ack(c));
  p.step(4);
  CHECK(p.scan.count() == 0);
}
void invalid(Pair &p) {
  auto a = allocated(p, 1, 0);
  auto good = completion(a);
  for (unsigned i = 0; i < 10; ++i) {
    auto c = good;
    switch (i) {
    case 0:
      c.valid = 0;
      break;
    case 1:
      c.attempt.identity.rob.slot = 15;
      break;
    case 2:
      c.attempt.identity.rob.generation = 2;
      break;
    case 3:
      c.attempt.identity.epoch.recovery_epoch = 1;
      break;
    case 4:
      c.attempt.identity.epoch.flow.stid = 1;
      break;
    case 5:
      c.attempt.identity.inst.original_pc = 0;
      break;
    case 6:
      c.attempt.identity.block.generation = 6;
      break;
    case 7:
      c.attempt.identity.rob.flow.stid = 1;
      break;
    case 8:
      c.attempt.identity.inst.flow.stid = 1;
      break;
    case 9:
      c.attempt.identity.block.flow.stid = 1;
      break;
    }
    p.offer(2, c);
    p.step(4);
    CHECK(p.scan.object<gfsim::SimQueue<RobCompletion>>(2).isEmpty());
    CHECK(!p.scan.entry(0).done);
    CHECK(p.output(1).isEmpty());
  }
  p.offer(3, ack(a));
  p.step(3);
  CHECK(p.scan.count() == 1);
  p.offer(2, good);
  auto c = committed(p, a);
  for (unsigned i = 0; i < 10; ++i) {
    auto h = ack(c);
    switch (i) {
    case 0:
      h.valid = 0;
      break;
    case 1:
      h.durable = 0;
      break;
    case 2:
      h.required_owner_mask = 1;
      break;
    case 3:
      h.received_owner_mask = 1;
      break;
    case 4:
      h.mpq_histories_required = 1;
      break;
    case 5:
      h.mpq_histories_acked = 1;
      break;
    case 6:
      h.rob.generation = 2;
      break;
    case 7:
      h.epoch.recovery_epoch = 1;
      break;
    case 8:
      h.inst.original_pc = 0;
      break;
    case 9:
      h.block.generation = 6;
      break;
    }
    p.offer(3, h);
    p.step(4);
    CHECK(p.scan.object<gfsim::SimQueue<RobHandoff>>(3).isEmpty());
    CHECK(p.scan.count() == 1 && p.scan.pending());
  }
  good.result = 0;
  p.offer(2, good);
  p.step(3);
  CHECK(p.scan.entry(0).result == c.result);
  p.offer(3, ack(c));
  p.step(4);
  p.offer(3, ack(c));
  p.step(4);
  CHECK(p.scan.count() == 0 && !p.scan.pending());
}
void recovery(Pair &p) {
  auto a = allocated(p, 1, 0);
  auto b = allocated(p, 2, 1);
  p.offer(0, flush(0));
  p.offer(2, completion(a));
  p.step(5);
  CHECK(p.scan.count() == 0 && p.scan.epoch() == 1 && p.output(1).isEmpty());
  p.offer(0, flush(0));
  p.offer(2, completion(b));
  p.step(4);
  CHECK(p.scan.epoch() == 1);
  CHECK(p.scan.object<gfsim::SimQueue<RobEvent>>(0).isEmpty());
  auto c = allocated(p, 3, 2, 1, 1);
  auto d = allocated(p, 4, 3, 1, 1);
  p.offer(2, completion(c));
  p.step(5);
  CHECK(p.scan.pending());
  CHECK(!p.output(1).isEmpty());
  auto published = p.output(1).committedValues().front();
  p.offer(0, flush(1));
  p.offer(2, completion(d));
  p.step(5);
  CHECK(p.scan.count() == 1 && p.scan.epoch() == 2 && p.scan.recovering());
  CHECK(p.output(1).committedValues().front() == published);
  p.offer(1, request(5, 2));
  p.offer(0, flush(1));
  p.step(4);
  CHECK(p.scan.count() == 1 && p.scan.epoch() == 2 &&
        !p.scan.object<gfsim::SimQueue<RobEvent>>(1).isEmpty());
  p.offer(0, flush(2));
  p.step(4);
  CHECK(p.scan.epoch() == 2); // repeated recovery while waiting
  CHECK(p.take(1) == published);
  p.offer(3, ack(published));
  auto e = p.take(0);
  CHECK(e.rob.slot == 4 && e.epoch.recovery_epoch == 2);
  CHECK(!p.scan.recovering());
  p.offer(2, completion(d));
  p.step(3);
  CHECK(!p.scan.entry(4).done);
  p.offer(2, completion(e));
  auto final = committed(p, e);
  p.offer(3, ack(final));
  p.step(3);
  CHECK(p.scan.count() == 0);
}
void conflicts(Pair &p) {
  auto a = allocated(p, 1, 0);
  auto b = allocated(p, 2, 1);
  p.offer(2, completion(a));
  auto c = committed(p, a);
  p.offer(0, flush(0));
  p.offer(2, completion(b));
  p.offer(3, ack(c));
  p.step(8);
  CHECK(p.scan.count() == 0 && p.scan.epoch() == 1 && !p.scan.recovering() &&
        !p.scan.pending());
  CHECK(p.output(1).isEmpty());
  auto left = allocated(p, 3, 2, 1, 1);
  auto right = allocated(p, 9, 0, 1, 0, 1);
  auto wrong = completion(right);
  p.offer(2, wrong);
  p.offer(6, completion(right));
  p.step(5);
  CHECK(!p.scan.entry(2).done);
  auto rc = committed(p, right, 1);
  CHECK(p.scan.count() == 1);
  p.offer(7, ack(rc));
  p.offer(2, completion(left));
  auto lc = committed(p, left);
  p.offer(3, ack(lc));
  p.step(4);
  CHECK(p.scan.count() == 0 && p.scan.count(1) == 0 && p.scan.epoch(1) == 0);
  auto ready = allocated(p, 4, 3, 1, 1);
  p.offer(2, completion(ready));
  p.step();
  // Flush becomes readable when completion commits: next Work sees a done
  // head and recovery together, before an irreversible handoff.
  p.offer(0, flush(1));
  p.step(6);
  CHECK(p.scan.epoch() == 2 && p.scan.count() == 0 && !p.scan.pending());
  CHECK(p.output(1).isEmpty());
}
int main(int argc, char **argv) {
  try {
    CHECK(argc == 2);
    Pair p(std::getenv("PYC_RECORD_REPLAY") != nullptr);
    std::string scenario = argv[1];
    if (scenario == "capacity")
      capacity(p);
    else if (scenario == "backpressure")
      backpressure(p);
    else if (scenario == "invalid")
      invalid(p);
    else if (scenario == "recovery")
      recovery(p);
    else if (scenario == "conflicts" && Flows == 2)
      conflicts(p);
    else
      CHECK(false);
    p.finish();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
