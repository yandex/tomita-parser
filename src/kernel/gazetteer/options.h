#pragma once

#include <kernel/gazetteer/base.pb.h>

#include <util/generic/ptr.h>
#include <util/charset/doccodes.h>

namespace NGzt
{

const ECharset DEFAULT_GZT_ENCODING = CODES_UTF8;

// Gazetteer file level options

class TOptions {
public:
    ECharset Encoding;
    TGztOptions GztOptions;

public:
    TOptions(const TOptions* parent = NULL)
        : Encoding(DEFAULT_GZT_ENCODING)    // encoding is not propagated
    {
        if (parent != NULL && parent->GztOptions.GetPropagate())
            GztOptions.CopyFrom(parent->GztOptions);
    }

    class TBackup {
    public:
        TBackup(THolder<TOptions>& options)
            : Ref(options)
            , Original(options.Release())
        {
            // make a copy (propagate)
            Ref.Reset(new TOptions(Original.Get()));
        }

        ~TBackup() {
            // restore original options
            Ref.Reset(Original.Release());
        }
    private:
        THolder<TOptions>& Ref;
        THolder<TOptions> Original;
    };
};


}   // namespace NGzt
