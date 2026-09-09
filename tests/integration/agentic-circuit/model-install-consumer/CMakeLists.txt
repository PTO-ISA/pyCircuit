cmake_minimum_required(VERSION 3.25)

project(AgenticCircuitModelConsumer VERSION 0.1.0 LANGUAGES CXX)

find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED COMPONENTS Runtime)

if(NOT IS_ABSOLUTE "${AGENTIC_MODEL_ROOT}")
  message(FATAL_ERROR "AGENTIC_MODEL_ROOT must be absolute")
endif()
include("${AGENTIC_MODEL_ROOT}/model-sources.cmake")

set(_agentic_model_sources "")
foreach(_source IN LISTS AGENTIC_MODEL_GENERATED_SOURCES)
  list(APPEND _agentic_model_sources "${AGENTIC_MODEL_ROOT}/${_source}")
endforeach()

add_library(model-plugin SHARED ${_agentic_model_sources})
target_compile_features(model-plugin PRIVATE cxx_std_20)
set_target_properties(model-plugin PROPERTIES
  CXX_EXTENSIONS OFF
  CXX_VISIBILITY_PRESET hidden
)
target_include_directories(model-plugin PRIVATE
  "${AGENTIC_MODEL_ROOT}/include")
get_target_property(_agentic_runtime_includes
  ${AGENTIC_MODEL_RUNTIME_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(model-plugin PUBLIC ${_agentic_runtime_includes})
target_link_libraries(model-plugin PRIVATE ${AGENTIC_MODEL_RUNTIME_TARGET})

if(APPLE)
  target_link_options(model-plugin PRIVATE
    "LINKER:-exported_symbol,_${AGENTIC_MODEL_QUERY_SYMBOL}")
elseif(UNIX)
  set(_agentic_model_exports "${CMAKE_CURRENT_BINARY_DIR}/agentic-model.exports")
  file(WRITE "${_agentic_model_exports}"
    "{ global: ${AGENTIC_MODEL_QUERY_SYMBOL}; local: *; };\n")
  target_link_options(model-plugin PRIVATE
    "LINKER:--exclude-libs,ALL"
    "LINKER:--version-script=${_agentic_model_exports}")
endif()

add_executable(model-consumer main.cpp)
target_compile_features(model-consumer PRIVATE cxx_std_20)
set_target_properties(model-consumer PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(model-consumer PRIVATE model-plugin)

if(NOT AGENTIC_MODEL_QUERY_SYMBOL STREQUAL "agentic_model_query_v1")
  message(FATAL_ERROR "generated model query symbol is incompatible")
endif()
