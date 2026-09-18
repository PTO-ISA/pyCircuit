#include "acir/Compiler/Driver.h"
#include "llvm/Support/Error.h"
#include <Python.h>
#include <optional>
#include <string>
#include <utility>

namespace {
using namespace acir::compiler;

class OwnedPy {
public:
  OwnedPy(PyObject *object = nullptr) : object_(object) {}
  ~OwnedPy() { Py_XDECREF(object_); }
  OwnedPy(const OwnedPy &) = delete;
  OwnedPy &operator=(const OwnedPy &) = delete;
  OwnedPy(OwnedPy &&other) noexcept
      : object_(std::exchange(other.object_, nullptr)) {}
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
OwnedPy pyNone() { Py_INCREF(Py_None); return OwnedPy(Py_None); }
bool setItem(PyObject *dictionary, const char *key, OwnedPy value) {
  return value && PyDict_SetItemString(dictionary, key, value.get()) == 0;
}

std::optional<std::string> pythonString(PyObject *value, const char *label) {
  if (!value || !PyUnicode_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be a string", label);
    return std::nullopt;
  }
  Py_ssize_t size = 0;
  const char *data = PyUnicode_AsUTF8AndSize(value, &size);
  return data ? std::optional<std::string>(std::string(data, size)) : std::nullopt;
}
std::optional<std::string> pythonBytes(PyObject *value, const char *label) {
  if (!value || !PyBytes_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be bytes", label);
    return std::nullopt;
  }
  char *data = nullptr;
  Py_ssize_t size = 0;
  return PyBytes_AsStringAndSize(value, &data, &size) == 0
             ? std::optional<std::string>(std::string(data, size))
             : std::nullopt;
}

std::optional<CompilerStage> parseStage(llvm::StringRef name) {
  if (name == "acir-parse") return CompilerStage::AcirParse;
  if (name == "acir-verify") return CompilerStage::AcirVerify;
  if (name == "acir-normalize") return CompilerStage::AcirNormalize;
  if (name == "topology-closure") return CompilerStage::TopologyClosure;
  return std::nullopt;
}
std::optional<ArtifactKind> parseArtifact(llvm::StringRef name) {
  if (name == "verified-acir" || name == "acir") return ArtifactKind::Acir;
  if (name == "report") return ArtifactKind::Report;
  return std::nullopt;
}
llvm::StringRef artifactName(ArtifactKind kind) {
  return kind == ArtifactKind::Acir ? "verified-acir" : "report";
}

bool parseStringTuple(PyObject *value, const char *label,
                      std::vector<std::string> &result) {
  if (!value || !PyTuple_Check(value)) {
    PyErr_Format(PyExc_TypeError, "%s must be a tuple", label);
    return false;
  }
  for (Py_ssize_t index = 0; index < PyTuple_Size(value); ++index) {
    auto item = pythonString(PyTuple_GetItem(value, index), label);
    if (!item) return false;
    result.push_back(std::move(*item));
  }
  return true;
}

std::optional<CompilerRequest> parseRequest(PyObject *value) {
  if (!PyDict_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "native compiler request must be a dictionary");
    return std::nullopt;
  }
  CompilerRequest request;
  auto bytes = pythonBytes(PyDict_GetItemString(value, "acir"), "acir");
  if (!bytes) return std::nullopt;
  request.acirBytes = std::move(*bytes);
  PyObject *stop = PyDict_GetItemString(value, "stop_after");
  if (stop && stop != Py_None) {
    auto name = pythonString(stop, "stop_after");
    if (!name || !(request.stopAfter = parseStage(*name))) {
      PyErr_SetString(PyExc_ValueError, "unknown native compiler stop stage");
      return std::nullopt;
    }
  }
  PyObject *emits = PyDict_GetItemString(value, "emits");
  if (!emits || !PyTuple_Check(emits)) {
    PyErr_SetString(PyExc_TypeError, "emits must be a tuple");
    return std::nullopt;
  }
  for (Py_ssize_t index = 0; index < PyTuple_Size(emits); ++index) {
    auto name = pythonString(PyTuple_GetItem(emits, index), "emit name");
    auto kind = name ? parseArtifact(*name) : std::nullopt;
    if (!kind) {
      PyErr_SetString(PyExc_ValueError, "unknown native compiler artifact");
      return std::nullopt;
    }
    request.emits.push_back(*kind);
  }
  PyObject *options = PyDict_GetItemString(value, "options");
  if (options && PyDict_Check(options)) {
    if (PyObject *profileValue = PyDict_GetItemString(options, "profile")) {
      auto profile = pythonString(profileValue, "profile");
      if (!profile) return std::nullopt;
      if (*profile == "fast") request.profile = CompilerProfile::Fast;
      else if (*profile == "validated") request.profile = CompilerProfile::Validated;
      else if (*profile == "custom") request.profile = CompilerProfile::Custom;
      else {
        PyErr_SetString(PyExc_ValueError, "profile must be fast, validated, or custom");
        return std::nullopt;
      }
    }
    if (PyObject *pipeline = PyDict_GetItemString(options, "custom_pipeline")) {
      auto text = pythonString(pipeline, "custom_pipeline");
      if (!text) return std::nullopt;
      request.customPipeline = std::move(*text);
    }
    if (PyObject *before = PyDict_GetItemString(options, "dump_before"))
      if (!parseStringTuple(before, "dump_before", request.dumpBefore)) return std::nullopt;
    if (PyObject *after = PyDict_GetItemString(options, "dump_after"))
      if (!parseStringTuple(after, "dump_after", request.dumpAfter)) return std::nullopt;
    if (PyObject *value = PyDict_GetItemString(options, "dump_after_each"))
      request.dumpAfterEach = PyObject_IsTrue(value);
    if (PyObject *verify = PyDict_GetItemString(options, "verify_after_each"))
      request.verifyAfterEach = PyObject_IsTrue(verify);
  }
  return request;
}

OwnedPy sourceToPython(const std::optional<SourceLocation> &source) {
  if (!source) return pyNone();
  OwnedPy result(PyDict_New());
  if (!result || !setItem(result.get(), "file", pyString(source->file)) ||
      !setItem(result.get(), "line", OwnedPy(PyLong_FromUnsignedLongLong(source->line))) ||
      !setItem(result.get(), "column", OwnedPy(PyLong_FromUnsignedLongLong(source->column))))
    return {};
  return result;
}
OwnedPy diagnosticToPython(const CompilerDiagnostic &diagnostic) {
  OwnedPy result(PyDict_New());
  if (!result || !setItem(result.get(), "stage", pyString(diagnostic.stage)) ||
      !setItem(result.get(), "code", pyString(diagnostic.code)) ||
      !setItem(result.get(), "severity", pyString(diagnostic.severity)) ||
      !setItem(result.get(), "message", pyString(diagnostic.message)) ||
      !setItem(result.get(), "source", sourceToPython(diagnostic.source))) return {};
  return result;
}
OwnedPy resultToPython(const CompilerResult &result) {
  OwnedPy output(PyDict_New());
  OwnedPy artifacts(PyTuple_New(result.artifacts.size()));
  OwnedPy diagnostics(PyTuple_New(result.diagnostics.size()));
  if (!output || !artifacts || !diagnostics) return {};
  for (size_t index = 0; index < result.artifacts.size(); ++index) {
    OwnedPy item(PyDict_New());
    const auto &artifact = result.artifacts[index];
    if (!item || !setItem(item.get(), "path", pyString(artifact.logicalPath)) ||
        !setItem(item.get(), "kind", pyString(artifactName(artifact.kind))) ||
        !setItem(item.get(), "data", pyBytes(artifact.bytes))) return {};
    PyTuple_SetItem(artifacts.get(), index, item.release());
  }
  for (size_t index = 0; index < result.diagnostics.size(); ++index) {
    OwnedPy item = diagnosticToPython(result.diagnostics[index]);
    if (!item) return {};
    PyTuple_SetItem(diagnostics.get(), index, item.release());
  }
  if (!setItem(output.get(), "artifacts", std::move(artifacts)) ||
      !setItem(output.get(), "diagnostics", std::move(diagnostics))) return {};
  return output;
}
CompilerResult failedResult(llvm::Error error) {
  CompilerResult result;
  llvm::handleAllErrors(std::move(error),
      [&](const CompilerError &failure) { result.diagnostics = failure.diagnostics(); },
      [&](const llvm::ErrorInfoBase &failure) {
        result.diagnostics.push_back({.stage = "native", .code = "ACIR-NATIVE-001",
                                      .severity = "error", .message = failure.message()});
      });
  return result;
}
PyObject *runCompiler(PyObject *, PyObject *arguments) {
  PyObject *object = nullptr;
  if (!PyArg_ParseTuple(arguments, "O:run_compiler", &object)) return nullptr;
  auto request = parseRequest(object);
  if (!request) return nullptr;
  auto compiled = acir::compiler::runCompiler(*request);
  OwnedPy result = compiled ? resultToPython(*compiled)
                            : resultToPython(failedResult(compiled.takeError()));
  return result.release();
}
PyObject *capabilities(PyObject *, PyObject *) {
  OwnedPy result(PyDict_New());
  if (!result || !setItem(result.get(), "compiler_build_id", pyString("agentic-circuit-0.1.0+llvm-22.1.8")) ||
      !setItem(result.get(), "runtime_build_id", pyString("gfsim-0.1.0+cxx20")) ||
      !setItem(result.get(), "items", OwnedPy(PyTuple_New(0)))) return nullptr;
  return result.release();
}
PyMethodDef methods[] = {{"run_compiler", runCompiler, METH_VARARGS, nullptr},
                         {"capabilities", capabilities, METH_NOARGS, nullptr},
                         {nullptr, nullptr, 0, nullptr}};
PyModuleDef module = {PyModuleDef_HEAD_INIT, "_native", nullptr, -1, methods};
} // namespace
PyMODINIT_FUNC PyInit__native() { return PyModule_Create(&module); }
