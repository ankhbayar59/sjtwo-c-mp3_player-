#include "FreeRTOS.h"
#include "acceleration.h"
#include "adc.h"
#include "board_io.h"
#include "common_macros.h"
#include "event_groups.h"
#include "ff.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "i2c.h"
#include "i2c_slave.h"
#include "i2c_slave_functions.h"
#include "lpc_peripherals.h"
#include "mp3_decoder.h"
#include "mp3_lcd.h"
#include "periodic_scheduler.h"
#include "pwm1.h"
#include "queue.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "song_list.h"
#include "ssp2_lab.h"
#include "task.h"
#include "uart_lab.h"
#include <stdio.h>
#include <string.h>

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);
uint8_t slave_buffer[256];

//               For the I2C LAb                  //

bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  bool no_error = false;
  if (256 < memory_index) {
    printf("Out of Bound\n");
  } else {
    *memory = slave_buffer[memory_index];
    no_error = true;
  }
  return no_error;
}

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  bool no_error = false;
  if (256 < memory_index) {
    printf("Out of Bound\n");
  } else {
    slave_buffer[memory_index] = memory_value;
    no_error = true;
  }
  return no_error;
}

// GLOBAL VARIABLES//

typedef char songname_t[128];
typedef char song_data_t[512];

// USED TO STORE METADATA AND GRAB IT//
songname_t title_list[32];
songname_t artist_list[32];
songname_t title = {};
songname_t artist = {};

// FLAGS AND VARIABLES USED BETWEEN INTERRUPT AND OTHER TASKS //

// USED FOR VOLUME CONTROL //
volatile bool volume_flag = false;
volatile int volume_value = 100;

// USED FOR TREBLE CONTROL //
volatile bool treble_flag = false;
volatile int treble_value = 0;
volatile int t_amp = 0;
volatile int t_freq = 0;

// USED FOR BASS CONTROL //
volatile bool bass_flag = false;
volatile int bass_value = 0;
volatile int b_amp = 0;
volatile int b_freq = 0;

// USED FOR SELECT BUTTON //
volatile bool initial_song = true;
volatile bool change_song = false;
volatile bool initial_song_request = false;
volatile bool request_current_song_info = false;

// PLAYLIST CONTROL //
volatile bool move_left_flag = false;
volatile bool move_right_flag = false;
volatile size_t current_song_index = 0;
volatile size_t num_of_songs;
volatile size_t song_list_index = 0;

// USED TO CHOOSE FROM VOLUME, BASS, TREBLE //
int mode = 0;
volatile bool changed_mode = false;

// PAUSE AND PLAY //
volatile bool pause = false;
volatile bool finished_playing = false;

// SEMAPHORE AND QUEUE HANDLE //
QueueHandle_t song_name_queue;
static QueueHandle_t mp3_song_data_queue;
static SemaphoreHandle_t display_song_list_name_signal;

// Interrupt Callback functions //

// SELECT BUTTON //
void pin2_isr(void) {
  initial_song_request = true;
  request_current_song_info = true;
  if (initial_song) {
    initial_song = false;
  } else {
    change_song = true;
  }
  current_song_index = song_list_index;
  xQueueSendFromISR(song_name_queue, song_list__get_name_for_item(song_list_index), NULL);
  xSemaphoreGiveFromISR(display_song_list_name_signal, NULL);
}

// MOVE LEFT BUTTON //
void pin4_isr(void) {
  move_left_flag = true;
  xSemaphoreGiveFromISR(display_song_list_name_signal, NULL);
}

// MOVE RIGHT BUTTON //
void pin5_isr(void) {
  move_right_flag = true;
  xSemaphoreGiveFromISR(display_song_list_name_signal, NULL);
}

// PAUSE AND PLAY BUTTON //
void pin6_isr(void) {
  if (pause) {
    pause = false;
  } else {
    pause = true;
  }
  request_current_song_info = true;
  xSemaphoreGiveFromISR(display_song_list_name_signal, NULL);
}

