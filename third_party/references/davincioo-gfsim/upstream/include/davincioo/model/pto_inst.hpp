#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace davincioo {

enum class PTOOpcode {
  Unknown,
  TLOAD,
  TSTORE,
  TSTORE_FP,
  MGATHER,
  MSCATTER,
  TPUT,
  TGET,
  TPUT_ASYNC,
  TGET_ASYNC,
  TBROADCAST,
  TREDUCE,
  RECORD_EVENT,
  WAIT_EVENT,
  BARRIER,
  TSYNC,
  TALLOC,
  TFREE,
  TPUSH,
  TPOP,
  TNOTIFY,
  TWAIT,
  TTEST,
  TADD,
  TSUB,
  TMUL,
  TDIV,
  TREM,
  TAND,
  TOR,
  TXOR,
  TSHL,
  TSHR,
  TMAX,
  TMIN,
  TPRELU,
  TCMP,
  TABS,
  TEXP,
  TLOG,
  TSQRT,
  TRSQRT,
  TRECIP,
  TNEG,
  TNOT,
  TRELU,
  TCVT,
  TADDC,
  TSUBC,
  TSEL,
  TADDS,
  TSUBS,
  TMULS,
  TDIVS,
  TREMS,
  TANDS,
  TORS,
  TXORS,
  TSHLS,
  TSHRS,
  TMAXS,
  TMINS,
  TLRELU,
  TAXPY,
  TCMPS,
  TADDSC,
  TSUBSC,
  TSELS,
  TEXPANDS,
  TROWSUM,
  TROWPROD,
  TROWMAX,
  TROWMIN,
  TROWARGMAX,
  TROWARGMIN,
  TROWEXPAND,
  TCOLSUM,
  TCOLPROD,
  TCOLMAX,
  TCOLMIN,
  TCOLARGMAX,
  TCOLARGMIN,
  TCOLEXPAND,
  TROWEXPANDDIV,
  TROWEXPANDMUL,
  TROWEXPANDSUB,
  TROWEXPANDADD,
  TROWEXPANDMAX,
  TROWEXPANDMIN,
  TROWEXPANDEXPDIF,
  TCOLEXPANDDIV,
  TCOLEXPANDMUL,
  TCOLEXPANDSUB,
  TCOLEXPANDADD,
  TCOLEXPANDMAX,
  TCOLEXPANDMIN,
  TCOLEXPANDEXPDIF,
  TFILLPAD,
  TFILLPAD_INPLACE,
  TFILLPAD_EXPAND,
  TMATMUL,
  TMATMUL_MX,
  TMATMUL_ACC,
  TMATMUL_BIAS,
  TMATMUL_MX_ACC,
  TMATMUL_MX_BIAS,
  TGEMV,
  TGEMV_ACC,
  TGEMV_BIAS,
  TGEMV_MX,
  TMOV,
  TMOV_FP,
  TTRANS,
  TEXTRACT,
  TEXTRACT_FP,
  TRESHAPE,
  TASSIGN,
  TCI,
  TTRI,
  TGATHER,
  TGATHERB,
  TSCATTER,
  TSORT32,
  TMRGSORT,
  TPARTADD,
  TPARTMUL,
  TPARTMAX,
  TPARTMIN,
  TPARTARGMAX,
  TPARTARGMIN,
  TRANDOM,
  TPRINT,
  TPREFETCH,
  TIMG2COL,
  SETFMATRIX,
  SET_IMG2COL_RPT,
  SET_IMG2COL_PADDING,
  TCONCAT,
  TDEQUANT,
  TINSERT,
  TINSERT_FP,
  TFMOD,
  TFMODS,
  THISTOGRAM,
  TQUANT,
  TGET_SCALE_ADDR,
  TSUBVIEW,
};

enum class PTOTileType {
  Unknown,
  Vec,
  Mat,
  Left,
  Right,
  Acc,
  Bias,
  Scaling,
  ScaleLeft,
  ScaleRight,
  Ctrl,
  Global,
};

