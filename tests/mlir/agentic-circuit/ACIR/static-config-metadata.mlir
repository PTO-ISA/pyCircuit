// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/valid.mlir | %FileCheck %s --check-prefix=VALID
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/forged-value.mlir 2>&1 | %FileCheck %s --check-prefix=FORGED-VALUE
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/forged-path.mlir 2>&1 | %FileCheck %s --check-prefix=FORGED-PATH
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/forged-schema.mlir 2>&1 | %FileCheck %s --check-prefix=FORGED-SCHEMA
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/missing-root.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-ROOT
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/wrong-root-type.mlir 2>&1 | %FileCheck %s --check-prefix=WRONG-ROOT-TYPE

// VALID: ac.static_config_bindings
// VALID: root = "cfg"
// FORGED-VALUE: error: static config projection 'cfg.entries' disagrees with its root binding
// FORGED-PATH: error: static config projection 'cfg.missing' disagrees with its root binding
// FORGED-SCHEMA: error: static config binding metadata is malformed
// MISSING-ROOT: error: static config projection 'cfg.entries' must match exactly one config root
// WRONG-ROOT-TYPE: error: static config bindings must be an array

//--- valid.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.static_config_bindings = [{root = "cfg", schema = "{\22fields\22:[{\22name\22:\22entries\22,\22type\22:{\22kind\22:\22scalar\22,\22name\22:\22int\22,\22version\22:1}}],\22kind\22:\22config\22,\22name\22:\22Config\22,\22version\22:1}", schema_sha256 = "sha256:8d50b171414319202ddc53539c063d17b2a5bb762254463f741967108429e99e", type = "Config", value = "{\22entries\22:5}"}],
  ac.static_type_bindings = {cfg.entries = 5 : i64},
  ac.static_type_checks = [{program = ["param:cfg.entries"], result = 5 : i64, target = "Entry.index:bits"}]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- forged-value.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.static_config_bindings = [{root = "cfg", schema = "{\22fields\22:[{\22name\22:\22entries\22,\22type\22:{\22kind\22:\22scalar\22,\22name\22:\22int\22,\22version\22:1}}],\22kind\22:\22config\22,\22name\22:\22Config\22,\22version\22:1}", schema_sha256 = "sha256:8d50b171414319202ddc53539c063d17b2a5bb762254463f741967108429e99e", type = "Config", value = "{\22entries\22:4}"}],
  ac.static_type_bindings = {cfg.entries = 5 : i64},
  ac.static_type_checks = [{program = ["param:cfg.entries"], result = 5 : i64, target = "Entry.index:bits"}]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- forged-path.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.static_config_bindings = [{root = "cfg", schema = "{\22fields\22:[{\22name\22:\22entries\22,\22type\22:{\22kind\22:\22scalar\22,\22name\22:\22int\22,\22version\22:1}}],\22kind\22:\22config\22,\22name\22:\22Config\22,\22version\22:1}", schema_sha256 = "sha256:8d50b171414319202ddc53539c063d17b2a5bb762254463f741967108429e99e", type = "Config", value = "{\22entries\22:5}"}],
  ac.static_type_bindings = {cfg.missing = 5 : i64},
  ac.static_type_checks = [{program = ["param:cfg.missing"], result = 5 : i64, target = "Entry.index:bits"}]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- forged-schema.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.static_config_bindings = [{root = "cfg", schema = "{\22fields\22:[{\22name\22:\22entries\22,\22type\22:{\22kind\22:\22scalar\22,\22name\22:\22int\22,\22version\22:1}}],\22kind\22:\22config\22,\22name\22:\22Config\22,\22version\22:1}", schema_sha256 = "sha256:forged", type = "Config", value = "{\22entries\22:5}"}],
  ac.static_type_bindings = {cfg.entries = 5 : i64},
  ac.static_type_checks = [{program = ["param:cfg.entries"], result = 5 : i64, target = "Entry.index:bits"}]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- missing-root.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.static_type_bindings = {cfg.entries = 5 : i64},
  ac.static_type_checks = [{program = ["param:cfg.entries"], result = 5 : i64, target = "Entry.index:bits"}]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}

//--- wrong-root-type.mlir
builtin.module attributes {
  ac.contract_epoch = "0.5",
  ac.static_config_bindings = "forged",
  ac.static_type_bindings = {cfg.entries = 5 : i64},
  ac.static_type_checks = [{program = ["param:cfg.entries"], result = 5 : i64, target = "Entry.index:bits"}]
} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "index", type = i5}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
}
