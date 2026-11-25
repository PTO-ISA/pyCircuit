#ifndef PYCIRCUIT_CONVERSION_LOWERSTRUCTUREDINPUTS_H
#define PYCIRCUIT_CONVERSION_LOWERSTRUCTUREDINPUTS_H

#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include <functional>
#include <string>

namespace acir::detail {

// A partial Struct has one physical input per leaf so each availability
// companion can be verified against its own payload. The body retains the
// nominal type reconstructed from those inputs.
struct StructuredInputLayout {
  StructuredInputLayout(const gfir::StructuredDefinition &facts, mlir::Builder &builder)
      : facts(facts), companionPayloads(facts.inputAvailability.size()) {
    auto source = facts.module;
    auto inputs = source.getFunctionType().getInputs();
    flattened.resize(inputs.size());
    for (const auto &input : facts.inputAvailability)
      if (!input.path.empty())
        flattened.set(input.port);

    llvm::StringSet<> usedNames;
    for (auto list : {source.getInputNames(), source.getOutputNames()})
      for (auto name : list)
        usedNames.insert(mlir::cast<mlir::StringAttr>(name).getValue());
    for (const auto &input : facts.inputAvailability)
      usedNames.insert(input.name.getValue());
    for (const auto &output : facts.outputAvailability)
      usedNames.insert(output.name.getValue());

    for (auto [port, type] : llvm::enumerate(inputs)) {
      firstPorts.push_back(types.size());
      for (auto [ordinal, input] : llvm::enumerate(facts.inputAvailability)) {
        if (input.port != port)
          continue;
        companionPayloads[ordinal] = types.size();
        if (!flattened.test(port))
          continue;
        auto name = "__pyc_field_in_" + std::to_string(port) + "_" + std::to_string(ordinal);
        while (!usedNames.insert(name).second)
          name += "_";
        types.push_back(facts.wires[input.wire].type);
        names.push_back(builder.getStringAttr(name));
      }
      if (!flattened.test(port)) {
        types.push_back(type);
        names.push_back(source.getInputNames()[port]);
      }
    }
  }

  mlir::LogicalResult mapInputs(mlir::Block &block, mlir::OpBuilder &builder,
                                const ac::HardwareAnalysis &hardware,
                                mlir::IRMapping &mapping) const {
    auto source = facts.module;
    for (auto argument : source.getBody().front().getArguments()) {
      unsigned port = argument.getArgNumber();
      if (!flattened.test(port)) {
        mapping.map(argument, block.getArgument(firstPorts[port]));
        continue;
      }
      llvm::SmallVector<mlir::StringAttr> path;
      std::function<mlir::FailureOr<mlir::Value>(mlir::Type)> rebuild =
          [&](mlir::Type type) -> mlir::FailureOr<mlir::Value> {
        if (auto record = mlir::dyn_cast<ac::StructType>(type)) {
          llvm::SmallVector<mlir::Value> fields;
          for (auto raw : hardware.lookupStruct(record).getFields()) {
            auto field = mlir::cast<mlir::DictionaryAttr>(raw);
            path.push_back(field.getAs<mlir::StringAttr>("name"));
            auto value = rebuild(field.getAs<mlir::TypeAttr>("type").getValue());
            path.pop_back();
            if (mlir::failed(value))
              return mlir::failure();
            fields.push_back(*value);
          }
          mlir::OperationState state(source.getLoc(), ac::StructCreateOp::getOperationName());
          state.addOperands(fields);
          state.addTypes(type);
          return builder.create(state)->getResult(0);
        }
        for (auto [ordinal, input] : llvm::enumerate(facts.inputAvailability))
          if (input.port == port && input.path == path)
            return block.getArgument(companionPayloads[ordinal]);
        return mlir::failure();
      };
      auto value = rebuild(argument.getType());
      if (mlir::failed(value))
        return source.emitOpError("missing verified nominal input field");
      mapping.map(argument, *value);
    }
    return mlir::success();
  }

private:
  const gfir::StructuredDefinition &facts;
  llvm::BitVector flattened;
  llvm::SmallVector<unsigned> firstPorts;

public:
  llvm::SmallVector<unsigned> companionPayloads;
  llvm::SmallVector<mlir::Type> types;
  llvm::SmallVector<mlir::Attribute> names;
};

} // namespace acir::detail

#endif
