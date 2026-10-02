# Patch the freshly extracted mm.o2r with the five validated native owl rooms.
execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "${SOURCE_DIR}/mm/tools/patch_expanded_owl_rooms.py" "${SOURCE_DIR}/mm/mm.o2r"
    RESULT_VARIABLE OWL_PATCH_RESULT
)
if(NOT OWL_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "Expanded owl room asset patch failed (${OWL_PATCH_RESULT})")
endif()
