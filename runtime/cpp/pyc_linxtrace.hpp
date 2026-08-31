#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace pyc::cpp {

class LinxTraceWriter {
public:
  bool open(const std::filesystem::path &path, std::uint64_t startCycle) {
    close();
    path_ = path;
    if (path_.extension() != ".linxtrace") {
      opened_ = false;
      return false;
    }
    if (path_.has_parent_path()) {
      std::filesystem::create_directories(path_.parent_path());
    }
    rows_.clear();
    stages_.clear();
    lanes_.clear();
    event_tmp_path_ = std::filesystem::path(path_.string() + ".events.tmp");
    event_out_.open(event_tmp_path_, std::ios::out | std::ios::trunc);
    if (!event_out_.is_open()) {
      opened_ = false;
      return false;
    }

    cur_cycle_ = startCycle;
    opened_ = true;
    return true;
  }

  void close() {
    if (!opened_) {
      return;
    }
    if (event_out_.is_open()) {
      event_out_.flush();
      event_out_.close();
    }
    finalizeTraceFile();
    std::error_code ec{};
    std::filesystem::remove(event_tmp_path_, ec);
    event_tmp_path_.clear();
    path_.clear();
    opened_ = false;
  }

  ~LinxTraceWriter() { close(); }

  bool isOpen() const {
    return opened_ && event_out_.is_open();
  }

  void atCycle(std::uint64_t cycle) {
    if (!isOpen()) {
      return;
    }
    if (cycle < cur_cycle_) {
      return;
    }
    cur_cycle_ = cycle;
  }

  void insn(std::uint64_t rowId, std::uint64_t simId, std::uint64_t threadId) {
    std::ostringstream uid;
    uid << "0x" << std::hex << simId << std::dec;
    insnV5(rowId, uid.str(), threadId, "0x0", "normal");
  }

  // LinxTrace protocol v5 record helper; this version is independent of the
  // pyCircuit product version.
  void insnV5(std::uint64_t rowId, const std::string &uidHex, std::uint64_t threadId,
              const std::string &parentUidHex, const std::string &kind) {
    if (!isOpen()) {
      return;
    }
    RowInfo &row = rows_[rowId];
    row.row_id = rowId;
    row.core_id = threadId;
    row.kind = sanitizeText(kind);
    row.parent_uid = sanitizeText(parentUidHex);
    row.row_kind = (row.kind == "block") ? "block" : "uop";
    if (row.row_kind == "block") {
      row.block_uid = sanitizeText(uidHex);
    } else {
      row.uop_uid = sanitizeText(uidHex);
    }

    std::ostringstream ss;
    ss << "{\"type\":\"OP_DEF\""
       << ",\"cycle\":" << cur_cycle_
       << ",\"row_id\":" << rowId
       << ",\"row_kind\":\"" << jsonEscape(row.row_kind) << "\""
       << ",\"core_id\":" << threadId
       << ",\"kind\":\"" << jsonEscape(row.kind) << "\""
       << ",\"row_sid\":\"" << jsonEscape(row.uop_uid.empty() ? "0x0" : row.uop_uid) << "\""
       << ",\"uop_uid\":\"" << jsonEscape(row.uop_uid.empty() ? "0x0" : row.uop_uid) << "\""
       << ",\"parent_uid\":\"" << jsonEscape(row.parent_uid.empty() ? "0x0" : row.parent_uid) << "\""
       << ",\"block_uid\":\"" << jsonEscape(row.block_uid.empty() ? "0x0" : row.block_uid) << "\""
       << "}";
    writeLine(ss.str());
  }

