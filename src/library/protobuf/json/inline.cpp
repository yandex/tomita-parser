#include "inline.h"

#include <util/string/cast.h>

namespace NProtobufJson {

Stroka TInliningPrinter::AddMarker(const Stroka& json) {
    char buf[32];
    TStringBuf index(buf, ::ToString(Entries.size(), buf, sizeof(buf)));
    Stroka marker(STRINGBUF("[[#DYNAMIC_MESSAGE_PLACEHOLDER#"), index, STRINGBUF("#]]"));
    TEntry entry = {json, marker};
    Entries.push_back(entry);
    return marker;
}

void TInliningPrinter::PopMarkers(Stroka& json) const {
    // TODO: slow, make it in single pass
    for (size_t i = 0; i < Entries.size(); ++i)
        ReplaceMarker(json, Entries[i]);
}

bool TInliningPrinter::ReplaceMarker(Stroka& trgJson, const TEntry& entry) const {
    size_t pos = trgJson.find(entry.Marker);
    if (pos == Stroka::npos || pos <= 0)
        return false;

    size_t start = pos - 1;
    size_t stop = pos + entry.Marker.size();

    if (stop < trgJson.size() && trgJson[start] == '"' && trgJson[stop] == '"') {
        trgJson.replace(start, stop - start + 1, entry.Json);
        return true;
    }

    return false;
}



}   // namespace NProtobufJson

