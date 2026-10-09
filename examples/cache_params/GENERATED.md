# cache_params: generated output

These excerpts come from the public compiler's actual output after a successful
native worker-1/worker-2 and RTL oracle run. They are reading samples, not a
second implementation or standalone replacement for the complete generated files.

Verified WORK trace rows: **14**. Final artifact SHA-256: `25d7caf2f7098aec9a87a7d1771cee0c4a163e9f350b1bd577cc74f3261076f4`.

Trace rows may be sparse checkpoints or summaries; see the example README for the number of checked epochs.

Source, runner and generated-file digests are recorded in [GENERATED.json](GENERATED.json).
The recipe below regenerates complete artifacts outside the source tree.

```sh
cmake -S examples/cache_params -B /absolute/build/cache_params -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/cache_params --parallel 4
ctest --test-dir /absolute/build/cache_params --output-on-failure --no-tests=error
python3 tools/example_catalog.py generated --example cache_params \
  --build /absolute/build/cache_params
```

## Verified MLIR

`cache_params.ac`, from line 4:

```mlir
  "ac.module"() <{function_type = (!ac.bits<<{kind = "literal", location = {column = 23 : i64, end_column = 29 : i64, end_line = 14 : i64, line = 14 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<40>}}>>) -> !ac.struct<"example_cache_params.cache_params.CacheResult">, input_names = ["addr"], output_names = ["result"], parameters = [], source_owner = {package = "example_cache_params", path = "cache_params.py"}, sym_name = "example_cache_params.cache_params.CacheParams", type_parameters = []}> ({
  ^bb0(%arg0: !ac.bits<<{kind = "literal", location = {column = 23 : i64, end_column = 29 : i64, end_line = 14 : i64, line = 14 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<40>}}>> loc("cache_params.py":14:1)):
    %0 = "ac.bits.extract"(%arg0) <{low = #ac.static_expr<{kind = "literal", location = {column = 33 : i64, end_column = 38 : i64, end_line = 15 : i64, line = 15 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}, {kind = "field", name = "keywords"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}, {kind = "field", name = "slice"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<12>}}>}> : (!ac.bits<<{kind = "literal", location = {column = 23 : i64, end_column = 29 : i64, end_line = 14 : i64, line = 14 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<40>}}>>) -> !ac.bits<<{kind = "literal", location = {column = 33 : i64, end_column = 38 : i64, end_line = 15 : i64, line = 15 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}, {kind = "field", name = "keywords"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}, {kind = "field", name = "slice"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<28>}}>> loc(#loc3)
    %1 = "ac.bits.constant"() <{value = #ac.static_expr<{kind = "literal", location = {column = 25 : i64, end_column = 26 : i64, end_line = 9 : i64, line = 9 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "value"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<8>}}>}> : () -> !ac.bits<<{kind = "literal", location = {column = 17 : i64, end_column = 22 : i64, end_line = 9 : i64, line = 9 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<9>}}>> loc(#loc4)
    %2 = "ac.bits.extract"(%1) <{low = #ac.static_expr<{kind = "literal", location = {column = 25 : i64, end_column = 26 : i64, end_line = 9 : i64, line = 9 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "value"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<0>}}>}> : (!ac.bits<<{kind = "literal", location = {column = 17 : i64, end_column = 22 : i64, end_line = 9 : i64, line = 9 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<9>}}>>) -> !ac.bits<<{kind = "literal", location = {column = 17 : i64, end_column = 22 : i64, end_line = 9 : i64, line = 9 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<9>}}>> loc(#loc4)
    %3 = "ac.bits.constant"() <{value = #ac.static_expr<{kind = "literal", location = {column = 23 : i64, end_column = 25 : i64, end_line = 10 : i64, line = 10 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "value"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<28>}}>}> : () -> !ac.bits<<{kind = "literal", location = {column = 15 : i64, end_column = 20 : i64, end_line = 10 : i64, line = 10 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<9>}}>> loc(#loc5)
    %4 = "ac.bits.extract"(%3) <{low = #ac.static_expr<{kind = "literal", location = {column = 23 : i64, end_column = 25 : i64, end_line = 10 : i64, line = 10 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "value"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<0>}}>}> : (!ac.bits<<{kind = "literal", location = {column = 15 : i64, end_column = 20 : i64, end_line = 10 : i64, line = 10 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<9>}}>>) -> !ac.bits<<{kind = "literal", location = {column = 15 : i64, end_column = 20 : i64, end_line = 10 : i64, line = 10 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<9>}}>> loc(#loc5)
    %5 = ac.struct.create(%0, %2, %4) : (!ac.bits<<{kind = "literal", location = {column = 33 : i64, end_column = 38 : i64, end_line = 15 : i64, line = 15 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}, {kind = "field", name = "keywords"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "value"}, {kind = "field", name = "slice"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<28>}}>>, !ac.bits<<{kind = "literal", location = {column = 17 : i64, end_column = 22 : i64, end_line = 9 : i64, line = 9 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 1 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<9>}}>>, !ac.bits<<{kind = "literal", location = {column = 15 : i64, end_column = 20 : i64, end_line = 10 : i64, line = 10 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "body"}, {kind = "index", value = 2 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheResult}}, value = {kind = "integer", value = #ac.math_int<9>}}>>) -> !ac.struct<"example_cache_params.cache_params.CacheResult"> loc(#loc6)
    "ac.yield"(%5) : (!ac.struct<"example_cache_params.cache_params.CacheResult">) -> () loc(#loc2)
  }) {ac.declaration_role = "definition", ac.domain_inputs = {}, ac.origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}], definition = @example_cache_params.cache_params.CacheParams}}, ac.parameters = [{binding = "positional_or_keyword", constraint = {kind = "hardware", source_kind = "fixed_bits", type = !ac.bits<<{kind = "literal", location = {column = 23 : i64, end_column = 29 : i64, end_line = 14 : i64, line = 14 : i64, path = "cache_params.py"}, origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}, {kind = "field", name = "annotation"}], definition = @example_cache_params.cache_params.CacheParams}}, value = {kind = "integer", value = #ac.math_int<40>}}>>}, default = {present = false}, location = {column = 17 : i64, end_column = 29 : i64, end_line = 14 : i64, line = 14 : i64, path = "cache_params.py"}, name = "addr", origin = {expansion = [], site = {ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}, {kind = "field", name = "args"}, {kind = "field", name = "args"}, {kind = "index", value = 0 : i64}], definition = @example_cache_params.cache_params.CacheParams}}}], ac.result_constraints = [{kind = "hardware", source_kind = "nominal", type = !ac.struct<"example_cache_params.cache_params.CacheResult">}], ac.return_form = "single"} : () -> () loc(#loc2)
  "ac.system"() <{domain = "default", entry = {callee = @example_cache_params.cache_params.CacheParams, parameters = [], type_arguments = []}}> : () -> () loc(#loc2)
} loc(#loc)
#loc = loc(unknown)
#loc1 = loc("cache_params.py":7:1)
```

