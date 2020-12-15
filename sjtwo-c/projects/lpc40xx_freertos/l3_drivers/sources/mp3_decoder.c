#include "mp3_decoder.h"
#include "ssp2_lab.h"
#include <stdio.h>
gpio_s dreq;
gpio_s rst;
gpio_s xcs;
gpio_s xdcs;

void configure_mp3_decoder(void) {
  gpio__construct_with_function(GPIO__PORT_1, 4, GPIO__FUNCTION_4);
  gpio__construct_with_function(GPIO__PORT_1, 0, GPIO__FUNCTION_4);
  gpio__construct_with_function(GPIO__PORT_1, 1, GPIO__FUNCTION_4);
  gpio__construct_with_function(GPIO__PORT_4, 29, GPIO__FUNCITON_0_IO_PIN);
  dreq = gpio__construct_as_input(GPIO__PORT_4, 29);
  gpio__construct_with_function(GPIO__PORT_4, 28, GPIO__FUNCITON_0_IO_PIN);
  rst = gpio__construct_as_output(GPIO__PORT_4, 28);
  gpio__construct_with_function(GPIO__PORT_0, 6, GPIO__FUNCITON_0_IO_PIN);
  xcs = gpio__construct_as_output(GPIO__PORT_0, 6);
  gpio__construct_with_function(GPIO__PORT_0, 7, GPIO__FUNCITON_0_IO_PIN);
  xdcs = gpio__construct_as_output(GPIO__PORT_0, 7);
  const uint32_t spi_clock_mhz = 1;
  ssp2_lab__init(spi_clock_mhz);
}

void init_mp3_decoder(void) {
  gpio__set(rst);
  gpio__set(xcs);
  gpio__set(xdcs);

  // volume at max
  write_register(0x0B, 0x00, 0x00);
  write_register(0x03, 0x60, 0x00);
  const uint32_t spi_clock_mhz = 6;
  ssp2_lab__init(spi_clock_mhz);
}

void read_register(uint8_t register_ad) {
  uint8_t ms_byte;
  uint8_t ls_byte;
  while (!(gpio__get(dreq))) {
    // wait untill dreq goes high
  }
  gpio__reset(xcs);
  ssp2_lab__exchange_byte(0x03);
  ssp2_lab__exchange_byte(register_ad);
  ms_byte = ssp2_lab__exchange_byte(0x00);
  ls_byte = ssp2_lab__exchange_byte(0x00);
  gpio__set(xcs);
  while (!(gpio__get(dreq))) {
    // wait untill dreq goes high
  }
  printf("ms: %u\n", ms_byte);
  printf("ls: %u\n", ls_byte);
}

void write_register(uint8_t register_ad, uint8_t ms_byte, uint8_t ls_byte) {
  while (!(gpio__get(dreq))) {
    // wait untill dreq goes high
  }
  gpio__reset(xcs);
  ssp2_lab__exchange_byte(0x02);
  ssp2_lab__exchange_byte(register_ad);
  ssp2_lab__exchange_byte(ms_byte);
  ssp2_lab__exchange_byte(ls_byte);
  gpio__set(xcs);

  while (!(gpio__get(dreq))) {
    // wait untill dreq goes high
  }
}

bool check_dreq(void) {
  bool value = false;
  if (gpio__get(dreq)) {
    value = true;
  }
  return value;
}

void send_byte(char data) {
  gpio__reset(xdcs);
  ssp2_lab__exchange_byte(data);
  gpio__set(xdcs);
}