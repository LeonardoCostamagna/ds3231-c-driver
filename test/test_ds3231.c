/**
 * @file test_ds3231.c
 * @brief Unit tests for the DS3231 driver using Ceedling, Unity, and CMock.
 */

#include "unity.h"
#include "ds3231.h"
#include "mock_interface.h"

static dev_ctx_t ctx;
static uint32_t dummy_handle = 0x1234;

void setUp(void) {
    ctx.read_reg = mock_i2c_read;
    ctx.write_reg = mock_i2c_write;
    ctx.mdelay = NULL;
    ctx.handle = &dummy_handle;
    ctx.priv_data = NULL;
}

void tearDown(void) {
}

/* ============================================================================
   1. UTILITY AND CONVERSION TESTS (Pure Functions)
   ============================================================================ */

void test_ds3231_bcd_to_dec_and_dec_to_bcd_limits_and_symmetry(void) {
    /* Boundary Cases */
    TEST_ASSERT_EQUAL_UINT8(0, ds3231_bcd_to_dec(0x00));
    TEST_ASSERT_EQUAL_UINT8(59, ds3231_bcd_to_dec(0x59));
    TEST_ASSERT_EQUAL_UINT8(99, ds3231_bcd_to_dec(0x99));

    TEST_ASSERT_EQUAL_UINT8(0x00, ds3231_dec_to_bcd(0));
    TEST_ASSERT_EQUAL_UINT8(0x59, ds3231_dec_to_bcd(59));
    TEST_ASSERT_EQUAL_UINT8(0x99, ds3231_dec_to_bcd(99));

    /* Symmetry / Involution */
    for (uint8_t i = 0; i <= 99; i++) {
        TEST_ASSERT_EQUAL_UINT8(i, ds3231_bcd_to_dec(ds3231_dec_to_bcd(i)));
    }
}

void test_ds3231_from_lsb_to_celsius(void) {
    /* Zero Value */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, ds3231_from_lsb_to_celsius(0));

    /* Positive Temperature: 25.0 °C -> LSB = 25 * 256 = 6400 (0x1900) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, ds3231_from_lsb_to_celsius(6400));

    /* Positive Temperature with Fractional Part: +25.75 °C -> 25.75 * 256 = 6592 (0x19C0) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.75f, ds3231_from_lsb_to_celsius(6592));

    /* Negative Temperature (Two's Complement): -2.0 °C -> -2 * 256 = -512 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, ds3231_from_lsb_to_celsius(-512));

    /* Negative Temperature with Fractional Part: -10.25 °C -> -10.25 * 256 = -2624 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.25f, ds3231_from_lsb_to_celsius(-2624));
}

/* ============================================================================
   2. DEFENSIVE PROGRAMMING TESTS (NULL Pointers)
   ============================================================================ */

void test_ds3231_null_context_guard(void) {
    uint8_t dummy_data = 0;
    ds3231_time_t time;
    ds3231_alarm1_t alarm1;
    ds3231_stat_t status;
    float_t temp;
    int16_t raw_temp;

    TEST_ASSERT_EQUAL_INT32(-1, ds3231_read_reg(NULL, DS3231_STATUS, &dummy_data, 1));
    TEST_ASSERT_EQUAL_INT32(-1, ds3231_write_reg(NULL, DS3231_STATUS, &dummy_data, 1));
    TEST_ASSERT_EQUAL_INT32(-1, ds3231_time_set(&ctx, NULL));
    TEST_ASSERT_EQUAL_INT32(-1, ds3231_time_set(NULL, &time));
    TEST_ASSERT_EQUAL_INT32(-1, ds3231_time_get(&ctx, NULL));
    TEST_ASSERT_EQUAL_INT32(-1, ds3231_time_get(NULL, &time));
    TEST_ASSERT_EQUAL_INT32(-1, ds3231_alarm1_set(&ctx, NULL));
    TEST_ASSERT_EQUAL_INT32(-1, ds3231_alarm1_set(NULL, &alarm1));
}

