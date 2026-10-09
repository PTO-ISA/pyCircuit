# fastfwd: generated output

These excerpts come from the public compiler's actual output after a successful
native worker-1/worker-2 and RTL oracle run. They are reading samples, not a
second implementation or standalone replacement for the complete generated files.

Verified WORK trace rows: **24**. Final artifact SHA-256: `6b447636a0bcbac58a7097274675bd6a49ba91e18e1517a310e36f78b087a679`.

Trace rows may be sparse checkpoints or summaries; see the example README for the number of checked epochs.

Source, runner and generated-file digests are recorded in [GENERATED.json](GENERATED.json).
The recipe below regenerates complete artifacts outside the source tree.

```sh
cmake -S examples/fastfwd -B /absolute/build/fastfwd -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/fastfwd --parallel 4
ctest --test-dir /absolute/build/fastfwd --output-on-failure --no-tests=error
python3 tools/example_catalog.py generated --example fastfwd \
  --build /absolute/build/fastfwd
```

## Verified MLIR

`fastfwd.ac`, from line 9:

```mlir
  "ac.module"() <{function_type = (!ac.struct<"example_fastfwd.fastfwd.LaneInput">) -> !ac.struct<"example_fastfwd.fastfwd.Channel">, input_names = ["packet"], output_names = ["result"], parameters = [], source_owner = {package = "example_fastfwd", path = "fastfwd.py"}, sym_name = "example_fastfwd.fastfwd.ForwardLane", type_parameters = []}> ({
  ^bb0(%arg0: !ac.struct<"example_fastfwd.fastfwd.LaneInput"> loc("fastfwd.py":42:1)):
    %0 = ac.struct.get %arg0["valid"] : (!ac.struct<"example_fastfwd.fastfwd.LaneInput">) -> !ac.bits<<{kind = "literal", location = {column = 12 : i64, end_column = 17 : i64, end_line = 14 : i64, line = 14 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_fastfwd.fastfwd.LaneInput}}, value = {kind = "integer", value = #ac.math_int<1>}}>> loc(#loc6)
    %1 = ac.struct.get %arg0["data"] : (!ac.struct<"example_fastfwd.fastfwd.LaneInput">) -> !ac.bits<<{kind = "literal", location = {column = 19 : i64, end_column = 22 : i64, end_line = 15 : i64, line = 15 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "annotation"}, {kind = "field", name = "slice"}], definition = @example_fastfwd.fastfwd.LaneInput}}, value = {kind = "integer", value = #ac.math_int<128>}}>> loc(#loc7)
    %2 = ac.struct.create(%0, %1) : (!ac.bits<<{kind = "literal", location = {column = 12 : i64, end_column = 17 : i64, end_line = 14 : i64, line = 14 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_fastfwd.fastfwd.LaneInput}}, value = {kind = "integer", value = #ac.math_int<1>}}>>, !ac.bits<<{kind = "literal", location = {column = 19 : i64, end_column = 22 : i64, end_line = 15 : i64, line = 15 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "annotation"}, {kind = "field", name = "slice"}], definition = @example_fastfwd.fastfwd.LaneInput}}, value = {kind = "integer", value = #ac.math_int<128>}}>>) -> !ac.struct<"example_fastfwd.fastfwd.Channel"> loc(#loc8)
    "ac.yield"(%2) : (!ac.struct<"example_fastfwd.fastfwd.Channel">) -> () loc(#loc5)
  }) {ac.declaration_role = "definition", ac.domain_inputs = {}, ac.origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 6 : i64}], definition = @example_fastfwd.fastfwd.ForwardLane}}, ac.parameters = [{binding = "positional_or_keyword", constraint = {kind = "hardware", source_kind = "nominal", type = !ac.struct<"example_fastfwd.fastfwd.LaneInput">}, default = {present = false}, location = {column = 17 : i64, end_column = 34 : i64, end_line = 42 : i64, line = 42 : i64, path = "fastfwd.py"}, name = "packet", origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 6 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}], definition = @example_fastfwd.fastfwd.ForwardLane}}}], ac.result_constraints = [{kind = "hardware", source_kind = "nominal", type = !ac.struct<"example_fastfwd.fastfwd.Channel">}], ac.return_form = "single"} : () -> () loc(#loc5)
  "ac.module"() <{function_type = (!ac.struct<"example_fastfwd.fastfwd.Channel">) -> !ac.struct<"example_fastfwd.fastfwd.EngineResult">, input_names = ["packet"], output_names = ["result"], parameters = [], source_owner = {package = "example_fastfwd", path = "fastfwd.py"}, sym_name = "example_fastfwd.fastfwd.ForwardEngine", type_parameters = []}> ({
  ^bb0(%arg0: !ac.struct<"example_fastfwd.fastfwd.Channel"> loc("fastfwd.py":47:1)):
    %0 = ac.struct.get %arg0["valid"] : (!ac.struct<"example_fastfwd.fastfwd.Channel">) -> !ac.bits<<{kind = "literal", location = {column = 12 : i64, end_column = 17 : i64, end_line = 8 : i64, line = 8 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_fastfwd.fastfwd.Channel}}, value = {kind = "integer", value = #ac.math_int<1>}}>> loc(#loc10)
    %1 = ac.struct.get %arg0["data"] : (!ac.struct<"example_fastfwd.fastfwd.Channel">) -> !ac.bits<<{kind = "literal", location = {column = 19 : i64, end_column = 22 : i64, end_line = 9 : i64, line = 9 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "annotation"}, {kind = "field", name = "slice"}], definition = @example_fastfwd.fastfwd.Channel}}, value = {kind = "integer", value = #ac.math_int<128>}}>> loc(#loc11)
    %2 = "ac.bits.constant"() <{value = #ac.static_expr<{kind = "literal", location = {column = 12 : i64, end_column = 62 : i64, end_line = 48 : i64, line = 48 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 7 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}], definition = @example_fastfwd.fastfwd.ForwardEngine}}, value = {kind = "integer", value = #ac.math_int<0>}}>}> : () -> !ac.bits<<{kind = "literal", location = {column = 14 : i64, end_column = 19 : i64, end_line = 23 : i64, line = 23 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "annotation"}], definition = @example_fastfwd.fastfwd.EngineResult}}, value = {kind = "integer", value = #ac.math_int<2>}}>> loc(#loc12)
    %3 = "ac.bits.constant"() <{value = #ac.static_expr<{kind = "literal", location = {column = 12 : i64, end_column = 62 : i64, end_line = 48 : i64, line = 48 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 7 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}], definition = @example_fastfwd.fastfwd.ForwardEngine}}, value = {kind = "integer", value = #ac.math_int<0>}}>}> : () -> !ac.bits<<{kind = "literal", location = {column = 21 : i64, end_column = 26 : i64, end_line = 24 : i64, line = 24 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "annotation"}], definition = @example_fastfwd.fastfwd.EngineResult}}, value = {kind = "integer", value = #ac.math_int<1>}}>> loc(#loc12)
    %4 = "ac.bits.constant"() <{value = #ac.static_expr<{kind = "literal", location = {column = 12 : i64, end_column = 62 : i64, end_line = 48 : i64, line = 48 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 7 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}], definition = @example_fastfwd.fastfwd.ForwardEngine}}, value = {kind = "integer", value = #ac.math_int<0>}}>}> : () -> !ac.bits<<{kind = "literal", location = {column = 28 : i64, end_column = 31 : i64, end_line = 25 : i64, line = 25 : i64, path = "fastfwd.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 4 : i64}, {kind = "field", name = "annotation"}, {kind = "field", name = "slice"}], definition = @example_fastfwd.fastfwd.EngineResult}}, value = {kind = "integer", value = #ac.math_int<128>}}>> loc(#loc12)
```

