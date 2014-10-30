#include "gztwrapper.h"


void TWordRusHomonymIterator::operator++()
{
    YASSERT(HomIt.Ok() || PredictedHomonyms.Ok());

    if (HomIt.Ok()) {
        if (Hyphenable && !HyphenChecked && HomIt->IsDictionary()) {
            size_t pos = CurrentLemma.rfind('-');
            //do not split if more then one hyphen
            if (pos != Stroka::npos && CurrentLemma.find('-') == pos) {
                CurrentLemma = CurrentLemma.substr(pos + 1);
                HyphenChecked = true;
                return;
            }
        }

        ++HomIt;
        if (HomIt.Ok()) {
            SetCurrent(&*HomIt);
            HyphenChecked = false;
            return;
        }
    } else {
        ++PredictedHomonyms;
        ++PredictedIndex;
    }

    if (PredictedHomonyms.Ok())
        SetCurrent(PredictedHomonyms->Get());
}

