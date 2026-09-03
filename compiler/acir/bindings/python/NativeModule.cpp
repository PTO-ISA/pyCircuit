#include "acir/Compiler/Driver.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <Python.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using acir::codegen::ArtifactKind;
using acir::codegen::FileHash;
using acir::codegen::NamedFingerprint;
using acir::compiler::CompilerArtifact;
using acir::compiler::CompilerDiagnostic;
using acir::compiler::CompilerProfile;
using acir::compiler::CompilerRequest;
using acir::compiler::CompilerResult;
using acir::compiler::CompilerStage;

class OwnedPy {
public:
  OwnedPy(PyObject *object = nullptr) : object_(object) {}
  ~OwnedPy() { Py_XDECREF(object_); }

  OwnedPy(const OwnedPy &) = delete;
  OwnedPy &operator=(const OwnedPy &) = delete;

  OwnedPy(OwnedPy &&other) noexcept
      : object_(std::exchange(other.object_, nullptr)) {}
  OwnedPy &operator=(OwnedPy &&other) noexcept {
    if (this != &other) {
      Py_XDECREF(object_);
      object_ = std::exchange(other.object_, nullptr);
    }
    return *this;
  }

  explicit operator bool() const { return object_ != nullptr; }
  PyObject *get() const { return object_; }
  PyObject *release() { return std::exchange(object_, nullptr); }

private:
  PyObject *object_;
};

OwnedPy pyString(llvm::StringRef value) {
  return OwnedPy(PyUnicode_FromStringAndSize(value.data(), value.size()));
}

OwnedPy pyBytes(llvm::StringRef value) {
  return OwnedPy(PyBytes_FromStringAndSize(value.data(), value.size()));
}

OwnedPy pyNone() {
  Py_INCREF(Py_None);
  return OwnedPy(Py_None);
}

bool setItem(PyObject *dictionary, const char *key, OwnedPy value) {
  return value && PyDict_SetItemString(dictionary, key, value.get()) == 0;
}

std::optional<std::string> pythonString(PyObject *value,
                                        llvm::StringRef label) {
  if (!PyUnicode_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be a string", label.str().c_str());
    return std::nullopt;
  }
  OwnedPy encoded(PyUnicode_AsUTF8String(value));
  if (!encoded)
    return std::nullopt;
  char *data = nullptr;
  Py_ssize_t size = 0;
  if (PyBytes_AsStringAndSize(encoded.get(), &data, &size) != 0)
    return std::nullopt;
  return std::string(data, static_cast<size_t>(size));
}

std::optional<std::string> pythonBytes(PyObject *value, llvm::StringRef label) {
  if (!PyBytes_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be bytes", label.str().c_str());
    return std::nullopt;
  }
  char *data = nullptr;
  Py_ssize_t size = 0;
  if (PyBytes_AsStringAndSize(value, &data, &size) != 0)
    return std::nullopt;
  return std::string(data, static_cast<size_t>(size));
}

bool hasOnlyKeys(PyObject *dictionary, llvm::ArrayRef<llvm::StringRef> keys,
                 llvm::StringRef label) {
  if (!PyDict_Check(dictionary)) {
    PyErr_Format(PyExc_TypeError, "%s must be a dictionary",
                 label.str().c_str());
    return false;
  }
  Py_ssize_t position = 0;
  PyObject *key = nullptr;
  PyObject *value = nullptr;
  while (PyDict_Next(dictionary, &position, &key, &value)) {
    auto name = pythonString(key, "dictionary key");
    if (!name)
      return false;
    bool known = false;
    for (llvm::StringRef candidate : keys)
      known |= candidate == *name;
    if (!known) {
      PyErr_Format(PyExc_ValueError, "%s contains unknown key '%s'",
                   label.str().c_str(), name->c_str());
      return false;
    }
  }
  return true;
}

std::optional<CompilerStage> parseStage(llvm::StringRef name) {
  static constexpr std::array<std::pair<llvm::StringLiteral, CompilerStage>, 11>
      stages{{
          {"acir-parse", CompilerStage::AcirParse},
          {"acir-verify", CompilerStage::AcirVerify},
          {"acir-normalize", CompilerStage::AcirNormalize},
          {"acir-freeze", CompilerStage::AcirFreeze},
          {"acsim-lower", CompilerStage::AcsimLower},
          {"acsim-verify", CompilerStage::AcsimVerify},
          {"cxx-emit", CompilerStage::CxxEmit},
          {"cxx-contract", CompilerStage::CxxContract},
          {"compile", CompilerStage::Compile},
          {"link", CompilerStage::Link},
          {"publish", CompilerStage::Publish},
      }};
  for (const auto &[spelling, stage] : stages)
    if (name == spelling)
      return stage;
  return std::nullopt;
}

