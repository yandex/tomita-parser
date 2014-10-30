#include "gramstr.h"


EGrammems Str2Grammem(const char* str)
{
    int i = IsInList_i((const char*)Grammems,GrammemsCount, str);
    return (i == -1) ? GrammemsCount : (EGrammems)i;

}

Wtroka ClauseType2Str(EClauseType ClauseType)
{
    YASSERT(ClauseType >= 0 && ClauseType < ClauseTypesCount);
    return UTF8ToWide(ClauseTypeNames[ClauseType]);
}

Wtroka Modality2Str(EModality Modality)
{
    YASSERT(Modality >= 0 && Modality < ModalityCount);
    return UTF8ToWide(ModalityStr[Modality]);
}

