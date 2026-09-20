get_filename_component(PYC_LIT_BIN_DIR "${ACIR_LIT_EXECUTABLE}" DIRECTORY)
find_program(PYC_TEST_PYTHON NAMES python python3
  HINTS "${PYC_LIT_BIN_DIR}"
  REQUIRED
)

configure_lit_site_cfg(
  "${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py"
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/../lit.cfg.py"
)

add_lit_testsuite(check-pyc "Running typed PYC lit tests"
  "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS pyc-opt pycc
)
