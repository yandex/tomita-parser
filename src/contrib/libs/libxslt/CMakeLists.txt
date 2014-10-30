LIBRARY()

IF (WIN32)
    CFLAGS(
        -DLIBXSLT_STATIC
        -DLIBXML_STATIC
    )
ENDIF ()

NOUTIL()
NO_COMPILER_WARNINGS()
ADDINCLSELF()

PEERDIR(
    contrib/libs/libxml
)

SRCS(
   attributes.c
   attrvt.c
   documents.c
   extensions.c
   extra.c
   functions.c
   imports.c
   keys.c
   namespaces.c
   numbers.c
   pattern.c
   preproc.c
   security.c
   templates.c
   transform.c
   variables.c
   xslt.c
   xsltlocale.c
   xsltutils.c
)

END()
