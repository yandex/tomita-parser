#pragma once

#include "autoarray.h"

#include <util/system/compat.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>

#include <cstdio>

const int bitmap_pagesize = 128*1024;
struct bitmap_1 {
    yvector<ui32*> vect;
    typedef yvector<ui32*>::size_type size_type;
    enum my_enum { elems_per_page = bitmap_pagesize * 32 };
    typedef struct bitmap_1_iterator iterator;
    bool set(ui32 id);   /// return true if value changed, e.g. !test()
    bool reset(ui32 id); /// return true if value changed, e.g. test()
    bool test(ui32 id) const {
        ui32 mask;
        mask = 1 << (id & 31);
        ui32 elh = id / (bitmap_pagesize * 32);
        id = id / 32 & (bitmap_pagesize - 1);
        if (elh >= vect.size() || !vect[elh]) return false;
        ui32 *v = vect[elh];
        if (v[id] & mask) return true;
        return false;
    }
    size_type numpages() const { return vect.size(); }
    ui32 count(ui32 page) const;
    ui32 count() const;
    size_type size() { return vect.size() * elems_per_page; }
    static ui32 getpage(ui32 id) { return id / elems_per_page; }
    ~bitmap_1() { clear(); }
    void clear();
    void swap(bitmap_1 &with) { vect.swap(with.vect); }
    void save(FILE *f) const throw();
    void load(FILE *f, const char *dbg_fname = "Unknown_file_name");
    void save(const char *fname) const;
    void load(const char *fname);
    void reserve(size_t sz) { vect.reserve(sz / 8); }
};

struct bitmap_1_iterator  : private TNonCopyable {
    size_t doc;

    bitmap_1_iterator(const bitmap_1 &v_);
    bool eof() const { return cur_page == num_pages; }
    bool operator++() {
        cur_mask = cur_mask >> 1;
        doc++;
        if (!cur_mask) {
            doc = (doc + 31) & ~31;
            pptr++;
            while (pptr < eptr && !*pptr)
                pptr++, doc += 32;
            if (EXPECT_FALSE(pptr == eptr)) {
                if (!next_page())
                    return false;
            } else {
                cur_mask = *pptr;
            }
        }
        while (true) {
            if (cur_mask & 1)
                return true;
            cur_mask = cur_mask >> 1;
            doc++;
        }
    }

  protected:
    size_t cur_page_end;
    size_t cur_page, num_pages;
    ui32 *cur_page_ptr;
    ui32 *pptr, *eptr;
    ui32 cur_mask;
    const bitmap_1 &v;
    bool next_page();
};

struct bitmap_f1 {
    autoarray<ui32> cont;

    bitmap_f1(size_t size) : cont((size + 31) / 32, 0) {}
    void swap(bitmap_f1 &with) { cont.swap(with.cont); }

    bool test(size_t id) const {
        if (ui32 v = cont[id >> 5]) {
            ui32 mask = 1 << (id & 31);
            if (v & mask)
                return true;
        }
        return false;
    }

    bool set(size_t id) { // return true if value was changed, i.e. !test()
        ui32 mask = 1 << (id & 31);
        ui32 &v = cont[id >> 5];
        if (v & mask) return false;
        v |= mask;
        return true;
    }

    bool reset(size_t id) { // return true if value was changed, i.e. test()
        ui32 mask = 1 << (id & 31);
        ui32 &v = cont[id >> 5];
        if (!(v & mask)) return false;
        v &= ~mask;
        return true;
    }

    // If we want to completely clean-up bitmap that contains only a few items
    // as fast as possible, we can neglect neighbouring bits that will be cleared anyway
    void reset_wide(size_t id) {
        cont[id >> 5] = 0;
    }

    template<typename T>
    void reset_wide(const T& v) {
        for (typename T::const_iterator i = v.begin(); i != v.end(); ++i)
            cont[*i >> 5] = 0;
    }
};

class bitmap_2 {
  public:
    enum SetType {
        SetReplace,
        SetBitwiseOr,
        SetBitwiseAnd
    };

    yvector<ui8*> vect;
    ui32 set(ui32 id, ui8 val, SetType type = SetReplace) {
        YASSERT((id & 0xffffff00u) != 0xffffff00u);
        ui32 vmask, mask, ushift = (id & 3) * 2;
        mask = 3 << ushift;
        vmask = (val & 3) << ushift;
        unsigned elh = id / (bitmap_pagesize * 4);
        id = id / 4 & (bitmap_pagesize - 1);
        if (elh >= vect.size()) { //resize
            vect.resize(elh + 16, (ui8*)0);
        }
        if (!vect[elh]) {
            vect[elh] = new ui8[bitmap_pagesize];
            memset(vect[elh], 0, bitmap_pagesize);
        }
        ui8 *v = vect[elh];
        if (type == SetBitwiseOr)
            vmask |= (v[id] & mask);
        else if (type == SetBitwiseAnd)
            vmask &= (v[id] & mask);
        ui32 ret = (v[id] >> ushift) & 3;
        v[id] = ui8((v[id] & ~mask) | vmask);
        return ret;
    }
    ui32 test(unsigned id) const {
        YASSERT((id & 0xffffff00u) != 0xffffff00u);
        ui32 ushift = (id & 3) * 2;
        unsigned elh = id / (bitmap_pagesize * 4);
        id = id / 4 & (bitmap_pagesize - 1);
        if (elh >= vect.size()) return 0;
        if (!vect[elh]) return 0;
        const ui8 *v = vect[elh];
        return (v[id] >> ushift) & 3;
    }
    ~bitmap_2() {
        for (unsigned n = 0; n < vect.size(); n++) delete[] vect[n];
    }
    void LoadDeletedDocuments(FILE *fportion, const ui8 val);
};
