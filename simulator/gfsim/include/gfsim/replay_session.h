#ifndef GFSIM_REPLAY_SESSION_H
#define GFSIM_REPLAY_SESSION_H

#include "gfsim/object.h"

#include <iterator>

namespace gfsim {

/// Adapter for testbenches that drive the generated dispatch rows directly.
/// The session must outlive all calls to those rows and their Queue endpoints.
class ReplaySession {
public:
  ReplaySession(const std::string &path, std::span<const DispatchRow> rows)
      : recorder_(path) {
    try {
      for (const auto &row : rows) {
        auto *object = static_cast<SimObject *>(row.object);
        object->attachReplay(recorder_);
        objects_.push_back(object);
      }
    } catch (...) {
      for (auto *object : objects_)
        object->detachReplay();
      throw;
    }
  }
  ~ReplaySession() {
    for (auto *object : objects_)
      object->detachReplay();
  }
  void source(const std::string &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
      throw std::runtime_error("replay: cannot read source attachment " + path);
    recorder_.attachment(path, {std::istreambuf_iterator<char>(stream), {}});
  }
  void start(ReplayValue::Object metadata = {}) {
    recorder_.start(std::move(metadata));
  }
  void begin(Epoch epoch) { recorder_.begin(epoch); }
  void end() { recorder_.end(); }
  void finish(std::string status = "completed") {
    recorder_.finish(std::move(status));
  }
  ReplayRecorder &recorder() { return recorder_; }

private:
  ReplayRecorder recorder_;
  std::vector<SimObject *> objects_;
};

} // namespace gfsim
#endif
