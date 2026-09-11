// RUN: %not %pycc %s --emit=none --probe-manifest=%t.json 2>&1 | %FileCheck %s

// CHECK: [PYC926] cannot emit probe manifest: missing `pyc.top`

module attributes {pyc.frontend.contract = "pycircuit"} {
}
