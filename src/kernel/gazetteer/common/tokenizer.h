#pragma once

#include <util/system/defaults.h>
#include <util/generic/yexception.h>

#include <contrib/libs/protobuf/compiler/parser.h>
#include <contrib/libs/protobuf/io/tokenizer.h>


namespace NGzt {

// Simple tokenizer of cpp-like language
// based on protobuf tokenization.

class TTokenizer {
public:
    typedef NProtoBuf::io::Tokenizer TProtobufTokenizer;
    typedef NProtoBuf::io::Tokenizer::Token TToken;

    TTokenizer(TProtobufTokenizer* slave)
        : Slave(slave)
        , PrevTokenLine(-1)
        , PrevTokenColumnStop(0)
        , NextColumnOffset(0)
    {
        CurrentToken = &slave->current();
    }

    void Next() {
        StoreCurrentPosition();
        Slave->Next();
        CurrentToken = &Slave->current();
    }

    const TToken& Current() const {
        return *CurrentToken;
    }

    void FirstNext() {
        if (CurrentToken->type != TProtobufTokenizer::TYPE_START)
            ythrow yexception() << "Should be called on tokenization start only.";
        Next();
        PrevTokenLine = -1;
        PrevTokenColumnStop = 0;
    }

    void StoreCurrentPosition() {
        YASSERT(CurrentToken != NULL);
        PrevTokenLine = CurrentToken->line;
        PrevTokenColumnStop = CurrentToken->column + CurrentToken->text.size() + NextColumnOffset;
        NextColumnOffset = 0;
    }

    inline bool AtSameLine() const {
        return CurrentToken->line == PrevTokenLine;
    }

    inline size_t SpaceBefore() const {
        if (AtSameLine())
            return Max<int>(0, CurrentToken->column - PrevTokenColumnStop);
        else
            return Max<int>(0, CurrentToken->column);
    }

private:
    TProtobufTokenizer* Slave;

protected:
    const TToken* CurrentToken;
    int PrevTokenLine, PrevTokenColumnStop;
    int NextColumnOffset;
};


}   // namespace NGzt