  void label(std::uint64_t rowId, int type, const std::string &text) {
    if (!isOpen()) {
      return;
    }
    const std::string labelType = (type == 0) ? "left" : "detail";
    RowInfo &row = rows_[rowId];
    row.row_id = rowId;
    if (type == 0) {
      row.left_label = sanitizeText(text);
    } else {
      row.detail_defaults = sanitizeText(text);
    }
    std::ostringstream ss;
    ss << "{\"type\":\"LABEL\""
       << ",\"cycle\":" << cur_cycle_
       << ",\"row_id\":" << rowId
       << ",\"label_type\":\"" << labelType << "\""
       << ",\"text\":\"" << jsonEscape(sanitizeText(text)) << "\""
       << "}";
    writeLine(ss.str());
  }

  // Kept for source compatibility with existing writers.
  void stageStart(std::uint64_t id, int laneId, const std::string &stage) {
    presence(id, laneId, stage, 0, "0");
  }

  // Kept for source compatibility with existing writers.
  void stageEnd(std::uint64_t, int, const std::string &) {}

  void retire(std::uint64_t rowId, std::uint64_t retireId, int type) {
    if (!isOpen()) {
      return;
    }
    const char *status = "ok";
    if (type == 1) {
      status = "flush";
    } else if (type == 2) {
      status = "trap";
    }
    std::ostringstream ss;
    ss << "{\"type\":\"RETIRE\""
       << ",\"cycle\":" << cur_cycle_
       << ",\"row_id\":" << rowId
       << ",\"retire_id\":" << retireId
       << ",\"status\":\"" << status << "\""
       << "}";
    writeLine(ss.str());
  }

  void presence(std::uint64_t rowId, int laneId, const std::string &stage, int stall,
                const std::string &stallCause) {
    std::ostringstream lane;
    lane << "c0.l" << laneId;
    presenceV5(rowId, lane.str(), stage, stall, stallCause);
  }

  void presenceV5(std::uint64_t rowId, const std::string &laneToken,
                  const std::string &stage, int stall,
                  const std::string &stallCause) {
    if (!isOpen()) {
      return;
    }
    const std::string lane = sanitizeText(laneToken);
    const std::string stageId = sanitizeText(stage);
    lanes_.insert(lane);
    stages_.insert(stageId);

    std::ostringstream ss;
    ss << "{\"type\":\"OCC\""
       << ",\"cycle\":" << cur_cycle_
       << ",\"row_id\":" << rowId
       << ",\"lane_id\":\"" << jsonEscape(lane) << "\""
       << ",\"stage_id\":\"" << jsonEscape(stageId) << "\""
       << ",\"stall\":" << (stall ? 1 : 0)
       << ",\"cause\":\"" << jsonEscape(sanitizeText(stallCause)) << "\""
       << "}";
    writeLine(ss.str());
  }

  // Kept for source compatibility; dependency rendering is handled by external tools.
  void dep(std::uint64_t consumerId, std::uint64_t producerId, int type) {
    if (!isOpen()) {
      return;
    }
    std::ostringstream ss;
    ss << "{\"type\":\"DEP\""
       << ",\"cycle\":" << cur_cycle_
       << ",\"consumer_row_id\":" << consumerId
       << ",\"producer_row_id\":" << producerId
       << ",\"dep_type\":" << type
       << "}";
    writeLine(ss.str());
  }

private:
  static constexpr std::array<const char *, 39> kCanonicalStageOrder = {
      "F0", "F1", "F2", "F3", "F4", "D1", "D2", "D3", "IQ", "S1",
      "S2", "P1", "I1", "I2", "E1", "E2", "E3", "E4", "W1", "W2",
      "LIQ", "LHQ", "STQ", "SCB", "MDB", "L1D", "BISQ", "BCTRL", "TMU", "TMA",
      "CUBE", "VEC", "TAU", "BROB", "ROB", "CMT", "FLS", "XCHK", "IB",
  };
  static constexpr const char *kPipelineSchemaId = "LC-TRACE1-1ABD8F1C22C0";

  struct RowInfo {
    std::uint64_t row_id = 0;
    std::string row_kind = "uop";
    std::uint64_t core_id = 0;
    std::string uop_uid{};
    std::string parent_uid{};
    std::string block_uid{};
    std::string kind = "normal";
    std::string left_label{};
    std::string detail_defaults{};
  };

