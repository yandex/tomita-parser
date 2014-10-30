#include "common_constants.h"


const Wtroka ACTANT_LABEL_SYMBOL = UTF8ToWide("Х");

const Wtroka RUS_WORD_I = UTF8ToWide("и");

int IsInList_i(const char* str_list, int count, const char* str)
{
    for (int i = 0; i < count; i++) {
        if (!strcmp(str_list + i* iMaxWordLen, str))
            return i;

    }

    return -1;
}

bool IsInList(const char* str_list, int count, const char* str)
{
    return IsInList_i(str_list, count, str) != -1;
}
