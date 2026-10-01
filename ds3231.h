/**
  * @file    ds3231.h
  * @author  Leonardo Costamagna
  * @brief   Header file containing API declarations, register definitions,
  *          and context structures for the DS3231 RTC driver.
  */

#ifndef DS3231_H
#define DS3231_H

/* Ensures C linkage compatibility when compiled with C++, preventing name mangling. */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <math.h>

/**
  * @defgroup DS3231_Driver DS3231 Real-Time Clock Driver
  * @{
  */

/**
  * @defgroup DS3231_Endianness Byte Ordering Definitions
  * @brief Configures target architecture byte alignment.
  * @{
  */

/* Detects target endianness to ensure portable and correct bit-field register mapping. */
#ifndef DRV_BYTE_ORDER
#ifndef __BYTE_ORDER__

/* Fallback definitions when toolchain does not supply built-in byte order macros */
#define DRV_LITTLE_ENDIAN 1234
#define DRV_BIG_ENDIAN    4321
#define DRV_BYTE_ORDER    DRV_LITTLE_ENDIAN

#else /* defined __BYTE_ORDER__ */

#define DRV_LITTLE_ENDIAN  __ORDER_LITTLE_ENDIAN__
#define DRV_BIG_ENDIAN     __ORDER_BIG_ENDIAN__
#define DRV_BYTE_ORDER     __BYTE_ORDER__

#endif /* __BYTE_ORDER__ */
#endif /* DRV_BYTE_ORDER */

/**
  * @} DS3231_Endianness
  */

/**
  * @defgroup DS3231_Bus_Abstraction Hardware Bus Context & Callbacks
  * @brief Low-level function pointers and device context abstraction.
  * @{
  */

/* Shared include guard across sensor/hardware drivers to prevent type redefinition errors for common base structures and bus callbacks. */
#ifndef MEMS_SHARED_TYPES
#define MEMS_SHARED_TYPES

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t bit0       : 1;
  uint8_t bit1       : 1;
  uint8_t bit2       : 1;
  uint8_t bit3       : 1;
  uint8_t bit4       : 1;
  uint8_t bit5       : 1;
  uint8_t bit6       : 1;
  uint8_t bit7       : 1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t bit7       : 1;
  uint8_t bit6       : 1;
  uint8_t bit5       : 1;
  uint8_t bit4       : 1;
  uint8_t bit3       : 1;
  uint8_t bit2       : 1;
  uint8_t bit1       : 1;
  uint8_t bit0       : 1;
#endif /* DRV_BYTE_ORDER */
} bitwise_t;

#define PROPERTY_DISABLE                (0U)
#define PROPERTY_ENABLE                 (1U)

typedef int32_t (*dev_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*dev_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);
typedef void (*dev_mdelay_ptr)(uint32_t millisec);

typedef struct
{
  /** Bus write callback */
  dev_write_ptr    write_reg;
  /** Bus read callback */
  dev_read_ptr     read_reg;
  /** Millisecond delay handler (optional) */
  dev_mdelay_ptr   mdelay;
  /** Hardware peripheral handler reference */
  void               *handle;
  /** Reserved private driver payload */
  void               *priv_data;
} dev_ctx_t;

#endif /* MEMS_SHARED_TYPES */

/**
  * @} DS3231_Bus_Abstraction
  */

/**
  * @defgroup DS3231_Addressing Device Addressing
  * @{
  */

/** Default 8-bit I2C address for DS3231 peripheral */
#define DS3231_I2C_ADD                   0xD0U

/**
  * @} DS3231_Addressing
  */

/**
  * @defgroup DS3231_Register_Map Internal Register Map
  * @{
  */

#define DS3231_SECONDS                   0x00U
#define DS3231_MINUTES                   0x01U
#define DS3231_HOURS                     0x02U
#define DS3231_DAY                       0x03U
#define DS3231_DATE                      0x04U
#define DS3231_MONTH_CENTURY             0x05U
#define DS3231_YEAR                      0x06U

#define DS3231_ALARM1_SECONDS            0x07U
#define DS3231_ALARM1_MINUTES            0x08U
#define DS3231_ALARM1_HOURS              0x09U
#define DS3231_ALARM1_DAY_DATE           0x0AU

#define DS3231_ALARM2_MINUTES            0x0BU
#define DS3231_ALARM2_HOURS              0x0CU
#define DS3231_ALARM2_DAY_DATE           0x0DU

#define DS3231_CONTROL                   0x0EU
typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t a1ie             : 1;
  uint8_t a2ie             : 1;
  uint8_t intcn            : 1;
  uint8_t rs1              : 1;
  uint8_t rs2              : 1;
  uint8_t conv             : 1;
  uint8_t bbsqw            : 1;
  uint8_t eosc             : 1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t eosc             : 1;
  uint8_t bbsqw            : 1;
  uint8_t conv             : 1;
  uint8_t rs2              : 1;
  uint8_t rs1              : 1;
  uint8_t intcn            : 1;
  uint8_t a2ie             : 1;
  uint8_t a1ie             : 1;
#endif /* DRV_BYTE_ORDER */
} ds3231_control_t;

