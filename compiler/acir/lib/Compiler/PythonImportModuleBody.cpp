#include "PythonImportModules.h"
#include "PythonImportRules.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/STLExtras.h"

#include <tuple>

using namespace mlir;

namespace acir::compiler::detail {
LogicalResult ModuleCompiler::emitBody(ModuleModel &model,
                                       RuleCompiler &rules) {
  OpBuilder &builder = sourceCompiler.builder;
  SmallVector<Attribute> currentPorts;
  SmallVector<Attribute> nextPorts;
  auto appendPort = [&](const ModuleParameter &parameter, StringRef role,
                        SmallVectorImpl<Attribute> &ports) {
    AstNode formal = parameter.syntax.parameter;
    AstNode relative = relativeToModule(model.declaration, formal);
    ports.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("parameter",
                             builder.getStringAttr(parameter.name)),
        builder.getNamedAttr("ordinal", builder.getUnitAttr()),
        builder.getNamedAttr("role", builder.getStringAttr(role)),
        builder.getNamedAttr("type", parameter.type),
        builder.getNamedAttr("origin",
                             occurrence(builder, model.symbol, relative)),
        builder.getNamedAttr(
            "location",
            sourceSpan(builder, sourceCompiler.source.path, formal)),
    }));
  };
  auto aggregateEffects = [&](const ModuleParameter &parameter) {
    bool found = false;
    bool read = false;
    bool write = false;
    for (const ModuleMember &member : model.members) {
      if (member.kind != ModuleMember::Kind::Connection ||
          member.parameter != parameter.name)
        continue;
      found = true;
      read |= member.read;
      write |= member.write;
    }
    return std::tuple(found, read, write);
  };
  for (const ModuleParameter &parameter : model.parameters) {
    auto [found, read, write] = aggregateEffects(parameter);
    if (!found)
      return sourceCompiler.emitError()
             << "module port parameter has no member";
    if (!read && !write)
      return sourceCompiler.emitError()
             << "module connection has no registered rule or child effect";
    if (read)
      appendPort(parameter, "current", currentPorts);
  }
  for (const ModuleParameter &parameter : model.parameters) {
    auto [found, read, write] = aggregateEffects(parameter);
    (void)found;
    (void)read;
    if (write)
      appendPort(parameter, "next", nextPorts);
  }
  SmallVector<Attribute> ports;
  llvm::append_range(ports, currentPorts);
  llvm::append_range(ports, nextPorts);

  SmallVector<NamedAttribute> attributes{
      builder.getNamedAttr(
          "name", builder.getStringAttr(model.declaration.string("name"))),
      builder.getNamedAttr(SymbolTable::getSymbolAttrName(),
                           builder.getStringAttr(model.symbol.getValue())),
      builder.getNamedAttr("ac.source_owner", sourceCompiler.owner),
      builder.getNamedAttr(
          "ac.origin",
          occurrence(builder, model.symbol,
                     relativeToModule(model.declaration, model.declaration))),
      builder.getNamedAttr("ac.declaration_role",
                           builder.getStringAttr("definition")),
      builder.getNamedAttr(
          "ac.control_ports",
          builder.getDictionaryAttr({
              builder.getNamedAttr("clock", builder.getI32IntegerAttr(0)),
              builder.getNamedAttr("reset", builder.getI32IntegerAttr(1)),
          })),
      builder.getNamedAttr("ac.ports", builder.getArrayAttr(ports)),
  };
  if (model.isSystem)
    attributes.push_back(
        builder.getNamedAttr("ac.root_kind", builder.getStringAttr("system")));
  builder.setInsertionPointToEnd(sourceCompiler.body->getBody());
  Operation *moduleOperation = createSourceOperation(
      builder,
      model.declaration.location(builder.getContext(),
                                 sourceCompiler.source.path),
      ac::ModuleOp::getOperationName(), {}, {}, attributes, 1);
  Block *body = new Block();
  moduleOperation->getRegion(0).push_back(body);
  Value clock =
      body->addArgument(builder.getI1Type(), moduleOperation->getLoc());
  Value reset =
      body->addArgument(builder.getI1Type(), moduleOperation->getLoc());
  for (Attribute rawPort : ports) {
    auto port = cast<DictionaryAttr>(rawPort);
    auto logical = port.getAs<DictionaryAttr>("type");
    Type payload = physicalType(logical, builder.getContext());
    Value handle =
        body->addArgument(ac::RegType::get(builder.getContext(), payload),
                          moduleOperation->getLoc());
    StringRef parameter = port.getAs<StringAttr>("parameter").getValue();
    StringRef role = port.getAs<StringAttr>("role").getValue();
    for (ModuleMember &member : model.members) {
      if (member.kind != ModuleMember::Kind::Connection ||
          member.parameter != parameter)
        continue;
      if (role == "current")
        member.currentHandle = handle;
      else
        member.nextHandle = handle;
    }
  }
  OpBuilder at = OpBuilder::atBlockEnd(body);
  Operation *terminator =
      createSourceOperation(at, moduleOperation->getLoc(),
                            ac::YieldOp::getOperationName(), {}, {}, {});

  for (const ModuleAction &action : model.actions) {
    builder.setInsertionPoint(terminator);
    if (action.kind == ModuleAction::Kind::RuleRegistration) {
      if (failed(rules.emit(action.index, moduleOperation)))
        return failure();
      continue;
    }
    ModuleMember &member = model.members[action.index];
    if (member.kind == ModuleMember::Kind::Connection)
      continue;
    if (member.kind == ModuleMember::Kind::OwnedState) {
      DictionaryAttr declaration =
          occurrence(builder, model.symbol,
                     relativeToModule(model.declaration, member.declaration));
      auto emitState = [&](StringRef name, ArrayAttr shape, ArrayAttr element) {
        SmallVector<NamedAttribute> stateAttributes{
            builder.getNamedAttr("name", builder.getStringAttr(name)),
            builder.getNamedAttr("ac.source_owner", sourceCompiler.owner),
            builder.getNamedAttr("ac.declaration", declaration),
            builder.getNamedAttr("ac.logical_element", member.logicalType),
            builder.getNamedAttr("ac.shape", shape),
            builder.getNamedAttr("ac.element", element),
            builder.getNamedAttr("ac.initial_value", member.initialValue),
            builder.getNamedAttr("ac.domain", builder.getStringAttr("default")),
        };
        Operation *state = createSourceOperation(
            builder,
            member.declaration.location(builder.getContext(),
                                        sourceCompiler.source.path),
            ac::RegOp::getOperationName(), ValueRange{clock, reset},
            TypeRange{
                ac::RegType::get(builder.getContext(), member.payloadType)},
            stateAttributes);
        return state->getResult(0);
      };
      if (member.isCollection()) {
        auto values = cast<DictionaryAttr>(member.initialValue)
                          .getAs<ArrayAttr>("values");
        member.elementHandles.reserve(values.size());
        for (size_t element = 0; element < values.size(); ++element) {
          std::string elementName =
              (Twine(member.name) + "__" + Twine(element)).str();
          member.elementHandles.push_back(emitState(
              elementName, member.shape,
              builder.getArrayAttr({builder.getI64IntegerAttr(element)})));
        }
        continue;
      }
      member.handle = emitState(member.name, builder.getArrayAttr({}),
                                builder.getArrayAttr({}));
      member.currentHandle = member.handle;
      member.nextHandle = member.handle;
      continue;
    }
    if (member.kind == ModuleMember::Kind::ChildInstance) {
      SmallVector<Value> inputs, outputs;
      for (size_t memberIndex : member.childInputMembers) {
        ModuleMember &actual = model.members[memberIndex];
        Value handle = actual.kind == ModuleMember::Kind::OwnedState
                           ? actual.handle
                           : actual.currentHandle;
        if (!handle)
          return sourceCompiler.emitError()
                 << "module instance input has no current reg handle";
        inputs.push_back(handle);
      }
      for (size_t memberIndex : member.childOutputMembers) {
        ModuleMember &actual = model.members[memberIndex];
        Value handle = actual.kind == ModuleMember::Kind::OwnedState
                           ? actual.handle
                           : actual.nextHandle;
        if (!handle)
          return sourceCompiler.emitError()
                 << "module instance output has no next reg handle";
        outputs.push_back(handle);
      }
      AstNode statement = member.declaration;
      AstNode call = statement.child("value");
      SmallVector<NamedAttribute> instanceAttributes{
          builder.getNamedAttr("name", builder.getStringAttr(member.name)),
          builder.getNamedAttr("callee", member.child),
          builder.getNamedAttr("operandSegmentSizes",
                               builder.getDenseI32ArrayAttr(
                                   {1, 1, static_cast<int32_t>(inputs.size()),
                                    static_cast<int32_t>(outputs.size())})),
          builder.getNamedAttr("ac.static_args", builder.getArrayAttr({})),
          builder.getNamedAttr(
              "ac.origin",
              occurrence(builder, model.symbol,
                         relativeToModule(model.declaration, call))),
      };
      SmallVector<Value> operands;
      operands.push_back(clock);
      operands.push_back(reset);
      llvm::append_range(operands, inputs);
      llvm::append_range(operands, outputs);
      SmallVector<Type> results;
      for (Value output : outputs) {
        Type payload = cast<ac::RegType>(output.getType()).getElementType();
        results.push_back(payload);
        results.push_back(builder.getI1Type());
      }
      createSourceOperation(
          builder,
          call.location(builder.getContext(), sourceCompiler.source.path),
          ac::InstanceOp::getOperationName(), operands, results,
          instanceAttributes);
    }
  }
  return success();
}

} // namespace acir::compiler::detail