## Source-owned C++ Work

`cpp/sources/example_cache_params/cache_params.hpp`, from line 12:

```cpp
  void Work() {
    try {
    gfsim::wire<gfsim::table<::example_cache_params::cache_params::CacheResult, (pyc_count * (1))>> pyc_value_0;
    gfsim::wire<gfsim::table<gfsim::Bits<28>, (pyc_count * (1))>> pyc_value_1;
    for (std::size_t pyc_lane = 0; pyc_lane < pyc_count; ++pyc_lane) pyc_value_1.element(pyc_lane) = gfsim::wire<gfsim::Bits<28>>::fromPacked(gfsim::extract<gfsim::hardware_traits<gfsim::Bits<28>>::width>(this->addr.element(pyc_lane).packed(), 12));
    gfsim::wire<gfsim::table<gfsim::Bits<9>, (pyc_count * (1))>> pyc_value_2;
    gfsim::wire<gfsim::table<gfsim::Bits<9>, (pyc_count * (1))>> pyc_value_3;
    for (std::size_t pyc_lane = 0; pyc_lane < pyc_count; ++pyc_lane) pyc_value_3.element(pyc_lane) = gfsim::wire<gfsim::Bits<9>>::fromPacked(gfsim::FourState<gfsim::hardware_traits<gfsim::Bits<9>>::width>::known(gfsim::Bits<gfsim::hardware_traits<gfsim::Bits<9>>::width>{8ULL}));
    for (std::size_t pyc_lane = 0; pyc_lane < pyc_count; ++pyc_lane) pyc_value_2.element(pyc_lane) = gfsim::wire<gfsim::Bits<9>>::fromPacked(gfsim::extract<gfsim::hardware_traits<gfsim::Bits<9>>::width>(pyc_value_3.element(pyc_lane).packed(), 0));
    gfsim::wire<gfsim::table<gfsim::Bits<9>, (pyc_count * (1))>> pyc_value_4;
    gfsim::wire<gfsim::table<gfsim::Bits<9>, (pyc_count * (1))>> pyc_value_5;
    for (std::size_t pyc_lane = 0; pyc_lane < pyc_count; ++pyc_lane) pyc_value_5.element(pyc_lane) = gfsim::wire<gfsim::Bits<9>>::fromPacked(gfsim::FourState<gfsim::hardware_traits<gfsim::Bits<9>>::width>::known(gfsim::Bits<gfsim::hardware_traits<gfsim::Bits<9>>::width>{28ULL}));
    for (std::size_t pyc_lane = 0; pyc_lane < pyc_count; ++pyc_lane) pyc_value_4.element(pyc_lane) = gfsim::wire<gfsim::Bits<9>>::fromPacked(gfsim::extract<gfsim::hardware_traits<gfsim::Bits<9>>::width>(pyc_value_5.element(pyc_lane).packed(), 0));
    for (std::size_t pyc_lane = 0; pyc_lane < (pyc_count * (1)); ++pyc_lane) pyc_value_0.element(pyc_lane) = gfsim::wire<::example_cache_params::cache_params::CacheResult>::fromPacked(gfsim::concat(gfsim::concat(pyc_value_1.element(pyc_lane).packed(), pyc_value_2.element(pyc_lane).packed()), pyc_value_4.element(pyc_lane).packed()));
```

## Source-owned Verilog

`verilog/sources/example_cache_params/cache_params.v`, from line 1:

```systemverilog
module ac_example_cache_params_cache_params_CacheParams (
  input wire logic [(40)-1:0] addr,
  output wire pycircuit_types::ac_example_cache_params_cache_params_CacheResult_t result
);
  wire pycircuit_types::ac_example_cache_params_cache_params_CacheResult_t pyc_net_0;
  wire logic [(28)-1:0] pyc_net_1;
  assign pyc_net_1 = addr[(12) +: 28];
  wire logic [(9)-1:0] pyc_net_2;
  wire logic [(9)-1:0] pyc_net_3;
  assign pyc_net_3 = (9)'( 4'd8 );
  assign pyc_net_2 = pyc_net_3[(0) +: 9];
  wire logic [(9)-1:0] pyc_net_4;
  wire logic [(9)-1:0] pyc_net_5;
  assign pyc_net_5 = (9)'( 5'd28 );
```
