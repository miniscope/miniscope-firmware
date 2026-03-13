#include "unity.h"
#include "sd_format.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_header_pack_unpack_roundtrip(void)
{
    sd_header_block_t hdr_in;
    memset(&hdr_in, 0, sizeof(hdr_in));
    hdr_in.gain = 2;
    hdr_in.led_power = 50;
    hdr_in.ewl_value = 128;
    hdr_in.record_length = 1000;
    hdr_in.fps = 20;
    hdr_in.delay_start = 500;
    hdr_in.battery_cutoff = 512;
    hdr_in.ewl_scan_mode = 1;
    hdr_in.ewl_scan_start = 10;
    hdr_in.ewl_scan_stop = 200;
    hdr_in.ewl_scan_step = 5;
    hdr_in.ewl_scan_interval = 10;

    uint8_t block[512];
    sd_format_pack_header(block, &hdr_in);

    sd_header_block_t hdr_out;
    sd_format_unpack_header(block, &hdr_out);

    TEST_ASSERT_EQUAL_UINT32(hdr_in.gain, hdr_out.gain);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.led_power, hdr_out.led_power);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_value, hdr_out.ewl_value);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.record_length, hdr_out.record_length);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.fps, hdr_out.fps);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.delay_start, hdr_out.delay_start);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.battery_cutoff, hdr_out.battery_cutoff);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_mode, hdr_out.ewl_scan_mode);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_start, hdr_out.ewl_scan_start);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_stop, hdr_out.ewl_scan_stop);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_step, hdr_out.ewl_scan_step);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_interval, hdr_out.ewl_scan_interval);
}

void test_header_gain_at_word4(void)
{
    /* Verify gain is at byte offset 16 (word 4), matching reference */
    sd_header_block_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.gain = 0xDEADBEEF;

    uint8_t block[512];
    sd_format_pack_header(block, &hdr);

    /* Word 4 = byte offset 16 */
    uint32_t val;
    memcpy(&val, &block[16], 4);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, val);
}

void test_config_pack_unpack_roundtrip(void)
{
    sd_config_block_t cfg_in;
    memset(&cfg_in, 0, sizeof(cfg_in));
    cfg_in.width = 608;
    cfg_in.height = 608;
    cfg_in.fps = 20;
    cfg_in.buffer_size = 20480;
    cfg_in.buffers_recorded = 5000;
    cfg_in.buffers_dropped = 3;

    uint8_t block[512];
    sd_format_pack_config(block, &cfg_in);

    sd_config_block_t cfg_out;
    sd_format_unpack_config(block, &cfg_out);

    TEST_ASSERT_EQUAL_UINT32(cfg_in.width, cfg_out.width);
    TEST_ASSERT_EQUAL_UINT32(cfg_in.height, cfg_out.height);
    TEST_ASSERT_EQUAL_UINT32(cfg_in.fps, cfg_out.fps);
    TEST_ASSERT_EQUAL_UINT32(cfg_in.buffer_size, cfg_out.buffer_size);
    TEST_ASSERT_EQUAL_UINT32(cfg_in.buffers_recorded, cfg_out.buffers_recorded);
    TEST_ASSERT_EQUAL_UINT32(cfg_in.buffers_dropped, cfg_out.buffers_dropped);
}

void test_buffer_meta_write_read_roundtrip(void)
{
    sd_buffer_meta_t meta_in = {
        .header_length = 12,
        .linked_list_pos = 3,
        .frame_number = 42,
        .buffer_count = 100,
        .frame_buffer_count = 5,
        .write_buffer_count = 98,
        .dropped_buffer_count = 2,
        .timestamp_ms = 2100,
        .data_length = 5108 * 4,
        .write_timestamp_ms = 2105,
        .battery_adc = 3200,
        .ewl_value = 128,
    };

    uint8_t buf[256];
    memset(buf, 0xFF, sizeof(buf));
    sd_format_write_meta(buf, &meta_in);

    sd_buffer_meta_t meta_out;
    sd_format_read_meta(buf, &meta_out);

    TEST_ASSERT_EQUAL_UINT32(meta_in.header_length, meta_out.header_length);
    TEST_ASSERT_EQUAL_UINT32(meta_in.linked_list_pos, meta_out.linked_list_pos);
    TEST_ASSERT_EQUAL_UINT32(meta_in.frame_number, meta_out.frame_number);
    TEST_ASSERT_EQUAL_UINT32(meta_in.buffer_count, meta_out.buffer_count);
    TEST_ASSERT_EQUAL_UINT32(meta_in.frame_buffer_count, meta_out.frame_buffer_count);
    TEST_ASSERT_EQUAL_UINT32(meta_in.write_buffer_count, meta_out.write_buffer_count);
    TEST_ASSERT_EQUAL_UINT32(meta_in.dropped_buffer_count, meta_out.dropped_buffer_count);
    TEST_ASSERT_EQUAL_UINT32(meta_in.timestamp_ms, meta_out.timestamp_ms);
    TEST_ASSERT_EQUAL_UINT32(meta_in.data_length, meta_out.data_length);
    TEST_ASSERT_EQUAL_UINT32(meta_in.write_timestamp_ms, meta_out.write_timestamp_ms);
    TEST_ASSERT_EQUAL_UINT32(meta_in.battery_adc, meta_out.battery_adc);
    TEST_ASSERT_EQUAL_UINT32(meta_in.ewl_value, meta_out.ewl_value);
}

void test_header_block_is_512_bytes(void)
{
    TEST_ASSERT_EQUAL(512, sizeof(sd_header_block_t));
}

void test_config_block_is_512_bytes(void)
{
    TEST_ASSERT_EQUAL(512, sizeof(sd_config_block_t));
}

void test_meta_is_48_bytes(void)
{
    TEST_ASSERT_EQUAL(48, sizeof(sd_buffer_meta_t));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_header_pack_unpack_roundtrip);
    RUN_TEST(test_header_gain_at_word4);
    RUN_TEST(test_config_pack_unpack_roundtrip);
    RUN_TEST(test_buffer_meta_write_read_roundtrip);
    RUN_TEST(test_header_block_is_512_bytes);
    RUN_TEST(test_config_block_is_512_bytes);
    RUN_TEST(test_meta_is_48_bytes);
    return UNITY_END();
}