enum class PTODType {
  kUnknown,
  kBool,
  kInt8,
  kInt32,
  kInt64,
  kUInt64,
  kFloat16,
  kBFloat16,
  kFloat32,
  kFloat8,
  kFloat4,
};

enum class PTOLayout {
  kUnknown,
  kND,
  kDN,
  kNZ,
  kScale,
  kMxAND,
  kMxADN,
  kMxAZZ,
  kMxBND,
  kMxBDN,
  kMxBNN,
  kNC1HWC0,
  kNCHW,
  kNHWC,
  kNDC1HWC0,
  kNCDHW,
  kFractalZ,
  kFractalZS16S8,
  kFractalZ3D,
};

enum class PTOEngineKind {
  Scalar,
  Vec,
  Cube,
  Tma,
};

struct PTOOpcodeDescriptor {
  std::string_view name;
  PTOOpcode opcode;
  PTOEngineKind engine_kind;
};

inline constexpr PTOOpcodeDescriptor kUnknownPTOOpcodeDescriptor{
    .name = "UNKNOWN",
    .opcode = PTOOpcode::Unknown,
    .engine_kind = PTOEngineKind::Scalar,
};

inline constexpr auto kPTOOpcodeDescriptors = std::to_array<PTOOpcodeDescriptor>({
    {"TLOAD", PTOOpcode::TLOAD, PTOEngineKind::Tma},
    {"TSTORE", PTOOpcode::TSTORE, PTOEngineKind::Tma},
    {"TSTORE_FP", PTOOpcode::TSTORE_FP, PTOEngineKind::Tma},
    {"MGATHER", PTOOpcode::MGATHER, PTOEngineKind::Tma},
    {"MSCATTER", PTOOpcode::MSCATTER, PTOEngineKind::Tma},
    {"TPUT", PTOOpcode::TPUT, PTOEngineKind::Tma},
    {"TGET", PTOOpcode::TGET, PTOEngineKind::Tma},
    {"TPUT_ASYNC", PTOOpcode::TPUT_ASYNC, PTOEngineKind::Tma},
    {"TGET_ASYNC", PTOOpcode::TGET_ASYNC, PTOEngineKind::Tma},
    {"TBROADCAST", PTOOpcode::TBROADCAST, PTOEngineKind::Tma},
    {"TREDUCE", PTOOpcode::TREDUCE, PTOEngineKind::Tma},
    {"RECORD_EVENT", PTOOpcode::RECORD_EVENT, PTOEngineKind::Scalar},
    {"WAIT_EVENT", PTOOpcode::WAIT_EVENT, PTOEngineKind::Scalar},
    {"BARRIER", PTOOpcode::BARRIER, PTOEngineKind::Scalar},
    {"TSYNC", PTOOpcode::TSYNC, PTOEngineKind::Scalar},
    {"TALLOC", PTOOpcode::TALLOC, PTOEngineKind::Scalar},
    {"TFREE", PTOOpcode::TFREE, PTOEngineKind::Scalar},
    {"TPUSH", PTOOpcode::TPUSH, PTOEngineKind::Scalar},
    {"TPOP", PTOOpcode::TPOP, PTOEngineKind::Scalar},
    {"TNOTIFY", PTOOpcode::TNOTIFY, PTOEngineKind::Scalar},
    {"TWAIT", PTOOpcode::TWAIT, PTOEngineKind::Scalar},
    {"TTEST", PTOOpcode::TTEST, PTOEngineKind::Scalar},
    {"TADD", PTOOpcode::TADD, PTOEngineKind::Vec},
    {"TSUB", PTOOpcode::TSUB, PTOEngineKind::Vec},
    {"TMUL", PTOOpcode::TMUL, PTOEngineKind::Vec},
    {"TDIV", PTOOpcode::TDIV, PTOEngineKind::Vec},
    {"TREM", PTOOpcode::TREM, PTOEngineKind::Vec},
    {"TAND", PTOOpcode::TAND, PTOEngineKind::Vec},
    {"TOR", PTOOpcode::TOR, PTOEngineKind::Vec},
    {"TXOR", PTOOpcode::TXOR, PTOEngineKind::Vec},
    {"TSHL", PTOOpcode::TSHL, PTOEngineKind::Vec},
    {"TSHR", PTOOpcode::TSHR, PTOEngineKind::Vec},
    {"TMAX", PTOOpcode::TMAX, PTOEngineKind::Vec},
    {"TMIN", PTOOpcode::TMIN, PTOEngineKind::Vec},
    {"TPRELU", PTOOpcode::TPRELU, PTOEngineKind::Vec},
    {"TCMP", PTOOpcode::TCMP, PTOEngineKind::Vec},
    {"TABS", PTOOpcode::TABS, PTOEngineKind::Vec},
    {"TEXP", PTOOpcode::TEXP, PTOEngineKind::Vec},
    {"TLOG", PTOOpcode::TLOG, PTOEngineKind::Vec},
    {"TSQRT", PTOOpcode::TSQRT, PTOEngineKind::Vec},
    {"TRSQRT", PTOOpcode::TRSQRT, PTOEngineKind::Vec},
    {"TRECIP", PTOOpcode::TRECIP, PTOEngineKind::Vec},
    {"TNEG", PTOOpcode::TNEG, PTOEngineKind::Vec},
    {"TNOT", PTOOpcode::TNOT, PTOEngineKind::Vec},
    {"TRELU", PTOOpcode::TRELU, PTOEngineKind::Vec},
    {"TCVT", PTOOpcode::TCVT, PTOEngineKind::Vec},
    {"TADDC", PTOOpcode::TADDC, PTOEngineKind::Vec},
    {"TSUBC", PTOOpcode::TSUBC, PTOEngineKind::Vec},
    {"TSEL", PTOOpcode::TSEL, PTOEngineKind::Vec},
    {"TADDS", PTOOpcode::TADDS, PTOEngineKind::Vec},
    {"TSUBS", PTOOpcode::TSUBS, PTOEngineKind::Vec},
    {"TMULS", PTOOpcode::TMULS, PTOEngineKind::Vec},
    {"TDIVS", PTOOpcode::TDIVS, PTOEngineKind::Vec},
    {"TREMS", PTOOpcode::TREMS, PTOEngineKind::Vec},
    {"TANDS", PTOOpcode::TANDS, PTOEngineKind::Vec},
    {"TORS", PTOOpcode::TORS, PTOEngineKind::Vec},
    {"TXORS", PTOOpcode::TXORS, PTOEngineKind::Vec},
    {"TSHLS", PTOOpcode::TSHLS, PTOEngineKind::Vec},
    {"TSHRS", PTOOpcode::TSHRS, PTOEngineKind::Vec},
    {"TMAXS", PTOOpcode::TMAXS, PTOEngineKind::Vec},
    {"TMINS", PTOOpcode::TMINS, PTOEngineKind::Vec},
    {"TLRELU", PTOOpcode::TLRELU, PTOEngineKind::Vec},
    {"TAXPY", PTOOpcode::TAXPY, PTOEngineKind::Vec},
    {"TCMPS", PTOOpcode::TCMPS, PTOEngineKind::Vec},
    {"TADDSC", PTOOpcode::TADDSC, PTOEngineKind::Vec},
    {"TSUBSC", PTOOpcode::TSUBSC, PTOEngineKind::Vec},
    {"TSELS", PTOOpcode::TSELS, PTOEngineKind::Vec},
    {"TEXPANDS", PTOOpcode::TEXPANDS, PTOEngineKind::Vec},
    {"TROWSUM", PTOOpcode::TROWSUM, PTOEngineKind::Vec},
    {"TROWPROD", PTOOpcode::TROWPROD, PTOEngineKind::Vec},
    {"TROWMAX", PTOOpcode::TROWMAX, PTOEngineKind::Vec},
    {"TROWMIN", PTOOpcode::TROWMIN, PTOEngineKind::Vec},
    {"TROWARGMAX", PTOOpcode::TROWARGMAX, PTOEngineKind::Vec},
    {"TROWARGMIN", PTOOpcode::TROWARGMIN, PTOEngineKind::Vec},
    {"TROWEXPAND", PTOOpcode::TROWEXPAND, PTOEngineKind::Vec},
    {"TCOLSUM", PTOOpcode::TCOLSUM, PTOEngineKind::Vec},
    {"TCOLPROD", PTOOpcode::TCOLPROD, PTOEngineKind::Vec},
    {"TCOLMAX", PTOOpcode::TCOLMAX, PTOEngineKind::Vec},
    {"TCOLMIN", PTOOpcode::TCOLMIN, PTOEngineKind::Vec},
    {"TCOLARGMAX", PTOOpcode::TCOLARGMAX, PTOEngineKind::Vec},
    {"TCOLARGMIN", PTOOpcode::TCOLARGMIN, PTOEngineKind::Vec},
    {"TCOLEXPAND", PTOOpcode::TCOLEXPAND, PTOEngineKind::Vec},
    {"TROWEXPANDDIV", PTOOpcode::TROWEXPANDDIV, PTOEngineKind::Vec},
    {"TROWEXPANDMUL", PTOOpcode::TROWEXPANDMUL, PTOEngineKind::Vec},
    {"TROWEXPANDSUB", PTOOpcode::TROWEXPANDSUB, PTOEngineKind::Vec},
    {"TROWEXPANDADD", PTOOpcode::TROWEXPANDADD, PTOEngineKind::Vec},
    {"TROWEXPANDMAX", PTOOpcode::TROWEXPANDMAX, PTOEngineKind::Vec},
    {"TROWEXPANDMIN", PTOOpcode::TROWEXPANDMIN, PTOEngineKind::Vec},
    {"TROWEXPANDEXPDIF", PTOOpcode::TROWEXPANDEXPDIF, PTOEngineKind::Vec},
    {"TCOLEXPANDDIV", PTOOpcode::TCOLEXPANDDIV, PTOEngineKind::Vec},
    {"TCOLEXPANDMUL", PTOOpcode::TCOLEXPANDMUL, PTOEngineKind::Vec},
    {"TCOLEXPANDSUB", PTOOpcode::TCOLEXPANDSUB, PTOEngineKind::Vec},
    {"TCOLEXPANDADD", PTOOpcode::TCOLEXPANDADD, PTOEngineKind::Vec},
    {"TCOLEXPANDMAX", PTOOpcode::TCOLEXPANDMAX, PTOEngineKind::Vec},
    {"TCOLEXPANDMIN", PTOOpcode::TCOLEXPANDMIN, PTOEngineKind::Vec},
    {"TCOLEXPANDEXPDIF", PTOOpcode::TCOLEXPANDEXPDIF, PTOEngineKind::Vec},
    {"TFILLPAD", PTOOpcode::TFILLPAD, PTOEngineKind::Vec},
    {"TFILLPAD_INPLACE", PTOOpcode::TFILLPAD_INPLACE, PTOEngineKind::Vec},
    {"TFILLPAD_EXPAND", PTOOpcode::TFILLPAD_EXPAND, PTOEngineKind::Vec},
    {"TMATMUL", PTOOpcode::TMATMUL, PTOEngineKind::Cube},
    {"TMATMUL_MX", PTOOpcode::TMATMUL_MX, PTOEngineKind::Cube},
    {"TMATMUL_ACC", PTOOpcode::TMATMUL_ACC, PTOEngineKind::Cube},
    {"TMATMUL_BIAS", PTOOpcode::TMATMUL_BIAS, PTOEngineKind::Cube},
    {"TMATMUL_MX_ACC", PTOOpcode::TMATMUL_MX_ACC, PTOEngineKind::Cube},
    {"TMATMUL_MX_BIAS", PTOOpcode::TMATMUL_MX_BIAS, PTOEngineKind::Cube},
    {"TGEMV", PTOOpcode::TGEMV, PTOEngineKind::Cube},
    {"TGEMV_ACC", PTOOpcode::TGEMV_ACC, PTOEngineKind::Cube},
    {"TGEMV_BIAS", PTOOpcode::TGEMV_BIAS, PTOEngineKind::Cube},
    {"TGEMV_MX", PTOOpcode::TGEMV_MX, PTOEngineKind::Cube},
    {"TMOV", PTOOpcode::TMOV, PTOEngineKind::Vec},
    {"TMOV_FP", PTOOpcode::TMOV_FP, PTOEngineKind::Vec},
    {"TTRANS", PTOOpcode::TTRANS, PTOEngineKind::Vec},
    {"TEXTRACT", PTOOpcode::TEXTRACT, PTOEngineKind::Vec},
    {"TEXTRACT_FP", PTOOpcode::TEXTRACT_FP, PTOEngineKind::Vec},
    {"TRESHAPE", PTOOpcode::TRESHAPE, PTOEngineKind::Scalar},
    {"TASSIGN", PTOOpcode::TASSIGN, PTOEngineKind::Scalar},
    {"TCI", PTOOpcode::TCI, PTOEngineKind::Scalar},
    {"TTRI", PTOOpcode::TTRI, PTOEngineKind::Vec},
    {"TGATHER", PTOOpcode::TGATHER, PTOEngineKind::Vec},
    {"TGATHERB", PTOOpcode::TGATHERB, PTOEngineKind::Vec},
    {"TSCATTER", PTOOpcode::TSCATTER, PTOEngineKind::Vec},
    {"TSORT32", PTOOpcode::TSORT32, PTOEngineKind::Vec},
    {"TMRGSORT", PTOOpcode::TMRGSORT, PTOEngineKind::Vec},
    {"TPARTADD", PTOOpcode::TPARTADD, PTOEngineKind::Vec},
    {"TPARTMUL", PTOOpcode::TPARTMUL, PTOEngineKind::Vec},
    {"TPARTMAX", PTOOpcode::TPARTMAX, PTOEngineKind::Vec},
    {"TPARTMIN", PTOOpcode::TPARTMIN, PTOEngineKind::Vec},
    {"TPARTARGMAX", PTOOpcode::TPARTARGMAX, PTOEngineKind::Vec},
    {"TPARTARGMIN", PTOOpcode::TPARTARGMIN, PTOEngineKind::Vec},
    {"TRANDOM", PTOOpcode::TRANDOM, PTOEngineKind::Vec},
    {"TPRINT", PTOOpcode::TPRINT, PTOEngineKind::Scalar},
    {"TPREFETCH", PTOOpcode::TPREFETCH, PTOEngineKind::Tma},
    {"TIMG2COL", PTOOpcode::TIMG2COL, PTOEngineKind::Tma},
    {"SETFMATRIX", PTOOpcode::SETFMATRIX, PTOEngineKind::Scalar},
    {"SET_IMG2COL_RPT", PTOOpcode::SET_IMG2COL_RPT, PTOEngineKind::Scalar},
    {"SET_IMG2COL_PADDING", PTOOpcode::SET_IMG2COL_PADDING, PTOEngineKind::Scalar},
    {"TCONCAT", PTOOpcode::TCONCAT, PTOEngineKind::Vec},
    {"TDEQUANT", PTOOpcode::TDEQUANT, PTOEngineKind::Vec},
    {"TINSERT", PTOOpcode::TINSERT, PTOEngineKind::Vec},
    {"TINSERT_FP", PTOOpcode::TINSERT_FP, PTOEngineKind::Vec},
    {"TFMOD", PTOOpcode::TFMOD, PTOEngineKind::Vec},
    {"TFMODS", PTOOpcode::TFMODS, PTOEngineKind::Vec},
    {"THISTOGRAM", PTOOpcode::THISTOGRAM, PTOEngineKind::Vec},
    {"TQUANT", PTOOpcode::TQUANT, PTOEngineKind::Vec},
    {"TGET_SCALE_ADDR", PTOOpcode::TGET_SCALE_ADDR, PTOEngineKind::Scalar},
    {"TSUBVIEW", PTOOpcode::TSUBVIEW, PTOEngineKind::Vec},
});

