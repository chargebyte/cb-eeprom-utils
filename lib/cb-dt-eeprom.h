/*
 * Copyright © 2026 chargebyte GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdint.h>

/* Header for Carrier Board Device Tree Overlay EEPROM */
struct cb_dt_eeprom_header {
    uint16_t signature;                  /* big-endian 'CB' = 0x4342 */
    uint8_t  reserved0;                  /* 0x00 */
    uint8_t  header_version;             /* 0x01 */
    uint16_t dt_offset;                  /* little-endian */
    uint16_t dt_size;                    /* little-endian */
    uint16_t dt_sig_offset;              /* little-endian */
    uint16_t dt_sig_size;                /* little-endian */
    uint32_t vendor_code;                /* little-endian */
    union {
        struct {
            uint32_t hw_rev;             /* special encoding, see below */
            uint8_t  order_code[32];     /* ASCII string */
            uint8_t  reserved1[72];
        } cb_dt_chargebyte;
    } vendor_specific;
    uint32_t crc32;                      /* little-endian */
} __attribute__((packed));

#define CB_DT_EERPOM_HDR_SIZE 128

_Static_assert(sizeof(struct cb_dt_eeprom_header) == CB_DT_EERPOM_HDR_SIZE, "The DT EEPROM header should be exactly 128 bytes.");

#define CB_DT_EEPROM_SIGNATURE      0x4342     /* 'CB' */
#define CB_DT_EERPOM_VC_CHARGEBYTE  0x63624342 /* 'cbCB' */
#define CB_DT_EERPOM_HDR_VERSION_1  0x1

#ifdef __cplusplus
}
#endif
