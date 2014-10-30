#pragma once

#include <util/system/yassert.h>
#include <util/system/defaults.h>

class TOutputStream;

/// @addtogroup Streams
/// @{

/// Поток ввода с прямым доступом к буферу.
class IZeroCopyInput {
    public:
        virtual ~IZeroCopyInput() {
        }

        /// Считывает из потока блок данных.
        /// @details Указатель на считанные данные записывается в outPtr,
        /// длина блока - в outlen.@n
        /// Возвращает true, если данные считаны успешно и false,
        /// если поток не содержит данных
        template <class T>
        inline bool Next(T** outPtr, size_t* outLen) {
            YASSERT(outPtr && outLen);

            return DoNext((const void**)outPtr, outLen);
        }

    private:
        virtual bool DoNext(const void** outPtr, size_t* outLen) = 0;
};

/// Переносит данные из потока ввода в поток вывода.
ui64 TransferData(IZeroCopyInput* in, TOutputStream* out);

/// @}