// VOLUME DOWN, TREBLE DOWN, BASS DOWN//
void pin7_isr(void) {
  if (0 == mode) {
    if (0 == volume_value) {
      volume_flag = true;
    } else {
      volume_flag = true;
      volume_value = volume_value - 25;
    }
  } else if (1 == mode) {
    if (0 == treble_value) {
      treble_flag = true;
    } else {
      treble_flag = true;
      treble_value = treble_value - 25;
    }
  } else {
    if (0 == bass_value) {
      bass_flag = true;
    } else {
      bass_flag = true;
      bass_value = bass_value - 10;
    }
  }
}

// VOLUME UP, TREBLE UP, BASS UP //
void pin8_isr(void) {
  if (0 == mode) {
    if (100 == volume_value) {
      volume_flag = true;
    } else {
      volume_flag = true;
      volume_value = volume_value + 25;
    }
  } else if (1 == mode) {
    if (100 == treble_value) {
      treble_flag = true;
    } else {
      treble_flag = true;
      treble_value = treble_value + 25;
    }
  } else {
    if (100 == bass_value) {
      bass_flag = true;
    } else {
      bass_flag = true;
      bass_value = bass_value + 10;
    }
  }
}

void pin9_isr(void) {
  changed_mode = true;
  if (2 == mode) {
    mode = 0;
  } else {
    mode++;
  }
  xSemaphoreGiveFromISR(display_song_list_name_signal, NULL);
}

void configure_your_gpio_interrupt(void) {

  gpio__construct_with_function(GPIO__PORT_2, 2, GPIO__FUNCITON_0_IO_PIN);
  LPC_GPIO2->DIR &= ~(1U << 2);
  gpio__construct_with_function(GPIO__PORT_2, 4, GPIO__FUNCITON_0_IO_PIN);
  LPC_GPIO2->DIR &= ~(1U << 4);
  gpio__construct_with_function(GPIO__PORT_2, 5, GPIO__FUNCITON_0_IO_PIN);
  LPC_GPIO2->DIR &= ~(1U << 5);
  gpio__construct_with_function(GPIO__PORT_2, 6, GPIO__FUNCITON_0_IO_PIN);
  LPC_GPIO2->DIR &= ~(1U << 6);
  gpio__construct_with_function(GPIO__PORT_2, 7, GPIO__FUNCITON_0_IO_PIN);
  LPC_GPIO2->DIR &= ~(1U << 7);
  gpio__construct_with_function(GPIO__PORT_2, 8, GPIO__FUNCITON_0_IO_PIN);
  LPC_GPIO2->DIR &= ~(1U << 8);
  gpio__construct_with_function(GPIO__PORT_2, 9, GPIO__FUNCITON_0_IO_PIN);
  LPC_GPIO2->DIR &= ~(1U << 9);

  gpio_interrupt_e pin2;
  gpio_interrupt_e pin4;
  gpio_interrupt_e pin5;
  gpio_interrupt_e pin6;
  gpio_interrupt_e pin7;
  gpio_interrupt_e pin8;
  gpio_interrupt_e pin9;

  pin2 = GPIO_INTR__FALLING_EDGE;
  pin4 = GPIO_INTR__FALLING_EDGE;
  pin5 = GPIO_INTR__FALLING_EDGE;
  pin6 = GPIO_INTR__FALLING_EDGE;
  pin7 = GPIO_INTR__FALLING_EDGE;
  pin8 = GPIO_INTR__FALLING_EDGE;
  pin9 = GPIO_INTR__FALLING_EDGE;

  gpio2__attach_interrupt(2, pin2, pin2_isr);
  gpio2__attach_interrupt(4, pin4, pin4_isr);
  gpio2__attach_interrupt(5, pin5, pin5_isr);
  gpio2__attach_interrupt(6, pin6, pin6_isr);
  gpio2__attach_interrupt(7, pin7, pin7_isr);
  gpio2__attach_interrupt(8, pin8, pin8_isr);
  gpio2__attach_interrupt(9, pin9, pin9_isr);

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio2__interrupt_dispatcher, "interrupt_gpio2");
}