#define DS3231_STATUS                    0x0FU
typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t a1f              : 1;
  uint8_t a2f              : 1;
  uint8_t bsy              : 1;
  uint8_t en32khz          : 1;
  uint8_t unused_01        : 3;
  uint8_t osf              : 1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t osf              : 1;
  uint8_t unused_01        : 3;
  uint8_t en32khz          : 1;
  uint8_t bsy              : 1;
  uint8_t a2f              : 1;
  uint8_t a1f              : 1;
#endif /* DRV_BYTE_ORDER */
} ds3231_status_t;

#define DS3231_AGING_OFFSET              0x10U
#define DS3231_TEMP_MSB                  0x11U
#define DS3231_TEMP_LSB                  0x12U

typedef union
{
  ds3231_control_t control;
  ds3231_status_t  status;
  bitwise_t        bitwise;
  uint8_t          byte;
} ds3231_reg_t;

/**
  * @} DS3231_Register_Map
  */

#ifndef __weak
#define __weak __attribute__((weak))
#endif /* __weak */

/**
  * @defgroup DS3231_Low_Level Low-Level Communication Prototypes
  * @brief Weak read/write wrappers for target HAL integration.
  * @{
  */

int32_t ds3231_read_reg(const dev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len);
int32_t ds3231_write_reg(const dev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len);

/**
  * @} DS3231_Low_Level
  */

/**
  * @defgroup DS3231_Helper_Utilities Format Conversion Helpers
  * @brief BCD encoding/decoding and temperature calculation routines.
  * @{
  */

extern float_t ds3231_from_lsb_to_celsius(int16_t lsb);
extern uint8_t ds3231_bcd_to_dec(uint8_t val);
extern uint8_t ds3231_dec_to_bcd(uint8_t val);

/**
  * @} DS3231_Helper_Utilities
  */

/**
  * @defgroup DS3231_Core_API High-Level Application Interface
  * @brief Primary API structures and function declarations.
  * @{
  */

/**
* @brief Structure representing full calendar date and time.
*/
typedef struct
{
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day_of_week;
  uint8_t day;
  uint8_t month;
  uint16_t year;
} ds3231_time_t;

/**
  * @brief Alarm 1 trigger match condition modes.
  */
typedef enum
{
  DS3231_ALARM_1_EVERY_SEC          = 0x0F,
  DS3231_ALARM_1_MATCH_SEC          = 0x0E,
  DS3231_ALARM_1_MATCH_MIN_SEC      = 0x0C,
  DS3231_ALARM_1_MATCH_HRS_MIN_SEC  = 0x08,
  DS3231_ALARM_1_MATCH_DATE_HRS_MIN = 0x00,
} ds3231_alarm1_mode_t;

/**
  * @brief Configuration structure for Alarm 1 settings.
  */
typedef struct
{
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day_date;
  ds3231_alarm1_mode_t mode;
} ds3231_alarm1_t;

/**
  * @brief Unpacked status flags structure.
  */
typedef struct
{
  uint8_t osf              : 1;
  uint8_t busy             : 1;
  uint8_t alarm1_flag      : 1;
  uint8_t alarm2_flag      : 1;
} ds3231_stat_t;

/**
  * @brief  Sets current RTC time and date registers.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Pointer to structure containing time and date values.
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t ds3231_time_set(const dev_ctx_t *ctx, const ds3231_time_t *val);

/**
  * @brief  Reads current RTC time and date registers.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Pointer to target structure to store date and time.
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t ds3231_time_get(const dev_ctx_t *ctx, ds3231_time_t *val);

/**
  * @brief  Reads unscaled raw 10-bit temperature value from RTC registers.
  * @param  ctx   Pointer to driver device context.
  * @param  buff  Pointer to variable storing 16-bit raw temperature output.
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t ds3231_temperature_raw_get(const dev_ctx_t *ctx, int16_t *buff);

/**
  * @brief  Reads internal temperature sensor and returns value in degrees Celsius.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Pointer to float output variable storing converted temperature.
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t ds3231_temperature_get(const dev_ctx_t *ctx, float_t *val);

/**
  * @brief  Configures threshold parameters and mode for Alarm 1.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Pointer to structure with Alarm 1 parameters.
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t ds3231_alarm1_set(const dev_ctx_t *ctx, const ds3231_alarm1_t *val);

/**
  * @brief  Enables or disables hardware interrupt output generation for Alarm 1.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Enable state (PROPERTY_ENABLE or PROPERTY_DISABLE).
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t ds3231_alarm1_interrupt_en_set(const dev_ctx_t *ctx, uint8_t val);

/**
  * @brief  Retrieves current status flags from RTC device.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Pointer to target structure storing status flags.
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t ds3231_status_get(const dev_ctx_t *ctx, ds3231_stat_t *val);

/**
  * @brief  Clears active status flags in hardware registers based on input mask.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Structure specifying flags to reset.
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t ds3231_status_clear(const dev_ctx_t *ctx, ds3231_stat_t val);

/**
  * @} DS3231_Core_API
  */

/**
  * @} DS3231_Driver
  */

#ifdef __cplusplus
}
#endif

#endif /* DS3231_H */