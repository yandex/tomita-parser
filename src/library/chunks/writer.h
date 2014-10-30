#pragma once

#include <util/generic/vector.h>
#include <util/stream/output.h>

template <typename T>
inline void WriteBin(TOutputStream* out, typename TTypeTraits<T>::TFuncParam t) {
    out->Write(&t, sizeof(T));
}

class TChunkedDataWriter: public TOutputStream {
    public:
        TChunkedDataWriter(TOutputStream& slave);
        ~TChunkedDataWriter() throw ();

        void NewBlock();

        template <typename T>
        inline void WriteBinary(typename TTypeTraits<T>::TFuncParam t) {
            this->Write(&t, sizeof(T));
        }

        void WriteFooter();

        size_t GetCurrentBlockOffset() const;
        size_t GetBlockCount() const;

    protected:
        virtual void DoWrite(const void* buf, size_t len) {
            Slave.Write(buf, len);
            Offset += len;
        }

    private:
        static inline size_t PaddingSize(size_t size, size_t boundary) throw () {
            const size_t boundaryViolation = size % boundary;

            return boundaryViolation == 0 ? 0 : boundary - boundaryViolation;
        }

        inline void Pad(size_t boundary) {
            const size_t newOffset = Offset + PaddingSize(Offset, boundary);

            while (Offset < newOffset) {
                Write('\0');
            }
        }

    private:
        static const ui64 Version = 1;

        TOutputStream& Slave;

        size_t Offset;
        yvector<ui64> Offsets;
        yvector<ui64> Lengths;
};
