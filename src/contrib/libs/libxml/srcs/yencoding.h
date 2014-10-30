/* The following code is the modified part of the libxml
 * available at http://xmlsoft.org
 * under the terms of the MIT License
 * http://opensource.org/licenses/mit-license.html
 */

#ifndef __XML_YENCODING_H__
#define __XML_YENCODING_H__

#ifdef __cplusplus
extern "C" {
#endif

int win1251ToUTF8(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen);

int UTF8Towin1251(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen);

int koi8ToUTF8(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen) ;

int UTF8Tokoi8(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen);

#ifdef __cplusplus
}
#endif

#endif
