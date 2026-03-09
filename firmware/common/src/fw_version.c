#include "fw_version.h"

static const fw_version_t s_version = {
    .major = FW_VERSION_MAJOR,
    .minor = FW_VERSION_MINOR,
    .patch = FW_VERSION_PATCH,
    .version_string = FW_VERSION_STRING,
    .git_hash = FW_VERSION_GIT_HASH,
};

const fw_version_t *fw_get_version(void)
{
    return &s_version;
}
