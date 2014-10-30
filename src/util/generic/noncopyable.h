#pragma once

#include <util/system/defaults.h>

/*!
 *  inherit your class from TNonCopyable if you want to protect copying
 *
 *  it is better than DECLARE_NOCOPY macro because of two reasons:
 *     1. you don't have to type your class name twice
 *     2. you are able to use your class without implicitly defined ctors.
 *
 *   example:
 *   <pre>
 *   class Foo : private TNonCopyable {
 *       ...
 *   };
 *   </pre>
 *
 */

namespace NNonCopyable { // protection from unintended ADL
    class TNonCopyable {
    private:  // emphasize the following members are private
        TNonCopyable(const TNonCopyable&);
        const TNonCopyable& operator=(const TNonCopyable&);
    protected:
        TNonCopyable() {
        }

        ~TNonCopyable() {
        }
    };
}

typedef NNonCopyable::TNonCopyable TNonCopyable;
