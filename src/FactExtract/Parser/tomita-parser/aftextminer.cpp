#include <util/system/progname.h>
#include <FactExtract/Parser/textminerlib/processor.h>
#include "aftextminer.h"

int main(int argc, char* argv[])
{
    SAVE_PROGRAM_NAME;
    CProcessor processor;
    if (!processor.Init(argc, argv)) {
        processor.m_Parm.PrintParameters();
        return 1;
    }
    if (!processor.Run()) {
        return 1;
    }

    return 0;
}
