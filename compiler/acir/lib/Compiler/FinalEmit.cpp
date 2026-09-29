#include "FinalEmitCpp.h"
#include "FinalEmitVerilog.h"
#include "FinalProgram.h"

using namespace mlir;

namespace acir::compiler {
namespace {

template <typename Emit>
FailureOr<std::string> emitVerified(const FinalProgram &program,
                                    ac::detail::EmitError emitError,
                                    Emit emit) {
  if (failed(verifyFinalProgram(program, emitError)))
    return failure();
  if (!program.isEmitReady())
    return emitError() << "final emitter requires an EmitReady FinalProgram";
  auto text = emit();
  if (failed(text))
    return failure();
  if (failed(verifyFinalProgram(program, emitError)))
    return emitError() << "final program changed during emission";
  return text;
}

} // namespace

FailureOr<std::string> emitFinalCpp(const FinalProgram &program,
                                    ac::detail::EmitError emitError) {
  return emitVerified(program, emitError,
                      [&] { return emitFinalCppBody(program, emitError); });
}

FailureOr<FinalVerilogEmission>
emitFinalVerilogParts(const FinalProgram &program,
                      ac::detail::EmitError emitError) {
  if (failed(verifyFinalProgram(program, emitError)))
    return failure();
  if (!program.isEmitReady())
    return emitError() << "final emitter requires an EmitReady FinalProgram";
  auto parts = emitFinalVerilogPartsBody(program, emitError);
  if (failed(parts))
    return failure();
  if (failed(verifyFinalProgram(program, emitError)))
    return emitError() << "final program changed during emission";
  return parts;
}

FailureOr<std::string> emitFinalVerilog(const FinalProgram &program,
                                        ac::detail::EmitError emitError) {
  auto parts = emitFinalVerilogParts(program, emitError);
  if (failed(parts))
    return failure();
  return parts->rtl + parts->runtimeGlue;
}

} // namespace acir::compiler