constexpr const PTOOpcodeDescriptor* FindPTOOpcodeDescriptor(std::string_view name) {
  for (const auto& descriptor : kPTOOpcodeDescriptors) {
    if (descriptor.name == name) {
      return &descriptor;
    }
  }
  return nullptr;
}

using PTOScalarValue = std::variant<std::monostate, bool, std::int64_t, std::uint64_t, double>;

struct PTOTileReg {
  PTOTileType tile_type = PTOTileType::Unknown;
  std::uint64_t address = 0;
  std::uint64_t tile_tag = std::numeric_limits<std::uint64_t>::max();
  PTODType dtype = PTODType::kUnknown;
  PTOLayout layout = PTOLayout::kUnknown;
  bool rename_managed = false;
  std::vector<std::int64_t> shape;
};

struct PTOScalarInput {
  PTODType dtype = PTODType::kUnknown;
  PTOScalarValue value;
};

constexpr std::uint64_t kUnsetCycleStamp = std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t kInvalidTileTag = std::numeric_limits<std::uint64_t>::max();

struct PTOInstTimestamps {
  std::uint64_t rob_alloc_cycle = kUnsetCycleStamp;
  std::uint64_t rename_cycle = kUnsetCycleStamp;
  std::uint64_t dispatch_cycle = kUnsetCycleStamp;
  std::uint64_t issue_cycle = kUnsetCycleStamp;
  std::uint64_t engine_pop_cycle = kUnsetCycleStamp;
  std::uint64_t engine_complete_cycle = kUnsetCycleStamp;
  std::uint64_t rob_retire_cycle = kUnsetCycleStamp;
};

