#include "FinalModelAbi.h"

namespace acir::compiler {

FinalModelAbiParts buildFinalModelAbiParts() {
  FinalModelAbiParts parts;
  parts.header = "#pragma once\n#include \"gfsim/model_api.h\"\n";
  parts.source = R"cpp(
#ifndef AGENTIC_MODEL_BUILD
#define AGENTIC_MODEL_BUILD
#endif
#include "dut.h"
#include "pycircuit_system.hpp"
#include "runner_metadata.hpp"
#include "gfsim/SimExecutor.h"

#include <cstdint>
#include <memory>
#include <vector>

struct AgenticModelV1 {
  ::FinalSystem system;
  ::std::vector<::gfsim::ReportGaugeDescriptor> reports;
  ::gfsim::SimExecutor executor;

  AgenticModelV1()
      : system(), reports(makeReports()),
        executor(system, system.Observations(), reports) {}

private:
  static ::std::vector<::gfsim::ReportGaugeDescriptor> makeReports() {
    const auto metadata = ::PycircuitRunnerMetadata();
    ::std::vector<::gfsim::ReportGaugeDescriptor> result;
    for (const auto &entry : metadata) {
      if (entry.kind == "report")
        result.push_back({entry.stableOrdinal, entry.instance, entry.reportName});
    }
    return result;
  }
};

namespace {

::AgenticModelStatusV1 createModel(::AgenticModelV1 **output) noexcept {
  if (!output)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  *output = nullptr;
  try {
    auto model = ::std::make_unique<::AgenticModelV1>();
    if (!model->executor.created())
      return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
    *output = model.release();
    return AGENTIC_MODEL_STATUS_V1_OK;
  } catch (...) {
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

void destroyModel(::AgenticModelV1 *model) noexcept {
  try {
    delete model;
  } catch (...) {
  }
}

::AgenticModelStatusV1 configureModel(::AgenticModelV1 *model,
                                      const ::std::uint8_t *data,
                                      ::std::uint64_t size) noexcept {
  if (!model)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  try {
    return model->executor.ConfigureJson(data, size);
  } catch (...) {
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

::AgenticModelStatusV1 resetModel(::AgenticModelV1 *model) noexcept {
  if (!model)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  try {
    return model->executor.Reset();
  } catch (...) {
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

::AgenticModelStatusV1 stepModel(
    ::AgenticModelV1 *model,
    ::AgenticModelStepResultV1 *result) noexcept {
  if (!model)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  try {
    return model->executor.Step(result);
  } catch (...) {
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

::AgenticModelStatusV1 statisticsModel(
    ::AgenticModelV1 *model,
    ::AgenticModelBufferV1 *result) noexcept {
  if (!model)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  try {
    return model->executor.StatisticsJson(result);
  } catch (...) {
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

::AgenticModelStatusV1 lastErrorModel(
    ::AgenticModelV1 *model,
    ::AgenticModelBufferV1 *result) noexcept {
  if (!model)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  try {
    return model->executor.LastError(result);
  } catch (...) {
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

const ::AgenticModelApiV1 kAgenticModelApiV1{
    sizeof(::AgenticModelApiV1),
    AGENTIC_MODEL_ABI_V1,
    &createModel,
    &destroyModel,
    &configureModel,
    &resetModel,
    &stepModel,
    &statisticsModel,
    &lastErrorModel,
};

} // namespace

extern "C" AGENTIC_MODEL_EXPORT const ::AgenticModelApiV1 *
agentic_model_query_v1(void) {
  return &kAgenticModelApiV1;
}
)cpp";
  return parts;
}

} // namespace acir::compiler
