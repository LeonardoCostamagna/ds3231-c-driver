# DS3231 Driver

## Overview

The **DS3231** is a high-precision I2C real-time clock (RTC) featuring an integrated crystal oscillator and temperature compensation (TCXO). Its design ensures an accuracy of ±2ppm across a temperature range of -40°C to +85°C. The device integrates a digital temperature sensor, programmable alarms, and a square-wave output while maintaining ultra-low power consumption. This makes it ideal for embedded system applications that require long-term accurate timekeeping, even on backup batteries.

### Key Features

* **High Portability:** Complete communication layer abstraction via an extensible access interface. Base functions are marked with the `__weak` qualifier, allowing for both dynamic callbacks and static redefinition of the transport channel.
* **Robust Design Patterns:** Utilizes *Hardware Proxy* and *Opaque Handles* patterns to minimize coupling and guarantee predictable memory usage.
* **Quality & Verification:** Includes a hardware-decoupled unit testing suite executable via Ceedling/Unity, along with Doxygen-compatible inline documentation.

---

## Project Structure

```text
ds3231/
├── docs/                 # Doxygen documentation and configuration file
├── ds3231.h              # Public interface and driver declarations
├── ds3231.c              # Driver logic implementation
├── test/                 # Unit testing environment
│   ├── support/          # Auxiliary headers and mocks
│   ├── project.yml       # Ceedling configuration
│   └── test_ds3231.c     # Unit test suite
└── README.md             # Main project documentation

```

---

## Integration and Getting Started

### Integration Schemes

The driver offers two independent mechanisms to connect with the target platform's HAL:

#### Option A: Via Function Pointers / Callbacks (Dynamic)

Transport functions (read/write) are defined and assigned to the members of the `dev_ctx_t` context structure at runtime. **Does not require** overriding the `__weak` functions.

#### Option B: Via `__weak` Function Overriding (Static)

Functions declared with the `__weak` attribute (`<driver>_read_reg` and `<driver>_write_reg`) are re-implemented directly in the application code. Under this scheme, setting up the `read_reg` and `write_reg` function pointers inside the `dev_ctx_t` context is not required.

---

### Integration Steps

1. Copy the driver files (`.h` and `.c`) into your project's directory tree and include them in your build system (Makefile, CMake, IDE).
2. Select the integration option that best fits your architecture (Option A or Option B).
3. Instantiate the `dev_ctx_t` context structure, associating the generic pointer to the microcontroller's I2C/SPI bus handler.

---

### Code Examples

#### Example 1: Integration via Callbacks (Option A)

```c
#include <stdio.h>
#include "ds3231.h"

/* 1. Definition of platform-specific wrapper functions */
static int32_t platform_bus_write(void *handle, uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    /* Integration with native platform HAL:
     * Return 0 on success, < 0 on error.
     */
    return 0; 
}

static int32_t platform_bus_read(void *handle, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    /* Integration with native platform HAL:
     * Return 0 on success, < 0 on error.
     */
    return 0; 
}

int main(void)
{
    /* Pointer/handle to MCU I2C/SPI peripheral */
    void *mcu_bus_handle = (void *)0x40005400; 

    /* Context declaration and initialization */
    dev_ctx_t dev_ctx;
    dev_ctx.write_reg = platform_bus_write;
    dev_ctx.read_reg  = platform_bus_read;
    dev_ctx.mdelay    = NULL;             /* Optional: delay in ms if required */
    dev_ctx.handle    = mcu_bus_handle;
    dev_ctx.priv_data = NULL;

    /* Device initialization/verification */
    if (ds3231_init(&dev_ctx) == 0) {
        printf("Device initialized successfully.\n");
    }

    return 0;
}

```

#### Example 2: Integration via `__weak` Redefinition (Option B)

```c
#include "ds3231.h"

/* Direct override of the weak read function */
int32_t ds3231_read_reg(const dev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (ctx == NULL || ctx->handle == NULL) {
        return -1;
    }

    /* Direct call to platform HAL using ctx->handle */
    /* Hypothetical example using STM32 HAL:
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)ctx->handle;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        hi2c, 
        DEVICE_I2C_ADD, 
        reg, 
        I2C_MEMADD_SIZE_8BIT, 
        data, 
        len, 
        100
    );
    return (status == HAL_OK) ? 0 : -1;
    */

    return 0;
}

/* Direct override of the weak write function */
int32_t ds3231_write_reg(const dev_ctx_t *ctx, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (ctx == NULL || ctx->handle == NULL) {
        return -1;
    }

    /* Direct call to platform HAL using ctx->handle */
    /* Hypothetical example using STM32 HAL:
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)ctx->handle;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        hi2c, 
        DEVICE_I2C_ADD, 
        reg, 
        I2C_MEMADD_SIZE_8BIT, 
        (uint8_t *)data, 
        len, 
        100
    );
    return (status == HAL_OK) ? 0 : -1;
    */

    return 0;
}

int main(void)
{
    void *mcu_bus_handle = (void *)0x40005400; 

    /* Simplified context initialization (does not require defining callbacks) */
    dev_ctx_t dev_ctx = {
        .handle = mcu_bus_handle
    };

    if (ds3231_init(&dev_ctx) == 0) {
        /* Device operations */
    }

    return 0;
}

```

---

## Running Unit Tests

The development environment uses **Ceedling** (along with **Unity** and **CMock**) for isolated execution of unit tests.

### Prerequisites

* **Ruby** (for executing the Ceedling engine)
* **Ceedling** (unit testing framework for C)
* **gcovr** (optional, for coverage reporting)

### Execution Commands

1. Navigate to the test directory:

```bash
cd test

```

2. Run the full test suite:

```bash
ceedling test:all

```

3. Generate code coverage report:

```bash
ceedling gcov:all

```

The resulting HTML report will be available at:
`test/build/artifacts/gcov/gcovr/index.html`

---

## Generating Documentation

The public API is documented using **Doxygen**-compatible comment blocks.

### Option 1: Via Command Line

1. Navigate to the `docs/` directory:

```bash
cd docs

```

2. Build the documentation using the `doxyfile` configuration file:

```bash
doxygen doxyfile

```

3. Open the generated file in your web browser:

```bash
xdg-open html/index.html

```

### Option 2: Via Graphical User Interface

1. Launch Doxywizard from the terminal:

```bash
doxywizard

```

2. Select **File > Open** and open the `doxyfile` inside the `docs/` directory.
3. **Run** tab -> Click **Run doxygen**.
4. Open `html/index.html` in your browser.

---

## License

This project is distributed under the **MIT** License.