struct PTOInst {
  PTOOpcode opcode = PTOOpcode::Unknown;
  std::string raw_opcode;
  PTOEngineKind engine_kind = PTOEngineKind::Scalar;
  std::uint64_t block_idx = 0;
  std::uint64_t sequence_id = 0;
  std::uint64_t rob_id = 0;
  std::uint64_t engine_id = std::numeric_limits<std::uint64_t>::max();
  bool dispatched = false;
  bool src_ready = false;
  std::size_t runtime_latency = 0;
  PTOInstTimestamps timestamps;
  std::vector<bool> tile_input_valid;
  std::vector<bool> tile_input_ready;
  std::vector<PTOTileReg> tile_reg_inputs;
  std::vector<PTOScalarInput> scalar_inputs;
  std::vector<PTOTileReg> tile_reg_outputs;
  std::vector<std::uint64_t> output_replaced_tile_tags;
};

using PTOInstPtr = std::unique_ptr<PTOInst>;
using PTOInstRef = std::shared_ptr<PTOInst>;

struct SimulationResult {
  std::size_t record_count = 0;
  std::size_t simulated_cycles = 0;
  std::size_t rob_capacity = 0;
  std::size_t rob_count = 0;
  std::size_t scalar_engine_count = 0;
  std::size_t vec_engine_count = 0;
  std::size_t cube_engine_count = 0;
  std::size_t tma_engine_count = 0;
  std::vector<PTOInst> processed;
  std::map<std::string, std::size_t> opcode_counts;
};

