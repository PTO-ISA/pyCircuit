add_subdirectory(Interfaces)
add_subdirectory(Dialect)
add_subdirectory(Transforms)
add_subdirectory(Compiler)
# component-split: superseded by pr/conversion (the ACIRConversionCommon,
# ACIRToGFIR and GFIRToHWIR targets return with the transaction conversion
# component).
# add_subdirectory(Conversion)
