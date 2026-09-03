#include "acir/CodeGen/Emitter.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace acir::codegen {

// ── CppEmitter ─────────────────────────────────────────────────────────

void CppEmitter::writeIndent() {
  for (int i = 0; i < indent_; ++i)
    os_ << "  ";
}

void CppEmitter::emitPragmaOnce() {
  os_ << "#pragma once\n\n";
  atLineStart_ = true;
}

void CppEmitter::emitInclude(const std::string &header, bool system) {
  if (system)
    os_ << "#include <" << header << ">\n";
  else
    os_ << "#include \"" << header << "\"\n";
  atLineStart_ = true;
}

void CppEmitter::emitNewline() {
  os_ << "\n";
  atLineStart_ = true;
}

void CppEmitter::emitComment(const std::string &text) {
  writeIndent();
  os_ << "// " << text << "\n";
  atLineStart_ = true;
}

void CppEmitter::beginNamespace(const std::string &name) {
  os_ << "namespace " << name << " {\n\n";
  atLineStart_ = true;
}

void CppEmitter::endNamespace() {
  os_ << "} // namespace\n\n";
  atLineStart_ = true;
}

void CppEmitter::beginClass(const std::string &name, const std::string &base,
                            bool isStruct, bool isFinal) {
  writeIndent();
  os_ << (isStruct ? "struct " : "class ") << name;
  if (isFinal)
    os_ << " final";
  if (!base.empty())
    os_ << " : public " << base;
  os_ << " {\n";
  indent();
  atLineStart_ = true;
}

void CppEmitter::endClass() {
  dedent();
  writeIndent();
  os_ << "};\n\n";
  atLineStart_ = true;
}

void CppEmitter::emitPublic() {
  dedent();
  writeIndent();
  os_ << "public:\n";
  indent();
  atLineStart_ = true;
}

void CppEmitter::emitPrivate() {
  dedent();
  writeIndent();
  os_ << "private:\n";
  indent();
  atLineStart_ = true;
}

void CppEmitter::emitProtected() {
  dedent();
  writeIndent();
  os_ << "protected:\n";
  indent();
  atLineStart_ = true;
}

void CppEmitter::emitMember(const std::string &type, const std::string &name,
                            const std::string &init) {
  writeIndent();
  os_ << type << " " << name;
  if (!init.empty())
    os_ << " = " << init;
  os_ << ";\n";
  atLineStart_ = true;
}

void CppEmitter::emitStaticMember(const std::string &type,
                                  const std::string &name,
                                  const std::string &init) {
  writeIndent();
  os_ << "static " << type << " " << name;
  if (!init.empty())
    os_ << " = " << init;
  os_ << ";\n";
  atLineStart_ = true;
}

void CppEmitter::emitMethod(
    const std::string &returnType, const std::string &name,
    const std::vector<std::pair<std::string, std::string>> &params,
    bool isConst, bool isOverride, bool isVirtual, bool isStatic) {
  writeIndent();
  if (isStatic)
    os_ << "static ";
  if (isVirtual)
    os_ << "virtual ";
  os_ << returnType << " " << name << "(";
  for (size_t i = 0; i < params.size(); ++i) {
    os_ << params[i].first << " " << params[i].second;
    if (i + 1 < params.size())
      os_ << ", ";
  }
  os_ << ")";
  if (isConst)
    os_ << " const";
  if (isOverride)
    os_ << " override";
  os_ << ";\n";
  atLineStart_ = true;
}

void CppEmitter::emitConstructor(
    const std::string &className,
    const std::vector<std::pair<std::string, std::string>> &params,
    const std::vector<std::string> &initializers, const std::string &body) {
  writeIndent();
  os_ << className << "(";
  for (size_t i = 0; i < params.size(); ++i) {
    os_ << params[i].first << " " << params[i].second;
    if (i + 1 < params.size())
      os_ << ", ";
  }
  os_ << ")";
  if (body.empty()) {
    os_ << ";\n";
    atLineStart_ = true;
    return;
  }
  if (!initializers.empty()) {
    os_ << "\n";
    writeIndent();
    for (size_t i = 0; i < initializers.size(); ++i) {
      os_ << "    : " << initializers[i];
      if (i + 1 < initializers.size()) {
        os_ << "\n";
        writeIndent();
      }
    }
  }
  os_ << " {\n";
  os_ << body;
  writeIndent();
  os_ << "}\n";
  atLineStart_ = true;
}

void CppEmitter::beginMethodBody() {
  os_ << " {\n";
  indent();
  atLineStart_ = true;
}

void CppEmitter::endMethodBody() {
  dedent();
  writeIndent();
  os_ << "}\n\n";
  atLineStart_ = true;
}

void CppEmitter::emitReturn(const std::string &expr) {
  writeIndent();
  if (expr.empty())
    os_ << "return;\n";
  else
    os_ << "return " << expr << ";\n";
  atLineStart_ = true;
}

void CppEmitter::emitStatement(const std::string &stmt) {
  writeIndent();
  os_ << stmt << ";\n";
  atLineStart_ = true;
}

void CppEmitter::emitEnum(const std::string &name,
                          const std::vector<std::string> &values,
                          const std::string &underlyingType) {
  writeIndent();
  os_ << "enum class " << name << " : " << underlyingType << " {\n";
  indent();
  for (size_t i = 0; i < values.size(); ++i) {
    writeIndent();
    os_ << values[i];
    if (i + 1 < values.size())
      os_ << ",";
    os_ << "\n";
  }
  dedent();
  writeIndent();
  os_ << "};\n\n";
  atLineStart_ = true;
}

