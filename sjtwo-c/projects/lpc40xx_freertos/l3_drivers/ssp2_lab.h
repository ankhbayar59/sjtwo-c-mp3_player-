#pragma once

#include <stdint.h>

void ssp2_lab__init(uint32_t max_clock_mhz);

uint8_t ssp2_lab__exchange_byte(uint8_t data_out);