## Source-owned C++ Work

`cpp/sources/example_fastfwd/fastfwd.hpp`, from line 12:

```cpp
  void Work() {
    try {
    gfsim::wire<gfsim::table<::example_fastfwd::fastfwd::Channel, (pyc_count * (1))>> pyc_value_0;
    gfsim::wire<gfsim::table<gfsim::Bits<1>, (pyc_count * (1))>> pyc_value_1;
    for (std::size_t pyc_lane = 0; pyc_lane < (pyc_count * (1)); ++pyc_lane) pyc_value_1.element(pyc_lane) = gfsim::wire<gfsim::Bits<1>>::fromPacked(gfsim::extract<gfsim::hardware_traits<gfsim::Bits<1>>::width>(this->packet.element(pyc_lane).packed(), 0 + gfsim::hardware_traits<gfsim::Bits<128>>::width + gfsim::hardware_traits<gfsim::Bits<5>>::width));
    gfsim::wire<gfsim::table<gfsim::Bits<128>, (pyc_count * (1))>> pyc_value_2;
    for (std::size_t pyc_lane = 0; pyc_lane < (pyc_count * (1)); ++pyc_lane) pyc_value_2.element(pyc_lane) = gfsim::wire<gfsim::Bits<128>>::fromPacked(gfsim::extract<gfsim::hardware_traits<gfsim::Bits<128>>::width>(this->packet.element(pyc_lane).packed(), 0 + gfsim::hardware_traits<gfsim::Bits<5>>::width));
    for (std::size_t pyc_lane = 0; pyc_lane < (pyc_count * (1)); ++pyc_lane) pyc_value_0.element(pyc_lane) = gfsim::wire<::example_fastfwd::fastfwd::Channel>::fromPacked(gfsim::concat(pyc_value_1.element(pyc_lane).packed(), pyc_value_2.element(pyc_lane).packed()));
    for (std::size_t pyc_pin = 0; pyc_pin < (pyc_count * (1)); ++pyc_pin) this->result.element(pyc_pin) = pyc_value_0.element(pyc_pin);
    } catch (...) { DiscardNext(); throw; }
  }
  void Xfer() noexcept {
  }
  void DiscardNext() noexcept {
```

## Source-owned Verilog

`verilog/sources/example_fastfwd/fastfwd.v`, from line 1:

```systemverilog
module ac_example_fastfwd_fastfwd_Fastfwd (
  input wire pycircuit_types::ac_example_fastfwd_fastfwd_LaneInput_t lane0,
  input wire pycircuit_types::ac_example_fastfwd_fastfwd_LaneInput_t lane1,
  input wire pycircuit_types::ac_example_fastfwd_fastfwd_LaneInput_t lane2,
  input wire pycircuit_types::ac_example_fastfwd_fastfwd_LaneInput_t lane3,
  input wire pycircuit_types::ac_example_fastfwd_fastfwd_Channel_t engine0,
  input wire pycircuit_types::ac_example_fastfwd_fastfwd_Channel_t engine1,
  input wire pycircuit_types::ac_example_fastfwd_fastfwd_Channel_t engine2,
  input wire pycircuit_types::ac_example_fastfwd_fastfwd_Channel_t engine3,
  output wire pycircuit_types::ac_example_fastfwd_fastfwd_FastResult_t result
);
  wire pycircuit_types::ac_example_fastfwd_fastfwd_FastResult_t pyc_net_0;
  wire logic [(1)-1:0] pyc_net_1;
  assign pyc_net_1 = (1)'( 1'd0 );
```
