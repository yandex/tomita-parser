TOOL(flex)

OWNER(pg)

NOUTIL()
NO_COMPILER_WARNINGS()
NO_OPTIMIZE()
NOJOINSRC()

SRCS(
    ccl.c
    dfa.c
    ecs.c
    gen.c
    main.c
    misc.c
    nfa.c
    skel.c
    sym.c
    tblcmp.c
    yylex.c
    scan.c
    parse.c
)

END()
