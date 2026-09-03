// RUN: %acir_opt_public %s | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  acsim.model @wrapper_activation epoch "0.4" root @Top
      construction ["Top.left", "Top.left.child", "Top.port_sink", "Top.resource_sink"]
      destruction ["Top.resource_sink", "Top.port_sink", "Top.left.child", "Top.left"]
      fingerprints {
        frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000001",
        binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000002",
        provider = "sha256:0000000000000000000000000000000000000000000000000000000000000003",
        profile = "sha256:0000000000000000000000000000000000000000000000000000000000000004",
        toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000005",
        schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000006"
      } {
    acsim.type @comb_domain cpp "Domain" kind "time_domain" fingerprint "sha256:0100000000000000000000000000000000000000000000000000000000000000"
    acsim.type @consumer cpp "Consumer" kind "role" fingerprint "sha256:0200000000000000000000000000000000000000000000000000000000000000"
    acsim.type @impl cpp "Component" kind "implementation" fingerprint "sha256:0300000000000000000000000000000000000000000000000000000000000000"
    acsim.type @initiator cpp "Initiator" kind "role" fingerprint "sha256:0400000000000000000000000000000000000000000000000000000000000000"
    acsim.type @interface cpp "Stream" kind "interface" fingerprint "sha256:0500000000000000000000000000000000000000000000000000000000000000"
    acsim.type @payload cpp "Packet" kind "packet" fingerprint "sha256:0600000000000000000000000000000000000000000000000000000000000000"
    acsim.type @port_in cpp "input" kind "accessor" fingerprint "sha256:0700000000000000000000000000000000000000000000000000000000000000"
    acsim.type @port_out cpp "output" kind "accessor" fingerprint "sha256:0800000000000000000000000000000000000000000000000000000000000000"
    acsim.type @protocol cpp "ReadyValid" kind "protocol" fingerprint "sha256:0900000000000000000000000000000000000000000000000000000000000000"
    acsim.type @provider cpp "Provider" kind "provider" fingerprint "sha256:0a00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @resource_in cpp "target" kind "accessor" fingerprint "sha256:0b00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @resource_kind cpp "Memory" kind "resource" fingerprint "sha256:0c00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @resource_out cpp "initiator" kind "accessor" fingerprint "sha256:0d00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @role cpp "Producer" kind "role" fingerprint "sha256:0e00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @schema cpp "schema" kind "schema" fingerprint "sha256:0f00000000000000000000000000000000000000000000000000000000000000"
    acsim.type @target cpp "Target" kind "role" fingerprint "sha256:1000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @value cpp "bool" kind "value" fingerprint "sha256:1100000000000000000000000000000000000000000000000000000000000000"

    acsim.binding @endpoint_binding record {
      activation_sources = [], availability = "available", binding = "endpoint_binding",
      binding_schema = "acsim-binding-0.1", component_schema = @schema,
      component_schema_fingerprint = "sha256:0f00000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.4",
      cpp = {concept = "StatefulComponent", entry_points = {pure = "", reset = "endpoint_reset", validate = "endpoint_validate", work = "endpoint_work", xfer = "endpoint_xfer"}, header = "endpoint.hpp", symbol = "Endpoint", target = "model"},
      cpp_type = @value, effect = "stateful", fingerprint = "sha256:1200000000000000000000000000000000000000000000000000000000000000",
      implementation = @impl, ownership = {kind = "unique", placement = "member_or_array"},
      parameters = [], ports = [
        {accessor = @port_in, cardinality = "exclusive", delegation = "allowed", direction = "input", interface = @interface, ownership = "borrowed", payload = @payload, protocol = @protocol, role = @consumer, time_domain = @comb_domain},
        {accessor = @port_out, cardinality = "exclusive", delegation = "allowed", direction = "output", interface = @interface, ownership = "borrowed", payload = @payload, protocol = @protocol, role = @role, time_domain = @comb_domain}
      ], provider = @provider,
      provider_implementation_fingerprint = "sha256:0300000000000000000000000000000000000000000000000000000000000000",
      resources = [
        {accessor = @resource_in, delegation = "allowed", mode = "target", ownership = "borrowed", resource = @resource_kind, role = @target, time_domain = @comb_domain},
        {accessor = @resource_out, delegation = "allowed", mode = "initiator", ownership = "borrowed", resource = @resource_kind, role = @initiator, time_domain = @comb_domain}
      ], results = []
    }
    acsim.module @Leaf interface {
      ports = [{accessor = @port_out, cardinality = "exclusive", delegation = "allowed", direction = "output", interface = @interface, name = "out", ownership = "borrowed", payload = @payload, protocol = @protocol, role = @role, time_domain = @comb_domain}],
      resources = [{accessor = @resource_out, delegation = "allowed", mode = "initiator", name = "out", ownership = "borrowed", resource = @resource_kind, role = @initiator, time_domain = @comb_domain}],
      results = []
    } static [] specialization "sha256:1300000000000000000000000000000000000000000000000000000000000000" exports [@out, @out] {
      %child = acsim.instance @child target @endpoint_binding args [] specialization "sha256:1400000000000000000000000000000000000000000000000000000000000000"
        : !acsim.owner<@endpoint_binding>
      %port = acsim.port %child accessor @port_out
        : !acsim.owner<@endpoint_binding> -> !acsim.port<@interface, @role, @payload, @protocol>
      %resource = acsim.resource %child accessor @resource_out
        : !acsim.owner<@endpoint_binding> -> !acsim.resource<@resource_kind, @initiator>
      %port_out = acsim.export @out %port role @role
        : !acsim.port<@interface, @role, @payload, @protocol> -> !acsim.port<@interface, @role, @payload, @protocol>
      %resource_out = acsim.export @out %resource role @initiator
        : !acsim.resource<@resource_kind, @initiator> -> !acsim.resource<@resource_kind, @initiator>
      acsim.return %port_out, %resource_out : !acsim.port<@interface, @role, @payload, @protocol>, !acsim.resource<@resource_kind, @initiator>
    }
    acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:1500000000000000000000000000000000000000000000000000000000000000" exports [] {
      %left = acsim.instance @left target @Leaf args [] specialization "sha256:1300000000000000000000000000000000000000000000000000000000000000"
        : !acsim.owner<@Leaf>
      %port_sink = acsim.instance @port_sink target @endpoint_binding args [] specialization "sha256:1400000000000000000000000000000000000000000000000000000000000000"
        : !acsim.owner<@endpoint_binding>
      %resource_sink = acsim.instance @resource_sink target @endpoint_binding args [] specialization "sha256:1400000000000000000000000000000000000000000000000000000000000000"
        : !acsim.owner<@endpoint_binding>
      %source_port = acsim.port %left accessor @port_out
        : !acsim.owner<@Leaf> -> !acsim.port<@interface, @role, @payload, @protocol>
      %target_port = acsim.port %port_sink accessor @port_in
        : !acsim.owner<@endpoint_binding> -> !acsim.port<@interface, @consumer, @payload, @protocol>
      %source_resource = acsim.resource %left accessor @resource_out
        : !acsim.owner<@Leaf> -> !acsim.resource<@resource_kind, @initiator>
      %target_resource = acsim.resource %resource_sink accessor @resource_in
        : !acsim.owner<@endpoint_binding> -> !acsim.resource<@resource_kind, @target>
      acsim.bind %source_port to %target_port kind "port"
        : !acsim.port<@interface, @role, @payload, @protocol> to !acsim.port<@interface, @consumer, @payload, @protocol>
      acsim.bind %source_resource to %target_resource kind "resource"
        : !acsim.resource<@resource_kind, @initiator> to !acsim.resource<@resource_kind, @target>
      acsim.return
    }

    %obj0, %act0 = acsim.dispatch @Leaf::@child path "Top.left.child" indices [] object 0 activation 0
      work "endpoint_work" xfer "endpoint_xfer" reset "endpoint_reset" validate "endpoint_validate"
      : !acsim.object_id, !acsim.activation_id
    %obj1, %act1 = acsim.dispatch @Top::@port_sink path "Top.port_sink" indices [] object 1 activation 1
      work "endpoint_work" xfer "endpoint_xfer" reset "endpoint_reset" validate "endpoint_validate"
      : !acsim.object_id, !acsim.activation_id
    %obj2, %act2 = acsim.dispatch @Top::@resource_sink path "Top.resource_sink" indices [] object 2 activation 2
      work "endpoint_work" xfer "endpoint_xfer" reset "endpoint_reset" validate "endpoint_validate"
      : !acsim.object_id, !acsim.activation_id
    acsim.activate %act0 to %obj0 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act0 to %obj1 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act0 to %obj2 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act1 to %obj1 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act2 to %obj2 : !acsim.activation_id to !acsim.object_id
  }
}

// CHECK: acsim.activate %{{.*}} to %{{.*}}
