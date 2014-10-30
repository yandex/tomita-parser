#pragma once

#include "fs.h"
#include "file.h"

#include <util/generic/stroka.h>

class TTempFile {
    public:
        inline TTempFile(const Stroka& fname)
            : Name_(fname)
        {
        }

        inline ~TTempFile() throw () {
            NFs::Remove(~Name());
        }

        inline const Stroka& Name() const throw () {
            return Name_;
        }

    private:
        const Stroka Name_;
};

class TTempFileHandle: public TTempFile, public TFile {
    public:
        inline TTempFileHandle(const Stroka& fname)
            : TTempFile(fname)
            , TFile(Name(), CreateAlways | RdWr)
        {
        }
};
