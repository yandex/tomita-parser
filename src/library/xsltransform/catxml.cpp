#include <util/generic/stroka.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/stream/str.h>
#include <util/stream/helpers.h>
#include "catxml.h"

/// Copy src to dst possibly skipping xml declaration
static void TransferWithoutDeclaration(TInputStream* src, TOutputStream* dst) {
    Stroka marker(5);

    if (src->Load(marker.begin(), 5) != 5)
        ythrow yexception() << "xml is too short";
    // omit declaration
    if (stroka(marker) == "<?xml")
        src->ReadLine();
    else
        *dst << marker;
    TransferData(src, dst);
}

static void ExtractDeclaration(TInputStream* src, Stroka& declaration, Stroka& rest) {
    Stroka marker(5);

    if (src->Load(marker.begin(), 5) != 5)
        ythrow yexception() << "xml is too short";

    if (stroka(marker) == "<?xml") {
        Stroka line = src->ReadLine();
        size_t pos  = line.find("?>");

        if (pos == Stroka::npos) {
            ythrow yexception() << "invalid xml declaration";
        } else {
            pos += 2;
        }

        declaration = marker + line.substr(0, pos);
        marker = line.substr(pos);
    } else {
        declaration.clear();
    }

    rest = marker + src->ReadAll();
}

void ConcatenateXmlStreams(const char* rootName, const yvector<TInputStream*>& streams, Stroka& result) {
    YASSERT(!streams.empty());

    TStringOutput out(result);
    Stroka marker(5);

    for (size_t i = 0; i < +streams; ++i) {
        TInputStream* stream = streams[i];

        if (i == 0) {
            Stroka decl, text;
            ExtractDeclaration(stream, decl, text);
            out << decl << '<' << rootName << '>' << text;
        } else {
            TransferWithoutDeclaration(stream, &out);
        }
    }

    out << "</" << rootName << ">" << Endl;
}

void ConcatenateXmlStreams(const char* rootName,
                           const TStreamVector& spvec,
                           Stroka& result)
{
    yvector<TInputStream*> vec;
    for (TStreamVector::const_iterator i = spvec.begin(); i != spvec.end(); ++i) {
        vec.push_back(i->Get());
    }
    ConcatenateXmlStreams(rootName, vec, result);
}

static void ConcatenateXmlStreamsImp(TXmlConcatTask& task, TOutputStream& out,
                                     TInputStream* first) {
    if (!task.Streams.empty() && task.Streams[0].Get() == first) {
        TInputStream* s = task.Streams[0].Get();
        Stroka decl, text;
        ExtractDeclaration(s, decl, text);
        out << decl << '<' << task.Element << '>' << text;
        task.Streams.erase(task.Streams.begin());
    } else {
        out << '<' << task.Element << '>';
    }

    for (TStreamVector::const_iterator i = task.Streams.begin();
                                                i != task.Streams.end(); ++i) {
        TransferWithoutDeclaration(i->Get(), &out);
    }

    for (yvector<TXmlConcatTask>::iterator i = task.Children.begin();
                                                i != task.Children.end(); ++i) {
        ConcatenateXmlStreamsImp(*i, out, first);
    }
    out << "</" << task.Element << '>';
}

static TInputStream* FindFirstStream(const TXmlConcatTask& task) {
    if (+task.Streams)
        return task.Streams[0].Get();
    else if (+task.Children) {
        for (size_t i = 0; i < +task.Children; ++i) {
            TInputStream* ret = FindFirstStream(task.Children[i]);
            if (ret)
                return ret;
        }
    }
    return 0;
}

void ConcatenateXmlStreams(const TXmlConcatTask& task, Stroka& result) {
    TStringOutput out(result);
    TXmlConcatTask copy = task;
    TInputStream* first = FindFirstStream(task);
    ConcatenateXmlStreamsImp(copy, out, first);
}

