include_guard(GLOBAL)

function(add_acir_library target)
  cmake_parse_arguments(ARG "" "" "SOURCES;LINK_LIBRARIES" ${ARGN})
  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "add_acir_library(${target}) requires SOURCES")
  endif()

  add_library(${target} ${ARG_SOURCES})
  target_compile_features(${target} PUBLIC cxx_std_20)
  set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
  target_link_libraries(
    ${target}
    PUBLIC
      AgenticCircuit::ProjectOptions
      MLIRIR
      ${ARG_LINK_LIBRARIES}
  )
endfunction()
