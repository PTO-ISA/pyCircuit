#include "FinalEmitCpp.h"
#include "FinalEmitVerilog.h"
#include "FinalProgram.h"

using namespace mlir;

namespace acir::compiler {
namespace {

// The single definition of the emission discipline every final emitter must
// follow: verify before, require an EmitReady program, verify after. Keeping it
// in one place means a future change cannot silently apply to only some
// emitters.
class EmissionGuard {
public:
  EmissionGuard(const FinalProgram &program, ac::detail::EmitError emitError)
      : program_(program), emitError_(emitError) {}

  LogicalResult begin() {
    if (failed(verifyFinalProgram(program_, emitError_)))
      return failure();
    if (!program_.isEmitReady())
      return emitError_() << "final emitter requires an EmitReady FinalProgram";
    return success();
  }

  LogicalResult finish() {
    if (failed(verifyFinalProgram(program_, emitError_)))
      return emitError_() << "final program changed during emission";
    return success();
  }

private:
  const FinalProgram &program_;
  ac::detail::EmitError emitError_;
};

template <typename Emit>
FailureOr<std::string> emitVerified(const FinalProgram &program,
                                    ac::detail::EmitError emitError,
                                    Emit emit) {
  EmissionGuard guard(program, emitError);
  if (failed(guard.begin()))
    return failure();
  auto text = emit();
  if (failed(text))
    return failure();
  if (failed(guard.finish()))
    return failure();
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
  EmissionGuard guard(program, emitError);
  if (failed(guard.begin()))
    return failure();
  auto parts = emitFinalVerilogPartsBody(program, emitError);
  if (failed(parts))
    return failure();
  if (failed(guard.finish()))
    return failure();
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
