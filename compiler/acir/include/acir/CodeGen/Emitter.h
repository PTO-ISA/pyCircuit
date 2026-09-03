#ifndef ACIR_CODEGEN_EMITTER_H
#define ACIR_CODEGEN_EMITTER_H

#include "acir/CodeGen/Manifest.h"

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace acir::codegen {

// ── Structured C++ emitter ────────────────────────────────────────────

/// Deterministic, indentation-aware C++ code emitter.
/// Produces reproducible output: no timestamps, no pointer values,
/// no host paths, no non-deterministic ordering.
class CppEmitter {
public:
  explicit CppEmitter(std::ostream &os) : os_(os) {}

  // ── File structure ──────────────────────────────────────────────────

  void emitInclude(const std::string &header, bool system = false);
  void emitPragmaOnce();
  void emitNewline();
  void emitComment(const std::string &text);

  // ── Namespace ───────────────────────────────────────────────────────

  void beginNamespace(const std::string &name);
  void endNamespace();

  // ── Class ───────────────────────────────────────────────────────────

  void beginClass(const std::string &name, const std::string &base = "",
                  bool isStruct = false, bool isFinal = false);
  void endClass();

  // ── Access ─────────────────────────────────────────────────────────

  void emitPublic();
  void emitPrivate();
  void emitProtected();

  // ── Members ─────────────────────────────────────────────────────────

  void emitMember(const std::string &type, const std::string &name,
                  const std::string &init = "");
  void emitStaticMember(const std::string &type, const std::string &name,
                        const std::string &init = "");

  // ── Methods ─────────────────────────────────────────────────────────

  void emitConstructor(
      const std::string &className,
      const std::vector<std::pair<std::string, std::string>> &params,
      const std::vector<std::string> &initializers,
      const std::string &body = "");
  void
  emitMethod(const std::string &returnType, const std::string &name,
             const std::vector<std::pair<std::string, std::string>> &params,
             bool isConst = false, bool isOverride = false,
             bool isVirtual = false, bool isStatic = false);
  void beginMethodBody();
  void endMethodBody();
  void emitReturn(const std::string &expr = "");
  void emitStatement(const std::string &stmt);

  // ── Enums ───────────────────────────────────────────────────────────

  void emitEnum(const std::string &name, const std::vector<std::string> &values,
                const std::string &underlyingType = "uint8_t");

  // ── Switch ──────────────────────────────────────────────────────────

  void emitSwitch(const std::string &expr);
  void emitCase(const std::string &value);
  void emitDefault();
  void emitBreak();
  void endSwitch();

  // ── Template ────────────────────────────────────────────────────────

  void emitTemplate(const std::string &params);

  // ── Raw ─────────────────────────────────────────────────────────────

  void emitRaw(const std::string &code);
  std::ostream &raw() { return os_; }

  // ── Indentation ─────────────────────────────────────────────────────

  void indent();
  void dedent();
  int depth() const { return indent_; }

private:
  void writeIndent();
  std::ostream &os_;
  int indent_ = 0;
  bool atLineStart_ = true;
};

// ── Deterministic code generation ─────────────────────────────────────

/// One generated static-dispatch row. Object expressions are C++ lvalues
/// rooted at the model parameter, for example `model.compute`.
struct DispatchEntry {
  uint32_t objectId = 0;
  std::string objectExpression;
};

struct ActivationEdge {
  uint32_t sourceId = 0;
  uint32_t targetId = 0;
};

/// Generate the deterministic dense gfsim dispatch-table factory for a model.
/// Entries may arrive in any order, but their IDs must be exactly [0, N).
SourceFile
generateDispatchHeader(const std::string &namespaceName,
                       const std::string &modelType,
                       std::vector<DispatchEntry> entries,
                       std::vector<ActivationEdge> activationEdges = {});

} // namespace acir::codegen

#endif // ACIR_CODEGEN_EMITTER_H
