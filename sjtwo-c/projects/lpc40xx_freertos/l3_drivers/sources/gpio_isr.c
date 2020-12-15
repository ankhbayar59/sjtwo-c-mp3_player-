#include "gpio_isr.h"

static function_pointer_t gpio0_re_callbacks[32];
static function_pointer_t gpio0_fe_callbacks[32];
static function_pointer_t gpio2_re_callbacks[32];
static function_pointer_t gpio2_fe_callbacks[32];

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (interrupt_type) {
    LPC_GPIOINT->IO0IntEnR |= (1U << pin);
    gpio0_re_callbacks[pin] = callback;
  } else {
    LPC_GPIOINT->IO0IntEnF |= (1U << pin);
    gpio0_fe_callbacks[pin] = callback;
  }
}

void gpio2__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (interrupt_type) {
    LPC_GPIOINT->IO2IntEnR |= (1U << pin);
    gpio2_re_callbacks[pin] = callback;
  } else {
    LPC_GPIOINT->IO2IntEnF |= (1U << pin);
    gpio2_fe_callbacks[pin] = callback;
  }
}

/*
void gpio1__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (interrupt_type) {
    LPC_GPIOINT->IO0IntEnR |= (1U << pin);
    gpio1_re_callbacks[pin] = callback;
  }
}
*/

pin_type find_pin_interrupt_generated(void) {
  pin_type pin;
  int i = 0;
  while (i < 32) {
    if (LPC_GPIOINT->IO0IntStatR & (1U << i)) {
      pin.pin_num = i;
      pin.configure = 1;
      break;
    } else if (LPC_GPIOINT->IO0IntStatF & (1U << i)) {
      pin.pin_num = i;
      pin.configure = 0;
      break;
    } else {
      i += 1;
    }
  }
  return pin;
}

pin_type find_pin_interrupt_generated_gpio2(void) {
  pin_type pin;
  int i = 0;
  while (i < 32) {
    if (LPC_GPIOINT->IO2IntStatR & (1U << i)) {
      pin.pin_num = i;
      pin.configure = 1;
      break;
    } else if (LPC_GPIOINT->IO2IntStatF & (1U << i)) {
      pin.pin_num = i;
      pin.configure = 0;
      break;
    } else {
      i += 1;
    }
  }
  return pin;
}

void clear_pin_interrupt(int pin_generated) { LPC_GPIOINT->IO0IntClr = (1U << pin_generated); }
void clear_pin_interrupt_gpio2(int pin_generated) { LPC_GPIOINT->IO2IntClr = (1U << pin_generated); }

void gpio0__interrupt_dispatcher(void) {

  pin_type pin_that_generated_interrupt = find_pin_interrupt_generated();

  if (pin_that_generated_interrupt.configure) {
    function_pointer_t attached_user_handler = gpio0_re_callbacks[pin_that_generated_interrupt.pin_num];
    attached_user_handler();
  } else {
    function_pointer_t attached_user_handler = gpio0_fe_callbacks[pin_that_generated_interrupt.pin_num];
    attached_user_handler();
  }
  clear_pin_interrupt(pin_that_generated_interrupt.pin_num);
}

void gpio2__interrupt_dispatcher(void) {

  pin_type pin_that_generated_interrupt = find_pin_interrupt_generated_gpio2();

  if (pin_that_generated_interrupt.configure) {
    function_pointer_t attached_user_handler = gpio2_re_callbacks[pin_that_generated_interrupt.pin_num];
    attached_user_handler();
  } else {
    function_pointer_t attached_user_handler = gpio2_fe_callbacks[pin_that_generated_interrupt.pin_num];
    attached_user_handler();
  }
  clear_pin_interrupt_gpio2(pin_that_generated_interrupt.pin_num);
}