static void get_meta_data(const char *filename, int index) {
  UINT br;
  FIL meta_data_file;
  FRESULT meta_data_result = f_open(&meta_data_file, filename, FA_READ);
  meta_data_result = f_lseek(&meta_data_file, f_size(&meta_data_file) - 128);
  songname_t meta_data = {};
  if (FR_OK == f_read(&meta_data_file, meta_data, sizeof(meta_data), &br)) {
    if (0 == br) {
      printf("error: Meta_data\n");
    }
  }
  memset(&title[0], 0, sizeof(title));
  memset(&artist[0], 0, sizeof(artist));
  int j = 0;
  for (int i = 3; i < 23; i++) {
    title[j] = meta_data[i];
    j++;
  }
  j = 0;
  for (int i = 33; i < 53; i++) {
    artist[j] = meta_data[i];
    j++;
  }
  strncpy(title_list[index], title, sizeof(songname_t) - 1);
  strncpy(artist_list[index], artist, sizeof(songname_t) - 1);
  f_close(&meta_data_file);
}

static void populate_meta_data(void) {
  for (int i = 0; i < num_of_songs; i++) {
    get_meta_data(song_list__get_name_for_item(i), i);
  }
}

static void read_file(const char *filename) {
  UINT br;
  FIL file;
  printf("Request received to play/read: '%s'\n", filename);
  FRESULT result = f_open(&file, filename, FA_READ);
  if (finished_playing) {
    finished_playing = false;
    request_current_song_info = true;
    xSemaphoreGive(display_song_list_name_signal);
  }
  if (FR_OK == result) {
    song_data_t buffer = {};
    while (1) {
      if (change_song) {
        change_song = false;
        break;
      }
      if (FR_OK == f_read(&file, buffer, sizeof(buffer), &br)) {
        if (0 == br) {
          break;
        } else {
          xQueueSend(mp3_song_data_queue, buffer, portMAX_DELAY);
          memset(&buffer[0], 0, sizeof(buffer));
        }
      }
    }
  } else {
    printf("ERROR: Coulnd't find the following song: '%s'\n", filename);
  }

  f_close(&file);
}

static void mp3_file_reader_task(void *p) {
  songname_t songname = {};
  while (1) {
    if (xQueueReceive(song_name_queue, songname, 5000)) {
      read_file(songname);
    } else {
      if (!(initial_song)) {
        finished_playing = true;
        if (song_list_index == (num_of_songs - 1)) {
          song_list_index = 0;
        } else {
          song_list_index = song_list_index + 1;
        }
        current_song_index = song_list_index;
        xQueueSend(song_name_queue, song_list__get_name_for_item(song_list_index), 0);
      } else {
        printf("NO REQUEST\n");
      }
    }
  }
}

static void mp3_decoder_send_block(song_data_t data) {
  for (size_t index = 0; index < sizeof(song_data_t); index++) {
    if (!(check_dreq())) {
      vTaskDelay(10);
    }
    send_byte(data[index]);
  }
}

static void handle_volume(void) {
  volume_flag = false;
  switch (volume_value) {
  case 100:
    write_register(0x0B, 0x00, 0x00);
    break;
  case 75:
    write_register(0x0B, 0x0A, 0x0A);
    break;
  case 50:
    write_register(0x0B, 0x14, 0x14);
    break;
  case 25:
    write_register(0x0B, 0x1E, 0x1E);
    break;
  case 0:
    write_register(0x0B, 0x28, 0x28);
    break;
  }
  changed_mode = true;
  xSemaphoreGiveFromISR(display_song_list_name_signal, NULL);
}

static void handle_treble(void) {
  treble_flag = false;
  switch (treble_value) {
  case 100:
    t_amp = 3;
    t_freq = 14;
    write_register(0x02, 0x3E, 0x00);
    break;
  case 75:
    t_amp = 3;
    t_freq = 13;
    write_register(0x02, 0x3D, 0x00);
    break;
  case 50:
    t_amp = 3;
    t_freq = 12;
    write_register(0x02, 0x3C, 0x00);
    break;
  case 25:
    t_amp = 3;
    t_freq = 11;
    write_register(0x02, 0x3B, 0x00);
    break;
  case 0:
    t_amp = 0;
    write_register(0x02, 0x00, 0x00);
    break;
  }
  changed_mode = true;
  xSemaphoreGiveFromISR(display_song_list_name_signal, NULL);
}

