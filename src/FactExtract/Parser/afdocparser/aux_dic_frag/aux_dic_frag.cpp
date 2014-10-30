#include <library/archive/yarchive.h>
#include <util/generic/singleton.h>
#include "aux_dic_frag.h"

static const unsigned char dicArchive[] = {
    #include "aux_dic.inc"
};

class TAuxDicReader {
public:
    TAuxDicReader()
        : Reader(TBlob::NoCopy(dicArchive, sizeof(dicArchive)))
    {
    }

    Stroka GetAuxDicByName(const Stroka& fileName) {
        return Reader.ObjectByKey("/" + fileName).Get()->ReadAll();
    }

private:
    TArchiveReader Reader;
};

Stroka GetAuxDicByName(const Stroka& fileName) {
    return Singleton<TAuxDicReader>()->GetAuxDicByName(fileName);
}

