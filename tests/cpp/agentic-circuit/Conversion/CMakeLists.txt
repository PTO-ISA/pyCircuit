find_package(GTest CONFIG REQUIRED)

add_executable(ACIRToACSimTests ACIRToACSimTest.cpp)
target_include_directories(ACIRToACSimTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIRToACSimTests PRIVATE
  ACIRToACSim
  ACIRDialect
  ACSimDialect
  LLVMSupport
  MLIRParser
  MLIRPass
  GTest::gtest_main
)
add_test(NAME ACIRToACSimTests COMMAND ACIRToACSimTests)
