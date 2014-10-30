#include "iterator.h"

#include <cstring>

static int SortFTSENTByName(const FTSENT** a, const FTSENT** b) {
    return strcmp((*a)->fts_name, (*b)->fts_name);
}

TDirIterator::TOptions& TDirIterator::TOptions::SetSortByName() throw () {
    return SetSortFunctor(SortFTSENTByName);
}
