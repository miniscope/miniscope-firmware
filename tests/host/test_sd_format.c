#include "unity.h"
#include "sd_format.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_header_pack_unpack_roundtrip(void)
{
    sd_header_block_t hdr_in = {
        .gain = 2,
        .led_power = 50,
        .ewl_value = 128,
        .record_length = 1000,
        .fps = 20,
        .delay_start = 500,
        .battery_cutoff = 512,
        .ewl_scan_start = 10,
        .ewl_scan_stop = 200,
        .ewl_scan_step = 5,
        .ewl_scan_interval = 10,
    };
    memset(hdr_in.reserved, 0, sizeof(hdr_in.reserved));

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
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_start, hdr_out.ewl_scan_start);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_stop, hdr_out.ewl_scan_stop);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_step, hdr_out.ewl_scan_step);
    TEST_ASSERT_EQUAL_UINT32(hdr_in.ewl_scan_interval, hdr_out.ewl_scan_interval);
}

void test_config_pack_unpack_roundtrip(void)
{
    sd_config_block_t cfg_in = {
        .width = 608,
        .height = 480,
        .fps = 20,
        .buffer_size = 20480,
        .buffers_recorded = 5000,
        .buffers_dropped = 3,
        .total_frames = 5003,
    };
    memset(cfg_in.reserved, 0, sizeof(cfg_in.reserved));

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
    TEST_ASSERT_EQUAL_UINT32(cfg_in.total_frames, cfg_out.total_frames);
}

void test_buffer_meta_write_read_roundtrip(void)
{
    sd_buffer_meta_t meta_in = {
        .frame_number = 42,
        .timestamp_ms = 2100,
        .buffer_count = 42,
        .battery_adc = 3200,
        .ewl_value = 128,
        .led_power = 50,
        .gain = 1,
        .dropped_count = 0,
        .flags = 0x01,
    };
    memset(meta_in.reserved, 0, sizeof(meta_in.reserved));

    uint8_t buf[256];
    memset(buf, 0xFF, sizeof(buf));
    sd_format_write_meta(buf, &meta_in);

    sd_buffer_meta_t meta_out;
    sd_format_read_meta(buf, &meta_out);

    TEST_ASSERT_EQUAL_UINT32(meta_in.frame_number, meta_out.frame_number);
    TEST_ASSERT_EQUAL_UINT32(meta_in.timestamp_ms, meta_out.timestamp_ms);
    TEST_ASSERT_EQUAL_UINT32(meta_in.buffer_count, meta_out.buffer_count);
    TEST_ASSERT_EQUAL_UINT32(meta_in.battery_adc, meta_out.battery_adc);
    TEST_ASSERT_EQUAL_UINT32(meta_in.ewl_value, meta_out.ewl_value);
    TEST_ASSERT_EQUAL_UINT32(meta_in.led_power, meta_out.led_power);
    TEST_ASSERT_EQUAL_UINT32(meta_in.gain, meta_out.gain);
    TEST_ASSERT_EQUAL_UINT32(meta_in.dropped_count, meta_out.dropped_count);
    TEST_ASSERT_EQUAL_UINT32(meta_in.flags, meta_out.flags);
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
    RUN_TEST(test_config_pack_unpack_roundtrip);
    RUN_TEST(test_buffer_meta_write_read_roundtrip);
    RUN_TEST(test_header_block_is_512_bytes);
    RUN_TEST(test_config_block_is_512_bytes);
    RUN_TEST(test_meta_is_48_bytes);
    return UNITY_END();
}
