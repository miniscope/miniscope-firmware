#ifndef SD_FORMAT_H
#define SD_FORMAT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * SD card data layout structures for Miniscope recordings.
 *
 * This module is pure data — no hardware dependency. It can be
 * tested on the host.
 *
 * Block layout:
 *   Block 1022: Header block (recording parameters from host)
 *   Block 1023: Config block (device parameters, written at end)
 *   Block 1024+: Data buffers (capture data with per-buffer metadata)
 *
 * Struct layouts match the reference firmware (definitions.h) exactly.
 */

#define SD_BLOCK_SIZE 512

/**
 * Header block (block 1022) — written by host before recording.
 *
 * Reference layout (definitions.h lines 89-103):
 *   Words 0-3: reserved (unused/write keys)
 *   Word 4:    gain
 *   Word 5:    led_power
 *   Word 6:    ewl_value
 *   Word 7:    record_length
 *   Word 8:    fps
 *   Word 9:    delay_start
 *   Word 10:   battery_cutoff
 *   Word 11:   ewl_scan_mode
 *   Word 12:   ewl_scan_start
 *   Word 13:   ewl_scan_stop
 *   Word 14:   ewl_scan_step
 *   Word 15:   ewl_scan_interval
 */
typedef struct __attribute__((packed)) {
    uint32_t reserved_keys[4];      /* Words 0-3: unused */
    uint32_t gain;                  /* Word 4 */
    uint32_t led_power;             /* Word 5 */
    uint32_t ewl_value;             /* Word 6 */
    uint32_t record_length;         /* Word 7 */
    uint32_t fps;                   /* Word 8 */
    uint32_t delay_start;           /* Word 9 */
    uint32_t battery_cutoff;        /* Word 10 */
    uint32_t ewl_scan_mode;         /* Word 11 */
    uint32_t ewl_scan_start;        /* Word 12 */
    uint32_t ewl_scan_stop;         /* Word 13 */
    uint32_t ewl_scan_step;         /* Word 14 */
    uint32_t ewl_scan_interval;     /* Word 15 */
    uint8_t  reserved[SD_BLOCK_SIZE - 64]; /* Pad to 512 bytes */
} sd_header_block_t;

/**
 * Config block (block 1023) — written by device at end of recording.
 *
 * Reference layout (definitions.h lines 55-60):
 *   Word 0: width
 *   Word 1: height
 *   Word 2: fps
 *   Word 3: buffer_size
 *   Word 4: buffers_recorded
 *   Word 5: buffers_dropped
 */
typedef struct __attribute__((packed)) {
    uint32_t width;             /* Image width in pixels */
    uint32_t height;            /* Image height in pixels */
    uint32_t fps;               /* Actual FPS achieved */
    uint32_t buffer_size;       /* Bytes per capture buffer */
    uint32_t buffers_recorded;  /* Total buffers successfully written */
    uint32_t buffers_dropped;   /* Buffers dropped due to SD write lag */
    uint8_t  reserved[SD_BLOCK_SIZE - 24]; /* Pad to 512 bytes */
} sd_config_block_t;

/**
 * Per-buffer metadata header (first 48 bytes / 12 words of each data buffer).
 *
 * Reference layout (definitions.h lines 37-51):
 *   Word 0:  header_length (always 12)
 *   Word 1:  linked_list_pos (bufferCount % NUM_BUFFERS)
 *   Word 2:  frame_number
 *   Word 3:  buffer_count (global)
 *   Word 4:  frame_buffer_count (buffers within current frame)
 *   Word 5:  write_buffer_count (SD writes completed)
 *   Word 6:  dropped_buffer_count
 *   Word 7:  timestamp_ms (ms since recording start)
 *   Word 8:  data_length (actual data bytes in buffer)
 *   Word 9:  write_timestamp_ms (ms when written to SD)
 *   Word 10: battery_adc
 *   Word 11: ewl_value
 */
typedef struct __attribute__((packed)) {
    uint32_t header_length;         /* [0]  Always 12 */
    uint32_t linked_list_pos;       /* [1]  bufferCount % NUM_BUFFERS */
    uint32_t frame_number;          /* [2]  Frame index */
    uint32_t buffer_count;          /* [3]  Global buffer count */
    uint32_t frame_buffer_count;    /* [4]  Buffers within current frame */
    uint32_t write_buffer_count;    /* [5]  SD write count */
    uint32_t dropped_buffer_count;  /* [6]  Dropped count */
    uint32_t timestamp_ms;          /* [7]  ms since recording start */
    uint32_t data_length;           /* [8]  Actual data bytes in buffer */
    uint32_t write_timestamp_ms;    /* [9]  ms when written to SD */
    uint32_t battery_adc;           /* [10] Battery voltage ADC */
    uint32_t ewl_value;             /* [11] EWL voltage */
} sd_buffer_meta_t;

/**
 * Pack a header block into a 512-byte buffer.
 * @param block  Output buffer (must be 512 bytes)
 * @param hdr    Header data to pack
 */
void sd_format_pack_header(uint8_t *block, const sd_header_block_t *hdr);

/**
 * Unpack a header block from a 512-byte buffer.
 * @param block  Input buffer (512 bytes)
 * @param hdr    Output header struct
 */
void sd_format_unpack_header(const uint8_t *block, sd_header_block_t *hdr);

/**
 * Pack a config block into a 512-byte buffer.
 */
void sd_format_pack_config(uint8_t *block, const sd_config_block_t *cfg);

/**
 * Unpack a config block from a 512-byte buffer.
 */
void sd_format_unpack_config(const uint8_t *block, sd_config_block_t *cfg);

/**
 * Write buffer metadata header into the start of a data buffer.
 * @param buf   Start of data buffer
 * @param meta  Metadata to write
 */
void sd_format_write_meta(uint8_t *buf, const sd_buffer_meta_t *meta);

/**
 * Read buffer metadata header from the start of a data buffer.
 */
void sd_format_read_meta(const uint8_t *buf, sd_buffer_meta_t *meta);

#endif /* SD_FORMAT_H */