std::optional<ArtifactKind> parseArtifactKind(llvm::StringRef name) {
  if (name == "acpy")
    return ArtifactKind::Acpy;
  if (name == "frozen-acir" || name == "acir")
    return ArtifactKind::Acir;
  if (name == "acsim")
    return ArtifactKind::Acsim;
  if (name == "cpp-source")
    return ArtifactKind::CppSource;
  if (name == "cpp-header")
    return ArtifactKind::CppHeader;
  if (name == "executable")
    return ArtifactKind::Executable;
  if (name == "report")
    return ArtifactKind::Report;
  return std::nullopt;
}

llvm::StringRef artifactKindName(ArtifactKind kind) {
  switch (kind) {
  case ArtifactKind::Acpy:
    return "acpy";
  case ArtifactKind::Acir:
    return "frozen-acir";
  case ArtifactKind::Acsim:
    return "acsim";
  case ArtifactKind::CppSource:
    return "cpp-source";
  case ArtifactKind::CppHeader:
    return "cpp-header";
  case ArtifactKind::Executable:
    return "executable";
  case ArtifactKind::Report:
    return "report";
  }
  return "report";
}

bool parseStringTuple(PyObject *value, llvm::StringRef label,
                      std::vector<std::string> &result) {
  if (!PyTuple_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be a tuple", label.str().c_str());
    return false;
  }
  const Py_ssize_t size = PyTuple_Size(value);
  if (size < 0)
    return false;
  result.reserve(static_cast<size_t>(size));
  for (Py_ssize_t index = 0; index < size; ++index) {
    auto item = pythonString(PyTuple_GetItem(value, index), label);
    if (!item)
      return false;
    result.push_back(std::move(*item));
  }
  return true;
}

bool parseStringList(PyObject *value, llvm::StringRef label,
                     std::vector<std::string> &result) {
  if (!PyList_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be a list", label.str().c_str());
    return false;
  }
  const Py_ssize_t size = PyList_Size(value);
  if (size < 0)
    return false;
  result.reserve(static_cast<size_t>(size));
  for (Py_ssize_t index = 0; index < size; ++index) {
    auto item = pythonString(PyList_GetItem(value, index), label);
    if (!item)
      return false;
    result.push_back(std::move(*item));
  }
  return true;
}

bool parseNamedFingerprints(PyObject *value, llvm::StringRef label,
                            std::vector<NamedFingerprint> &result) {
  if (!PyList_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be a list", label.str().c_str());
    return false;
  }
  const Py_ssize_t size = PyList_Size(value);
  if (size < 0)
    return false;
  for (Py_ssize_t index = 0; index < size; ++index) {
    PyObject *entry = PyList_GetItem(value, index);
    static constexpr std::array<llvm::StringRef, 2> keys{"name", "fingerprint"};
    if (!hasOnlyKeys(entry, keys, label))
      return false;
    auto name = pythonString(PyDict_GetItemString(entry, "name"), label);
    auto fingerprint =
        pythonString(PyDict_GetItemString(entry, "fingerprint"), label);
    if (!name || !fingerprint)
      return false;
    result.push_back({std::move(*name), std::move(*fingerprint)});
  }
  return true;
}

bool parseFileHashes(PyObject *value, std::vector<FileHash> &result) {
  if (!PyList_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "source_files must be a list");
    return false;
  }
  const Py_ssize_t size = PyList_Size(value);
  if (size < 0)
    return false;
  for (Py_ssize_t index = 0; index < size; ++index) {
    PyObject *entry = PyList_GetItem(value, index);
    static constexpr std::array<llvm::StringRef, 2> keys{"path", "sha256"};
    if (!hasOnlyKeys(entry, keys, "source file"))
      return false;
    auto path =
        pythonString(PyDict_GetItemString(entry, "path"), "source path");
    auto fingerprint =
        pythonString(PyDict_GetItemString(entry, "sha256"), "source hash");
    if (!path || !fingerprint)
      return false;
    result.push_back({std::move(*path), std::move(*fingerprint)});
  }
  return true;
}

