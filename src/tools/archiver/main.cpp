#include <library/archive/yarchive.h>
#include <library/getopt/opt2.h>

#include <util/digest/md5.h>
#include <util/folder/dirut.h>
#include <util/folder/filelist.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/hex.h>
#include <util/string/subst.h>

#define COLS 10

namespace {
    class THexOutput: public TOutputStream {
    public:
        inline THexOutput(TOutputStream* slave)
            : mC(0)
            , mSlave(slave)
        {
        }

        virtual ~THexOutput() throw () {
        }

    private:
        virtual void DoFinish() {
            mSlave->Write('\n');
            mSlave->Finish();
        }

        virtual void DoWrite(const void* data, size_t len) {
            const char* b = (const char*)data;

            while (len) {
                const unsigned char c = *b;
                char buf[12];
                char* tmp = buf;

                if (mC % COLS == 0) {
                    *tmp++ = ' ';
                    *tmp++ = ' ';
                    *tmp++ = ' ';
                    *tmp++ = ' ';
                }

                if (mC && mC % COLS != 0) {
                    *tmp++ = ',';
                    *tmp++ = ' ';
                }

                *tmp++ = '0';
                *tmp++ = 'x';
                tmp = HexEncode(&c, 1, tmp);

                if ((mC % COLS) == (COLS - 1)) {
                    *tmp++ = ',';
                    *tmp++ = '\n';
                }

                mSlave->Write(buf, tmp - buf);

                --len;
                ++b;
                ++mC;
            }
        }

    private:
        ui64 mC;
        TOutputStream* mSlave;
    };
}

static inline TAutoPtr<TOutputStream> OpenOutput(const Stroka& url) {
    if (url.empty()) {
        return new TBuffered<TFileOutput>(8192, Duplicate(1));
    } else {
        return new TBuffered<TFileOutput>(8192, url);
    }
}

static inline bool IsDelim(char ch) throw () {
    return ch == '/' || ch == '\\';
}

static inline Stroka GetFile(const Stroka& s) {
    const char* e = s.end();
    const char* b = s.begin();
    const char* c = e - 1;

    while (c != b && !IsDelim(*c)) {
        --c;
    }

    if (c != e && IsDelim(*c)) {
        ++c;
    }

    return Stroka(c, e - c);
}

static inline Stroka Fix(Stroka f) {
    if (!f.empty() && IsDelim(f[+f - 1])) {
        f.pop_back();
    }

    return f;
}

static bool Quiet = false;

static inline void Append(TArchiveWriter& w, const Stroka& fname, const Stroka& rname) {
    TMappedFileInput in(fname);

    if (!Quiet)
        Cerr << "--> " << rname << Endl;

    w.Add(rname, &in);
}

static inline void Append(TOutputStream& w, const Stroka& fname, const Stroka& rname) {
    TMappedFileInput in(fname);

    if (!Quiet)
        Cerr << "--> " << rname << Endl;

    TransferData((TInputStream*)&in, &w);
}

namespace {
struct TRec {
    bool Recursive;
    Stroka Path;
    Stroka Prefix;

    TRec() : Recursive(false), Path(), Prefix()
    {}

    inline void Fix() {
        ::Fix(Path);
        ::Fix(Prefix);
    }

    template <typename T>
    inline void Recurse(T& w) const {
        if (IsDir(Path)) {
            DoRecurse(w, "/");
        } else {
            Append(w, Path, Prefix + "/" + GetFile(Path));
        }
    }

    template <typename T>
    inline void DoRecurse(T& w, const Stroka& off) const {
        {
            TFileList fl;

            const char* name;
            const Stroka p = Path + off;

            fl.Fill(~p);

            while ((name = fl.Next())) {
                const Stroka fname = p + name;
                const Stroka rname = Prefix + off + name;

                Append(w, fname, rname);
            }
        }

        if (Recursive) {
            TDirsList dl;

            const char* name;
            const Stroka p = Path + off;

            dl.Fill(~p);

            while ((name = dl.Next())) {
                if (strcmp(name, ".") && strcmp(name, "..")) {
                    DoRecurse(w, off + name + "/");
                }
            }
        }
    }
};
}

static Stroka cutFirstSlash(const Stroka& fileName) {
    if (fileName[0] == '/') {
        return fileName.substr(1);
    } else {
        return fileName;
    }
}


