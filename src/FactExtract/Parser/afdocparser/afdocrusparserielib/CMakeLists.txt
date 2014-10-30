LIBRARY()

PEERDIR(
    contrib/libs/libxml
    dict/dictutil
    FactExtract/Parser/afdocparser/afdocruspureparserlib
    FactExtract/Parser/afdocparser/aux_dic_frag
    FactExtract/Parser/afdocparser/builtins
    FactExtract/Parser/afdocparser/rusie/facts
    FactExtract/Parser/afglrparserlib
    FactExtract/Parser/auxdictlib
    FactExtract/Parser/common
    FactExtract/Parser/inflectorlib
    FactExtract/Parser/geo_hierarchy
    FactExtract/Parser/surname_predictor
    FactExtract/Parser/tomalanglib # for on-the-fly compilation
    FactExtract/Parser/lemmerlib
    kernel/gazetteer
    kernel/gazetteer/config
    kernel/gazetteer/common
    library/archive
    library/json
    library/iter
    library/lemmer/dictlib
    library/xsltransform
    library/protobuf/json
)

IF (NOT DEFINED TOMITA_EXTERNAL)
    PEERDIR(
        FactExtract/Parser/tag_lib/tomita
    )
ENDIF ()

ARCHIVE(
    NAME afdump_xsl.inc
    ${CURDIR}/../rusie/AfDump.xsl
)

SRCS(
    afdocrusie.cpp
    ../rusie/gztwrapper.cpp
    ../rusie/afdocplaintextwriter.cpp
    ../rusie/leadgenerator.cpp
    ../rusie/prettysitwriter.cpp
    ../rusie/multiwordcreator.cpp
    ../rusie/prettywriter.cpp
    ../rusie/normalization.cpp
    ../rusie/analyticformbuilder.cpp
    ../rusie/parseroptions.cpp
    ../rusie/anaphoraresolution.cpp
    ../rusie/periodtree.cpp
    ../rusie/clausenode.cpp
    ../rusie/primitivegroup.cpp
    ../rusie/clauserules.cpp
    ../rusie/propnameinquotesfinder.cpp
    ../rusie/clausesubtree.cpp
    ../rusie/quotesfinder.cpp
    ../rusie/clausevariant.cpp
    ../rusie/referencesearcher.cpp
    ../rusie/commongrammarinterpretation.cpp
    ../rusie/sentencerusprocessor.cpp
    ../rusie/datechain.cpp
    ../rusie/sitfactinterpretation.cpp
    ../rusie/dictsholder.cpp
    ../rusie/situationssearcher.cpp
    ../rusie/docinsidestatuscluster.cpp
    ../rusie/situationssearcherinclause.cpp
    ../rusie/factfields.cpp
    ../rusie/synchain.cpp
    ../rusie/factgroup.cpp
    ../rusie/syngroup.cpp
    ../rusie/factscollection.cpp
    ../rusie/terminalsassigner.cpp
    ../rusie/factsxmlwriter.cpp
    ../rusie/factsproto.cpp
    ../rusie/textprocessor.cpp
    ../rusie/filtercheck.cpp
    ../rusie/textwordsequence.cpp
    ../rusie/floatingdategroup.cpp
    ../rusie/tomitachainsearcher.cpp
    ../rusie/fragmentationrunner.cpp
    ../rusie/tomitaitemsholder.cpp
    ../rusie/group.cpp
    ../rusie/topclauses.cpp
    ../rusie/groupsubtree.cpp
    ../rusie/clauses.cpp
    ../rusie/keywordsfinder.cpp
    ../rusie/extractedfieldsearcher.cpp
    ../rusie/solveperiodambiguity.cpp
    ../rusie/interview_fact_creator.cpp
    ../normalize/homonym.cpp
    ../normalize/sequence.cpp
    ${ARCADIA_ROOT}/FactExtract/Parser/tomadictscompiler/tdcompiler.cpp #for running tomdictscompiler on-the-fly
)

END()