static void handle_bass(void) {
  bass_flag = false;
  switch (bass_value) {
  case 100:
    b_amp = 15;
    b_freq = 120;
    write_register(0x02, 0x00, 0xFC);
    break;
  case 90:
    b_amp = 9;
    b_freq = 110;
    write_register(0x02, 0x00, 0xFB);
    break;
  case 80:
    b_amp = 8;
    b_freq = 100;
    write_register(0x02, 0x00, 0xFA);
    break;
  case 70:
    b_amp = 7;
    b_freq = 90;
    write_register(0x02, 0x00, 0xF9);
    break;
  case 60:
    b_amp = 6;
    b_freq = 80;
    write_register(0x02, 0x00, 0xF8);
    break;
  case 50:
    b_amp = 5;
    b_freq = 70;
    write_register(0x02, 0x00, 0xF7);
    break;
  case 40:
    b_amp = 4;
    b_freq = 60;
    write_register(0x02, 0x00, 0xF6);
    break;
  case 30:
    b_amp = 3;
    b_freq = 50;
    write_register(0x02, 0x00, 0xF5);
    break;
  case 20:
    b_amp = 2;
    b_freq = 40;
    write_register(0x02, 0x00, 0xF4);
    break;
  case 10:
    b_amp = 1;
    b_freq = 30;
    write_register(0x02, 0x00, 0xF3);
    break;
  case 0:
    b_amp = 0;
    b_freq = 20;
    write_register(0x02, 0x00, 0x02);
    break;
  }
  changed_mode = true;
  xSemaphoreGiveFromISR(display_song_list_name_signal, NULL);
}

static void mp3_data_player_task(void *p) {
  song_data_t songdata;
  while (1) {
    if (xQueueReceive(mp3_song_data_queue, &songdata[0], portMAX_DELAY)) {
      if (volume_flag) {
        handle_volume();
      } else if (treble_flag) {
        handle_treble();
      } else if (bass_flag) {
        handle_bass();
      }
      while (pause) {
        vTaskDelay(100);
      }
      mp3_decoder_send_block(songdata);
    }
  }
}

// Display playlist with current song //

static void display_playlist(char playlist_song[20], char current_song[20]) {
  lcd_instruction_write(0x01);
  lcd_write_song_name(playlist_song);
  lcd_instruction_write(0xC0);
  lcd_write_song_name(song_list__get_name_for_item(song_list_index));
  lcd_instruction_write(0x94);
  lcd_write_song_name(current_song);
  lcd_instruction_write(0xD4);
  lcd_write_song_name(song_list__get_name_for_item(current_song_index));
}

// Display only playlist since no song was requested // 

static void display_only_playlist(char playlist_song[20]) {
  lcd_instruction_write(0x01);
  lcd_write_song_name(playlist_song);
  lcd_instruction_write(0xC0);
  lcd_write_song_name((song_list__get_name_for_item(song_list_index)));
}

// Handling previous button //

static void configure_move_left() {
  move_left_flag = false;
  if (song_list_index == 0) {
    song_list_index = num_of_songs - 1;
  } else {
    song_list_index = song_list_index - 1;
  }
}

// Handling next button //

static void configure_move_right() {
  move_right_flag = false;
  if (song_list_index == (num_of_songs - 1)) {
    song_list_index = 0;
  } else {
    song_list_index = song_list_index + 1;
  }
}

// This is called to display the metadata of the mp3 //
static void display_meta_data() {
  request_current_song_info = false;
  lcd_instruction_write(0x01);
  lcd_write_song_name("Title:");
  lcd_instruction_write(0xC0);
  lcd_write_song_name(title_list[current_song_index]);
  lcd_instruction_write(0x94);
  lcd_write_song_name("Artist");
  lcd_instruction_write(0xD4);
  lcd_write_song_name(artist_list[current_song_index]);
}

