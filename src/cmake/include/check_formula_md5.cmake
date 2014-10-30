# =============================================================================
# CHECK_FORMULA_MD5
# md5 of file_name must be equal of md5 stored in first line of file file_name.factors.
#
MACRO(CHECK_FORMULA_MD5 file_name)
    RUN_PROGRAM(tools/check_formula_md5 -f ${file_name} -m ${file_name}.factors
        CWD ${CMAKE_CURRENT_BINARY_DIR} IN ${file_name}.factors ${file_name} STDOUT gen_file.unused.h
    )
ENDMACRO ()
