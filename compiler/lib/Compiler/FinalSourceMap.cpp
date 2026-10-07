#include "FinalSourceMap.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "FinalCppNames.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <tuple>

using namespace mlir;

namespace acir::compiler {
namespace {

struct OriginRow {
  std::string operation;
  std::string origin;
  std::string location;
};

bool byteLess(StringRef left, StringRef right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(), [](char a, char b) {
        return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
      });
}

bool rowLess(const OriginRow &left, const OriginRow &right) {
  if (left.operation != right.operation)
    return byteLess(left.operation, right.operation);
  if (left.origin != right.origin)
    return byteLess(left.origin, right.origin);
  return byteLess(left.location, right.location);
}

bool rowEqual(const OriginRow &left, const OriginRow &right) {
  return left.operation == right.operation && left.origin == right.origin &&
         left.location == right.location;
}

FailureOr<std::string>
printAndVerifyOccurrence(DictionaryAttr occurrence, MLIRContext *context,
                         ac::detail::EmitError emitError) {
  if (!occurrence ||
      failed(ac::detail::verifyOccurrence(occurrence, emitError)))
    return emitError() << "final source map origin is not a valid Occurrence";
  std::string text;
  llvm::raw_string_ostream stream(text);
  occurrence.print(stream);
  stream.flush();
  Attribute parsed = parseAttribute(text, context);
  auto parsedOccurrence = dyn_cast_or_null<DictionaryAttr>(parsed);
  if (!parsedOccurrence || parsedOccurrence != occurrence ||
      failed(ac::detail::verifyOccurrence(parsedOccurrence, emitError)))
    return emitError()
           << "final source map Occurrence does not round-trip canonically";
  std::string canonical;
  llvm::raw_string_ostream canonicalStream(canonical);
  parsedOccurrence.print(canonicalStream);
  canonicalStream.flush();
  if (canonical != text)
    return emitError() << "final source map Occurrence printing is not stable";
  return text;
}

FailureOr<std::string> printAndVerifyLocation(Location location,
                                              MLIRContext *context,
                                              ac::detail::EmitError emitError) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  location.print(stream);
  stream.flush();
  auto parsed = dyn_cast_or_null<LocationAttr>(parseAttribute(text, context));
  if (!parsed || parsed != location)
    return emitError()
           << "final source map Location does not round-trip canonically";
  std::string canonical;
  llvm::raw_string_ostream canonicalStream(canonical);
  parsed.print(canonicalStream);
  canonicalStream.flush();
  if (canonical != text)
    return emitError() << "final source map Location printing is not stable";
  return text;
}

bool safeRelativePath(StringRef path) {
  if (path.empty() || path.front() == '/' || path.contains('\\') ||
      path.contains('\0') || path.contains(':'))
    return false;
  SmallVector<StringRef> components;
  path.split(components, '/', -1, true);
  return llvm::all_of(components, [](StringRef component) {
    return !component.empty() && component != "." && component != "..";
  });
}

FailureOr<SmallVector<OriginRow>>
collectOrigins(ArrayRef<Operation *> definitions,
               ac::detail::EmitError emitError) {
  SmallVector<OriginRow> rows;
  LogicalResult result = success();
  for (Operation *definition : definitions)
    definition->walk([&](Operation *operation) {
      if (failed(result))
        return;
      Attribute rawOccurrence = operation->getAttr("ac.origin");
      if (!rawOccurrence)
        rawOccurrence = operation->getAttr("occurrence");
      if (!rawOccurrence)
        return;
      auto origin =
          printAndVerifyOccurrence(dyn_cast<DictionaryAttr>(rawOccurrence),
                                   operation->getContext(), emitError);
      auto location = printAndVerifyLocation(
          operation->getLoc(), operation->getContext(), emitError);
      if (failed(origin) || failed(location)) {
        result = failure();
        return;
      }
      rows.push_back({operation->getName().getStringRef().str(),
                      std::move(*origin), std::move(*location)});
    });
  if (failed(result))
    return failure();
  llvm::sort(rows, rowLess);
  rows.erase(std::unique(rows.begin(), rows.end(), rowEqual), rows.end());
  return rows;
}

FailureOr<std::string> sourceMapPath(DictionaryAttr owner,
                                     ac::detail::EmitError emitError) {
  auto components = sourceOwnerComponents(owner, emitError);
  if (failed(components))
    return failure();
  return sourcePathName(components->filePath, ".source-map.json", emitError);
}

