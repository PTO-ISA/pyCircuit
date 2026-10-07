# hier_modules: generated output

These excerpts come from the public compiler's actual output after a successful
native worker-1/worker-2 and RTL oracle run. They are reading samples, not a
second implementation or standalone replacement for the complete generated files.

Verified WORK trace rows: **12**. Final artifact SHA-256: `ceca14e215657b30d000ae038fe8714b9c0a632b03fc63cff10b5becbfcbb048`.

Trace rows may be sparse checkpoints or summaries; see the example README for the number of checked epochs.

Source, runner and generated-file digests are recorded in [GENERATED.json](GENERATED.json).
The recipe below regenerates complete artifacts outside the source tree.

```sh
cmake -S examples/hier_modules -B /absolute/build/hier_modules -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/hier_modules --parallel 4
ctest --test-dir /absolute/build/hier_modules --output-on-failure --no-tests=error
python3 tools/pycircuit/example_catalog.py generated --example hier_modules \
  --build /absolute/build/hier_modules
```

## Verified MLIR

`hier_modules.ac`, from line 5:

```mlir
  "ac.module"() <{function_type = (!ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<8>}}>>) -> !ac.struct<"example_hier_modules.hier_modules.IncrementResult">, input_names = ["x"], output_names = ["result"], parameters = [], source_owner = {package = "example_hier_modules", path = "hier_modules.py"}, sym_name = "example_hier_modules.hier_modules.Incrementer", type_parameters = []}> ({
  ^bb0(%arg0: !ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<8>}}>> loc("hier_modules.py":12:1)):
    %0 = "ac.bits.constant"() <{value = #ac.static_expr<{kind = "literal", location = {column = 34 : i64, end_column = 35 : i64, end_line = 13 : i64, line = 13 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}, {kind = "field", name = "keywords"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}, {kind = "field", name = "right"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<1>}}>}> : () -> !ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<8>}}>> loc(#loc3)
    %1 = "ac.bits.binary"(%arg0, %0) <{opcode = "add"}> : (!ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<8>}}>>, !ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<8>}}>>) -> !ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<8>}}>> loc(#loc4)
    %2 = ac.struct.create(%1) : (!ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<8>}}>>) -> !ac.struct<"example_hier_modules.hier_modules.IncrementResult"> loc(#loc5)
    "ac.yield"(%2) : (!ac.struct<"example_hier_modules.hier_modules.IncrementResult">) -> () loc(#loc2)
  }) {ac.declaration_role = "definition", ac.domain_inputs = {}, ac.origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}], definition = @example_hier_modules.hier_modules.Incrementer}}, ac.parameters = [{binding = "positional_or_keyword", constraint = {kind = "hardware", source_kind = "fixed_bits", type = !ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.Incrementer}}, value = {kind = "integer", value = #ac.math_int<8>}}>>}, default = {present = false}, location = {column = 17 : i64, end_column = 25 : i64, end_line = 12 : i64, line = 12 : i64, path = "hier_modules.py"}, name = "x", origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}], definition = @example_hier_modules.hier_modules.Incrementer}}}], ac.result_constraints = [{kind = "hardware", source_kind = "nominal", type = !ac.struct<"example_hier_modules.hier_modules.IncrementResult">}], ac.return_form = "single"} : () -> () loc(#loc2)
  "ac.module"() <{function_type = (!ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 17 : i64, line = 17 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.HierModules}}, value = {kind = "integer", value = #ac.math_int<8>}}>>) -> !ac.struct<"example_hier_modules.hier_modules.IncrementResult">, input_names = ["x"], output_names = ["result"], parameters = [], source_owner = {package = "example_hier_modules", path = "hier_modules.py"}, sym_name = "example_hier_modules.hier_modules.HierModules", type_parameters = []}> ({
  ^bb0(%arg0: !ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 17 : i64, line = 17 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.HierModules}}, value = {kind = "integer", value = #ac.math_int<8>}}>> loc("hier_modules.py":17:1)):
    %0 = "ac.instance"(%arg0) <{callee = @example_hier_modules.hier_modules.Incrementer, instance_name = "__pyc_call_0", occurrence = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}], definition = @example_hier_modules.hier_modules.HierModules}}, parameters = [], type_arguments = []}> : (!ac.bits<<{kind = "literal", location = {column = 20 : i64, end_column = 25 : i64, end_line = 17 : i64, line = 17 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.HierModules}}, value = {kind = "integer", value = #ac.math_int<8>}}>>) -> !ac.struct<"example_hier_modules.hier_modules.IncrementResult"> loc(#loc7)
    %1 = "ac.instance"(%3) <{callee = @example_hier_modules.hier_modules.Incrementer, instance_name = "__pyc_call_1", occurrence = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "value"}], definition = @example_hier_modules.hier_modules.HierModules}}, parameters = [], type_arguments = []}> : (!ac.bits<<{kind = "literal", location = {column = 8 : i64, end_column = 13 : i64, end_line = 8 : i64, line = 8 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.IncrementResult}}, value = {kind = "integer", value = #ac.math_int<8>}}>>) -> !ac.struct<"example_hier_modules.hier_modules.IncrementResult"> loc(#loc8)
    %2 = "ac.instance"(%4) <{callee = @example_hier_modules.hier_modules.Incrementer, instance_name = "__pyc_call_2", occurrence = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "value"}], definition = @example_hier_modules.hier_modules.HierModules}}, parameters = [], type_arguments = []}> : (!ac.bits<<{kind = "literal", location = {column = 8 : i64, end_column = 13 : i64, end_line = 8 : i64, line = 8 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.IncrementResult}}, value = {kind = "integer", value = #ac.math_int<8>}}>>) -> !ac.struct<"example_hier_modules.hier_modules.IncrementResult"> loc(#loc9)
    %3 = ac.struct.get %0["y"] : (!ac.struct<"example_hier_modules.hier_modules.IncrementResult">) -> !ac.bits<<{kind = "literal", location = {column = 8 : i64, end_column = 13 : i64, end_line = 8 : i64, line = 8 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.IncrementResult}}, value = {kind = "integer", value = #ac.math_int<8>}}>> loc(#loc10)
    %4 = ac.struct.get %1["y"] : (!ac.struct<"example_hier_modules.hier_modules.IncrementResult">) -> !ac.bits<<{kind = "literal", location = {column = 8 : i64, end_column = 13 : i64, end_line = 8 : i64, line = 8 : i64, path = "hier_modules.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_hier_modules.hier_modules.IncrementResult}}, value = {kind = "integer", value = #ac.math_int<8>}}>> loc(#loc11)
```

