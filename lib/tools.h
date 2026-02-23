/*
 * Copyright © 2026 chargebyte GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int parse_hw_rev(const char *s, uint32_t *out);

#ifdef __cplusplus
}
#endif
