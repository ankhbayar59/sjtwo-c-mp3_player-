#include "uart_lab.h"
#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Private queue handle of our uart_lab.c
static QueueHandle_t uart_rx_queue;

bool uart_number_is2 = true;
static void receive_interrupt(void) {
  const uint8_t rdr_empty = (1U << 0);
  char received_data;
  if (uart_number_is2) {
    if (!(LPC_UART2->IIR & (1U << 0))) {
      if ((LPC_UART2->IIR & (1U << 2)) && !(LPC_UART2->IIR & (1U << 1)) && !(LPC_UART2->IIR & (1U << 3))) {
        while (!(LPC_UART2->LSR & rdr_empty)) {
          // do nothing
        }
        received_data = LPC_UART2->RBR;
      }
    }
  } else {
    if (!(LPC_UART3->IIR & (1U << 0))) {
      if ((LPC_UART3->IIR & (1U << 2)) && !(LPC_UART3->IIR & (1U << 1)) && !(LPC_UART3->IIR & (1U << 3))) {
        while (!(LPC_UART3->LSR & rdr_empty)) {
          // do nothing
        }
        received_data = LPC_UART3->RBR;
      }
    }
  }
  xQueueSendFromISR(uart_rx_queue, &received_data, NULL);
}

void uart__enable_receive_interrupt(uart_number_e uart_number) {
  if (uart_number == UART_2) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, receive_interrupt, "receive interrupt");
    LPC_UART2->IER |= (1U << 0);
  } else if (uart_number == UART_3) {
    uart_number_is2 = false;
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, receive_interrupt, "receive interrupt");
    LPC_UART3->IER |= (1U << 0);
  }
  uart_rx_queue = xQueueCreate(10, sizeof(char));
}

bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(uart_rx_queue, input_byte, timeout);
}

void uart_lab__init(uart_number_e uart, uint32_t baud_rate) {
  const uint16_t divider_16_bit = 96 * 1000 * 1000 / (16 * baud_rate);
  const uint8_t DLAB_bit = (1U << 7);
  if (uart == UART_2) {
    LPC_SC->PCONP |= (1U << 24);
    LPC_UART2->LCR |= DLAB_bit;
    LPC_UART2->DLM = (divider_16_bit >> 8) & 0xFF;
    LPC_UART2->DLL = (divider_16_bit >> 0) & 0xFF;
    LPC_UART2->LCR &= ~DLAB_bit;
    LPC_UART2->LCR = 3;
  }

  else if (uart == UART_3) {
    LPC_SC->PCONP |= (1U << 25);
    LPC_UART3->LCR |= DLAB_bit;
    LPC_UART3->DLM = (divider_16_bit >> 8) & 0xFF;
    LPC_UART3->DLL = (divider_16_bit >> 0) & 0xFF;
    LPC_UART3->LCR &= ~DLAB_bit;
    LPC_UART3->LCR = 3;
  }
}

bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  bool data_available = false;
  const uint8_t rdr_empty = (1U << 0);
  if (uart == UART_2) {
    while (!(LPC_UART2->LSR & rdr_empty)) {
      // do nothing
    }
    data_available = true;
    *input_byte = LPC_UART2->RBR;
  } else if (uart == UART_3) {
    while (!(LPC_UART3->LSR & rdr_empty)) {
      // do nothing
    }
    data_available = true;
    *input_byte = LPC_UART3->RBR;
  }
  return data_available;
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  bool empty = false;
  const uint8_t thr_empty = (1U << 5);
  if (uart == UART_2) {
    while (!(LPC_UART2->LSR & thr_empty)) {
      // do nothing
    }
    empty = true;
    LPC_UART2->THR = output_byte;
  } else if (uart == UART_3) {
    while (!(LPC_UART3->LSR & thr_empty)) {
      // do nothing
    }
    empty = true;
    LPC_UART3->THR = output_byte;
  }
  return empty;
}