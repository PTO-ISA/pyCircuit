#include "pycircuit/Transforms/Passes.h"
#include "pycircuit/Transforms/SourceControlFlow.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Pass/Pass.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <limits>
#include <optional>

using namespace mlir;

namespace acir {
namespace {
namespace ac = acir::ac;

// Reuse the existing source-check work ceiling. This is an expansion guard,
// not a user-authored termination claim or an additional hardware schedule.
struct Normalizer {
  mlir::ModuleOp package;
  uint64_t remaining = ac::HardwareSourceCheckLimits{}.maxWorkUnits;

  LogicalResult charge(Operation *site, uint64_t amount = 1) {
    if (amount > remaining)
      return site->emitOpError()
             << "source control-flow expansion limit exceeded";
    remaining -= amount;
    return success();
  }

  using ConstantCache = llvm::DenseMap<Value, std::optional<llvm::APInt>>;
  std::optional<llvm::APInt> constant(Value value, ConstantCache &cache,
                                      unsigned depth) {
    auto found = cache.find(value);
    if (found != cache.end())
      return found->second;
    cache.try_emplace(value, std::nullopt);
    auto result = constantImpl(value, cache, depth);
    cache[value] = result;
    return result;
  }
  std::optional<llvm::APInt> constant(Value value) {
    ConstantCache cache;
    return constant(value, cache, 0);
  }
  std::optional<llvm::APInt> constantImpl(Value value, ConstantCache &cache,
                                          unsigned depth) {
    if (depth > 128)
      return std::nullopt;
    auto *op = value.getDefiningOp();
    if (!op || failed(charge(op)))
      return std::nullopt;
    if (auto c = dyn_cast<arith::ConstantOp>(op)) {
      if (auto integer = dyn_cast<IntegerAttr>(c.getValue()))
        return integer.getValue();
      return std::nullopt;
    }
    if (op->getNumOperands() == 2) {
      auto lhs = constant(op->getOperand(0), cache, depth + 1);
      auto rhs = constant(op->getOperand(1), cache, depth + 1);
      if (!lhs || !rhs || lhs->getBitWidth() != rhs->getBitWidth())
        return std::nullopt;
      if (isa<arith::AddIOp>(op))
        return *lhs + *rhs;
      if (isa<arith::SubIOp>(op))
        return *lhs - *rhs;
      if (isa<arith::MulIOp>(op))
        return *lhs * *rhs;
      if (isa<arith::AndIOp>(op))
        return *lhs & *rhs;
      if (isa<arith::OrIOp>(op))
        return *lhs | *rhs;
      if (isa<arith::XOrIOp>(op))
        return *lhs ^ *rhs;
      if (auto cmp = dyn_cast<arith::CmpIOp>(op))
        return llvm::APInt(
            1, arith::applyCmpPredicate(cmp.getPredicate(), *lhs, *rhs));
    }
    if (auto select = dyn_cast<arith::SelectOp>(op)) {
      auto condition = constant(select.getCondition(), cache, depth + 1);
      if (condition)
        return constant(condition->isZero() ? select.getFalseValue()
                                            : select.getTrueValue(),
                        cache, depth + 1);
    }
    if (isa<arith::IndexCastOp, arith::IndexCastUIOp, arith::ExtSIOp,
            arith::ExtUIOp, arith::TruncIOp>(op)) {
      auto input = constant(op->getOperand(0), cache, depth + 1);
      if (!input)
        return std::nullopt;
      auto type = value.getType();
      unsigned width = type.isIndex() ? 64 : cast<IntegerType>(type).getWidth();
      return isa<arith::IndexCastUIOp, arith::ExtUIOp>(op)
                 ? input->zextOrTrunc(width)
                 : input->sextOrTrunc(width);
    }
    return std::nullopt;
  }

  FailureOr<DictionaryAttr> loopOccurrence(Operation *loop) {
    auto origin = loop->getAttrOfType<DictionaryAttr>("occurrence");
    if (failed(ac::detail::verifyOccurrence(
            origin, [&] { return loop->emitOpError(); })))
      return failure();
    return origin;
  }

  FailureOr<Attribute> expand(Attribute attribute, DictionaryAttr frame,
                              unsigned prefixLength, Operation *site,
                              unsigned depth = 0) {
    if (depth > 128 || failed(charge(site)))
      return failure();
    if (auto expression = dyn_cast<ac::StaticExprAttr>(attribute)) {
      auto tree = expand(expression.getTree(), frame, prefixLength, site, depth + 1);
      if (failed(tree))
        return failure();
      return Attribute(ac::StaticExprAttr::get(site->getContext(),
                                               cast<DictionaryAttr>(*tree)));
    }
    if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
      if (dictionary.getAs<DictionaryAttr>("site") &&
          dictionary.getAs<ArrayAttr>("expansion")) {
        NamedAttrList fields(dictionary);
        SmallVector<Attribute> frames(
            dictionary.getAs<ArrayAttr>("expansion").begin(),
            dictionary.getAs<ArrayAttr>("expansion").end());
        // A body may already carry helper-call frames. The loop iteration
        // encloses those calls and follows the loop's own expansion prefix.
        if (prefixLength > frames.size())
          return site->emitOpError() << "source expansion prefix disagrees";
        frames.insert(frames.begin() + prefixLength, frame);
        fields.set("expansion", ArrayAttr::get(site->getContext(), frames));
        return Attribute(fields.getDictionary(site->getContext()));
      }
      NamedAttrList fields;
      for (NamedAttribute field : dictionary) {
        auto value = expand(field.getValue(), frame, prefixLength, site, depth + 1);
        if (failed(value))
          return failure();
        fields.append(field.getName(), *value);
      }
      return Attribute(fields.getDictionary(site->getContext()));
    }
    if (auto array = dyn_cast<ArrayAttr>(attribute)) {
      SmallVector<Attribute> items;
      for (Attribute item : array) {
        auto value = expand(item, frame, prefixLength, site, depth + 1);
        if (failed(value))
          return failure();
        items.push_back(*value);
      }
      return Attribute(ArrayAttr::get(site->getContext(), items));
    }
    return attribute;
  }

