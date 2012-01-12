/* base64.c - subroutine to do base64 encoding and decoding. The code came from
 * SiFang 
 */

#include "rodsClient.h"
#include "base64.h"
#define OK 0

static const char *codes =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char mymap[256] = {
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
  7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
 19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
 37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
 49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255 };

/**
   base64 Encode a buffer (NUL terminated)
   @param in      The input buffer to encode
   @param inlen   The length of the input buffer
   @param out     [out] The destination of the base64 encoded data
   @param outlen  [in/out] The max size and resulting size
   @return OK if successful
*/
int base64_encode(const unsigned char *in,  unsigned long inlen,
                        unsigned char *out, unsigned long *outlen)
{
   unsigned long i, len2, leven;
   unsigned char *p;

#if 0
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
#endif

   /* valid output size ? */
   len2 = 4 * ((inlen + 2) / 3);
   if (*outlen < len2 + 1) {
      return BASE64_BUFFER_OVERFLOW;
   }
   p = out;
   leven = 3*(inlen / 3);
   for (i = 0; i < leven; i += 3) {
       *p++ = codes[(in[0] >> 2) & 0x3F];
       *p++ = codes[(((in[0] & 3) << 4) + (in[1] >> 4)) & 0x3F];
       *p++ = codes[(((in[1] & 0xf) << 2) + (in[2] >> 6)) & 0x3F];
       *p++ = codes[in[2] & 0x3F];
       in += 3;
   }
   /* Pad it if necessary...  */
   if (i < inlen) {
       unsigned a = in[0];
       unsigned b = (i+1 < inlen) ? in[1] : 0;

       *p++ = codes[(a >> 2) & 0x3F];
       *p++ = codes[(((a & 3) << 4) + (b >> 4)) & 0x3F];
       *p++ = (i+1 < inlen) ? codes[(((b & 0xf) << 2)) & 0x3F] : '=';
       *p++ = '=';
   }

   /* append a NULL byte */
   *p = '\0';

   /* return ok */
   *outlen = p - out;
   return OK;
}

/**
   base64 decode a block of memory
   @param in       The base64 data to decode
   @param inlen    The length of the base64 data
   @param out      [out] The destination of the binary decoded data
   @param outlen   [in/out] The max size and resulting size of the decoded data
   @return OK if successful
*/
int base64_decode(const unsigned char *in,  unsigned long inlen,
                        unsigned char *out, unsigned long *outlen)
{
   unsigned long t, x, y, z;
   unsigned char c;
   int           g;
#if 0
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
#endif

   g = 3;
   for (x = y = z = t = 0; x < inlen; x++) {
       c = mymap[in[x]&0xFF];
       if (c == 255) continue;
       /* the final = symbols are read and used to trim the remaining bytes */
       if (c == 254) {
          c = 0;
          /* prevent g < 0 which would potentially allow an overflow later */
          if (--g < 0) {
             return BASE64_INVALID_PACKET;
          }
       } else if (g != 3) {
          /* we only allow = to be at the end */
          return BASE64_INVALID_PACKET;
       }

       t = (t<<6)|c;

       if (++y == 4) {
          if (z + g > *outlen) {
             return BASE64_BUFFER_OVERFLOW;
          }
          out[z++] = (unsigned char)((t>>16)&255);
          if (g > 1) out[z++] = (unsigned char)((t>>8)&255);
          if (g > 2) out[z++] = (unsigned char)(t&255);
          y = t = 0;
       }
   }
   if (y != 0) {
       return BASE64_INVALID_PACKET;
   }
   *outlen = z;
   return OK;
}

