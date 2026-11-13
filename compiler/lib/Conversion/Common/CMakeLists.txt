# ACIRConversionCommon: helpers shared verbatim by both conversion stages
# (budget prepayment, constant-admission folding, atomic candidate
# acceptance). They belong to neither stage: hosting them in one conversion
# target would force the forbidden cross-conversion dependency, and they are
# above ACIRAnalysis (they debit SourceCheckBudget) without belonging to any
# dialect. pycircuit_verify_layering forbids both cross-conversion edges.
add_acir_library(ACIRConversionCommon SOURCES
  ConversionCommon.cpp
)
target_link_libraries(ACIRConversionCommon PUBLIC
  ACIRDialect
  ACIRAnalysis
)
target_include_directories(ACIRConversionCommon PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib"
)
