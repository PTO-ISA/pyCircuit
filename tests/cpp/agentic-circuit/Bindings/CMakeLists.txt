find_package(GTest CONFIG REQUIRED)

add_executable(ACIRBindingTests CanonicalBindingTest.cpp)
target_include_directories(ACIRBindingTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIRBindingTests PRIVATE
  ACIRBindings
  ACIRDialect
  ACIRTransforms
  ACSimDialect
  LLVMSupport
  MLIRFuncDialect
  MLIRIndexDialect
  MLIRParser
  MLIRSCFDialect
  GTest::gtest_main
)
add_test(NAME ACIRBindingTests COMMAND ACIRBindingTests)