## Source-owned C++ Work

`cpp/sources/example_hier_modules/hier_modules.hpp`, from line 12:

```cpp
  void Work() {
    try {
    gfsim::wire<gfsim::table<::example_hier_modules::hier_modules::IncrementResult, (pyc_count * (1))>> pyc_value_0;
    gfsim::wire<gfsim::table<gfsim::Bits<8>, (pyc_count * (1))>> pyc_value_1;
    gfsim::wire<gfsim::table<gfsim::Bits<8>, (pyc_count * (1))>> pyc_value_2;
    for (std::size_t pyc_lane = 0; pyc_lane < pyc_count; ++pyc_lane) pyc_value_2.element(pyc_lane) = gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::FourState<gfsim::hardware_traits<gfsim::Bits<8>>::width>::known(gfsim::Bits<gfsim::hardware_traits<gfsim::Bits<8>>::width>{1ULL}));
    for (std::size_t pyc_lane = 0; pyc_lane < pyc_count; ++pyc_lane) pyc_value_1.element(pyc_lane) = gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::add(this->x.element(pyc_lane).packed(), pyc_value_2.element(pyc_lane).packed()));
    for (std::size_t pyc_lane = 0; pyc_lane < (pyc_count * (1)); ++pyc_lane) pyc_value_0.element(pyc_lane) = gfsim::wire<::example_hier_modules::hier_modules::IncrementResult>::fromPacked(pyc_value_1.element(pyc_lane).packed());
    for (std::size_t pyc_pin = 0; pyc_pin < (pyc_count * (1)); ++pyc_pin) this->result.element(pyc_pin) = pyc_value_0.element(pyc_pin);
    } catch (...) { DiscardNext(); throw; }
  }
  void Xfer() noexcept {
  }
  void DiscardNext() noexcept {
```

## Source-owned Verilog

`verilog/sources/example_hier_modules/hier_modules.v`, from line 1:

```systemverilog
module ac_example_hier_modules_hier_modules_HierModules (
  input wire logic [(8)-1:0] x,
  output wire pycircuit_types::ac_example_hier_modules_hier_modules_IncrementResult_t result
);
  wire pycircuit_types::ac_example_hier_modules_hier_modules_IncrementResult_t pyc_net_0;
  assign result = pyc_net_0;
  wire pycircuit_types::ac_example_hier_modules_hier_modules_IncrementResult_t pyc_net_1;
  ac_example_hier_modules_hier_modules_Incrementer pyc_instance_pyc_5f5f7079635f63616c6c5f30 (
    .x(x),
    .result(pyc_net_1)
  );
  wire logic [(8)-1:0] pyc_net_2;
  assign pyc_net_2 = pyc_net_1.y;
  wire pycircuit_types::ac_example_hier_modules_hier_modules_IncrementResult_t pyc_net_3;
```