FailureOr<std::string> serializeMap(DictionaryAttr owner,
                                    ArrayRef<std::string> files,
                                    ArrayRef<OriginRow> origins,
                                    ac::detail::EmitError emitError) {
  auto package = owner.getAs<StringAttr>("package");
  auto path = owner.getAs<StringAttr>("path");
  if (failed(ac::detail::verifySourceOwner(owner, emitError)) || !package ||
      !path)
    return emitError() << "final source map has an invalid SourceOwner";

  llvm::json::Array generatedFiles;
  for (const std::string &file : files)
    generatedFiles.emplace_back(file);
  llvm::json::Array originArray;
  for (const OriginRow &origin : origins) {
    if (origin.operation.empty() || origin.origin.empty() ||
        origin.location.empty())
      return emitError() << "final source map contains an empty origin field";
    originArray.emplace_back(llvm::json::Object{
        {"operation", origin.operation},
        {"origin", origin.origin},
        {"location", origin.location},
    });
  }
  llvm::json::Object source{{"package", package.getValue().str()},
                            {"path", path.getValue().str()}};
  llvm::json::Object map{
      {"kind", "pycircuit-source-map"},
      {"source", std::move(source)},
      {"generated_files", std::move(generatedFiles)},
      {"origins", std::move(originArray)},
  };
  std::string text;
  llvm::raw_string_ostream output(text);
  llvm::json::OStream json(output, 2);
  json.value(llvm::json::Value(std::move(map)));
  output << '\n';
  output.flush();

  auto parsed = llvm::json::parse(text);
  if (!parsed)
    return emitError() << "native source map JSON failed to parse";
  auto parsedMap = parsed->getAsObject();
  if (!parsedMap || parsedMap->size() != 4 || !parsedMap->getString("kind") ||
      *parsedMap->getString("kind") != "pycircuit-source-map")
    return emitError() << "native source map JSON failed schema validation";
  auto parsedSource = parsedMap->getObject("source");
  if (!parsedSource || parsedSource->size() != 2 ||
      parsedSource->getString("package") != package.getValue() ||
      parsedSource->getString("path") != path.getValue())
    return emitError() << "native source map SourceOwner failed validation";
  auto parsedFiles = parsedMap->getArray("generated_files");
  auto parsedOrigins = parsedMap->getArray("origins");
  if (!parsedFiles || !parsedOrigins || parsedFiles->size() != files.size() ||
      parsedOrigins->size() != origins.size())
    return emitError() << "native source map arrays failed schema validation";
  for (auto [index, file] : llvm::enumerate(*parsedFiles))
    if (file.getAsString() != files[index])
      return emitError()
             << "native source map generated_files changed on parse";
  for (auto [index, row] : llvm::enumerate(*parsedOrigins)) {
    auto object = row.getAsObject();
    const OriginRow &expected = origins[index];
    if (!object || object->size() != 3 ||
        object->getString("operation") != expected.operation ||
        object->getString("origin") != expected.origin ||
        object->getString("location") != expected.location)
      return emitError() << "native source map origin failed schema validation";
  }
  if (failed(validateFinalSourceMapText(text, *owner.getContext(), emitError)))
    return failure();
  return text;
}

