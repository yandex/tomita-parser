/* The following code is the modified part of the libxml
 * available at http://xmlsoft.org
 * under the terms of the MIT License
 * http://opensource.org/licenses/mit-license.html
 */


#include "yencoding.h"

#include <util/charset/codepage.h>
#include <util/charset/recyr.hh>


int
win1251ToUTF8(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen) {
    size_t in_readed, out_writed;
    RECODE_RESULT res = Recode(CODES_WIN, CODES_UTF8, (const char*)in, (char*)out, (size_t)*inlen, (size_t)*outlen, in_readed, out_writed);
    *inlen = (int)in_readed;
    *outlen = (int)out_writed;
    return (res == RECODE_OK)? 0 : -1;
}

int
UTF8Towin1251(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen) {
    size_t in_readed, out_writed;
    RECODE_RESULT res = Recode(CODES_UTF8, CODES_WIN, (const char*)in, (char*)out, (size_t)*inlen, (size_t)*outlen, in_readed, out_writed);
    *inlen = (int)in_readed;
    *outlen = (int)out_writed;
    return (res == RECODE_OK)? 0 : -1;
}


int
koi8ToUTF8(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen) {
    size_t in_readed, out_writed;
    RECODE_RESULT res = Recode(CODES_KOI8, CODES_UTF8, (const char*)in, (char*)out, (size_t)*inlen, (size_t)*outlen, in_readed, out_writed);
    *inlen = (int)in_readed;
    *outlen = (int)out_writed;
    return (res == RECODE_OK)? 0 : -1;
}

int
UTF8Tokoi8(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen) {
    size_t in_readed, out_writed;
    RECODE_RESULT res = Recode(CODES_UTF8, CODES_KOI8, (const char*)in, (char*)out, (size_t)*inlen, (size_t)*outlen, in_readed, out_writed);
    *inlen = (int)in_readed;
    *outlen = (int)out_writed;
    return (res == RECODE_OK)? 0 : -1;
}

