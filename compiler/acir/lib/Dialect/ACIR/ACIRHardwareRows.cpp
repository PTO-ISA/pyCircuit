#include "ACIRHardwareClosureDetail.h"

#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac::hardware_detail {

LogicalResult verifyInstanceRows(Closure &closure,
                                 ac::detail::EmitError emitError) {
  SmallVector<View *> ordered(closure.views.begin(), closure.views.end());
  llvm::sort(ordered, [](View *left, View *right) {
    return detail::compareClosedSourceStructure(left->owner, right->owner) < 0;
  });
  if (ordered.size() != closure.rows.size())
    return emitError()
           << "ac.instance_bindings does not cover the instance tree";
  Builder builder(closure.package.getContext());
  Attribute previous;
  for (auto [rowIndex, view] : llvm::enumerate(ordered)) {
    auto raw = dyn_cast<DictionaryAttr>(closure.rows[rowIndex]);
    auto owner = raw ? raw.getAs<DictionaryAttr>("owner") : DictionaryAttr();
    auto key = raw ? raw.getAs<DictionaryAttr>("key") : DictionaryAttr();
    auto ports = raw ? raw.getAs<ArrayAttr>("ports") : ArrayAttr();
    if (!raw || raw.size() != 3 ||
        failed(detail::verifyOwnerRef(owner, emitError)) ||
        failed(detail::verifySpecKey(key, emitError)) || !ports ||
        owner != view->owner || key != view->key ||
        ports.size() != view->portStates.size() ||
        (previous &&
         detail::compareClosedSourceStructure(previous, owner) >= 0))
      return emitError() << "final InstanceBinding row is stale or unordered";
    previous = owner;
    for (auto [portIndex, rawPort] : llvm::enumerate(ports)) {
      auto binding = dyn_cast<DictionaryAttr>(rawPort);
      auto ordinal =
          binding ? binding.getAs<IntegerAttr>("port") : IntegerAttr();
      auto decoded =
          detail::decodeU32(ordinal, "InstanceBinding port", emitError);
      auto state =
          binding ? binding.getAs<DictionaryAttr>("state") : DictionaryAttr();
      if (!binding || binding.size() != 2 || failed(decoded) ||
          *decoded != portIndex ||
          failed(detail::verifyStateID(state, emitError)) ||
          state != view->portStates[portIndex])
        return emitError()
               << "final InstanceBinding port differs from reconstructed alias";
    }
  }
  return success();
}

} // namespace acir::ac::hardware_detail
