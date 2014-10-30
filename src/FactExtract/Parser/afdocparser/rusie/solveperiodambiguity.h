#pragma once

#include <util/generic/vector.h>
#include <FactExtract/Parser/afglrparserlib/ahokorasick.h>

void SolveAmbiguity (yvector< COccurrence >& Occurrences);

//non-dropping version, puts best occurrences indices into @res
void SolveAmbiguity(yvector<COccurrence>& Occurrences, yvector<size_t>& res);

//dropping version, also returns dropped occurrences
void SolveAmbiguity(yvector<COccurrence>& Occurrences, yvector<COccurrence>& DroppedOccurrences);
