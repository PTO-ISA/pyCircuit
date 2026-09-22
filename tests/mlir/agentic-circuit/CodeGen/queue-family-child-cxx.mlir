// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/family_child.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %FileCheck %s --check-prefix=CHILD < %t/bundle/include/generated/modules/bank.hpp
// RUN: %FileCheck %s --check-prefix=CASE < %t/bundle/include/generated/modules/stage.hpp
// RUN: %FileCheck %s --check-prefix=ROOT < %t/bundle/include/generated/dut.h
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include -fsyntax-only %t/bundle/src/generated/modules/stage.cpp

// Issue #223 acceptance criterion 8 -- family body with a child instance.
//
// A `@ac.module_decl(parameters=(banks,), finite_cases=(2, 4))` family whose
// body instantiates a single-case child module used to fail while specializing
// the child: the child template had already been re-registered from the
// implementation's empty keyword-only signature, so the family's own
// `static=case(("banks", 4))` selection was reported as
// "ACPY-MODULE-007: unknown module static argument 'banks'".
//
// The family template now keeps the declaration's static fields for every body
// shape, so a family body may be a composite of child instances. Each concrete
// case is emitted, and the child keeps its bare readable class name (`Bank`)
// because it is not itself a parameter family.

// EMIT: emitted model bundle v1

// CHILD: class Bank final : public gfsim::Module {

// CASE: class StageBanks2 final : public gfsim::Module {
// CASE: std::unique_ptr<Bank> child_0_;
// CASE: class StageBanks4 final : public gfsim::Module {
// CASE: std::unique_ptr<Bank> child_0_;

// ROOT: class Composite final : public gfsim::Module {
// ROOT: std::make_unique<StageBanks4>("stage_0", 2, this, &input_0_, &stage_0_)

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="generated/module.py")
)

//--- family_child.py
import agentic_circuit as ac


@ac.struct
class Packet:
    value: ac.bits[8]
    valid: ac.bits[1]


@ac.module_decl(source="generated/module.py")
def bank(packet: ac.Queue[Packet, 1, 1]) -> ac.Queue[Packet, 1, 1]:
    ...


bank_decl = bank


@ac.module(declaration=bank_decl)
def bank(packet: ac.Queue[Packet, 1, 1]) -> ac.Queue[Packet, 1, 1]:
    return packet


@ac.module_decl(
    source="generated/module.py",
    parameters=(ac.static_parameter("banks", ac.static_int(width=4, signed=False)),),
    finite_cases=(ac.case(("banks", 2)), ac.case(("banks", 4))),
)
def stage(packet: ac.Queue[Packet, 1, 1]) -> ac.Queue[Packet, 1, 1]:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(packet: ac.Queue[Packet, 1, 1]) -> ac.Queue[Packet, 1, 1]:
    result = bank(packet)
    return result


@ac.system
def composite(packet: ac.Queue[Packet, 1, 1]) -> ac.Queue[Packet, 1, 1]:
    return stage(packet, static=ac.case(("banks", 4)))
