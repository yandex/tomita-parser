#pragma once

class Stroka;

namespace NFs {
    int Remove(const char* name);
    int Rename(const char* oldName, const char* newName);
    void HardLinkOrCopy(const char* oldName, const char* newName);
    bool SymLink(const char* targetName, const char* linkName);
    Stroka ReadLink(const char* path);
    void Cat(const char* dstName, const char* srcName);
    void Copy(const char* oldName, const char* newName);
}