/* ============================================================================
   3. TIME AND DATE OPERATIONS (ds3231_time_set / ds3231_time_get)
   ============================================================================ */

void test_ds3231_time_set_success(void) {
    ds3231_time_t time = {
        .seconds = 30,
        .minutes = 15,
        .hours = 10,
        .day_of_week = 4,
        .day = 1,
        .month = 10,
        .year = 2026
    };

    /* Expected BCD values: [0x30, 0x15, 0x10, 0x04, 0x01, 0x10, 0x26] */
    uint8_t expected_reg[7] = { 0x30, 0x15, 0x10, 0x04, 0x01, 0x10, 0x26 };

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_handle, 1, DS3231_SECONDS, expected_reg, 7, 7, 0);

    TEST_ASSERT_EQUAL_INT32(0, ds3231_time_set(&ctx, &time));
}

void test_ds3231_time_get_success(void) {
    ds3231_time_t time_out;
    /* Simulate RTC response with mask flags:
       sec: 0x15, min: 0x42, hr: 0x21 (24h mode), dow: 0x05, day: 0x18, month: 0x09, year: 0x26 */
    uint8_t simulated_reg[7] = { 0x15, 0x42, 0x21, 0x05, 0x18, 0x09, 0x26 };

    mock_i2c_read_ExpectAndReturn(&dummy_handle, DS3231_SECONDS, NULL, 7, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(simulated_reg, 7);

    TEST_ASSERT_EQUAL_INT32(0, ds3231_time_get(&ctx, &time_out));
    TEST_ASSERT_EQUAL_UINT8(15, time_out.seconds);
    TEST_ASSERT_EQUAL_UINT8(42, time_out.minutes);
    TEST_ASSERT_EQUAL_UINT8(21, time_out.hours);
    TEST_ASSERT_EQUAL_UINT8(5,  time_out.day_of_week);
    TEST_ASSERT_EQUAL_UINT8(18, time_out.day);
    TEST_ASSERT_EQUAL_UINT8(9,  time_out.month);
    TEST_ASSERT_EQUAL_UINT16(2026, time_out.year);
}

/* ============================================================================
   4. TEMPERATURE (ds3231_temperature_raw_get / ds3231_temperature_get)
   ============================================================================ */

void test_ds3231_temperature_get_success(void) {
    float_t temp_celsius = 0.0f;
    /* MSB = 0x19 (25 decimal), LSB = 0xC0 (+0.75 °C) -> Raw = 0x19C0 (6592) -> 25.75 °C */
    uint8_t simulated_bytes[2] = { 0x19, 0xC0 };

    mock_i2c_read_ExpectAndReturn(&dummy_handle, DS3231_TEMP_MSB, NULL, 2, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(simulated_bytes, 2);

    TEST_ASSERT_EQUAL_INT32(0, ds3231_temperature_get(&ctx, &temp_celsius));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.75f, temp_celsius);
}

/* ============================================================================
   5. ALARM AND INTERRUPT CONFIGURATION
   ============================================================================ */

void test_ds3231_alarm1_set_success(void) {
    ds3231_alarm1_t alarm1 = {
        .seconds = 10,
        .minutes = 20,
        .hours = 12,
        .day_date = 5,
        .mode = DS3231_ALARM_1_MATCH_HRS_MIN_SEC /* mode = 0x08 -> bit3=1 (A1M4=1 in reg[3]) */
    };

    /* BCD + Mode Bit Shift:
       reg[0] = 0x10 | ((0x08 & 0x01) << 7) = 0x10
       reg[1] = 0x20 | ((0x08 & 0x02) << 6) = 0x20
       reg[2] = 0x12 | ((0x08 & 0x04) << 5) = 0x12
       reg[3] = 0x05 | ((0x08 & 0x08) << 4) = 0x85 */
    uint8_t expected_reg[4] = { 0x10, 0x20, 0x12, 0x85 };

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_handle, 1, DS3231_ALARM1_SECONDS, expected_reg, 4, 4, 0);

    TEST_ASSERT_EQUAL_INT32(0, ds3231_alarm1_set(&ctx, &alarm1));
}

