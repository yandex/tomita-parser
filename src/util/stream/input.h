#pragma once

#include <util/system/defaults.h>
#include <util/generic/stroka.h>

/// @addtogroup Streams_Base
/// @{

/// Базовый поток ввода.
class TInputStream {
    public:
        TInputStream() throw ();
        virtual ~TInputStream() throw ();

        /*
         * return 0 if end-of-stream, can return less bytes than requested
         * use Load() method if you want to fill input buffer as much as possible
         */
        inline size_t Read(void* buf, size_t len) {
            if (len == 0) {
                return 0;
            }

            return DoRead(buf, len);
        }

        inline bool ReadChar(char& c) {
            return DoRead(&c, 1) > 0;
        }

        /// Считывает из потока все символы до первого вхождения ch и записывает их
        /// в строку st.
        /// @details Возвращает false, если поток не содержит данных и true в
        /// противном случае.
        inline bool ReadTo(Stroka& st, char ch) {
            return DoReadTo(st, ch);
        }

        /// Считывает len символов из потока в buf. Возвращает количество
        /// считанных символов.
        /// @details Чтение прекращается, если прочитано len символов или
        /// достигнут конец файла.
        size_t Load(void* buf, size_t len);

        /// Гарантированно считывает ровно len байт. Падает с ошибкой, если не получилось.
        void LoadOrFail(void* buf, size_t len);

        /// Считывает все данные потока.
        Stroka ReadAll();

        /// Считывает из потока все символы до первого вхождения "\n".
        /// @details Like ReadLine(Stroka) but throws exception on EOF.
        Stroka ReadLine();

        /// Считывает из потока все символы до первого вхождения ch.
        Stroka ReadTo(char ch);

        /// Считывает из потока все символы до первого вхождения "\n" и записывает
        /// их в строку st.
        bool ReadLine(Stroka& st);

        /// Считывает из потока все символы в кодировке UTF8 до первого вхождения "\n", перекодирует и записывает
        /// их в широкую строку w.
        bool ReadLine(Wtroka& w);

        /// Позволяет пропустить часть потока без копирования
        size_t Skip(size_t len);
    protected:
        /**
         * @throws yexception if IO error occurs.
         */
        virtual size_t DoRead(void* buf, size_t len) = 0;
        /**
         * @throws yexception if IO error occurs.
         * @return the true size of the skipped data
         */
        virtual size_t DoSkip(size_t len);
        /**
         * Read stream until given character appears.
         * Character is removed from stream but not stored in stroka.
         *
         * Stroka is cleared iff data available in the stream.
         *
         * @return false iff no data available in the stream (i.e. EOF)
         */
        virtual bool DoReadTo(Stroka& st, char ch);
};

/*
 * common streams
 */
TInputStream& StdInStream() throw ();

template <typename T>
void In(TInputStream& in, T& t);

template <typename T>
inline TInputStream& operator>> (TInputStream& in, T& t) {
    In<T>(in, t);
    return in;
}

#define Cin (StdInStream())

/// @}
