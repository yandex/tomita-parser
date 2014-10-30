#include "exceptions.h"
#include "str.h"
#include <util/string/printf.h>

////////////////////////////////////////////////////////////////////////////////
//
// ExSource (exception source)
//

TExSource TExSource::Line(const char* file, int lineNo) {
    const char* ptr = strstr(file, "arcadia");
    if (ptr) {
        ptr += 7;
        if (*ptr == '/' || *ptr == '\\')
            ++ptr;
    }
    else {
        ptr = file;
    }

    char lineBuf[32];
    sprintf(lineBuf, "%d", lineNo);

    std::string src(ptr);
    src.push_back(':');
    src.append(lineBuf);
    return TExSource(src);
}

TExSource TExSource::Class(const char className[]) {
    return TExSource(className);
}

TExSource TExSource::ClassOrPath(const char className[], const Stroka& path) {
    return TExSource(path.empty() ? className : path.c_str());
}

TExSource TExSource::Method(const char className[], const char methodName[]) {
    Stroka src(className);
    src.append(1, '.').append(methodName);
    return TExSource(src);
}

////////////////////////////////////////////////////////////////////////////////
//
// Exception
//

TException::TException(const TExSource& src, const char* fmt, ...) {
    Stroka msg;
    VA_SPRINTF(msg, fmt, args);
    (*this) << src.c_str() << ": " << msg;
}

////////////////////////////////////////////////////////////////////////////////
//
// LogicError
//

TLogicError::TLogicError(const TExSource& src, const char* fmt, ...)
  : std::logic_error("")
{
    Stroka msg;
    VA_SPRINTF(msg, fmt, args);

    *(std::logic_error*)this = std::logic_error(src.str() + ": " + msg.c_str());
}

////////////////////////////////////////////////////////////////////////////////
//
// InvalidArgumentException
//

TInvalidArgumentException::TInvalidArgumentException(const TExSource& src, const char* fmt, ...)
  : std::invalid_argument("")
{
    Stroka msg;
    VA_SPRINTF(msg, fmt, args);

    *(std::invalid_argument*)this = std::invalid_argument(src.str() + ": " + msg.c_str());
}

std::invalid_argument TInvalidArgumentException::ForArg(const TExSource& src, const char arg[]) {
    return std::invalid_argument(stringPrintf("%s: Invalid argument '%s'", src.c_str(), arg));
}

std::invalid_argument
TInvalidArgumentException::ForNullArg(const TExSource& src, const char arg[]) {
    return std::invalid_argument(stringPrintf("%s: Null pointer argument '%s'", src.c_str(), arg));
}

////////////////////////////////////////////////////////////////////////////////
//
// DataCorruptedException
//

TDataCorruptedException::TDataCorruptedException(const TExSource& src, const char* fmt, ...)
  : Msg(src.str())
{
    Stroka msg;
    VA_SPRINTF(msg, fmt, args);

    Msg.append(": ").append(msg);
}

////////////////////////////////////////////////////////////////////////////////
//
// Helper functions
//

std::string stringPrintf(const char* fmt, ...) {
    Stroka msg;
    VA_SPRINTF(msg, fmt, args);
    return msg.c_str();
}
