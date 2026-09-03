find_package(GTest CONFIG REQUIRED)

add_executable(ACIRTypesTests TypesTest.cpp)
target_link_libraries(ACIRTypesTests PRIVATE
  ACIRDialect
  MLIRAsmParser
  MLIRIR
  GTest::gtest_main
)
add_test(NAME ACIRTypesTests COMMAND ACIRTypesTests)

add_executable(ACIROpsTests OpsTest.cpp)
target_include_directories(ACIROpsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIROpsTests PRIVATE
  ACIRDialect
  ACIRTransforms
  ACSimDialect
  MLIRAsmParser
  MLIRIndexDialect
  MLIRFuncDialect
  MLIRSCFDialect
  MLIROptLib
  MLIRIR
  MLIRDataLayoutInterfaces
  MLIRSideEffectInterfaces
  GTest::gtest_main
)
add_test(NAME ACIROpsTests COMMAND ACIROpsTests)
