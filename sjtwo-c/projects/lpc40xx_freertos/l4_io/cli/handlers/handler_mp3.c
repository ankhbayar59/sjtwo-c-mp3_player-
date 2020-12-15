#include "cli_handlers.h"
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
extern QueueHandle_t song_name_queue;
typedef char songname_t[32];

app_cli_status_e cli__mp3(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                          app_cli__print_string_function cli_output) {
  sl_string_t s = user_input_minus_command_name;
  if (sl_string__begins_with_ignore_case(s, "play")) {
    char *temp = strchr(s, ' ');
    if (temp != NULL) {
      s = temp + 1;
    }
    if (strlen(s) <= 32) {
      songname_t songname = {};
      strncpy(songname, s, sizeof(songname) - 1);
      if (xQueueSend(song_name_queue, &songname, 1000)) {
        cli_output(NULL, "SUCCESS: Sent song name to the Queue\n");
      } else {
        cli_output(NULL, "ERROR: Failed to queue song name\n");
      }
    } else {
      sl_string__insert_at(s, 0, "Given song name is greater than 32 bytes:");
      cli_output(NULL, s);
    }
  }
  return APP_CLI_STATUS__SUCCESS;
}