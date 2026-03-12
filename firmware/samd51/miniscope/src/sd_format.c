#include "sd_format.h"
#include <string.h>

void sd_format_pack_header(uint8_t *block, const sd_header_block_t *hdr)
{
    memcpy(block, hdr, sizeof(sd_header_block_t));
}

void sd_format_unpack_header(const uint8_t *block, sd_header_block_t *hdr)
{
    memcpy(hdr, block, sizeof(sd_header_block_t));
}

void sd_format_pack_config(uint8_t *block, const sd_config_block_t *cfg)
{
    memcpy(block, cfg, sizeof(sd_config_block_t));
}

void sd_format_unpack_config(const uint8_t *block, sd_config_block_t *cfg)
{
    memcpy(cfg, block, sizeof(sd_config_block_t));
}

void sd_format_write_meta(uint8_t *buf, const sd_buffer_meta_t *meta)
{
    memcpy(buf, meta, sizeof(sd_buffer_meta_t));
}

void sd_format_read_meta(const uint8_t *buf, sd_buffer_meta_t *meta)
{
    memcpy(meta, buf, sizeof(sd_buffer_meta_t));
}
