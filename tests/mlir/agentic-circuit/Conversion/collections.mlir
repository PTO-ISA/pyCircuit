// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.out
// RUN: %acir_opt_public %t.out -o /dev/null

// Placement collections: a one-dimensional and a two-dimensional array of a
// structural module. Arrays expand lexicographic row-major in the
// construction order and lower to acsim.array with the exact dense shape.
// Array elements of structural modules are ownership-only, so the workload
// process remains the sole dispatch row.

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Cell() parameters {} graph {
    ac.return
  }
  ac.module @Top() parameters {} graph {
    ac.array @cells of @Cell shape [2]() static [{}, {}]
        id "cells" path "cells" : () -> ()
    ac.array @grid of @Cell shape [2, 2]() static [{}, {}, {}, {}]
        id "grid" path "grid" : () -> ()
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}

// CHECK:      acsim.model @soc epoch "0.5" root @Top
// CHECK-SAME:   construction ["root.cells[0]", "root.cells[1]", "root.grid[0][0]", "root.grid[0][1]", "root.grid[1][0]", "root.grid[1][1]", "root.workload"]
// CHECK-SAME:   destruction ["root.workload", "root.grid[1][1]", "root.grid[1][0]", "root.grid[0][1]", "root.grid[0][0]", "root.cells[1]", "root.cells[0]"]
// CHECK:        acsim.module @Cell interface {ports = [], resources = [], results = []} static [] specialization "[[CELL_FP:sha256:[0-9a-f]+]]" exports [] {
// CHECK:        acsim.module @Top
// CHECK-NEXT:     %{{.+}} = acsim.array @cells target @Cell args [] specialization "[[CELL_FP]]" shape [2] : !acsim.array<[2], !acsim.owner<@Cell>>
// CHECK-NEXT:     %{{.+}} = acsim.array @grid target @Cell args [] specialization "[[CELL_FP]]" shape [2, 2] : !acsim.array<[2, 2], !acsim.owner<@Cell>>
// CHECK:          acsim.process @workload
// CHECK:          acsim.return
// CHECK-NEXT:   }
// CHECK-NEXT:   %{{.+}}, %{{.+}} = acsim.dispatch @Top::@workload path "root.workload" indices [] object 0 activation 0
// CHECK-NOT:    acsim.dispatch
// CHECK:        acsim.activate
// CHECK-NOT:    acsim.activate
