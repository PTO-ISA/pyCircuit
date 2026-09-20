// RUN: rm -rf %t.cpp %t.bundle %t.v %t.invalid %t.missing.cpp %t.missing.v %t.fake %t.failed.v
// RUN: %acir_opt --ac-freeze-topology %s -o %t.ac
// RUN: %acc -c %t.ac -emit-cpp -o %t.cpp
// RUN: %FileCheck %s --check-prefix=CPP < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.cpp
// RUN: %acc -c %t.ac -emit-cpp-bundle -o %t.bundle
// RUN: %FileCheck %s --check-prefix=BUNDLE < %t.bundle/src/generated/queuegraph.cpp
// RUN: test -s %t.bundle/include/generated/model.h
// RUN: %acc -c %t.ac -emit-verilog -o %t.v
// RUN: %FileCheck %s --check-prefix=VERILOG < %t.v
// RUN: mkdir -p %t.fake
// RUN: cp %acc %t.fake/acc
// RUN: printf '#!/bin/sh\nprintf partial > "$3"\nexit 1\n' > %t.fake/pycc
// RUN: chmod +x %t.fake/pycc
// RUN: %not %t.fake/acc -c %t.ac -emit-verilog -o %t.failed.v 2>&1 | %FileCheck %s --check-prefix=PYCC-ERROR
// RUN: test ! -e %t.failed.v
// RUN: %not %acc -c %t.ac -emit-cpp -emit-cpp-bundle -o %t.invalid 2>&1 | %FileCheck %s --check-prefix=MODE-ERROR
// RUN: test ! -e %t.invalid
// RUN: %not %acc -c %t.missing.ac -emit-cpp -o %t.missing.cpp 2>&1 | %FileCheck %s --check-prefix=PARSE-ERROR
// RUN: test ! -e %t.missing.cpp
// RUN: %not %acc -c %t.missing.ac -emit-verilog -o %t.missing.v 2>&1 | %FileCheck %s --check-prefix=PARSE-ERROR
// RUN: test ! -e %t.missing.v
// RUN: %acc -c %s -verify
// RUN: %not %acc -c %t.ac -emit-cpp -o %t.cpp 2>&1 | %FileCheck %s --check-prefix=EXISTS-ERROR
// RUN: %FileCheck %s --check-prefix=CPP < %t.cpp

module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle",
  ac.system = "acc_driver"
} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"}
      : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^transform(%item: !ac.var<i8>):
    %one = ac.var.constant 1 : i8 as !ac.var<i8>
    %next = ac.var.add %item, %one : !ac.var<i8>
    ac.transform.yield %next : !ac.var<i8>
  } {ac.output_names = ["output"]} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

// CPP: class AccDriver final : public gfsim::Module
// BUNDLE: class AccDriver final : public gfsim::Module
// VERILOG: module acc_driver (
// VERILOG: endmodule
// PYCC-ERROR: ACLOWER-QUEUE-CXX: acc: pycc Verilog emission failed
// MODE-ERROR: ACLOWER-QUEUE-CXX: acc: exactly one emit mode is required
// PARSE-ERROR: ACLOWER-QUEUE-CXX: acc: AC unit parsing failed
// EXISTS-ERROR: ACLOWER-QUEUE-CXX: acc: output already exists; refusing a partial replacement