  static std::string sanitizeText(const std::string &s) {
    std::string out = s;
    for (char &c : out) {
      if (c == '\t' || c == '\n' || c == '\r') {
        c = ' ';
      }
    }
    return out;
  }

  static std::string jsonEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (const char c : s) {
      switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      default:
        out += c;
        break;
      }
    }
    return out;
  }

  static std::string normalizeHexId(const std::string &s) {
    if (s.empty()) {
      return "0x0";
    }
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
      return s;
    }
    return std::string("0x") + s;
  }

  static std::string stageGroup(const std::string &stage) {
    if (stage.rfind("F", 0) == 0 || stage == "IB") {
      return "frontend";
    }
    if (stage == "D1" || stage == "D2" || stage == "D3" || stage == "IQ" || stage == "S1" ||
        stage == "S2" || stage == "P1" || stage == "I1" || stage == "I2" || stage == "E1" ||
        stage == "E2" || stage == "E3" || stage == "E4" || stage == "W1" || stage == "W2") {
      return "backend";
    }
    if (stage == "LIQ" || stage == "LHQ" || stage == "STQ" || stage == "SCB" || stage == "MDB" ||
        stage == "L1D") {
      return "lsu";
    }
    if (stage == "BISQ" || stage == "BCTRL" || stage == "TMU" || stage == "TMA" ||
        stage == "CUBE" || stage == "VEC" || stage == "TAU" || stage == "BROB") {
      return "block";
    }
    if (stage == "ROB" || stage == "CMT" || stage == "FLS" || stage == "XCHK") {
      return "retire";
    }
    return "other";
  }

  static std::string stageColor(const std::string &stage) {
    if (stage == "FLS" || stage == "XCHK") {
      return "#ef4444";
    }
    if (stage == "CMT") {
      return "#22c55e";
    }
    if (stage == "ROB" || stage == "BROB") {
      return "#f97316";
    }
    if (stage.rfind("F", 0) == 0 || stage == "IB") {
      return "#38bdf8";
    }
    if (stage == "IQ" || stage == "S1" || stage == "S2" || stage == "D1" || stage == "D2" ||
        stage == "D3") {
      return "#a78bfa";
    }
    if (stage == "P1" || stage == "I1" || stage == "I2" || stage == "E1" || stage == "E2" ||
        stage == "E3" || stage == "E4" || stage == "W1" || stage == "W2") {
      return "#34d399";
    }
    if (stage == "LIQ" || stage == "LHQ" || stage == "STQ" || stage == "SCB" || stage == "MDB" ||
        stage == "L1D") {
      return "#fbbf24";
    }
    return "#9ca3af";
  }

  static std::uint64_t fnv1a64(const std::string &s) {
    std::uint64_t h = 1469598103934665603ull;
    for (unsigned char c : s) {
      h ^= static_cast<std::uint64_t>(c);
      h *= 1099511628211ull;
    }
    return h;
  }

  void writeLine(const std::string &line) {
    if (!isOpen()) {
      return;
    }
    event_out_ << line << "\n";
  }

  void finalizeTraceFile() {
    if (path_.empty() || event_tmp_path_.empty()) {
      return;
    }
    std::ofstream out(path_, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
      return;
    }
    writeMetaRecord(out);
    std::ifstream events(event_tmp_path_, std::ios::in);
    if (!events.is_open()) {
      return;
    }
    out << events.rdbuf();
  }

  void writeMetaRecord(std::ostream &out) const {
    std::vector<std::string> stageList{};
    stageList.reserve(kCanonicalStageOrder.size());
    for (const char *stage : kCanonicalStageOrder) {
      stageList.emplace_back(stage);
    }
    std::vector<std::string> laneList(lanes_.begin(), lanes_.end());

    std::ostringstream contractSeed;
    contractSeed << kPipelineSchemaId << "|";
    for (std::size_t i = 0; i < stageList.size(); ++i) {
      if (i) contractSeed << ",";
      contractSeed << stageList[i];
    }
    contractSeed << "|";
    for (std::size_t i = 0; i < laneList.size(); ++i) {
      if (i) contractSeed << ",";
      contractSeed << laneList[i];
    }
    contractSeed << "|";
    bool firstRow = true;
    for (const auto &[rowId, row] : rows_) {
      if (!firstRow) {
        contractSeed << ";";
      }
      firstRow = false;
      contractSeed << rowId << ":" << row.row_kind;
    }
    contractSeed << "|linxtrace.v1";
    const std::uint64_t h = fnv1a64(contractSeed.str());
    std::ostringstream idHex;
    // Keep the contract hash fixed-width so consumers can string-compare reliably.
    idHex << kPipelineSchemaId << "-" << std::uppercase << std::hex << std::setw(16) << std::setfill('0')
          << h;
    const std::string stageCsv = [&]() {
      std::ostringstream ss;
      for (std::size_t i = 0; i < stageList.size(); ++i) {
        if (i) ss << ",";
        ss << stageList[i];
      }
      return ss.str();
    }();

    out << "{\"type\":\"META\"";
    out << ",\"format\":\"linxtrace.v1\"";
    out << ",\"contract_id\":\"" << idHex.str() << "\"";
    out << ",\"pipeline_schema_id\":\"" << kPipelineSchemaId << "\"";
    out << ",\"stage_order_csv\":\"" << jsonEscape(stageCsv) << "\"";
    out << ",\"stage_catalog\":[";
    for (std::size_t i = 0; i < stageList.size(); ++i) {
      const std::string &s = stageList[i];
      out << "{\"stage_id\":\"" << jsonEscape(s) << "\",\"label\":\"" << jsonEscape(s)
          << "\",\"color\":\"" << stageColor(s) << "\",\"group\":\"" << stageGroup(s) << "\"}";
      if (i + 1 < stageList.size()) {
        out << ",";
      }
    }
    out << "]";
    out << ",\"lane_catalog\":[";
    for (std::size_t i = 0; i < laneList.size(); ++i) {
      const std::string &l = laneList[i];
      out << "{\"lane_id\":\"" << jsonEscape(l) << "\",\"label\":\"" << jsonEscape(l) << "\"}";
      if (i + 1 < laneList.size()) {
        out << ",";
      }
    }
    out << "]";
    out << ",\"row_catalog\":[";
    std::size_t idx = 0;
    for (const auto &[rowId, row] : rows_) {
      out << "{"
          << "\"row_id\":" << rowId << ","
          << "\"row_kind\":\"" << jsonEscape(row.row_kind) << "\","
          << "\"core_id\":" << row.core_id << ","
          << "\"block_uid\":\"" << jsonEscape(normalizeHexId(row.block_uid.empty() ? "0x0" : row.block_uid))
          << "\","
          << "\"row_sid\":\"" << jsonEscape(normalizeHexId(row.uop_uid.empty() ? "0x0" : row.uop_uid))
          << "\","
          << "\"uop_uid\":\"" << jsonEscape(normalizeHexId(row.uop_uid.empty() ? "0x0" : row.uop_uid))
          << "\","
          << "\"left_label\":\"" << jsonEscape(row.left_label) << "\","
          << "\"detail_defaults\":\"" << jsonEscape(row.detail_defaults) << "\""
          << "}";
      if (++idx < rows_.size()) {
        out << ",";
      }
    }
    out << "]";
    out << ",\"render_prefs\":{\"theme\":\"high-contrast\",\"show_symbols\":true}";
    out << "}\n";
  }

  std::ofstream event_out_{};
  std::filesystem::path path_{};
  std::filesystem::path event_tmp_path_{};
  bool opened_ = false;
  std::uint64_t cur_cycle_ = 0;
  std::map<std::uint64_t, RowInfo> rows_{};
  std::set<std::string> stages_{};
  std::set<std::string> lanes_{};
};

} // namespace pyc::cpp