void CppEmitter::emitSwitch(const std::string &expr) {
  writeIndent();
  os_ << "switch (" << expr << ") {\n";
  indent();
  atLineStart_ = true;
}

void CppEmitter::emitCase(const std::string &value) {
  dedent();
  writeIndent();
  os_ << "case " << value << ":\n";
  indent();
  atLineStart_ = true;
}

void CppEmitter::emitDefault() {
  dedent();
  writeIndent();
  os_ << "default:\n";
  indent();
  atLineStart_ = true;
}

void CppEmitter::emitBreak() {
  writeIndent();
  os_ << "break;\n";
  atLineStart_ = true;
}

void CppEmitter::endSwitch() {
  dedent();
  writeIndent();
  os_ << "}\n";
  atLineStart_ = true;
}

void CppEmitter::emitTemplate(const std::string &params) {
  writeIndent();
  os_ << "template <" << params << ">\n";
  atLineStart_ = true;
}

void CppEmitter::emitRaw(const std::string &code) {
  os_ << code;
  atLineStart_ = false;
}

void CppEmitter::indent() { ++indent_; }
void CppEmitter::dedent() {
  if (indent_ > 0)
    --indent_;
}

// ── Deterministic code generation ─────────────────────────────────────

SourceFile generateDispatchHeader(const std::string &namespaceName,
                                  const std::string &modelType,
                                  std::vector<DispatchEntry> entries,
                                  std::vector<ActivationEdge> activationEdges) {
  if (namespaceName.empty() || modelType.empty())
    throw std::invalid_argument(
        "dispatch namespace and model type must be non-empty");
  if (entries.size() > std::numeric_limits<uint32_t>::max())
    throw std::invalid_argument("dispatch table exceeds the uint32_t ID space");

  std::sort(entries.begin(), entries.end(),
            [](const DispatchEntry &left, const DispatchEntry &right) {
              return left.objectId < right.objectId;
            });
  for (size_t index = 0; index < entries.size(); ++index) {
    if (entries[index].objectId != index ||
        entries[index].objectExpression.empty())
      throw std::invalid_argument(
          "dispatch object IDs must be dense and expressions non-empty");
  }

  std::sort(activationEdges.begin(), activationEdges.end(),
            [](const ActivationEdge &left, const ActivationEdge &right) {
              return std::tie(left.sourceId, left.targetId) <
                     std::tie(right.sourceId, right.targetId);
            });
  activationEdges.erase(
      std::unique(activationEdges.begin(), activationEdges.end(),
                  [](const ActivationEdge &left, const ActivationEdge &right) {
                    return left.sourceId == right.sourceId &&
                           left.targetId == right.targetId;
                  }),
      activationEdges.end());
  if (activationEdges.size() > std::numeric_limits<uint32_t>::max())
    throw std::invalid_argument(
        "activation adjacency exceeds the uint32_t offset space");
  std::vector<uint32_t> activationOffsets(entries.size() + 1, 0);
  std::vector<uint32_t> activationTargets;
  activationTargets.reserve(activationEdges.size());
  for (const ActivationEdge &edge : activationEdges) {
    if (edge.sourceId >= entries.size() || edge.targetId >= entries.size())
      throw std::invalid_argument(
          "activation edge endpoint is outside the dense dispatch table");
    ++activationOffsets[edge.sourceId + 1];
    activationTargets.push_back(edge.targetId);
  }
  for (size_t index = 1; index < activationOffsets.size(); ++index)
    activationOffsets[index] += activationOffsets[index - 1];

  std::ostringstream os;
  os << "#pragma once\n\n"
        "#include \"gfsim/dispatch.h\"\n\n"
        "#include <array>\n"
        "#include <cstdint>\n\n"
     << "namespace " << namespaceName << " {\n\n"
     << "inline std::array<gfsim::DispatchRow, " << entries.size() << ">\n"
     << "makeDispatchTable(" << modelType << " &model) {\n"
     << "  return {\n";
  for (const DispatchEntry &entry : entries)
    os << "      gfsim::makeDispatchRow(&" << entry.objectExpression << "),\n";
  os << "  };\n"
        "}\n\n";
  os << "inline constexpr std::array<uint32_t, " << activationOffsets.size()
     << "> kActivationOffsets = {";
  for (size_t index = 0; index < activationOffsets.size(); ++index) {
    if (index != 0)
      os << ", ";
    os << activationOffsets[index];
  }
  os << "};\n";
  os << "inline constexpr std::array<gfsim::ObjectId, "
     << activationTargets.size() << "> kActivationTargets = {";
  for (size_t index = 0; index < activationTargets.size(); ++index) {
    if (index != 0)
      os << ", ";
    os << activationTargets[index];
  }
  os << "};\n\n"
     << "} // namespace " << namespaceName << "\n";

  std::string pathNamespace = namespaceName;
  constexpr std::string_view generatedPrefix = "generated::";
  if (pathNamespace.starts_with(generatedPrefix))
    pathNamespace.erase(0, generatedPrefix.size());
  for (size_t separator = pathNamespace.find("::");
       separator != std::string::npos;
       separator = pathNamespace.find("::", separator + 1))
    pathNamespace.replace(separator, 2, "/");

  SourceFile sf;
  sf.relativePath = "include/generated/" + pathNamespace + "/dispatch.h";
  sf.content = os.str();
  sf.fingerprint = computeFingerprint(sf.content);
  return sf;
}

} // namespace acir::codegen
