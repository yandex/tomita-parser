#pragma once

class TContIOVector {
        typedef TOutputStream::TPart TPart;
    public:
        inline TContIOVector(TPart* parts, size_t count)
            : Parts_(parts)
            , Count_(count)
        {
        }

        inline void Proceed(size_t len) throw () {
            while (Count_) {
                if (len < Parts_->len) {
                    Parts_->len -= len;
                    Parts_->buf = (const char*)Parts_->buf + len;

                    return;
                } else {
                    len -= Parts_->len;
                    --Count_;
                    ++Parts_;
                }
            }

            if (len)
                YASSERT(0 && "shit happen");
        }

        inline const TPart* Parts() const throw () {
            return Parts_;
        }

        inline size_t Count() const throw () {
            return Count_;
        }

        static inline size_t Bytes(const TPart* parts, size_t count) throw () {
            size_t ret = 0;

            for (size_t i = 0; i < count; ++i) {
                ret += parts[i].len;
            }

            return ret;
        }

        inline size_t Bytes() const throw () {
            return Bytes(Parts_, Count_);
        }

        inline bool Complete() const throw () {
            return !Count();
        }

    private:
        TPart* Parts_;
        size_t Count_;
};
