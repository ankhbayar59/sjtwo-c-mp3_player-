#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef char songname_t[128];

void configuration_lcd(void);

void lcd_init(void);

bool is_lcd_busy(void);

void lcd_write(uint8_t data);

void lcd_char_write(uint8_t data);

void lcd_write_song_name(songname_t data);

void lcd_instruction_write(uint8_t inst_code);
