/**
  * @file    ds3231.c
  * @author  Leonardo Costamagna
  * @brief   DS3231 RTC driver implementation.
  */

#include "ds3231.h"

/* Low-level Register Read/Write Weak Functions */

int32_t __weak ds3231_read_reg(const dev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
  if (ctx == NULL)
  {
    return -1;
  }
  return ctx->read_reg(ctx->handle, reg, data, len);
}

int32_t __weak ds3231_write_reg(const dev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
  if (ctx == NULL)
  {
    return -1;
  }
  return ctx->write_reg(ctx->handle, reg, data, len);
}

/* Helper Utilities */

uint8_t ds3231_bcd_to_dec(uint8_t val)
{
  return (uint8_t)(((val >> 4) * 10) + (val & 0x0FU));
}

uint8_t ds3231_dec_to_bcd(uint8_t val)
{
  return (uint8_t)(((val / 10U) << 4) | (val % 10U));
}

float_t ds3231_from_lsb_to_celsius(int16_t lsb)
{
  return ((float_t)lsb / 256.0f);
}

/* Time/Date Operations */

int32_t ds3231_time_set(const dev_ctx_t *ctx, const ds3231_time_t *val)
{
  uint8_t reg[7];
  int32_t ret;

  if (val == NULL)
  {
    return -1;
  }

  reg[0] = ds3231_dec_to_bcd(val->seconds);
  reg[1] = ds3231_dec_to_bcd(val->minutes);
  reg[2] = ds3231_dec_to_bcd(val->hours);
  reg[3] = ds3231_dec_to_bcd(val->day_of_week);
  reg[4] = ds3231_dec_to_bcd(val->day);
  reg[5] = ds3231_dec_to_bcd(val->month);
  reg[6] = ds3231_dec_to_bcd((uint8_t)(val->year % 100U));

  ret = ds3231_write_reg(ctx, DS3231_SECONDS, reg, 7);
  return ret;
}

int32_t ds3231_time_get(const dev_ctx_t *ctx, ds3231_time_t *val)
{
  uint8_t reg[7];
  int32_t ret;

  if (val == NULL)
  {
    return -1;
  }

  ret = ds3231_read_reg(ctx, DS3231_SECONDS, reg, 7);
  if (ret == 0)
  {
    val->seconds     = ds3231_bcd_to_dec(reg[0] & 0x7FU);
    val->minutes     = ds3231_bcd_to_dec(reg[1] & 0x7FU);
    val->hours       = ds3231_bcd_to_dec(reg[2] & 0x3FU);
    val->day_of_week = ds3231_bcd_to_dec(reg[3] & 0x07U);
    val->day         = ds3231_bcd_to_dec(reg[4] & 0x3FU);
    val->month       = ds3231_bcd_to_dec(reg[5] & 0x1FU);
    val->year        = 2000U + ds3231_bcd_to_dec(reg[6]);
  }

  return ret;
}

/* Temperature Measurements */

int32_t ds3231_temperature_raw_get(const dev_ctx_t *ctx, int16_t *buff)
{
  uint8_t reg[2];
  int32_t ret;

  ret = ds3231_read_reg(ctx, DS3231_TEMP_MSB, reg, 2);
  if (ret == 0)
  {
    *buff = (int16_t)(((uint16_t)reg[0] << 8) | reg[1]);
  }

  return ret;
}

int32_t ds3231_temperature_get(const dev_ctx_t *ctx, float_t *val)
{
  int16_t raw_temp = 0;
  int32_t ret;

  ret = ds3231_temperature_raw_get(ctx, &raw_temp);
  if (ret == 0)
  {
    *val = ds3231_from_lsb_to_celsius(raw_temp);
  }

  return ret;
}

/* Alarm Configuration */

int32_t ds3231_alarm1_set(const dev_ctx_t *ctx, const ds3231_alarm1_t *val)
{
  uint8_t reg[4];
  int32_t ret;

  if (val == NULL)
  {
    return -1;
  }

  reg[0] = ds3231_dec_to_bcd(val->seconds)  | (((uint8_t)val->mode & 0x01U) << 7);
  reg[1] = ds3231_dec_to_bcd(val->minutes)  | (((uint8_t)val->mode & 0x02U) << 6);
  reg[2] = ds3231_dec_to_bcd(val->hours)    | (((uint8_t)val->mode & 0x04U) << 5);
  reg[3] = ds3231_dec_to_bcd(val->day_date) | (((uint8_t)val->mode & 0x08U) << 4);

  ret = ds3231_write_reg(ctx, DS3231_ALARM1_SECONDS, reg, 4);
  return ret;
}

int32_t ds3231_alarm1_interrupt_en_set(const dev_ctx_t *ctx, uint8_t val)
{
  ds3231_control_t control;
  int32_t ret;

  ret = ds3231_read_reg(ctx, DS3231_CONTROL, (uint8_t *)&control, 1);
  if (ret == 0)
  {
    control.a1ie = (val & 0x01U);
    control.intcn = PROPERTY_ENABLE; /* Direct INT/SQW pin to reflect interrupt */
    ret = ds3231_write_reg(ctx, DS3231_CONTROL, (uint8_t *)&control, 1);
  }

  return ret;
}

/* Device Status Management */

int32_t ds3231_status_get(const dev_ctx_t *ctx, ds3231_stat_t *val)
{
  ds3231_status_t status;
  int32_t ret;

  ret = ds3231_read_reg(ctx, DS3231_STATUS, (uint8_t *)&status, 1);
  if (ret == 0)
  {
    val->alarm1_flag = status.a1f;
    val->alarm2_flag = status.a2f;
    val->busy        = status.bsy;
    val->osf         = status.osf;
  }

  return ret;
}

int32_t ds3231_status_clear(const dev_ctx_t *ctx, ds3231_stat_t val)
{
  ds3231_status_t status;
  int32_t ret;

  ret = ds3231_read_reg(ctx, DS3231_STATUS, (uint8_t *)&status, 1);
  if (ret == 0)
  {
    if (val.alarm1_flag) status.a1f = 0;
    if (val.alarm2_flag) status.a2f = 0;
    if (val.osf)         status.osf = 0;

    ret = ds3231_write_reg(ctx, DS3231_STATUS, (uint8_t *)&status, 1);
  }

  return ret;
}