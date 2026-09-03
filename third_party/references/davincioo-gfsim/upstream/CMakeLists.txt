cmake_minimum_required(VERSION 3.20)

project(gfsim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(gfsim
  src/backend/cube.cpp
  src/backend/dispatch.cpp
  src/backend/engine.cpp
  src/backend/issue_queue.cpp
  src/backend/rename.cpp
  src/backend/ready_table.cpp
  src/backend/rob.cpp
  src/backend/scalar.cpp
  src/backend/tma.cpp
  src/backend/vector.cpp
  src/pto_inst.cpp
  src/frontend/trace.cpp
  src/frontend/trace_source_module.cpp
  src/model_top/config.cpp
  src/model_top/cli.cpp
  src/model_top/core.cpp
  src/model_top/core_system.cpp
  src/model_top/kanata_pipeview.cpp
  src/model_top/perfetto_trace.cpp
  src/model_top/main.cpp
)

target_compile_features(gfsim PRIVATE cxx_std_20)
target_include_directories(gfsim PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