bool parseBuildOptions(PyObject *value, CompilerRequest &request) {
  static constexpr std::array<llvm::StringRef, 14> keys{
      "project_name",      "project_identity",
      "system_name",       "system_identity",
      "source_files",      "python_version",
      "helper_identities", "compiler",
      "standard_library",  "instrumentation_layers",
      "output_root",       "include_roots",
      "link_inputs",       "linker_flags"};
  if (!hasOnlyKeys(value, keys, "native build options"))
    return false;
  for (llvm::StringRef key : keys)
    if (!PyDict_GetItemString(value, key.data())) {
      PyErr_Format(PyExc_ValueError, "native build options are missing '%s'",
                   key.str().c_str());
      return false;
    }
  auto projectName =
      pythonString(PyDict_GetItemString(value, "project_name"), "project_name");
  auto projectIdentity = pythonString(
      PyDict_GetItemString(value, "project_identity"), "project_identity");
  auto systemName =
      pythonString(PyDict_GetItemString(value, "system_name"), "system_name");
  auto systemIdentity = pythonString(
      PyDict_GetItemString(value, "system_identity"), "system_identity");
  auto pythonVersion = pythonString(
      PyDict_GetItemString(value, "python_version"), "python_version");
  auto compiler =
      pythonString(PyDict_GetItemString(value, "compiler"), "compiler");
  auto standardLibrary = pythonString(
      PyDict_GetItemString(value, "standard_library"), "standard_library");
  auto outputRoot =
      pythonString(PyDict_GetItemString(value, "output_root"), "output_root");
  if (!projectName || !projectIdentity || !systemName || !systemIdentity ||
      !pythonVersion || !compiler || !standardLibrary || !outputRoot)
    return false;

  auto toolchain =
      acir::codegen::identifyToolchain(*compiler, *standardLibrary, "default",
#ifdef __APPLE__
                                       "mach-o",
#else
                                       "elf",
#endif
                                       {"-std=c++20"});
  if (!toolchain) {
    std::string message = llvm::toString(toolchain.takeError());
    PyErr_SetString(PyExc_ValueError, message.c_str());
    return false;
  }

  auto &build = request.build;
  build.project = {std::move(*projectName), std::move(*projectIdentity)};
  build.system = {std::move(*systemName), std::move(*systemIdentity)};
  build.frontend.pythonVersion = std::move(*pythonVersion);
  if (!parseFileHashes(PyDict_GetItemString(value, "source_files"),
                       build.frontend.sourceFiles) ||
      !parseNamedFingerprints(PyDict_GetItemString(value, "helper_identities"),
                              "helper_identities",
                              build.frontend.helperIdentities) ||
      !parseStringList(PyDict_GetItemString(value, "instrumentation_layers"),
                       "instrumentation_layers", build.instrumentationLayers) ||
      !parseStringList(PyDict_GetItemString(value, "include_roots"),
                       "include_roots", build.includeRoots) ||
      !parseStringList(PyDict_GetItemString(value, "link_inputs"),
                       "link_inputs", build.linkInputs) ||
      !parseStringList(PyDict_GetItemString(value, "linker_flags"),
                       "linker_flags", build.linkerFlags))
    return false;
  build.frontend.acpy = {
      "input/model.acpy.json", ArtifactKind::Acpy,
      acir::codegen::computeFingerprint(build.frontend.acpyBytes)};
  build.frontend.canonicalAcir = {
      "input/model.ac.mlir", ArtifactKind::Acir,
      acir::codegen::computeFingerprint(build.frontend.canonicalAcirBytes)};
  build.toolchain = std::move(*toolchain);
  if (build.linkerFlags.empty()) {
    llvm::StringRef llvmLinkerFlags(ACIR_NATIVE_LLVM_LINK_FLAGS);
    while (!llvmLinkerFlags.empty()) {
      auto [flag, remainder] = llvmLinkerFlags.split('|');
      if (!flag.empty())
        build.linkerFlags.push_back(flag.str());
      llvmLinkerFlags = remainder;
    }
  }
  build.outputRoot = std::move(*outputRoot);
  return true;
}

