#include "mem.h"
#include "buffered.h"

#include <util/memory/addstorage.h>
#include <util/generic/yexception.h>

class TBufferedInput::TImpl: public TAdditionalStorage<TImpl> {
    public:
        inline TImpl(TInputStream* slave)
            : Slave_(slave)
            , MemInput_(0, 0)
        {
        }

        inline ~TImpl() throw () {
        }

        inline bool Next(const void** ptr, size_t* len) {
            if (MemInput_.Exhausted()) {
                MemInput_.Reset(Buf(), Slave_->Read(Buf(), BufLen()));
            }

            return MemInput_.Next(ptr, len);
        }

        inline size_t Read(void* buf, size_t len) {
            if (MemInput_.Exhausted()) {
                if (len > BufLen() / 2) {
                    return Slave_->Read(buf, len);
                }

                MemInput_.Reset(Buf(), Slave_->Read(Buf(), BufLen()));
            }

            return MemInput_.Read(buf, len);
        }

        inline bool ReadTo(Stroka& st, char to) {
            Stroka res;
            Stroka s_tmp;

            bool ret = false;

            while (true) {
                if (MemInput_.Exhausted()) {
                    const size_t readed = Slave_->Read(Buf(), BufLen());

                    if (!readed) {
                        break;
                    }

                    MemInput_.Reset(Buf(), readed);
                }

                const size_t a_len(MemInput_.Avail());
                MemInput_.ReadTo(s_tmp, to);
                const size_t s_len = s_tmp.length();

                /*
                 * mega-optimization
                 */
                if (res.empty()) {
                    res.swap(s_tmp);
                } else {
                    res += s_tmp;
                }

                ret = true;

                if (s_len != a_len) {
                    break;
                }
            }

            st.swap(res);

            return ret;
        }

        inline void Reset(TInputStream* slave) {
            Slave_ = slave;
        }

    private:
        inline size_t BufLen() const throw () {
            return AdditionalDataLength();
        }

        inline void* Buf() const throw () {
            return AdditionalData();
        }

    private:
        TInputStream* Slave_;
        TMemoryInput MemInput_;
};

TBufferedInput::TBufferedInput(TInputStream* slave, size_t buflen)
    : Impl_(new (buflen) TImpl(slave))
{
}

TBufferedInput::~TBufferedInput() throw () {
}

size_t TBufferedInput::DoRead(void* buf, size_t len) {
    return Impl_->Read(buf, len);
}

bool TBufferedInput::DoNext(const void** ptr, size_t* len) {
    return Impl_->Next(ptr, len);
}

bool TBufferedInput::DoReadTo(Stroka& st, char ch) {
    return Impl_->ReadTo(st, ch);
}

void TBufferedInput::Reset(TInputStream* slave) {
    Impl_->Reset(slave);
}

class TBufferedOutput::TImpl: public TAdditionalStorage<TImpl> {
    public:
        inline TImpl(TOutputStream* slave)
            : Slave_(slave)
            , MemOut_(Buf(), Len())
            , PropagateFlush_(false)
            , PropagateFinish_(false)
        {
        }

        inline ~TImpl() throw () {
        }

        inline void Write(const void* buf, size_t len) {
            if (len <= MemOut_.Avail()) {
                /*
                 * fast path
                 */

                MemOut_.Write(buf, len);
            } else {
#if 1
                const size_t stored = Stored();
                const size_t full_len = stored + len;
                const size_t good_len = DownToBufferGranularity(full_len);
                const size_t write_from_buf = good_len - stored;

                typedef TOutputStream::TPart TPart;
                const TPart parts[] = {
                    TPart(Buf(), stored),
                    TPart(buf, write_from_buf)
                };

                Slave_->Write(parts, sizeof(parts) / sizeof(*parts));
                Reset();

                if (write_from_buf < len) {
                    MemOut_.Write((const char*)buf + write_from_buf, len - write_from_buf);
                }
#else
                typedef TOutputStream::TPart TPart;
                const TPart parts[] = {
                    TPart(Buf(), Stored()),
                    TPart(buf, len)
                };

                Slave_->Write(parts, sizeof(parts) / sizeof(*parts));
                Reset();
#endif
            }
        }

        inline void SetFlushPropagateMode(bool mode) throw () {
            PropagateFlush_ = mode;
        }

        inline void SetFinishPropagateMode(bool mode) throw () {
            PropagateFinish_ = mode;
        }

        inline void Flush() {
            {
                Slave_->Write(Buf(), Stored());
                Reset();
            }

            if (PropagateFlush_) {
                Slave_->Flush();
            }
        }

        inline void Finish() {
            try {
                Flush();
            } catch (...) {
                try {
                    DoFinish();
                } catch (...) {
                }

                throw;
            }

            DoFinish();
        }

    private:
        inline void DoFinish() {
            if (PropagateFinish_) {
                Slave_->Finish();
            }
        }

        inline size_t Stored() const throw () {
            return Len() - MemOut_.Avail();
        }

        inline void Reset() throw () {
            MemOut_.Reset(Buf(), Len());
        }

        inline size_t DownToBufferGranularity(size_t l) const throw () {
            return l - (l % Len());
        }

        inline void* Buf() const throw () {
            return AdditionalData();
        }

        inline size_t Len() const throw () {
            return AdditionalDataLength();
        }

    private:
        TOutputStream* Slave_;
        TMemoryOutput MemOut_;
        bool PropagateFlush_;
        bool PropagateFinish_;
};

TBufferedOutput::TBufferedOutput(TOutputStream* slave, size_t buflen)
    : Impl_(new (buflen) TImpl(slave))
{
}

TBufferedOutput::~TBufferedOutput() throw () {
    try {
        Finish();
    } catch (...) {
    }
}

void TBufferedOutput::DoWrite(const void* data, size_t len) {
    if (Impl_.Get()) {
        Impl_->Write(data, len);
    } else {
        ythrow yexception() << "can not write to finished stream";
    }
}

void TBufferedOutput::DoFlush() {
    if (Impl_.Get()) {
        Impl_->Flush();
    }
}

void TBufferedOutput::DoFinish() {
    THolder<TImpl> impl(Impl_.Release());

    if (impl) {
        impl->Finish();
    }
}

void TBufferedOutput::SetFlushPropagateMode(bool propagate) throw () {
    if (Impl_.Get()) {
        Impl_->SetFlushPropagateMode(propagate);
    }
}

void TBufferedOutput::SetFinishPropagateMode(bool propagate) throw () {
    if (Impl_.Get()) {
        Impl_->SetFinishPropagateMode(propagate);
    }
}
