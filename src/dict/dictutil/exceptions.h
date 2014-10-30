#pragma once

////////////////////////////////////////////////////////////////////////////////
//
// exceptions.h -- Java-like exceptions for specific error types
//

// Exceptions in this file are inspired by standard Java exceptions in 'java.lang';
// see Java documentation for specific exception usage guidelines:
//
//     http://docs.oracle.com/javase/7/docs/api/java/lang/package-summary.html,
//
// grep for "Exception Summary".

#include <stdexcept>
#include <string>
#include <util/generic/yexception.h>

extern std::string stringPrintf(const char* fmt, ...) printf_format(1, 2);

////////////////////////////////////////////////////////////////////////////////
//
// ExceptionSource
//

class TExSource {
// Constructors
public:
    TExSource(const char* s) : Value(s) {}
    TExSource(const std::string& s) : Value(s) {}
    TExSource(const Stroka& s) : Value(~s) {}
    static TExSource Line(const char* file, int lineNo);
    static TExSource Class(const char className[]);
    static TExSource ClassOrPath(const char className[], const Stroka& path);
    static TExSource Method(const char className[], const char methodName[]);

// Methods
public:
    const char* c_str() const { return Value.c_str(); }
    const std::string& str() const { return Value; }

// Fields
private:
    std::string Value;
};

#define EXSRC_CLASS TExSource::Class(ClassName)
#define EXSRC_LINE  TExSource::Line(__FILE__, __LINE__)
#define EXSRC_METHOD(method) TExSource::Method(ClassName, method)

////////////////////////////////////////////////////////////////////////////////
//
// Exception
//

class TException : public yexception {
// Constructor
public:
    TException(const TExSource& src, const char* fmt, ...) printf_format(3, 4);
};

////////////////////////////////////////////////////////////////////////////////
//
// LogicError
//

class TLogicError : public std::logic_error {
// Constructor
public:
    TLogicError(const TExSource& src, const char* fmt, ...) printf_format(3, 4);
};

////////////////////////////////////////////////////////////////////////////////
//
// InvalidArgumentException
//

class TInvalidArgumentException : public std::invalid_argument {
// Constructor
public:
    TInvalidArgumentException(const TExSource& src, const char* fmt, ...) printf_format(3, 4);

    // Invalid argument 'arg'
    static std::invalid_argument ForArg(const TExSource& src, const char arg[]);

    // Null pointer argument 'arg'
    static std::invalid_argument ForNullArg(const TExSource& src, const char arg[]);
};

////////////////////////////////////////////////////////////////////////////////
//
// InvalidOperationException
//

class TInvalidOperationException : public TLogicError {
// Constructor
public:
    TInvalidOperationException(const TExSource& src)
      : TLogicError(src, "Invalid operation")
    {}
    TInvalidOperationException(const TExSource& src, const char* message)
      : TLogicError(src, "%s", message)
    {}
};

////////////////////////////////////////////////////////////////////////////////
//
// UnsupportedOperationException
//

class TUnsupportedOperationException : public TLogicError {
// Constructor
public:
    TUnsupportedOperationException(const TExSource& src)
      : TLogicError(src, "Unsupported operation")
    {}
    TUnsupportedOperationException(const TExSource& src, const char* message)
      : TLogicError(src, "%s", message)
    {}
};

////////////////////////////////////////////////////////////////////////////////
//
// IOException
//

class TIOException : public TIoException {
// Constructor
public:
    TIOException(const TExSource& src, const char* message) {
        (*this) << src.str() << ": " << message;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// EOFException
//

class TEOFException : public TIOException {
public:
    TEOFException(const TExSource& src)
        : TIOException(src, "End of stream")
    {}
};

////////////////////////////////////////////////////////////////////////////////
//
// DataCorruptedException
//

class TDataCorruptedException : public std::exception {
public:
    TDataCorruptedException(const TExSource& src, const char* fmt, ...) printf_format(3, 4);
    virtual ~TDataCorruptedException() throw() {}

    virtual const char* what() const throw() { return Msg.c_str(); }

private:
    std::string Msg;
};

////////////////////////////////////////////////////////////////////////////////
//
// Check arguments
//

#define CHECK_STATE(cond, ...) \
    if (EXPECT_FALSE(!(cond))) throw TInvalidOperationException(EXSRC_LINE, ##__VA_ARGS__)
#define CHECK_ARG(cond, arg) \
    if (EXPECT_FALSE(!(cond))) throw TInvalidArgumentException::ForArg(EXSRC_LINE, arg)
#define CHECK_ARG_NULL(cond, arg) \
    if (EXPECT_FALSE(!(cond))) throw TInvalidArgumentException::ForNullArg(EXSRC_LINE, arg)

#define UNSUPPORTED_OPERATION() \
    throw TUnsupportedOperationException(EXSRC_LINE)
#define UNSUPPORTED_METHOD(method) \
    throw TUnsupportedOperationException(TExSource::Method(ClassName, method))
