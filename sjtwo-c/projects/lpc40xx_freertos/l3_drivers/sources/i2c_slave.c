#include "i2c_slave.h"
#include "FreeRTOS.h"
#include "gpio.h"
#include "i2c.h"
#include "semphr.h"
#include "task.h"
#include <stdio.h>

#include "common_macros.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

void i2c_slave_init(uint8_t slave_address_to_assign) {
  LPC_IOCON->P0_0 |= (1U << 10);          // Setting the SDA1 as open drain output
  LPC_IOCON->P0_1 |= (1U << 10);          // Setting the SCL1 as open drain output
  gpio__construct_with_function(0, 0, 3); // port is programmed to function as SDA1
  gpio__construct_with_function(0, 1, 3); // port is programmed to function as SCL1
  const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  i2c__initialize(I2C__1, i2c_speed_hz, clock__get_peripheral_clock_hz());
  LPC_I2C1->ADR0 |= ((slave_address_to_assign) << 0);
  LPC_I2C1->CONSET = 0x44;
  for (unsigned slave_address = 2; slave_address <= 254; slave_address += 2) {
    if (i2c__detect(I2C__2, slave_address)) {
      printf("I2C slave detected at address: 0x%02X\n", slave_address);
    }
  }
}
