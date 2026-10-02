# Loaded by CMAKE_PROJECT_Ripes_INCLUDE; upstream source remains untouched.
file(READ "${CMAKE_CURRENT_LIST_DIR}/version.json" RIPES_LOCK)
string(JSON RIPES_EXPECTED GET "${RIPES_LOCK}" ripes)
string(JSON VSRTL_EXPECTED GET "${RIPES_LOCK}" submodules external/VSRTL)
execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                OUTPUT_VARIABLE RIPES_SHA OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/external/VSRTL"
                OUTPUT_VARIABLE VSRTL_SHA OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
if(NOT RIPES_SHA STREQUAL RIPES_EXPECTED OR NOT VSRTL_SHA STREQUAL VSRTL_EXPECTED)
    message(FATAL_ERROR "Ripes / VSRTL checkout does not match version.json")
endif()
foreach(REF_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/external/VSRTL")
    execute_process(COMMAND git status --porcelain --untracked-files=no WORKING_DIRECTORY "${REF_DIR}"
                    OUTPUT_VARIABLE REF_STATUS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    if(REF_STATUS)
        message(FATAL_ERROR "Modified upstream source: ${REF_DIR}")
    endif()
endforeach()
add_executable(ripes5-reference "${CMAKE_CURRENT_LIST_DIR}/runner.cpp")
target_link_libraries(ripes5-reference PRIVATE ripes_lib Qt6::Core)
target_compile_definitions(ripes5-reference PRIVATE RIPES_SHA="${RIPES_SHA}" VSRTL_SHA="${VSRTL_SHA}")
