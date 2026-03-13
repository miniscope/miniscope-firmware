#ifndef FW_VERSION_H
#define FW_VERSION_H

/**
 * Mock fw_version.h for host-based testing.
 */

#include <stdint.h>

#define FW_VERSION_MAJOR    0
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0
#define FW_VERSION_STRING   "0.0.0-test"
#define FW_VERSION_GIT_HASH "0000000"

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    const char *version_string;
    const char *git_hash;
} fw_version_t;

static inline const fw_version_t *fw_get_version(void)
{
    static const fw_version_t v = {0, 0, 0, "0.0.0-test", "0000000"};
    return &v;
}

#endif /* FW_VERSION_H */
