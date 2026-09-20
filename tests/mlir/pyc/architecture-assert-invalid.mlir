// RUN: %not %pyc_opt -split-input-file %s 2>&1 | %FileCheck %s

module {
  func.func @missing_metadata(%condition: i1) {
    // expected-error @+1 {{architecture assertion requires complete ID, kind, severity, sampling, anchor, source, and message metadata}}
    pyc.assert %condition {obligation_id = "range:bounded"}
    return
  }
}

// CHECK: architecture assertion requires complete ID, kind, severity, sampling, anchor, source, and message metadata

// -----

module {
  func.func @bad_severity(%condition: i1) {
    // expected-error @+1 {{architecture assertion severity must be error or fatal}}
    pyc.assert %condition {msg = "bounded", obligation_id = "range:bounded", obligation_kind = "range", severity = "warning", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = []}
    return
  }
}

// CHECK: architecture assertion severity must be error or fatal

// -----

module {
  func.func @bad_sampling(%condition: i1) {
    // expected-error @+1 {{phase-one architecture assertions require pre_publish sampling with the inherited firing clock}}
    pyc.assert %condition {msg = "bounded", obligation_id = "range:bounded", obligation_kind = "range", severity = "error", sampling_kind = "tick_observation", sampling_edge = "none", sample_anchor = "bounded", source = "fixture.py:7:3", ndf_ids = []}
    return
  }
}

// CHECK: phase-one architecture assertions require pre_publish sampling with the inherited firing clock

// -----

module {
  func.func @liveness_rejected(%condition: i1) {
    // expected-error @+1 {{architecture assertion kind must be a closed phase-one safety kind}}
    pyc.assert %condition {msg = "eventually", obligation_id = "eventually:bad", obligation_kind = "liveness", severity = "error", sampling_kind = "pre_publish", sampling_edge = "none", sample_anchor = "bad", source = "fixture.py:7:3", ndf_ids = []}
    return
  }
}

// CHECK: architecture assertion kind must be a closed phase-one safety kind
