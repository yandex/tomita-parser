#include "yexception.h"

#include <util/folder/dirut.h>

#include <stlport/string>

#include <stdexcept>

Stroka CurrentExceptionMessage() {
    try {
        throw;
    } catch (const std::exception& e) {
        return e.what();
    } catch (...) {
    }

    return "unknown error";
}

bool UncaughtException() throw () {
    return std::uncaught_exception();
}

void ThrowBadAlloc() {
    throw std::bad_alloc();
}

void ThrowLengthError(const char* descr) {
    throw std::length_error(descr);
}

void ThrowRangeError(const char* descr) {
    throw std::out_of_range(descr);
}

void TSystemError::Init() {
    yexception& exc = *this;

    exc << STRINGBUF("(");
    exc << TStringBuf(LastSystemErrorText(Status()));
    exc << STRINGBUF(") ");
}

template <>
void Out<NPrivateException::TLineInfo>(TOutputStream& o, const NPrivateException::TLineInfo& t) {
    o << t.File << ":" << t.Line << ": ";
}

static inline const char* ZeroTerminate(TTempBuf& buf) {
    char* end = (char*)buf.Current();

    if (!buf.Left()) {
        --end;
    }

    *end = 0;

    return buf.Data();
}

const char* NPrivateException::yexception::what() const throw () {
    return ZeroTerminate(Buf_);
}
