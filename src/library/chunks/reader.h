#pragma once

#include <util/generic/vector.h>
#include <util/generic/yexception.h>

class TBlob;

class TChunkedDataReader {
    public:
        TChunkedDataReader(const TBlob& blob);

        inline const void* GetBlock(size_t index) const {
            CheckIndex(index);
            return Offsets[index];
        }

        inline size_t GetBlockLen(size_t index) const {
            CheckIndex(index);

            if (Version == 0) {
                if (index + 1 < Offsets.size()) {
                    return Offsets[index + 1] - Offsets[index];
                }

                return Size - (Offsets.back() - Offsets.front());
            }

            return Lengths[index];
        }

        inline size_t GetBlocksCount() const {
            return Offsets.size();
        }

    private:
        inline void CheckIndex(size_t index) const {
            if (index >= GetBlocksCount()) {
                ythrow yexception() << "requested block " << index << " of " << GetBlocksCount() << " blocks";
            }
        }

    private:
        ui64 Version;
        yvector<const char*> Offsets;
        yvector<size_t> Lengths;
        size_t Size;
};
