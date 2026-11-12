cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(pycircuit VERSION 6.1.0 LANGUAGES C CXX)
endif()

set(PYCIRCUIT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(PYCIRCUIT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
get_filename_component(PYCIRCUIT_REPO_ROOT "${PYCIRCUIT_SOURCE_DIR}/.." ABSOLUTE)

if(NOT TARGET MLIRIR)
  find_package(LLVM 22.1.8 EXACT CONFIG REQUIRED)
  find_package(MLIR 22.1.8 EXACT CONFIG REQUIRED)
endif()

list(APPEND CMAKE_MODULE_PATH "${LLVM_DIR}" "${MLIR_DIR}"
     "${PYCIRCUIT_SOURCE_DIR}/cmake")
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(AddACIR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
include_directories("${PYCIRCUIT_SOURCE_DIR}/include" "${PYCIRCUIT_BINARY_DIR}/include")
add_definitions(${LLVM_DEFINITIONS})

add_library(pycircuit_project_options INTERFACE)
target_compile_features(pycircuit_project_options INTERFACE cxx_std_20)
# Compiler consumers must match LLVM's RTTI configuration. Keep this on the
# compiler interface so standalone Runtime targets retain their own policy.
if(NOT LLVM_ENABLE_RTTI)
  if(MSVC)
    target_compile_options(pycircuit_project_options INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:/GR->")
  else()
    target_compile_options(pycircuit_project_options INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>")
  endif()
endif()

option(PYCIRCUIT_BUILD_TESTING "Build isolated source-unit tests" OFF)
if(PYCIRCUIT_BUILD_TESTING)
  include(CTest)
  enable_testing()
endif()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools)

if(PYCIRCUIT_BUILD_TESTING)
  add_subdirectory(
    "${PYCIRCUIT_REPO_ROOT}/tests/compiler/IR"
    "${PYCIRCUIT_BINARY_DIR}/tests/source-unit"
  )
endif()


# ---------------------------------------------------------------------------
# Layer-direction enforcement (B2 step-2 design r9 §4): resolve each layer
# target's transitive INTERFACE_LINK_LIBRARIES at configure time and fail
# on forbidden edges. Generator-expression values are stripped; ALIAS
# targets resolve; a visited set guards cycles.
function(_pyc_split_link_entries raw out)
  # Split a link line into entries at semicolons OUTSIDE generator
  # expressions (r8 review follow-up): a list argument inside
  # $<BUILD_INTERFACE:A;B> must stay one entry, or the halves read as
  # unbalanced expressions.
  set(result "")
  set(current "")
  set(depth 0)
  string(LENGTH "${raw}" rawLength)
  set(scan 0)
  while(scan LESS rawLength)
    string(SUBSTRING "${raw}" ${scan} 2 pair)
    string(SUBSTRING "${raw}" ${scan} 1 char)
    if(pair STREQUAL "$<")
      math(EXPR depth "${depth} + 1")
      string(APPEND current "$<")
      math(EXPR scan "${scan} + 2")
      continue()
    endif()
    if(char STREQUAL ">" AND depth GREATER 0)
      math(EXPR depth "${depth} - 1")
      string(APPEND current ">")
      math(EXPR scan "${scan} + 1")
      continue()
    endif()
    if(char STREQUAL ";" AND depth EQUAL 0)
      if(NOT current STREQUAL "")
        # Escape the entry's semicolons so the list round-trip keeps each
        # entry whole for the caller's foreach (r8 review follow-up).
        string(REGEX REPLACE ";" "\\\\;" current "${current}")
        list(APPEND result "${current}")
      endif()
      set(current "")
      math(EXPR scan "${scan} + 1")
      continue()
    endif()
    string(APPEND current "${char}")
    math(EXPR scan "${scan} + 1")
  endwhile()
  if(NOT current STREQUAL "")
    string(REGEX REPLACE ";" "\\\\;" current "${current}")
    list(APPEND result "${current}")
  endif()
  set(${out} "${result}" PARENT_SCOPE)
endfunction()

function(_pyc_link_closure target out)
  set(acc "")
  set(pending "${target}")
  set(seen "")
  # Terminate on the empty string, not on truthiness: a queued target
  # named OFF (or NO, IGNORE, etc.) would otherwise end the walk early
  # and skip its outgoing edges (r8 review follow-up).
  while(NOT pending STREQUAL "")
    list(POP_FRONT pending head)
    if(head IN_LIST seen)
      continue()
    endif()
    # Normalize ALIAS targets first: an indirect edge through an alias must
    # be attributed to the aliased layer target (r8 review fix).
    if(TARGET "${head}")
      get_target_property(_pyc_aliased "${head}" ALIASED_TARGET)
      if(_pyc_aliased)
        set(head "${_pyc_aliased}")
      endif()
    endif()
    list(APPEND seen "${head}")
    list(APPEND acc "${head}")
    if(NOT TARGET "${head}")
      continue()
    endif()
    get_target_property(iface "${head}" INTERFACE_LINK_LIBRARIES)
    # The seed target's own private link line is examined too (IX): a
    # forbidden edge must not survive as a non-interface dependency of the
    # layer target itself. Missing properties and false-constant values
    # (e.g. an entry named OFF) are handled explicitly — a truthiness test
    # would silently drop a real dependency line (r8 review follow-up).
    get_target_property(priv "${head}" LINK_LIBRARIES)
    # get_target_property returns the link line as a CMake list, so a
    # semicolon inside $<BUILD_INTERFACE:A;B> arrives as a list separator:
    # re-join the raw entries with ';' and run ONE depth-aware split over
    # the whole line (r8 review follow-up).
    set(ifaceRaw "")
    set(privRaw "")
    if(NOT iface STREQUAL "iface-NOTFOUND" AND NOT iface STREQUAL "")
      foreach(raw IN LISTS iface)
        string(APPEND ifaceRaw "${raw};")
      endforeach()
    endif()
    if(NOT priv STREQUAL "priv-NOTFOUND" AND NOT priv STREQUAL "")
      foreach(raw IN LISTS priv)
        string(APPEND privRaw "${raw};")
      endforeach()
    endif()
    set(ifaceEntries "")
    set(privEntries "")
    _pyc_split_link_entries("${ifaceRaw}" ifaceEntries)
    _pyc_split_link_entries("${privRaw}" privEntries)
    foreach(entries "${ifaceEntries}" "${privEntries}")
      foreach(entry IN LISTS entries)
          # Generator expressions may carry real target names
          # ($<LINK_ONLY:ACIRDialect>, $<BUILD_INTERFACE:x>); strip the
          # wrappers and examine every remaining token so hidden forbidden
          # edges cannot survive inside an expression.
          # Reduce generator expressions STRUCTURALLY (r8 review batch 3
          # follow-up): an expression is $<OP:BODY> with OP ending at the
          # first depth-0 colon. Condition operators (CONFIG, TARGET_EXISTS,
          # ...) do not link their arguments — only the BODY is the
          # conditionally linked result; LINK_ONLY/BUILD_INTERFACE/
          # INSTALL_INTERFACE carry their whole argument as content; and a
          # TARGET_PROPERTY (or any other value-producing operator) cannot
          # be resolved statically, so the configure fails with the exact
          # target and expression instead of silently missing the edge.
          set(remainder "${entry}")
          while(remainder MATCHES "\\$<")
            string(FIND "${remainder}" "$<" openAt)
            string(LENGTH "${remainder}" remainderLength)
            string(SUBSTRING "${remainder}" 2 -1 afterOpen)
            set(depth 1)
            set(cursor 2)
            set(closeAt -1)
            string(LENGTH "${afterOpen}" scanLength)
            set(scan 0)
            while(scan LESS scanLength)
              string(SUBSTRING "${afterOpen}" ${scan} 2 pair)
              string(SUBSTRING "${afterOpen}" ${scan} 1 char)
              if(pair STREQUAL "$<")
                math(EXPR depth "${depth} + 1")
                math(EXPR scan "${scan} + 2")
                continue()
              endif()
              if(char STREQUAL ">")
                math(EXPR depth "${depth} - 1")
                if(depth EQUAL 0)
                  math(EXPR closeAt "${cursor} + ${scan}")
                  break()
                endif()
              endif()
              math(EXPR scan "${scan} + 1")
            endwhile()
            if(closeAt EQUAL -1)
              message(FATAL_ERROR
                "layering walk: ${head} carries the unbalanced generator "
                "expression '${entry}'")
            endif()
            math(EXPR innerStart 2)
            math(EXPR innerLength "${closeAt} - 2")
            string(SUBSTRING "${remainder}" 2 ${innerLength} inner)
            # Split the inner text at the first depth-0 colon.
            set(iDepth 1)
            set(splitAt -1)
            string(LENGTH "${inner}" innerLength2)
            set(scan 0)
            while(scan LESS innerLength2)
              string(SUBSTRING "${inner}" ${scan} 2 pair)
              string(SUBSTRING "${inner}" ${scan} 1 char)
              if(pair STREQUAL "$<")
                math(EXPR iDepth "${iDepth} + 1")
                math(EXPR scan "${scan} + 2")
                continue()
              endif()
              if(char STREQUAL ">")
                math(EXPR iDepth "${iDepth} - 1")
              endif()
              if(char STREQUAL ":" AND iDepth EQUAL 1)
                set(splitAt ${scan})
                break()
              endif()
              math(EXPR scan "${scan} + 1")
            endwhile()
            if(splitAt EQUAL -1)
              set(op "${inner}")
              set(body "")
            else()
              string(SUBSTRING "${inner}" 0 ${splitAt} op)
              math(EXPR bodyStart "${splitAt} + 1")
              math(EXPR bodyLength "${innerLength2} - ${bodyStart}")
              string(SUBSTRING "${inner}" ${bodyStart} ${bodyLength} body)
            endif()
            # Only explicitly supported forms are parsed (r8 review batch 3
            # follow-up): content operators carry their argument as linked
            # content; value-conditional operators keep their first argument
            # as the conditionally linked result; pure conditions contribute
            # nothing; IF contributes its true/false branches after dropping
            # the condition argument. Everything else — TARGET_PROPERTY,
            # string transforms, unknown operators — cannot be resolved
            # statically and fails the configure with the exact target and
            # expression.
            if(op MATCHES "\\$<")
              # The operator position is itself a nested condition genex
              # ($<$<TARGET_EXISTS:x>:y>): drop it and keep the result.
              set(replacement "${body}")
            elseif(op STREQUAL "TARGET_PROPERTY")
              message(FATAL_ERROR
                "layering walk: ${head} carries '${entry}' whose "
                "TARGET_PROPERTY result cannot be resolved statically; "
                "replace it on a layer link line")
            elseif(op MATCHES "^(LINK_ONLY|BUILD_INTERFACE|INSTALL_INTERFACE)$")
              set(replacement "${body}")
            elseif(op STREQUAL "IF")
              # $<IF:cond,true,false>: drop the condition, keep both
              # branches (the forbidden edge may sit in either). The split
              # tracks generator-expression depth so a comma inside a nested
              # expression (e.g. a STREQUAL condition) never splits a branch,
              # and the loop terminates on the empty string — branch text
              # like OFF must not be mistaken for the loop's end (r8 review
              # batch 3 follow-up).
              set(branches "${body}")
              set(replacement "")
              set(branchIndex 0)
              while(NOT branches STREQUAL "")
                set(depth 0)
                set(scan 0)
                set(commaAt -1)
                string(LENGTH "${branches}" branchLength)
                while(scan LESS branchLength)
                  string(SUBSTRING "${branches}" ${scan} 2 pair)
                  string(SUBSTRING "${branches}" ${scan} 1 char)
                  if(pair STREQUAL "$<")
                    math(EXPR depth "${depth} + 1")
                    math(EXPR scan "${scan} + 2")
                    continue()
                  endif()
                  if(char STREQUAL ">")
                    math(EXPR depth "${depth} - 1")
                  endif()
                  if(char STREQUAL "," AND depth EQUAL 0)
                    set(commaAt ${scan})
                    break()
                  endif()
                  math(EXPR scan "${scan} + 1")
                endwhile()
                if(commaAt EQUAL -1)
                  set(branch "${branches}")
                  set(branches "")
                else()
                  string(SUBSTRING "${branches}" 0 ${commaAt} branch)
                  math(EXPR commaAt "${commaAt} + 1")
                  string(SUBSTRING "${branches}" ${commaAt} -1 branches)
                endif()
                if(branchIndex GREATER 0)
                  string(APPEND replacement " " "${branch}")
                endif()
                math(EXPR branchIndex "${branchIndex} + 1")
              endwhile()
            elseif(op MATCHES "^(CONFIG|TARGET_EXISTS|BOOL|IN_LIST)$")
              # $<CONFIG:cfg> / $<TARGET_EXISTS:t> / $<BOOL:v> / $<IN_LIST:
              # item,list>: the first argument is the conditionally linked
              # result (IN_LIST drops the list argument).
              set(replacement "${body}")
              if(op STREQUAL "IN_LIST")
                string(FIND "${body}" "," commaAt)
                if(NOT commaAt EQUAL -1)
                  string(SUBSTRING "${body}" 0 ${commaAt} replacement)
                endif()
              endif()
            elseif(op MATCHES "^(NOT|AND|OR|MATCHES|COMPARE_|VERSION_)")
              set(replacement "")
            else()
              message(FATAL_ERROR
                "layering walk: ${head} carries '${entry}' whose operator "
                "'${op}' is not a supported generator-expression form; "
                "replace it on a layer link line")
            endif()
            string(LENGTH "${remainder}" tailLength)
            math(EXPR tailStart "${closeAt} + 1")
            math(EXPR tailLength "${tailLength} - ${tailStart}")
            string(SUBSTRING "${remainder}" ${tailStart} ${tailLength} tail)
            set(remainder "${replacement} ${tail}")
          endwhile()
          # Tokens keep namespace separators (::) inside a target name but
          # split on single colons, so a "LINK_ONLY:X" or ":X" fragment can
          # never pass as the target itself while a "ns::X" dependency stays
          # one name (r8 review batch 3).
          string(REGEX MATCHALL "[A-Za-z0-9_.-]+(::[A-Za-z0-9_.-]+)*" names "${remainder}")
          foreach(name IN LISTS names)
            if(TARGET "${name}")
              get_target_property(_pyc_dep_aliased "${name}" ALIASED_TARGET)
              if(_pyc_dep_aliased)
                set(name "${_pyc_dep_aliased}")
              endif()
              list(APPEND pending "${name}")
            endif()
          endforeach()
          # A layer target mentioned inside an entry that did not resolve to
          # a dependency must not slip through silently (r8 review batch 3).
          foreach(_pyc_layer IN ITEMS GFIRDialect ACIRDialect ACIRAnalysis ACIRCore
              GFIRValidation HWIRValidation ACIRConversionCommon ACIRToGFIR
              GFIRToHWIR)
            if("${entry}" MATCHES "(^|[^A-Za-z0-9_:.${_pyc_layer}])${_pyc_layer}([^A-Za-z0-9_:.]|$)")
              set(_pyc_resolved 0)
              foreach(name IN LISTS names)
                if(name MATCHES "${_pyc_layer}")
                  set(_pyc_resolved 1)
                endif()
              endforeach()
              if(NOT _pyc_resolved)
                message(WARNING
                  "layering walk: ${head} carries '${entry}' which mentions "
                  "layer target ${_pyc_layer} but resolved no dependency for "
                  "it; inspect this expression manually")
              endif()
            endif()
          endforeach()
        endforeach()
    endforeach()
  endwhile()
  set("${out}" "${acc}" PARENT_SCOPE)
endfunction()

function(pycircuit_verify_layering)
  set(forbidden
    "GFIRDialect:ACIRDialect"
    "GFIRDialect:ACIRAnalysis"
    "ACIRCore:ACIRDialect"
    "ACIRCore:ACIRAnalysis"
    "ACIRDialect:ACIRAnalysis"
    "ACIRAnalysis:ACIRTransforms"
    "ACIRAnalysis:ACIRCodegen"
    # The two package-verification targets are independent siblings above
    # ACIRAnalysis (C-package design §5): neither links the other, neither
    # links GFIRDialect (the routed walk compares registered op NAME
    # strings), and neither may grow an upper-layer dependency.
    "GFIRValidation:HWIRValidation"
    "HWIRValidation:GFIRValidation"
    "GFIRValidation:GFIRDialect"
    "HWIRValidation:GFIRDialect"
    "GFIRValidation:ACIRTransforms"
    "GFIRValidation:ACIRCodegen"
    "HWIRValidation:ACIRTransforms"
    "HWIRValidation:ACIRCodegen"
    # The two conversion stages sit between the analysis/validation layer and
    # the integration/emitter layer (C-package design §1.2/B2 tail): each may
    # consume both dialects, the staged analysis and the two validations
    # through the conversion-common helpers, but never each other, never a
    # source transform, never the integration owner and never an emitter.
    "ACIRToGFIR:GFIRToHWIR"
    "GFIRToHWIR:ACIRToGFIR"
    "ACIRToGFIR:ACIRTransforms"
    "GFIRToHWIR:ACIRTransforms"
    "ACIRToGFIR:ACIRSourceCompiler"
    "GFIRToHWIR:ACIRSourceCompiler"
    "ACIRToGFIR:ACIRCodegen"
    "GFIRToHWIR:ACIRCodegen")
  foreach(pair IN LISTS forbidden)
    string(REPLACE ":" ";" ends "${pair}")
    list(GET ends 0 lower)
    list(GET ends 1 upper)
    if(NOT TARGET "${lower}")
      continue()
    endif()
    _pyc_link_closure("${lower}" closure)
    if("${upper}" IN_LIST closure)
      message(FATAL_ERROR
        "layering violation: ${lower} transitively links ${upper}")
    endif()
  endforeach()
  # The prospective conversion DAG is enforced positively as well: each
  # conversion stage must actually consume its declared analysis and
  # dialect/validation inputs, not merely avoid the forbidden edges.
  set(required
    "ACIRToGFIR:ACIRAnalysis"
    "ACIRToGFIR:GFIRDialect"
    "GFIRToHWIR:GFIRDialect"
    "GFIRToHWIR:HWIRValidation")
  foreach(pair IN LISTS required)
    string(REPLACE ":" ";" ends "${pair}")
    list(GET ends 0 lower)
    list(GET ends 1 upper)
    if(NOT TARGET "${lower}")
      continue()
    endif()
    _pyc_link_closure("${lower}" closure)
    if(NOT "${upper}" IN_LIST closure)
      message(FATAL_ERROR
        "layering violation: ${lower} does not link required ${upper}")
    endif()
  endforeach()
  message(STATUS "verify-layering: layer directions OK")
endfunction()
