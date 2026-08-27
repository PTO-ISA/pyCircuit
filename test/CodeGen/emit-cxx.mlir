// RUN: rm -rf %t.out %t.out2 %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC
// RUN: %FileCheck %s --input-file=%t.out/src/generated/main.cpp --check-prefix=MAIN
// RUN: %FileCheck %s --input-file=%t.out/build-manifest.json --check-prefix=MAN
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out2 %t.frozen -o /dev/null
// RUN: diff %t.out/src/generated/model.cpp %t.out2/src/generated/model.cpp
// RUN: diff %t.out/include/generated/model.h %t.out2/include/generated/model.h

// ACSim → C++ emission for a yield-only workload: generated owner/process
// classes, exact dispatch thunks, compressed activation adjacency, and a
// schema-shaped build manifest. Regeneration is byte-identical.

builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 7 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Top() parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}

// HDR: struct Process {
// HDR:   enum class Pc : std::uint8_t {entry};
// HDR: gfsim::TerminationResult simulate();
// HDR: constexpr char kBuildFingerprint[] =

// SRC: acir::generated::wake_next_delta impl_wake_next_delta_
// SRC: return acir::generated::wake_next_delta{epoch.nextDelta()};
// SRC: void {{.*}}Process::work(gfsim::Epoch epoch)
// SRC: switch (running)
// SRC: case Pc::entry:
// SRC: proposedWake_ =
// SRC: suspended_ = true;
// SRC: commitQueues(
// SRC: system->scheduleWork(id, proposedWake_);
// SRC: dispatch[0] =
// SRC: system.setLegacyDispatchTable(
// SRC: system.setLegacyActivationGraph(

// MAIN: --max-ticks
// MAIN: GeneratedModel model;
// MAIN: result.classification

// MAN-DAG: "schema":"agentic-circuit-build-manifest"
// MAN-DAG: "contract_epoch":"0.1"
// MAN-DAG: "build_profile":"fast"
// MAN-DAG: "pass_pipeline":["acsim-emit-cxx"]
// MAN-DAG: "kind":"cpp_header"
// MAN-DAG: "kind":"cpp_source"
// MAN-DAG: "specialization_inputs"
// MAN-DAG: "build_fingerprint":"sha256:
