// Only the fixture producer consults the canonical registry. Behavioral
// expectations remain in the independent test drivers, not in this catalog.
std::string withNativeCDCImport(std::string text, llvm::StringRef kind) {
  mlir::MLIRContext context;
  context.loadDialect<ac::ACIRDialect, gf::GFIRDialect>();
  auto registry = SourceHeaderRegistry::create(
      &context, {}, [&] { return mlir::emitError(mlir::UnknownLoc::get(&context)); });
  EXPECT_TRUE(mlir::succeeded(registry));
  if (mlir::failed(registry)) return text;
  auto builtin = registry->lookupBuiltin(kind);
  EXPECT_TRUE(builtin);
  if (!builtin) return text;
  const auto package = text.find("module attributes");
  const auto insertion = text.find('\n', package);
  EXPECT_NE(insertion, std::string::npos);
  if (insertion != std::string::npos)
    text.insert(insertion + 1, print(builtin.declaration));
  return text;
}

// Hand-authored System fixture. No source routing and no generated FIFO topology.
std::string fifoSystemIR(bool routed) {
  const auto original = stateIR(false);
  std::string text = original.substr(0, original.find("module attributes"));
  text += R"ir(
#fifo_depth = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<4>}, origin = #occ, location = #span}>
#reader = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#fifo = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}
#rid = {registration = #reader, check = #reader, obligation = 0 : i64}
#wo = {registration = #writer, site = #writer, ordinal = 0 : i64, invocation = [], owner = {instance_path = []}}
#ro = {registration = #reader, site = #reader, ordinal = 0 : i64, invocation = [], owner = {instance_path = []}}
#packet_decl = {site = {definition = @"reset_value.fixture.Packet", ast_path = []}, expansion = []}
!packet = !ac.struct<"reset_value.fixture.Packet">
)ir";
  text += routed
      ? "module attributes {ac.stage = \"routed\", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {\n"
      : "module attributes {ac.stage = \"source\", ac.unit_kind = \"implementation\", ac.source_owner = #owner, ac.interfaces = [#owner], ac.exports = [], ac.import_bindings = []} {\n";
  text += R"ir("ac.struct"() {sym_name = "reset_value.fixture.Packet", fields = [{name = "sequence", type = !b9}, {name = "checksum", type = !b9}], ac.source_owner = #owner, ac.origin = #packet_decl, ac.declaration_role = "definition"} : () -> ()
)ir";
  text += R"ir(
  "ac.module"() ({
  ^bb0(%wclk: !b1, %rclk: !b1, %rst: !b1):
)ir";
  if (routed) text += R"ir(
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b9
    %sequence = "gfir.state"(%wclk, %rst, %zero, %zero) {instance_name = "sequence", occurrence = #occ, domain = "W", operandSegmentSizes = array<i32: 1, 1, 1, 1>, init_kind = "explicit", edge = "posedge", reset_kind = "async", reset_polarity = "high"} : (!b1, !b1, !b9, !b9) -> !b9
    %head, %wa, %ra = "ac.instance"(%wclk, %rclk, %rst, %packet, %wf, %rf) {instance_name = "requests", callee = @"pycircuit.__builtins__.async_fifo", parameters = [#fifo_depth], type_arguments = [!packet], occurrence = #fifo, write_domain = "W", read_domain = "R", write_edge = "posedge", read_edge = "posedge", reset_polarity = "high", resource_contract = 1 : i64} : (!b1, !b1, !b1, !packet, !b1, !b1) -> (!packet, !b1, !b1)
    %checksum = "ac.bits.unary"(%sequence) {opcode = "not"} : (!b9) -> !b9
    %packet = "ac.struct.create"(%sequence, %checksum) : (!b9, !b9) -> !packet
    %next, %wen, %wf, %wp, %wv, %wc, %wr, %wg, %wreach = "ac.rule"(%wclk, %rst, %sequence, %wa) ({
    ^bb0(%w_clock: !b1, %w_reset: !b1, %w_old: !b9, %w_available: !b1):
      %w_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %w_step = "ac.bits.constant"() {value = #w1} : () -> !b9
      %w_next = "ac.bits.binary"(%w_old, %w_step) {opcode = "add"} : (!b9, !b9) -> !b9
      %w_guard = "ac.bits.compare"(%w_one, %w_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %w_fire = "ac.bits.binary"(%w_guard, %w_available) {opcode = "and"} : (!b1, !b1) -> !b1
      %w_enabled = "ac.bits.binary"(%w_fire, %w_one) {opcode = "and"} : (!b1, !b1) -> !b1
      "ac.yield"(%w_next, %w_enabled, %w_fire, %w_fire, %w_old, %w_clock, %w_reset, %w_one, %w_one) : (!b9, !b1, !b1, !b1, !b9, !b1, !b1, !b1, !b1) -> ()
    }) {name = "push", occurrence = #writer, ac.required_checks = [{id = #id, kind = "guard_known", location = #span}], ac.sampled_admission = {clock_capture = 0 : i64, edge = "posedge", resets = [{capture = 1 : i64, kind = "async", polarity = "high"}], fire_result = 2 : i64, controls = [{kind = "callable_guard", id = #id, invocation = [], predecessor = -1 : i64, path = {}}], body_checks = [], observations = [{id = #wo, value_count = 3 : i64, path = {}}]}} : (!b1, !b1, !b9, !b1) -> (!b9, !b1, !b1, !b1, !b9, !b1, !b1, !b1, !b1)
    %rf, %rp, %rv, %rtag, %rc, %rr, %rg, %rreach = "ac.rule"(%rclk, %rst, %head, %ra) ({
    ^bb0(%r_clock: !b1, %r_reset: !b1, %r_read_packet: !packet, %r_available: !b1):
      %r_value = "ac.struct.get"(%r_read_packet) {field = "sequence"} : (!packet) -> !b9
      %r_checksum = "ac.struct.get"(%r_read_packet) {field = "checksum"} : (!packet) -> !b9
      %r_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %r_guard = "ac.bits.compare"(%r_one, %r_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %r_fire = "ac.bits.binary"(%r_guard, %r_available) {opcode = "and"} : (!b1, !b1) -> !b1
      "ac.yield"(%r_fire, %r_fire, %r_value, %r_checksum, %r_clock, %r_reset, %r_one, %r_one) : (!b1, !b1, !b9, !b9, !b1, !b1, !b1, !b1) -> ()
    }) {name = "pop", occurrence = #reader, ac.required_checks = [{id = #rid, kind = "guard_known", location = #span}], ac.sampled_admission = {clock_capture = 0 : i64, edge = "posedge", resets = [{capture = 1 : i64, kind = "async", polarity = "high"}], fire_result = 0 : i64, controls = [{kind = "callable_guard", id = #rid, invocation = [], predecessor = -1 : i64, path = {}}], body_checks = [], observations = [{id = #ro, value_count = 4 : i64, path = {}}]}} : (!b1, !b1, !packet, !b1) -> (!b1, !b1, !b9, !b9, !b1, !b1, !b1, !b1)
    "gfir.state.update"(%sequence, %next, %wen) : (!b9, !b9, !b1) -> ()
    "ac.expect"(%wg, %wreach, %wclk, %rst) {kind = "guard_known", location = #span, ac.check_id = #id} : (!b1, !b1, !b1, !b1) -> ()
    "ac.expect"(%rg, %rreach, %rclk, %rst) {kind = "guard_known", location = #span, ac.check_id = #rid} : (!b1, !b1, !b1, !b1) -> ()
    "ac.observe"(%wp, %wv, %wc, %wr) {kind = "log", spec = {level = "info", event = "fifo push", items = [{kind = "value", ordinal = 0 : i32}, {kind = "value", ordinal = 1 : i32}, {kind = "value", ordinal = 2 : i32}]}, ac.location = #span, ac.observation_id = #wo} : (!b1, !b9, !b1, !b1) -> ()
    "ac.observe"(%rp, %rv, %rtag, %rc, %rr) {kind = "log", spec = {level = "info", event = "fifo pop", items = [{kind = "value", ordinal = 0 : i32}, {kind = "value", ordinal = 1 : i32}, {kind = "value", ordinal = 2 : i32}, {kind = "value", ordinal = 3 : i32}]}, ac.location = #span, ac.observation_id = #ro} : (!b1, !b9, !b9, !b1, !b1) -> ()
)ir";
  text += R"ir(
    "ac.yield"() : () -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> (), input_names = ["write_clock", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.origin = #occ, ac.declaration_role = "definition", ac.root_kind = "system"
)ir";
  if (routed)
    text += R"ir(, ac.domain_params = ["W", "R"], ac.domain_clock_inputs = {W = 0 : i32, R = 1 : i32}, ac.transaction_wiring = {states = [#occ], incoming = [], writers = [{registration = #writer, ordinary_results = 0 : i64, candidates = [{state = #state_id, data = 0 : i64, enable = 1 : i64}]}, {registration = #reader, ordinary_results = 0 : i64, candidates = []}]})ir";
  text += ", ac.system_samples = {inputs = [0 : i32, 1 : i32, 2 : i32], rows = [\"001\", \"000\"";
  // Release, write-only fill, read-only drain, sustained wrap, then reset while
  // the read clock is stopped, and resume both domains. All rows are explicit.
  for (unsigned phase = 0; phase < 7; ++phase) {
    if (phase == 4) text += ", \"001\", \"000\"";
    for (unsigned cycle = 0; cycle < (phase == 3 ? 32u : 12u); ++cycle) {
      text += ", \"000\", \"";
      text += (phase == 1 || phase == 4) ? "100"
              : (phase == 2 || phase == 6) ? "010" : "110";
      text += "\"";
    }
  }
  text += "], origin = #occ, location = #span}} : () -> ()\n";
  if (routed)
    text += "\"ac.system\"() {entry = {callee = @\"reset_value.fixture.Held\", parameters = [], type_arguments = []}, domain = \"default\"} : () -> ()\n";
  text += "}\n";
  return routed ? withNativeCDCImport(text, "async_fifo") : text;
}

TEST_F(StateResetValueTest, PublishFIFOSystemForIndependentScoreboard) {
  publishSaved(fifoSystemIR(false), fifoSystemIR(true),
               "PYCIRCUIT_TEST_FIFO_ANCHOR", true);
}

TEST_F(StateResetValueTest, HierarchicalDomainBindingsUseActualClockIdentity) {
  const auto original = stateIR(false);
  std::string text = original.substr(0, original.find("module attributes"));
  text += R"ir(
#child = {site = {definition = @"reset_value.fixture.Child", ast_path = []}, expansion = []}
#left = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}
#right = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}
module attributes {ac.stage = "routed", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1):
    %five = "ac.bits.constant"() {value = #w5} : () -> !b9
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b9
    %false = "ac.bits.constant"() {value = #w0} : () -> !b1
    %q = "gfir.state"(%clk, %rst, %five, %zero) {instance_name = "local", occurrence = #child, domain = "D", operandSegmentSizes = array<i32: 1, 1, 1, 1>, init_kind = "explicit", edge = "posedge", reset_kind = "sync", reset_polarity = "high"} : (!b1, !b1, !b9, !b9) -> !b9
    "gfir.state.update"(%q, %q, %false) : (!b9, !b9, !b1) -> ()
    "ac.yield"(%q) : (!b9) -> ()
  }) {sym_name = "reset_value.fixture.Child", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1) -> !b9, input_names = ["pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 0 : i64, reset = 1 : i64}, ac.domain_params = ["D"], ac.default_domain = "D", ac.transaction_wiring = {states = [#child], incoming = [], writers = []}} : () -> ()
  "ac.module"() ({
  ^bb0(%wclk: !b1, %rclk: !b1, %rst: !b1):
    %left = "ac.instance"(%wclk, %rst) {instance_name = "left", callee = @"reset_value.fixture.Child", parameters = [], type_arguments = [], occurrence = #left, ac.domain_bindings = {D = "W"}} : (!b1, !b1) -> !b9
    %right = "ac.instance"(%rclk, %rst) {instance_name = "right", callee = @"reset_value.fixture.Child", parameters = [], type_arguments = [], occurrence = #right, ac.domain_bindings = {D = "R"}} : (!b1, !b1) -> !b9
    "ac.yield"() : () -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> (), input_names = ["write_clock", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.domain_params = ["W", "R"], ac.domain_clock_inputs = {W = 0 : i32, R = 1 : i32}, ac.root_kind = "system"} : () -> ()
  "ac.system"() {entry = {callee = @"reset_value.fixture.Held", parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir";
  auto package = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
  ASSERT_TRUE(package);
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*package)));
  ac::InstanceOp left, right;
  package->walk([&](ac::InstanceOp instance) {
    if (instance.getInstanceName() == "left") left = instance;
    if (instance.getInstanceName() == "right") right = instance;
  });
  ASSERT_TRUE(left); ASSERT_TRUE(right);
  // A matching local name is not sufficient: break the clock SSA while keeping
  // the logical binding unchanged. This is one genuine identity rejection.
  const auto originalClock = left->getOperand(0);
  left->setOperand(0, right->getOperand(0));
  {
    mlir::ScopedDiagnosticHandler expectedFailure(
        &context, [](mlir::Diagnostic &) { return mlir::success(); });
    EXPECT_TRUE(mlir::failed(ac::verifyRoutedPackage(*package)));
  }
  left->setOperand(0, originalClock);
  EXPECT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*package)));
}

std::string asyncRegSystemIR(bool routed) {
  const auto original = stateIR(false);
  std::string text = original.substr(0, original.find("module attributes"));
  text += R"ir(
#sync = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 8 : i64}]}, expansion = []}
#observation = {registration = #writer, site = #writer, ordinal = 0 : i64, invocation = [], owner = {instance_path = []}}
)ir";
  text += routed
      ? "module attributes {ac.stage = \"routed\", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {\n"
      : "module attributes {ac.stage = \"source\", ac.unit_kind = \"implementation\", ac.source_owner = #owner, ac.interfaces = [#owner], ac.exports = [], ac.import_bindings = []} {\n";
  text += R"ir(
  "ac.module"() ({
  ^bb0(%input: !b1, %wclk: !b1, %rclk: !b1, %rst: !b1):
)ir";
  if (routed) text += R"ir(
    %synced = "ac.instance"(%wclk, %rclk, %rst, %input) {instance_name = "level", callee = @"pycircuit.__builtins__.async_reg", parameters = [], type_arguments = [], occurrence = #sync, source_domain = "W", target_domain = "R", target_edge = "posedge", reset_polarity = "high", resource_contract = 1 : i64} : (!b1, !b1, !b1, !b1) -> !b1
    %fire, %path, %value, %clock, %reset, %guard, %reached = "ac.rule"(%rclk, %rst, %synced) ({
    ^bb0(%clock: !b1, %reset: !b1, %value: !b1):
      %one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %fire = "ac.bits.compare"(%one, %one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      "ac.yield"(%fire, %fire, %value, %clock, %reset, %one, %one) : (!b1, !b1, !b1, !b1, !b1, !b1, !b1) -> ()
    }) {name = "observe_level", occurrence = #writer, ac.required_checks = [{id = #id, kind = "guard_known", location = #span}], ac.sampled_admission = {clock_capture = 0 : i64, edge = "posedge", resets = [{capture = 1 : i64, kind = "async", polarity = "high"}], fire_result = 0 : i64, controls = [{kind = "callable_guard", id = #id, invocation = [], predecessor = -1 : i64, path = {}}], body_checks = [], observations = [{id = #observation, value_count = 3 : i64, path = {}}]}} : (!b1, !b1, !b1) -> (!b1, !b1, !b1, !b1, !b1, !b1, !b1)
    "ac.expect"(%guard, %reached, %rclk, %rst) {kind = "guard_known", location = #span, ac.check_id = #id} : (!b1, !b1, !b1, !b1) -> ()
    "ac.observe"(%path, %value, %clock, %reset) {kind = "log", spec = {level = "info", event = "async level", items = [{kind = "value", ordinal = 0 : i32}, {kind = "value", ordinal = 1 : i32}, {kind = "value", ordinal = 2 : i32}]}, ac.location = #span, ac.observation_id = #observation} : (!b1, !b1, !b1, !b1) -> ()
)ir";
  text += R"ir(
    "ac.yield"() : () -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1) -> (), input_names = ["source_bit", "write_clock", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 2 : i64, reset = 3 : i64}, ac.origin = #occ, ac.declaration_role = "definition", ac.root_kind = "system"
)ir";
  if (routed)
    text += R"ir(, ac.domain_params = ["W", "R"], ac.default_domain = "W", ac.domain_clock_inputs = {W = 1 : i32, R = 2 : i32}, ac.transaction_wiring = {states = [], incoming = [], writers = [{registration = #writer, ordinary_results = 0 : i64, candidates = []}]})ir";
  text += ", ac.system_samples = {inputs = [0 : i32, 1 : i32, 2 : i32, 3 : i32], rows = [\"0001\", \"0000\"";
  // The source clock never advances. The destination alone releases reset and
  // eventually sees a stable level; a pulse between its edges may disappear.
  for (unsigned phase = 0; phase < 4; ++phase) {
    if (phase == 2) text += ", \"0001\", \"0000\"";
    if (phase == 3) text += ", \"1000\", \"0000\"";
    for (unsigned cycle = 0; cycle < 10; ++cycle)
      text += phase == 1 ? ", \"1000\", \"1010\""
                         : ", \"0000\", \"0010\"";
  }
  text += "], origin = #occ, location = #span}} : () -> ()\n";
  if (routed)
    text += "\"ac.system\"() {entry = {callee = @\"reset_value.fixture.Held\", parameters = [], type_arguments = []}, domain = \"default\"} : () -> ()\n";
  text += "}\n";
  return routed ? withNativeCDCImport(text, "async_reg") : text;
}

TEST_F(StateResetValueTest, PublishAsyncRegSystemForIndependentTrace) {
  publishSaved(asyncRegSystemIR(false), asyncRegSystemIR(true),
               "PYCIRCUIT_TEST_ASYNC_REG_ANCHOR");
}

std::string physicalTwoDomainIR(bool routed) {
  auto replaceAll = [](std::string &text, const std::string &from,
                       const std::string &to) {
    size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
      text.replace(position, from.size(), to);
      position += to.size();
    }
  };
  const auto original = stateIR(false);
  std::string text = original.substr(0, original.find("module attributes"));
  std::string children;
  for (bool failing : {false, true}) {
    const std::string prefix = failing ? "slow" : "fast";
    const std::string symbol = failing ? "Slow" : "Fast";
    auto child = routed ? asyncNegativeStateIR() : stateIR(false);
    replaceAll(child, "reset_value.fixture.Held", "reset_value.fixture." + symbol);
    for (const std::string alias : {"#state_id", "#writer", "#occ", "#id"})
      replaceAll(child, alias, "#" + prefix + alias.substr(1));
    // The aliases retain definition-local identities, shared by source and
    // anchor. Only logical child results are used as testbench witnesses.
    const auto aliasesBegin = child.find("#" + prefix + "occ =");
    const auto aliasesEnd = child.find("#span =");
    text += child.substr(aliasesBegin, aliasesEnd - aliasesBegin);
    const auto begin = child.find("  \"ac.module\"");
    auto end = child.find("  \"ac.system\"", begin);
    if (end == std::string::npos) end = child.rfind("}\n");
    child = child.substr(begin, end - begin);
    replaceAll(child, "%enable: !b1, ", "");
    replaceAll(child, "function_type = (!b1, !b1, !b1)", "function_type = (!b1, !b1)");
    replaceAll(child, "input_names = [\"enable\", \"pyc_clk\", \"pyc_rst\"]",
               "input_names = [\"pyc_clk\", \"pyc_rst\"]");
    replaceAll(child, "clock = 1 : i64, reset = 2 : i64", "clock = 0 : i64, reset = 1 : i64");
    replaceAll(child, ", ac.domain_clock_inputs = {D = 1 : i32}", "");
    replaceAll(child, "negedge", "posedge");
    replaceAll(child, "\"async\"", "\"sync\"");
    replaceAll(child, "\"low\"", "\"high\"");
    if (routed) {
      std::string guard = "    %enable = \"ac.bits.constant\"() {value = #w1} : () -> !b1\n";
      if (failing) {
        text += "#poison = {site = {definition = @\"reset_value.fixture.Slow\", ast_path = [{kind = \"index\", value = 2 : i64}]}, expansion = []}\n";
        guard = R"ir(
    %zero_bit = "ac.bits.constant"() {value = #w0} : () -> !b1
    %one_bit = "ac.bits.constant"() {value = #w1} : () -> !b1
    %step_bit = "ac.bits.constant"() {value = #w1} : () -> !b9
    %six = "ac.bits.binary"(%initial, %step_bit) {opcode = "add"} : (!b9, !b9) -> !b9
    %seven = "ac.bits.binary"(%six, %step_bit) {opcode = "add"} : (!b9, !b9) -> !b9
    %at_seven = "ac.bits.compare"(%q, %seven) {predicate = "case_eq"} : (!b9, !b9) -> !b1
    %unknown = "gfir.state"(%clk) {instance_name = "poison", occurrence = #poison, domain = "D", operandSegmentSizes = array<i32: 1, 0, 0, 0>, init_kind = "unspecified", edge = "posedge", reset_kind = "none"} : (!b1) -> !b1
    "gfir.state.update"(%unknown, %unknown, %zero_bit) : (!b1, !b1, !b1) -> ()
    %enable = "ac.bits.select"(%at_seven, %unknown, %one_bit) : (!b1, !b1, !b1) -> !b1
)ir";
        replaceAll(child, "states = [#slowocc]", "states = [#slowocc, #poison]");
      }
      replaceAll(child, "    %next, %enabled", guard + "    %next, %enabled");
    }
    children += child;
  }
  text += R"ir(
#left = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}
#right = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}
)ir";
  text += routed
      ? "module attributes {ac.stage = \"routed\", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {\n"
      : "module attributes {ac.stage = \"source\", ac.unit_kind = \"implementation\", ac.source_owner = #owner, ac.interfaces = [#owner], ac.exports = [], ac.import_bindings = []} {\n";
  text += children;
  text += R"ir(
  "ac.module"() ({
  ^bb0(%wclk: !b1, %rclk: !b1, %rst: !b1):
    %fast = "ac.instance"(%wclk, %rst) {instance_name = "fast", callee = @"reset_value.fixture.Fast", parameters = [], type_arguments = [], occurrence = #left
)ir";
  if (routed) text += ", ac.domain_bindings = {D = \"W\"}";
  text += R"ir(} : (!b1, !b1) -> !b9
    %slow = "ac.instance"(%rclk, %rst) {instance_name = "slow", callee = @"reset_value.fixture.Slow", parameters = [], type_arguments = [], occurrence = #right
)ir";
  if (routed) text += ", ac.domain_bindings = {D = \"R\"}";
  text += R"ir(} : (!b1, !b1) -> !b9
    "ac.yield"() : () -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> (), input_names = ["write_clock", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.origin = #occ, ac.declaration_role = "definition"
)ir";
  if (routed)
    text += R"ir(, ac.domain_params = ["W", "R"], ac.default_domain = "R", ac.domain_clock_inputs = {W = 0 : i32, R = 1 : i32})ir";
  text += "} : () -> ()\n";
  if (routed)
    text += "\"ac.system\"() {entry = {callee = @\"reset_value.fixture.Held\", parameters = [], type_arguments = []}, domain = \"default\"} : () -> ()\n";
  return text + "}\n";
}

TEST_F(StateResetValueTest, PublishTwoDomainPhysicalBarrierForIndependentTrace) {
  publishSaved(physicalTwoDomainIR(false), physicalTwoDomainIR(true),
               "PYCIRCUIT_TEST_PHYSICAL_BARRIER_ANCHOR");
}

TEST_F(StateResetValueTest, ControlRolesDoNotEraseOrdinaryDataDomains) {
  const auto original = stateIR(false);
  const std::string aliases = original.substr(0, original.find("module attributes"));
  auto direct = mlir::parseSourceString<mlir::ModuleOp>(aliases + R"ir(
#slow = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 9 : i64}]}, expansion = []}
module attributes {ac.stage = "routed", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {
  "ac.module"() ({
  ^bb0(%fast_clock: !b1, %slow_clock: !b1, %rst: !b1):
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b1
    %one = "ac.bits.constant"() {value = #w1} : () -> !b1
    %fast = "gfir.state"(%fast_clock, %one) {instance_name = "fast", occurrence = #occ, domain = "W", operandSegmentSizes = array<i32: 1, 0, 1, 0>, init_kind = "explicit", edge = "posedge", reset_kind = "none"} : (!b1, !b1) -> !b1
    %slow = "gfir.state"(%slow_clock, %fast, %zero, %zero) {instance_name = "slow", occurrence = #slow, domain = "R", operandSegmentSizes = array<i32: 1, 1, 1, 1>, init_kind = "explicit", edge = "posedge", reset_kind = "sync", reset_polarity = "high"} : (!b1, !b1, !b1, !b1) -> !b1
    "gfir.state.update"(%fast, %fast, %zero) : (!b1, !b1, !b1) -> ()
    "gfir.state.update"(%slow, %zero, %one) : (!b1, !b1, !b1) -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> (), input_names = ["fast_clock", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.domain_params = ["W", "R"], ac.default_domain = "R", ac.domain_clock_inputs = {W = 0 : i32, R = 1 : i32}} : () -> ()
  "ac.system"() {entry = {callee = @"reset_value.fixture.Held", parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

)ir", &context);
  ASSERT_TRUE(direct);
  ASSERT_TRUE(mlir::succeeded(ac::verifyHardwareDomains(*direct)));
  gf::StateOp fast;
  gf::StateUpdateOp slowUpdate;
  direct->walk([&](gf::StateOp state) {
    if (state.getInstanceName() == "fast") fast = state;
  });
  direct->walk([&](gf::StateUpdateOp update) {
    if (update->getOperand(0).getDefiningOp<gf::StateOp>().getInstanceName() == "slow")
      slowUpdate = update;
  });
  ASSERT_TRUE(fast); ASSERT_TRUE(slowUpdate);
  // The same fast Q already drives the slow reset. Reusing it as ordinary
  // candidate data must retain its fast-domain dependency.
  slowUpdate->setOperand(1, fast->getResult(0));
  {
    mlir::ScopedDiagnosticHandler expectedFailure(
        &context, [](mlir::Diagnostic &) { return mlir::success(); });
    EXPECT_TRUE(mlir::failed(ac::verifyHardwareDomains(*direct)));
  }
  auto wrapper = mlir::parseSourceString<mlir::ModuleOp>(aliases + R"ir(
#child = {site = {definition = @"reset_value.fixture.Child", ast_path = []}, expansion = []}
module attributes {ac.stage = "routed", ac.routed_contract = 7 : i64, ac.source_units = [#owner]} {
  "ac.module"() ({
  ^bb0(%data: !b1, %clk: !b1, %rst: !b1):
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b1
    %one = "ac.bits.constant"() {value = #w1} : () -> !b1
    %q = "gfir.state"(%clk, %zero) {instance_name = "q", occurrence = #child, domain = "D", operandSegmentSizes = array<i32: 1, 0, 1, 0>, init_kind = "explicit", edge = "posedge", reset_kind = "none"} : (!b1, !b1) -> !b1
    "gfir.state.update"(%q, %data, %one) : (!b1, !b1, !b1) -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "reset_value.fixture.Child", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> (), input_names = ["data", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.domain_params = ["D"]} : () -> ()
  "ac.module"() ({
  ^bb0(%data: !b1, %slow_clock: !b1, %fast_clock: !b1, %rst: !b1):
    "ac.instance"(%data, %slow_clock, %rst) {instance_name = "child", callee = @"reset_value.fixture.Child", parameters = [], type_arguments = [], occurrence = #occ, ac.domain_bindings = {D = "R"}} : (!b1, !b1, !b1) -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1) -> (), input_names = ["data", "slow_clock", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 2 : i64, reset = 3 : i64}, ac.domain_params = ["W", "R"], ac.default_domain = "W", ac.domain_clock_inputs = {W = 2 : i32, R = 1 : i32}} : () -> ()
  "ac.system"() {entry = {callee = @"reset_value.fixture.Held", parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
)ir", &context);
  ASSERT_TRUE(wrapper);
  {
    mlir::ScopedDiagnosticHandler expectedFailure(
        &context, [](mlir::Diagnostic &) { return mlir::success(); });
    EXPECT_TRUE(mlir::failed(ac::verifyHardwareDomains(*wrapper)));
  }
}

std::string genericFIFOSystemIR(bool routed, bool mixed = false) {
  auto replace = [](std::string &text, const std::string &from, const std::string &to) {
    const auto position = text.find(from);
    EXPECT_NE(position, std::string::npos);
    if (position != std::string::npos) text.replace(position, from.size(), to);
  };
  auto replaceAll = [&](std::string &text, const std::string &from, const std::string &to) {
    size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
      text.replace(position, from.size(), to); position += to.size();
    }
  };
  auto base = fifoSystemIR(routed);
  const auto module = base.find("  \"ac.module\"");
  const auto sampleStart = base.find(", ac.system_samples");
  const auto sampleEnd = base.find("} : () -> ()", sampleStart);
  std::string samples = base.substr(sampleStart, sampleEnd - sampleStart);
  replaceAll(samples, "origin = #occ", "origin = #rootocc");
  std::string prefix = base.substr(0, module);
  replaceAll(prefix, "reset_value.fixture.Held", "reset_value.fixture.GenericFIFO");
  const auto packageStart = prefix.find("module attributes");
  prefix.insert(packageStart, R"ir(
!payload = !ac.type_param<@"reset_value.fixture.GenericFIFO", "T">
#rootocc = {site = {definition = @"reset_value.fixture.Held", ast_path = []}, expansion = []}
#child_instance = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#parent_reader = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#parent_check = {registration = #parent_reader, check = #parent_reader, obligation = 0 : i64}
#parent_observation = {registration = #parent_reader, site = #parent_reader, ordinal = 0 : i64, invocation = [], owner = {instance_path = []}}
)ir");
  if (mixed) prefix.insert(prefix.find("module attributes"), R"ir(
#scalar_instance = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}
#scalar_reader = {site = {definition = @"reset_value.fixture.Held", ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}
#scalar_check = {registration = #scalar_reader, check = #scalar_reader, obligation = 0 : i64}
#scalar_observation = {registration = #scalar_reader, site = #scalar_reader, ordinal = 0 : i64, invocation = [], owner = {instance_path = []}}
)ir");
  std::string child = base.substr(module, sampleStart - module) + "} : () -> ()\n";
  replaceAll(child, "reset_value.fixture.Held", "reset_value.fixture.GenericFIFO");
  replace(child, "type_parameters = []", "type_parameters = [\"T\"]");
  replace(child, "function_type = (!b1, !b1, !b1) -> ()",
          "function_type = (!b1, !b1, !b1) -> (!payload, !b1)");
  replace(child, "output_names = []", "output_names = [\"head\", \"read_available\"]");
  replace(child, ", ac.root_kind = \"system\"", "");
  if (routed) {
    replace(child, ", ac.domain_clock_inputs = {W = 0 : i32, R = 1 : i32}",
            ", ac.default_domain = \"R\"");
    replaceAll(child, "!packet", "!payload");
    const auto packetStart = child.find("    %checksum =");
    const auto packetEnd = child.find("    %next, %wen", packetStart);
    child.replace(packetStart, packetEnd - packetStart,
                  "    %packet = \"ac.value.zero\"() : () -> !payload\n");
    const auto popStart = child.find("    %rf, %rp");
    const auto popEnd = child.find("    \"gfir.state.update\"", popStart);
    child.replace(popStart, popEnd - popStart, R"ir(
    %rf, %rg, %rreach = "ac.rule"(%rclk, %rst, %ra) ({
    ^bb0(%gr_clock: !b1, %gr_reset: !b1, %gr_available: !b1):
      %gr_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %gr_guard = "ac.bits.compare"(%gr_one, %gr_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      %gr_fire = "ac.bits.binary"(%gr_guard, %gr_available) {opcode = "and"} : (!b1, !b1) -> !b1
      "ac.yield"(%gr_fire, %gr_one, %gr_one) : (!b1, !b1, !b1) -> ()
    }) {name = "pop", occurrence = #reader, ac.required_checks = [{id = #rid, kind = "guard_known", location = #span}], ac.sampled_admission = {clock_capture = 0 : i64, edge = "posedge", resets = [{capture = 1 : i64, kind = "async", polarity = "high"}], fire_result = 0 : i64, controls = [{kind = "callable_guard", id = #rid, invocation = [], predecessor = -1 : i64, path = {}}], body_checks = [], observations = []}} : (!b1, !b1, !b1) -> (!b1, !b1, !b1)
)ir");
    const auto popObserve = child.find("    \"ac.observe\"(%rp");
    child.erase(popObserve, child.find('\n', popObserve) - popObserve + 1);
    replace(child, "    \"ac.yield\"() : () -> ()",
            "    \"ac.yield\"(%head, %ra) : (!payload, !b1) -> ()");
  } else {
    replace(child, "    \"ac.yield\"() : () -> ()", R"ir(
    %head = "ac.value.zero"() : () -> !payload
    %ra = "ac.bits.constant"() {value = #w0} : () -> !b1
    "ac.yield"(%head, %ra) : (!payload, !b1) -> ()
)ir");
  }
  std::string text = prefix + child + R"ir(
  "ac.module"() ({
  ^bb0(%wclk: !b1, %rclk: !b1, %rst: !b1):
    %head, %available = "ac.instance"(%wclk, %rclk, %rst) {instance_name = "generic_fifo", callee = @"reset_value.fixture.GenericFIFO", parameters = [], type_arguments = [!packet], occurrence = #child_instance
)ir";
  if (routed) text += ", ac.domain_bindings = {W = \"W\", R = \"R\"}";
  text += "} : (!b1, !b1, !b1) -> (!packet, !b1)\n";
  if (mixed) {
    text += R"ir(
    %scalar_head, %scalar_available = "ac.instance"(%wclk, %rclk, %rst) {instance_name = "scalar_fifo", callee = @"reset_value.fixture.GenericFIFO", parameters = [], type_arguments = [!b9], occurrence = #scalar_instance
)ir";
    if (routed) text += R"ir(, ac.domain_bindings = {W = "W", R = "R"})ir";
    text += "} : (!b1, !b1, !b1) -> (!b9, !b1)\n";
  }
  if (routed) text += R"ir(
    %fire, %path, %seq, %tag, %clock, %reset, %guard, %reached = "ac.rule"(%rclk, %rst, %head, %available) ({
    ^bb0(%o_clock: !b1, %o_reset: !b1, %o_packet: !packet, %o_available: !b1):
      %o_value = "ac.struct.get"(%o_packet) {field = "sequence"} : (!packet) -> !b9
      %o_checksum = "ac.struct.get"(%o_packet) {field = "checksum"} : (!packet) -> !b9
      %o_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %o_fire = "ac.bits.compare"(%o_available, %o_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      "ac.yield"(%o_fire, %o_fire, %o_value, %o_checksum, %o_clock, %o_reset, %o_available, %o_one) : (!b1, !b1, !b9, !b9, !b1, !b1, !b1, !b1) -> ()
    }) {name = "observe_pop", occurrence = #parent_reader, ac.required_checks = [{id = #parent_check, kind = "guard_known", location = #span}], ac.sampled_admission = {clock_capture = 0 : i64, edge = "posedge", resets = [{capture = 1 : i64, kind = "async", polarity = "high"}], fire_result = 0 : i64, controls = [{kind = "callable_guard", id = #parent_check, invocation = [], predecessor = -1 : i64, path = {}}], body_checks = [], observations = [{id = #parent_observation, value_count = 4 : i64, path = {}}]}} : (!b1, !b1, !packet, !b1) -> (!b1, !b1, !b9, !b9, !b1, !b1, !b1, !b1)
    "ac.expect"(%guard, %reached, %rclk, %rst) {kind = "guard_known", location = #span, ac.check_id = #parent_check} : (!b1, !b1, !b1, !b1) -> ()
    "ac.observe"(%path, %seq, %tag, %clock, %reset) {kind = "log", spec = {level = "info", event = "fifo pop", items = [{kind = "value", ordinal = 0 : i32}, {kind = "value", ordinal = 1 : i32}, {kind = "value", ordinal = 2 : i32}, {kind = "value", ordinal = 3 : i32}]}, ac.location = #span, ac.observation_id = #parent_observation} : (!b1, !b9, !b9, !b1, !b1) -> ()
)ir";
  if (routed && mixed) text += R"ir(
    %sf, %sp, %sv, %sc, %sr, %sg, %sreach = "ac.rule"(%rclk, %rst, %scalar_head, %scalar_available) ({
    ^bb0(%s_clock: !b1, %s_reset: !b1, %s_value: !b9, %s_available: !b1):
      %s_one = "ac.bits.constant"() {value = #w1} : () -> !b1
      %s_fire = "ac.bits.compare"(%s_available, %s_one) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      "ac.yield"(%s_fire, %s_fire, %s_value, %s_clock, %s_reset, %s_available, %s_one) : (!b1, !b1, !b9, !b1, !b1, !b1, !b1) -> ()
    }) {name = "observe_scalar_pop", occurrence = #scalar_reader, ac.required_checks = [{id = #scalar_check, kind = "guard_known", location = #span}], ac.sampled_admission = {clock_capture = 0 : i64, edge = "posedge", resets = [{capture = 1 : i64, kind = "async", polarity = "high"}], fire_result = 0 : i64, controls = [{kind = "callable_guard", id = #scalar_check, invocation = [], predecessor = -1 : i64, path = {}}], body_checks = [], observations = [{id = #scalar_observation, value_count = 3 : i64, path = {}}]}} : (!b1, !b1, !b9, !b1) -> (!b1, !b1, !b9, !b1, !b1, !b1, !b1)
    "ac.expect"(%sg, %sreach, %rclk, %rst) {kind = "guard_known", location = #span, ac.check_id = #scalar_check} : (!b1, !b1, !b1, !b1) -> ()
    "ac.observe"(%sp, %sv, %sc, %sr) {kind = "log", spec = {level = "info", event = "scalar fifo pop", items = [{kind = "value", ordinal = 0 : i32}, {kind = "value", ordinal = 1 : i32}, {kind = "value", ordinal = 2 : i32}]}, ac.location = #span, ac.observation_id = #scalar_observation} : (!b1, !b9, !b1, !b1) -> ()
)ir";
  text += R"ir(
    "ac.yield"() : () -> ()
  }) {sym_name = "reset_value.fixture.Held", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> (), input_names = ["write_clock", "pyc_clk", "pyc_rst"], output_names = [], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.origin = #rootocc, ac.declaration_role = "definition", ac.root_kind = "system"
)ir";
  if (routed)
    text += R"ir(, ac.domain_params = ["W", "R"], ac.domain_clock_inputs = {W = 0 : i32, R = 1 : i32}, ac.transaction_wiring = {states = [], incoming = [], writers = [{registration = #parent_reader, ordinary_results = 0 : i64, candidates = []}]})ir";
  if (routed && mixed)
    replace(text, "writers = [{registration = #parent_reader, ordinary_results = 0 : i64, candidates = []}]",
            "writers = [{registration = #parent_reader, ordinary_results = 0 : i64, candidates = []}, {registration = #scalar_reader, ordinary_results = 0 : i64, candidates = []}]");
  text += samples + "} : () -> ()\n";
  if (routed)
    text += "\"ac.system\"() {entry = {callee = @\"reset_value.fixture.Held\", parameters = [], type_arguments = []}, domain = \"default\"} : () -> ()\n";
  return text + "}\n";
}

TEST_F(StateResetValueTest, PublishGenericFIFOSystemForIndependentScoreboard) {
  publishSaved(genericFIFOSystemIR(false), genericFIFOSystemIR(true),
               "PYCIRCUIT_TEST_GENERIC_FIFO_ANCHOR");
}

TEST_F(StateResetValueTest, PublishMixedActualFIFOSystemForIndependentScoreboards) {
  auto routedText = genericFIFOSystemIR(true, true);
  auto logical = mlir::parseSourceString<mlir::ModuleOp>(routedText, &context);
  ASSERT_TRUE(logical);
  unsigned definitions = 0;
  ac::InstanceOp packet, scalar;
  logical->walk([&](ac::ModuleOp module) {
    if (module.getSymName() == "reset_value.fixture.GenericFIFO") ++definitions;
  });
  logical->walk([&](ac::InstanceOp instance) {
    if (instance.getInstanceName() == "generic_fifo") packet = instance;
    if (instance.getInstanceName() == "scalar_fifo") scalar = instance;
  });
  ASSERT_EQ(definitions, 1u);
  ASSERT_TRUE(packet); ASSERT_TRUE(scalar);
  EXPECT_EQ(packet.getCallee(), scalar.getCallee());
  EXPECT_NE(packet.getOccurrence(), scalar.getOccurrence());
  EXPECT_NE(packet.getTypeArguments(), scalar.getTypeArguments());
  EXPECT_TRUE(mlir::isa<ac::StructType>(packet.getResult(0).getType()));
  EXPECT_TRUE(mlir::isa<ac::BitsType>(scalar.getResult(0).getType()));
  publishSaved(genericFIFOSystemIR(false, true), routedText,
               "PYCIRCUIT_TEST_MIXED_ACTUAL_FIFO_ANCHOR");
}

TEST_F(StateResetValueTest, ResourceSamplingAcceptsActualClockAliases) {
  auto text = genericFIFOSystemIR(true);
  auto replace = [&](const std::string &from, const std::string &to) {
    const auto position = text.find(from);
    ASSERT_NE(position, std::string::npos);
    text.replace(position, from.size(), to);
  };
  // These remain two distinct callee arguments and domain formals. The parent
  // binds both to one actual clock; endpoint sampling must follow that identity.
  replace("\"ac.instance\"(%wclk, %rclk, %rst)",
          "\"ac.instance\"(%wclk, %wclk, %rst)");
  replace("ac.domain_bindings = {W = \"W\", R = \"R\"}",
          "ac.domain_bindings = {W = \"W\", R = \"W\"}");
  replace("ac.domain_clock_inputs = {W = 0 : i32, R = 1 : i32}",
          "ac.domain_clock_inputs = {W = 0 : i32, R = 0 : i32}");
  replace("\"ac.rule\"(%wclk, %rst, %sequence, %wa)",
          "\"ac.rule\"(%rclk, %rst, %sequence, %wa)");
  replace("\"ac.expect\"(%wg, %wreach, %wclk, %rst)",
          "\"ac.expect\"(%wg, %wreach, %rclk, %rst)");
  replace("\"ac.rule\"(%rclk, %rst, %head, %available)",
          "\"ac.rule\"(%wclk, %rst, %head, %available)");
  replace("\"ac.expect\"(%guard, %reached, %rclk, %rst)",
          "\"ac.expect\"(%guard, %reached, %wclk, %rst)");
  auto package = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
  ASSERT_TRUE(package);
  EXPECT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*package)));
}

std::string cdcRollbackIR(bool routed) {
  auto text = routed ? asyncNegativeStateIR() : stateIR(false);
  auto replace = [&](const std::string &from, const std::string &to) {
    const auto position = text.find(from);
    EXPECT_NE(position, std::string::npos);
    if (position != std::string::npos) text.replace(position, from.size(), to);
  };
  text.insert(text.find("module attributes"),
      "#synchronizer = {site = {definition = @\"reset_value.fixture.Held\", ast_path = [{kind = \"index\", value = 8 : i64}]}, expansion = []}\n");
  const std::string synced = routed ? R"ir(
    %synced = "ac.instance"(%clk, %clk, %rst, %enable) {instance_name = "synchronizer", callee = @"pycircuit.__builtins__.async_reg", parameters = [], type_arguments = [], occurrence = #synchronizer, source_domain = "D", target_domain = "D", target_edge = "negedge", reset_polarity = "low", resource_contract = 1 : i64} : (!b1, !b1, !b1, !b1) -> !b1
)ir" : "    %synced = \"ac.bits.constant\"() {value = #w0} : () -> !b1\n";
  replace("    \"ac.yield\"(%q) : (!b9)",
          synced + "    \"ac.yield\"(%q, %synced) : (!b9, !b1)");
  replace("function_type = (!b1, !b1, !b1) -> !b9",
          "function_type = (!b1, !b1, !b1) -> (!b9, !b1)");
  replace("output_names = [\"q\"]", "output_names = [\"q\", \"synced\"]");
  return routed ? withNativeCDCImport(text, "async_reg") : text;
}

TEST_F(StateResetValueTest, PublishCDCRollbackForIndependentReplay) {
  publishSaved(cdcRollbackIR(false), cdcRollbackIR(true),
               "PYCIRCUIT_TEST_CDC_ROLLBACK_ANCHOR");
}

TEST_F(StateResetValueTest, PublishLogicalDepthThreeFIFOWithoutPhysicalization) {
  auto text = fifoSystemIR(true);
  const auto depth = text.find("#ac.math_int<4>");
  ASSERT_NE(depth, std::string::npos);
  text.replace(depth, std::string("#ac.math_int<4>").size(), "#ac.math_int<3>");
  auto source = mlir::parseSourceString<mlir::ModuleOp>(fifoSystemIR(false), &context);
  auto routed = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
  ASSERT_TRUE(source); ASSERT_TRUE(routed);
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*routed)));
  auto header = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(source->getOperation()->clone()));
  mlir::PassManager manager(&context);
  manager.addPass(acir::createExtractSourceInterfacePass());
  ASSERT_TRUE(mlir::succeeded(manager.run(*header)));
  AnchorSourceProvenance provenance{{*source, *header},
      anchorSha256(print(source->getOperation())), print(header->getOperation())};
  ASSERT_TRUE(mlir::succeeded(writeAnchorArtifacts(
      *routed, {provenance}, directory.string(), [&] {
        return mlir::emitError(mlir::UnknownLoc::get(&context));
      })));
  source = {}; header = {}; routed = {};
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, gf::GFIRDialect>();
  auto error = [&] { return mlir::emitError(mlir::UnknownLoc::get(&fresh)); };
  auto loaded = loadVerifiedAnchor(directory.string(), fresh, error);
  ASSERT_TRUE(mlir::succeeded(loaded));
  const auto before = print(loaded->package->getOperation());
  {
    mlir::ScopedDiagnosticHandler expectedFailure(
        &fresh, [](mlir::Diagnostic &) { return mlir::success(); });
    EXPECT_TRUE(mlir::failed(refineVerifiedAnchor(*loaded, error)));
  }
  EXPECT_EQ(print(loaded->package->getOperation()), before);
  if (const char *output = std::getenv("PYCIRCUIT_TEST_DEPTH_THREE_FIFO_ANCHOR")) {
    ASSERT_FALSE(fs::exists(output));
    fs::copy(directory, output, fs::copy_options::recursive);
  }
}

std::string boundaryStateIR(bool routed) {
  auto text = routed ? asyncNegativeStateIR() : stateIR(false);
  const auto rootEnd = text.find("} : () -> ()\n");
  EXPECT_NE(rootEnd, std::string::npos);
  text.replace(rootEnd, std::string("} : () -> ()\n").size(),
               "} : () -> () loc(\"fixture.py\":1:1)\n");
  if (!routed) return text;
  text.insert(text.find("module attributes"),
      "#entry_occ = {site = {definition = @\"pycircuit.__link_entry__\", ast_path = []}, expansion = []}\n");
  const auto system = text.find("  \"ac.system\"");
  EXPECT_NE(system, std::string::npos);
  text.replace(system, text.size() - system, R"ir(
  "ac.module"() ({
  ^bb0(%enable: !b1, %clk: !b1, %rst: !b1):
    %q = "ac.instance"(%enable, %clk, %rst) {instance_name = "entry", callee = @"reset_value.fixture.Held", parameters = [], type_arguments = [], occurrence = #entry_occ, ac.domain_bindings = {D = "D"}} : (!b1, !b1, !b1) -> !b9 loc("fixture.py":1:1)
    "ac.yield"(%q) : (!b9) -> () loc("fixture.py":1:1)
  }) {sym_name = "pycircuit.__link_entry__", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> !b9, input_names = ["enable", "pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}, ac.domain_params = ["D"], ac.default_domain = "D", ac.domain_clock_inputs = {D = 1 : i32}} : () -> () loc("fixture.py":1:1)
  "ac.system"() {entry = {callee = @"pycircuit.__link_entry__", parameters = [], type_arguments = []}, domain = "default", ac.entry_boundary = {version = 1 : i64, logical_entry = {callee = @"reset_value.fixture.Held", parameters = [], type_arguments = []}}} : () -> ()
}
)ir");
  return text;
}

TEST_F(StateResetValueTest, PublishNativeEntryBoundaryForIndependentConsumers) {
  publishSaved(boundaryStateIR(false), boundaryStateIR(true),
               "PYCIRCUIT_TEST_BOUNDARY_STATE_ANCHOR");
}

TEST_F(StateResetValueTest, CDCRefinementRetainsLogicalSourceDomainIdentity) {
  auto logical = mlir::parseSourceString<mlir::ModuleOp>(asyncRegSystemIR(true), &context);
  ASSERT_TRUE(logical);
  ac::InstanceOp resource;
  logical->walk([&](ac::InstanceOp candidate) {
    if (candidate.getCallee() == "pycircuit.__builtins__.async_reg")
      resource = candidate;
  });
  ASSERT_TRUE(resource);
  // A domain-neutral constant keeps both source-domain declarations locally
  // legal, so this test isolates correspondence to the original resource.
  mlir::OpBuilder builder(resource);
  mlir::OperationState constant(resource.getLoc(), "ac.bits.constant4");
  constant.addTypes(resource->getOperand(3).getType());
  constant.addAttribute("pattern", builder.getStringAttr("1"));
  resource->setOperand(3, builder.create(constant)->getResult(0));
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*logical)));
  auto expanded = ac::materializeHardwareCDC(*logical);
  ASSERT_TRUE(mlir::succeeded(expanded));
  ASSERT_TRUE(mlir::succeeded(ac::verifyHardwareCDCMaterialization(*logical, **expanded)));
  ASSERT_TRUE(mlir::succeeded(ac::verifyFinalHardwareCDCComponents(**expanded)));
  ac::CDCRefinementOp witness;
  (*expanded)->walk([&](ac::CDCRefinementOp candidate) {
    if (candidate.getLogicalKind() == "async_reg") witness = candidate;
  });
  ASSERT_TRUE(witness);
  ASSERT_NE(witness->getOperand(0), witness->getOperand(1));
  witness->setOperand(0, witness->getOperand(1));
  witness->setAttr("source_domain", witness->getAttr("target_domain"));
  // Physical equations do not sample the source clock. Their self-consistency
  // must not substitute for the logical-to-expanded identity proof.
  EXPECT_TRUE(mlir::succeeded(ac::verifyFinalHardwareCDCComponents(**expanded)));
  mlir::ScopedDiagnosticHandler expectedFailure(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  EXPECT_TRUE(mlir::failed(ac::verifyHardwareCDCMaterialization(*logical, **expanded)));
}

TEST_F(StateResetValueTest, PublishTableSelectedResetForManagedMetadata) {
  auto text = asyncNegativeStateIR();
  auto replace = [&](const std::string &from, const std::string &to) {
    const auto position = text.find(from);
    ASSERT_NE(position, std::string::npos);
    text.replace(position, from.size(), to);
  };
  text.insert(text.find("module attributes"), R"ir(
#reset_extent = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = #occ, location = #span}>
!reset_table = !ac.table<[#reset_extent], !b1>
)ir");
  replace("    %initial =", R"ir(
    %reset_zero = "ac.bits.constant"() {value = #w0} : () -> !b1
    %reset_one = "ac.bits.constant"() {value = #w1} : () -> !b1
    %reset_levels = "ac.table.create"(%reset_zero, %reset_one) : (!b1, !b1) -> !reset_table
    %selected_reset = "ac.table.read"(%reset_levels, %enable) : (!reset_table, !b1) -> !b1
    %initial =)ir");
  replace("\"gfir.state\"(%clk, %rst,", "\"gfir.state\"(%clk, %selected_reset,");
  replace("\"ac.rule\"(%clk, %rst,", "\"ac.rule\"(%clk, %selected_reset,");
  replace("\"ac.expect\"(%raw, %reached, %clk, %rst)",
          "\"ac.expect\"(%raw, %reached, %clk, %selected_reset)");
  publishSaved(stateIR(false), text,
               "PYCIRCUIT_TEST_TABLE_RESET_ANCHOR");
}
