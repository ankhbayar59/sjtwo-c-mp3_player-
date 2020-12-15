#pragma once
#include "gpio.h"

// SETTING UP MP3_DECODER //
void configure_mp3_decoder(void);

// INIT MP3_DECODER //
void init_mp3_decoder(void);

// READ FROM MP3_DECODER REGISTER //
void read_register(uint8_t register_ad);

// WRTIE TO MP3_DECODER REGISTER //
void write_register(uint8_t register_ad, uint8_t ms_byte, uint8_t ls_byte);

bool check_dreq(void);

void send_byte(char data);