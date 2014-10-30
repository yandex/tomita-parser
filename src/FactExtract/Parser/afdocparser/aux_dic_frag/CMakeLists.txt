LIBRARY()

ARCHIVE(
    NAME aux_dic.inc
    aux_dic_conj.txt
    aux_dic_pred.txt
    aux_dic_prep.txt
    aux_dic_template.txt
)

PEERDIR(
    library/archive
)

SRCS(
    aux_dic_frag.cpp
)

END()