LogicalResult validateMapText(StringRef text, MLIRContext &context,
                              ac::detail::EmitError emitError,
                              std::string *canonicalPath) {
  auto parsed = llvm::json::parse(text);
  if (!parsed)
    return emitError() << "source map is not valid JSON";
  auto map = parsed->getAsObject();
  auto kind = map ? map->getString("kind") : std::nullopt;
  auto source = map ? map->getObject("source") : nullptr;
  auto files = map ? map->getArray("generated_files") : nullptr;
  auto origins = map ? map->getArray("origins") : nullptr;
  if (!map || map->size() != 4 || !kind || *kind != "pycircuit-source-map" ||
      !source || source->size() != 2 || !files || !origins)
    return emitError() << "source map has unknown, missing or mistyped fields";
  auto package = source->getString("package");
  auto path = source->getString("path");
  if (!package || !path)
    return emitError() << "source map SourceOwner fields must be strings";
  Builder builder(&context);
  auto owner = builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr(*package)),
      builder.getNamedAttr("path", builder.getStringAttr(*path)),
  });
  if (failed(ac::detail::verifySourceOwner(owner, emitError)))
    return emitError() << "source map contains an invalid SourceOwner";

  std::string previousFile;
  bool havePreviousFile = false;
  for (const llvm::json::Value &fileValue : *files) {
    auto file = fileValue.getAsString();
    if (!file || !safeRelativePath(*file))
      return emitError() << "source map generated_files has an unsafe path";
    if (havePreviousFile && !byteLess(previousFile, *file))
      return emitError()
             << "source map generated_files must be sorted and unique";
    previousFile = file->str();
    havePreviousFile = true;
  }

  OriginRow previous;
  bool havePrevious = false;
  for (const llvm::json::Value &originValue : *origins) {
    auto object = originValue.getAsObject();
    auto operation = object ? object->getString("operation") : std::nullopt;
    auto originText = object ? object->getString("origin") : std::nullopt;
    auto locationText = object ? object->getString("location") : std::nullopt;
    if (!object || object->size() != 3 || !operation || operation->empty() ||
        !originText || originText->empty() || !locationText ||
        locationText->empty())
      return emitError() << "source map origin has invalid fields";
    Attribute parsedOrigin = parseAttribute(*originText, &context);
    auto occurrence = dyn_cast_or_null<DictionaryAttr>(parsedOrigin);
    auto checkedOrigin =
        printAndVerifyOccurrence(occurrence, &context, emitError);
    auto parsedLocation =
        dyn_cast_or_null<LocationAttr>(parseAttribute(*locationText, &context));
    if (!parsedLocation)
      return emitError() << "source map location is not a Location attribute";
    auto checkedLocation =
        printAndVerifyLocation(Location(parsedLocation), &context, emitError);
    if (failed(checkedOrigin) || failed(checkedLocation) ||
        *checkedOrigin != *originText || *checkedLocation != *locationText)
      return emitError()
             << "source map origin/location is not canonical MLIR text";
    OriginRow current{operation->str(), originText->str(), locationText->str()};
    if (havePrevious && !rowLess(previous, current))
      return emitError() << "source map origins must be sorted and unique";
    previous = std::move(current);
    havePrevious = true;
  }
  auto pathForOwner = sourceMapPath(owner, emitError);
  if (failed(pathForOwner))
    return failure();
  if (canonicalPath)
    *canonicalPath = std::move(*pathForOwner);
  return success();
}

} // namespace

LogicalResult validateFinalSourceMapText(StringRef text, MLIRContext &context,
                                         ac::detail::EmitError emitError,
                                         std::string *canonicalPath) {
  return validateMapText(text, context, emitError, canonicalPath);
}

FailureOr<SmallVector<FinalSourceMap>>
emitFinalSourceMaps(ModuleOp package,
                    ArrayRef<FinalSourceMembership> membership,
                    ac::detail::EmitError emitError) {
  if (!package || failed(ac::verifyHardwarePackage(package)))
    return emitError() << "final source maps require verified hardware IR";
  auto units = ac::collectFinalSourceUnits(package, emitError);
  if (failed(units))
    return failure();
  llvm::DenseMap<Attribute, unsigned> unitIndices;
  for (auto [index, unit] : llvm::enumerate(*units))
    unitIndices.try_emplace(unit.owner, index);

  llvm::DenseMap<Attribute, SmallVector<std::string>> filesByOwner;
  for (const FinalSourceMembership &group : membership) {
    if (!group.sourceOwner || !unitIndices.contains(group.sourceOwner) ||
        !filesByOwner.try_emplace(group.sourceOwner).second ||
        failed(ac::detail::verifySourceOwner(group.sourceOwner, emitError)))
      return emitError() << "native source membership has a missing, extra or "
                            "duplicate owner";
    SmallVector<std::string> files = group.generatedFiles;
    for (const std::string &file : files)
      if (!safeRelativePath(file))
        return emitError() << "native source membership has an unsafe path";
    llvm::sort(files, [](const std::string &left, const std::string &right) {
      return byteLess(left, right);
    });
    if (std::adjacent_find(files.begin(), files.end()) != files.end())
      return emitError() << "native source membership repeats a generated file";
    filesByOwner[group.sourceOwner] = std::move(files);
  }
  if (filesByOwner.size() != units->size())
    return emitError() << "native source membership omits a final SourceOwner";

  SmallVector<FinalSourceMap> maps;
  std::set<std::string> paths;
  for (const ac::FinalSourceUnitView &unit : *units) {
    DictionaryAttr owner = unit.owner;
    auto origins = collectOrigins(unit.declarations, emitError);
    auto path = sourceMapPath(owner, emitError);
    if (failed(origins) || failed(path))
      return failure();
    if (!paths.insert(*path).second)
      return emitError() << "legalized source-map paths collide";
    auto files = filesByOwner.lookup(owner);
    auto text = serializeMap(owner, files, *origins, emitError);
    if (failed(text))
      return failure();
    maps.push_back(
        {owner, std::move(*path), std::move(*text), std::move(files)});
  }
  return maps;
}

} // namespace acir::compiler
