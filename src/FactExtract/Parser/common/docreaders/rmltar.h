/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2003 Mark D. Roth
**  All rights reserved.
**
**  libtar.h - header file for libtar library
**
**  Mark D. Roth <roth@uiuc.edu>
**  Campus Information Technologies and Educational Services
**  University of Illinois at Urbana-Champaign
*/

#ifndef LIBTAR_H
#define LIBTAR_H

#include <util/generic/vector.h>

struct TAR;
/* open a new tarfile handle */
int tar_open(TAR **t, const char *pathname);
/* close tarfile handle */
int tar_close(TAR *t);
/* get next file, it returns false, if Result contains a file, otherwise there is no more files in the tar archive  */
bool tar_get_next_file(TAR *t, Stroka& FileName, yvector<char>& Result, bool& bError);

struct tar_header
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
    char *gnu_longname;
    char *gnu_longlink;

    void set_empty();
    tar_header();
    bool IsDirectory() const;
    bool IsRegFile() const;
    bool IsLongName() const;
    bool IsLongLink() const;
    int GetSize() const;
    Stroka th_get_pathname() const;
};

#endif /* ! LIBTAR_H */
