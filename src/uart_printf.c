#include "uart_printf.h"

#include <stdio.h>

#define TX_BUFFER_SIZE 256

static char tx_buffer[TX_BUFFER_SIZE];
static volatile unsigned int head = 0;
static volatile unsigned int tail = 0;
static volatile char tx_idle = 1;

static inline int buffer_empty(void) {
  return head == tail;
}

static inline int buffer_full(void) {
  return ((head + 1) % TX_BUFFER_SIZE) == tail;
}

static void buffer_put(char c) {
  while (buffer_full());
  tx_buffer[head] = c;
  head = (head + 1) % TX_BUFFER_SIZE;
}

void uart_putchar(char c) {
  if (c == '\n') {
      buffer_put('\r');
  }
  buffer_put(c);

  if (tx_idle) {
    tx_idle = 0;
    uart_tx_start();
  }
}

void uart_puts(const char* str) {
  while (*str) {
    uart_putchar(*str++);
  }
}

void uart_printf(const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  uart_puts(buf);
}

void uart_tx_irq_handler(void) {
  if (buffer_empty()) {
    tx_idle = 1;
    return;
  }

  char c = tx_buffer[tail];
  tail = (tail + 1) % TX_BUFFER_SIZE;

  uart_write_byte(c);
}
