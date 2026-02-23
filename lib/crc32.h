/*
 * Copyright © 2026 chargebyte GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t crc32_uboot(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