bool parseOptions(PyObject *options, CompilerRequest &request) {
  static constexpr std::array<llvm::StringRef, 11> keys{
      "profile",
      "binding_lock",
      "binding_registry",
      "custom_pipeline",
      "dump_before",
      "dump_after",
      "dump_after_each",
      "verify_after_each",
      "frontend_acpy",
      "frontend_acir",
      "build",
  };
  if (!hasOnlyKeys(options, keys, "native compiler options"))
    return false;

  if (PyObject *value = PyDict_GetItemString(options, "profile")) {
    auto profile = pythonString(value, "profile");
    if (!profile)
      return false;
    if (*profile == "fast")
      request.profile = CompilerProfile::Fast;
    else if (*profile == "validated")
      request.profile = CompilerProfile::Validated;
    else if (*profile == "custom")
      request.profile = CompilerProfile::Custom;
    else {
      PyErr_SetString(PyExc_ValueError,
                      "profile must be fast, validated, or custom");
      return false;
    }
  }
  if (PyObject *value = PyDict_GetItemString(options, "binding_lock")) {
    auto bytes = pythonBytes(value, "binding_lock");
    if (!bytes)
      return false;
    request.bindingLockBytes = std::move(*bytes);
  }
  if (PyObject *value = PyDict_GetItemString(options, "binding_registry")) {
    auto bytes = pythonBytes(value, "binding_registry");
    if (!bytes)
      return false;
    request.bindingRegistryBytes = std::move(*bytes);
  }
  if (PyObject *value = PyDict_GetItemString(options, "frontend_acpy")) {
    auto bytes = pythonBytes(value, "frontend_acpy");
    if (!bytes)
      return false;
    request.build.frontend.acpyBytes = std::move(*bytes);
  }
  if (PyObject *value = PyDict_GetItemString(options, "frontend_acir")) {
    auto bytes = pythonBytes(value, "frontend_acir");
    if (!bytes)
      return false;
    request.build.frontend.canonicalAcirBytes = std::move(*bytes);
  }
  if (PyObject *value = PyDict_GetItemString(options, "build"))
    if (!parseBuildOptions(value, request))
      return false;
  if (PyObject *value = PyDict_GetItemString(options, "custom_pipeline")) {
    auto pipeline = pythonString(value, "custom_pipeline");
    if (!pipeline)
      return false;
    request.customPipeline = std::move(*pipeline);
  }
  if (PyObject *value = PyDict_GetItemString(options, "dump_before"))
    if (!parseStringTuple(value, "dump_before", request.dumpBefore))
      return false;
  if (PyObject *value = PyDict_GetItemString(options, "dump_after"))
    if (!parseStringTuple(value, "dump_after", request.dumpAfter))
      return false;
  auto parseBoolean = [&](const char *key, bool &target) {
    PyObject *value = PyDict_GetItemString(options, key);
    if (!value)
      return true;
    if (!PyBool_Check(value)) {
      PyErr_Format(PyExc_TypeError, "%s must be a boolean", key);
      return false;
    }
    target = value == Py_True;
    return true;
  };
  return parseBoolean("dump_after_each", request.dumpAfterEach) &&
         parseBoolean("verify_after_each", request.verifyAfterEach);
}

std::optional<CompilerRequest> parseRequest(PyObject *value) {
  static constexpr std::array<llvm::StringRef, 4> keys{"acir", "stop_after",
                                                       "emits", "options"};
  if (!hasOnlyKeys(value, keys, "native compiler request"))
    return std::nullopt;
  for (llvm::StringRef key : keys)
    if (!PyDict_GetItemString(value, key.data())) {
      PyErr_Format(PyExc_ValueError, "native compiler request is missing '%s'",
                   key.str().c_str());
      return std::nullopt;
    }

  CompilerRequest request;
  auto bytes = pythonBytes(PyDict_GetItemString(value, "acir"), "acir");
  if (!bytes)
    return std::nullopt;
  request.acirBytes = std::move(*bytes);

  PyObject *stop = PyDict_GetItemString(value, "stop_after");
  if (stop != Py_None) {
    auto name = pythonString(stop, "stop_after");
    if (!name)
      return std::nullopt;
    request.stopAfter = parseStage(*name);
    if (!request.stopAfter) {
      PyErr_SetString(PyExc_ValueError, "unknown native compiler stop stage");
      return std::nullopt;
    }
  }

  PyObject *emits = PyDict_GetItemString(value, "emits");
  if (!PyTuple_Check(emits)) {
    PyErr_SetString(PyExc_TypeError, "emits must be a tuple");
    return std::nullopt;
  }
  const Py_ssize_t emitCount = PyTuple_Size(emits);
  if (emitCount < 0)
    return std::nullopt;
  for (Py_ssize_t index = 0; index < emitCount; ++index) {
    auto name = pythonString(PyTuple_GetItem(emits, index), "emit name");
    if (!name)
      return std::nullopt;
    auto kind = parseArtifactKind(*name);
    if (!kind) {
      PyErr_SetString(PyExc_ValueError, "unknown native compiler artifact");
      return std::nullopt;
    }
    request.emits.push_back(*kind);
  }

  if (!parseOptions(PyDict_GetItemString(value, "options"), request))
    return std::nullopt;
  return request;
}

