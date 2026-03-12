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
 */

#define SD_BLOCK_SIZE 512

/** Header block (block 1022) — written by host before recording */
typedef struct __attribute__((packed)) {
    uint32_t gain;              /* Sensor gain setting */
    uint32_t led_power;         /* LED PWM duty (0-100) */
    uint32_t ewl_value;         /* EWL voltage (0-255) */
    uint32_t record_length;     /* Max frames to record (0 = unlimited) */
    uint32_t fps;               /* Target frames per second */
    uint32_t delay_start;       /* Delay before recording starts (ms) */
    uint32_t battery_cutoff;    /* Battery ADC threshold for auto-stop */
    uint32_t ewl_scan_start;    /* EWL scan start voltage */
    uint32_t ewl_scan_stop;     /* EWL scan stop voltage */
    uint32_t ewl_scan_step;     /* EWL scan step size */
    uint32_t ewl_scan_interval; /* Frames between EWL steps */
    uint8_t  reserved[SD_BLOCK_SIZE - 44]; /* Pad to 512 bytes */
} sd_header_block_t;

/** Config block (block 1023) — written by device at end of recording */
typedef struct __attribute__((packed)) {
    uint32_t width;             /* Image width in pixels */
    uint32_t height;            /* Image height in pixels */
    uint32_t fps;               /* Actual FPS achieved */
    uint32_t buffer_size;       /* Bytes per capture buffer */
    uint32_t buffers_recorded;  /* Total buffers successfully written */
    uint32_t buffers_dropped;   /* Buffers dropped due to SD write lag */
    uint32_t total_frames;      /* Total frames captured */
    uint8_t  reserved[SD_BLOCK_SIZE - 28]; /* Pad to 512 bytes */
} sd_config_block_t;

/** Per-buffer metadata header (first 48 bytes of each data buffer) */
typedef struct __attribute__((packed)) {
    uint32_t frame_number;      /* Frame index since recording start */
    uint32_t timestamp_ms;      /* Milliseconds since recording start */
    uint32_t buffer_count;      /* Buffer index since recording start */
    uint32_t battery_adc;       /* Battery ADC reading at capture time */
    uint32_t ewl_value;         /* EWL voltage at capture time */
    uint32_t led_power;         /* LED duty at capture time */
    uint32_t gain;              /* Sensor gain at capture time */
    uint32_t dropped_count;     /* Cumulative dropped buffers */
    uint32_t flags;             /* Bit flags (frame valid, etc.) */
    uint32_t reserved[3];       /* Future use */
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