  DictionaryAttr iteration(OpBuilder &builder, DictionaryAttr origin,
                           uint64_t ordinal, int64_t value) {
    auto integer = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
         builder.getNamedAttr("value",
                              ac::MathIntAttr::get(builder.getContext(),
                                                   llvm::APSInt::get(value)))});
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("iteration")),
         builder.getNamedAttr("site", origin.getAs<DictionaryAttr>("site")),
         builder.getNamedAttr("ordinal", builder.getI64IntegerAttr(ordinal)),
         builder.getNamedAttr("value", integer)});
  }

  FailureOr<Value> bitsConstant(OpBuilder &builder, Operation *site,
                                ac::BitsType type, const llvm::APInt &value,
                                bool signedValue, DictionaryAttr origin,
                                DictionaryAttr location) {
    ac::HardwareAnalysis hardware(package);
    ac::HardwareBindings bindings;
    bindings.owner = site->getParentOfType<ac::ModuleOp>();
    auto width = hardware.getPackedWidth(type, bindings, site);
    if (failed(width) || *width > std::numeric_limits<unsigned>::max())
      return failure();
    if (failed(charge(site, 2 * ((*width + 63) / 64))))
      return failure();
    unsigned needed = signedValue ? value.getSignificantBits()
                                  : std::max(1u, value.getActiveBits());
    if (needed > *width)
      return site->emitOpError() << "source induction value does not fit its "
                                    "declared representation";
    llvm::APSInt encoded(signedValue ? value.sextOrTrunc(*width)
                                     : value.zextOrTrunc(*width),
                         true);
    auto integer = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
         builder.getNamedAttr(
             "value", ac::MathIntAttr::get(builder.getContext(), encoded))});
    auto tree = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
         builder.getNamedAttr("value", integer),
         builder.getNamedAttr("origin", origin),
         builder.getNamedAttr("location", location)});
    OperationState state(site->getLoc(),
                         ac::BitsConstantOp::getOperationName());
    state.addTypes(type);
    state.addAttribute("value",
                       ac::StaticExprAttr::get(builder.getContext(), tree));
    return builder.create(state)->getResult(0);
  }

  // The staging region keeps yield uses alive while nested rewrites replace
  // their producers. Never retain an IRMapping result after deleting its op.
  FailureOr<SmallVector<Value>> cloneBlock(Block &source, ValueRange arguments,
                                           Operation *before,
                                           DictionaryAttr frame = {},
                                           unsigned prefixLength = 0,
                                           bool includeCondition = false) {
    if (source.getNumArguments() != arguments.size())
      return before->emitOpError() << "SCF block argument binding disagrees";
    if (source.empty())
      return before->emitOpError() << "source SCF body requires a terminator";
    auto yield = dyn_cast<scf::YieldOp>(source.getTerminator());
    auto condition = dyn_cast<scf::ConditionOp>(source.getTerminator());
    if (!yield && !condition)
      return before->emitOpError()
             << "source SCF body requires a structured terminator";
    SmallVector<Value> outputs;
    if (condition && includeCondition)
      outputs.push_back(condition.getCondition());
    llvm::append_range(outputs,
                       yield ? yield.getOperands() : condition.getArgs());
    OpBuilder builder(before);
    auto staged = scf::ExecuteRegionOp::create(builder, before->getLoc(),
                                               ValueRange(outputs).getTypes());
    auto *block = new Block;
    staged.getRegion().push_back(block);
    IRMapping mapping;
    for (auto [formal, actual] : llvm::zip(source.getArguments(), arguments))
      mapping.map(formal, actual);
    OpBuilder inside = OpBuilder::atBlockEnd(block);
    for (Operation &operation : source.without_terminator()) {
      uint64_t size = 0;
      operation.walk([&](Operation *) { ++size; });
      if (failed(charge(before, size)))
        return failure();
      Operation *copy = inside.clone(operation, mapping);
      if (frame) {
        auto result = copy->walk([&](Operation *nested) {
          auto attrs = expand(nested->getAttrDictionary(), frame, prefixLength, nested);
          if (failed(attrs))
            return WalkResult::interrupt();
          nested->setAttrs(cast<DictionaryAttr>(*attrs));
          return WalkResult::advance();
        });
        if (result.wasInterrupted())
          return failure();
      }
    }
    SmallVector<Value> results;
    for (Value output : outputs)
      results.push_back(mapping.lookupOrDefault(output));
    scf::YieldOp::create(inside, before->getLoc(), results);
    if (failed(normalizeBlock(*block)))
      return failure();
    return inlineRegion(staged);
  }

  FailureOr<SmallVector<Value>> inlineRegion(scf::ExecuteRegionOp operation) {
    auto &region = operation.getRegion();
    if (!region.hasOneBlock() || region.front().getNumArguments())
      return operation.emitOpError()
             << "source execute_region requires one argument-free block";
    auto &block = region.front();
    if (block.empty())
      return operation.emitOpError()
             << "source execute_region requires a terminator";
    auto yield = dyn_cast<scf::YieldOp>(block.getTerminator());
    if (!yield || yield.getOperandTypes() != operation.getResultTypes())
      return operation.emitOpError()
             << "source execute_region result types disagree";
    SmallVector<Value> values(yield.getOperands());
    for (Operation &nested :
         llvm::make_early_inc_range(block.without_terminator()))
      nested.moveBefore(operation);
    operation.replaceAllUsesWith(values);
    operation.erase();
    return values;
  }

  LogicalResult normalizeFor(scf::ForOp operation) {
    if (!operation.getRegion().hasOneBlock() || operation.getUnsignedCmp() ||
        !operation.getLowerBound().getType().isIndex())
      return operation.emitOpError()
             << "source for requires one block and signed index comparison";
    auto lower = constant(operation.getLowerBound());
    auto upper = constant(operation.getUpperBound());
    auto step = constant(operation.getStep());
    if (!lower || !upper || !step || lower->getBitWidth() > 64 ||
        upper->getBitWidth() > 64 || step->getBitWidth() > 64 ||
        step->isNegative() || step->isZero())
      return operation.emitOpError()
             << "source for requires exact finite bounds and a positive step";
    auto origin = loopOccurrence(operation);
    if (failed(origin))
      return failure();
    llvm::APInt lo = lower->sext(128), hi = upper->sext(128),
                increment = step->sext(128);
    llvm::APInt tripCount(128, 0);
    if (lo.slt(hi))
      tripCount = (hi - lo + increment - 1).udiv(increment);
    if (tripCount.getActiveBits() > 64 || tripCount.getZExtValue() > remaining)
      return operation.emitOpError() << "source for expansion limit exceeded";
    uint64_t trips = tripCount.getZExtValue();
    if (!(lo + increment * trips).isSignedIntN(64))
      return operation.emitOpError()
             << "source for induction can wrap before termination";
    uint64_t bodySize = 1;
    operation.getRegion().walk([&](Operation *) { ++bodySize; });
    if (trips > remaining / bodySize)
      return operation.emitOpError() << "source for expansion limit exceeded";
    SmallVector<Value> carried(operation.getInitArgs());
    for (uint64_t ordinal = 0; ordinal < trips; ++ordinal) {
      auto index = lo + increment * ordinal;
      if (!index.isSignedIntN(64))
        return operation.emitOpError()
               << "source for induction overflows index";
      OpBuilder builder(operation);
      auto induction = arith::ConstantIndexOp::create(
          builder, operation.getLoc(), index.getSExtValue());
      SmallVector<Value> arguments{induction};
      llvm::append_range(arguments, carried);
      auto frame = iteration(builder, *origin, ordinal, index.getSExtValue());
      auto results = cloneBlock(operation.getRegion().front(), arguments,
                                operation, frame,
                                (*origin).getAs<ArrayAttr>("expansion").size());
      if (failed(results))
        return failure();
      carried = std::move(*results);
    }
    if (TypeRange(ValueRange(carried)) != operation.getResultTypes())
      return operation.emitOpError()
             << "source for carried result types disagree";
    operation.replaceAllUsesWith(carried);
    operation.erase();
    return success();
  }

  LogicalResult normalizeIf(scf::IfOp operation) {
    for (Region &region : operation->getRegions()) {
      if (region.empty())
        continue;
      if (!region.hasOneBlock() || region.front().getNumArguments())
        return operation.emitOpError()
               << "source if requires single-block arms";
      if (failed(normalizeBlock(region.front())))
        return failure();
    }
    auto bridge =
        operation.getCondition().getDefiningOp<ac::SourcePredicateOp>();
    auto known = constant(operation.getCondition());
    auto callable = operation->getParentOfType<ac::SourceMethodOp>();
    auto transaction = operation->getParentOfType<ac::SourceTransactionOp>();
    if (!callable && !transaction) {
      if (!known)
        return operation.emitOpError()
               << "dynamic source SCF if requires a transaction body";
      Region &chosen = known->isZero() ? operation.getElseRegion()
                                       : operation.getThenRegion();
      if (chosen.empty()) {
        if (operation.getNumResults())
          return operation.emitOpError() << "valued source if requires else";
        operation.erase();
        return success();
      }
      auto values = cloneBlock(chosen.front(), {}, operation);
      if (failed(values))
        return failure();
      operation.replaceAllUsesWith(*values);
      operation.erase();
      return success();
    }
    for (Type type : operation.getResultTypes())
      if (!isa<ac::BitsType, ac::EnumType, ac::StructType, ac::TableType>(type))
        return operation.emitOpError()
               << "source if carries require finite hardware payload types";
    auto origin = loopOccurrence(operation);
    auto location =
        operation->getAttrOfType<DictionaryAttr>("ac.condition_location");
    if (failed(origin) || failed(ac::detail::verifySourceSpan(location, [&] {
          return operation.emitOpError();
        })))
      return failure();
    OpBuilder builder(operation);
    Value condition;
    if (bridge)
      condition = bridge.getInput();
    else if (known) {
      // Keep both source arms for closure/effect validation, even for a known
      // selector. Materialize the equivalent known four-state predicate.
      auto literal = [&](uint64_t value) {
        auto integer = builder.getDictionaryAttr(
            {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
             builder.getNamedAttr(
                 "value",
                 ac::MathIntAttr::get(builder.getContext(),
                                      llvm::APSInt::getUnsigned(value)))});
        return ac::StaticExprAttr::get(
            builder.getContext(),
            builder.getDictionaryAttr(
                {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
                 builder.getNamedAttr("value", integer),
                 builder.getNamedAttr("origin", *origin),
                 builder.getNamedAttr("location", location)}));
      };
      auto value =
          bitsConstant(builder, operation,
                       ac::BitsType::get(builder.getContext(), literal(1)),
                       *known, false, *origin, location);
      if (failed(value))
        return failure();
      condition = *value;
    } else
      return operation.emitOpError()
             << "source SCF if needs a proven-known condition or source "
                "predicate transport";
    OperationState state(operation.getLoc(),
                         ac::SourceBranchOp::getOperationName());
    state.addOperands(condition);
    state.addTypes(operation.getResultTypes());
    state.addAttribute("occurrence", *origin);
    state.addAttribute("ac.condition_location", location);
    state.addRegion();
    state.addRegion();
    auto branch = cast<ac::SourceBranchOp>(builder.create(state));
    for (auto [source, destination] :
         llvm::zip(operation->getRegions(), branch->getRegions())) {
      destination.takeBody(source);
      if (destination.empty())
        destination.push_back(new Block);
      auto &block = destination.front();
      OpBuilder inside = OpBuilder::atBlockEnd(&block);
      if (block.empty()) {
        ac::YieldOp::create(inside, operation.getLoc(), ValueRange{});
        continue;
      }
      auto yield = dyn_cast<scf::YieldOp>(block.getTerminator());
      if (!yield)
        return operation.emitOpError() << "source if requires scf.yield";
      inside.setInsertionPoint(yield);
      ac::YieldOp::create(inside, yield.getLoc(), yield.getOperands());
      yield.erase();
    }
    operation.replaceAllUsesWith(branch.getResults());
    operation.erase();
    return success();
  }

  LogicalResult normalizeWhile(scf::WhileOp operation);
  LogicalResult normalizeHardwareWhile(scf::WhileOp operation);

  std::optional<llvm::APInt> hardwareConstant(Value value) {
    auto constant = value.getDefiningOp<ac::BitsConstantOp>();
    if (!constant)
      return std::nullopt;
    // Optional knowledge must not emit a diagnostic for an unresolved generic
    // parameter or substitute a parameter's default as its actual value.
    auto tree = constant.getValue().getTree();
    auto kind = tree.getAs<StringAttr>("kind");
    auto literal = tree.getAs<DictionaryAttr>("value");
    auto integer =
        literal ? literal.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
    if (!kind || kind.getValue() != "literal" || !integer)
      return std::nullopt;
    ac::HardwareAnalysis hardware(package);
    ac::HardwareBindings bindings;
    bindings.owner = constant->getParentOfType<ac::ModuleOp>();
    auto width = hardware.getPackedWidth(value.getType(), bindings, constant);
    if (failed(width) || *width > std::numeric_limits<unsigned>::max())
      return std::nullopt;
    llvm::APSInt raw(integer.getCanonicalValue());
    if (raw.isNegative() || raw.getActiveBits() > *width)
      return std::nullopt;
    return raw.zextOrTrunc(*width);
  }

  bool unitUpdate(Value value, Value induction, bool ascending) {
    auto operation = value.getDefiningOp<ac::BitsBinaryOp>();
    if (!operation)
      return false;
    Value amount;
    if (ascending && operation.getOpcode() == "add") {
      if (operation.getLhs() == induction)
        amount = operation.getRhs();
      else if (operation.getRhs() == induction)
        amount = operation.getLhs();
    } else if (!ascending && operation.getOpcode() == "sub" &&
               operation.getLhs() == induction)
      amount = operation.getRhs();
    auto constant = amount ? hardwareConstant(amount) : std::nullopt;
    return constant && constant->isOne();
  }

  struct ExitFlag {
    unsigned slot;
    bool continueWhenOne;
  };

  bool progressOrExit(Value counter, Value induction, ArrayRef<ExitFlag> flags,
                      ArrayRef<Value> flagValues, bool ascending,
                      Operation *site, unsigned depth = 0) {
    if (depth > 128 || failed(charge(site)))
      return false;
    if (unitUpdate(counter, induction, ascending))
      return true;
    for (auto [flag, value] : llvm::zip(flags, flagValues))
      if (auto known = hardwareConstant(value);
          known && !known->isZero() != flag.continueWhenOne)
        return true;
    ac::SourceBranchOp selected = counter.getDefiningOp<ac::SourceBranchOp>();
    if (!selected)
      for (Value value : flagValues)
        if ((selected = value.getDefiningOp<ac::SourceBranchOp>()))
          break;
    if (!selected)
      return false;
    for (Region *region :
         {&selected.getThenRegion(), &selected.getElseRegion()}) {
      if (!region->hasOneBlock() || region->front().empty())
        return false;
      auto yield = dyn_cast<ac::YieldOp>(region->front().getTerminator());
      if (!yield || yield.getNumOperands() != selected.getNumResults())
        return false;
      auto substitute = [&](Value value) {
        if (value.getDefiningOp() != selected.getOperation())
          return value;
        return yield.getOperand(cast<OpResult>(value).getResultNumber());
      };
      SmallVector<Value> armFlags;
      for (Value value : flagValues)
        armFlags.push_back(substitute(value));
      if (!progressOrExit(substitute(counter), induction, flags, armFlags,
                          ascending, site, depth + 1))
        return false;
    }
    return true;
  }

  FailureOr<Value> knownPredicate(OpBuilder &builder, Operation *site,
                                  bool value, ac::BitsType type,
                                  DictionaryAttr origin, DictionaryAttr span) {
    return bitsConstant(builder, site, type, llvm::APInt(1, value), false,
                        origin, span);
  }

  ac::SourceBranchOp branch(OpBuilder &builder, Operation *site,
                            Value condition, TypeRange types,
                            DictionaryAttr origin, DictionaryAttr span,
                            bool continuation) {
    OperationState state(site->getLoc(),
                         ac::SourceBranchOp::getOperationName());
    state.addOperands(condition);
    state.addTypes(types);
    state.addAttribute("occurrence", origin);
    state.addAttribute("ac.condition_location", span);
    if (continuation)
      state.addAttribute("ac.loop_continuation", builder.getBoolAttr(true));
    state.addRegion();
    state.addRegion();
    return cast<ac::SourceBranchOp>(builder.create(state));
  }

  LogicalResult normalizeBlock(Block &block) {
    for (Operation &operation : llvm::make_early_inc_range(block)) {
      if (failed(charge(&operation)))
        return failure();
      if (auto loop = dyn_cast<scf::ForOp>(operation)) {
        if (failed(normalizeFor(loop)))
          return failure();
      } else if (auto loop = dyn_cast<scf::WhileOp>(operation)) {
        if (failed(normalizeWhile(loop)))
          return failure();
      } else if (auto branch = dyn_cast<scf::IfOp>(operation)) {
        if (failed(normalizeIf(branch)))
          return failure();
      } else if (auto execute = dyn_cast<scf::ExecuteRegionOp>(operation)) {
        if (!execute.getRegion().hasOneBlock() ||
            failed(normalizeBlock(execute.getRegion().front())) ||
            failed(inlineRegion(execute)))
          return failure();
      } else if (auto index = dyn_cast<ac::SourceIndexOp>(operation)) {
        auto value = constant(index.getInput());
        if (!value)
          return index.emitOpError()
                 << "source induction requires a proven exact index";
        if (!index.getIsSigned() && value->isNegative())
          return index.emitOpError()
                 << "negative induction needs signed Integer representation";
        OpBuilder builder(index);
        auto folded =
            bitsConstant(builder, index, index.getResult().getType(), *value,
                         index.getIsSigned(), index.getOccurrence(),
                         index.getSourceLocation());
        if (failed(folded))
          return failure();
        index.getResult().replaceAllUsesWith(*folded);
        index.erase();
      } else if (isa<scf::YieldOp, scf::ConditionOp>(operation)) {
        continue;
      } else if (operation.getName().getDialectNamespace() == "scf") {
        return operation.emitOpError()
               << "unsupported source SCF operation; parallel effects need an "
                  "independence proof";
      } else {
        for (Region &region : operation.getRegions())
          for (Block &nested : region)
            if (failed(normalizeBlock(nested)))
              return failure();
      }
    }
    return success();
  }

  LogicalResult run() {
    if (failed(normalizeBlock(package.getBodyRegion().front())))
      return failure();
    bool changed;
    do {
      changed = false;
      package.walk<WalkOrder::PostOrder>([&](Operation *operation) {
        bool temporary = isa<ac::SourcePredicateOp>(operation) ||
                         operation->getName().getDialectNamespace() == "arith";
        if (temporary && operation->use_empty() &&
            isMemoryEffectFree(operation)) {
          operation->erase();
          changed = true;
        }
      });
    } while (changed);
    auto residual = package.walk([&](Operation *operation) {
      if (isa<ac::SourceIndexOp, ac::SourcePredicateOp>(operation) ||
          operation->getName().getDialectNamespace() == "scf") {
        operation->emitOpError()
            << "unresolved source control flow cannot be published";
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    return failure(residual.wasInterrupted());
  }
};

LogicalResult Normalizer::normalizeWhile(scf::WhileOp operation) {
  if (!operation.getBefore().hasOneBlock() ||
      !operation.getAfter().hasOneBlock())
    return operation.emitOpError()
           << "source while requires single-block regions";
  auto &before = operation.getBefore().front();
  auto &after = operation.getAfter().front();
  if (before.empty() || after.empty())
    return operation.emitOpError()
           << "source while regions require terminators";
  auto condition = dyn_cast<scf::ConditionOp>(before.getTerminator());
  auto yield = dyn_cast<scf::YieldOp>(after.getTerminator());
  if (!condition || !yield ||
      operation.getOperandTypes() != operation.getResultTypes() ||
      before.getArgumentTypes() != operation.getOperandTypes() ||
      after.getArgumentTypes() != operation.getResultTypes() ||
      condition.getArgs().getTypes() != operation.getResultTypes() ||
      yield.getOperandTypes() != operation.getOperandTypes())
    return operation.emitOpError()
           << "source while typed carried bindings disagree";
  auto compare = condition.getCondition().getDefiningOp<arith::CmpIOp>();
  if (!compare)
    return normalizeHardwareWhile(operation);
  auto induction = dyn_cast<BlockArgument>(compare.getLhs());
  auto bound = constant(compare.getRhs());
  auto predicate = compare.getPredicate();
  if ((!induction || induction.getOwner() != &before) &&
      isa<BlockArgument>(compare.getRhs())) {
    induction = dyn_cast<BlockArgument>(compare.getRhs());
    bound = constant(compare.getLhs());
    switch (predicate) {
    case arith::CmpIPredicate::slt:
      predicate = arith::CmpIPredicate::sgt;
      break;
    case arith::CmpIPredicate::sle:
      predicate = arith::CmpIPredicate::sge;
      break;
    case arith::CmpIPredicate::sgt:
      predicate = arith::CmpIPredicate::slt;
      break;
    case arith::CmpIPredicate::sge:
      predicate = arith::CmpIPredicate::sle;
      break;
    default:
      break;
    }
  }
  if (!induction || induction.getOwner() != &before ||
      !induction.getType().isIndex() || !bound || bound->getBitWidth() > 64)
    return operation.emitOpError()
           << "source while needs an exact index bound and induction argument";
  unsigned slot = induction.getArgNumber();
  if (condition.getArgs()[slot] != induction)
    return operation.emitOpError()
           << "source while must forward its induction unchanged";
  auto start = constant(operation.getOperand(slot));
  if (!start || start->getBitWidth() > 64)
    return operation.emitOpError()
           << "source while needs an exact initial induction";
  Value next = yield.getOperand(slot);
  std::optional<llvm::APInt> stride;
  Value afterInduction = after.getArgument(slot);
  if (auto add = next.getDefiningOp<arith::AddIOp>()) {
    if (add.getLhs() == afterInduction)
      stride = constant(add.getRhs());
    else if (add.getRhs() == afterInduction)
      stride = constant(add.getLhs());
  } else if (auto sub = next.getDefiningOp<arith::SubIOp>()) {
    if (sub.getLhs() == afterInduction) {
      stride = constant(sub.getRhs());
      if (stride) {
        if (stride->isMinSignedValue())
          return operation.emitOpError()
                 << "source while induction subtraction can wrap";
        *stride = -*stride;
      }
    }
  }
  if (!stride || stride->getBitWidth() > 64 || stride->isZero())
    return operation.emitOpError()
           << "source while needs a nonzero constant induction step";
  llvm::APInt initial = start->sext(128), limit = bound->sext(128),
              step = stride->sext(128), distance(128, 0);
  bool ascending = predicate == arith::CmpIPredicate::slt ||
                   predicate == arith::CmpIPredicate::sle;
  bool descending = predicate == arith::CmpIPredicate::sgt ||
                    predicate == arith::CmpIPredicate::sge;
  if ((!ascending && !descending) || (ascending && step.isNegative()) ||
      (descending && !step.isNegative()))
    return operation.emitOpError()
           << "source while comparison and step do not prove termination";
  if (ascending)
    distance = limit - initial;
  else
    distance = initial - limit;
  if (predicate == arith::CmpIPredicate::sle ||
      predicate == arith::CmpIPredicate::sge)
    distance += 1;
  llvm::APInt magnitude = descending ? -step : step;
  llvm::APInt count(128, 0);
  if (!distance.isNegative() && !distance.isZero())
    count = (distance + magnitude - 1).udiv(magnitude);
  if (count.getActiveBits() > 64 || count.getZExtValue() >= remaining)
    return operation.emitOpError() << "source while expansion limit exceeded";
  uint64_t trips = count.getZExtValue();
  if (!(initial + step * trips).isSignedIntN(64))
    return operation.emitOpError()
           << "source while induction can wrap before termination";
  auto origin = loopOccurrence(operation);
  if (failed(origin))
    return failure();
  // Condition code may inspect state, but does not open another effect scope.
  for (Operation &nested : before.without_terminator()) {
    auto name = nested.getName().getDialectNamespace();
    if (name != "arith" &&
        !isa<ac::SourceStateReadOp, ac::SourceIndexOp, ac::SourcePredicateOp>(
            nested) &&
        !ac::detail::isCombinationalHardwareOperation(&nested))
      return nested.emitOpError()
             << "source while condition must be pure apart from state reads";
  }
  SmallVector<Value> carried(operation.getOperands());
  for (uint64_t ordinal = 0; ordinal <= trips; ++ordinal) {
    OpBuilder builder(operation);
    int64_t value = (initial + step * ordinal).getSExtValue();
    auto frame = iteration(builder, *origin, ordinal, value);
    auto visible = cloneBlock(before, carried, operation, frame,
                              (*origin).getAs<ArrayAttr>("expansion").size());
    if (failed(visible))
      return failure();
    if (ordinal == trips) {
      operation.replaceAllUsesWith(*visible);
      operation.erase();
      return success();
    }
    auto updated = cloneBlock(after, *visible, operation, frame,
                              (*origin).getAs<ArrayAttr>("expansion").size());
    if (failed(updated))
      return failure();
    carried = std::move(*updated);
    // The checked affine update is exact. Re-materializing its proven index
    // avoids a recursively growing constant-expression chain between stages.
    carried[slot] = arith::ConstantIndexOp::create(
        builder, operation.getLoc(),
        (initial + step * (ordinal + 1)).getSExtValue());
  }
  llvm_unreachable("finite source while must reach its proven exit");
}

// Runtime continuation stays four-state. The proof is over actual comparison
// and update SSA, not a trusted bound annotation or the mere finiteness of a
// carrier. A strict comparison plus a unit step cannot wrap on a taken path.
LogicalResult Normalizer::normalizeHardwareWhile(scf::WhileOp operation) {
  auto &before = operation.getBefore().front();
  auto &after = operation.getAfter().front();
  auto condition = cast<scf::ConditionOp>(before.getTerminator());
  auto yielded = cast<scf::YieldOp>(after.getTerminator());
  auto transport =
      condition.getCondition().getDefiningOp<ac::SourcePredicateOp>();
  if (!transport || (!operation->getParentOfType<ac::SourceTransactionOp>() &&
                     !operation->getParentOfType<ac::SourceMethodOp>()))
    return operation.emitOpError()
           << "runtime source while requires four-state continuation in a "
              "transaction";
  for (Type type : operation.getResultTypes())
    if (!isa<ac::BitsType, ac::EnumType, ac::StructType, ac::TableType>(type))
      return operation.emitOpError()
             << "runtime source while carries require finite hardware payloads";
  for (auto [argument, forwarded] :
       llvm::zip(before.getArguments(), condition.getArgs()))
    if (argument != forwarded)
      return operation.emitOpError()
             << "runtime source while currently requires unchanged condition "
                "forwarding";
  // An AND conjunct is necessary whenever the complete control is true. An OR
  // operand is not such a termination witness and is deliberately not followed.
  SmallVector<Value> pending{transport.getInput()}, necessary;
  while (!pending.empty()) {
    Value predicate = pending.pop_back_val();
    if (failed(charge(operation)))
      return failure();
    if (auto conjunction = predicate.getDefiningOp<ac::BitsBinaryOp>();
        conjunction && conjunction.getOpcode() == "and") {
      pending.push_back(conjunction.getLhs());
      pending.push_back(conjunction.getRhs());
    } else
      necessary.push_back(predicate);
  }
  SmallVector<ExitFlag> exits;
  SmallVector<Value> exitValues;
  for (Value predicate : necessary) {
    bool continueWhenOne = true;
    if (auto inverse = predicate.getDefiningOp<ac::BitsUnaryOp>();
        inverse && inverse.getOpcode() == "not") {
      continueWhenOne = false;
      predicate = inverse.getInput();
    }
    auto argument = dyn_cast<BlockArgument>(predicate);
    if (!argument || argument.getOwner() != &before ||
        !isa<ac::BitsType>(argument.getType()))
      continue;
    ac::HardwareAnalysis analysis(package);
    ac::HardwareBindings owner;
    owner.owner = operation->getParentOfType<ac::ModuleOp>();
    auto width = analysis.getPackedWidth(argument.getType(), owner, operation);
    if (failed(width))
      return failure();
    if (*width != 1)
      continue;
    exits.push_back({argument.getArgNumber(), continueWhenOne});
    exitValues.push_back(yielded.getOperand(argument.getArgNumber()));
  }
  ac::BitsCompareOp comparison;
  BlockArgument induction;
  Value bound;
  bool ascending = false, signedCompare = false;
  for (Value predicate : necessary) {
    auto candidate = predicate.getDefiningOp<ac::BitsCompareOp>();
    if (!candidate)
      continue;
    StringRef kind = candidate.getPredicate();
    bool up = kind == "ult" || kind == "slt";
    bool down = kind == "ugt" || kind == "sgt";
    if (!up && !down)
      continue;
    auto argument = dyn_cast<BlockArgument>(candidate.getLhs());
    Value limit = candidate.getRhs();
    if (!argument || argument.getOwner() != &before) {
      argument = dyn_cast<BlockArgument>(candidate.getRhs());
      limit = candidate.getLhs();
      std::swap(up, down);
    }
    if (!argument || argument.getOwner() != &before ||
        !isa<ac::BitsType>(argument.getType()))
      continue;
    unsigned slot = argument.getArgNumber();
    if (condition.getArgs()[slot] != argument ||
        !progressOrExit(yielded.getOperand(slot), after.getArgument(slot),
                        exits, exitValues, up, operation))
      continue;
    comparison = candidate;
    induction = argument;
    bound = limit;
    ascending = up;
    signedCompare = kind.starts_with("s");
    break;
  }
  if (!comparison)
    return operation.emitOpError()
           << "runtime source while needs a strict finite counter comparison "
              "and unconditional unit progress";
  ac::HardwareAnalysis hardware(package);
  ac::HardwareBindings bindings;
  bindings.owner = operation->getParentOfType<ac::ModuleOp>();
  auto width =
      hardware.getPackedWidth(induction.getType(), bindings, operation);
  if (failed(width) || *width >= std::numeric_limits<unsigned>::max())
    return failure();
  // Pay the finite carrier arithmetic before allocating its APInt buffers.
  // This uses the same expansion work ceiling as cloning and does not create
  // a separate user-authored budget or termination permission.
  if (failed(charge(operation, 16 * ((*width + 64) / 64))))
    return failure();
  unsigned bits = *width;
  llvm::APInt minimum =
      signedCompare ? llvm::APInt::getSignedMinValue(bits).sext(bits + 1)
                    : llvm::APInt(bits + 1, 0);
  llvm::APInt maximum =
      signedCompare ? llvm::APInt::getSignedMaxValue(bits).sext(bits + 1)
                    : llvm::APInt::getMaxValue(bits).zext(bits + 1);
  auto extend = [&](const llvm::APInt &value) {
    return signedCompare ? value.sext(bits + 1) : value.zext(bits + 1);
  };
  llvm::APInt initial = ascending ? minimum : maximum;
  if (auto exact =
          hardwareConstant(operation.getOperand(induction.getArgNumber())))
    initial = extend(*exact);
  llvm::APInt endpoint = ascending ? maximum : minimum;
  if (auto exact = hardwareConstant(bound))
    endpoint = extend(*exact);
  llvm::APInt distance = ascending ? endpoint - initial : initial - endpoint;
  if (distance.isNegative())
    distance = llvm::APInt(bits + 1, 0);
  if (distance.getActiveBits() > 64)
    return operation.emitOpError()
           << "runtime source while expansion limit exceeded";
  uint64_t count = distance.getZExtValue();
  uint64_t size = 1;
  operation->walk([&](Operation *) { ++size; });
  if (count >= remaining / size)
    return operation.emitOpError()
           << "runtime source while expansion limit exceeded";
  for (Operation &nested : before.without_terminator()) {
    if (!isa<ac::SourcePredicateOp, ac::SourceIndexOp, ac::SourceStateReadOp>(
            nested) &&
        nested.getName().getDialectNamespace() != "arith" &&
        !ac::detail::isCombinationalHardwareOperation(&nested))
      return nested.emitOpError()
             << "source while condition must be pure apart from state reads";
  }
  auto origin = loopOccurrence(operation);
  auto span = operation->getAttrOfType<DictionaryAttr>("ac.condition_location");
  if (failed(origin) || failed(ac::detail::verifySourceSpan(
                            span, [&] { return operation.emitOpError(); })))
    return failure();
  OpBuilder builder(operation);
  auto predicateType = transport.getInput().getType();
  auto one =
      knownPredicate(builder, operation, true, predicateType, *origin, span);
  if (failed(one))
    return failure();
  Value active = *one;
  SmallVector<Value> carried(operation.getOperands());
  for (uint64_t ordinal = 0; ordinal <= count; ++ordinal) {
    auto frame = iteration(builder, *origin, ordinal, ordinal);
    auto expanded = expand(
        *origin, frame, (*origin).getAs<ArrayAttr>("expansion").size(), operation);
    if (failed(expanded))
      return failure();
    auto identity = cast<DictionaryAttr>(*expanded);
    auto values = cloneBlock(before, carried, operation, frame,
                             (*origin).getAs<ArrayAttr>("expansion").size(), true);
    if (failed(values))
      return failure();
    auto predicate = (*values)[0].getDefiningOp<ac::SourcePredicateOp>();
    if (!predicate)
      return operation.emitOpError()
             << "runtime while continuation lost its four-state transport";
    OperationState conjunction(operation.getLoc(),
                               ac::BitsBinaryOp::getOperationName());
    conjunction.addOperands({active, predicate.getInput()});
    conjunction.addTypes(predicateType);
    conjunction.addAttribute("opcode", builder.getStringAttr("and"));
    active = builder.create(conjunction)->getResult(0);
    SmallVector<Value> visible(values->begin() + 1, values->end());
    bool exitCheck = ordinal == count;
    auto selected =
        branch(builder, operation, active,
               exitCheck ? TypeRange{} : TypeRange(operation.getResultTypes()),
               identity, span, true);
    for (Region &region : selected->getRegions())
      region.push_back(new Block);
    auto &thenBlock = selected.getThenRegion().front();
    auto &elseBlock = selected.getElseRegion().front();
    OpBuilder thenBuilder = OpBuilder::atBlockEnd(&thenBlock);
    OpBuilder elseBuilder = OpBuilder::atBlockEnd(&elseBlock);
    if (exitCheck) {
      // The proof rules out another known-true iteration, but the original
      // reached condition still owes its X/Z check after the last body.
      ac::YieldOp::create(thenBuilder, operation.getLoc(), ValueRange{});
      ac::YieldOp::create(elseBuilder, operation.getLoc(), ValueRange{});
      operation.replaceAllUsesWith(visible);
      operation.erase();
      return success();
    }
    auto anchor =
        scf::YieldOp::create(thenBuilder, operation.getLoc(), ValueRange{});
    auto updated = cloneBlock(after, visible, anchor, frame,
                              (*origin).getAs<ArrayAttr>("expansion").size());
    if (failed(updated))
      return failure();
    thenBuilder.setInsertionPoint(anchor);
    ac::YieldOp::create(thenBuilder, operation.getLoc(), *updated);
    anchor.erase();
    ac::YieldOp::create(elseBuilder, operation.getLoc(), visible);
    carried.assign(selected.getResults().begin(), selected.getResults().end());
  }
  llvm_unreachable("finite runtime while must reach its checked exit");
}

class NormalizeSourceControlFlowPass
    : public PassWrapper<NormalizeSourceControlFlowPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(NormalizeSourceControlFlowPass)
  StringRef getArgument() const final {
    return "ac-normalize-source-control-flow";
  }
  StringRef getDescription() const final {
    return "Normalize finite typed source SCF before transaction analysis";
  }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ac::ACIRDialect, arith::ArithDialect, scf::SCFDialect>();
  }
  void runOnOperation() override {
    if (failed(normalizeSourceControlFlow(getOperation())))
      signalPassFailure();
  }
};
} // namespace

