#include <util/folder/dirut.h>
#include <util/string/printf.h>

#include "rmltar.h"
#include "tar.h"
#include <FactExtract/Parser/common/utilit.h>


const char GNU_LONGNAME_TYPE    = 'L';
const char GNU_LONGLINK_TYPE    = 'K';
const int T_BLOCKSIZE            = 512;

#define BIT_ISSET(bitmask, bit) ((bitmask) & (bit))
// macros for reading/writing tarchive blocks

// determine full path name
Stroka tar_header::th_get_pathname() const
{
    if (gnu_longname)
        return gnu_longname;

    if (prefix[0] != '\0')
        return Sprintf("%.155s/%.100s", prefix, name);

    return Sprintf("%.100s", name);
}

struct TAR
{
    Stroka pathname;
    FILE* m_pFile;
    tar_header th_buf;

    size_t tar_block_read(void *buf)  const {
        return fread(buf,  1, T_BLOCKSIZE, m_pFile);
    };

    int th_read_internal();
    int th_read();
    void th_print_long_ls() const;
    int    tar_extract_regfile(const char *realname);
    int    tar_extract_regfile_to_buffer(yvector<char>& Buffer);
    int tar_extract_file(const char *realname);
    int tar_skip_regfile();
};

//=====================================================
//===========  tar_header ===================================
//=====================================================

void tar_header::set_empty()
{
    memset(this, 0, sizeof(struct tar_header));
}

tar_header::tar_header()
{
    set_empty();
}
bool tar_header::IsDirectory() const
{
    return       (typeflag == DIRTYPE)
            || (typeflag == AREGTYPE
                    &&  (name[strlen(name) - 1] == '/')
                );
};

bool tar_header::IsRegFile() const
{
    return        (typeflag == REGTYPE)
            ||  (typeflag == AREGTYPE)
            ||  (typeflag == CONTTYPE);

};
bool tar_header::IsLongName() const
{
    return        typeflag == GNU_LONGNAME_TYPE;
};
bool tar_header::IsLongLink() const
{
    return        typeflag == GNU_LONGLINK_TYPE;
};
int tar_header::GetSize() const
{
    int i;
    sscanf(size, "%o", &i);
    return i;
}

//=====================================================
//===========  TAR ===================================
//=====================================================

int TAR::th_read_internal()
{
    int num_zero_blocks = 0;
    int i;

    while ((i=tar_block_read(&(th_buf))) == T_BLOCKSIZE) {
        // two all-zero blocks mark EOF
        if (th_buf.name[0] == '\0') {
            num_zero_blocks++;
            if (num_zero_blocks >= 2)
                return 0;    // EOF
            else
                continue;
        }

        break;
    }

    return i;
}

// wrapper function for th_read_internal() to handle GNU extensions
int TAR::th_read()
{
    int i, j;
    char *ptr;

    if (th_buf.gnu_longname != NULL) {
        delete []th_buf.gnu_longname;
        th_buf.gnu_longname = 0;
    }
    if (th_buf.gnu_longlink != NULL) {
        delete []th_buf.gnu_longlink;
        th_buf.gnu_longlink = 0;
    };

    th_buf.set_empty();

    i = th_read_internal();
    if (i == 0)
        return 1;
    else if (i != T_BLOCKSIZE) {
        return -1;
    }

    // check for GNU long link extention
    if (th_buf.IsLongLink()) {
        size_t sz = th_buf.GetSize();
        j = (sz / T_BLOCKSIZE) + (sz % T_BLOCKSIZE ? 1 : 0);
        th_buf.gnu_longlink = new char[j * T_BLOCKSIZE];
        if (th_buf.gnu_longlink == NULL)
            return -1;

        for (ptr = th_buf.gnu_longlink; j > 0;
             j--, ptr += T_BLOCKSIZE) {
            if (!tar_block_read(ptr)) {
                return -1;
            }
        }

        i = th_read_internal();
        if (i != T_BLOCKSIZE) {
            return -1;
        }
    }

    // check for GNU long name extention
    if (th_buf.IsLongName()) {
        size_t sz = th_buf.GetSize();
        j = (sz / T_BLOCKSIZE) + (sz % T_BLOCKSIZE ? 1 : 0);
        th_buf.gnu_longname = new char[j * T_BLOCKSIZE];
        if (th_buf.gnu_longname == NULL)
            return -1;

        for (ptr = th_buf.gnu_longname; j > 0;
             j--, ptr += T_BLOCKSIZE) {
            if (!tar_block_read(ptr)) {
                return -1;
            }
        }

        i = th_read_internal();
        if (i != T_BLOCKSIZE) {
            return -1;
        }
    }

    return 0;
}

