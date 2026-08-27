// RUN: %acir_opt_public %s | %FileCheck %s
// RUN: %acir_opt_public %s | %acir_opt_public > %t.roundtrip
// RUN: %acir_opt_public %s > %t.canonical
// RUN: diff %t.canonical %t.roundtrip
// RUN: %acir_opt_public --emit-bytecode -o %t.bc %s
// RUN: %acir_opt_public %t.bc | %FileCheck %s
// RUN: %acir_opt_public --pass-pipeline='builtin.module(canonicalize,cse)' %s | %FileCheck %s --check-prefix=RETAIN

builtin.module attributes {ac.contract_epoch = "0.3"} {
  acsim.model @demo epoch "0.3" root @Top
      construction ["Top.fifo", "Top.lanes[0]", "Top.lanes[1]", "Top.tick"]
      destruction ["Top.tick", "Top.lanes[1]", "Top.lanes[0]", "Top.fifo"]
      fingerprints {
        frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000001",
        binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000002",
        provider = "sha256:0000000000000000000000000000000000000000000000000000000000000003",
        profile = "sha256:0000000000000000000000000000000000000000000000000000000000000004",
        toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000005",
        schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000006"
      } {
    acsim.type @comb_domain cpp "gfsim::CombinationalDomain" kind "time_domain" fingerprint "sha256:0e00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @consumer cpp "gfsim::Consumer" kind "role" fingerprint "sha256:0f00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @cpp_bool cpp "bool" kind "value" fingerprint "sha256:1000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @event_kind cpp "gfsim::EventWake" kind "wake" fingerprint "sha256:2000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @gfsim cpp "gfsim" kind "provider" fingerprint "sha256:3000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @initiator cpp "gfsim::Initiator" kind "role" fingerprint "sha256:3f00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @interface cpp "gfsim::Stream" kind "interface" fingerprint "sha256:4000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @interface_alt cpp "gfsim::AlternateStream" kind "interface" fingerprint "sha256:4100000000000000000000000000000000000000000000000000000000000000"
    acsim.type @payload cpp "Packet" kind "packet" fingerprint "sha256:5000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @payload_alt cpp "AlternatePacket" kind "packet" fingerprint "sha256:5100000000000000000000000000000000000000000000000000000000000000"
    acsim.type @port_accessor cpp "output" kind "accessor" fingerprint "sha256:6000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @port_in_accessor cpp "input" kind "accessor" fingerprint "sha256:6f00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @protocol cpp "gfsim::ReadyValid" kind "protocol" fingerprint "sha256:7000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @protocol_alt cpp "gfsim::AlternateProtocol" kind "protocol" fingerprint "sha256:7100000000000000000000000000000000000000000000000000000000000000"
    acsim.type @pure_impl cpp "gfsim::is_ready" kind "implementation" fingerprint "sha256:8000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @pure_schema cpp "pure.schema" kind "schema" fingerprint "sha256:9000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @resource_accessor cpp "initiator" kind "accessor" fingerprint "sha256:a000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @resource_alt cpp "gfsim::AlternateMemory" kind "resource" fingerprint "sha256:a100000000000000000000000000000000000000000000000000000000000000"
    acsim.type @resource_in_accessor cpp "target" kind "accessor" fingerprint "sha256:af00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @resource_kind cpp "gfsim::Memory" kind "resource" fingerprint "sha256:b000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @role cpp "gfsim::Producer" kind "role" fingerprint "sha256:c000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @stateful_impl cpp "gfsim::Fifo" kind "implementation" fingerprint "sha256:d000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @stateful_schema cpp "fifo.schema" kind "schema" fingerprint "sha256:e000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @target cpp "gfsim::Target" kind "role" fingerprint "sha256:ef00000000000000000000000000000000000000000000000000000000000000"

    acsim.binding @pure record {
      activation_sources = [], availability = "available", binding = "pure",
      binding_schema = "acsim-binding-0.1", component_schema = @pure_schema,
      component_schema_fingerprint = "sha256:9000000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.3",
      cpp = {concept = "gfsim::PureModel", entry_points = {pure = "gfsim::is_ready", reset = "", validate = "", work = "", xfer = ""}, header = "gfsim/pure.hpp", symbol = "gfsim::Pure", target = "gfsim"},
      cpp_type = @cpp_bool, effect = "pure", fingerprint = "sha256:1100000000000000000000000000000000000000000000000000000000000000",
      implementation = @pure_impl, ownership = {kind = "none", placement = "inline"},
      parameters = [], ports = [], provider = @gfsim,
      provider_implementation_fingerprint = "sha256:8000000000000000000000000000000000000000000000000000000000000000",
      resources = [], results = [{cpp_type = @cpp_bool, name = "result"}]
    }
    acsim.binding @stateful record {
      activation_sources = [{kind = @event_kind, name = "commit"}], availability = "available", binding = "stateful",
      binding_schema = "acsim-binding-0.1", component_schema = @stateful_schema,
      component_schema_fingerprint = "sha256:e000000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.3",
      cpp = {concept = "gfsim::StatefulModel", entry_points = {pure = "", reset = "fifo_reset", validate = "fifo_validate", work = "fifo_work", xfer = "fifo_xfer"}, header = "gfsim/fifo.hpp", symbol = "gfsim::Fifo", target = "gfsim"},
      cpp_type = @cpp_bool, effect = "stateful", fingerprint = "sha256:1200000000000000000000000000000000000000000000000000000000000000",
      implementation = @stateful_impl, ownership = {kind = "unique", placement = "member_or_array"},
      parameters = [],
      ports = [
        {accessor = @port_accessor, cardinality = "exclusive", delegation = "forbidden", direction = "output", interface = @interface, ownership = "borrowed", payload = @payload, protocol = @protocol, role = @role, time_domain = @comb_domain},
        {accessor = @port_in_accessor, cardinality = "exclusive", delegation = "forbidden", direction = "input", interface = @interface, ownership = "borrowed", payload = @payload, protocol = @protocol, role = @consumer, time_domain = @comb_domain}
      ], provider = @gfsim,
      provider_implementation_fingerprint = "sha256:d000000000000000000000000000000000000000000000000000000000000000",
      resources = [
        {accessor = @resource_accessor, delegation = "forbidden", mode = "initiator", ownership = "borrowed", resource = @resource_kind, role = @initiator, time_domain = @comb_domain},
        {accessor = @resource_in_accessor, delegation = "forbidden", mode = "target", ownership = "borrowed", resource = @resource_kind, role = @target, time_domain = @comb_domain}
      ], results = []
    }
    acsim.module @Top interface {
      ports = [{accessor = @port_accessor, cardinality = "exclusive", delegation = "forbidden", direction = "output", interface = @interface, name = "out_port", ownership = "borrowed", payload = @payload, protocol = @protocol, role = @role, time_domain = @comb_domain}],
      resources = [{accessor = @resource_accessor, delegation = "forbidden", mode = "initiator", name = "memory", ownership = "borrowed", resource = @resource_kind, role = @initiator, time_domain = @comb_domain}],
      results = [{cpp_type = @cpp_bool, name = "out"}]
    } static [] specialization "sha256:2100000000000000000000000000000000000000000000000000000000000000" exports [@out_port, @memory, @out] {
      %fifo = acsim.instance @fifo target @stateful args [] specialization "sha256:2200000000000000000000000000000000000000000000000000000000000000"
        : !acsim.owner<@stateful>
      %lanes = acsim.array @lanes target @stateful args [] specialization "sha256:2200000000000000000000000000000000000000000000000000000000000000" shape [2]
        : !acsim.array<[2], !acsim.owner<@stateful>>
      %lane0 = acsim.element %lanes indices [0]
        : !acsim.array<[2], !acsim.owner<@stateful>> -> !acsim.ref<@stateful>
      %lane1 = acsim.element %lanes indices [1]
        : !acsim.array<[2], !acsim.owner<@stateful>> -> !acsim.ref<@stateful>
      %port0 = acsim.port %lane0 accessor @port_accessor
        : !acsim.ref<@stateful> -> !acsim.port<@interface, @role, @payload, @protocol>
      %port1 = acsim.port %lane1 accessor @port_in_accessor
        : !acsim.ref<@stateful> -> !acsim.port<@interface, @consumer, @payload, @protocol>
      %resource0 = acsim.resource %lane0 accessor @resource_accessor
        : !acsim.ref<@stateful> -> !acsim.resource<@resource_kind, @initiator>
      %resource1 = acsim.resource %lane1 accessor @resource_in_accessor
        : !acsim.ref<@stateful> -> !acsim.resource<@resource_kind, @target>
      acsim.bind %port0 to %port1 kind "port"
        : !acsim.port<@interface, @role, @payload, @protocol>
          to !acsim.port<@interface, @consumer, @payload, @protocol>
      acsim.bind %resource0 to %resource1 kind "resource"
        : !acsim.resource<@resource_kind, @initiator>
          to !acsim.resource<@resource_kind, @target>
      %expr = acsim.inline @pure()
        : () -> !acsim.expr<@cpp_bool>
      %expr2 = acsim.inline @pure(%expr)
        : (!acsim.expr<@cpp_bool>) -> !acsim.expr<@cpp_bool>
      %generated_expr = acsim.inline @pure_impl()
        : () -> !acsim.expr<@cpp_bool>
      acsim.bind %expr to %expr2 kind "pure_view"
        : !acsim.expr<@cpp_bool> to !acsim.expr<@cpp_bool>
      %out_port = acsim.export @out_port %port0 role @role
        : !acsim.port<@interface, @role, @payload, @protocol> -> !acsim.port<@interface, @role, @payload, @protocol>
      %memory = acsim.export @memory %resource0 role @initiator
        : !acsim.resource<@resource_kind, @initiator> -> !acsim.resource<@resource_kind, @initiator>
      %out = acsim.export @out %expr2 role @role
        : !acsim.expr<@cpp_bool> -> !acsim.expr<@cpp_bool>
      acsim.process @tick captures(%lane0 : !acsim.ref<@stateful>) names ["lane"] entry @entry
          pcs [@entry, @wait, @done]
          live [{name = "counter", type = !acsim.value<@cpp_bool>}]
          fairness 8 specialization "sha256:2300000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
        ^bb0(%lane_entry : !acsim.ref<@stateful>):
          %old = acsim.live.load @tick slot "counter"
            : !acsim.value<@cpp_bool>
          acsim.live.store %old in @tick slot "counter" : !acsim.value<@cpp_bool>
          %next = acsim.invoke @stateful(%old)
            : (!acsim.value<@cpp_bool>) -> !acsim.value<@cpp_bool>
          acsim.continue @wait
        }
        state @wait {
        ^bb0(%lane_wait : !acsim.ref<@stateful>):
          %wake = acsim.invoke @stateful()
            : () -> !acsim.wake<@event_kind>
          acsim.suspend @done on %wake : !acsim.wake<@event_kind>
        }
        state @done {
        ^bb0(%lane_done : !acsim.ref<@stateful>):
          %scalar = acsim.inline @pure_impl() : () -> i32
          %wrapped = acsim.inline @pure_impl() : () -> !acsim.value<@cpp_bool>
          %generated_wake = acsim.invoke @stateful_impl()
            : () -> !acsim.wake<@event_kind>
          acsim.terminate "success"
        }
      }
      acsim.return %out_port, %memory, %out : !acsim.port<@interface, @role, @payload, @protocol>, !acsim.resource<@resource_kind, @initiator>, !acsim.expr<@cpp_bool>
    }

    %obj0, %act0 = acsim.dispatch @Top::@fifo path "Top.fifo" indices [] object 0 activation 0
      work "fifo_work" xfer "fifo_xfer" reset "fifo_reset" validate "fifo_validate"
      : !acsim.object_id, !acsim.activation_id
    %obj1, %act1 = acsim.dispatch @Top::@lanes path "Top.lanes[0]" indices [0] object 1 activation 1
      work "fifo_work" xfer "fifo_xfer" reset "fifo_reset" validate "fifo_validate"
      : !acsim.object_id, !acsim.activation_id
    %obj2, %act2 = acsim.dispatch @Top::@lanes path "Top.lanes[1]" indices [1] object 2 activation 2
      work "fifo_work" xfer "fifo_xfer" reset "fifo_reset" validate "fifo_validate"
      : !acsim.object_id, !acsim.activation_id
    %obj3, %act3 = acsim.dispatch @Top::@tick path "Top.tick" indices [] object 3 activation 3
      work "acsim_generated::Top::s2100000000000000000000000000000000000000000000000000000000000000::tick::p2300000000000000000000000000000000000000000000000000000000000000::work"
      xfer "acsim_generated::Top::s2100000000000000000000000000000000000000000000000000000000000000::tick::p2300000000000000000000000000000000000000000000000000000000000000::xfer"
      reset "acsim_generated::Top::s2100000000000000000000000000000000000000000000000000000000000000::tick::p2300000000000000000000000000000000000000000000000000000000000000::reset"
      validate "acsim_generated::Top::s2100000000000000000000000000000000000000000000000000000000000000::tick::p2300000000000000000000000000000000000000000000000000000000000000::validate"
      : !acsim.object_id, !acsim.activation_id
    acsim.activate %act1 to %obj2 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act1 to %obj3 : !acsim.activation_id to !acsim.object_id
  }
}

// CHECK: acsim.model @demo epoch "0.3"
// CHECK: acsim.type @cpp_bool
// CHECK: acsim.binding @pure
// CHECK: acsim.module @Top
// CHECK: acsim.instance @fifo
// CHECK: acsim.array @lanes
// CHECK: acsim.element
// CHECK: acsim.port
// CHECK: acsim.resource
// CHECK: acsim.bind
// CHECK: acsim.inline
// CHECK: acsim.export @out
// CHECK: acsim.process @tick
// CHECK: acsim.live.load
// CHECK: acsim.live.store
// CHECK: acsim.invoke
// CHECK: acsim.continue
// CHECK: acsim.suspend
// CHECK: acsim.terminate
// CHECK: acsim.return
// CHECK: acsim.dispatch @Top::@fifo
// CHECK: acsim.activate %
// RETAIN-COUNT-3: acsim.bind %
// RETAIN-COUNT-4: acsim.dispatch @
// RETAIN-COUNT-2: acsim.activate %
