#include "StructuralCppBackend.h"
#include "pycircuit/Dialect/ACIR/FinalHardwarePackage.h"

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/HWIR/HWIRDialect.h"
#include "pycircuit/Dialect/HWIR/HWIRVerification.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/raw_ostream.h"

#include <limits>

using namespace mlir;

namespace acir::compiler {
namespace {

void emitCppStringLiteral(raw_ostream &out, StringRef value) {
  out << '"';
  out.write_escaped(value, false);
  out << '"';
}

LogicalResult
emitPlan(const ConcreteDesignPlan &plan, raw_ostream &out,
         SmallVectorImpl<StructuralCppWorkShard> *workShards = nullptr,
         bool emitCombinedWork = false) {
  constexpr size_t kWorkChunkPreferredBytes = 128 * 1024;
  constexpr size_t kWorkChunkMaximumBytes = 256 * 1024;
  constexpr size_t kExternalExecutionThresholdBytes = 512 * 1024;
  constexpr size_t kGeneratedFunctionMaximumBytes = 512 * 1024;
  constexpr size_t kGeneratedTranslationUnitMaximumBytes = 1024 * 1024;
  constexpr StringLiteral kWorkShardPreamble = "#include \"design.hpp\"\n\n";
  SmallVector<unsigned, 0> moduleSourceUnits(
      plan.modules.size(), std::numeric_limits<unsigned>::max());
  for (auto [sourceUnit, unit] : llvm::enumerate(plan.sourceUnits))
    for (unsigned moduleIndex : unit.moduleIndices) {
      if (moduleIndex >= moduleSourceUnits.size())
        return failure();
      moduleSourceUnits[moduleIndex] = sourceUnit;
    }
  SmallVector<std::string, 0> pendingWorkShards(plan.sourceUnits.size());
  SmallVector<unsigned, 0> nextWorkShard(plan.sourceUnits.size(), 0);
  std::string combinedWork;
  auto flushWorkShard = [&](unsigned sourceUnit) -> LogicalResult {
    if (!workShards || sourceUnit >= pendingWorkShards.size() ||
        pendingWorkShards[sourceUnit].empty())
      return success();
    if (kWorkShardPreamble.size() + pendingWorkShards[sourceUnit].size() >
        kGeneratedTranslationUnitMaximumBytes)
      return failure();
    const unsigned shard = nextWorkShard[sourceUnit]++;
    workShards->push_back(
        {sourceUnit, shard,
         ("work_unit_" + Twine(sourceUnit) + "_" + Twine(shard) + ".cpp").str(),
         kWorkShardPreamble.str() + std::move(pendingWorkShards[sourceUnit])});
    pendingWorkShards[sourceUnit].clear();
    return success();
  };
  auto appendWorkDefinition = [&](unsigned moduleIndex,
                                  StringRef definition) -> LogicalResult {
    if (!workShards && !emitCombinedWork)
      return success();
    if (definition.size() > kGeneratedFunctionMaximumBytes)
      return failure();
    if (emitCombinedWork) {
      combinedWork.append(definition);
      return success();
    }
    if (moduleIndex >= moduleSourceUnits.size() ||
        moduleSourceUnits[moduleIndex] == std::numeric_limits<unsigned>::max())
      return failure();
    const unsigned sourceUnit = moduleSourceUnits[moduleIndex];
    std::string &text = pendingWorkShards[sourceUnit];
    if (!text.empty() &&
        kWorkShardPreamble.size() + text.size() + definition.size() >
            kGeneratedTranslationUnitMaximumBytes)
      if (failed(flushWorkShard(sourceUnit)))
        return failure();
    if (kWorkShardPreamble.size() + text.size() + definition.size() >
        kGeneratedTranslationUnitMaximumBytes)
      return failure();
    text.append(definition);
    return success();
  };
  llvm::StringSet<> runtimeHeaders;
  for (const ConcreteModulePlan &module : plan.modules)
    for (const ConcreteChildPlan &child : module.children)
      if (child.primitive && !child.primitive->runtimeHeader.empty())
        runtimeHeaders.insert(child.primitive->runtimeHeader);
  out << "#include <gfsim/SimModule.h>\n#include <gfsim/wire.h>\n"
         "#include <gfsim/bits.h>\n"
         "#include <gfsim/four_state.h>\n"
         "#include <gfsim/ObservationSlot.h>\n"
         "#include <gfsim/SimSystem.h>\n";
  for (const auto &header : runtimeHeaders)
    out << "#include <" << header.getKey() << ">\n";
  out << "#include <array>\n#include <cstddef>\n#include <stdexcept>\n"
         "#include <string>\n"
         "#include <utility>\n\n"
         "#if defined(_MSC_VER)\n"
         "#define PYCIRCUIT_NOINLINE __declspec(noinline)\n"
         "#else\n"
         "#define PYCIRCUIT_NOINLINE __attribute__((noinline))\n"
         "#endif\n\n"
         "struct pyc_observation_sink {\n"
         "  gfsim::ObservationSlots *slots = nullptr;\n"
         "  const gfsim::SimSystem *system = nullptr;\n"
         "};\n\n";
  for (const ConcreteNamedTypePlan &type : plan.namedTypes) {
    StringRef qualified(type.cppQualifiedName);
    qualified.consume_front("::");
    SmallVector<StringRef> parts;
    qualified.split(parts, "::");
    if (parts.empty())
      return failure();
    for (StringRef name : ArrayRef<StringRef>(parts).drop_back())
      out << "namespace " << name << " { ";
    out << "using " << parts.back() << " = gfsim::Bits<" << type.packedWidth
        << ">;";
    for (size_t index = 1; index < parts.size(); ++index)
      out << " }";
    out << "\n";
  }
  if (!plan.namedTypes.empty())
    out << "\n";
  for (const ConcreteModulePlan &module : llvm::reverse(plan.modules)) {
    const unsigned moduleIndex =
        static_cast<unsigned>(&module - plan.modules.data());
    out << "class " << module.cppName
        << " final : public gfsim::SimModule {\n"
           "public:\n  explicit "
        << module.cppName
        << "(std::string name, pyc_observation_sink *pyc_sink = nullptr, "
           "std::size_t pyc_observation_base = 0, "
           "bool pyc_refresh_outputs_on_xfer = true, "
           "bool pyc_host_driven = false);\n";
    for (const ConcretePortPlan &port : module.inputs)
      out << "  gfsim::wire<" << port.type.cppTypeName << "> " << port.cppName
          << ";\n";
    for (const ConcretePortPlan &port : module.outputs)
      out << "  gfsim::wire<" << port.type.cppTypeName << "> " << port.cppName
          << ";\n";
    for (const ConcreteValuePlan &value : module.values)
      if (value.kind == ConcreteValueKind::OperationResult)
        out << "  gfsim::wire<" << value.type.cppTypeName << "> "
            << value.cppName << ";\n";
    for (const ConcreteChildPlan &child : module.children) {
      if (child.kind == ConcreteChildKind::Module) {
        if (!child.moduleIndex)
          return failure();
        out << "  " << plan.modules[*child.moduleIndex].cppName << " "
            << child.cppName << ";\n";
      } else if (child.primitive && !child.primitive->runtimeCppType.empty()) {
        if (child.primitive->inputMembers.size() != child.inputs.size() ||
            child.primitive->outputMembers.size() != child.outputs.size())
          return child.occurrence->emitOpError()
                 << "structural C++ primitive plan has an incomplete pin "
                    "schema";
        out << "  " << child.primitive->runtimeCppType << " " << child.cppName
            << ";\n";
      } else {
        return child.occurrence->emitOpError()
               << "structural C++ vertical slice does not support primitive '"
               << child.primitiveKind.getValue() << "'";
      }
    }
    out << "  pyc_observation_sink *pyc_sink_ = nullptr;\n"
           "  std::size_t pyc_observation_base_ = 0;\n"
           "  bool pyc_refresh_outputs_on_xfer_ = true;\n"
           "  bool pyc_host_driven_ = false;\n";
    out << "  mutable gfsim::SimFailureInfo pyc_check_failure_{};\n";
    const bool hasGuardChecks =
        llvm::any_of(plan.checks, [&](const ConcreteCheckPlan &check) {
          return check.moduleIndex ==
                     static_cast<unsigned>(&module - plan.modules.data()) &&
                 check.kind == "guard_known";
        });
    if (hasGuardChecks)
      out << "  bool pyc_sampling_clock_ = false;\n";
    auto emitSteps = [&](ArrayRef<ConcreteScheduleStep> steps,
                         raw_ostream &out) -> LogicalResult {
      for (const ConcreteScheduleStep &step : steps) {
        if (step.kind == ConcreteScheduleStepKind::ComputeValue) {
          if (!step.compute ||
              ((step.compute->kind == ConcreteComputeKind::TableGet ||
                step.compute->kind == ConcreteComputeKind::TableChoose ||
                step.compute->kind == ConcreteComputeKind::ValueMerge ||
                step.compute->kind == ConcreteComputeKind::EnumFromBits)
                   ? (step.compute->kind == ConcreteComputeKind::TableChoose
                          ? step.outputs.size() != *step.compute->immediate * 2
                          : step.outputs.size() != 2)
                   : step.outputs.size() != 1))
            return step.operation->emitOpError()
                   << "structural C++ vertical slice lacks a compute mapping";
          const std::string result =
              module.values[step.outputs.front()].cppName;
          const auto &compute = *step.compute;
          if (compute.kind == ConcreteComputeKind::BitsConstant) {
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::known("
                << module.values[step.outputs.front()].type.cppTypeName << "{";
            for (auto [word, value] : llvm::enumerate(compute.constantWords)) {
              if (word)
                out << ", ";
              out << value << "ULL";
            }
            out << "});\n";
          } else if (compute.kind == ConcreteComputeKind::BitsConstant4) {
            const auto &type = module.values[step.outputs.front()].type;
            auto emitWords = [&](ArrayRef<uint64_t> words) {
              out << type.cppTypeName << "{";
              for (auto [word, value] : llvm::enumerate(words)) {
                if (word)
                  out << ", ";
                out << value << "ULL";
              }
              out << "}";
            };
            out << "    " << result << " = gfsim::wire<" << type.cppTypeName
                << ">::fromPacked(gfsim::FourState<" << type.packedWidth
                << ">::fromMasks(";
            emitWords(compute.constantWords);
            out << ", ";
            emitWords(compute.knownWords);
            out << ", ";
            emitWords(compute.zWords);
            out << "));\n";
          } else if (compute.kind == ConcreteComputeKind::BitsUnary) {
            if (step.inputs.size() != 1 || compute.opcode != "bit_not")
              return step.operation->emitOpError()
                     << "structural C++ slice only supports unary not";
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::fromPacked(gfsim::bit_not("
                << module.values[step.inputs[0]].cppName << ".packed()));\n";
          } else if (compute.kind == ConcreteComputeKind::BitsBinary) {
            if (step.inputs.size() != 2)
              return failure();
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::fromPacked(gfsim::" << compute.opcode << "("
                << module.values[step.inputs[0]].cppName << ".packed(), "
                << module.values[step.inputs[1]].cppName << ".packed()));\n";
          } else if (compute.kind == ConcreteComputeKind::BitsCompare) {
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::fromPacked(gfsim::compare<gfsim::ComparePredicate::"
                << compute.opcode << ">("
                << module.values[step.inputs[0]].cppName << ".packed(), "
                << module.values[step.inputs[1]].cppName << ".packed()));\n";
          } else if (compute.kind == ConcreteComputeKind::BitsSelect) {
            if (step.inputs.size() != 3)
              return failure();
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::fromPacked(gfsim::select("
                << module.values[step.inputs[0]].cppName << ".packed(), "
                << module.values[step.inputs[1]].cppName << ".packed(), "
                << module.values[step.inputs[2]].cppName << ".packed()));\n";
          } else if (compute.kind == ConcreteComputeKind::BitsConcat) {
            if (compute.opcode == "splat") {
              if (step.inputs.size() != 1 || !compute.immediate ||
                  *compute.immediate == 0)
                return failure();
              out << "    " << result << " = gfsim::wire<"
                  << module.values[step.outputs.front()].type.cppTypeName
                  << ">::fromPacked(";
              for (uint64_t lane = 1; lane < *compute.immediate; ++lane)
                out << "gfsim::concat(";
              out << module.values[step.inputs[0]].cppName << ".packed()";
              for (uint64_t lane = 1; lane < *compute.immediate; ++lane)
                out << ", " << module.values[step.inputs[0]].cppName
                    << ".packed())";
              out << ");\n";
              continue;
            }
            std::function<void(unsigned)> emitConcat = [&](unsigned input) {
              if (input + 1 == step.inputs.size()) {
                out << module.values[step.inputs[input]].cppName << ".packed()";
                return;
              }
              out << "gfsim::concat("
                  << module.values[step.inputs[input]].cppName << ".packed(), ";
              emitConcat(input + 1);
              out << ")";
            };
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::fromPacked(";
            emitConcat(0);
            out << ");\n";
          } else if (compute.kind == ConcreteComputeKind::BitsExtract) {
            const uint64_t outputWidth =
                module.values[step.outputs.front()].type.packedWidth;
            const uint64_t inputWidth =
                module.values[step.inputs.front()].type.packedWidth;
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::fromPacked(gfsim::extract<" << outputWidth << ", "
                << inputWidth << ">(" << module.values[step.inputs[0]].cppName
                << ".packed(), " << *compute.immediate << "));\n";
          } else if (compute.kind == ConcreteComputeKind::BitsResize) {
            const uint64_t outputWidth =
                module.values[step.outputs.front()].type.packedWidth;
            const uint64_t inputWidth =
                module.values[step.inputs.front()].type.packedWidth;
            StringRef function = compute.opcode == "trunc"  ? "truncate"
                                 : compute.opcode == "zext" ? "zero_extend"
                                                            : "sign_extend";
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::fromPacked(gfsim::" << function << "<" << outputWidth
                << ", " << inputWidth << ">("
                << module.values[step.inputs[0]].cppName << ".packed()));\n";
          } else if (compute.kind == ConcreteComputeKind::PackedIdentity) {
            out << "    " << result << " = gfsim::wire<"
                << module.values[step.outputs.front()].type.cppTypeName
                << ">::fromPacked(" << module.values[step.inputs[0]].cppName
                << ".packed());\n";
          } else if (compute.kind == ConcreteComputeKind::EnumFromBits) {
            const auto &input = module.values[step.inputs[0]];
            const auto &value = module.values[step.outputs[0]];
            const auto &member = module.values[step.outputs[1]];
            out << "    " << value.cppName << " = gfsim::wire<"
                << value.type.cppTypeName << ">::fromPacked(" << input.cppName
                << ".packed());\n";
            out << "    " << member.cppName << " = gfsim::wire<"
                << member.type.cppTypeName << ">::fromPacked(";
            auto emitEquality = [&](ArrayRef<uint64_t> words) {
              out << "gfsim::compare<gfsim::ComparePredicate::EQ>("
                  << input.cppName << ".packed(), gfsim::FourState<"
                  << input.type.packedWidth << ">::known(gfsim::Bits<"
                  << input.type.packedWidth << ">{";
              for (auto [wordIndex, word] : llvm::enumerate(words)) {
                if (wordIndex)
                  out << ", ";
                out << word << "ULL";
              }
              out << "}))";
            };
            std::function<void(size_t)> emitMembership = [&](size_t index) {
              if (index + 1 == compute.enumCodes.size()) {
                emitEquality(compute.enumCodes[index]);
                return;
              }
              out << "gfsim::bit_or(";
              emitEquality(compute.enumCodes[index]);
              out << ", ";
              emitMembership(index + 1);
              out << ")";
            };
            emitMembership(0);
            out << " );\n";
          } else if (compute.kind == ConcreteComputeKind::TableCreate) {
            const auto &output = module.values[step.outputs[0]];
            for (auto [lane, input] : llvm::enumerate(step.inputs))
              out << "    " << output.cppName << ".element(" << lane
                  << ") = " << module.values[input].cppName << ";\n";
          } else if (compute.kind == ConcreteComputeKind::TableSplat) {
            const auto &input = module.values[step.inputs[0]];
            const auto &output = module.values[step.outputs[0]];
            uint64_t lanes = output.type.packedWidth / input.type.packedWidth;
            for (uint64_t lane = 0; lane < lanes; ++lane)
              out << "    " << output.cppName << ".element(" << lane
                  << ") = " << input.cppName << ";\n";
          } else if (compute.kind == ConcreteComputeKind::TableGet) {
            const auto &table = module.values[step.inputs[0]];
            const auto &index = module.values[step.inputs[1]];
            const auto &value = module.values[step.outputs[0]];
            const auto &valid = module.values[step.outputs[1]];
            uint64_t lanes = table.type.packedWidth / value.type.packedWidth;
            out << "    if (" << index.cppName << ".packed().isFullyKnown() && "
                << index.cppName << ".packed().value().value() < " << lanes
                << ") {\n"
                << "      " << value.cppName << " = " << table.cppName
                << ".element(" << index.cppName
                << ".packed().value().value());\n"
                << "      " << valid.cppName
                << " = gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{1});\n"
                << "    } else {\n      " << value.cppName << " = gfsim::wire<"
                << value.type.cppTypeName << ">::unknown();\n      "
                << valid.cppName
                << " = gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{0});\n"
                << "    }\n";
          } else if (compute.kind == ConcreteComputeKind::TableView) {
            const auto &input = module.values[step.inputs[0]];
            const auto &output = module.values[step.outputs[0]];
            for (auto [lane, source] : llvm::enumerate(compute.laneMap))
              out << "    " << output.cppName << ".element(" << lane
                  << ") = " << input.cppName << ".element(" << source << ");\n";
          } else if (compute.kind == ConcreteComputeKind::TableIndex) {
            const auto &output = module.values[step.outputs[0]];
            const unsigned sumWidth =
                std::max<uint64_t>(output.type.packedWidth + 1, 65);
            out << "    { auto pyc_bounds = "
                   "gfsim::FourState<1>::known(gfsim::Bits<1>{1});\n"
                << "      auto pyc_sum = gfsim::FourState<" << sumWidth
                << ">::known(gfsim::Bits<" << sumWidth << ">{0});\n";
            uint64_t stride = 1;
            llvm::SmallVector<uint64_t> strides(compute.laneMap.size());
            for (size_t i = compute.laneMap.size(); i-- > 0;) {
              strides[i] = stride;
              stride *= compute.laneMap[i];
            }
            for (auto [i, inputId] : llvm::enumerate(step.inputs)) {
              const auto &input = module.values[inputId];
              unsigned compareWidth =
                  std::max<uint64_t>(input.type.packedWidth, 65);
              out << "      pyc_bounds = gfsim::bit_and(pyc_bounds, "
                     "gfsim::compare<gfsim::ComparePredicate::ULT>(gfsim::zero_"
                     "extend<"
                  << compareWidth << ">(" << input.cppName
                  << ".packed()), gfsim::FourState<" << compareWidth
                  << ">::known(gfsim::Bits<" << compareWidth << ">{"
                  << compute.laneMap[i] << "})));\n"
                  << "      pyc_sum = gfsim::add(pyc_sum, "
                     "gfsim::mul(gfsim::zero_extend<"
                  << sumWidth << ">(" << input.cppName
                  << ".packed()), gfsim::FourState<" << sumWidth
                  << ">::known(gfsim::Bits<" << sumWidth << ">{" << strides[i]
                  << "})));\n";
            }
            out << "      " << output.cppName << " = gfsim::wire<"
                << output.type.cppTypeName
                << ">::fromPacked(gfsim::select(pyc_bounds, gfsim::truncate<"
                << output.type.packedWidth << ">(pyc_sum), gfsim::FourState<"
                << output.type.packedWidth << ">::known(gfsim::Bits<"
                << output.type.packedWidth << ">{" << stride << "}))); }\n";
          } else if (compute.kind == ConcreteComputeKind::TableMatch) {
            const auto &output = module.values[step.outputs[0]];
            out << "    " << output.cppName << " = gfsim::wire<"
                << output.type.cppTypeName << ">::known("
                << output.type.cppTypeName << "{0});\n";
            for (auto [lane, inputId] : llvm::enumerate(step.inputs))
              out << "    " << output.cppName << ".assignSlice(" << lane << ", "
                  << module.values[inputId].cppName << ".packed());\n";
          } else if (compute.kind == ConcreteComputeKind::TableChoose) {
            const auto &mask = module.values[step.inputs[1]];
            uint64_t count = *compute.immediate;
            const auto &indexType = module.values[step.outputs[0]].type;
            out << "    { auto pyc_remaining = " << mask.cppName
                << ".packed();\n";
            for (uint64_t rank = 0; rank < count; ++rank) {
              out << "      auto pyc_index_" << rank << " = gfsim::FourState<"
                  << indexType.packedWidth << ">::known(gfsim::Bits<"
                  << indexType.packedWidth << ">{0});\n"
                  << "      auto pyc_valid_" << rank
                  << " = gfsim::FourState<1>::known(gfsim::Bits<1>{0});\n";
              if (compute.opcode == "low")
                out << "      for (std::size_t pyc_bit = "
                    << mask.type.packedWidth << "; pyc_bit-- > 0;) {\n";
              else
                out << "      for (std::size_t pyc_bit = 0; pyc_bit < "
                    << mask.type.packedWidth << "; ++pyc_bit) {\n";
              out << "        auto pyc_hit = gfsim::extract<1>(pyc_remaining, "
                     "pyc_bit);\n"
                  << "        pyc_valid_" << rank
                  << " = gfsim::bit_or(pyc_valid_" << rank << ", pyc_hit);\n"
                  << "        pyc_index_" << rank
                  << " = gfsim::select(pyc_hit, gfsim::FourState<"
                  << indexType.packedWidth << ">::known(gfsim::Bits<"
                  << indexType.packedWidth << ">{pyc_bit}), pyc_index_" << rank
                  << "); }\n"
                  << "      " << module.values[step.outputs[rank]].cppName
                  << " = gfsim::wire<" << indexType.cppTypeName
                  << ">::fromPacked(pyc_index_" << rank << ");\n"
                  << "      "
                  << module.values[step.outputs[count + rank]].cppName
                  << " = gfsim::wire<gfsim::Bits<1>>::fromPacked(pyc_valid_"
                  << rank << ");\n"
                  << "      for (std::size_t pyc_bit = 0; pyc_bit < "
                  << mask.type.packedWidth
                  << "; ++pyc_bit) { auto pyc_remove = "
                     "gfsim::bit_and(pyc_valid_"
                  << rank
                  << ", gfsim::compare<gfsim::ComparePredicate::EQ>(pyc_index_"
                  << rank << ", gfsim::FourState<" << indexType.packedWidth
                  << ">::known(gfsim::Bits<" << indexType.packedWidth
                  << ">{pyc_bit}))); pyc_remaining.assignSlice(pyc_bit, "
                     "gfsim::bit_and(gfsim::extract<1>(pyc_remaining, "
                     "pyc_bit), gfsim::bit_not(pyc_remove))); }\n";
            }
            out << "    }\n";
          } else if (compute.kind == ConcreteComputeKind::TableFold) {
            const auto &input = module.values[step.inputs[0]];
            const auto &output = module.values[step.outputs[0]];
            uint64_t lanes = input.type.packedWidth / output.type.packedWidth;
            out << "    { std::array<gfsim::wire<" << output.type.cppTypeName
                << ">, " << lanes << "> pyc_tree;\n"
                << "      for (std::size_t i = 0; i < " << lanes
                << "; ++i) pyc_tree[i] = " << input.cppName << ".element(i);\n"
                << "      for (std::size_t n = " << lanes
                << "; n > 1; n = (n + 1) / 2) { for (std::size_t p = 0; p < n "
                   "/ 2; ++p) { auto a = pyc_tree[2*p].packed(), b = "
                   "pyc_tree[2*p+1].packed(); pyc_tree[p] = gfsim::wire<"
                << output.type.cppTypeName << ">::fromPacked(";
            if (compute.opcode == "min" || compute.opcode == "max")
              out << "gfsim::select(gfsim::compare<gfsim::ComparePredicate::"
                  << (compute.opcode == "min" ? "ULE" : "UGE") << ">(a,b),a,b)";
            else
              out << "gfsim::"
                  << llvm::StringSwitch<StringRef>(compute.opcode)
                         .Case("and", "bit_and")
                         .Case("or", "bit_or")
                         .Case("xor", "bit_xor")
                         .Default(compute.opcode)
                  << "(a,b)";
            out << "); } if (n % 2) pyc_tree[n/2] = pyc_tree[n-1]; } "
                << output.cppName << " = pyc_tree[0]; }\n";
          } else if (compute.kind == ConcreteComputeKind::ValueMerge) {
            const auto &base = module.values[step.inputs[0]];
            const auto &next = module.values[step.outputs[0]];
            const auto &enable = module.values[step.outputs[1]];
            const std::string mergeEnable =
                "pyc_merge_en_" + std::to_string(step.outputs[1]);
            out << "    " << next.cppName << " = " << base.cppName << ";\n"
                << "    auto " << mergeEnable
                << " = gfsim::FourState<1>::known(gfsim::Bits<1>{0});\n";
            uint64_t writes = (step.inputs.size() - 1) / 2;
            for (uint64_t i = 0; i < writes; ++i) {
              const auto &guard = module.values[step.inputs[1 + i]];
              const auto &value = module.values[step.inputs[1 + writes + i]];
              uint64_t low = compute.laneMap[2 * i],
                       width = compute.laneMap[2 * i + 1];
              out << "    " << next.cppName << ".assignSlice(" << low
                  << ", gfsim::select(" << guard.cppName << ".packed(), "
                  << value.cppName << ".packed(), gfsim::extract<" << width
                  << ">(" << base.cppName << ".packed(), " << low << ")));\n"
                  << "    " << mergeEnable << " = gfsim::bit_or(" << mergeEnable
                  << ", " << guard.cppName << ".packed());\n";
            }
            out << "    " << enable.cppName
                << " = gfsim::wire<gfsim::Bits<1>>::fromPacked(" << mergeEnable
                << ");\n";
          } else {
            return step.operation->emitOpError()
                   << "structural C++ renderer has no mapping for planned "
                      "compute kind "
                   << static_cast<unsigned>(compute.kind);
          }
        } else if (step.kind == ConcreteScheduleStepKind::AvailabilityBarrier) {
          // Plan-only dependency edge: lane writes already updated the aliased
          // table storage; the barrier merely publishes whole-table
          // availability.
        } else if (step.kind == ConcreteScheduleStepKind::DriveChildInput ||
                   step.kind == ConcreteScheduleStepKind::DrivePrimitiveInput) {
          const auto &child = module.children[*step.childIndex];
          const auto &binding = child.inputs[*step.portIndex];
          StringRef pin = binding.cppName;
          if (child.kind != ConcreteChildKind::Module) {
            if (!child.primitive ||
                *step.portIndex >= child.primitive->inputMembers.size())
              return step.operation->emitOpError()
                     << "structural C++ primitive input is absent from the "
                        "plan";
            pin = child.primitive->inputMembers[*step.portIndex];
          } else if (child.moduleIndex) {
            pin = plan.modules[*child.moduleIndex]
                      .inputs[*step.portIndex]
                      .cppName;
          }
          out << "    " << child.cppName << "." << pin << " = "
              << module.values[binding.value].cppName << ";\n";
        } else if (step.kind == ConcreteScheduleStepKind::InvokeChildWork ||
                   step.kind == ConcreteScheduleStepKind::InvokePrimitiveWork) {
          out << "    " << module.children[*step.childIndex].cppName
              << ".Work();\n";
        } else if (step.kind == ConcreteScheduleStepKind::ReadChildOutput ||
                   step.kind ==
                       ConcreteScheduleStepKind::ReadChildOutputField ||
                   step.kind == ConcreteScheduleStepKind::ReadPrimitiveOutput) {
          const auto &child = module.children[*step.childIndex];
          const auto &binding = child.outputs[*step.portIndex];
          StringRef pin = binding.cppName;
          if (child.kind != ConcreteChildKind::Module) {
            if (!child.primitive ||
                *step.portIndex >= child.primitive->outputMembers.size())
              return step.operation->emitOpError()
                     << "structural C++ primitive output is absent from the "
                        "plan";
            pin = child.primitive->outputMembers[*step.portIndex];
            if (!child.primitive->outputIsTemporal[*step.portIndex] &&
                child.primitive->hasPureOutputRefresh)
              out << "    " << child.cppName << ".RefreshOutputs();\n";
          } else if (child.moduleIndex) {
            pin = plan.modules[*child.moduleIndex]
                      .outputs[*step.portIndex]
                      .cppName;
            if (step.kind == ConcreteScheduleStepKind::ReadChildOutputField)
              out << "    " << child.cppName << ".RefreshPureOutputs();\n";
          }
          if (step.kind == ConcreteScheduleStepKind::ReadChildOutputField) {
            const auto &value = module.values[step.outputs.front()];
            const auto &whole =
                plan.modules[*child.moduleIndex].outputs[*step.portIndex].type;
            out << "    " << value.cppName << " = gfsim::wire<"
                << value.type.cppTypeName << ">::fromPacked(gfsim::extract<"
                << value.type.packedWidth << ", " << whole.packedWidth << ">("
                << child.cppName << "." << pin << ".packed(), "
                << *step.fieldLow << "));\n";
          } else {
            out << "    " << module.values[binding.value].cppName << " = "
                << child.cppName << "." << pin << ";\n";
          }
        } else if (step.kind == ConcreteScheduleStepKind::MaterializeOutput) {
          if (step.fieldLow) {
            const auto &value = module.values[step.inputs.front()];
            out << "    " << module.outputs[*step.portIndex].cppName
                << ".assignSlice(" << *step.fieldLow << ", " << value.cppName
                << ".packed());\n";
          } else {
            out << "    " << module.outputs[*step.portIndex].cppName << " = "
                << module.values[step.inputs.front()].cppName << ";\n";
          }
        }
      }
      return success();
    };
    // Keep the public Work entry point small.  The old emitter placed every
    // scheduled operation in this one function, which made an ARM64 `bl` to
    // the next symbol exceed the +/-128 MiB branch range at -O0.  Values are
    // already persistent module members, so schedule-boundary helpers are the
    // scratch space: no semantic state has to be copied between chunks.
    struct WorkChunk {
      std::string name;
      std::string body;
      bool bindsInputs = false;
    };
    SmallVector<WorkChunk, 0> workChunks;
    std::string scheduleBody;
    auto finishScheduleChunk = [&] {
      if (scheduleBody.empty())
        return;
      workChunks.push_back(
          {("pyc_work_chunk_" + Twine(workChunks.size())).str(),
           std::move(scheduleBody), true});
      scheduleBody.clear();
    };
    auto isSemanticBoundary = [](ConcreteScheduleStepKind kind) {
      return kind == ConcreteScheduleStepKind::AvailabilityBarrier ||
             kind == ConcreteScheduleStepKind::InvokeChildWork ||
             kind == ConcreteScheduleStepKind::InvokePrimitiveWork ||
             kind == ConcreteScheduleStepKind::MaterializeOutput;
    };
    for (const ConcreteScheduleStep &step : module.concreteSchedule.steps) {
      std::string rendered;
      llvm::raw_string_ostream renderedStream(rendered);
      if (failed(emitSteps(ArrayRef<ConcreteScheduleStep>(&step, 1),
                           renderedStream)))
        return failure();
      renderedStream.flush();
      if (rendered.size() > kWorkChunkMaximumBytes)
        return failure();
      if (!scheduleBody.empty() &&
          scheduleBody.size() + rendered.size() > kWorkChunkMaximumBytes)
        finishScheduleChunk();
      scheduleBody.append(rendered);
      if (scheduleBody.size() >= kWorkChunkPreferredBytes &&
          isSemanticBoundary(step.kind))
        finishScheduleChunk();
    }
    finishScheduleChunk();

    std::string observationBody;
    auto finishObservationChunk = [&] {
      if (observationBody.empty())
        return;
      workChunks.push_back(
          {("pyc_work_observations_" + Twine(workChunks.size())).str(),
           std::move(observationBody), true});
      observationBody.clear();
    };
    for (auto [siteIndex, site] : llvm::enumerate(module.observationSites)) {
      const ConcreteValuePlan &value = module.values[site.value];
      const ConcreteValuePlan &path = module.values[site.path];
      std::string rendered;
      llvm::raw_string_ostream renderedStream(rendered);
      renderedStream
          << "    if (pyc_sink_ != nullptr && pyc_sink_->slots != nullptr && "
             "pyc_sink_->system != nullptr) {\n"
             "      const auto &pyc_observed_"
          << siteIndex << " = " << value.cppName << ".packed();\n"
          << "      const auto &pyc_path_" << siteIndex << " = " << path.cppName
          << ".packed();\n"
          << "      if (!pyc_sink_->slots->StagePacked(pyc_observation_base_ + "
          << siteIndex << ", pyc_observed_" << siteIndex << ", "
          << (site.kind == ConcreteObservationKind::Event &&
                      site.payloadWidth == 1
                  ? "true"
                  : "false")
          << ", pyc_path_" << siteIndex << ".isFullyKnown(), pyc_path_"
          << siteIndex << ".isFullyKnown() && static_cast<bool>(pyc_path_"
          << siteIndex
          << ".value().value() & 1u), pyc_sink_->system->cycle()))\n"
             "        throw std::logic_error(\"structural observation staging "
             "failed\");\n"
             "    }\n";
      renderedStream.flush();
      if (rendered.size() > kWorkChunkMaximumBytes)
        return failure();
      if (!observationBody.empty() &&
          observationBody.size() + rendered.size() > kWorkChunkMaximumBytes)
        finishObservationChunk();
      observationBody.append(rendered);
    }
    finishObservationChunk();

    size_t workSourceBytes = 0;
    for (const WorkChunk &chunk : workChunks)
      workSourceBytes += chunk.body.size();
    // Preserve the lightweight header-only shape for ordinary modules.  Very
    // large modules move helpers to source-owned shards so their C++ frontend
    // and optimizer work can run independently.
    const bool externalizeWork =
        emitCombinedWork || workSourceBytes > kExternalExecutionThresholdBytes;

    std::string workDispatcher;
    llvm::raw_string_ostream workDispatcherStream(workDispatcher);
    workDispatcherStream << "  void Work() override {\n";
    for (const WorkChunk &chunk : workChunks)
      workDispatcherStream << "    " << chunk.name << "();\n";
    workDispatcherStream << "  }\n";
    workDispatcherStream.flush();
    if (workDispatcher.size() > kGeneratedFunctionMaximumBytes)
      return failure();
    out << workDispatcher;
    for (const WorkChunk &chunk : workChunks) {
      if (!externalizeWork) {
        std::string definition;
        llvm::raw_string_ostream definitionStream(definition);
        definitionStream << "  PYCIRCUIT_NOINLINE void " << chunk.name
                         << "() {\n";
        if (chunk.bindsInputs)
          for (const ConcretePortPlan &port : module.inputs)
            definitionStream << "    auto &input_" << port.value << " = "
                             << port.cppName << ";\n";
        definitionStream << chunk.body << "  }\n";
        definitionStream.flush();
        if (definition.size() > kGeneratedFunctionMaximumBytes)
          return failure();
        out << definition;
        continue;
      }
      out << "  PYCIRCUIT_NOINLINE void " << chunk.name << "();\n";
      std::string definition;
      llvm::raw_string_ostream definitionStream(definition);
      definitionStream << "PYCIRCUIT_NOINLINE void " << module.cppName
                       << "::" << chunk.name << "() {\n";
      if (chunk.bindsInputs)
        for (const ConcretePortPlan &port : module.inputs)
          definitionStream << "    auto &input_" << port.value << " = "
                           << port.cppName << ";\n";
      definitionStream << chunk.body << "}\n\n";
      definitionStream.flush();
      if (failed(appendWorkDefinition(moduleIndex, definition)))
        return failure();
    }
    struct LifecycleProjection {
      StringRef entry;
      StringRef helperPrefix;
      ArrayRef<ConcreteScheduleStep> steps;
      bool bindsInputs = false;
    };
    auto emitLifecycleProjection = [&](LifecycleProjection projection) {
      SmallVector<WorkChunk, 0> chunks;
      std::string body;
      auto finishChunk = [&] {
        if (body.empty())
          return;
        chunks.push_back(
            {(projection.helperPrefix + Twine(chunks.size())).str(),
             std::move(body), projection.bindsInputs});
        body.clear();
      };
      size_t sourceBytes = 0;
      for (const ConcreteScheduleStep &step : projection.steps) {
        std::string rendered;
        llvm::raw_string_ostream renderedStream(rendered);
        if (failed(emitSteps(ArrayRef<ConcreteScheduleStep>(&step, 1),
                             renderedStream)))
          return failure();
        renderedStream.flush();
        if (rendered.size() > kWorkChunkMaximumBytes)
          return failure();
        if (!body.empty() &&
            body.size() + rendered.size() > kWorkChunkMaximumBytes)
          finishChunk();
        body.append(rendered);
        sourceBytes += rendered.size();
        if (body.size() >= kWorkChunkPreferredBytes)
          finishChunk();
      }
      finishChunk();

      const bool externalize =
          emitCombinedWork || sourceBytes > kExternalExecutionThresholdBytes;
      if (externalize) {
        std::string dispatcher;
        llvm::raw_string_ostream dispatcherStream(dispatcher);
        dispatcherStream << "  void " << projection.entry << "() noexcept {\n";
        for (const WorkChunk &chunk : chunks)
          dispatcherStream << "    " << chunk.name << "();\n";
        dispatcherStream << "  }\n";
        dispatcherStream.flush();
        if (dispatcher.size() > kGeneratedFunctionMaximumBytes)
          return failure();
        out << dispatcher;
        for (const WorkChunk &chunk : chunks) {
          out << "  PYCIRCUIT_NOINLINE void " << chunk.name << "() noexcept;\n";
          std::string definition;
          llvm::raw_string_ostream definitionStream(definition);
          definitionStream << "PYCIRCUIT_NOINLINE void " << module.cppName
                           << "::" << chunk.name << "() noexcept {\n";
          if (chunk.bindsInputs)
            for (const ConcretePortPlan &port : module.inputs)
              definitionStream << "    auto &input_" << port.value << " = "
                               << port.cppName << ";\n";
          definitionStream << chunk.body << "}\n\n";
          definitionStream.flush();
          if (failed(appendWorkDefinition(moduleIndex, definition)))
            return failure();
        }
        return success();
      }

      std::string definition;
      llvm::raw_string_ostream definitionStream(definition);
      definitionStream << "  void " << projection.entry << "() noexcept {\n";
      if (projection.bindsInputs)
        for (const ConcretePortPlan &port : module.inputs)
          definitionStream << "    auto &input_" << port.value << " = "
                           << port.cppName << ";\n";
      for (const WorkChunk &chunk : chunks)
        definitionStream << chunk.body;
      definitionStream << "  }\n";
      definitionStream.flush();
      if (definition.size() > kGeneratedFunctionMaximumBytes)
        return failure();
      out << definition;
      return success();
    };
    if (failed(emitLifecycleProjection(
            {"RefreshPureOutputs", "pyc_refresh_pure_chunk_",
             module.pureOutputRefresh.steps, true})) ||
        failed(emitLifecycleProjection({"RefreshTemporalOutputs",
                                        "pyc_refresh_temporal_chunk_",
                                        module.temporalRefresh.steps, true})) ||
        failed(emitLifecycleProjection({"PrepareResetInputs",
                                        "pyc_prepare_reset_chunk_",
                                        module.resetPreparation.steps, true})))
      return failure();
    out << "  void Build() override;\n"
           "  bool ValidateChecks() const noexcept;\n";
    out << "  gfsim::SimFailureInfo CheckFailure() const noexcept { return "
           "pyc_check_failure_; }\n";
    out << "  void Xfer() noexcept override;\n"
           "  void DiscardNext() noexcept override;\n"
           "  void Reset() noexcept override;\n"
           "  void ReportStat() override;\n"
           "  bool HasWork() const noexcept override;\n"
           "};\n\n";
  }
  for (unsigned sourceUnit = 0; sourceUnit < pendingWorkShards.size();
       ++sourceUnit)
    if (failed(flushWorkShard(sourceUnit)))
      return failure();
  out << combinedWork;
  return success();
}

LogicalResult emitSourceUnit(const ConcreteDesignPlan &plan,
                             unsigned sourceUnit, llvm::raw_ostream &out,
                             bool includeHeader) {
  if (sourceUnit >= plan.sourceUnits.size())
    return failure();
  if (includeHeader)
    out << "#include \"design.hpp\"\n\n";
  for (unsigned moduleIndex : plan.sourceUnits[sourceUnit].moduleIndices) {
    if (moduleIndex >= plan.modules.size())
      return failure();
    const ConcreteModulePlan &module = plan.modules[moduleIndex];
    const bool hasGuardChecks =
        llvm::any_of(plan.checks, [&](const ConcreteCheckPlan &check) {
          return check.moduleIndex == moduleIndex &&
                 check.kind == "guard_known";
        });
    out << module.cppName << "::" << module.cppName
        << "(std::string name, pyc_observation_sink *pyc_sink, "
           "std::size_t pyc_observation_base, "
           "bool pyc_refresh_outputs_on_xfer, bool pyc_host_driven) : "
           "gfsim::SimModule(std::move(name)), pyc_sink_(pyc_sink), "
           "pyc_observation_base_(pyc_observation_base), "
           "pyc_refresh_outputs_on_xfer_(pyc_refresh_outputs_on_xfer), "
           "pyc_host_driven_(pyc_host_driven)";
    uint64_t childObservationBase = module.observationSites.size();
    for (const ConcreteChildPlan &child : module.children) {
      out << ", " << child.cppName << "(\"" << child.instanceName.getValue()
          << "\"";
      if (child.kind == ConcreteChildKind::Module) {
        if (!child.moduleIndex)
          return failure();
        out << ", pyc_sink, pyc_observation_base + " << childObservationBase;
        childObservationBase +=
            plan.modules[*child.moduleIndex].observationSubtreeWidth;
      }
      out << ")";
    }
    out << " {}\n\n";

    out << "void " << module.cppName << "::Build() {\n";
    for (const ConcreteChildPlan &child : module.children)
      out << "  " << child.cppName << ".Build();\n";
    out << "}\n\nbool " << module.cppName
        << "::ValidateChecks() const noexcept {\n  bool accepted = true;\n";
    out << "  pyc_check_failure_ = {};\n";
    std::string resetExpression =
        "gfsim::FourState<1>::known(gfsim::Bits<1>{0})";
    std::string clockExpression =
        "gfsim::FourState<1>::known(gfsim::Bits<1>{0})";
    if (module.clockInput && *module.clockInput < module.inputs.size())
      clockExpression = module.inputs[*module.clockInput].cppName + ".packed()";
    if (module.resetInput && *module.resetInput < module.inputs.size())
      resetExpression = module.inputs[*module.resetInput].cppName + ".packed()";
    if (hasGuardChecks)
      out << "  const auto &pyc_sampling_clock = " << clockExpression
          << ";\n  const auto &pyc_sampling_reset = " << resetExpression
          << ";\n  const bool pyc_sampling_rising = "
             "pyc_sampling_clock.isFullyKnown() && "
             "pyc_sampling_clock.value().toBool() && "
             "!pyc_sampling_clock_;\n"
             "  const bool pyc_sampling_reset_active = "
             "pyc_sampling_reset.isFullyKnown() && "
             "pyc_sampling_reset.value().toBool();\n"
             "  accepted &= pyc_sampling_clock.isFullyKnown() && "
             "(!pyc_sampling_rising || pyc_sampling_reset.isFullyKnown());\n";
    for (const ConcreteCheckPlan &check : plan.checks)
      if (check.moduleIndex == moduleIndex) {
        const auto &path = module.values[check.path];
        const auto &condition = module.values[check.condition];
        if (check.kind == "guard_known") {
          out << "  accepted &= !pyc_sampling_rising || "
                 "pyc_sampling_reset_active || ("
              << path.cppName << ".packed().isFullyKnown() && (!"
              << path.cppName << ".packed().value().toBool() || "
              << condition.cppName << ".packed().isFullyKnown()));\n";
        } else {
          out << "  {\n    const auto pyc_error = gfsim::bit_and("
              << "gfsim::bit_not(" << resetExpression << "), gfsim::bit_and("
              << path.cppName << ".packed(), gfsim::bit_not("
              << condition.cppName
              << ".packed())));\n"
                 "    const bool pyc_ok = pyc_error.isFullyKnown() && "
                 "pyc_error.value() == gfsim::Bits<1>{0};\n"
                 "    if (!pyc_ok && pyc_check_failure_.code.empty())\n"
                 "      pyc_check_failure_ = {gfsim::SimFailurePhase::Check, "
                 "\"source_check_failed\", ";
          emitCppStringLiteral(out, check.message);
          out << ", name(), ";
          emitCppStringLiteral(out, check.locationJson);
          out << ", ";
          emitCppStringLiteral(out, check.checkIdJson);
          out << "};\n    accepted &= pyc_ok;\n  }\n";
        }
      }
    for (const ConcreteChildPlan &child : module.children)
      if (child.kind == ConcreteChildKind::Module)
        out << "  {\n"
               "    const bool pyc_child_ok = "
            << child.cppName
            << ".ValidateChecks();\n"
               "    if (!pyc_child_ok && pyc_check_failure_.code.empty())\n"
               "      pyc_check_failure_ = "
            << child.cppName
            << ".CheckFailure();\n"
               "    accepted &= pyc_child_ok;\n"
               "  }\n";
    out << "  return accepted;\n}\n\n";

    out << "void " << module.cppName << "::Xfer() noexcept {\n";
    for (const ConcreteLifecycleStep &step : module.lifecycle.xfer)
      out << "  " << module.children[*step.childIndex].cppName << ".Xfer();\n";
    if (hasGuardChecks)
      out << "  if (" << clockExpression
          << ".isFullyKnown()) pyc_sampling_clock_ = " << clockExpression
          << ".value().toBool();\n";
    out << "  if (pyc_refresh_outputs_on_xfer_) RefreshTemporalOutputs();\n";
    out << "}\n\nvoid " << module.cppName << "::DiscardNext() noexcept {\n";
    for (const ConcreteLifecycleStep &step : module.lifecycle.discard)
      out << "  " << module.children[*step.childIndex].cppName
          << ".DiscardNext();\n";
    out << "}\n\nvoid " << module.cppName << "::Reset() noexcept {\n";
    out << "  PrepareResetInputs();\n";
    if (hasGuardChecks)
      out << "  pyc_sampling_clock_ = false;\n";
    for (const ConcreteLifecycleStep &step : module.lifecycle.reset)
      out << "  " << module.children[*step.childIndex].cppName << ".Reset();\n";
    out << "}\n\nvoid " << module.cppName << "::ReportStat() {\n";
    for (const ConcreteChildPlan &child : module.children)
      out << "  " << child.cppName << ".ReportStat();\n";
    out << "}\n\nbool " << module.cppName << "::HasWork() const noexcept {\n";
    out << "  return pyc_host_driven_";
    if (!module.children.empty()) {
      for (auto [index, child] : llvm::enumerate(module.children)) {
        (void)index;
        out << " || " << child.cppName << ".HasWork()";
      }
    }
    out << ";\n";
    out << "}\n\n";
  }
  return success();
}

LogicalResult fail(Operation *site, StringRef message) {
  return site->emitOpError() << message;
}

FailureOr<ac::ModuleOp> structuralRoot(ModuleOp package,
                                       ac::HardwareAnalysis &analysis) {
  auto systems = package.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return fail(package.getOperation(),
                "structural C++ requires exactly one ac.system");
  auto system = *systems.begin();
  auto entry = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
  auto root = entry ? dyn_cast_or_null<ac::ModuleOp>(
                          analysis.lookupDefinition(entry.getValue()))
                    : ac::ModuleOp();
  if (!root)
    return fail(system, "structural C++ system entry must resolve to a module");
  return root;
}

} // namespace

LogicalResult verifyStructuralCppInput(ModuleOp package,
                                       ac::HardwareAnalysis &analysis) {
  if (!package)
    return failure();
  auto stage = package->getAttrOfType<StringAttr>("ac.stage");
  if (!stage || stage.getValue() != "final" ||
      !ac::isHardwareOnlyPackage(package) || failed(analysis.verify()) ||
      failed(structuralRoot(package, analysis)))
    return failure();
  return success();
}

LogicalResult emitStructuralCpp(ModuleOp package,
                                ac::HardwareAnalysis &analysis,
                                llvm::raw_ostream &out) {
  if (failed(verifyStructuralCppInput(package, analysis)))
    return failure();
  auto plan = buildConcreteDesignPlan(package, analysis);
  return failed(plan) ? failure() : emitStructuralCpp(*plan, out);
}

FailureOr<ConcreteDesignPlan> prepareStructuralCpp(ModuleOp package) {
  if (!package)
    return failure();
  if (!package->hasAttr("hwir.contract"))
    return fail(package.getOperation(),
                "structural C++ requires an independent HWIR package");
  if (failed(hwir::verifyHWIRPackage(package)))
    return failure();
  ac::HardwareAnalysis sharedPayloadAnalysis(package);
  return buildConcreteDesignPlan(package, sharedPayloadAnalysis);
}

LogicalResult emitStructuralCpp(ModuleOp package, llvm::raw_ostream &out) {
  auto plan = prepareStructuralCpp(package);
  if (failed(plan))
    return failure();
  return emitStructuralCpp(*plan, out);
}

LogicalResult emitStructuralCpp(const ConcreteDesignPlan &plan,
                                llvm::raw_ostream &out) {
  if (failed(verifyConcreteDesignPlan(plan)))
    return failure();
  if (failed(emitPlan(plan, out, nullptr, true)))
    return failure();
  for (unsigned index = 0; index < plan.sourceUnits.size(); ++index)
    if (failed(emitSourceUnit(plan, index, out, false)))
      return failure();
  return success();
}

LogicalResult emitStructuralCppHeader(const ConcreteDesignPlan &plan,
                                      llvm::raw_ostream &out) {
  if (failed(verifyConcreteDesignPlan(plan)))
    return failure();
  out << "#pragma once\n";
  return emitPlan(plan, out);
}

LogicalResult emitStructuralCppHostApi(const ConcreteDesignPlan &plan,
                                       llvm::raw_ostream &out) {
  if (failed(verifyConcreteDesignPlan(plan)) ||
      plan.rootModule >= plan.modules.size())
    return failure();
  const auto &root = plan.modules[plan.rootModule];
  out << "\nclass pyc_structural_system final : public gfsim::SimSystem {\n"
         "public:\n  explicit pyc_structural_system("
      << root.cppName
      << " &root) noexcept : root_(root) {}\n"
         "private:\n  bool Precheck() noexcept override {\n";
  if (plan.system && plan.system->resetInput < root.inputs.size()) {
    out << "    const auto &pyc_reset = root_."
        << root.inputs[plan.system->resetInput].cppName
        << ".packed();\n"
           "    if (pyc_reset.isFullyKnown() && "
           "pyc_reset.value().toBool())\n"
           "      return true;\n";
  }
  out << "    return root_.ValidateChecks();\n  }\n  "
         "gfsim::SimFailureInfo DescribeFailure("
         "gfsim::SimFailurePhase phase) const noexcept override {\n"
         "    if (phase == gfsim::SimFailurePhase::Check)\n"
         "      return root_.CheckFailure();\n"
         "    return gfsim::SimSystem::DescribeFailure(phase);\n"
         "  }\n  "
      << root.cppName
      << " &root_;\n};\n\n"
         "class pyc_dut final {\npublic:\n  struct Inputs {\n";
  for (const ConcretePortPlan &port : root.inputs)
    out << "    gfsim::wire<" << port.type.cppTypeName << "> " << port.cppName
        << ";\n";
  out << "  };\n  struct Outputs {\n";
  for (const ConcretePortPlan &port : root.outputs)
    out << "    gfsim::wire<" << port.type.cppTypeName << "> " << port.cppName
        << ";\n";
  out << "  };\n  explicit pyc_dut(std::size_t workers = 1) : "
         "sink_{&observations_, nullptr}, root_(\"root\", &sink_, 0, false, "
         "true), "
         "system_(root_) {\n"
         "    (void)workers;\n    sink_.system = &system_;\n"
         "    static constexpr std::array<gfsim::ObservationDescriptor, "
      << plan.observations.size() << "> descriptors{{\n";
  for (const ConcreteObservationPlan &observation : plan.observations)
    out << "      {" << observation.stableOrdinal << ", "
        << observation.ownerKey << ", " << observation.registrationKey << ", "
        << observation.siteKey << ", gfsim::ObservationKind::"
        << (observation.kind == ConcreteObservationKind::Gauge ? "Gauge"
                                                               : "Event")
        << ", " << observation.groupIndex << ", " << observation.groupSize
        << ", " << observation.payloadWidth << "},\n";
  out << "    }};\n"
         "    if (!observations_.Configure(descriptors, "
      << plan.observationEventCapacity
      << ") || !system_.AttachObservations(observations_) || "
         "!system_.AddModule(root_))\n"
         "      throw std::logic_error(\"structural DUT setup failed\");\n"
         "  }\n  pyc_dut(const pyc_dut &) = delete;\n"
         "  pyc_dut &operator=(const pyc_dut &) = delete;\n"
         "  void drive(const Inputs &inputs) {\n";
  for (const ConcretePortPlan &port : root.inputs)
    out << "    root_." << port.cppName << " = inputs." << port.cppName
        << ";\n";
  out << "  }\n  Outputs sample() const {\n"
         "    if (system_.state() != gfsim::SimSystemState::Ready || "
         "system_.cycle() == 0)\n"
         "      throw std::logic_error(\"no successful DUT sample\");\n"
         "    return {";
  llvm::interleaveComma(root.outputs, out, [&](const ConcretePortPlan &port) {
    out << "root_." << port.cppName;
  });
  out << "};\n  }\n"
         "  gfsim::SimSystem &system() noexcept { return system_; }\n"
         "  gfsim::ObservationSlots &observations() noexcept { return "
         "observations_; }\n  "
      << root.cppName
      << " &root() noexcept { return root_; }\n"
         "private:\n  gfsim::ObservationSlots observations_;\n"
         "  pyc_observation_sink sink_;\n  "
      << root.cppName << " root_;\n  pyc_structural_system system_;\n};\n";
  return success();
}

LogicalResult emitStructuralCppSourceUnit(const ConcreteDesignPlan &plan,
                                          unsigned sourceUnit,
                                          llvm::raw_ostream &out) {
  if (failed(verifyConcreteDesignPlan(plan)))
    return failure();
  return emitSourceUnit(plan, sourceUnit, out, true);
}

FailureOr<SmallVector<StructuralCppWorkShard, 0>>
emitStructuralCppWorkShards(const ConcreteDesignPlan &plan) {
  if (failed(verifyConcreteDesignPlan(plan)))
    return failure();
  SmallVector<StructuralCppWorkShard, 0> result;
  llvm::raw_null_ostream sink;
  if (failed(emitPlan(plan, sink, &result)))
    return failure();
  return result;
}

} // namespace acir::compiler
