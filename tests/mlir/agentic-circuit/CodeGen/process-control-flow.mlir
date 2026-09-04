// RUN: %acir_opt %s -o /dev/null

module attributes {ac.contract_epoch = "0.5"} {
  acsim.model @soc epoch "0.5" root @Top construction ["root.workload"] destruction ["root.workload"] fingerprints {binding_lock = "sha256:4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945", frozen_acir = "sha256:a2576d5d0598c9e71d4f09c4663e4d63c59950c0be81cf669b2024de9e74160e", profile = "sha256:079c9d12005aad817f722d2f0a34ccc3185b5ec0ce06ee243f945e4e1bb7b4c7", provider = "sha256:4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945", schema_set = "sha256:4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945", toolchain = "sha256:1db0fba21ec705a314dbbced115002a1c868a1c90beb0313edd03005070db9ac"} {
    acsim.type @acir_impl_wake_condition cpp "acir::generated::impl_wake_condition" kind "implementation" fingerprint "sha256:45b3c3013f80d8a2402c692e1664935a088c00032f93510b39647faaecfe7fc5"
    acsim.type @acir_impl_wake_next_delta cpp "acir::generated::impl_wake_next_delta" kind "implementation" fingerprint "sha256:a311590813b87bcc30389b814d751def9dfefb49dcd07a9c485b90701a83617e"
    acsim.type @acir_wake_condition cpp "acir::generated::wake_condition" kind "wake" fingerprint "sha256:698e1e5b1308d66e1487b9860c75ee4596c425662a471714d9d49fad05c1d371"
    acsim.type @acir_wake_next_delta cpp "acir::generated::wake_next_delta" kind "wake" fingerprint "sha256:8cf214054e3ad1f49ca7091e040092971fe7dec32ccfd59554fdef160e889c2a"
    acsim.module @Top interface {ports = [], resources = [], results = []} static [] specialization "sha256:6e2009a7f73501ebb11af899ad6a1f8e80e16424e8ad0ff5b6ab08c5fb19bc2a" exports [] {
      acsim.process @workload captures() names [] entry @entry pcs [@entry, @resume] live [] fairness 8 specialization "sha256:c278be558d87192021d412c469fb2aa37ce1c17c083bc136163ae1dca4ab5588" {
        state @entry {
          %condition = arith.constant true
          %seed = arith.constant 7 : i32
          cf.cond_br %condition, ^then(%seed : i32), ^otherwise(%seed : i32)
        ^then(%value : i32):
          %doubled = arith.addi %value, %value : i32
          %wake = acsim.invoke @acir_impl_wake_condition() : () -> !acsim.wake<@acir_wake_condition>
          acsim.suspend @resume on %wake : !acsim.wake<@acir_wake_condition>
        ^otherwise(%other : i32):
          %other_doubled = arith.addi %other, %other : i32
          %other_wake = acsim.invoke @acir_impl_wake_next_delta() : () -> !acsim.wake<@acir_wake_next_delta>
          acsim.suspend @entry on %other_wake : !acsim.wake<@acir_wake_next_delta>
        }
        state @resume {
          %wake = acsim.invoke @acir_impl_wake_next_delta() : () -> !acsim.wake<@acir_wake_next_delta>
          acsim.suspend @entry on %wake : !acsim.wake<@acir_wake_next_delta>
        }
      }
      acsim.return
    }
    %object, %activation = acsim.dispatch @Top::@workload path "root.workload" indices [] object 0 activation 0
      work "acsim_generated::Top::s6e2009a7f73501ebb11af899ad6a1f8e80e16424e8ad0ff5b6ab08c5fb19bc2a::workload::pc278be558d87192021d412c469fb2aa37ce1c17c083bc136163ae1dca4ab5588::work"
      xfer "acsim_generated::Top::s6e2009a7f73501ebb11af899ad6a1f8e80e16424e8ad0ff5b6ab08c5fb19bc2a::workload::pc278be558d87192021d412c469fb2aa37ce1c17c083bc136163ae1dca4ab5588::xfer"
      reset "acsim_generated::Top::s6e2009a7f73501ebb11af899ad6a1f8e80e16424e8ad0ff5b6ab08c5fb19bc2a::workload::pc278be558d87192021d412c469fb2aa37ce1c17c083bc136163ae1dca4ab5588::reset"
      validate "acsim_generated::Top::s6e2009a7f73501ebb11af899ad6a1f8e80e16424e8ad0ff5b6ab08c5fb19bc2a::workload::pc278be558d87192021d412c469fb2aa37ce1c17c083bc136163ae1dca4ab5588::validate"
      : !acsim.object_id, !acsim.activation_id
    acsim.activate %activation to %object : !acsim.activation_id to !acsim.object_id
  }
}
