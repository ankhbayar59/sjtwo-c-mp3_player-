#include "gpio_lab.h"
#include "lpc40xx.h"

void gpio1__set_as_input(uint8_t pin_num) {
  const uint32_t pin = (1U << pin_num);
  LPC_GPIO1->DIR &= ~(pin);
}

void all_gpio__set_as_input(uint8_t pin_num, uint8_t port_num) {
  const uint32_t pin = (1U << pin_num);
  switch (port_num) {
  case 0:
    LPC_GPIO0->DIR &= ~(pin);
    break;
  case 1:
    LPC_GPIO1->DIR &= ~(pin);
    break;
  case 2:
    LPC_GPIO2->DIR &= ~(pin);
    break;
  case 3:
    LPC_GPIO3->DIR &= ~(pin);
    break;
  case 4:
    LPC_GPIO4->DIR &= ~(pin);
    break;
  case 5:
    LPC_GPIO5->DIR &= ~(pin);
    break;
  }
}

void gpio1__set_as_output(uint8_t pin_num) {
  const uint32_t pin = (1U << pin_num);
  LPC_GPIO1->DIR != (pin);
}

void all_gpio__set_as_output(uint8_t pin_num, uint8_t port_num) {
  const uint32_t pin = (1U << pin_num);
  switch (port_num) {
  case 0:
    LPC_GPIO0->DIR |= (pin);
    break;
  case 1:
    LPC_GPIO1->DIR |= (pin);
    break;
  case 2:
    LPC_GPIO2->DIR |= (pin);
    break;
  case 3:
    LPC_GPIO3->DIR |= (pin);
    break;
  case 4:
    LPC_GPIO4->DIR |= (pin);
    break;
  case 5:
    LPC_GPIO5->DIR |= (pin);
    break;
  }
}

void gpio1__set_high(uint8_t pin_num) {
  const uint32_t pin = (1U << pin_num);
  LPC_GPIO1->SET = pin;
}

void all_gpio__set_high(uint8_t pin_num, uint8_t port_num) {
  const uint32_t pin = (1U << pin_num);
  switch (port_num) {
  case 0:
    LPC_GPIO0->SET = (pin);
    break;
  case 1:
    LPC_GPIO1->SET = (pin);
    break;
  case 2:
    LPC_GPIO2->SET = (pin);
    break;
  case 3:
    LPC_GPIO3->SET = (pin);
    break;
  case 4:
    LPC_GPIO4->SET = (pin);
    break;
  case 5:
    LPC_GPIO5->SET = (pin);
    break;
  }
}

void gpio1__set_low(uint8_t pin_num) {
  const uint32_t pin = (1U << pin_num);
  LPC_GPIO1->CLR = pin;
}

void all_gpio__set_low(uint8_t pin_num, uint8_t port_num) {
  const uint32_t pin = (1U << pin_num);
  switch (port_num) {
  case 0:
    LPC_GPIO0->CLR = (pin);
    break;
  case 1:
    LPC_GPIO1->CLR = (pin);
    break;
  case 2:
    LPC_GPIO2->CLR = (pin);
    break;
  case 3:
    LPC_GPIO3->CLR = (pin);
    break;
  case 4:
    LPC_GPIO4->CLR = (pin);
    break;
  case 5:
    LPC_GPIO5->CLR = (pin);
    break;
  }
}

void gpio1__set(uint8_t pin_num, bool high) {
  const uint32_t pin = (1U << pin_num);
  if (high) {
    LPC_GPIO1->SET = pin;
  } else {
    LPC_GPIO1->CLR = pin;
  }
}

void all_gpio__set(uint8_t pin_num, uint8_t port_num, bool high) {
  const uint32_t pin = (1U << pin_num);
  if (high) {
    switch (port_num) {
    case 0:
      LPC_GPIO0->SET = (pin);
      break;
    case 1:
      LPC_GPIO1->SET = (pin);
      break;
    case 2:
      LPC_GPIO2->SET = (pin);
      break;
    case 3:
      LPC_GPIO3->SET = (pin);
      break;
    case 4:
      LPC_GPIO4->SET = (pin);
      break;
    case 5:
      LPC_GPIO5->SET = (pin);
      break;
    }
  } else {
    switch (port_num) {
    case 0:
      LPC_GPIO0->CLR = (pin);
      break;
    case 1:
      LPC_GPIO1->CLR = (pin);
      break;
    case 2:
      LPC_GPIO2->CLR = (pin);
      break;
    case 3:
      LPC_GPIO3->CLR = (pin);
      break;
    case 4:
      LPC_GPIO4->CLR = (pin);
      break;
    case 5:
      LPC_GPIO5->CLR = (pin);
      break;
    }
  }
}

bool gpio1__get_level(uint8_t pin_num) {
  bool state = false;
  const uint32_t pin = (1U << pin_num);
  if (LPC_GPIO1->PIN & (pin)) {
    state = true;
  }
  return state;
}

bool all_gpio__get_level(uint8_t pin_num, uint8_t port_num) {
  bool state = false;
  const uint32_t pin = (1U << pin_num);
  switch (port_num) {
  case 0:
    if (LPC_GPIO0->PIN & (pin)) {
      state = true;
    }
    break;
  case 1:
    if (LPC_GPIO1->PIN & (pin)) {
      state = true;
    }
    break;
  case 2:
    if (LPC_GPIO2->PIN & (pin)) {
      state = true;
    }
    break;
  case 3:
    if (LPC_GPIO3->PIN & (pin)) {
      state = true;
    }
    break;
  case 4:
    if (LPC_GPIO4->PIN & (pin)) {
      state = true;
    }
    break;
  case 5:
    if (LPC_GPIO5->PIN & (pin)) {
      state = true;
    }
    break;
  }
  return state;
}