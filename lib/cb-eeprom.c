/*
 * Copyright © 2026 chargebyte GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "cb-eeprom.h"
#include "cb-dt-eeprom.h"
#include "crc32.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>
#include <stddef.h>

#define COMPATIBLE_PATH "/proc/device-tree/compatible"

static int bcd_byte(uint8_t value, unsigned int *result)
{
    if ((value >> 4) > 9 || (value & 0x0f) > 9)
        return -1;

    *result = ((unsigned int)(value >> 4) * 10) + (value & 0x0f);

    return 0;
 }

static int print_assignment(const char *name, const char *value,
                            const char *selected, int value_only)
{
    if (selected && strcmp(selected, name) != 0)
        return 0;

    if (value_only)
        printf("%s\n", value);
    else
        printf("%s=%s\n", name, value);

    return 1;
}

static int format_mac(char *out, size_t out_size, const uint8_t mac[CB_EEPROM_MAC_ADDR_LEN])
{
    int written = snprintf(out, out_size, "%02x:%02x:%02x:%02x:%02x:%02x",
                           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return written < 0 || (size_t)written >= out_size ? -1 : 0;
}

static int format_dmc(char *out, size_t out_size, const uint8_t dmc[CB_EEPROM_PCB_DMC_LEN])
{
    size_t digits = 0;
    size_t length = CB_EEPROM_PCB_DMC_LEN;

    if (dmc[0] == 'P' && dmc[1] == 'T') {
        length = 0;
        while (length < CB_EEPROM_PCB_DMC_LEN && dmc[length] != '\0')
            length++;
        if (length + 1 > out_size)
            return -1;
        for (size_t i = 0; i < length; i++) {
            if (!isprint((unsigned char)dmc[i]))
                return -1;
            out[i] = (char)dmc[i];
        }
        out[length] = '\0';
        return 0;
    }

    if (dmc[8] == 0 && dmc[9] == 0 && dmc[10] == 0 && dmc[11] == 0)
        length = 8;

    if (length * 2 + 1 > out_size)
        return -1;

    for (size_t i = 0; i < length; i++) {
        if ((dmc[i] >> 4) > 9 || (dmc[i] & 0x0f) > 9)
            return -1;

        out[digits++] = (char)('0' + (dmc[i] >> 4));
        out[digits++] = (char)('0' + (dmc[i] & 0x0f));
    }
    out[digits] = '\0';

    return 0;
}

static int format_serial(char *out, size_t out_size, const uint8_t serial[CB_EEPROM_SERIAL_LEN])
{
    char full[CB_EEPROM_SERIAL_LEN * 2 + 1];
    size_t first = 0;
    size_t digits = 0;

    for (size_t i = 0; i < CB_EEPROM_SERIAL_LEN; i++) {
        if ((serial[i] >> 4) > 9 || (serial[i] & 0x0f) > 9)
            return -1;
        full[digits++] = (char)('0' + (serial[i] >> 4));
        full[digits++] = (char)('0' + (serial[i] & 0x0f));
    }

    full[digits] = '\0';

    while (full[first] == '0' && full[first + 1] != '\0')
        first++;

    if (strlen(full + first) + 1 > out_size)
        return -1;

    strcpy(out, full + first);

    return 0;
}

static int emit_dmc(const char *name, const uint8_t dmc[CB_EEPROM_PCB_DMC_LEN],
                    const char *selected, int value_only, int *matched)
{
    char value[CB_EEPROM_PCB_DMC_LEN * 2 + 1];

    if (format_dmc(value, sizeof(value), dmc) != 0) {
        fprintf(stderr, "Error: invalid PCB DMC data\n");
        return -1;
    }

    *matched += print_assignment(name, value, selected, value_only);

    return 0;
}

static int emit_serial(const char *name, const uint8_t serial[CB_EEPROM_SERIAL_LEN],
                       const char *selected, int value_only, int *matched)
{
    char value[CB_EEPROM_SERIAL_LEN * 2 + 1];

    if (format_serial(value, sizeof(value), serial) != 0) {
        fprintf(stderr, "Error: invalid serial number data\n");
        return -1;
    }

    *matched += print_assignment(name, value, selected, value_only);

    return 0;
}

static int emit_mac(const char *name, const uint8_t mac[CB_EEPROM_MAC_ADDR_LEN],
                    const char *selected, int value_only, int *matched)
{
    char value[18];

    if (format_mac(value, sizeof(value), mac) != 0)
        return -1;

    *matched += print_assignment(name, value, selected, value_only);

    return 0;
}

static int emit_hw_revision(const char *name, const struct hwrev *revision,
                            const char *selected, int value_only, int *matched,
                            const char *unset_hw_revision)
{
    const char *fallback = unset_hw_revision ? unset_hw_revision : "unknown";
    unsigned int major, minor;
    char value[16];
    int written;

    if (revision->major_bcd == 0xaa && revision->minor_bcd == 0x55 &&
        revision->bom_revision == 0xaa && revision->reserved == 0x55) {
        *matched += print_assignment(name, fallback, selected, value_only);
        return 0;
    }

    if (bcd_byte(revision->major_bcd, &major) != 0 ||
        bcd_byte(revision->minor_bcd, &minor) != 0 ||
        revision->reserved != 0 || !isprint((unsigned char)revision->bom_revision)) {
        fprintf(stderr, "Error: invalid hardware revision data\n");
        return -1;
    }

    written = snprintf(value, sizeof(value), "V%uR%u%c", major, minor,
                       revision->bom_revision);
    if (written < 0 || (size_t)written >= sizeof(value))
        return -1;

    *matched += print_assignment(name, value, selected, value_only);
    return 0;
}

static int valid_hw_revision(const struct hwrev *revision)
{
    unsigned int major, minor;

    return bcd_byte(revision->major_bcd, &major) == 0 &&
           bcd_byte(revision->minor_bcd, &minor) == 0 &&
           revision->reserved == 0 &&
           isprint((unsigned char)revision->bom_revision);
}

static int emit_som_hw_revision(const struct hwrev *revision,
                                const char *selected, int value_only,
                                int *matched, const char *unset_hw_revision)
{
    if (revision->major_bcd == 0xff && revision->minor_bcd == 0xff &&
        revision->bom_revision == 0xff && revision->reserved == 0xff) {
        if (unset_hw_revision)
            return emit_hw_revision("som_hw_rev", revision, selected, value_only,
                                    matched, unset_hw_revision);

        fprintf(stderr, "Warning: SOM hardware revision unavailable for unsupported platform\n");
        return 0;
    }

    if (!valid_hw_revision(revision)) {
        fprintf(stderr, "Warning: invalid SOM hardware revision data\n");
        return 0;
    }

    return emit_hw_revision("som_hw_rev", revision, selected, value_only,
                            matched, unset_hw_revision);
}

int cb_eeprom_read_at(const char *path, void *buffer, size_t length, off_t offset)
{
    size_t done = 0;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "Error: could not open %s: %s\n", path, strerror(errno));
        return -1;
    }

    while (done < length) {
        ssize_t count = pread(fd, (uint8_t *)buffer + done, length - done,
                              offset + (off_t)done);
        if (count < 0) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "Error: could not read %s: %s\n", path, strerror(errno));
            close(fd);
            return -1;
        }
        if (count == 0) {
            fprintf(stderr, "Error: %s is shorter than expected\n", path);
            close(fd);
            return -1;
        }
        done += (size_t)count;
    }

    close(fd);

    return 0;
}

int cb_eeprom_read_dt_hw_revision(const char *path, struct hwrev *revision)
{
    struct cb_dt_eeprom_header header;
    uint32_t expected_crc;
    uint32_t actual_crc;

    if (access(path, F_OK) != 0)
        return 0;

    if (cb_eeprom_read_at(path, &header, sizeof(header), 0) != 0)
        return 0;

    expected_crc = le32toh(header.crc32);
    actual_crc = crc32_uboot((const uint8_t *)&header,
                             offsetof(struct cb_dt_eeprom_header, crc32));
    if (expected_crc != actual_crc) {
        fprintf(stderr, "Warning: invalid DT EEPROM CRC in %s\n", path);
        return 0;
    }

    memcpy(revision, &header.vendor_specific.cb_dt_chargebyte.hw_rev,
           sizeof(*revision));
    return 1;
}

int cb_eeprom_read_compatible(char *buffer, size_t capacity, size_t *length)
{
    ssize_t count;
    int fd = open(COMPATIBLE_PATH, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "Error: could not open %s: %s\n", COMPATIBLE_PATH,
                strerror(errno));
        return -1;
    }

    count = read(fd, buffer, capacity);
    if (count < 0) {
        fprintf(stderr, "Error: could not read %s: %s\n", COMPATIBLE_PATH,
                strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    *length = (size_t)count;

    return 0;
}

int cb_eeprom_compatible_contains(const char *buffer, size_t length, const char *value)
{
    size_t value_len = strlen(value);
    size_t pos = 0;

    while (pos < length) {
        size_t end = pos;
        while (end < length && buffer[end] != '\0')
            end++;
        if (end - pos == value_len && memcmp(buffer + pos, value, value_len) == 0)
            return 1;
        if (end == length)
            break;
        pos = end + 1;
    }

    return 0;
}

int cb_eeprom_dump_som(const struct cb_eeprom *eeprom, const char *selected,
                       int value_only, int *matched,
                       const char *unset_hw_revision)
{
    int result = 0;

    result |= emit_mac("mac_cp_host", eeprom->mac_cp_host, selected, value_only, matched);
    result |= emit_mac("mac_cp_firmware", eeprom->mac_cp_firmware, selected, value_only, matched);
    result |= emit_dmc("som_pcb_dmc", eeprom->pcb_dmc, selected, value_only, matched);
    result |= emit_serial("som_serial", eeprom->cb_serial, selected, value_only, matched);
    result |= emit_som_hw_revision(&eeprom->hw_rev, selected, value_only, matched,
                                   unset_hw_revision);

    return result;
}

static int dump_mint(const uint8_t *data, const char *selected, int value_only,
                     int *matched, const char *unset_hw_revision,
                     const struct hwrev *dt_hw_revision)
{
    const struct cb_mint_eeprom *mint = (const struct cb_mint_eeprom *)data;
    int result = 0;

    result |= emit_dmc("cb_pcb_dmc", mint->pcb_dmc, selected, value_only, matched);
    result |= emit_serial("cb_serial", mint->cb_serial, selected, value_only, matched);

    if (dt_hw_revision && mint->hw_rev.major_bcd == 0xaa &&
        mint->hw_rev.minor_bcd == 0x55 && mint->hw_rev.bom_revision == 0xaa &&
        mint->hw_rev.reserved == 0x55) {
        result |= emit_hw_revision("cb_hw_rev", dt_hw_revision, selected, value_only,
                                   matched, unset_hw_revision);
    } else if (emit_hw_revision("cb_hw_rev", &mint->hw_rev, selected,
                                value_only, matched, unset_hw_revision)) {
        result = -1;
    }

    return result;
}

static int dump_doublemint(const uint8_t *data, const char *selected, int value_only,
                           int *matched, const char *unset_hw_revision,
                           const struct hwrev *dt_hw_revision)
{
    const struct cb_doublemint_eeprom *dm =
        (const struct cb_doublemint_eeprom *)data;
    int result = 0;

    result |= emit_dmc("cb_pcb_dmc", dm->pcb_dmc, selected, value_only, matched);
    result |= emit_serial("cb_serial", dm->cb_serial, selected, value_only, matched);
    result |= emit_mac("mac_cp2_host", dm->mac_cp2_host, selected, value_only, matched);
    result |= emit_mac("mac_cp2_fw", dm->mac_cp2_fw, selected, value_only, matched);
    result |= emit_mac("mac_lan2", dm->mac_lan2, selected, value_only, matched);

    if (dt_hw_revision && dm->hw_rev.major_bcd == 0xaa &&
        dm->hw_rev.minor_bcd == 0x55 && dm->hw_rev.bom_revision == 0xaa &&
        dm->hw_rev.reserved == 0x55) {
        result |= emit_hw_revision("cb_hw_rev", dt_hw_revision, selected, value_only,
                                   matched, unset_hw_revision);
    } else if (emit_hw_revision("cb_hw_rev", &dm->hw_rev, selected, value_only,
                                matched, unset_hw_revision)) {
        result = -1;
    }

    return result;
}

static const struct cb_eeprom_mapping mappings[] = {
    {
        "chargebyte,imx93-charge-som-dc-evb",
        sizeof(struct cb_mint_eeprom),
        offsetof(struct cb_mint_eeprom, hw_rev),
        dump_mint,
        "V0R2a",
    },
    {
        "chargebyte,imx93-ac-power-board",
        sizeof(struct cb_mint_eeprom),
        offsetof(struct cb_mint_eeprom, hw_rev),
        dump_mint,
        "V0R1a",
    },
    {
        "chargebyte,imx93-charge-control-v",
        sizeof(struct cb_doublemint_eeprom),
        offsetof(struct cb_doublemint_eeprom, hw_rev),
        dump_doublemint,
        "V0R1a",
    },
};

static const struct cb_som_eeprom_mapping som_mappings[] = {
    {
        "chargebyte,imx93-charge-som",
        "V0R2a",
    },
    {
        "chargebyte,imx93-charge-control-y",
        "V0R5a",
    },
    {
        "chargebyte,imx93-lime",
        "V0R1a",
    },
    {
        "chargebyte,imx93-protolime",
        "V0R1a",
    },
};

const struct cb_eeprom_mapping *cb_eeprom_find_mapping(const char *buffer, size_t length)
{
    for (size_t i = 0; i < sizeof(mappings) / sizeof(mappings[0]); i++) {
        if (cb_eeprom_compatible_contains(buffer, length, mappings[i].compatible))
            return &mappings[i];
    }

    return NULL;
}

const struct cb_som_eeprom_mapping *cb_eeprom_find_som_mapping(const char *buffer,
                                                               size_t length)
{
    for (size_t i = 0; i < sizeof(som_mappings) / sizeof(som_mappings[0]); i++) {
        if (cb_eeprom_compatible_contains(buffer, length, som_mappings[i].compatible))
            return &som_mappings[i];
    }

    return NULL;
}
