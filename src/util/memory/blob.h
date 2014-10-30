#pragma once

#include <util/system/defaults.h>
#include <util/generic/utility.h>

class Stroka;
class TMemoryMap;
class TInputStream;
class TFile;
class TBuffer;

/// @addtogroup BLOBs
/// @{

class TBlob {
    public:
        class TBase {
            public:
                inline TBase() throw () {}
                virtual ~TBase() throw () {}

                virtual void Ref() throw () = 0;
                virtual void UnRef() throw () = 0;
        };

    private:
        struct TStorage {
            const void* Data;
            size_t Length;
            TBase* Base;

            inline TStorage(const void* data, size_t length, TBase* base) throw ()
                : Data(data)
                , Length(length)
                , Base(base)
            {
            }

            inline ~TStorage() throw () {
            }

            inline void Swap(TStorage& r) throw () {
                DoSwap(Data, r.Data);
                DoSwap(Length, r.Length);
                DoSwap(Base, r.Base);
            }
        };

    public:
        typedef unsigned char value_type;
        typedef const value_type& const_reference;
        typedef const value_type* const_pointer;
        typedef const_pointer const_iterator;

        /*
         * construct null blob
         */
        TBlob() throw ();

        inline TBlob(const TBlob& r) throw ()
            : S_(r.S_)
        {
            Ref();
        }

        inline TBlob(const void* data, size_t length, TBase* base) throw ()
            : S_(data, length, base)
        {
            Ref();
        }

        inline ~TBlob() throw () {
            UnRef();
        }

        inline TBlob& operator= (const TBlob& r) throw () {
            TBlob(r).Swap(*this);

            return *this;
        }

        /// Обменивает содержимое двух массивов данных.
        inline void Swap(TBlob& r) throw () {
            S_.Swap(r.S_);
        }

        /// Возвращает ссылку на неизменяемый массив данных.
        inline const void* Data() const throw () {
            return S_.Data;
        }

        /// Возвращает размер массива данных в байтах.
        inline size_t Length() const throw () {
            return S_.Length;
        }

        /// Проверяет, содержит ли объект непустой массив данных.
        inline bool Empty() const throw () {
            return !Length();
        }

        /// Проверяет, содержит ли объект массив данных.
        inline bool IsNull() const throw () {
            return !Data();
        }

        /// Возвращает массив данных в виде ссылки на неизменяемый массив символов char.
        inline const char* AsCharPtr() const throw () {
            return (const char*)Data();
        }

        /// Возвращает массив данных в виде ссылки на неизменяемый массив символов unsigned char.
        inline const unsigned char* AsUnsignedCharPtr() const throw () {
            return (const unsigned char*)Data();
        }

        /// Удаляет массив данных.
        inline void Drop() throw () {
            TBlob().Swap(*this);
        }

        /*
         * some stl-like methods
         */

        /// Возвращает размер массива данных в байтах.
        inline size_t Size() const throw () {
            return Length();
        }

        /// Стандартный итератор.
        inline const_iterator Begin() const throw () {
            return AsUnsignedCharPtr();
        }

        /// Стандартный итератор.
        inline const_iterator End() const throw () {
            return Begin() + Size();
        }

        inline value_type operator[] (size_t n) const throw () {
            return *(Begin() + n);
        }

        inline const char* operator~ () const throw () {
            return AsCharPtr();
        }

        inline size_t operator+ () const throw () {
            return Size();
        }

        /*
         * return [0, len) sub blob
         */
        /// Вызывает SubBlob(0, len)
        TBlob SubBlob(size_t len) const;

        /*
         * return [begin, end) sub blob
         */
        /// Создает новый объект из указанного диапазона внутренних данных, без выделения памяти и копирования.
        /// @details Счетчик ссылок текущего объекта увеличивается
        TBlob SubBlob(size_t begin, size_t end) const;

        /*
         * make deep copy of blob
         */
        /// Вызывает Copy() для внутренних данных
        TBlob DeepCopy() const;

        /*
         * dynamically allocate data for new blob
         */
        /// Cоздает объект с однопоточным счетчиком ссылок, динамически выделяя память и копируя данные.
        static TBlob CopySingleThreaded(const void* data, size_t length);

        /*
         * dynamically allocate data for new blob, with atomic refcounter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок, динамически выделяя память и копируя данные.
        static TBlob Copy(const void* data, size_t length);

        /*
         * make blob without data copying
         */
        /// Создает объект, не владеющий данными - без счетчика ссылок, выделения памяти и копирования данных.
        static TBlob NoCopy(const void* data, size_t length);

