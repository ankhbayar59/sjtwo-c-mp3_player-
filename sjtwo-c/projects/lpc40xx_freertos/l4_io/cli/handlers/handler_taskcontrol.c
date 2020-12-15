#include <string.h>

#include "cli_handlers.h"

#include "FreeRTOS.h"
#include "task.h"

app_cli_status_e cli__task_control(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                                   app_cli__print_string_function cli_output) {
  sl_string_t s = user_input_minus_command_name;

  // If the user types 'taskcontrol suspend led0' then we need to suspend a task with the name of 'led0'
  // In this case, the user_input_minus_command_name will be set to 'suspend led0' with the command-name removed
  if (sl_string__begins_with_ignore_case(s, "suspend")) {
    // TODO: Use sl_string API to remove the first word, such that variable 's' will equal to 'led0'
    // TODO: Or you can do this: char name[16]; sl_string__scanf("%*s %16s", name);
    char *tmp = strchr(s, ' ');
    if (tmp != NULL)
      s = tmp + 1;
    // Now try to query the tasks with the name 'led0'
    TaskHandle_t task_handle = xTaskGetHandle(s);
    if (NULL == task_handle) {
      // note: we cannot use 'sl_string__printf("Failed to find %s", s);' because that would print existing string onto
      // itself
      sl_string__insert_at(s, 0, "Could not find a task with name:");
      cli_output(NULL, s);
    } else {
      // TODO: Use vTaskSuspend()
      vTaskSuspend(task_handle);
    }

  } else if (sl_string__begins_with_ignore_case(s, "resume")) {
    // TODO
    char *tmp = strchr(s, ' ');
    if (tmp != NULL)
      s = tmp + 1;
    TaskHandle_t task_handle = xTaskGetHandle(s);
    if (NULL == task_handle) {
      sl_string__insert_at(s, 0, "Could not find a task with name:");
      cli_output(NULL, s);
    } else {
      vTaskResume(task_handle);
    }
  } else {
    cli_output(NULL, "Did you mean to say suspend or resume?\n");
  }

  return APP_CLI_STATUS__SUCCESS;
}