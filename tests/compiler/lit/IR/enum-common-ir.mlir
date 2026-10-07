// RUN: %pycircuit_opt %s | %pycircuit_opt | %FileCheck %s
// Native declaration/value verifier coverage only: no hardware-execution claim.
// Decimal code oracles were independently derived with arbitrary-precision
// integer arithmetic. Derived codes are explicitly present and must verify.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w65 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#w130 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
!b4 = !ac.bits<#w4>
!b5 = !ac.bits<#w5>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!same = !ac.enum<"first.protocol.State">
module {
  "ac.enum"() {sym_name = "pkg.Sequential", width = #ac.math_int<3>, encoding = "binary_sequential", members = [{name = "S0", code = #ac.math_int<0>}, {name = "S1", code = #ac.math_int<1>}, {name = "S2", code = #ac.math_int<2>}, {name = "S3", code = #ac.math_int<3>}]} : () -> ()
  "ac.enum"() {sym_name = "pkg.Gray", width = #ac.math_int<3>, encoding = "gray_sequential", members = [{name = "G0", code = #ac.math_int<0>}, {name = "G1", code = #ac.math_int<1>}, {name = "G2", code = #ac.math_int<3>}, {name = "G3", code = #ac.math_int<2>}, {name = "G4", code = #ac.math_int<6>}]} : () -> ()
  "ac.enum"() {sym_name = "pkg.OneHot", width = #ac.math_int<4>, encoding = "binary_one_hot", members = [{name = "H0", code = #ac.math_int<1>}, {name = "H1", code = #ac.math_int<2>}, {name = "H2", code = #ac.math_int<4>}, {name = "H3", code = #ac.math_int<8>}]} : () -> ()
  "ac.enum"() {sym_name = "pkg.Explicit", width = #ac.math_int<5>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "SPARSE", code = #ac.math_int<7>}, {name = "MAX", code = #ac.math_int<31>}]} : () -> ()
  "ac.enum"() {sym_name = "pkg.Singleton", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "ONLY", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "first.protocol.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  "ac.enum"() {sym_name = "second.protocol.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  "ac.enum"() {sym_name = "pkg.Wide65", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "HIGH", code = #ac.math_int<18446744073709551619>}, {name = "MAX", code = #ac.math_int<36893488147419103231>}]} : () -> ()
  "ac.enum"() {sym_name = "pkg.Wide130", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "HIGH", code = #ac.math_int<680564733841876926926749214863536422913>}, {name = "MAX", code = #ac.math_int<1361129467683753853853498429727072845823>}]} : () -> ()
  // Ordinal 129 produces a code above every host 64-bit integer.
  "ac.enum"() {sym_name = "pkg.OneHot130", width = #ac.math_int<130>, encoding = "binary_one_hot", members = [
    {name = "H0", code = #ac.math_int<1>},
    {name = "H1", code = #ac.math_int<2>},
    {name = "H2", code = #ac.math_int<4>},
    {name = "H3", code = #ac.math_int<8>},
    {name = "H4", code = #ac.math_int<16>},
    {name = "H5", code = #ac.math_int<32>},
    {name = "H6", code = #ac.math_int<64>},
    {name = "H7", code = #ac.math_int<128>},
    {name = "H8", code = #ac.math_int<256>},
    {name = "H9", code = #ac.math_int<512>},
    {name = "H10", code = #ac.math_int<1024>},
    {name = "H11", code = #ac.math_int<2048>},
    {name = "H12", code = #ac.math_int<4096>},
    {name = "H13", code = #ac.math_int<8192>},
    {name = "H14", code = #ac.math_int<16384>},
    {name = "H15", code = #ac.math_int<32768>},
    {name = "H16", code = #ac.math_int<65536>},
    {name = "H17", code = #ac.math_int<131072>},
    {name = "H18", code = #ac.math_int<262144>},
    {name = "H19", code = #ac.math_int<524288>},
    {name = "H20", code = #ac.math_int<1048576>},
    {name = "H21", code = #ac.math_int<2097152>},
    {name = "H22", code = #ac.math_int<4194304>},
    {name = "H23", code = #ac.math_int<8388608>},
    {name = "H24", code = #ac.math_int<16777216>},
    {name = "H25", code = #ac.math_int<33554432>},
    {name = "H26", code = #ac.math_int<67108864>},
    {name = "H27", code = #ac.math_int<134217728>},
    {name = "H28", code = #ac.math_int<268435456>},
    {name = "H29", code = #ac.math_int<536870912>},
    {name = "H30", code = #ac.math_int<1073741824>},
    {name = "H31", code = #ac.math_int<2147483648>},
    {name = "H32", code = #ac.math_int<4294967296>},
    {name = "H33", code = #ac.math_int<8589934592>},
    {name = "H34", code = #ac.math_int<17179869184>},
    {name = "H35", code = #ac.math_int<34359738368>},
    {name = "H36", code = #ac.math_int<68719476736>},
    {name = "H37", code = #ac.math_int<137438953472>},
    {name = "H38", code = #ac.math_int<274877906944>},
    {name = "H39", code = #ac.math_int<549755813888>},
    {name = "H40", code = #ac.math_int<1099511627776>},
    {name = "H41", code = #ac.math_int<2199023255552>},
    {name = "H42", code = #ac.math_int<4398046511104>},
    {name = "H43", code = #ac.math_int<8796093022208>},
    {name = "H44", code = #ac.math_int<17592186044416>},
    {name = "H45", code = #ac.math_int<35184372088832>},
    {name = "H46", code = #ac.math_int<70368744177664>},
    {name = "H47", code = #ac.math_int<140737488355328>},
    {name = "H48", code = #ac.math_int<281474976710656>},
    {name = "H49", code = #ac.math_int<562949953421312>},
    {name = "H50", code = #ac.math_int<1125899906842624>},
    {name = "H51", code = #ac.math_int<2251799813685248>},
    {name = "H52", code = #ac.math_int<4503599627370496>},
    {name = "H53", code = #ac.math_int<9007199254740992>},
    {name = "H54", code = #ac.math_int<18014398509481984>},
    {name = "H55", code = #ac.math_int<36028797018963968>},
    {name = "H56", code = #ac.math_int<72057594037927936>},
    {name = "H57", code = #ac.math_int<144115188075855872>},
    {name = "H58", code = #ac.math_int<288230376151711744>},
    {name = "H59", code = #ac.math_int<576460752303423488>},
    {name = "H60", code = #ac.math_int<1152921504606846976>},
    {name = "H61", code = #ac.math_int<2305843009213693952>},
    {name = "H62", code = #ac.math_int<4611686018427387904>},
    {name = "H63", code = #ac.math_int<9223372036854775808>},
    {name = "H64", code = #ac.math_int<18446744073709551616>},
    {name = "H65", code = #ac.math_int<36893488147419103232>},
    {name = "H66", code = #ac.math_int<73786976294838206464>},
    {name = "H67", code = #ac.math_int<147573952589676412928>},
    {name = "H68", code = #ac.math_int<295147905179352825856>},
    {name = "H69", code = #ac.math_int<590295810358705651712>},
    {name = "H70", code = #ac.math_int<1180591620717411303424>},
    {name = "H71", code = #ac.math_int<2361183241434822606848>},
    {name = "H72", code = #ac.math_int<4722366482869645213696>},
    {name = "H73", code = #ac.math_int<9444732965739290427392>},
    {name = "H74", code = #ac.math_int<18889465931478580854784>},
    {name = "H75", code = #ac.math_int<37778931862957161709568>},
    {name = "H76", code = #ac.math_int<75557863725914323419136>},
    {name = "H77", code = #ac.math_int<151115727451828646838272>},
    {name = "H78", code = #ac.math_int<302231454903657293676544>},
    {name = "H79", code = #ac.math_int<604462909807314587353088>},
    {name = "H80", code = #ac.math_int<1208925819614629174706176>},
    {name = "H81", code = #ac.math_int<2417851639229258349412352>},
    {name = "H82", code = #ac.math_int<4835703278458516698824704>},
    {name = "H83", code = #ac.math_int<9671406556917033397649408>},
    {name = "H84", code = #ac.math_int<19342813113834066795298816>},
    {name = "H85", code = #ac.math_int<38685626227668133590597632>},
    {name = "H86", code = #ac.math_int<77371252455336267181195264>},
    {name = "H87", code = #ac.math_int<154742504910672534362390528>},
    {name = "H88", code = #ac.math_int<309485009821345068724781056>},
    {name = "H89", code = #ac.math_int<618970019642690137449562112>},
    {name = "H90", code = #ac.math_int<1237940039285380274899124224>},
    {name = "H91", code = #ac.math_int<2475880078570760549798248448>},
    {name = "H92", code = #ac.math_int<4951760157141521099596496896>},
    {name = "H93", code = #ac.math_int<9903520314283042199192993792>},
    {name = "H94", code = #ac.math_int<19807040628566084398385987584>},
    {name = "H95", code = #ac.math_int<39614081257132168796771975168>},
    {name = "H96", code = #ac.math_int<79228162514264337593543950336>},
    {name = "H97", code = #ac.math_int<158456325028528675187087900672>},
    {name = "H98", code = #ac.math_int<316912650057057350374175801344>},
    {name = "H99", code = #ac.math_int<633825300114114700748351602688>},
    {name = "H100", code = #ac.math_int<1267650600228229401496703205376>},
    {name = "H101", code = #ac.math_int<2535301200456458802993406410752>},
    {name = "H102", code = #ac.math_int<5070602400912917605986812821504>},
    {name = "H103", code = #ac.math_int<10141204801825835211973625643008>},
    {name = "H104", code = #ac.math_int<20282409603651670423947251286016>},
    {name = "H105", code = #ac.math_int<40564819207303340847894502572032>},
    {name = "H106", code = #ac.math_int<81129638414606681695789005144064>},
    {name = "H107", code = #ac.math_int<162259276829213363391578010288128>},
    {name = "H108", code = #ac.math_int<324518553658426726783156020576256>},
    {name = "H109", code = #ac.math_int<649037107316853453566312041152512>},
    {name = "H110", code = #ac.math_int<1298074214633706907132624082305024>},
    {name = "H111", code = #ac.math_int<2596148429267413814265248164610048>},
    {name = "H112", code = #ac.math_int<5192296858534827628530496329220096>},
    {name = "H113", code = #ac.math_int<10384593717069655257060992658440192>},
    {name = "H114", code = #ac.math_int<20769187434139310514121985316880384>},
    {name = "H115", code = #ac.math_int<41538374868278621028243970633760768>},
    {name = "H116", code = #ac.math_int<83076749736557242056487941267521536>},
    {name = "H117", code = #ac.math_int<166153499473114484112975882535043072>},
    {name = "H118", code = #ac.math_int<332306998946228968225951765070086144>},
    {name = "H119", code = #ac.math_int<664613997892457936451903530140172288>},
    {name = "H120", code = #ac.math_int<1329227995784915872903807060280344576>},
    {name = "H121", code = #ac.math_int<2658455991569831745807614120560689152>},
    {name = "H122", code = #ac.math_int<5316911983139663491615228241121378304>},
    {name = "H123", code = #ac.math_int<10633823966279326983230456482242756608>},
    {name = "H124", code = #ac.math_int<21267647932558653966460912964485513216>},
    {name = "H125", code = #ac.math_int<42535295865117307932921825928971026432>},
    {name = "H126", code = #ac.math_int<85070591730234615865843651857942052864>},
    {name = "H127", code = #ac.math_int<170141183460469231731687303715884105728>},
    {name = "H128", code = #ac.math_int<340282366920938463463374607431768211456>},
    {name = "H129", code = #ac.math_int<680564733841876926926749214863536422912>}
  ]} : () -> ()
  %sequential = "ac.enum.create"() {member = "S3"} : () -> !ac.enum<"pkg.Sequential">
  %sequential_bits = "ac.enum.to_bits"(%sequential) : (!ac.enum<"pkg.Sequential">) -> !b3
  %sequential_carrier, %sequential_member = "ac.enum.from_bits"(%sequential_bits) : (!b3) -> (!ac.enum<"pkg.Sequential">, !b1)
  %gray = "ac.enum.create"() {member = "G4"} : () -> !ac.enum<"pkg.Gray">
  %gray_bits = "ac.enum.to_bits"(%gray) : (!ac.enum<"pkg.Gray">) -> !b3
  %gray_carrier, %gray_member = "ac.enum.from_bits"(%gray_bits) : (!b3) -> (!ac.enum<"pkg.Gray">, !b1)
  %onehot = "ac.enum.create"() {member = "H3"} : () -> !ac.enum<"pkg.OneHot">
  %onehot_bits = "ac.enum.to_bits"(%onehot) : (!ac.enum<"pkg.OneHot">) -> !b4
  %onehot_carrier, %onehot_member = "ac.enum.from_bits"(%onehot_bits) : (!b4) -> (!ac.enum<"pkg.OneHot">, !b1)
  %explicit = "ac.enum.create"() {member = "SPARSE"} : () -> !ac.enum<"pkg.Explicit">
  %explicit_bits = "ac.enum.to_bits"(%explicit) : (!ac.enum<"pkg.Explicit">) -> !b5
  %explicit_carrier, %explicit_member = "ac.enum.from_bits"(%explicit_bits) : (!b5) -> (!ac.enum<"pkg.Explicit">, !b1)
  %singleton = "ac.enum.create"() {member = "ONLY"} : () -> !ac.enum<"pkg.Singleton">
  %singleton_bits = "ac.enum.to_bits"(%singleton) : (!ac.enum<"pkg.Singleton">) -> !b1
  %singleton_carrier, %singleton_member = "ac.enum.from_bits"(%singleton_bits) : (!b1) -> (!ac.enum<"pkg.Singleton">, !b1)
  %wide65 = "ac.enum.create"() {member = "HIGH"} : () -> !ac.enum<"pkg.Wide65">
  %wide65_bits = "ac.enum.to_bits"(%wide65) : (!ac.enum<"pkg.Wide65">) -> !b65
  %wide65_carrier, %wide65_member = "ac.enum.from_bits"(%wide65_bits) : (!b65) -> (!ac.enum<"pkg.Wide65">, !b1)
  %wide130 = "ac.enum.create"() {member = "HIGH"} : () -> !ac.enum<"pkg.Wide130">
  %wide130_bits = "ac.enum.to_bits"(%wide130) : (!ac.enum<"pkg.Wide130">) -> !b130
  %wide130_carrier, %wide130_member = "ac.enum.from_bits"(%wide130_bits) : (!b130) -> (!ac.enum<"pkg.Wide130">, !b1)
  %onehot130 = "ac.enum.create"() {member = "H129"} : () -> !ac.enum<"pkg.OneHot130">
  %onehot130_bits = "ac.enum.to_bits"(%onehot130) : (!ac.enum<"pkg.OneHot130">) -> !b130
  %onehot130_carrier, %onehot130_member = "ac.enum.from_bits"(%onehot130_bits) : (!b130) -> (!ac.enum<"pkg.OneHot130">, !b1)
  %first = "ac.enum.create"() {member = "RUN"} : () -> !ac.enum<"first.protocol.State">
  %second = "ac.enum.create"() {member = "RUN"} : () -> !ac.enum<"second.protocol.State">
  %same_bits = "ac.enum.to_bits"(%first) : (!same) -> !b3
  %other_bits = "ac.enum.to_bits"(%second) : (!ac.enum<"second.protocol.State">) -> !b3
  // A known invalid carrier is admitted; membership is a separate result.
  %invalid = "ac.bits.constant"() {value = #w0} : () -> !b4
  %carrier, %membership = "ac.enum.from_bits"(%invalid) : (!b4) -> (!ac.enum<"pkg.OneHot">, !b1)
  %roundtrip = "ac.enum.to_bits"(%carrier) : (!ac.enum<"pkg.OneHot">) -> !b4
}

// CHECK: encoding = "binary_sequential"
// CHECK: encoding = "gray_sequential"
// CHECK: encoding = "binary_one_hot"
// CHECK: encoding = "explicit"
// CHECK: sym_name = "first.protocol.State"
// CHECK: sym_name = "second.protocol.State"
// CHECK: 18446744073709551619
// CHECK: 680564733841876926926749214863536422913
// CHECK: 1361129467683753853853498429727072845823
// CHECK: code = #ac.math_int<680564733841876926926749214863536422912>
// CHECK-SAME: name = "H129"
// CHECK: "ac.enum.create"
// CHECK: "ac.enum.to_bits"
// CHECK: "ac.enum.from_bits"
// CHECK: !ac.enum<"first.protocol.State">
// CHECK: !ac.enum<"second.protocol.State">
// CHECK: "ac.enum.from_bits"
// CHECK: "ac.enum.to_bits"