OwnedPy jsonToPython(const llvm::json::Value &value) {
  switch (value.kind()) {
  case llvm::json::Value::Null:
    return pyNone();
  case llvm::json::Value::Boolean:
    return OwnedPy(PyBool_FromLong(*value.getAsBoolean()));
  case llvm::json::Value::Number:
    if (auto integer = value.getAsInteger())
      return OwnedPy(PyLong_FromLongLong(*integer));
    return OwnedPy(PyFloat_FromDouble(*value.getAsNumber()));
  case llvm::json::Value::String:
    return pyString(*value.getAsString());
  case llvm::json::Value::Array: {
    const llvm::json::Array &array = *value.getAsArray();
    OwnedPy result(PyList_New(array.size()));
    if (!result)
      return {};
    for (size_t index = 0; index < array.size(); ++index) {
      OwnedPy item = jsonToPython(array[index]);
      if (!item)
        return {};
      PyList_SetItem(result.get(), index, item.release());
    }
    return result;
  }
  case llvm::json::Value::Object: {
    OwnedPy result(PyDict_New());
    if (!result)
      return {};
    for (const auto &entry : *value.getAsObject())
      if (!setItem(result.get(), entry.first.str().c_str(),
                   jsonToPython(entry.second)))
        return {};
    return result;
  }
  }
  return {};
}

OwnedPy
sourceToPython(const std::optional<acir::compiler::SourceLocation> &source) {
  if (!source)
    return pyNone();
  OwnedPy result(PyDict_New());
  if (!result || !setItem(result.get(), "file", pyString(source->file)) ||
      !setItem(result.get(), "line",
               OwnedPy(PyLong_FromUnsignedLongLong(source->line))) ||
      !setItem(result.get(), "column",
               OwnedPy(PyLong_FromUnsignedLongLong(source->column))))
    return {};
  return result;
}

OwnedPy optionalString(const std::optional<std::string> &value) {
  return value ? pyString(*value) : pyNone();
}

OwnedPy diagnosticToPython(const CompilerDiagnostic &diagnostic) {
  OwnedPy result(PyDict_New());
  if (!result || !setItem(result.get(), "stage", pyString(diagnostic.stage)) ||
      !setItem(result.get(), "code", pyString(diagnostic.code)) ||
      !setItem(result.get(), "severity", pyString(diagnostic.severity)) ||
      !setItem(result.get(), "message", pyString(diagnostic.message)) ||
      !setItem(result.get(), "source", sourceToPython(diagnostic.source)) ||
      !setItem(result.get(), "object_path",
               optionalString(diagnostic.objectPath)) ||
      !setItem(result.get(), "expected", jsonToPython(diagnostic.expected)) ||
      !setItem(result.get(), "actual", jsonToPython(diagnostic.actual)))
    return {};

  OwnedPy related(PyTuple_New(diagnostic.related.size()));
  if (!related)
    return {};
  for (size_t index = 0; index < diagnostic.related.size(); ++index) {
    const auto &item = diagnostic.related[index];
    OwnedPy entry(PyDict_New());
    if (!entry || !setItem(entry.get(), "message", pyString(item.message)) ||
        !setItem(entry.get(), "source", sourceToPython(item.source)) ||
        !setItem(entry.get(), "object_path", optionalString(item.objectPath)))
      return {};
    PyTuple_SetItem(related.get(), index, entry.release());
  }
  if (!setItem(result.get(), "related", std::move(related)))
    return {};

  OwnedPy fixits(PyTuple_New(diagnostic.fixits.size()));
  if (!fixits)
    return {};
  for (size_t index = 0; index < diagnostic.fixits.size(); ++index) {
    OwnedPy entry(PyDict_New());
    if (!entry || !setItem(entry.get(), "message",
                           pyString(diagnostic.fixits[index].message)))
      return {};
    PyTuple_SetItem(fixits.get(), index, entry.release());
  }
  if (!setItem(result.get(), "fixits", std::move(fixits)))
    return {};
  return result;
}