constexpr std::string_view ToString(PTOOpcode opcode) {
  if (opcode == PTOOpcode::Unknown) {
    return "UNKNOWN";
  }
  for (const auto& descriptor : kPTOOpcodeDescriptors) {
    if (descriptor.opcode == opcode) {
      return descriptor.name;
    }
  }
  return "UNKNOWN";
}

constexpr std::string_view ToString(PTODType dtype) {
  switch (dtype) {
    case PTODType::kBool:
      return "bool";
    case PTODType::kInt8:
      return "int8";
    case PTODType::kInt32:
      return "int32";
    case PTODType::kInt64:
      return "int64";
    case PTODType::kUInt64:
      return "uint64";
    case PTODType::kFloat16:
      return "float16";
    case PTODType::kBFloat16:
      return "bfloat16";
    case PTODType::kFloat32:
      return "float32";
    case PTODType::kFloat8:
      return "float8";
    case PTODType::kFloat4:
      return "float4";
    case PTODType::kUnknown:
    default:
      return "unknown";
  }
}

constexpr std::string_view ToString(PTOTileType tile_type) {
  switch (tile_type) {
    case PTOTileType::Vec:
      return "Vec";
    case PTOTileType::Mat:
      return "Mat";
    case PTOTileType::Left:
      return "Left";
    case PTOTileType::Right:
      return "Right";
    case PTOTileType::Acc:
      return "Acc";
    case PTOTileType::Bias:
      return "Bias";
    case PTOTileType::Scaling:
      return "Scaling";
    case PTOTileType::ScaleLeft:
      return "ScaleLeft";
    case PTOTileType::ScaleRight:
      return "ScaleRight";
    case PTOTileType::Ctrl:
      return "Ctrl";
    case PTOTileType::Global:
      return "Global";
    case PTOTileType::Unknown:
    default:
      return "Unknown";
  }
}

