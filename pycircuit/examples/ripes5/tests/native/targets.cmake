function(add_reference_runner)
  add_executable(ripes5-reference "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/runner.cpp")
  target_link_libraries(ripes5-reference PRIVATE ripes_lib)
  target_compile_definitions(ripes5-reference PRIVATE
    RIPES_SHA="5b8a616edcb6f0a2ddb07e78951348b72497f1e1"
    VSRTL_SHA="8497dd14fe80e57efcff4c424a9a3b6363d93eb7")
endfunction()
cmake_language(DEFER CALL add_reference_runner)
