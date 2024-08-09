// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/** Major version number (X.x.x) */
#define PSYCHIC_VERSION_MAJOR 2
/** Minor version number (x.X.x) */
#define PSYCHIC_VERSION_MINOR 0
/** Patch version number (x.x.X) */
#define PSYCHIC_VERSION_PATCH 0

/**
 * Macro to convert ARDUINO version number into an integer
 *
 * To be used in comparisons, such as PSYCHIC_VERSION >= PSYCHIC_VERSION_VAL(2, 0, 0)
 */
#define PSYCHIC_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current ARDUINO version, as an integer
 *
 * To be used in comparisons, such as PSYCHIC_VERSION >= PSYCHIC_VERSION_VAL(2, 0, 0)
 */
#define PSYCHIC_VERSION PSYCHIC_VERSION_VAL(PSYCHIC_VERSION_MAJOR, PSYCHIC_VERSION_MINOR, PSYCHIC_VERSION_PATCH)

/**
 * Current ARDUINO version, as string
 */
#define df2xstr(s)          #s
#define df2str(s)           df2xstr(s)
#define PSYCHIC_VERSION_STR df2str(PSYCHIC_VERSION_MAJOR) "." df2str(PSYCHIC_VERSION_MINOR) "." df2str(PSYCHIC_VERSION_PATCH)

#ifdef __cplusplus
}
#endif