constexpr std::string_view ToString(PTOLayout layout) {
  switch (layout) {
    case PTOLayout::kND:
      return "ND";
    case PTOLayout::kDN:
      return "DN";
    case PTOLayout::kNZ:
      return "NZ";
    case PTOLayout::kScale:
      return "SCALE";
    case PTOLayout::kMxAND:
      return "MX_A_ND";
    case PTOLayout::kMxADN:
      return "MX_A_DN";
    case PTOLayout::kMxAZZ:
      return "MX_A_ZZ";
    case PTOLayout::kMxBND:
      return "MX_B_ND";
    case PTOLayout::kMxBDN:
      return "MX_B_DN";
    case PTOLayout::kMxBNN:
      return "MX_B_NN";
    case PTOLayout::kNC1HWC0:
      return "NC1HWC0";
    case PTOLayout::kNCHW:
      return "NCHW";
    case PTOLayout::kNHWC:
      return "NHWC";
    case PTOLayout::kNDC1HWC0:
      return "NDC1HWC0";
    case PTOLayout::kNCDHW:
      return "NCDHW";
    case PTOLayout::kFractalZ:
      return "FRACTAL_Z";
    case PTOLayout::kFractalZS16S8:
      return "FRACTAL_Z_S16S8";
    case PTOLayout::kFractalZ3D:
      return "FRACTAL_Z_3D";
    case PTOLayout::kUnknown:
    default:
      return "UNKNOWN";
  }
}

constexpr std::string_view ToString(PTOEngineKind kind) {
  switch (kind) {
    case PTOEngineKind::Scalar:
      return "SCALAR";
    case PTOEngineKind::Vec:
      return "VEC";
    case PTOEngineKind::Cube:
      return "CUBE";
    case PTOEngineKind::Tma:
      return "TMA";
    default:
      return "SCALAR";
  }
}

std::string OpcodeName(const PTOInst& inst);
std::string DumpPTOInst(const PTOInst& inst);

}  // namespace davincioo