/* util/folder/dirut.h:mkpath used instead
// mkdirhier() - create all directories in a given path
bool mkdirhier(const char *path)
{
    Stroka dst;
    StringTokenizer tok(path, "/\\");
    bool bResult = true;
    while (tok())
    {
        if (!dst.empty())
            dst+="/";
        dst += tok.val();
        bResult = (_mkdir(dst.c_str()) != -1);
    }

    return bResult;
}
*/

// extract regular file
int TAR::tar_extract_regfile(const char *realname)
{
    size_t size;
    int i;
    char buf[T_BLOCKSIZE];
    //const char *filename = realname;
    size = th_buf.GetSize();

    FILE * fdout = fopen (realname, "wb");
    if (fdout == 0) {
        return -1;
    }

    // extract the file
    for (i = size; i > 0; i -= T_BLOCKSIZE) {
        size_t count = ((i > T_BLOCKSIZE) ? T_BLOCKSIZE : i);
        if (!tar_block_read(buf)) {
            return -1;
        }

        // write block to output file
        if (fwrite(buf,1,count ,fdout) != count)
            return -1;
    }

    // close output file
    fclose(fdout);

    return 0;
}

// extract regular file to buffer
int TAR::tar_extract_regfile_to_buffer(yvector<char>& Buffer)
{
    char buf[T_BLOCKSIZE];
    size_t size = th_buf.GetSize();
    Buffer.clear();
    Buffer.reserve(size);

    // extract the file
    for (int i = size; i > 0; i -= T_BLOCKSIZE) {
        size_t count = ((i > T_BLOCKSIZE) ? T_BLOCKSIZE : i);
        if (!tar_block_read(buf)) {
            return -1;
        }
        Buffer.insert(Buffer.end(), buf,buf+count);
    }

    return 0;
}

// switchboard
int TAR::tar_extract_file(const char *realname)
{
    if (th_buf.IsDirectory()) {
        Stroka namecopy = realname;
        return (mkpath(namecopy.begin()) != -1)? 0 : 1;
    }

    if (!th_buf.IsRegFile()) {
        fprintf (stderr, "unsopported  entity in the tar file (a special link, for example)");
        return 1;
    };

    return  tar_extract_regfile(realname);
}

int TAR::tar_skip_regfile()
{
    size_t size = th_buf.GetSize();;
    char buf[T_BLOCKSIZE];

    for (int i = size; i > 0; i -= T_BLOCKSIZE) {
        if (tar_block_read(buf)) {
            return -1;
        }
    }

    return 0;
}

void TAR::th_print_long_ls() const
{
    printf("%9ld ", (long)th_buf.GetSize());
    printf(" %s", th_buf.th_get_pathname().c_str());
    putchar('\n');
}

int tar_open(TAR **t, const char *pathname)
{
    *t = new TAR;
    if (*t == NULL)
        return -1;

    (*t)->pathname = pathname;

    (*t)->m_pFile = fopen(pathname, "rb");
    if ((*t)->m_pFile == 0) {
        delete *t;
        *t = 0;
        return -1;
    }

    return 0;
}

// close tarfile handle
int tar_close(TAR *t)
{
    fclose(t->m_pFile);
    delete t;
    return 0;
}

bool tar_get_next_file(TAR *t, Stroka& FileName, yvector<char>& Result, bool& bError)
{
    bError = false;
    while (t->th_read() == 0) {
        if (t->th_buf.IsRegFile()) {
            FileName = t->th_buf.th_get_pathname();
            if (t->tar_extract_regfile_to_buffer(Result) != 0) {
                bError = true;
                return false;
            } else
                return true;
        };
    };

    return false;
};
/*

int tar_open(TAR **t, const char *pathname)
{
    return 0;
}



// close tarfile handle
int tar_close(TAR *t)
{
    return 0;
}


bool tar_get_next_file(TAR *t, Stroka& FileName, vector<char>& Result, bool& bError)
{

    return false;
};*/
