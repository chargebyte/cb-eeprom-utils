/*
 * Copyright © 2026 chargebyte GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define CB_EEPROM_MAC_ADDR_LEN 6
#define CB_EEPROM_PCB_DMC_LEN 12
#define CB_EEPROM_SERIAL_LEN 6

struct hwrev {
    uint8_t major_bcd;
    uint8_t minor_bcd;
    uint8_t bom_revision;
    uint8_t reserved;
} __attribute__((packed));

struct cb_eeprom {
    uint8_t mac_cp_host[CB_EEPROM_MAC_ADDR_LEN];
    uint16_t reserved0;
    uint8_t mac_cp_firmware[CB_EEPROM_MAC_ADDR_LEN];
    uint16_t reserved1;
    uint8_t pcb_dmc[CB_EEPROM_PCB_DMC_LEN];
    uint8_t cb_serial[CB_EEPROM_SERIAL_LEN];
    struct hwrev hw_rev;
} __attribute__((packed));

struct cb_mint_eeprom {
    uint8_t pcb_dmc[CB_EEPROM_PCB_DMC_LEN];
    uint8_t cb_serial[CB_EEPROM_SERIAL_LEN];
    struct hwrev hw_rev;
} __attribute__((packed));

struct cb_doublemint_eeprom {
    uint8_t pcb_dmc[CB_EEPROM_PCB_DMC_LEN];
    uint8_t cb_serial[CB_EEPROM_SERIAL_LEN];
    uint8_t mac_cp2_host[CB_EEPROM_MAC_ADDR_LEN];
    uint8_t mac_cp2_fw[CB_EEPROM_MAC_ADDR_LEN];
    uint8_t mac_lan2[CB_EEPROM_MAC_ADDR_LEN];
    struct hwrev hw_rev;
} __attribute__((packed));

typedef int (*cb_eeprom_dump_callback)(const uint8_t *data,
                                       const char *selected, int value_only,
                                       int *matched,
                                       const char *unset_hw_revision,
                                       const struct hwrev *dt_hw_revision);

struct cb_eeprom_mapping {
    const char *compatible;
    size_t eeprom_size;
    size_t hw_revision_offset;
    cb_eeprom_dump_callback dump;
    const char *unset_hw_revision;
};

_Static_assert(sizeof(struct hwrev) == 4, "unexpected hwrev size");
_Static_assert(sizeof(struct cb_eeprom) == 38, "unexpected SOM EEPROM size");
_Static_assert(sizeof(struct cb_mint_eeprom) == 22, "unexpected Mint EEPROM size");
_Static_assert(sizeof(struct cb_doublemint_eeprom) == 40, "unexpected Doublemint EEPROM size");

int cb_eeprom_read_at(const char *path, void *buffer, size_t length, off_t offset);
int cb_eeprom_read_dt_hw_revision(const char *path, struct hwrev *revision);
int cb_eeprom_read_compatible(char *buffer, size_t capacity, size_t *length);
int cb_eeprom_compatible_contains(const char *buffer, size_t length,
                                  const char *value);
const struct cb_eeprom_mapping *cb_eeprom_find_mapping(const char *buffer,
                                                       size_t length);
int cb_eeprom_dump_som(const struct cb_eeprom *eeprom, const char *selected,
                       int value_only, int *matched);
