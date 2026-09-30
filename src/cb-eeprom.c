/*
 * Copyright © 2026 chargebyte GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <cb-eeprom.h>
#include <version.h>

/*
 * Note: We use the I2C paths as stable anchors over all products - nvmem path/files differ.
 */

/* This is the EEPROM on the phyCORE (tm) SOM. */
#define SOM_EEPROM_PATH "/sys/class/i2c-dev/i2c-2/device/2-0050/eeprom"

/* Carrier Board DT EEPROM */
#define DT_EEPROM_PATH "/sys/class/i2c-dev/i2c-0/device/0-0050/eeprom"

/* RTC EEPROM */
#define RV3028_EEPROM_PATH "/sys/class/i2c-dev/i2c-0/device/0-0052/rv3028_eeprom0/nvmem"

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [-n] [variable]\n"
            "\n"
            "Options:\n"
            "  -n, --no-header   do not print variable name\n"
            "      --som-eeprom <path>  SOM EEPROM path\n"
            "      --cb-eeprom <path>   carrier-board EEPROM path\n"
            "      --dt-eeprom <path>   DT EEPROM path\n"
            "  -v, --version     show version\n"
            "  -h, --help        show this help\n",
            prog);
}

int main(int argc, char **argv)
{
    char compatible[4096];
    size_t compatible_length = 0;
    const char *selected = NULL;
    int value_only = 0;
    int som_present, cb_present;
    int matched = 0;
    int opt;
    const char *som_eeprom_path = SOM_EEPROM_PATH;
    const char *cb_eeprom_path = RV3028_EEPROM_PATH;
    const char *dt_eeprom_path = DT_EEPROM_PATH;
    static const struct option options[] = {
        { "no-header", no_argument, NULL, 'n' },
        { "som-eeprom", required_argument, NULL, 1000 },
        { "cb-eeprom", required_argument, NULL, 1001 },
        { "dt-eeprom", required_argument, NULL, 1002 },
        { "version",    no_argument,       NULL, 'v' },
        { "help",      no_argument, NULL, 'h' },
        { NULL,        0,           NULL,  0  }
    };

    while ((opt = getopt_long(argc, argv, "nvh", options, NULL)) != -1) {
        switch (opt) {
        case 'n': value_only = 1; break;
        case 'v': puts(PACKAGE_STRING); return 0;
        case 'h': usage(argv[0]); return 0;
        case 1000: som_eeprom_path = optarg; break;
        case 1001: cb_eeprom_path = optarg; break;
        case 1002: dt_eeprom_path = optarg; break;
        default: usage(argv[0]); return 2;
        }
    }
    if (argc - optind > 1) {
        usage(argv[0]);
        return 2;
    }
    if (argc != optind)
        selected = argv[optind];

    som_present = access(som_eeprom_path, F_OK) == 0;
    cb_present = access(cb_eeprom_path, F_OK) == 0;
    if (!som_present && !cb_present) {
        fprintf(stderr, "Error: no supported EEPROM device found\n");
        return 1;
    }
    if (som_present) {
        struct cb_eeprom eeprom;
        if (cb_eeprom_read_at(som_eeprom_path, &eeprom, sizeof(eeprom), 256) != 0 ||
            cb_eeprom_dump_som(&eeprom, selected, value_only, &matched) != 0)
            return 1;
    }
    if (cb_present) {
        uint8_t data[sizeof(struct cb_doublemint_eeprom)];
        const struct cb_eeprom_mapping *mapping;
        struct hwrev dt_hw_revision;
        const struct hwrev *dt_hw_revision_ptr = NULL;

        if (cb_eeprom_read_compatible(compatible, sizeof(compatible),
                                       &compatible_length) != 0)
            return 1;
        mapping = cb_eeprom_find_mapping(compatible, compatible_length);
        if (!mapping) {
            fprintf(stderr, "Error: unsupported platform in /proc/device-tree/compatible\n");
            return 1;
        }
        if (cb_eeprom_read_at(cb_eeprom_path, data, mapping->eeprom_size, 0) != 0)
            return 1;

        if (data[mapping->hw_revision_offset] == 0xaa &&
            data[mapping->hw_revision_offset + 1] == 0x55 &&
            data[mapping->hw_revision_offset + 2] == 0xaa &&
            data[mapping->hw_revision_offset + 3] == 0x55 &&
            cb_eeprom_read_dt_hw_revision(dt_eeprom_path, &dt_hw_revision) > 0)
            dt_hw_revision_ptr = &dt_hw_revision;

        if (mapping->dump(data, selected, value_only, &matched,
                          mapping->unset_hw_revision, dt_hw_revision_ptr) != 0)
            return 1;
    }
    if (selected && matched == 0) {
        fprintf(stderr, "Error: unknown or unavailable variable: %s\n", selected);
        return 1;
    }
    return 0;
}
