#include "ssp2_lab.h"
#include "gpio.h"
#include "lpc40xx.h"
#include <stdint.h>

void ssp2_lab__init(uint32_t max_clock_mhz) {
  LPC_SC->PCONP |= (1U << 20); // Power on SSP2
  LPC_SSP2->CR0 = (7U << 0);
  LPC_SSP2->CR1 = (1U << 1);
  // Code below is used to setup prescalar register
  int remainder;
  int quotient;
  quotient = 96 / max_clock_mhz;
  remainder = 96 % max_clock_mhz;
  if (remainder > 0) {
    quotient += 1;
  }
  remainder = quotient % 2;
  if (remainder > 0) {
    quotient += 1;
  }
  LPC_SSP2->CPSR = quotient;
}

uint8_t ssp2_lab__exchange_byte(uint8_t data_out) {
  LPC_SSP2->DR = data_out;
  while (LPC_SSP2->SR & (1U << 4)) {
  }
  return LPC_SSP2->DR;
}