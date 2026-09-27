#include "PythonImportModules.h"
#include "PythonImportRules.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler::detail {
LogicalResult ModuleCompiler::emitBody(ModuleModel &model,
                                       RuleCompiler &rules) {
  OpBuilder &builder = sourceCompiler.builder;
  SmallVector<Attribute> currentPorts;
  SmallVector<Attribute> nextPorts;
  llvm::StringMap<size_t> memberForParameter;
  for (size_t index = 0; index < model.members.size(); ++index) {
    const ModuleMember &member = model.members[index];
    if (member.kind == ModuleMember::Kind::Connection)
      memberForParameter[member.parameter] = index;
  }
  auto appendPort = [&](const ModuleParameter &parameter,
                        const ModuleMember &member, StringRef role,
                        SmallVectorImpl<Attribute> &ports) {
    AstNode formal = parameter.syntax.parameter;
    AstNode relative = relativeToModule(model.declaration, formal);
    ports.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("parameter",
                             builder.getStringAttr(parameter.name)),
        builder.getNamedAttr("ordinal", builder.getUnitAttr()),
        builder.getNamedAttr("role", builder.getStringAttr(role)),
        builder.getNamedAttr("type", member.logicalType),
        builder.getNamedAttr("origin",
                             occurrence(builder, model.symbol, relative)),
        builder.getNamedAttr(
            "location",
            sourceSpan(builder, sourceCompiler.source.path, formal)),
    }));
  };
  for (const ModuleParameter &parameter : model.parameters) {
    auto found = memberForParameter.find(parameter.name);
    if (found == memberForParameter.end())
      return sourceCompiler.emitError()
             << "module port parameter has no member";
    const ModuleMember &member = model.members[found->second];
    if (!member.read && !member.write)
      return sourceCompiler.emitError()
             << "module connection has no registered rule or child effect";
    if (member.read)
      appendPort(parameter, member, "current", currentPorts);
  }
  for (const ModuleParameter &parameter : model.parameters) {
    const ModuleMember &member =
        model.members[memberForParameter.lookup(parameter.name)];
    if (member.write)
      appendPort(parameter, member, "next", nextPorts);
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
      builder.getNamedAttr("ac.ports", builder.getArrayAttr(ports)),
  };
  builder.setInsertionPointToEnd(sourceCompiler.body->getBody());
  Operation *moduleOperation = createSourceOperation(
      builder,
      model.declaration.location(builder.getContext(),
                                 sourceCompiler.source.path),
      ac::ModuleOp::getOperationName(), {}, {}, attributes, 1);
  Block *body = new Block();
  moduleOperation->getRegion(0).push_back(body);
  for (Attribute rawPort : ports) {
    auto port = cast<DictionaryAttr>(rawPort);
    auto logical = port.getAs<DictionaryAttr>("type");
    Type payload = physicalType(logical, builder.getContext());
    Value handle =
        body->addArgument(ac::DffeType::get(builder.getContext(), payload),
                          moduleOperation->getLoc());
    StringRef parameter = port.getAs<StringAttr>("parameter").getValue();
    StringRef role = port.getAs<StringAttr>("role").getValue();
    ModuleMember &member = model.members[memberForParameter.lookup(parameter)];
    if (role == "current")
      member.currentHandle = handle;
    else
      member.nextHandle = handle;
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
      SmallVector<NamedAttribute> stateAttributes{
          builder.getNamedAttr("name", builder.getStringAttr(member.name)),
          builder.getNamedAttr("ac.source_owner", sourceCompiler.owner),
          builder.getNamedAttr(
              "ac.declaration",
              occurrence(
                  builder, model.symbol,
                  relativeToModule(model.declaration, member.declaration))),
          builder.getNamedAttr("ac.logical_element", member.logicalType),
          builder.getNamedAttr("ac.shape", builder.getArrayAttr({})),
          builder.getNamedAttr("ac.initial_value", member.initialValue),
          builder.getNamedAttr("ac.domain", builder.getStringAttr("default")),
      };
      Operation *state = createSourceOperation(
          builder,
          member.declaration.location(builder.getContext(),
                                      sourceCompiler.source.path),
          ac::DffeOp::getOperationName(), {},
          TypeRange{
              ac::DffeType::get(builder.getContext(), member.payloadType)},
          stateAttributes);
      member.handle = state->getResult(0);
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
                 << "module instance input has no current DFFE handle";
        inputs.push_back(handle);
      }
      for (size_t memberIndex : member.childOutputMembers) {
        ModuleMember &actual = model.members[memberIndex];
        Value handle = actual.kind == ModuleMember::Kind::OwnedState
                           ? actual.handle
                           : actual.nextHandle;
        if (!handle)
          return sourceCompiler.emitError()
                 << "module instance output has no next DFFE handle";
        outputs.push_back(handle);
      }
      AstNode statement = member.declaration;
      AstNode call = statement.child("value");
      SmallVector<NamedAttribute> instanceAttributes{
          builder.getNamedAttr("name", builder.getStringAttr(member.name)),
          builder.getNamedAttr("callee", member.child),
          builder.getNamedAttr("operandSegmentSizes",
                               builder.getDenseI32ArrayAttr(
                                   {static_cast<int32_t>(inputs.size()),
                                    static_cast<int32_t>(outputs.size())})),
          builder.getNamedAttr("ac.static_args", builder.getArrayAttr({})),
          builder.getNamedAttr(
              "ac.origin",
              occurrence(builder, model.symbol,
                         relativeToModule(model.declaration, call))),
      };
      SmallVector<Value> operands;
      llvm::append_range(operands, inputs);
      llvm::append_range(operands, outputs);
      createSourceOperation(
          builder,
          call.location(builder.getContext(), sourceCompiler.source.path),
          ac::InstanceOp::getOperationName(), operands, {}, instanceAttributes);
    }
  }
  return success();
}

} // namespace acir::compiler::detail
