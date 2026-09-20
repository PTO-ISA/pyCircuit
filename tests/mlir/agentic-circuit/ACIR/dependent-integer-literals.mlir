// RUN: %acir_opt %s | %FileCheck %s

builtin.module attributes {
  ac.big_positive = #ac.dependent_integer<1208925819614629174706177>,
  ac.big_negative = #ac.dependent_integer<-1208925819614629174706177>,
  ac.big_add = #ac.dependent_add<
    #ac.dependent_value<#ac.dependent_integer<1208925819614629174706177>>,
    #ac.dependent_value<#ac.dependent_integer<1208925819614629174706179>>>,
  ac.big_sub = #ac.dependent_sub<
    #ac.dependent_value<#ac.dependent_integer<-1208925819614629174706177>>,
    #ac.dependent_value<#ac.dependent_integer<1208925819614629174706179>>>,
  ac.big_mul = #ac.dependent_mul<
    #ac.dependent_value<#ac.dependent_integer<1208925819614629174706177>>,
    #ac.dependent_value<#ac.dependent_integer<-1208925819614629174706179>>>
} {
}

// CHECK: ac.big_negative = #ac.dependent_integer<-1208925819614629174706177>
// CHECK-SAME: ac.big_positive = #ac.dependent_integer<1208925819614629174706177>
// CHECK-SAME: ac.big_sub = #ac.dependent_sub<
// CHECK-SAME: #ac.dependent_integer<-1208925819614629174706177>
