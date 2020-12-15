#include "mp3_lcd.h"
#include "FreeRTOS.h"
#include "gpio.h"
#include "task.h"
typedef char songname_t[128];

gpio_s rs;
gpio_s rw;
gpio_s cs;
gpio_s D0;
gpio_s D1;
gpio_s D2;
gpio_s D3;
gpio_s D4;
gpio_s D5;
gpio_s D6;
gpio_s D7;

void configuration_lcd(void) {
  gpio__construct_with_function(GPIO__PORT_0, 8, GPIO__FUNCITON_0_IO_PIN);
  rs = gpio__construct_as_output(GPIO__PORT_0, 8);
  gpio__construct_with_function(GPIO__PORT_0, 9, GPIO__FUNCITON_0_IO_PIN);
  rw = gpio__construct_as_output(GPIO__PORT_0, 9);
  gpio__construct_with_function(GPIO__PORT_0, 26, GPIO__FUNCITON_0_IO_PIN);
  cs = gpio__construct_as_output(GPIO__PORT_0, 26);
  gpio__construct_with_function(GPIO__PORT_0, 25, GPIO__FUNCITON_0_IO_PIN);
  D0 = gpio__construct_as_output(GPIO__PORT_0, 25);
  gpio__construct_with_function(GPIO__PORT_1, 31, GPIO__FUNCITON_0_IO_PIN);
  D1 = gpio__construct_as_output(GPIO__PORT_1, 31);
  gpio__construct_with_function(GPIO__PORT_1, 30, GPIO__FUNCITON_0_IO_PIN);
  D2 = gpio__construct_as_output(GPIO__PORT_1, 30);
  gpio__construct_with_function(GPIO__PORT_1, 20, GPIO__FUNCITON_0_IO_PIN);
  D3 = gpio__construct_as_output(GPIO__PORT_1, 20);
  gpio__construct_with_function(GPIO__PORT_1, 23, GPIO__FUNCITON_0_IO_PIN);
  D4 = gpio__construct_as_output(GPIO__PORT_1, 23);
  gpio__construct_with_function(GPIO__PORT_1, 28, GPIO__FUNCITON_0_IO_PIN);
  D5 = gpio__construct_as_output(GPIO__PORT_1, 28);
  gpio__construct_with_function(GPIO__PORT_1, 29, GPIO__FUNCITON_0_IO_PIN);
  D6 = gpio__construct_as_output(GPIO__PORT_1, 29);
  gpio__construct_with_function(GPIO__PORT_2, 0, GPIO__FUNCITON_0_IO_PIN);
  D7 = gpio__construct_as_output(GPIO__PORT_2, 0);
  gpio__reset(cs);
  gpio__set(rs);
  gpio__reset(rw);
}

void lcd_init(void) {
  vTaskDelay(40);
  lcd_instruction_write(0x30); // function set

  vTaskDelay(1);
  lcd_instruction_write(0x30);

  vTaskDelay(1);
  lcd_instruction_write(0x38);
  vTaskDelay(1);

  // TURNING DISPLAY OFF
  lcd_instruction_write(0x08);
  vTaskDelay(1);

  // CLEARING LCD
  lcd_instruction_write(0x01);
  vTaskDelay(2);

  // ENTRY MODE ---- LEFT TO WRITE
  lcd_instruction_write(0x06);
  vTaskDelay(1);

  // TURNING DISPLAY ON
  lcd_instruction_write(0x0C);
  vTaskDelay(1);
}

bool is_lcd_busy(void) {
  bool is_busy = false;
  D7 = gpio__construct_as_input(GPIO__PORT_2, 0);
  gpio__reset(rs);
  gpio__set(rw);
  gpio__set(cs);
  vTaskDelay(1);
  is_busy = gpio__get(D7);
  gpio__reset(cs);
  vTaskDelay(1);
  return is_busy;
}

void lcd_write(uint8_t data) {
  if (data & (1U << 7)) {
    gpio__set(D7);
  } else {
    gpio__reset(D7);
  }
  if (data & (1U << 6)) {
    gpio__set(D6);
  } else {
    gpio__reset(D6);
  }
  if (data & (1U << 5)) {
    gpio__set(D5);
  } else {
    gpio__reset(D5);
  }
  if (data & (1U << 4)) {
    gpio__set(D4);
  } else {
    gpio__reset(D4);
  }
  if (data & (1U << 3)) {
    gpio__set(D3);
  } else {
    gpio__reset(D3);
  }
  if (data & (1U << 2)) {
    gpio__set(D2);
  } else {
    gpio__reset(D2);
  }
  if (data & (1U << 1)) {
    gpio__set(D1);
  } else {
    gpio__reset(D1);
  }
  if (data & (1U << 0)) {
    gpio__set(D0);
  } else {
    gpio__reset(D0);
  }

  gpio__set(cs);
  vTaskDelay(1);
  gpio__reset(cs);
  vTaskDelay(1);
}

void lcd_char_write(uint8_t data) {
  gpio__set(rs);
  gpio__reset(cs);
  lcd_write(data);
}

void lcd_write_song_name(songname_t data) {
  int i = 0;
  while (data[i] != 0) {
    while (is_lcd_busy()) {
      // wait until the LCD is ready
    }
    gpio__reset(rw);
    D7 = gpio__construct_as_output(GPIO__PORT_2, 0);
    lcd_char_write(data[i]);
    i++;
    vTaskDelay(1);
  }
}

void lcd_instruction_write(uint8_t inst_code) {
  gpio__reset(rs);
  gpio__reset(rw);
  gpio__reset(cs);
  lcd_write(inst_code);
}