LogicalResult normalizeSourceControlFlow(mlir::ModuleOp package) {
  bool transient = false;
  package.walk([&](Operation *operation) {
    transient |= isa<ac::SourceIndexOp, ac::SourcePredicateOp>(operation) ||
                 operation->getName().getDialectNamespace() == "scf";
  });
  if (!transient)
    return success();
  auto legal = package.walk([&](Operation *operation) {
    bool sourceControl =
        isa<ac::SourceIndexOp, ac::SourcePredicateOp>(operation) ||
        operation->getName().getDialectNamespace() == "scf";
    if (sourceControl &&
        (failed(operation->getName().verifyInvariants(operation)) ||
         failed(operation->getName().verifyRegionInvariants(operation))))
      return WalkResult::interrupt();
    return WalkResult::advance();
  });
  if (legal.wasInterrupted())
    return failure();
  uint64_t operations = 0;
  auto scan = package.walk([&](Operation *operation) {
    if (++operations > ac::HardwareSourceCheckLimits{}.maxWorkUnits) {
      operation->emitOpError() << "source control-flow input limit exceeded";
      return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });
  if (scan.wasInterrupted())
    return failure();
  OwningOpRef<mlir::ModuleOp> candidate(cast<mlir::ModuleOp>(package->clone()));
  Normalizer normalizer{*candidate};
  if (failed(normalizer.run()) || failed(verify(*candidate)))
    return failure();
  package->setAttrs((*candidate)->getAttrs());
  package->setLoc(candidate->getLoc());
  package.getBodyRegion().takeBody(candidate->getBodyRegion());
  return success();
}

std::unique_ptr<Pass> createNormalizeSourceControlFlowPass() {
  return std::make_unique<NormalizeSourceControlFlowPass>();
}
} // namespace acir