void test_ds3231_alarm1_interrupt_en_set_read_modify_write(void) {
    /* 1. Simulate initial CONTROL read: a1ie=0, intcn=0, bbsqw=1, eosc=0 (0x40) */
    uint8_t initial_ctrl = 0x40;
    /* 2. Expected final value: a1ie=1, intcn=1 (PROPERTY_ENABLE) -> 0x40 | 0x01 | 0x04 = 0x45 */
    uint8_t expected_ctrl = 0x45;

    mock_i2c_read_ExpectAndReturn(&dummy_handle, DS3231_CONTROL, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&initial_ctrl, 1);

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_handle, 1, DS3231_CONTROL, &expected_ctrl, 1, 1, 0);

    TEST_ASSERT_EQUAL_INT32(0, ds3231_alarm1_interrupt_en_set(&ctx, PROPERTY_ENABLE));
}

/* ============================================================================
   6. STATUS AND SELECTIVE CLEARING (ds3231_status_get / ds3231_status_clear)
   ============================================================================ */

void test_ds3231_status_get_success(void) {
    ds3231_stat_t stat;
    /* Simulate STATUS register with a1f=1, a2f=0, bsy=1, osf=1 -> bit0=1, bit1=0, bit2=1, bit7=1 (0x85) */
    uint8_t simulated_status = 0x85;

    mock_i2c_read_ExpectAndReturn(&dummy_handle, DS3231_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&simulated_status, 1);

    TEST_ASSERT_EQUAL_INT32(0, ds3231_status_get(&ctx, &stat));
    TEST_ASSERT_EQUAL_UINT8(1, stat.alarm1_flag);
    TEST_ASSERT_EQUAL_UINT8(0, stat.alarm2_flag);
    TEST_ASSERT_EQUAL_UINT8(1, stat.busy);
    TEST_ASSERT_EQUAL_UINT8(1, stat.osf);
}

void test_ds3231_status_clear_selective(void) {
    /* Simulate initial hardware state with all flags set (a1f=1, a2f=1, osf=1 -> 0x83) */
    uint8_t initial_status = 0x83;
    /* Request to clear only alarm1_flag (val.alarm1_flag = 1) */
    ds3231_stat_t clear_mask = { .alarm1_flag = 1, .alarm2_flag = 0, .osf = 0 };
    /* Expected result: a1f becomes 0; a2f and osf remain 1 (0x82) */
    uint8_t expected_status = 0x82;

    mock_i2c_read_ExpectAndReturn(&dummy_handle, DS3231_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&initial_status, 1);

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_handle, 1, DS3231_STATUS, &expected_status, 1, 1, 0);

    TEST_ASSERT_EQUAL_INT32(0, ds3231_status_clear(&ctx, clear_mask));
}

/* ============================================================================
   7. I2C BUS ERROR HANDLING AND PROPAGATION
   ============================================================================ */

void test_ds3231_time_get_bus_error_propagation(void) {
    ds3231_time_t time_out;

    /* Simulate error on I2C read (-1) */
    mock_i2c_read_ExpectAndReturn(&dummy_handle, DS3231_SECONDS, NULL, 7, -1);
    mock_i2c_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL_INT32(-1, ds3231_time_get(&ctx, &time_out));
}

void test_ds3231_alarm1_interrupt_en_set_read_error_propagation(void) {
    /* If the initial read fails during Read-Modify-Write, it must abort without writing */
    mock_i2c_read_ExpectAndReturn(&dummy_handle, DS3231_CONTROL, NULL, 1, -1);
    mock_i2c_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL_INT32(-1, ds3231_alarm1_interrupt_en_set(&ctx, PROPERTY_ENABLE));
}