        /*
         * make blob from file content
         */
        /// Cоздает объект с однопоточным счетчиком ссылок, маппируя указанный файл.
        static TBlob FromFileSingleThreaded(const Stroka& path);

        /*
         * make blob from file content, with atomic counter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок, маппируя указанный файл.
        static TBlob FromFile(const Stroka& path);

        /*
         * make blob from file content
         */
        /// Cоздает объект с однопоточным счетчиком ссылок, маппируя указанный файл.
        static TBlob FromFileSingleThreaded(const TFile& file);

        /*
         * make blob from file content, with atomic counter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок, маппируя указанный файл.
        static TBlob FromFile(const TFile& file);

        /*
         * make precharged blob from file content
         */
        /// Cоздает объект с однопоточным счетчиком ссылок, маппируя указанный файл и делая для него precharge.
        static TBlob PrechargedFromFileSingleThreaded(const Stroka& path);

        /*
         * make precharged blob from file content, with atomic counter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок, маппируя указанный файл и делая для него precharge.
        static TBlob PrechargedFromFile(const Stroka& path);

        /*
         * make precharged blob from file content
         */
        /// Cоздает объект с однопоточным счетчиком ссылок, маппируя указанный файл и делая для него precharge.
        static TBlob PrechargedFromFileSingleThreaded(const TFile& file);

        /*
         * make precharged blob from file content, with atomic counter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок, маппируя указанный файл и делая для него precharge.
        static TBlob PrechargedFromFile(const TFile& file);

        /*
         * make blob from mapped file
         */
        /// Cоздает объект с однопоточным счетчиком ссылок из указанной части отмаппированной памяти.
        static TBlob FromMemoryMapSingleThreaded(const TMemoryMap& map, ui64 offset, size_t length);

        /*
         * make blob from mapped file, with atomic counter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок из указанной части отмаппированной памяти.
        static TBlob FromMemoryMap(const TMemoryMap& map, ui64 offset, size_t length);

        /*
         * make blob from file content
         */
        /// Cоздает объект с однопоточным счетчиком ссылок, динамически выделяя память и копируя данные из файла с помощью pread().
        static TBlob FromFileContentSingleThreaded(const Stroka& path);

        /*
         * make blob from file content, with atomic counter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок, динамически выделяя память и копируя данные из файла с помощью pread().
        static TBlob FromFileContent(const Stroka& path);

        /*
         * make blob from file content
         */
        /// Cоздает объект с однопоточным счетчиком ссылок, динамически выделяя память и копируя данные из файла с помощью pread().
        static TBlob FromFileContentSingleThreaded(const TFile& file);

        /*
         * make blob from file content, with atomic counter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок, динамически выделяя память и копируя данные из файла с помощью pread().
        static TBlob FromFileContent(const TFile& file);

        /*
         * make blob from file content
         */
        /// Cоздает объект с однопоточным счетчиком ссылок, динамически выделяя память и копируя данные из указанного диапазона файла с помощью pread().
        static TBlob FromFileContentSingleThreaded(const TFile& file, ui64 offset, size_t length);

        /*
         * make blob from file content, with atomic counter
         */
        /// Cоздает объект с многопоточным счетчиком ссылок, динамически выделяя память и копируя данные из указанного диапазона файла с помощью pread().
        static TBlob FromFileContent(const TFile& file, ui64 offset, size_t length);

        /*
         * make blob from stream content
         */
        static TBlob FromStreamSingleThreaded(TInputStream& in);

        /*
         * make blob from stream content, with atomic counter
         */
        static TBlob FromStream(TInputStream& in);

        /// Cоздает объект с однопоточным счетчиком ссылок, без выделения памяти и копирования.
        /// @details Входной объект становится пустым
        static TBlob FromBufferSingleThreaded(TBuffer& in);

        /// Cоздает объект с многопоточным счетчиком ссылок, без выделения памяти и копирования.
        /// @details Входной объект становится пустым
        static TBlob FromBuffer(TBuffer& in);

        /*
         * make blob from Stroka, do not copy it content
         */
        static TBlob FromStringSingleThreaded(const Stroka& s);

        /*
         * make blob from Stroka, do not copy it content, with atomic counter
         */
        static TBlob FromString(const Stroka& s);

    private:
        inline void Ref() throw () {
            S_.Base->Ref();
        }

        inline void UnRef() throw () {
            S_.Base->UnRef();
        }

    private:
        TStorage S_;
};

/// @}
