#ifndef PYCIRCUIT_INTERFACES_MODULECHILDINTERFACES_TD
#define PYCIRCUIT_INTERFACES_MODULECHILDINTERFACES_TD

include "mlir/IR/OpBase.td"

// Declared-category contracts for module-graph membership, shared by the
// common staged analyses without concrete ACIR op classes (B2 step-2
// design r9). Implementers are exactly the ACIR operations named below;
// logical states participate in the same collision scans through the
// separate LogicalStateOpInterface, and source ac.state implements
// neither (the existing asymmetry is intended). No method performs
// lookup, caching or admission; these are access contracts only.
def ModuleChildOpInterface : OpInterface<"ModuleChildOpInterface"> {
  let description = [{
    A declared child allocation of a module graph (instance or
    collection), identified by category rather than by operation name.
  }];
  let methods = [
    InterfaceMethod<"Declared instance name attribute.",
                    "mlir::StringAttr", "getDeclaredInstanceName">,
    InterfaceMethod<"Declared occurrence attribute.",
                    "mlir::DictionaryAttr", "getDeclaredOccurrence">,
  ];
}

def OwningModuleOpInterface : OpInterface<"OwningModuleOpInterface"> {
  let description = [{
    The owning module declaration of a module graph. Sole implementer
    today: ac.ModuleOp.
  }];
  let methods = [
    InterfaceMethod<"Owning symbol name attribute.",
                    "mlir::StringAttr", "getOwnerSymName">,
  ];
}

#endif // PYCIRCUIT_INTERFACES_MODULECHILDINTERFACES_TD
