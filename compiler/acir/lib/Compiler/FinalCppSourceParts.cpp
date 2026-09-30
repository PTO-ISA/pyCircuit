#include "FinalCppSourceParts.h"
#include "FinalCppEmission.h"
#include "FinalEmitCpp.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <set>

using namespace mlir;

namespace acir::compiler {
namespace {

void openNamespace(llvm::raw_ostream &out, llvm::StringRef name) {
  if (!name.empty())
    out << "namespace " << name << " {\n";
}

void closeNamespace(llvm::raw_ostream &out, llvm::StringRef name) {
  if (!name.empty())
    out << "} // namespace " << name << "\n";
}

void forwardFamily(llvm::raw_ostream &out, const CppSpecNames &names) {
  openNamespace(out, names.nameSpace);
  out << "template<class... StaticArguments> class " << names.familyName
      << ";\n";
  closeNamespace(out, names.nameSpace);
}

} // namespace

FailureOr<FinalCppSourceParts>
emitFinalCppSourcePartsBody(const FinalProgram &program,
                            ac::detail::EmitError emitError) {
  auto planOr = buildCppEmissionPlan(program, /*sourceOwned=*/true, emitError);
  if (failed(planOr))
    return failure();
  const auto &plan = *planOr;
  const auto &names = plan.names;
  FinalCppSourceParts result;
  result.supportHeader = "#pragma once\n" + plan.support;

  for (const auto &owner : names.sourceGroups) {
    FinalCppSourceGroup group;
    group.sourceOwner = owner.sourceOwner;
    group.headerPath = owner.headerPath;
    group.sourcePath = owner.sourcePath;
    llvm::raw_string_ostream header(group.header);
    llvm::raw_string_ostream source(group.source);
    header << "#pragma once\n#include <cstdint>\n";
    if (!owner.definitions.empty()) {
      header << "#include \"pycircuit_support.hpp\"\n";
      for (const auto &dependency : owner.childHeaders)
        header << "#include \"" << dependency << "\"\n";
      source << "#include \"" << owner.headerPath << "\"\n";
    }
    if (!owner.declarations.empty()) {
      openNamespace(header, owner.nameSpace);
      for (const auto &declaration : owner.declarations)
        header << declaration.text;
      closeNamespace(header, owner.nameSpace);
    }

    // Own primary templates and parent friend families need declarations.
    // Parents must not be included here: their by-value members already require
    // this child header, so doing so would create an include cycle.
    ::std::set<size_t> forwards(owner.definitions.begin(),
                                owner.definitions.end());
    for (const auto &instance : program.instances()) {
      for (size_t child : instance.childOrdinals) {
        size_t childDef = names.defByInstance[child];
        if (llvm::is_contained(owner.definitions, childDef))
          forwards.insert(names.defByInstance[instance.ordinal]);
      }
    }
    for (size_t definition : forwards)
      forwardFamily(header, names.specs[definition]);

    // The single core already emitted these sections from verified IR. Route
    // them directly to their owning files; never parse or split generated C++.
    for (size_t definition : plan.definitionPostOrder) {
      if (!llvm::is_contained(owner.definitions, definition))
        continue;
      const auto &spec = names.specs[definition];
      const auto &sections = plan.definitions[definition];
      openNamespace(header, spec.nameSpace);
      header << sections.declaration;
      closeNamespace(header, spec.nameSpace);
      openNamespace(source, spec.nameSpace);
      source << sections.constructor << sections.methods;
      closeNamespace(source, spec.nameSpace);
    }
    header.flush();
    source.flush();
    if (group.sourcePath.empty())
      group.source.clear();
    result.sourceGroups.push_back(::std::move(group));
  }

  size_t root = names.defByInstance[program.rootInstanceOrdinal()];
  llvm::raw_string_ostream system(result.systemHeader);
  system << "#pragma once\n#include \"" << names.specs[root].headerPath
         << "\"\n"
         << plan.system;
  system.flush();
  return result;
}

} // namespace acir::compiler