static void UnpackArchive(const Stroka& archive) {
    TBlob blob = TBlob::FromFileContentSingleThreaded(~archive);
    TArchiveReader reader(blob);
    const size_t count = reader.Count();
    for (size_t i = 0; i < count; ++i) {
        const Stroka key = reader.KeyByIndex(i);
        const Stroka fileName = cutFirstSlash(key);
        if (!Quiet)
            Cerr << archive << " --> " << fileName << Endl;
        TAutoPtr<TInputStream> in = reader.ObjectByKey(key);
        TBufferedFileOutput out("./" + fileName);
        TransferData(in.Get(), &out);
        out.Finish();
    }
}

static void ListArchive(const Stroka& archive) {
    TBlob blob = TBlob::FromFileContentSingleThreaded(~archive);
    TArchiveReader reader(blob);
    const size_t count = reader.Count();
    for (size_t i = 0; i < count; ++i) {
        const Stroka key = reader.KeyByIndex(i);
        const Stroka fileName = cutFirstSlash(key);
        Cout << fileName << Endl;
    }
}

static void ListArchiveMd5(const Stroka& archive) {
    TBlob blob = TBlob::FromFileContentSingleThreaded(~archive);
    TArchiveReader reader(blob);
    const size_t count = reader.Count();
    for (size_t i = 0; i < count; ++i) {
        const Stroka key = reader.KeyByIndex(i);
        const Stroka fileName = cutFirstSlash(key);
        char md5buf[33];
        Cout << fileName << '\t' << MD5::Stream(reader.ObjectByKey(key).Get(), md5buf) << Endl;
    }
}

int main(int argc, char** argv) {
    Opt2 opt(argc, argv, "rxcpulma:z:qo:", IntRange(1, 999999), "hexdump=x,cat=c,plain=p,unpack=u,list=l,md5=m,recursive=r,quiet=q,prepend=z,append=a,output=o");
    const bool hexdump   = opt.Has('x', "- produce hexdump");
    const bool cat       = opt.Has('c', "- do not store keys (file names), just cat uncompressed files");
    const bool doNotZip  = opt.Has('p', "- do not use compression");
    const bool unpack    = opt.Has('u', "- unpack archive in current dir");
    const bool list      = opt.Has('l', "- list files in archive");
    const bool listMd5   = opt.Has('m', "- list files in archive with md5");
    const bool recursive = opt.Has('r', "- read all files under each directory, recursively");
    Quiet                = opt.Has('q', "- do not output progress on stderr.");
    Stroka prepend = opt.Arg('z', "<PREFIX> - prepend string to output", 0);
    Stroka append  = opt.Arg('a', "<SUFFIX> - append string to output", 0);
    const Stroka outputf = opt.Arg('o', "<FILE> - output to file instead stdout", 0);
    opt.AutoUsageErr("<file>..."); // "Files or directories to archive."

    SubstGlobal(append, "\\n", "\n");
    SubstGlobal(prepend, "\\n", "\n");

    yvector<TRec> recs;
    for (size_t i = 0; i != opt.Pos.size(); ++i) {
        Stroka path = opt.Pos[i];
        size_t off = 0;
#ifdef _win_
        if (path[0] > 0 && isalpha(path[0]) && path[1] == ':')
            off = 2; // skip drive letter ("d:")
#endif // _win_
        const size_t pos = path.find(':', off);
        TRec cur;
        cur.Path = path.substr(0, pos);
        if (pos != Stroka::npos)
            cur.Prefix = path.substr(pos + 1);
        cur.Recursive = recursive;
        cur.Fix();
        recs.push_back(cur);
    }

    try {
        if (listMd5) {
            for (yvector<TRec>::const_iterator it = recs.begin(); it != recs.end(); ++it) {
                ListArchiveMd5(it->Path);
            }
        } else if (list) {
            for (yvector<TRec>::const_iterator it = recs.begin(); it != recs.end(); ++it) {
                ListArchive(it->Path);
            }
        } else if (unpack) {
            for (yvector<TRec>::const_iterator it = recs.begin(); it != recs.end(); ++it) {
                UnpackArchive(it->Path);
            }
        } else {
            TAutoPtr<TOutputStream> outf(OpenOutput(outputf));
            TOutputStream* out = outf.Get();
            THolder<TOutputStream> hexout;

            if (hexdump) {
                hexout.Reset(new THexOutput(out));
                out = hexout.Get();
            }

            outf->Write(~prepend, +prepend);

            if (cat) {
                for (yvector<TRec>::const_iterator it = recs.begin(); it != recs.end(); ++it) {
                    it->Recurse(*out);
                }
            } else {
                TArchiveWriter w(out, !doNotZip);
                for (yvector<TRec>::const_iterator it = recs.begin(); it != recs.end(); ++it) {
                    it->Recurse(w);
                }
                w.Finish();
            }

            outf->Write(~append, +append);

            try {
                out->Finish();
            } catch (...) {
            }
        }
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }

    return 0;
}
