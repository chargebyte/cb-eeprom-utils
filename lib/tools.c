/*
 * Copyright © 2026 chargebyte GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include "tools.h"

/* HW revision parser:
 *
 * V15R24z -> { 0x15, 0x24, 'z', 0x00 }
 */
int parse_hw_rev(const char *s, uint32_t *out)
{
    const char *p = s;
    uint8_t bytes[4];
    uint8_t letter;
    int v = 0;
    int r = 0;
    uint32_t val;

    if (*p == 'V' || *p == 'v')
        p++;

    while (isdigit((unsigned char)*p)) {
        v = v * 10 + (*p - '0');
        p++;
    }

    if (*p != 'R' && *p != 'r')
        return -1;
    p++;

    while (isdigit((unsigned char)*p)) {
        r = r * 10 + (*p - '0');
        p++;
    }

    if (!isalpha((unsigned char)*p))
        return -1;

    letter = (uint8_t)*p++;

    if (*p != '\0')
        return -1;

    if (v > 99 || r > 99)
        return -1;

    bytes[0] = ((v / 10) << 4) | (v % 10);
    bytes[1] = ((r / 10) << 4) | (r % 10);
    bytes[2] = letter;
    bytes[3] = 0x00;

    memcpy(&val, bytes, sizeof(val));
    *out = val;

    return 0;
}
