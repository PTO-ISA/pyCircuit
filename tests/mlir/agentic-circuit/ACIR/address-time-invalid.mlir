// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/period-zero.mlir 2>&1 | %FileCheck %s --check-prefix=PERIOD
// RUN: %not %acir_opt %t/phase-negative.mlir 2>&1 | %FileCheck %s --check-prefix=PHASE
// RUN: %not %acir_opt %t/tick-overflow.mlir 2>&1 | %FileCheck %s --check-prefix=TICK-OVERFLOW
// RUN: %not %acir_opt %t/domain-cycle.mlir 2>&1 | %FileCheck %s --check-prefix=DOMAIN-CYCLE
// RUN: %not %acir_opt %t/missing-bridge.mlir 2>&1 | %FileCheck %s --check-prefix=BRIDGE
// RUN: %not %acir_opt %t/width.mlir 2>&1 | %FileCheck %s --check-prefix=WIDTH
// RUN: %not %acir_opt %t/address-cycle.mlir 2>&1 | %FileCheck %s --check-prefix=ADDRESS-CYCLE
// RUN: %not %acir_opt %t/range-overflow.mlir 2>&1 | %FileCheck %s --check-prefix=RANGE
// RUN: %not %acir_opt %t/overlap.mlir 2>&1 | %FileCheck %s --check-prefix=OVERLAP
// RUN: %not %acir_opt %t/overlap.mlir > /dev/null 2> %t/overlap.first
// RUN: %not %acir_opt %t/overlap.mlir > /dev/null 2> %t/overlap.second
// RUN: diff %t/overlap.first %t/overlap.second
// RUN: %not %acir_opt %t/equal-priority.mlir 2>&1 | %FileCheck %s --check-prefix=PRIORITY
// RUN: %not %acir_opt %t/interleave.mlir 2>&1 | %FileCheck %s --check-prefix=INTERLEAVE
// RUN: %not %acir_opt %t/default.mlir 2>&1 | %FileCheck %s --check-prefix=DEFAULT
// RUN: %not %acir_opt %t/address-segment.mlir 2>&1 | %FileCheck %s --check-prefix=ADDRESS-SEGMENT
// RUN: %not %acir_opt %t/address-unit.mlir 2>&1 | %FileCheck %s --check-prefix=ADDRESS-UNIT
// RUN: %not %acir_opt %t/address-layout.mlir 2>&1 | %FileCheck %s --check-prefix=ADDRESS-LAYOUT
// RUN: %not %acir_opt %t/address-parent-pair.mlir 2>&1 | %FileCheck %s --check-prefix=PARENT-PAIR
// RUN: %not %acir_opt %t/address-parent.mlir 2>&1 | %FileCheck %s --check-prefix=PARENT
// RUN: %not %acir_opt %t/address-translation.mlir 2>&1 | %FileCheck %s --check-prefix=TRANSLATION
// RUN: %not %acir_opt %t/address-width-translation.mlir 2>&1 | %FileCheck %s --check-prefix=TRANSLATED-WIDTH
// RUN: %not %acir_opt %t/map-name.mlir 2>&1 | %FileCheck %s --check-prefix=MAP-NAME
// RUN: %not %acir_opt %t/map-source.mlir 2>&1 | %FileCheck %s --check-prefix=MAP-SOURCE
// RUN: %not %acir_opt %t/source-width.mlir 2>&1 | %FileCheck %s --check-prefix=SOURCE-WIDTH
// RUN: %not %acir_opt %t/target-width.mlir 2>&1 | %FileCheck %s --check-prefix=TARGET-WIDTH
// RUN: %not %acir_opt %t/permissions.mlir 2>&1 | %FileCheck %s --check-prefix=PERMISSIONS
// RUN: %not %acir_opt %t/interleave-stripe.mlir 2>&1 | %FileCheck %s --check-prefix=INTERLEAVE-STRIPE
// RUN: %not %acir_opt %t/default-target.mlir 2>&1 | %FileCheck %s --check-prefix=DEFAULT-TARGET
// RUN: %not %acir_opt %t/time-name.mlir 2>&1 | %FileCheck %s --check-prefix=TIME-NAME
// RUN: %not %acir_opt %t/time-parent.mlir 2>&1 | %FileCheck %s --check-prefix=TIME-PARENT
// RUN: %not %acir_opt %t/bridge-schema.mlir 2>&1 | %FileCheck %s --check-prefix=BRIDGE-SCHEMA
// RUN: %not %acir_opt %t/address-self.mlir 2>&1 | %FileCheck %s --check-prefix=ADDRESS-SELF
// RUN: %not %acir_opt %t/map-target.mlir 2>&1 | %FileCheck %s --check-prefix=MAP-TARGET
// RUN: %not %acir_opt %t/map-key.mlir 2>&1 | %FileCheck %s --check-prefix=MAP-KEY
// RUN: %not %acir_opt %t/map-unknown-key.mlir 2>&1 | %FileCheck %s --check-prefix=MAP-UNKNOWN-KEY
// RUN: %not %acir_opt %t/map-class.mlir 2>&1 | %FileCheck %s --check-prefix=MAP-CLASS
// RUN: %not %acir_opt %t/target-overflow.mlir 2>&1 | %FileCheck %s --check-prefix=TARGET-OVERFLOW
// RUN: %not %acir_opt %t/time-self.mlir 2>&1 | %FileCheck %s --check-prefix=TIME-SELF
// RUN: %not %acir_opt %t/noncanonical-rational.mlir 2>&1 | %FileCheck %s --check-prefix=RATIONAL
// RUN: %not %acir_opt %t/fractional-no-alignment.mlir 2>&1 | %FileCheck %s --check-prefix=NO-ALIGNMENT
// RUN: %not %acir_opt %t/fractional-bad-alignment.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-ALIGNMENT
// RUN: %not %acir_opt %t/zero-size.mlir 2>&1 | %FileCheck %s --check-prefix=ZERO-SIZE
// RUN: %not %acir_opt %t/interleave-alignment.mlir 2>&1 | %FileCheck %s --check-prefix=INTERLEAVE-ALIGNMENT
// RUN: %not %acir_opt %t/mixed-interleave-overlap.mlir 2>&1 | %FileCheck %s --check-prefix=MIXED-INTERLEAVE-OVERLAP
// RUN: %not %acir_opt %t/relation-limit.mlir 2>&1 | %FileCheck %s --check-prefix=RELATION-LIMIT
// RUN: %not %acir_opt %t/different-granularity-overlap.mlir 2>&1 | %FileCheck %s --check-prefix=DIFFERENT-GRANULARITY-OVERLAP

