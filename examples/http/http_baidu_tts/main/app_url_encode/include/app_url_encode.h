#pragma once

#include <stddef.h>

/**
 * @brief https://www.toolhelper.cn/EncodeDecode/Url
 *
 * @param src
 * @param slen
 * @param olen
 * @param dst
 * @param dlen
 * @return int
 */
int url_encode(const unsigned char *src, size_t slen, size_t *olen, unsigned char *dst, size_t dlen);
