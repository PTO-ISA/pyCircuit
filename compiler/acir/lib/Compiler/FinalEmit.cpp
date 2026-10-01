#include "FinalEmitCpp.h"
#include "FinalEmitVerilog.h"
#include "FinalProgram.h"
#include "FinalRunnerEmission.h"

using namespace mlir;

namespace acir::compiler {
namespace {

LogicalResult
rejectUnemittableRecordDeclarations(const FinalProgram &program,
                                    ac::detail::EmitError emitError) {
  bool foundRecord = false;
  program.hardware().walk([&](ac::StructOp) { foundRecord = true; });
  if (foundRecord)
    return emitError()
           << "final record declarations are not yet supported by emitters";
  return success();
}

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
    if (failed(rejectUnemittableRecordDeclarations(program_, emitError_)))
      return failure();
    return success();
  }

  LogicalResult finish() {
    if (failed(verifyFinalProgram(program_, emitError_)))
      return emitError_() << "final program changed during emission";
    if (failed(rejectUnemittableRecordDeclarations(program_, emitError_)))
      return emitError_() << "final program changed during emission";
    return success();
  }

private:
  const FinalProgram &program_;
  ac::detail::EmitError emitError_;
};

template <typename Result, typename Emit>
FailureOr<Result> emitVerified(const FinalProgram &program,
                               ac::detail::EmitError emitError, Emit emit) {
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

FailureOr<FinalRunnerParts>
emitFinalRunnerParts(const FinalProgram &program,
                     ac::detail::EmitError emitError) {
  return emitVerified<FinalRunnerParts>(program, emitError, [&] {
    return emitFinalRunnerPartsBody(program, emitError);
  });
}

FailureOr<std::string> emitFinalCpp(const FinalProgram &program,
                                    ac::detail::EmitError emitError) {
  return emitVerified<std::string>(
      program, emitError, [&] { return emitFinalCppBody(program, emitError); });
}

FailureOr<FinalCppSourceParts>
emitFinalCppSourceParts(const FinalProgram &program,
                        ac::detail::EmitError emitError) {
  return emitVerified<FinalCppSourceParts>(program, emitError, [&] {
    return emitFinalCppSourcePartsBody(program, emitError);
  });
}

FailureOr<FinalVerilogSourceParts>
emitFinalVerilogSourceParts(const FinalProgram &program,
                            ac::detail::EmitError emitError) {
  return emitVerified<FinalVerilogSourceParts>(program, emitError, [&] {
    return emitFinalVerilogSourcePartsBody(program, emitError);
  });
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
