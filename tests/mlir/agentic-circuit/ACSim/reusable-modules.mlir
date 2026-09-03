// RUN: %acir_opt_public %s | %FileCheck %s
// RUN: %acir_opt_public --emit-bytecode -o %t.bc %s
// RUN: %acir_opt_public %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  acsim.model @reused epoch "0.4" root @Top
      construction ["Top.left", "Top.left.child", "Top.left.pulse", "Top.right[0]", "Top.right[0].child", "Top.right[0].pulse", "Top.right[1]", "Top.right[1].child", "Top.right[1].pulse"]
      destruction ["Top.right[1].pulse", "Top.right[1].child", "Top.right[1]", "Top.right[0].pulse", "Top.right[0].child", "Top.right[0]", "Top.left.pulse", "Top.left.child", "Top.left"]
      fingerprints {
        frozen_acir = "sha256:0000000000000000000000000000000000000000000000000000000000000001",
        binding_lock = "sha256:0000000000000000000000000000000000000000000000000000000000000002",
        provider = "sha256:0000000000000000000000000000000000000000000000000000000000000003",
        profile = "sha256:0000000000000000000000000000000000000000000000000000000000000004",
        toolchain = "sha256:0000000000000000000000000000000000000000000000000000000000000005",
        schema_set = "sha256:0000000000000000000000000000000000000000000000000000000000000006"
      } {
    acsim.type @impl cpp "Component" kind "implementation" fingerprint "sha256:1000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @provider cpp "Provider" kind "provider" fingerprint "sha256:2000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @schema cpp "schema" kind "schema" fingerprint "sha256:3000000000000000000000000000000000000000000000000000000000000000"
    acsim.type @value cpp "bool" kind "value" fingerprint "sha256:4000000000000000000000000000000000000000000000000000000000000000"

    acsim.binding @child_binding record {
      activation_sources = [], availability = "available", binding = "child_binding",
      binding_schema = "acsim-binding-0.1", component_schema = @schema,
      component_schema_fingerprint = "sha256:3000000000000000000000000000000000000000000000000000000000000000",
      construction = {arguments = [], kind = "constructor"}, contract_epoch = "0.4",
      cpp = {concept = "StatefulComponent", entry_points = {pure = "", reset = "child_reset", validate = "child_validate", work = "child_work", xfer = "child_xfer"}, header = "child.hpp", symbol = "Child", target = "model"},
      cpp_type = @value, effect = "stateful", fingerprint = "sha256:5000000000000000000000000000000000000000000000000000000000000000",
      implementation = @impl, ownership = {kind = "unique", placement = "member_or_array"},
      parameters = [], ports = [], provider = @provider,
      provider_implementation_fingerprint = "sha256:1000000000000000000000000000000000000000000000000000000000000000",
      resources = [], results = []
    }
    acsim.module @Leaf interface {ports = [], resources = [], results = []} static [2 : i64] specialization "sha256:8000000000000000000000000000000000000000000000000000000000000000" exports [] {
      %child = acsim.instance @child target @child_binding args [] specialization "sha256:9000000000000000000000000000000000000000000000000000000000000000"
        : !acsim.owner<@child_binding>
      acsim.process @pulse captures() names [] entry @entry pcs [@entry] live [] fairness 1 specialization "sha256:b000000000000000000000000000000000000000000000000000000000000000" {
        state @entry {
          acsim.terminate "success"
        }
      }
      acsim.return
    }
    acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:a000000000000000000000000000000000000000000000000000000000000000" exports [] {
      %left = acsim.instance @left target @Leaf args [2 : i64] specialization "sha256:8000000000000000000000000000000000000000000000000000000000000000"
        : !acsim.owner<@Leaf>
      %right = acsim.array @right target @Leaf args [2 : i64] specialization "sha256:8000000000000000000000000000000000000000000000000000000000000000" shape [2]
        : !acsim.array<[2], !acsim.owner<@Leaf>>
      acsim.return
    }

    %obj0, %act0 = acsim.dispatch @Leaf::@child path "Top.left.child" indices [] object 0 activation 0
      work "child_work" xfer "child_xfer" reset "child_reset" validate "child_validate"
      : !acsim.object_id, !acsim.activation_id
    %obj1, %act1 = acsim.dispatch @Leaf::@pulse path "Top.left.pulse" indices [] object 1 activation 1
      work "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::work"
      xfer "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::xfer"
      reset "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::reset"
      validate "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::validate"
      : !acsim.object_id, !acsim.activation_id
    %obj2, %act2 = acsim.dispatch @Leaf::@child path "Top.right[0].child" indices [] object 2 activation 2
      work "child_work" xfer "child_xfer" reset "child_reset" validate "child_validate"
      : !acsim.object_id, !acsim.activation_id
    %obj3, %act3 = acsim.dispatch @Leaf::@pulse path "Top.right[0].pulse" indices [] object 3 activation 3
      work "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::work"
      xfer "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::xfer"
      reset "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::reset"
      validate "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::validate"
      : !acsim.object_id, !acsim.activation_id
    %obj4, %act4 = acsim.dispatch @Leaf::@child path "Top.right[1].child" indices [] object 4 activation 4
      work "child_work" xfer "child_xfer" reset "child_reset" validate "child_validate"
      : !acsim.object_id, !acsim.activation_id
    %obj5, %act5 = acsim.dispatch @Leaf::@pulse path "Top.right[1].pulse" indices [] object 5 activation 5
      work "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::work"
      xfer "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::xfer"
      reset "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::reset"
      validate "acsim_generated::Leaf::s8000000000000000000000000000000000000000000000000000000000000000::pulse::pb000000000000000000000000000000000000000000000000000000000000000::validate"
      : !acsim.object_id, !acsim.activation_id
    acsim.activate %act0 to %obj0 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act1 to %obj1 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act2 to %obj2 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act3 to %obj3 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act4 to %obj4 : !acsim.activation_id to !acsim.object_id
    acsim.activate %act5 to %obj5 : !acsim.activation_id to !acsim.object_id
  }
}

// CHECK: acsim.module @Leaf
// CHECK: acsim.module @Top
// CHECK: acsim.dispatch @Leaf::@child path "Top.left.child"
// CHECK: acsim.dispatch @Leaf::@pulse path "Top.left.pulse"
// CHECK: acsim.dispatch @Leaf::@child path "Top.right[0].child"
// CHECK: acsim.dispatch @Leaf::@pulse path "Top.right[0].pulse"
// CHECK: acsim.dispatch @Leaf::@child path "Top.right[1].child"
// CHECK: acsim.dispatch @Leaf::@pulse path "Top.right[1].pulse"