// Used for displaying the which mode and current value of the mode //
// Also the same function is called when + or - switches is pressed // 

static void display_mode() {
  changed_mode = false;
  char volume_buffer[20];
  char treble_buffer[20];
  char bass_buffer[20];
  sprintf(volume_buffer, "%i", volume_value);
  sprintf(treble_buffer, "amp:%idB, fre:%iHz", t_amp, t_freq);
  sprintf(bass_buffer, "amp:%idB, fre:%ikHz", b_amp, b_freq);
  lcd_instruction_write(0x01);
  if (0 == mode) {
    lcd_write_song_name("Control Mode");
    lcd_instruction_write(0xC0);
    lcd_write_song_name("Volume");
    lcd_instruction_write(0x94);
    lcd_write_song_name("Current Value");
    lcd_instruction_write(0xD4);
    lcd_write_song_name(volume_buffer);
  } else if (1 == mode) {
    lcd_write_song_name("Control Mode");
    lcd_instruction_write(0xC0);
    lcd_write_song_name("Treble");
    lcd_instruction_write(0x94);
    lcd_write_song_name("Current Value");
    lcd_instruction_write(0xD4);
    lcd_write_song_name(treble_buffer);
  } else if (2 == mode) {
    lcd_write_song_name("Control Mode");
    lcd_instruction_write(0xC0);
    lcd_write_song_name("Bass");
    lcd_instruction_write(0x94);
    lcd_write_song_name("Current Value");
    lcd_instruction_write(0xD4);
    lcd_write_song_name(bass_buffer);
  }
}
static void display_task(void *p) {
  lcd_init();
  char song_list_buffer[20];
  char current_list_buffer[20];
  sprintf(song_list_buffer, "Song List: %i", song_list_index);
  display_only_playlist(&song_list_buffer[0]);
  while (1) {
    if (xSemaphoreTake(display_song_list_name_signal, portMAX_DELAY)) {
      memset(&song_list_buffer[0], 0, sizeof(song_list_buffer));
      memset(&current_list_buffer[0], 0, sizeof(current_list_buffer));
      if (move_left_flag) {
        configure_move_left();
        sprintf(song_list_buffer, "Song List: %i", song_list_index);
        sprintf(current_list_buffer, "Current Song: %i", current_song_index);
        if (initial_song_request) {
          display_playlist(song_list_buffer, current_list_buffer);
        } else {
          display_only_playlist(song_list_buffer);
        }
      } else if (move_right_flag) {
        configure_move_right();
        sprintf(song_list_buffer, "Song List: %i", song_list_index);
        sprintf(current_list_buffer, "Current Song: %i", current_song_index);
        if (initial_song_request) {
          display_playlist(song_list_buffer, &current_list_buffer);
        } else {
          display_only_playlist(song_list_buffer);
        }
      } else if (request_current_song_info) {
        display_meta_data();
      } else if (changed_mode) {
        display_mode();
      } else {
        sprintf(song_list_buffer, "Song List: %i", song_list_index);
        display_only_playlist(song_list_buffer);
      }
    }
  }
}

int main(void) {
  // create_blinky_tasks();
  // create_uart_task();
  puts("Starting RTOS");
  sj2_cli__init();
  setvbuf(stdout, 0, _IONBF, 0);
  configure_your_gpio_interrupt();
  NVIC_EnableIRQ(GPIO_IRQn);
  configure_mp3_decoder();
  init_mp3_decoder();
  configuration_lcd();
  song_list__populate();
  num_of_songs = song_list__get_item_count();
  populate_meta_data();
  song_name_queue = xQueueCreate(1, sizeof(songname_t));
  mp3_song_data_queue = xQueueCreate(3, sizeof(song_data_t));
  display_song_list_name_signal = xSemaphoreCreateBinary();
  xTaskCreate(mp3_file_reader_task, "reader", 4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(display_task, "menu", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_data_player_task, "player", 4096 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  vTaskStartScheduler();
  return 0;
}

static void create_blinky_tasks(void) {
  /**
   * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
   * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
   */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes, !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}
