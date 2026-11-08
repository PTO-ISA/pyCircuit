find_package(GTest CONFIG REQUIRED)

# component-split: temporary probe-only test graph for the core/dialect
# component. Superseded by pr/analysis (ACIRAnalysisStandaloneTests),
# pr/validation (GFIR/HWIRValidationStandaloneTests) and pr/integration
# (the full api/lit/runtime test inventory).

add_executable(ACIRCoreStandaloneTests LayerProbesTest.cpp)
target_compile_definitions(ACIRCoreStandaloneTests PRIVATE
  PYCIRCUIT_CORE_PROBE=1)
target_include_directories(ACIRCoreStandaloneTests PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib")
target_link_libraries(ACIRCoreStandaloneTests PRIVATE
  "$<LINK_LIBRARY:WHOLE_ARCHIVE,ACIRCore>" GTest::gtest_main)
add_test(NAME ACIRCoreStandaloneTests COMMAND ACIRCoreStandaloneTests)
set_tests_properties(ACIRCoreStandaloneTests PROPERTIES LABELS "api;gate;nightly")

add_executable(ACIROpsStandaloneTests LayerProbesTest.cpp)
target_include_directories(ACIROpsStandaloneTests PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib")
target_link_libraries(ACIROpsStandaloneTests PRIVATE
  ACIRDialect GTest::gtest_main)
add_test(NAME ACIROpsStandaloneTests COMMAND ACIROpsStandaloneTests)
set_tests_properties(ACIROpsStandaloneTests PROPERTIES LABELS "api;gate;nightly")

add_executable(ACIRGFIRStandaloneTests GFIRStandaloneTest.cpp)
target_link_libraries(ACIRGFIRStandaloneTests PRIVATE
  GFIRDialect GTest::gtest_main)
add_test(NAME ACIRGFIRStandaloneTests COMMAND ACIRGFIRStandaloneTests)
set_tests_properties(ACIRGFIRStandaloneTests PROPERTIES LABELS "api;gate;nightly")