OwnedPy artifactToPython(const CompilerArtifact &artifact) {
  OwnedPy result(PyDict_New());
  if (!result ||
      !setItem(result.get(), "path", pyString(artifact.logicalPath)) ||
      !setItem(result.get(), "kind",
               pyString(artifactKindName(artifact.kind))) ||
      !setItem(result.get(), "data", pyBytes(artifact.bytes)) ||
      !setItem(result.get(), "sha256", pyString(artifact.sha256)))
    return {};
  return result;
}

OwnedPy resultToPython(const CompilerResult &result) {
  OwnedPy output(PyDict_New());
  OwnedPy artifacts(PyTuple_New(result.artifacts.size()));
  OwnedPy diagnostics(PyTuple_New(result.diagnostics.size()));
  if (!output || !artifacts || !diagnostics)
    return {};
  for (size_t index = 0; index < result.artifacts.size(); ++index) {
    OwnedPy item = artifactToPython(result.artifacts[index]);
    if (!item)
      return {};
    PyTuple_SetItem(artifacts.get(), index, item.release());
  }
  for (size_t index = 0; index < result.diagnostics.size(); ++index) {
    OwnedPy item = diagnosticToPython(result.diagnostics[index]);
    if (!item)
      return {};
    PyTuple_SetItem(diagnostics.get(), index, item.release());
  }
  if (!setItem(output.get(), "artifacts", std::move(artifacts)) ||
      !setItem(output.get(), "diagnostics", std::move(diagnostics)))
    return {};
  if (result.build) {
    if (!setItem(output.get(), "build_directory",
                 pyString(result.build->buildDirectory)) ||
        !setItem(output.get(), "executable",
                 pyString(result.build->executable)) ||
        !setItem(output.get(), "build_fingerprint",
                 pyString(result.build->buildFingerprint)) ||
        !setItem(output.get(), "cache_hit",
                 OwnedPy(PyBool_FromLong(result.build->cacheHit))))
      return {};
  } else if (!setItem(output.get(), "build_directory", pyNone()) ||
             !setItem(output.get(), "executable", pyNone()) ||
             !setItem(output.get(), "build_fingerprint", pyNone()) ||
             !setItem(output.get(), "cache_hit", pyNone())) {
    return {};
  }
  return output;
}

CompilerResult failedResult(llvm::Error error) {
  CompilerResult result;
  llvm::handleAllErrors(
      std::move(error),
      [&](const acir::compiler::CompilerError &failure) {
        result.diagnostics = failure.diagnostics();
      },
      [&](const llvm::ErrorInfoBase &failure) {
        result.diagnostics.push_back({.stage = "native",
                                      .code = "ACLOWER-NATIVE-001",
                                      .severity = "error",
                                      .message = failure.message()});
      });
  return result;
}

PyObject *runCompiler(PyObject *, PyObject *arguments) {
  PyObject *requestObject = nullptr;
  if (!PyArg_ParseTuple(arguments, "O:run_compiler", &requestObject))
    return nullptr;
  auto request = parseRequest(requestObject);
  if (!request)
    return nullptr;
  auto compiled = acir::compiler::runCompiler(*request);
  OwnedPy result = compiled
                       ? resultToPython(*compiled)
                       : resultToPython(failedResult(compiled.takeError()));
  return result.release();
}

PyObject *capabilities(PyObject *, PyObject *) {
  OwnedPy result(PyDict_New());
  if (!result ||
      !setItem(result.get(), "compiler_build_id",
               pyString("agentic-circuit-0.1.0+llvm-22.1.8")) ||
      !setItem(result.get(), "runtime_build_id",
               pyString("gfsim-0.1.0+cxx20")) ||
      !setItem(result.get(), "items", OwnedPy(PyTuple_New(0))))
    return nullptr;
  return result.release();
}

PyMethodDef methods[] = {
    {"run_compiler", runCompiler, METH_VARARGS,
     "Run the closed native compiler request."},
    {"capabilities", capabilities, METH_NOARGS,
     "Return native compiler and runtime capabilities."},
    {nullptr, nullptr, 0, nullptr},
};

PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "_native",
    "Private Agentic Circuit native compiler bridge.",
    -1,
    methods,
};

} // namespace

PyMODINIT_FUNC PyInit__native() { return PyModule_Create(&module); }