//--- period-zero.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.time_domain"() <{sym_name = "t", period = 0 : i64, phase = 0 : i64, tick_scale = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// PERIOD: period must be positive global ticks

//--- phase-negative.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.time_domain"() <{sym_name = "t", period = 1 : i64, phase = -1 : i64, tick_scale = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// PHASE: phase must be non-negative global ticks

//--- tick-overflow.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.time_domain"() <{sym_name = "t", period = 9223372036854775807 : i64, phase = 1 : i64, tick_scale = 4294967297 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// TICK-OVERFLOW: tick scale exceeds implementation capability

//--- domain-cycle.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "Bridge", function_type = () -> (), static_params = {}}> ({
    "ac.return"() : () -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.instance"() <{definition = @Bridge, sym_name = "x", stable_id = "x", path = "x", static_args = {}}> : () -> ()
    "ac.time_domain"() <{sym_name = "a", period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64, parent = @b, bridge = {kind = "explicit", owner = @x}}> : () -> ()
    "ac.time_domain"() <{sym_name = "b", period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64, parent = @a, bridge = {kind = "explicit", owner = @x}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// DOMAIN-CYCLE: time-domain parent cycle

//--- missing-bridge.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.time_domain"() <{sym_name = "a", period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64}> : () -> ()
    "ac.time_domain"() <{sym_name = "b", period = 2 : i64, phase = 0 : i64, tick_scale = 1 : i64, parent = @a}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// BRIDGE: cross-domain parent relation requires explicit bridge metadata

//--- width.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 65 : i64, address_unit = "byte"}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// WIDTH: address width must be in [1, 64]

//--- address-cycle.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte", parent = @b, translation = {numerator = 1 : i64, denominator = 1 : i64, offset = 0 : i64}}> : () -> ()
    "ac.address_space"() <{sym_name = "b", stable_id = "b", path = "b", address_width = 32 : i64, address_unit = "byte", parent = @a, translation = {numerator = 1 : i64, denominator = 1 : i64, offset = 0 : i64}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ADDRESS-CYCLE: address-space parent cycle

//--- range-overflow.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 64 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = -1 : i64, size = 2 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = []}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// RANGE: address interval exceeds source address width

//--- overlap.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 8 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = []}, {base = 4 : i64, size = 8 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = []}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// OVERLAP: overlapping selector intersections require explicit distinct priorities

//--- equal-priority.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 8 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = [], priority = 1 : i64}, {base = 4 : i64, size = 8 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = [], priority = 1 : i64}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// PRIORITY: overlapping selector intersections have equal priority

//--- interleave.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 256 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = [], interleave = {granularity = 64 : i64, banks = 4 : i64, bank = 4 : i64}}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// INTERLEAVE: interleave bank must be in [0, banks)

//--- default.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [], default_behavior = {kind = "drop"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// DEFAULT: default behavior requires exact unmapped or target schema

//--- address-segment.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a.b", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ADDRESS-SEGMENT: address-space name, stable id, and path must be stable local segments

//--- address-unit.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "word"}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ADDRESS-UNIT: address unit must be exactly 'byte' or 'bit'

//--- address-layout.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte", data_layout = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ADDRESS-LAYOUT: data layout hook must implement DataLayoutSpecInterface

//--- address-parent-pair.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte", parent = @b}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// PARENT-PAIR: parent address space and translation must appear together

//--- address-parent.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte", parent = @missing, translation = {numerator = 1 : i64, denominator = 1 : i64, offset = 0 : i64}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// PARENT: parent address space '@missing' is unresolved

//--- address-translation.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "parent", stable_id = "parent", path = "parent", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_space"() <{sym_name = "child", stable_id = "child", path = "child", address_width = 16 : i64, address_unit = "byte", parent = @parent, translation = {numerator = 0 : i64, denominator = 1 : i64, offset = 0 : i64}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// TRANSLATION: translation values must be unsigned signless i64 with positive ratio

//--- address-width-translation.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "parent", stable_id = "parent", path = "parent", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_space"() <{sym_name = "child", stable_id = "child", path = "child", address_width = 8 : i64, address_unit = "byte", parent = @parent, translation = {numerator = 2 : i64, denominator = 1 : i64, offset = 0 : i64}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// TRANSLATED-WIDTH: translated child address range exceeds parent address width

//--- map-name.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_map"() <{sym_name = "bad.name", source = @a, entries = [], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// MAP-NAME: address-map name must be one stable local segment

//--- map-source.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_map"() <{sym_name = "m", source = @missing, entries = [], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// MAP-SOURCE: source address space '@missing' is unresolved

//--- source-width.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 4 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 8 : i64, size = 9 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = []}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// SOURCE-WIDTH: address interval exceeds source address width

//--- target-width.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "source", stable_id = "source", path = "source", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_space"() <{sym_name = "target", stable_id = "target", path = "target", address_width = 4 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @source, entries = [{base = 0 : i64, size = 2 : i64, target = @target, offset = 15 : i64, permissions = ["read"], classes = []}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// TARGET-WIDTH: address target range exceeds target address width

//--- permissions.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 1 : i64, target = @a, offset = 0 : i64, permissions = ["write", "write"], classes = []}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// PERMISSIONS: address-map permissions must be unique read/write/execute values

//--- interleave-stripe.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 16 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 255 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = [], interleave = {granularity = 2 : i64, banks = -1 : i64, bank = 0 : i64}}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// INTERLEAVE-STRIPE: interleave geometry exceeds unsigned i64

//--- default-target.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [], default_behavior = {kind = "target", target = @missing}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// DEFAULT-TARGET: default target behavior is unresolved

//--- time-name.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.time_domain"() <{sym_name = "bad.name", period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// TIME-NAME: time-domain name must be one stable local segment

//--- time-parent.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "Bridge", function_type = () -> (), static_params = {}}> ({ "ac.return"() : () -> () }) : () -> ()
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.instance"() <{definition = @Bridge, sym_name = "x", stable_id = "x", path = "x", static_args = {}}> : () -> ()
    "ac.time_domain"() <{sym_name = "t", period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64, parent = @missing, bridge = {kind = "explicit", owner = @x}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// TIME-PARENT: parent time domain '@missing' is unresolved

//--- bridge-schema.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "Bridge", function_type = () -> (), static_params = {}}> ({ "ac.return"() : () -> () }) : () -> ()
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.instance"() <{definition = @Bridge, sym_name = "x", stable_id = "x", path = "x", static_args = {}}> : () -> ()
    "ac.time_domain"() <{sym_name = "parent", period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64}> : () -> ()
    "ac.time_domain"() <{sym_name = "child", period = 2 : i64, phase = 0 : i64, tick_scale = 1 : i64, parent = @parent, bridge = {kind = "implicit", owner = @x}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// BRIDGE-SCHEMA: bridge requires exact {kind = "explicit", owner = local symbol} schema

//--- address-self.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte", parent = @a, translation = {numerator = 1 : i64, denominator = 1 : i64, offset = 0 : i64}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ADDRESS-SELF: address-space parent cycle: @a -> @a

//--- map-target.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 1 : i64, target = @missing, offset = 0 : i64, permissions = ["read"], classes = []}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// MAP-TARGET: address-map target '@missing' is unresolved

//--- map-key.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 1 : i64, target = @a, offset = 0 : i64, permissions = ["read"]}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// MAP-KEY: address-map entry is missing 'classes'

//--- map-unknown-key.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 1 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = [], extra = true}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// MAP-UNKNOWN-KEY: unknown address-map entry key 'extra'

//--- map-class.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 32 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 1 : i64, target = @a, offset = 0 : i64, permissions = ["read"], classes = [@missing]}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// MAP-CLASS: address-map transaction class '@missing' is unresolved

//--- target-overflow.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 64 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 0 : i64, size = 2 : i64, target = @a, offset = -1 : i64, permissions = ["read"], classes = []}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// TARGET-OVERFLOW: address target range exceeds target address width

//--- time-self.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "Bridge", function_type = () -> (), static_params = {}}> ({
    "ac.return"() : () -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.instance"() <{definition = @Bridge, sym_name = "x", stable_id = "x", path = "x", static_args = {}}> : () -> ()
    "ac.time_domain"() <{sym_name = "t", period = 1 : i64, phase = 0 : i64, tick_scale = 1 : i64, parent = @t, bridge = {kind = "explicit", owner = @x}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// TIME-SELF: time-domain parent cycle: @t -> @t

//--- noncanonical-rational.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "parent", stable_id = "parent", path = "parent", address_width = 16 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_space"() <{sym_name = "child", stable_id = "child", path = "child", address_width = 8 : i64, address_unit = "byte", parent = @parent, translation = {numerator = 2 : i64, denominator = 2 : i64, offset = 0 : i64}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// RATIONAL: translation rational must be in canonical reduced form

//--- fractional-no-alignment.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "parent", stable_id = "parent", path = "parent", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_space"() <{sym_name = "child", stable_id = "child", path = "child", address_width = 8 : i64, address_unit = "byte", parent = @parent, translation = {numerator = 1 : i64, denominator = 2 : i64, offset = 0 : i64}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// NO-ALIGNMENT: fractional translation requires exact positive alignment proving divisibility

//--- fractional-bad-alignment.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "parent", stable_id = "parent", path = "parent", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_space"() <{sym_name = "child", stable_id = "child", path = "child", address_width = 8 : i64, address_unit = "byte", parent = @parent, translation = {numerator = 1 : i64, denominator = 2 : i64, offset = 0 : i64, alignment = 1 : i64}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// BAD-ALIGNMENT: fractional translation requires exact positive alignment proving divisibility

//--- zero-size.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 64 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = -1 : i64, size = 0 : i64, target = @a, offset = -1 : i64, permissions = ["read"], classes = []}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ZERO-SIZE: address-map entry size must be positive

//--- interleave-alignment.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "a", stable_id = "a", path = "a", address_width = 16 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @a, entries = [{base = 1 : i64, size = 256 : i64, target = @a, offset = 1 : i64, permissions = ["read"], classes = [], interleave = {granularity = 64 : i64, banks = 4 : i64, bank = 0 : i64}}], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// INTERLEAVE-ALIGNMENT: interleave offset must be aligned to granularity

//--- mixed-interleave-overlap.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "space", stable_id = "space", path = "space", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @space, entries = [
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}}
    ], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// MIXED-INTERLEAVE-OVERLAP: overlapping selector intersections require explicit distinct priorities

//--- different-granularity-overlap.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "space", stable_id = "space", path = "space", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @space, entries = [
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 2 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 1 : i64}}
    ], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// DIFFERENT-GRANULARITY-OVERLAP: overlapping selector intersections require explicit distinct priorities

//--- relation-limit.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.module"() <{sym_name = "M", function_type = () -> (), static_params = {}}> ({
    "ac.address_space"() <{sym_name = "space", stable_id = "space", path = "space", address_width = 8 : i64, address_unit = "byte"}> : () -> ()
    "ac.address_map"() <{sym_name = "m", source = @space, entries = [
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}}
    ], default_behavior = {kind = "unmapped"}}> : () -> ()
    "ac.return"() : () -> ()
  }) : () -> ()
}
// RELATION-LIMIT: general mixed interleave analysis exceeds ACIR limit 256
