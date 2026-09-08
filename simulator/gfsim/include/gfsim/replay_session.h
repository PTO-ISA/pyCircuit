#ifndef GFSIM_REPLAY_SESSION_H
#define GFSIM_REPLAY_SESSION_H

#include "gfsim/object.h"
#include "gfsim/queue.h"
#include "gfsim/replay.h"
#include <iterator>
#include <memory>

namespace gfsim {
template <typename Entry> class SimTable;

// External adapter: owns all recording state, codecs, and the file writer.
// Registered objects must outlive the session. No callbacks retain value views.
class ReplaySession final : public StateObserver {
public:
  ReplaySession(const std::string &path, std::span<const DispatchRow> rows)
      : recorder_(path) {
    for (const auto &row : rows)
      add(*static_cast<SimObject *>(row.object));
  }
  ReplaySession(const std::string &path, std::span<SimObject *const> objects)
      : recorder_(path) {
    for (auto *object : objects)
      add(*object);
  }
  ~ReplaySession() { detach(); }
  ReplaySession(const ReplaySession &) = delete;
  ReplaySession &operator=(const ReplaySession &) = delete;

  void add(SimObject &object) {
    requireRegistration();
    if (auto it = objects_.find(object.id()); it != objects_.end()) {
      if (it->second != &object)
        throw std::runtime_error("replay: duplicate object identity");
      return;
    }
    objects_.emplace(object.id(), &object);
    descriptors_[object.id()] = {
        {"name", std::string(object.name())},
        {"path", std::string(object.path())},
        {"parent_path", object.parent() ? std::string(object.parent()->path())
                                        : std::string{}},
        {"object_kind", replayValue(static_cast<uint8_t>(object.kind()))}};
    recorder_.add(object.id(), {descriptors_.at(object.id()),
                                [] { return ReplayValue::Object{}; }});
  }
  template <typename T> void add(SimQueue<T> &queue) {
    add(static_cast<SimObject &>(queue));
    auto &r = resource<T>(queue.id());
    r.latency = queue.latency();
    r.initialize = [this, &queue, &r] {
      for (size_t i = 0; i < queue.committedSize(); ++i)
        r.tokens.push_back(recorder_.nextToken());
      for (size_t i = 0; i < queue.delayedValues().size(); ++i)
        r.delayed.push_back(recorder_.nextToken());
      r.pushOwners.assign(queue.pendingPushCount(), kInvalidObjectId);
      r.popOwners.assign(queue.pendingPopCount(), kInvalidObjectId);
    };
    auto &d = descriptors_.at(queue.id());
    d["visual"] = "queue";
    d["capacity"] = replayValue(queue.entryCapacity());
    entryDescriptor<T>(d);
    r.snapshot = [&queue, &r] {
      return ReplayValue::Object{
          {"values", replayValue(queue.committedValues())},
          {"delayed", replayValue(queue.delayedValues())},
          {"tokens", replayValue(r.tokens)},
          {"delayed_tokens", replayValue(r.delayed)},
          {"depth", replayValue(queue.entryCapacity())},
          {"latency", replayValue(queue.latency())},
          {"rate", replayValue(queue.rate())},
          {"push_count", replayValue(queue.totalPushes())},
          {"pop_count", replayValue(queue.totalPops())}};
    };
    update(queue.id());
  }
  template <typename T> void add(SimTable<T> &table) {
    add(static_cast<SimObject &>(table));
    auto &r = resource<T>(table.id());
    auto &d = descriptors_.at(table.id());
    d["visual"] = "table";
    d["rows"] = replayValue(table.size());
    entryDescriptor<T>(d);
    r.snapshot = [&table] {
      return ReplayValue::Object{
          {"entries", replayValue(table.committedValues())}};
    };
    update(table.id());
  }
  void connect(SimObject &object,
               std::initializer_list<SimObject *> inputs = {},
               std::initializer_list<SimObject *> outputs = {},
               std::initializer_list<SimObject *> tables = {},
               std::initializer_list<SimObject *> control = {},
               std::initializer_list<SimObject *> feedback = {}) {
    add(object);
    auto &d = descriptors_.at(object.id());
    for (auto [name, refs] : {std::pair{"inputs", inputs},
                              {"outputs", outputs},
                              {"tables", tables},
                              {"control", control},
                              {"feedback", feedback}})
      d[name] = replayValue(refs);
    update(object.id());
  }
  void attach(SimSystem &system) {
    if (system_ || system.stateObserver())
      throw std::runtime_error("replay: system already observed");
    system.setStateObserver(this);
    system_ = &system;
  }
  void source(const std::string &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
      throw std::runtime_error("replay: cannot read source attachment " + path);
    recorder_.attachment(path, {std::istreambuf_iterator<char>(stream), {}});
  }
  void start(ReplayValue::Object metadata = {}) {
    requireRegistration();
    for (const auto &[id, object] : objects_) {
      auto &descriptor = descriptors_.at(id);
      descriptor["display_name"] = std::string(object->displayName());
      descriptor["display_path"] = object->displayPath();
      descriptor["display_parent_path"] = object->displayParentPath();
      descriptor["module_parameters"] = std::string(object->sourceParameters());
      update(id);
    }
    started_ = true;
    try {
      for (const auto &[id, r] : resources_)
        if (!r.snapshot)
          throw std::runtime_error("replay: incomplete state registration");
      for (auto &[id, r] : resources_)
        if (r.initialize)
          r.initialize();
      recorder_.start(std::move(metadata));
      for (const auto &[id, object] : objects_) {
        if (object->stateObserver())
          throw std::runtime_error("replay: object already observed");
        object->setStateObserver(this);
        attached_.push_back(object);
      }
      started_ = true;
    } catch (...) {
      detach();
      throw;
    }
  }
  void begin(Epoch epoch) override { recorder_.begin(epoch); }
  void end() override { recorder_.end(); }
  void finish(std::string status = "completed") {
    recorder_.finish(std::move(status));
    detach();
  }
  void context(ObjectId owner, ExecutionPhase phase) override {
    recorder_.context(owner, phase == ExecutionPhase::Work ? "Work" : "other");
  }
  void clearContext() override { recorder_.clearContext(); }
  void beforeReset() override {
    if (recorder_.recording())
      throw std::runtime_error("replay: finish recording before reset");
  }
  void notify(const StateEvent &e) override {
    if (!recorder_.recording())
      return;
    auto it = resources_.find(e.object);
    if (it == resources_.end())
      throw std::runtime_error(
          "replay: state resource lacks typed registration");
    auto &r = it->second;
    const auto owner = recorder_.owner();
    auto value = [&] { return r.encode(e.value); };
    auto emit = [&](std::string_view action, ReplayValue::Object fields) {
      recorder_.event(e.object, action, std::move(fields));
    };
    switch (e.action) {
    case StateAction::OfferPush:
    case StateAction::OfferPop: {
      auto &owners =
          e.action == StateAction::OfferPush ? r.pushOwners : r.popOwners;
      if (owners.size() != e.index)
        throw std::runtime_error("replay: proposal position mismatch");
      owners.push_back(owner);
      break;
    }
    case StateAction::Dequeue:
      emit("dequeue", {{"owner", replayValue(r.popOwners.at(e.index))},
                       {"token", replayValue(r.tokens.at(e.index))},
                       {"value", value()}});
      ++r.popped;
      break;
    case StateAction::Ready: {
      const auto token = r.delayed.at(e.index);
      emit("ready", {{"owner", replayValue(kInvalidObjectId)},
                     {"token", replayValue(token)},
                     {"value", value()}});
      r.tokens.push_back(token);
      r.delayed.erase(r.delayed.begin() + e.index);
      break;
    }
    case StateAction::Enqueue: {
      const auto token = recorder_.nextToken();
      emit("enqueue", {{"owner", replayValue(r.pushOwners.at(e.index))},
                       {"token", replayValue(token)},
                       {"value", value()},
                       {"ready_time", replayValue(e.readyTime)}});
      (r.latency == 1 ? r.tokens : r.delayed).push_back(token);
      break;
    }
    case StateAction::QueueCommitEnd:
      r.tokens.erase(r.tokens.begin(), r.tokens.begin() + r.popped);
      r.popped = 0;
      r.pushOwners.clear();
      r.popOwners.clear();
      break;
    case StateAction::TableRead:
      emit("state_read", {{"index", replayValue(e.index)}, {"value", value()}});
      break;
    case StateAction::BeforeWrite:
      if (r.before)
        throw std::runtime_error("replay: overlapping write notifications");
      r.before = value();
      break;
    case StateAction::AfterWrite:
      if (!r.before)
        throw std::runtime_error("replay: missing before-write notification");
      emit("state_write", {{"owner", replayValue(e.writer)},
                           {"index", replayValue(e.index)},
                           {"before", std::move(*r.before)},
                           {"after", value()},
                           {"fields", replayValue(e.fields)},
                           {"mode", replayValue(e.mode)}});
      r.before.reset();
      break;
    }
  }
  ReplayRecorder &recorder() { return recorder_; }

private:
  struct Resource {
    std::function<ReplayValue(const void *)> encode;
    std::function<ReplayValue()> snapshot;
    std::function<void()> initialize;
    std::vector<uint64_t> tokens, delayed;
    std::vector<ObjectId> pushOwners, popOwners;
    std::optional<ReplayValue> before;
    size_t latency = 1, popped = 0;
  };
  template <typename T> Resource &resource(ObjectId id) {
    requireRegistration();
    auto [it, inserted] = resources_.try_emplace(id);
    if (!inserted)
      throw std::runtime_error("replay: duplicate state registration");
    it->second.encode = [](const void *value) {
      return replayValue(*static_cast<const T *>(value));
    };
    return it->second;
  }
  template <typename T> static void entryDescriptor(ReplayValue::Object &d) {
    d["entry"] = replayValue(T{});
    d["fields"] = replayFields<T>();
    d["flat"] = replayFlat<T>();
  }
  void update(ObjectId id) {
    auto it = resources_.find(id);
    recorder_.replace(
        id, {descriptors_.at(id), it == resources_.end()
                                      ? std::function<ReplayValue()>{[] {
                                          return ReplayValue::Object{};
                                        }}
                                      : it->second.snapshot});
  }
  void requireRegistration() const {
    if (started_)
      throw std::runtime_error("replay: registration already closed");
  }
  void detach() {
    if (system_ && system_->stateObserver() == this)
      system_->SimObject::setStateObserver(nullptr);
    system_ = nullptr;
    for (auto *object : attached_)
      if (object->stateObserver() == this)
        object->setStateObserver(nullptr);
    attached_.clear();
  }
  ReplayRecorder recorder_;
  std::map<ObjectId, SimObject *> objects_;
  std::map<ObjectId, ReplayValue::Object> descriptors_;
  std::map<ObjectId, Resource> resources_;
  std::vector<SimObject *> attached_;
  SimSystem *system_ = nullptr;
  bool started_ = false;
};
} // namespace gfsim
#endif
