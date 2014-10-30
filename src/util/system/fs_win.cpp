#include "fs_win.h"
#include "defaults.h"
#include "maxlen.h"

#include <util/folder/dirut.h>
#include <util/charset/wide.h>
#include <util/system/file.h>

#include <WinIoCtl.h>

static LPCWSTR UTF8ToWCHAR(const TStringBuf& str, Wtroka& wstr) {
    wstr.resize(+str);
    size_t written = 0;
    if (!UTF8ToWide(~str, +str, wstr.begin(), written))
        return NULL;
    wstr.erase(written);
    STATIC_ASSERT(sizeof(WCHAR) == sizeof(wchar16));
    return (const WCHAR*)~wstr;
}

HANDLE CreateFileWithUtf8Name(const TStringBuf& fName, ui32 accessMode, ui32 shareMode, ui32 createMode, ui32 attributes) {
    Wtroka wstr;
    LPCWSTR wname = UTF8ToWCHAR(fName, wstr);
    if (!wname) {
        ::SetLastError(ERROR_INVALID_NAME);
        return INVALID_HANDLE_VALUE;
    }
    SECURITY_ATTRIBUTES secAttrs;
    secAttrs.bInheritHandle = TRUE;
    secAttrs.lpSecurityDescriptor = NULL;
    secAttrs.nLength = sizeof(secAttrs);
    return ::CreateFileW(wname, accessMode, shareMode, &secAttrs, createMode, attributes, NULL);
}

int WinRemove(const char* fName) {
    Wtroka wstr;
    LPCWSTR wname = UTF8ToWCHAR(fName, wstr);
    if (!wname) {
        ::SetLastError(ERROR_INVALID_NAME);
        return -1;
    }
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (::GetFileAttributesExW(wname, GetFileExInfoStandard, &fad)) {
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            return ::RemoveDirectoryW(wname) ? 0 : -1;
        return ::DeleteFileW(wname) ? 0 : -1;
    }
    return -1;
}

bool WinSymLink(const char* targetName, const char* linkName) {
    Stroka tName(targetName);
    {
        size_t pos;
        while ((pos = tName.find('/')) != Stroka::npos)
            tName.replace(pos, 1, LOCSLASH_S);
    }
    // we can't create a dangling link to a dir in this way
    ui32 attr = ::GetFileAttributesA(~tName);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        TTempBuf result;
        if (GetFullPathName(linkName, result.Size(), result.Data(), 0) != 0) {
            const char* p = strrchr(result.Data(), '\\');
            if (p) {
                Stroka linkDir(result.Data(), p);
                Stroka fullTarget(tName);
                resolvepath(fullTarget, linkDir);
                attr = ::GetFileAttributesA(~fullTarget);
            }
        }
    }
    return 0 != CreateSymbolicLinkA(linkName, ~tName, attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0);
}

// edited part of <Ntifs.h> from Windows DDK

#define SYMLINK_FLAG_RELATIVE 1

struct TReparseBufferHeader {
    USHORT SubstituteNameOffset;
    USHORT SubstituteNameLength;
    USHORT PrintNameOffset;
    USHORT PrintNameLength;
};

struct TSymbolicLinkReparseBuffer : public TReparseBufferHeader {
    ULONG Flags; // 0 or SYMLINK_FLAG_RELATIVE
    wchar16 PathBuffer[1];
};

struct TMountPointReparseBuffer : public TReparseBufferHeader {
    wchar16 PathBuffer[1];
};

struct TGenericReparseBuffer {
    wchar16 DataBuffer[1];
};

struct REPARSE_DATA_BUFFER {
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        TSymbolicLinkReparseBuffer SymbolicLinkReparseBuffer;
        TMountPointReparseBuffer MountPointReparseBuffer;
        TGenericReparseBuffer GenericReparseBuffer;
    };
};

// the end of edited part of <Ntifs.h>

Stroka WinReadLink(const char* name) {
    TFileHandle h = CreateFileWithUtf8Name(name, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
                                           FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS);
    TTempBuf buf;
    while (true) {
        DWORD bytesReturned = 0;
        BOOL res = DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, buf.Data(), buf.Size(), &bytesReturned, NULL);
        if (res) {
            REPARSE_DATA_BUFFER* rdb = (REPARSE_DATA_BUFFER*)buf.Data();
            if (rdb->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
                wchar16* str = (wchar16*)&rdb->SymbolicLinkReparseBuffer.PathBuffer[rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(wchar16)];
                size_t len = rdb->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(wchar16);
                return WideToUTF8(str, len);
            } else if (rdb->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
                wchar16* str = (wchar16*)&rdb->MountPointReparseBuffer.PathBuffer[rdb->MountPointReparseBuffer.SubstituteNameOffset / sizeof(wchar16)];
                size_t len = rdb->MountPointReparseBuffer.SubstituteNameLength / sizeof(wchar16);
                return WideToUTF8(str, len);
            }
            //this reparse point is unsupported in arcadia
            return Stroka();
        } else {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                buf = TTempBuf(buf.Size() * 2);
            } else {
                ythrow yexception() << "can't read link " << name;
            }
        }
    }
}

// we can't use this function to get an analog of unix inode due to a lot of NTFS folders do not have this GUID
//(it will be 'create' case really)
/*
bool GetObjectId(const char* path, GUID* id) {
    TFileHandle h = CreateFileWithUtf8Name(path, 0, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS);
    if (h.IsOpen()) {
        FILE_OBJECTID_BUFFER fob;
        DWORD resSize = 0;
        if (DeviceIoControl(h, FSCTL_CREATE_OR_GET_OBJECT_ID, NULL, 0, &fob, sizeof(fob), &resSize, NULL)) {
            YASSERT(resSize == sizeof(fob));
            memcpy(id, &fob.ObjectId, sizeof(GUID));
            return true;
        }
    }
    memset(id, 0, sizeof(GUID));
    return false;
}
*/
