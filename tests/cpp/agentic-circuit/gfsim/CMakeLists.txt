find_package(GTest CONFIG REQUIRED)
add_executable(GfsimTests
  NpuTest.cpp
  QueueBlocksTest.cpp
  ShowcaseTest.cpp
  core_test.cpp
)
target_include_directories(GfsimTests PRIVATE
  ${ACIR_GFSIM_SOURCE_DIR}/include
)
target_compile_definitions(GfsimTests PRIVATE
  ACIR_TEST_SOURCE_DIR="${ACIR_REPO_ROOT}"
)
target_link_libraries(GfsimTests PRIVATE
  gfsim
  GTest::gtest_main
)
add_test(NAME GfsimTests COMMAND GfsimTests)
