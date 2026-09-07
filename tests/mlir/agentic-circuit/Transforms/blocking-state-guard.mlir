// RUN: %python -c "from pathlib import Path; from agentic_circuit._queue_frontend import lower_queue_source; print(lower_queue_source(Path(r'%S/Inputs/blocking_state_guard.py').read_text(), 'blocking_state_guard'))" > %t.raw.mlir
// RUN: %FileCheck %s --check-prefix=RAW < %t.raw.mlir
// RUN: %python %source_root/compiler/acir/tools/ac-queue-cxxgen.py %S/Inputs/blocking_state_guard.py --system blocking_state_guard --acir-opt %acir_opt --queue-plan-tool %acir_queue_plan --queue-cxxgen-tool %acir_queue_cxxgen --acir-output %t.frozen.mlir --plan-output %t.plan.json -o %t.cpp
// RUN: %FileCheck %s --check-prefix=FROZEN < %t.frozen.mlir
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -DMODEL_FILE='"%t.cpp"' %S/Inputs/blocking_state_guard.cpp -o %t.exe
// RUN: %t.exe

// The guard reads the SSA value at `if`, while the mirror sees the subsequent
// scalar assignment. Check both before and after storage/schedule lowering.
// RAW: %[[COUNT:.*]] = ac.var.read @count
// RAW: %[[LIMIT:.*]] = ac.var.constant 4
// RAW: %[[GUARD:.*]] = ac.var.cmp "ne" %[[COUNT]], %[[LIMIT]]
// RAW: %[[NEXT:.*]] = ac.var.add %[[COUNT]],
// RAW: ac.rule.condition %[[GUARD]]
// RAW: ac.var.assign @count = %[[NEXT]]
// RAW: ac.var.assign @mirror = %[[NEXT]]
// FROZEN: %[[COUNT:.*]] = ac.table.get @count[
// FROZEN: %[[GUARD:.*]] = ac.var.cmp "ne" %[[COUNT]],
// FROZEN: ac.firing.condition %[[GUARD]]
