/*
 * Copyright © 2026 chargebyte GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <getopt.h>
#include <endian.h>
#include <stddef.h>
#include <cb-eeprom-utils.h>

static int mmap_file(void **p, size_t *file_size, const char *descr, const char *fn)
{
    struct stat st;
    int fd = -1;
    int rv = -1;

    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: could not open %s (%s): %m\n", descr, fn);
        goto err_out;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Error: could not fstat %s (%s): %m\n", descr, fn);
        goto err_out;
    }

    if (st.st_size == 0) {
        fprintf(stderr, "Error: %s (%s) must not have file size of zero.\n", descr, fn);
        goto err_out;
    }

    if (st.st_size > 0xFFFF) {
        fprintf(stderr, "Error: %s (%s) is too big with a file size of %ld.\n", descr, fn, (long)st.st_size);
        goto err_out;
    }

    *p = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (*p == MAP_FAILED) {
        fprintf(stderr, "Error: could not mmap %s (%s): %m.\n", descr, fn);
        goto err_out;
    }

    rv = 0;
    *file_size = (size_t)st.st_size;

err_out:
    if (fd != -1)
        close(fd);

    return rv;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options] <dt-overlay.bin> [<dt-overlay.bin.sig>]\n"
        "\n"
        "Options:\n"
        "  -c, --vendor-code <val>  Vendor code (hex or dec, default 0x63624342 [chargebyte])\n"
        "  -r, --hw-rev <str>       HW revision (e.g. V0R1a)\n"
        "  -o, --order-code <str>   Order code string (max 32 bytes)\n"
        "  -h, --help               Show this help\n",
        prog);
}

int main(int argc, char **argv)
{
    uint32_t vendor_code = CB_DT_EERPOM_VC_CHARGEBYTE;
    const char *order_code = NULL;
    uint32_t hw_rev = 0;
    void *dt_map = NULL, *sig_map = NULL;
    size_t dt_size = 0, sig_size = 0;
    struct cb_dt_eeprom_header hdr;
    int opt;

    static const struct option long_opts[] = {
        { "vendor-code", required_argument, NULL, 'c' },
        { "hw-rev",      required_argument, NULL, 'r' },
        { "order-code",  required_argument, NULL, 'o' },
        { "help",        no_argument,       NULL, 'h' },
        { NULL,          0,                 NULL,  0  }
    };

    while ((opt = getopt_long(argc, argv, "c:r:o:h", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'c':
            vendor_code = strtoul(optarg, NULL, 0);
            break;
        case 'r':
            if (parse_hw_rev(optarg, &hw_rev) != 0) {
                fprintf(stderr, "Invalid hw_rev format: '%s'\n", optarg);
                return 1;
            }
            break;
        case 'o':
            order_code = optarg;
            break;
        case 'h':
        default:
            usage(argv[0]);
            return 1;
        }
    }

    argc -= optind;

    if (argc != 1 && argc != 2) {
        usage(argv[0]);
        return 1;
    }

    argv += optind;

    if (mmap_file(&dt_map, &dt_size, "DT overlay", argv[0]) < 0)
        return 1;

    if (argc == 2) {
        if (mmap_file(&sig_map, &sig_size, "DT overlay signature", argv[1]) < 0)
            return 1;
    }

    memset(&hdr, 0, sizeof(hdr));

    hdr.signature         = htobe16(CB_DT_EEPROM_SIGNATURE);
    hdr.header_version    = CB_DT_EERPOM_HDR_VERSION_1;
    hdr.dt_offset         = htole16(CB_DT_EERPOM_HDR_SIZE);
    hdr.dt_size           = htole16(dt_size);
    if (sig_map) {
        hdr.dt_sig_offset = htole16(le16toh(hdr.dt_offset) + le16toh(hdr.dt_size));
    }
    hdr.dt_sig_size       = htole16(sig_size);
    hdr.vendor_code       = htole32(vendor_code);

    if (le32toh(hdr.vendor_code) == CB_DT_EERPOM_VC_CHARGEBYTE) {
        hdr.vendor_specific.cb_dt_chargebyte.hw_rev = htole32(hw_rev);

        if (order_code) {
            strncpy((char *)hdr.vendor_specific.cb_dt_chargebyte.order_code,
                    order_code,
                    sizeof(hdr.vendor_specific.cb_dt_chargebyte.order_code));
        }
    }

    hdr.crc32 = htole32(crc32_uboot((const uint8_t *)&hdr, offsetof(struct cb_dt_eeprom_header, crc32)));

    fprintf(stderr, "DT overlay size           : %zu bytes\n", dt_size);
    fprintf(stderr, "DT overlay signature size : %zu bytes\n", sig_size);
    fprintf(stderr, "Header CRC32              : 0x%08x\n", le32toh(hdr.crc32));

    fwrite(&hdr, 1, sizeof(hdr), stdout);

    fwrite(dt_map, dt_size, 1, stdout);
    munmap(dt_map, dt_size);

    if (sig_map) {
        fwrite(sig_map, sig_size, 1, stdout);
        munmap(sig_map, sig_size);
    }

    return 0;
}
