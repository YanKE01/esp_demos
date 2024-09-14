#include "app_url_encode.h"

int url_encode(const unsigned char *src, size_t slen, size_t *olen, unsigned char *dst, size_t dlen)
{
    size_t i, j = 0;

    // clac encoed size
    for (i = 0; i < slen; i++) {
        if ((src[i] >= 'A' && src[i] <= 'Z') ||
                (src[i] >= 'a' && src[i] <= 'z') ||
                (src[i] >= '0' && src[i] <= '9') ||
                src[i] == '-' || src[i] == '_' || src[i] == '.' || src[i] == '~') {
            j++;
        } else if (src[i] == ' ') {
            j++;
        } else {
            j += 3; // length of %xx is three
        }
    }

    // check buffer size
    if (dlen < j + 1) {
        *olen = j + 1; // reytun need size
        return -1;
    }

    // url encode
    for (i = 0, j = 0; i < slen; i++) {
        if ((src[i] >= 'A' && src[i] <= 'Z') ||
                (src[i] >= 'a' && src[i] <= 'z') ||
                (src[i] >= '0' && src[i] <= '9') ||
                src[i] == '-' || src[i] == '_' || src[i] == '.' || src[i] == '~') {
            dst[j++] = src[i];
        } else if (src[i] == ' ') {
            dst[j++] = '+';
        } else {
            dst[j++] = '%';
            dst[j++] = "0123456789ABCDEF"[src[i] >> 4];
            dst[j++] = "0123456789ABCDEF"[src[i] & 0x0F];
        }
    }

    // add end
    dst[j] = '\0';

    // return size
    *olen = j;

    return 0;
}
