PROGRAM()

NOUTIL()

NO_COMPILER_WARNINGS()

SRCS(
    src/builtin.c
    src/debug.c
    src/eval.c
    src/format.c
    src/freeze.c
    src/input.c
    src/m4.c
    src/macro.c
    src/output.c
    src/path.c
    src/symtab.c

    src/cpp.cpp
)

PEERDIR(
    contrib/tools/bison/gnulib
)

END()
